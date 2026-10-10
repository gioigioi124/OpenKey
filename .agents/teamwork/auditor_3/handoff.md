# Forensic Audit & Integrity Verification Report (auditor_3)

**Work Product**: OpenKey Win32 Milestone 3 (Parent / Root Owner Window Tracing for UserForm & Child Dialogs)  
**Profile**: General Project  
**Integrity Mode**: Development (from `ORIGINAL_REQUEST.md:91`)  
**Auditor**: Forensic Integrity Auditor (`auditor_3`)  
**Verdict**: **CLEAN**

---

## 1. Observation

### 1.1. Codebase Scope and Modifications
The audit examined git working tree modifications under `Sources/` and `tests/`:
- `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKeyHelper.h`:
  - Added prototypes:
    - Line 32: `static string getWindowTitleUtf8(HWND hwnd);`
    - Line 33: `static HWND getProcessRootOwner(HWND hwnd);`
- `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKeyHelper.cpp`:
  - Lines 158-168: `getWindowTitleUtf8(HWND hwnd)` - generalized UTF-8 title retrieval from any `HWND` using `GetWindowTextW` and `WideCharToMultiByte(CP_UTF8, ...)`.
  - Lines 170-172: `getFrontMostWindowTitleUtf8()` - delegates to `getWindowTitleUtf8(GetForegroundWindow())`.
  - Lines 174-213: `getProcessRootOwner(HWND hwnd)` - authentic hierarchical traversal:
    - Verifies valid window and non-zero PID (`GetWindowThreadProcessId`).
    - Priority 1: `GetAncestor(hwnd, GA_ROOTOWNER)` with cross-process check (`rootPid == targetPid`) and desktop filter.
    - Priority 2: Safe iteration over `GetWindow(cur, GW_OWNER)` / `GetParent(cur)` bounded by `depth < 10`, cycle check (`next == cur`), and PID boundary check (`nextPid != targetPid`).
- `Sources/OpenKey/win32/OpenKey/OpenKey/ProcessRuleHelper.h`:
  - Lines 30-31: Added `getCodeTableForTitleOnly` and `getCodeTableForWindow(HWND hwnd, const std::string& exeName)`.
- `Sources/OpenKey/win32/OpenKey/OpenKey/ProcessRuleHelper.cpp`:
  - Lines 230-259: `getCodeTableForTitleOnly(exeName, windowTitle)` - evaluates Pass 1 (process + title) and Pass 2 (title only).
  - Lines 261-278: `getCodeTableForProcess(exeName)` - evaluates Pass 3 (process only).
  - Lines 280-284: `getCodeTableForProcessAndTitle` - calls title-only first, then falls back to process-level.
  - Lines 286-342: `getCodeTableForWindow(HWND hwnd, const std::string& exeName)`:
    - Priority 1: Child window title check (`getCodeTableForTitleOnly(exeName, childTitle)`).
    - Priority 2a: Immediate owner (`GW_OWNER` / `GetParent`) matching child PID.
    - Priority 2b: Root owner (`getProcessRootOwner`) matching child PID.
    - Priority 3: Process-level rules (`getCodeTableForProcess(exeName)`).
    - Priority 4: Returns `-1` if no match found.
- `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp`:
  - Lines 726-743: `winEventProcCallback` invokes `ProcessRuleHelper::getCodeTableForWindow(hActiveWnd, exe)`.
  - Encoding change synchronized centrally via `AppDelegate::getInstance()->onTableCode(ruleCode)` and `SystemTrayHelper::updateData()`.
  - Fallback to Unicode (`vFallbackToUnicode`) is isolated and triggered only when `ruleCode == -1`.

### 1.2. Static Anti-Cheat & Forensic Scan
- Scanned `Sources/` for hardcoded strings:
  - `"UserForm"`: 0 occurrences in `Sources/`.
  - `"a.xlsx"` / `".xlsx"`: 0 occurrences in `Sources/`.
  - `"Tong hop ban hang"`: 0 occurrences in `Sources/`.
- Scanned workspace for pre-populated logs/results (`*.log`, `*result*`, `*output*`): 0 files found.
- No dummy facades or stub functions returning constant test values.

### 1.3. Build Execution
- Command executed:
  ```cmd
  cmd.exe /c "set PATH=C:\llvm-mingw\bin;C:\w64devkit\bin;%PATH% && build.bat"
  ```
- Result: **Exit Code 0**
- Verbatim Output:
  ```
  ==============================================
    Building OpenKey with UTF-8 / Vietnamese UI
  ==============================================
  [1/3] Compiling resources with codepage 65001...
  [2/3] Compiling C++ sources...
  [3/3] Linking OpenKey.exe...
  Cleaning intermediate files...
          1 file(s) copied.
  ==============================================
    Build Successful OpenKey.exe is updated.
  ==============================================
  ```
- Binary metadata:
  - `C:\Users\03102025\Desktop\OpenKey\OpenKey.exe`: 1,372,672 bytes, LastWriteTime: 10/10/2026 9:53:45 AM
  - `C:\Users\03102025\Desktop\OpenKey\Sources\OpenKey\win32\OpenKey\OpenKey\OpenKey.exe`: 1,372,672 bytes, LastWriteTime: 10/10/2026 9:53:45 AM
  - SHA256: `B820256BAD98A248457752F1E860EB6BD6657EC15F47C220BCA2CF92D540938E` (identical match across root and build folder).

### 1.4. Independent Test Executions
1. `tests/test_parent_window_rule.cpp`:
   - Executed via:
     ```cmd
     clang++ -std=c++14 -O2 -D_WIN32 -DUNICODE -D_UNICODE -I Sources/OpenKey/win32/OpenKey/OpenKey -I Sources/OpenKey/engine tests/test_parent_window_rule.cpp Sources/OpenKey/win32/OpenKey/OpenKey/OpenKeyHelper.cpp Sources/OpenKey/win32/OpenKey/OpenKey/ProcessRuleHelper.cpp Sources/OpenKey/win32/OpenKey/OpenKey/stdafx.cpp Sources/OpenKey/engine/Engine.cpp Sources/OpenKey/engine/Vietnamese.cpp Sources/OpenKey/engine/Macro.cpp Sources/OpenKey/engine/ConvertTool.cpp -lcomctl32 -lversion -lurlmon -lpsapi -lshell32 -lole32 -luxtheme -luuid -o tests/test_parent_window_rule.exe && .\tests\test_parent_window_rule.exe
     ```
   - Result: **10 PASSED, 0 FAILED**.
   - Performance benchmark: 100,000 resolutions in 82.34 ms (0.82 microseconds per resolution).
2. `tests/test_lazy_macro.cpp`:
   - Executed via:
     ```cmd
     clang++ -std=c++14 -O2 -I Sources/OpenKey/engine tests/test_lazy_macro.cpp Sources/OpenKey/engine/Engine.cpp Sources/OpenKey/engine/Macro.cpp Sources/OpenKey/engine/Vietnamese.cpp Sources/OpenKey/engine/ConvertTool.cpp -o tests/test_lazy_macro.exe && .\tests\test_lazy_macro.exe
     ```
   - Result: **14 PASSED, 0 FAILED**.
3. `tests/test_auditor_independent.cpp`:
   - Executed via:
     ```cmd
     clang++ -std=c++14 -O2 -I Sources/OpenKey/engine tests/test_auditor_independent.cpp Sources/OpenKey/engine/Engine.cpp Sources/OpenKey/engine/Macro.cpp Sources/OpenKey/engine/Vietnamese.cpp Sources/OpenKey/engine/ConvertTool.cpp -o tests/test_auditor_independent.exe && .\tests\test_auditor_independent.exe
     ```
   - Result: **5 PASSED, 0 FAILED**.

### 1.5. Documentation and Acceptance Criteria
- `CHANGELOG.md`: Section `Version 2.2.0: (10/10/2026)` documents parent window tracing, userform encoding protection, rule hierarchy, process boundary isolation, and latency metrics.
- `DOCS_AUTO_ENCODING_AND_HOTKEY.md`: Section 6 documents technical diagnosis, 3-tier hierarchy architecture, safety mechanisms, 3-agent distribution, and test matrices.
- Acceptance criteria in `ORIGINAL_REQUEST.md:85-126`: All 7 criteria verified and satisfied.

---

## 2. Logic Chain

1. **Premise 1 (Anti-cheat & Generality)**: Observations 1.1 and 1.2 demonstrate that the new implementation does not use hardcoded application names (`excel.exe`), form titles (`UserForm1`), or mock shortcuts in `Sources/`. The window traversal algorithm uses standard Win32 APIs (`GetAncestor`, `GetWindow`, `GetParent`, `GetWindowThreadProcessId`) that operate uniformly across any Win32 process (Excel, Word, CAD, accounting tools).
2. **Premise 2 (Functional Correctness)**: Observation 1.4 confirms that in real Win32 window hierarchies created with `CreateWindowExW`, child forms inherit owner encoding (TC-02, TC-03), child-specific rules override parent rules (TC-04), untitled windows safely trace owners (TC-05), unmatched windows allow fallback (TC-06), deep hierarchies terminate cleanly without loops (TC-08), and PID boundaries cannot be breached (TC-10).
3. **Premise 3 (Performance & Non-Interference)**: Observation 1.4 measures average resolution latency at ~0.82 microseconds. Resolution occurs strictly during `winEventProcCallback` (window activation / name change), leaving `keyboardHookProcess` untouched.
4. **Premise 4 (Regression Prevention)**: Observations 1.4 (test 2 and test 3) prove that lazy macro expansion across all 5 code tables, AutoCaps, and O(1) table switching remain 100% functional.
5. **Premise 5 (Completeness)**: Observations 1.3 and 1.5 show that `build.bat` compiles cleanly to produce `OpenKey.exe`, and documentation thoroughly reflects the architecture.
6. **Deduction**: Therefore, the work product satisfies all functional and non-functional requirements without integrity violations.

---

## 3. Caveats

- **Multi-Process Child Architectures**: If a third-party application hosts child UI windows in separate processes with different PIDs (e.g., Chromium out-of-process iframes), PID isolation will safely stop traversal at the process boundary. For standard Windows desktop applications including Microsoft Office (Excel, Word) and VBA forms, all windows share the same PID, so traversal operates as intended.
- **No other caveats**: All observations were verified directly through command execution and source code review.

---

## 4. Conclusion

- **Audit Verdict**: **CLEAN**.
- The implementation of Milestone 3 (Parent / Root Owner Window Tracing for UserForms and Child Dialogs) is authentic, robust, high-performing, and fully verified.
- The work product is **ACCEPTED**.

---

## 5. Verification Method

To reproduce and verify the audit findings:

1. **Rebuild the project**:
   ```cmd
   cmd.exe /c "set PATH=C:\llvm-mingw\bin;C:\w64devkit\bin;%PATH% && build.bat"
   ```
   *Expected*: Exit code 0, binary updated.

2. **Run the parent window hierarchy test suite**:
   ```cmd
   cmd.exe /c "set PATH=C:\llvm-mingw\bin;C:\w64devkit\bin;%PATH% && clang++ -std=c++14 -O2 -D_WIN32 -DUNICODE -D_UNICODE -I Sources/OpenKey/win32/OpenKey/OpenKey -I Sources/OpenKey/engine tests/test_parent_window_rule.cpp Sources/OpenKey/win32/OpenKey/OpenKey/OpenKeyHelper.cpp Sources/OpenKey/win32/OpenKey/OpenKey/ProcessRuleHelper.cpp Sources/OpenKey/win32/OpenKey/OpenKey/stdafx.cpp Sources/OpenKey/engine/Engine.cpp Sources/OpenKey/engine/Vietnamese.cpp Sources/OpenKey/engine/Macro.cpp Sources/OpenKey/engine/ConvertTool.cpp -lcomctl32 -lversion -lurlmon -lpsapi -lshell32 -lole32 -luxtheme -luuid -o tests/test_parent_window_rule.exe && .\tests\test_parent_window_rule.exe"
   ```
   *Expected*: 10 PASSED, 0 FAILED.

3. **Run the lazy macro regression test suite**:
   ```cmd
   cmd.exe /c "set PATH=C:\llvm-mingw\bin;C:\w64devkit\bin;%PATH% && clang++ -std=c++14 -O2 -I Sources/OpenKey/engine tests/test_lazy_macro.cpp Sources/OpenKey/engine/Engine.cpp Sources/OpenKey/engine/Macro.cpp Sources/OpenKey/engine/Vietnamese.cpp Sources/OpenKey/engine/ConvertTool.cpp -o tests/test_lazy_macro.exe && .\tests\test_lazy_macro.exe"
   ```
   *Expected*: 14 PASSED, 0 FAILED.

4. **Run the auditor independent test suite**:
   ```cmd
   cmd.exe /c "set PATH=C:\llvm-mingw\bin;C:\w64devkit\bin;%PATH% && clang++ -std=c++14 -O2 -I Sources/OpenKey/engine tests/test_auditor_independent.cpp Sources/OpenKey/engine/Engine.cpp Sources/OpenKey/engine/Macro.cpp Sources/OpenKey/engine/Vietnamese.cpp Sources/OpenKey/engine/ConvertTool.cpp -o tests/test_auditor_independent.exe && .\tests\test_auditor_independent.exe"
   ```
   *Expected*: 5 PASSED, 0 FAILED.
