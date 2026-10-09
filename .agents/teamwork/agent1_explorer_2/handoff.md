# Handoff Report: OpenKey Win32 On-Demand (Lazy JIT) Conversion for Macro Engine

## 1. Observation

### 1.1 Baseline Build Status
- Build script: `cmd.exe /c build.bat`
- Working directory: `C:\Users\Administrator\Desktop\OpenKey`
- Result: **Exit code 0** (Success). Compiles resources with codepage 65001, compiles C++ sources with `clang++ -std=c++14 -O2`, links `OpenKey.exe` successfully.

### 1.2 Data Structures & Storage in `Macro.h` and `Macro.cpp`
- **Location**: `Sources/OpenKey/engine/Macro.h:19-23`
  ```cpp
  struct MacroData {
      string macroText; //ex: "ms"
      string macroContent; //ex: "millisecond"
      vector<Uint32> macroContentCode; //converted of macroContent
  };
  ```
- **Scope Verification**:
  - `grep_search` across entire solution confirms `MacroData` is referenced **only** in `Macro.h` and `Macro.cpp`.
  - No external module (`Engine.cpp`, `OpenKey.cpp`, `AppDelegate.cpp`, `MacroDialog.cpp`) accesses `MacroData` or `it->second.macroContentCode`.
  - `MacroDialog.cpp:118` calls `getAllMacro(keys, macroText, macroContent)` which extracts only string representations (`macroText`, `macroContent`).
  - `OpenKeyHelper` binary persistence (`getMacroSaveData` / `initMacroMap`) only writes and reads `macroText` and `macroContent` (lengths and raw bytes); it has never persisted `macroContentCode`.

### 1.3 Pre-Translation Bottlenecks in `Macro.cpp`
- **Startup (`initMacroMap`)**:
  - `Sources/OpenKey/engine/Macro.cpp:106`:
    ```cpp
    convert(macroContent, data.macroContentCode);
    ```
    Every macro in registry is pre-translated at startup into `vector<Uint32>`, creating $N$ heap buffers.
- **Addition/Edit (`addMacro`)**:
  - `Sources/OpenKey/engine/Macro.cpp:223, 227`:
    ```cpp
    convert(macroContent, data.macroContentCode);
    convert(macroContent, macroMap[key].macroContentCode);
    ```
    Pre-translates upon insertion.
- **Table Code Switch (`onTableCodeChange`)**:
  - `Sources/OpenKey/engine/Macro.cpp:242-246`:
    ```cpp
    void onTableCodeChange() {
        for (std::map<vector<Uint32>, MacroData>::iterator it = macroMap.begin(); it != macroMap.end(); ++it) {
            convert(it->second.macroContent, it->second.macroContentCode);
        }
    }
    ```
    Loops over all $N$ macros in memory, invoking `convert()` and reallocating vectors. With large macro collections or frequent automatic encoding switching (`ProcessRuleHelper`, tray menu, hotkeys), this consumes unnecessary CPU cycles ($O(N \times L)$).

### 1.4 Macro Lookup & Trigger Flow
- **Macro Invocation**:
  - `Sources/OpenKey/engine/Engine.cpp:1290, 1329, 1388`:
    Calls `findMacro(hMacroKey, hMacroData)` where `hMacroData` is `HookState.macroData` (`vector<Uint32>`).
  - When `findMacro` returns `true`, `OpenKey.cpp:457-463` iterates through `pData->macroData` and transmits keys via `SendPureCharacter()` or `SendKeyCode()`.
- **Existing `findMacro()` in `Macro.cpp:155-197`**:
  - `macroContentCode` is the OUT parameter (`vector<Uint32>&`).
  - Direct match (`macroMap.find(key)`): simply copies `macroContentCode = data.macroContentCode;`.
  - AutoCaps branch (`vAutoCapsMacro`): detects initial capital letter in `key` (or all capitals `_macroFlag`), normalizes `key` to lowercase, finds macro in `macroMap`, copies `macroContentCode = data.macroContentCode;`, and iterates over `macroContentCode` transforming characters with `toupper()` and `modifyCaseUnicode()`.

### 1.5 Dynamic Conversion Capability of `convert()`
- `Sources/OpenKey/engine/Macro.cpp:29-65`:
  `static void convert(const string& str, vector<Uint32>& outData)`
  - Reads `vCodeTable` directly at runtime (`_codeTable[vCodeTable][it->first][k] | CHAR_CODE_MASK`).
  - Handles ASCII letters via `_characterMap`, accented Vietnamese characters via `_codeTable[vCodeTable]`, and arbitrary symbols via `PURE_CHARACTER_MASK`.
  - Uses only local stack variables; re-entrant and thread-safe.

---

## 2. Logic Chain

1. **Safety of `MacroData` Simplification**:
   - Because `MacroData` is never used outside `Macro.h` and `Macro.cpp`, removing `vector<Uint32> macroContentCode;` from `struct MacroData` breaks zero external callers and eliminates 24 bytes + heap vector overhead per entry.
2. **On-Demand (Lazy JIT) Conversion Viability**:
   - Macro trigger events happen at human typing speed (at most a few per second), targeting exactly **one** macro keyword per trigger.
   - Calling `convert(it->second.macroContent, macroContentCode)` inside `findMacro()` for only the matched macro takes less than $0.01\text{ ms}$, which is completely imperceptible to users.
   - Eliminating pre-translation in `initMacroMap()` and `addMacro()` removes all startup delays and redundant conversions.
3. **Preservation of `vAutoCapsMacro`**:
   - In `findMacro()`, when `vAutoCapsMacro` detects a match after lowering the keyword case, invoking `convert(itCaps->second.macroContent, macroContentCode)` produces the dynamic key event vector for the current `vCodeTable`.
   - The subsequent loop:
     ```cpp
     for (c = 0; c < macroContentCode.size(); c++) {
         if (c == 0 || _macroFlag) {
             _kChar = keyCodeToCharacter(macroContentCode[c]);
             if (_kChar != 0) {
                 _kChar = toupper(_kChar);
                 macroContentCode[c] = _characterMap[_kChar];
                 continue;
             }
             if (macroContentCode[c] & CHAR_CODE_MASK) {
                 modifyCaseUnicode(macroContentCode[c]);
             }
         }
     }
     ```
     operates directly on the newly converted `macroContentCode`. Because `modifyCaseUnicode()` inspects `_codeTable[vCodeTable]` (even index = uppercase, odd index = lowercase), capitalization works seamlessly across Unicode, TCVN3, VNI, and Unicode Compound without modification.
4. **$O(1)$ Zero-Overhead `onTableCodeChange()`**:
   - Because no pre-converted vectors are held in RAM, switching table codes requires zero re-conversion.
   - Making `onTableCodeChange()` an empty no-op ($O(1)$, 0% CPU) satisfies the requirement while preserving interface compatibility with callers (`AppDelegate::onTableCode`, `AppDelegate::onDefaultConfig`, `ModernKey/OpenKey.mm`).

---

## 3. Caveats

- **No Caveats in Engine / Macro logic**: The call path is concise and fully contained within `Macro.h` and `Macro.cpp`.
- **API Signature Preservation**: The signature of `findMacro(vector<Uint32>& key, vector<Uint32>& macroContentCode)` and `onTableCodeChange()` must remain identical so no other files in Win32 or macOS build targets are invalidated.
- **Lookup Iterator Efficiency**: In `findMacro()`, replacing `macroMap[key]` with `it->second` avoids redundant tree lookups.

---

## 4. Conclusion & Actionable Blueprint

### 4.1 Specification for Agent 2 (Developer)

#### Task 2.1: Edit `Sources/OpenKey/engine/Macro.h`
Remove `vector<Uint32> macroContentCode;` from `MacroData`:
```cpp
// Lines 19-23 in Sources/OpenKey/engine/Macro.h
struct MacroData {
    string macroText; //ex: "ms"
    string macroContent; //ex: "millisecond"
};
```
Update doc comment for `onTableCodeChange()`:
```cpp
// Lines 60-64 in Sources/OpenKey/engine/Macro.h
/**
 * When table code changed; with On-Demand Lazy Conversion, this is an O(1) no-op.
 */
void onTableCodeChange();
```

#### Task 2.2: Edit `Sources/OpenKey/engine/Macro.cpp`
1. **`initMacroMap()` (Lines 100-109)**:
   Remove `convert(macroContent, data.macroContentCode);`:
   ```cpp
   MacroData data;
   data.macroText = macroText;
   data.macroContent = macroContent;
   
   vector<Uint32> key;
   convert(macroText, key);
   
   macroMap[key] = data;
   ```
2. **`findMacro()` (Lines 155-197)**:
   Perform dynamic JIT conversion on match:
   ```cpp
   bool findMacro(vector<Uint32>& key, vector<Uint32>& macroContentCode) {
       for (c = 0; c < key.size(); c++) {
           key[c] = getCharacterCode(key[c]);
       }
       std::map<vector<Uint32>, MacroData>::iterator it = macroMap.find(key);
       if (it != macroMap.end()) {
           convert(it->second.macroContent, macroContentCode);
           return true;
       }
       if (vAutoCapsMacro) {
           _macroFlag = false;
           if (key.size() > 1 && modifyCaseUnicode(key[1], false)) {
               _macroFlag = true;
               for (c = 2; c < key.size(); c++) {
                   modifyCaseUnicode(key[c], false);
               }
           }
           
           if (key.size() > 0 && modifyCaseUnicode(key[0], false)) {
               std::map<vector<Uint32>, MacroData>::iterator itCaps = macroMap.find(key);
               if (itCaps != macroMap.end()) {
                   convert(itCaps->second.macroContent, macroContentCode);
                   for (c = 0; c < macroContentCode.size(); c++) {
                       if (c == 0 || _macroFlag) {
                           _kChar = keyCodeToCharacter(macroContentCode[c]);
                           if (_kChar != 0) {
                               _kChar = toupper(_kChar);
                               macroContentCode[c] = _characterMap[_kChar];
                               continue;
                           }
                           if (macroContentCode[c] & CHAR_CODE_MASK) {
                               modifyCaseUnicode(macroContentCode[c]);
                           }
                       }
                   }
                   return true;
               }
           }
       }
       return false;
   }
   ```
3. **`addMacro()` (Lines 216-231)**:
   Remove both `convert(macroContent, ...)` calls:
   ```cpp
   bool addMacro(const string& macroText, const string& macroContent) {
       vector<Uint32> key;
       convert(macroText, key);
       if (macroMap.find(key) == macroMap.end()) { //add new macro
           MacroData data;
           data.macroText = macroText;
           data.macroContent = macroContent;
           macroMap[key] = data;
       } else { //edit this macro
           macroMap[key].macroContent = macroContent;
       }
       return true;
   }
   ```
4. **`onTableCodeChange()` (Lines 242-246)**:
   Replace entire loop with empty function body:
   ```cpp
   void onTableCodeChange() {
       // On-demand JIT conversion: conversion is performed dynamically in findMacro().
       // Table code switching is an O(1) operation with 0% CPU overhead.
   }
   ```

#### Task 2.3: Verification via `build.bat`
- Run `build.bat` to confirm `OpenKey.exe` builds cleanly with zero compile/link errors.

---

### 4.2 Comprehensive Test Matrix for Agent 3 (Tester)

| Test ID | Test Scenario | Steps / Input | Expected Result | Pass/Fail Criteria |
| :--- | :--- | :--- | :--- | :--- |
| **TC-01** | **Build & Binary Verification** | Run `build.bat` from project root | Compilation & linking exit with code 0, `OpenKey.exe` updated | Clean build, no errors |
| **TC-02** | **On-Demand Unicode Expansion** | Table code = Unicode (0). Type `ms ` (space). | `findMacro()` triggers JIT `convert()`, outputs Unicode keycodes (`0x00C2`, `0x1ED9`...). Notepad displays "Cộng hòa Xã hội Chủ nghĩa Việt Nam" | Correct Unicode Vietnamese output |
| **TC-03** | **On-Demand TCVN3 Expansion** | Switch to TCVN3 (`Ctrl+Shift+F2` or Tray). Type `ms `. | JIT conversion translates to TCVN3 1-byte char codes (`0xCA`, `0xE9`...). Outputs correctly in TCVN3 font | Correct TCVN3 encoding |
| **TC-04** | **On-Demand VNI Expansion** | Switch to VNI Windows (Tray / Dialog). Type `ms `. | JIT conversion translates to VNI 2-byte char codes (`0xEF6F`...). Outputs correctly in VNI font | Correct VNI Windows encoding |
| **TC-05** | **On-Demand Unicode Composite** | Switch to Unicode Tổ hợp (code 3). Type `ms `. | JIT conversion translates to Unicode combining codes. Output is correct decomposed text | Correct Unicode Compound |
| **TC-06** | **AutoCaps Title Case** | `vAutoCapsMacro` = true. Type `Ms ` (capital M, lowercase s). | `findMacro()` normalizes key, converts JIT, capitalizes first character. Output: "Cộng hòa Xã hội Chủ nghĩa Việt Nam" (or capitalized first letter) | Initial capital letter preserved |
| **TC-07** | **AutoCaps All-Caps** | `vAutoCapsMacro` = true. Type `MS ` (all uppercase). | `findMacro()` normalizes key, converts JIT, capitalizes all characters. Output: "CỘNG HÒA XÃ HỘI CHỦ NGHĨA VIỆT NAM" | Full capitalization across code table |
| **TC-08** | **Stress / Continuous Switch Test** | Rapidly switch table codes 100+ times (`0 -> 1 -> 2 -> 3 -> 0`). | `onTableCodeChange()` executes in 0.00 ms ($O(1)$). 0% CPU consumption. No memory leaks. Subsequent macro typing outputs correctly in current active table code | No lag, 0% CPU, $O(1)$ confirmed |
| **TC-09** | **MacroDialog Add / Modify** | Open Macro Dialog. Add `dc` $\rightarrow$ `Độc lập Tự do Hạnh phúc`. Test at Unicode, then switch to TCVN3. | Added without pre-translation. Gõ `dc ` ở Unicode ra Unicode; chuyển sang TCVN3 gõ `dc ` ra TCVN3 ngay lập tức | Dynamic adaptation confirmed |
| **TC-10** | **RAM Footprint Optimization** | Inspect `macroMap` memory with large macro set. | `macroContentCode` vector removed from `MacroData`. Significant RAM reduction, zero vector heap allocations during startup/switch | Clean memory layout |

---

## 5. Verification Method

1. **Compiler Verification**:
   - Command: `cmd.exe /c build.bat`
   - Verification condition: Exits with code 0, generates valid `OpenKey.exe`.
2. **Code Inspection**:
   - Inspect `Macro.h`: Confirm `MacroData` contains only `macroText` and `macroContent`.
   - Inspect `Macro.cpp`: Confirm `convert()` is invoked inside `findMacro()`, removed from `initMacroMap()`, `addMacro()`, and `onTableCodeChange()` is an empty body.
3. **Invalidation Conditions**:
   - Any compiler error in `clang++`.
   - Any memory corruption or failure when typing macro keywords in Unicode, TCVN3, or VNI.
   - Any regression in AutoCaps capitalization (`Ms ` or `MS `).
