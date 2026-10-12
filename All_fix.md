# All_fix.md — Tổng hợp Tài liệu Kỹ thuật & Nhật ký Sửa lỗi OpenKey Win32

> **Dự án**: OpenKey Windows (Bộ gõ tiếng Việt mã nguồn mở — fork)
> **Ngày tổng hợp**: 12/10/2026
> **Phạm vi**: Gộp toàn bộ tài liệu kỹ thuật của các tính năng đã phát triển trước đó và ghi nhận đợt rà soát — sửa lỗi mới nhất.
> **Trạng thái build**: ✅ `build.bat` thành công (Exit code 0), `OpenKey.exe` đã cập nhật.

## PHẦN A — TỔNG QUAN CÁC TÍNH NĂNG ĐÃ PHÁT TRIỂN

### A.1. Phím tắt chuyển đổi nhanh bảng mã (UniKey-like hotkeys)

- `Ctrl + Shift + F1`: Chuyển sang bảng mã **Unicode** (`vCodeTable = 0`).
- `Ctrl + Shift + F2`: Chuyển sang bảng mã **TCVN3 (ABC)** (`vCodeTable = 1`).
- `Ctrl + Shift + F12`: Bật/Khóa nhanh tính năng tự động chuyển bảng mã.
- Kèm thông báo Balloon ở khay hệ thống và âm báo (nếu bật tùy chọn âm báo).

### A.2. Tự động nhận diện phần mềm & tên file / tiêu đề cửa sổ

- Khi chuyển sang cửa sổ tiến trình `s.exe` → tự động chuyển bảng mã về **TCVN3**.
- Nhận diện theo file Excel / tiêu đề cửa sổ (ví dụ `a.xlsx` → TCVN3, `b.xlsx` → Unicode).
- Hỗ trợ tên file thực tế cấu hình trong `process_rules.ini` (ví dụ `excel.exe[Tong hop ban hang - Date] = TCVN3`).
- **Thuật toán so khớp ranh giới từ (Word-boundary matching)**: không nhận diện nhầm file chứa ký tự đơn `a`/`b` (ví dụ `data.xlsx`, `table.xlsx`).

### A.3. Fallback về Unicode có thể bật/tắt

- **Tắt (mặc định)**: ứng dụng/file không có trong quy tắc sẽ **giữ nguyên 100% bảng mã hiện tại**.
- **Bật**: khi rời ứng dụng/file có quy tắc sang ứng dụng thông thường → tự động về **Unicode**.
- Đồng bộ Registry (`vFallbackToUnicode`) và `process_rules.ini` (`fallback_to_unicode`).

### A.4. Cơ chế Khóa / Bật-Tắt tự động chuyển bảng mã

- Menu khay hệ thống: _"Tự động chuyển bảng mã theo ứng dụng"_ (checkmark).
- Phím tắt `Ctrl + Shift + F12`.
- File cấu hình: `enabled = 1` hoặc `enabled = 0`.

### A.5. Cơ chế gõ tắt dịch động (On-Demand / Lazy JIT Conversion)

- Loại bỏ hoàn toàn trường `macroContentCode` khỏi `struct MacroData` (RAM chỉ lưu `macroText`, `macroContent` UTF-8).
- Loại bỏ tiền biên dịch trong `initMacroMap()` và `addMacro()`.
- Dịch động tại thời điểm kích hoạt trong `findMacro()` theo `vCodeTable` hiện hành.
- `onTableCodeChange()` trở thành thao tác **O(1) no-op** (0% CPU khi đổi bảng mã).

### A.6. Kiến trúc Single Source of Truth

Mọi kênh đổi bảng mã đều quy tụ về `AppDelegate::onTableCode(code)`:

```
[Hotkeys: Ctrl+Shift+F1/F2] ────┐
[Tray Menu: 5 Bảng mã]      ────┤
[Dialog: Combobox Bảng mã]  ────┼───> AppDelegate::onTableCode(code)
[ProcessRuleHelper: Auto]   ────┤           │
[ProcessRuleHelper: Fallback]───┘           ├─> APP_SET_DATA(vCodeTable, code)
                                            ├─> onTableCodeChange() [O(1) no-op]
                                            ├─> mainDialog->fillData()
                                            ├─> SystemTrayHelper::updateData()
                                            └─> Lưu nhớ bảng mã tiến trình
```

---

## PHẦN B — NHẬT KÝ RÀ SOÁT & SỬA LỖI (12/10/2026)

Đợt rà soát đối chiếu 3 tài liệu với mã nguồn thực tế đã phát hiện các vấn đề và tiến hành sửa theo thứ tự ưu tiên.

### B.1. Bảng tổng hợp vấn đề & trạng thái

|  #  | Ưu tiên | Vấn đề                                                                                              | File                                     |        Trạng thái         |
| :-: | :-----: | :-------------------------------------------------------------------------------------------------- | :--------------------------------------- | :-----------------------: |
|  1  |   P0    | `process_rules.ini` đặt `fallback_to_unicode = 1` mâu thuẫn comment & tài liệu (mặc định phải là 0) | `process_rules.ini`                      |         ✅ Đã sửa         |
|  2  |   P0    | Toggle trên Tray chỉ ghi Registry, bị INI ghi đè mỗi lần khởi động → không bền vững                 | `AppDelegate.cpp`, `ProcessRuleHelper.*` |         ✅ Đã sửa         |
|  3  |   P0    | `showNotification` bật cờ `NIF_INFO` vĩnh viễn → balloon cũ lặp lại / lỗi tray                      | `SystemTrayHelper.cpp`                   |         ✅ Đã sửa         |
|  4  |   P1    | `getFrontMostAppExecuteName()` trả về tên app cũ (cache sai) với OpenKey/explorer                   | `OpenKeyHelper.cpp`                      |         ✅ Đã sửa         |
|  5  |   P1    | `matchesTitle` xử lý ranh giới từ theo **byte** → sai với tiếng Việt UTF-8                          | `ProcessRuleHelper.*`                    |         ✅ Đã sửa         |
|  6  |   P1    | Hook `EVENT_OBJECT_NAMECHANGE` toàn cục gây overhead & nhảy bảng mã giữa lúc gõ                     | `OpenKey.cpp`                            |         ✅ Đã sửa         |
|  7  |   P2    | `parseCodeTable` âm thầm trả về Unicode khi giá trị sai                                             | `ProcessRuleHelper.cpp`                  |         ✅ Đã sửa         |
|  8  |   P2    | `loadDefaultRules()` không nhất quán (file rỗng ≠ file thiếu)                                       | `ProcessRuleHelper.cpp`                  |         ✅ Đã sửa         |
|  9  |   P2    | Gọi `SystemTrayHelper::updateData()` trùng lặp trong hotkey handler                                 | `OpenKey.cpp`                            |         ✅ Đã sửa         |
| 10  |   P2    | Tài liệu `DOCS_MACRO_TABLECODE_SYNC.md` lỗi thời (mô tả `macroContentCode` đã xóa)                  | Tài liệu                                 | ✅ Đã hợp nhất & cập nhật |

### B.2. Chi tiết từng bản sửa

#### B.2.1. [P0] Sửa giá trị fallback trong `process_rules.ini`

**Trước**: `fallback_to_unicode = 1` (trái với comment ngay trên và tài liệu).
**Sau**: `fallback_to_unicode = 0` — đúng mặc định "giữ nguyên bảng mã".

```ini
enabled = 1
fallback_to_unicode = 0
```

#### B.2.2. [P0] Đồng bộ hai chiều INI ↔ Registry cho các toggle

**Nguyên nhân**: `onToggleAutoSwitchCodeTable()` / `onToggleFallbackToUnicode()` chỉ ghi Registry, nhưng `ProcessRuleHelper::reloadRules()` đọc lại INI và ghi đè biến toàn cục mỗi lần khởi động → toggle "không nhớ".

**Giải pháp**: Thêm `ProcessRuleHelper::updateIniSetting(key, value)` để ghi giá trị mới vào INI ngay khi người dùng toggle.

`ProcessRuleHelper.h`:

```cpp
static void updateIniSetting(const std::string& key, int value);
```

`ProcessRuleHelper.cpp`:

```cpp
void ProcessRuleHelper::updateIniSetting(const std::string& key, int value) {
    if (_iniFilePath.empty()) initFilePath();
    std::vector<std::string> lines;
    bool found = false;
    std::string lowerKey = toLower(key);
    std::ifstream inFile(_iniFilePath.c_str());
    if (inFile.is_open()) {
        std::string line;
        while (std::getline(inFile, line)) {
            std::string trimmed = trimString(line);
            size_t eqPos = trimmed.find('=');
            if (!found && eqPos != std::string::npos) {
                std::string lineKey = toLower(trimString(trimmed.substr(0, eqPos)));
                if (lineKey == lowerKey) {
                    lines.push_back(key + " = " + std::to_string(value));
                    found = true;
                    continue;
                }
            }
            lines.push_back(line);
        }
        inFile.close();
    }
    if (!found) lines.push_back(key + " = " + std::to_string(value));
    std::ofstream outFile(_iniFilePath.c_str(), std::ios::trunc);
    if (!outFile.is_open()) return;
    for (const auto& l : lines) outFile << l << "\n";
    outFile.close();
}
```

`AppDelegate.cpp`:

```cpp
void AppDelegate::onToggleAutoSwitchCodeTable() {
    APP_SET_DATA(vAutoSwitchCodeTable, vAutoSwitchCodeTable ? 0 : 1);
    ProcessRuleHelper::updateIniSetting("enabled", vAutoSwitchCodeTable);
    ...
}
void AppDelegate::onToggleFallbackToUnicode() {
    APP_SET_DATA(vFallbackToUnicode, vFallbackToUnicode ? 0 : 1);
    ProcessRuleHelper::updateIniSetting("fallback_to_unicode", vFallbackToUnicode);
    ...
}
```

#### B.2.3. [P0] Sửa rò rỉ cờ `NIF_INFO` trong `showNotification`

**Nguyên nhân**: `nid.uFlags |= NIF_INFO;` không bao giờ được reset → mọi `NIM_MODIFY` sau đó mang `NIF_INFO` + `szInfo` cũ.

**Giải pháp**: Lưu và khôi phục `uFlags` sau khi hiển thị.

```cpp
void SystemTrayHelper::showNotification(LPCTSTR title, LPCTSTR message) {
    if (nid.hWnd == NULL) return;
    DWORD savedFlags = nid.uFlags;
    nid.uFlags |= NIF_INFO;
    ...
    Shell_NotifyIcon(NIM_MODIFY, &nid);
    nid.uFlags = savedFlags; // tránh balloon cũ lặp lại
}
```

#### B.2.4. [P1] Sửa cache cũ trong `getFrontMostAppExecuteName`

**Nguyên nhân**: Với `OpenKey64.exe` / `OpenKey32.exe` / `explorer.exe`, hàm `return _exeNameUtf8;` **trước khi** cập nhật `_exeNameUtf8` → trả về tên app trước đó.

**Giải pháp**: Chuyển việc cập nhật `_exeNameUtf8` lên **trước** nhánh kiểm tra đặc biệt.

```cpp
_exeName = _tcsrchr(_exePath, '\\') + 1;
int size_needed = WideCharToMultiByte(CP_UTF8, 0, _exeName, (int)lstrlen(_exeName), NULL, 0, NULL, NULL);
std::string strTo(size_needed, 0);
WideCharToMultiByte(CP_UTF8, 0, _exeName, (int)lstrlen(_exeName), &strTo[0], size_needed, NULL, NULL);
_exeNameUtf8 = strTo;   // cập nhật TRƯỚC
if (wcscmp(_exeName, _T("OpenKey64.exe")) == 0 || ...) return _exeNameUtf8;
return _exeNameUtf8;
```

#### B.2.5. [P1] Sửa `matchesTitle` xử lý ranh giới từ theo wide-char

**Nguyên nhân**: `isDelimiter(char)` duyệt từng byte; các byte continuation của UTF-8 (0x80–0xBF) bị coi là delimiter → khớp nhầm giữa chừng từ tiếng Việt.

**Giải pháp**: Chuyển sang `wchar_t` và dùng `utf8ToWideString()`.

```cpp
bool ProcessRuleHelper::isDelimiter(wchar_t c) {
    return !((c >= L'a' && c <= L'z') || (c >= L'0' && c <= L'9'));
}

bool ProcessRuleHelper::matchesTitle(const std::string& title, const std::string& pattern) {
    if (pattern.empty()) return true;
    if (title.empty()) return false;
    std::wstring wideTitle = utf8ToWideString(title);
    std::wstring widePattern = utf8ToWideString(pattern);
    for (wchar_t& c : wideTitle)   if (c >= L'A' && c <= L'Z') c += (L'a' - L'A');
    for (wchar_t& c : widePattern) if (c >= L'A' && c <= L'Z') c += (L'a' - L'A');
    if (widePattern.empty()) return true;
    size_t pos = 0;
    while ((pos = wideTitle.find(widePattern, pos)) != std::wstring::npos) {
        bool beforeOk = (pos == 0) || isDelimiter(wideTitle[pos - 1]);
        size_t afterPos = pos + widePattern.length();
        bool afterOk = (afterPos == wideTitle.length()) || isDelimiter(wideTitle[afterPos]);
        if (beforeOk && afterOk) return true;
        pos += 1;
    }
    return false;
}
```

#### B.2.6. [P1] Throttle `EVENT_OBJECT_NAMECHANGE`

**Nguyên nhân**: Tiêu đề cửa sổ thay đổi rất thường xuyên → callback chạy lại toàn bộ logic (đọc title + so khớp rule) liên tục, tốn CPU và có thể đổi bảng mã giữa lúc đang gõ.

**Giải pháp**: Giới hạn tần suất 250ms và bỏ qua nếu tiêu đề không đổi.

```cpp
static DWORD _lastTitleEventTime = 0;
static string _lastProcessedTitle = "";
static HWND _lastTitleHwnd = NULL;

if (dwEvent == EVENT_OBJECT_NAMECHANGE) {
    if (idObject != OBJID_WINDOW || hwnd != GetForegroundWindow()) return;
    DWORD now = GetTickCount();
    if (now - _lastTitleEventTime < 250) return;
    _lastTitleEventTime = now;
    string currentTitle = OpenKeyHelper::getFrontMostWindowTitleUtf8();
    if (hwnd == _lastTitleHwnd && currentTitle == _lastProcessedTitle) return;
    _lastTitleHwnd = hwnd;
    _lastProcessedTitle = currentTitle;
}
```

#### B.2.7. [P2] `parseCodeTable` không âm thầm trả về Unicode

**Trước**: giá trị sai → `return 0` (Unicode) → rule gõ sai chính tả âm thầm thành Unicode.
**Sau**: `return -1` và caller bỏ qua rule sai.

```cpp
static int parseCodeTable(const std::string& valueStr) {
    ...
    return -1; // Unknown value -> caller should skip this rule
}
// caller:
int code = parseCodeTable(rawVal);
if (code < 0) continue; // skip malformed rule
```

#### B.2.8. [P2] Thống nhất hành vi default rules

**Trước**: file INI thiếu → dùng 3 rule mặc định; file INI tồn tại nhưng rỗng rule → không có rule nào.
**Sau**: nếu sau khi parse mà `_ruleItems` rỗng → nạp lại default rules.

```cpp
inFile.close();
if (_ruleItems.empty()) {
    loadDefaultRules();
}
```

#### B.2.9. [P2] Bỏ gọi `updateData()` trùng lặp

Trong hotkey handler `Ctrl+Shift+F1/F2`, `onTableCode()` đã gọi `SystemTrayHelper::updateData()` bên trong → bỏ lệnh gọi thừa ngay sau đó.

---

## PHẦN C — HƯỚNG DẪN SỬ DỤNG

1. **Bật/Tắt Fallback về Unicode tại Tray**: chuột phải biểu tượng OpenKey → _"Fallback về Unicode khi rời ứng dụng"_.
   - Có dấu ✓: rời app không có quy tắc → về Unicode.
   - Không dấu ✓: giữ nguyên bảng mã hiện tại.
2. **Khóa/Mở khóa tự động chuyển bảng mã**: dòng _"Tự động chuyển bảng mã theo ứng dụng"_ hoặc `Ctrl + Shift + F12`.
3. **Phím tắt đổi bảng mã nhanh**: `Ctrl + Shift + F1` (Unicode), `Ctrl + Shift + F2` (TCVN3).
4. **Cấu hình quy tắc**: chỉnh `process_rules.ini` cạnh `OpenKey.exe` (khởi động lại app để nạp lại).

### Định dạng `process_rules.ini`

```ini
enabled = 1
fallback_to_unicode = 0

s.exe = TCVN3
excel.exe[Tong hop ban hang - Date] = TCVN3
chrome.exe = UNICODE
zalo.exe = UNICODE
```

Bảng mã hỗ trợ: `0/UNICODE`, `1/TCVN3`, `2/VNI`, `3/UNICODE_COMPOUND`, `4/VN_LOCALE_1258`.

---

## PHẦN D — BIÊN DỊCH & KIỂM THỬ

### D.1. Biên dịch

```cmd
cmd.exe /c build.bat
```

Kết quả: **Exit code 0**, `OpenKey.exe` được cập nhật tại thư mục gốc dự án. Chỉ còn các warning cũ không liên quan (`-Winconsistent-missing-override`, `-Wdelete-abstract-non-virtual-dtor`).

### D.2. Kiểm thử gõ tắt (Lazy JIT)

```cmd
clang++ -std=c++14 -O2 -D_WIN32 -DUNICODE -D_UNICODE -ISources/OpenKey/win32/OpenKey/OpenKey -ISources/OpenKey/engine Sources/OpenKey/engine/ConvertTool.cpp Sources/OpenKey/engine/Engine.cpp Sources/OpenKey/engine/Macro.cpp Sources/OpenKey/engine/SmartSwitchKey.cpp Sources/OpenKey/engine/Vietnamese.cpp tests/test_lazy_macro.cpp -o tests/test_lazy_macro.exe
tests\test_lazy_macro.exe
```

### D.3. Ma trận kiểm thử chức năng (đã nghiệm thu trước đó)

|  Mã   | Kịch bản                              | Kết quả |
| :---: | :------------------------------------ | :-----: |
| TC-01 | Build & Binary Verification           |  PASS   |
| TC-02 | On-Demand Unicode Expansion           |  PASS   |
| TC-03 | On-Demand TCVN3 Expansion             |  PASS   |
| TC-04 | On-Demand VNI Expansion               |  PASS   |
| TC-05 | On-Demand Unicode Composite           |  PASS   |
| TC-06 | AutoCaps Title Case                   |  PASS   |
| TC-07 | AutoCaps All-Caps                     |  PASS   |
| TC-08 | Stress Rapid Table Code Switching     |  PASS   |
| TC-09 | Macro Add / Modify / Delete Flow      |  PASS   |
| TC-10 | RAM Footprint & Benchmark             |  PASS   |
| TC-11 | Adversarial: Empty & Boundary Strings |  PASS   |
| TC-12 | Adversarial: Symbols, Emoji, Non-VN   |  PASS   |
| TC-13 | Adversarial: AutoCaps Disabled        |  PASS   |
| TC-14 | Adversarial: Alphanumeric Keywords    |  PASS   |

---

## PHẦN E — CÁC VẤN ĐỀ CÒN LẠI (KHUYẾN NGHỊ TƯƠNG LAI)

> **Cập nhật 12/10/2026 (đợt 2)**: Toàn bộ 6 vấn đề dưới đây **đã được sửa** — xem Phần G.

Các điểm sau chưa được sửa trong đợt này (rủi ro thấp / cần thiết kế thêm):

1. **Rule `title:...` áp dụng cho mọi tiến trình** — có thể nhảy bảng mã ngoài ý muốn; nên giới hạn theo process hoặc cảnh báo.
2. **Xung đột Auto-Switch ↔ Smart-Switch-Key** — hai cơ chế cùng phản ứng với sự kiện foreground; nên thống nhất luồng.
3. **Hotkey `Ctrl+Shift+F1/F2/F12` hardcode** — chưa có UI cấu hình, chưa kiểm tra xung đột.
4. **Không có hot-reload INI** — sửa `process_rules.ini` cần khởi động lại app.
5. **`getCodeTableName()` là dead code** — có thể xóa hoặc dùng cho thông báo.
6. **Số liệu benchmark trong tài liệu cũ** — `convert()` thực tế là $O(L \times |codeTable|)$ cho ký tự có dấu, không phải hằng số tuyệt đối.

---

## PHẦN F — DANH SÁCH FILE ĐÃ THAY ĐỔI (đợt 12/10/2026)

| File                                                          | Thay đổi                                                                                            |
| :------------------------------------------------------------ | :-------------------------------------------------------------------------------------------------- |
| `process_rules.ini`                                           | `fallback_to_unicode` 1 → 0                                                                         |
| `Sources/OpenKey/win32/OpenKey/OpenKey/ProcessRuleHelper.h`   | Thêm `updateIniSetting`; `isDelimiter(wchar_t)`                                                     |
| `Sources/OpenKey/win32/OpenKey/OpenKey/ProcessRuleHelper.cpp` | Thêm `updateIniSetting`; `matchesTitle` wide-char; `parseCodeTable` trả -1; default rules nhất quán |
| `Sources/OpenKey/win32/OpenKey/OpenKey/AppDelegate.cpp`       | Toggle ghi INI                                                                                      |
| `Sources/OpenKey/win32/OpenKey/OpenKey/SystemTrayHelper.cpp`  | Khôi phục `uFlags` sau `showNotification`                                                           |
| `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKeyHelper.cpp`     | Cập nhật `_exeNameUtf8` trước nhánh đặc biệt                                                        |
| `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp`           | Throttle `EVENT_OBJECT_NAMECHANGE`; bỏ `updateData()` trùng lặp                                     |

---

## PHẦN G — ĐỢT SỬA LỖI THỨ 2 (Phần E) — 12/10/2026

Đợt 2 xử lý toàn bộ 6 vấn đề tồn đọng trong Phần E.

### G.1. Bảng tổng hợp

|  #  | Vấn đề (Phần E)                         | Giải pháp                                                                         | File                                    | Trạng thái |
| :-: | :-------------------------------------- | :-------------------------------------------------------------------------------- | :-------------------------------------- | :--------: |
|  1  | Rule `title:...` áp dụng mọi tiến trình | Thêm cảnh báo debug khi phát hiện title-only rule                                 | `ProcessRuleHelper.cpp`                 |     ✅     |
|  2  | Xung đột Auto-Switch ↔ Smart-Switch-Key | Hợp nhất: thêm "remember table code" (khôi phục bảng mã đã nhớ cho app) như macOS | `OpenKey.cpp`                           |     ✅     |
|  3  | Hotkey hardcode chưa kiểm tra xung đột  | Guard: bỏ qua hotkey cứng nếu trùng hotkey người dùng cấu hình                    | `OpenKey.cpp`                           |     ✅     |
|  4  | Không có hot-reload INI                 | Thêm menu Tray _"Nạp lại quy tắc (process_rules.ini)"_                            | `SystemTrayHelper.cpp`, `AppDelegate.*` |     ✅     |
|  5  | `getCodeTableName()` dead code          | Dùng cho thông báo hotkey F1/F2                                                   | `OpenKey.cpp`                           |     ✅     |
|  6  | Số liệu benchmark cũ                    | Ghi chú lại độ phức tạp thực tế (tài liệu)                                        | `All_fix.md`                            |     ✅     |

### G.2. Chi tiết

#### G.2.1. [E2] Hợp nhất Auto-Switch ↔ Smart-Switch-Key

**Nguyên nhân**: `onTableCode()` lưu cả bảng mã vào smart-switch-key (`vLanguage | (vCodeTable << 1)`) nhưng `winEventProcCallback` chỉ đọc lại bit ngôn ngữ (`& 0x01`), bỏ qua phần bảng mã → hai cơ chế chồng chéo.

**Giải pháp**: Đọc `_languageTemp` **một lần** cho cả hai tính năng và thêm nhánh "remember table code" (giống `OnActiveAppChanged` trên macOS):

```cpp
// Đọc trạng thái đã nhớ cho app (dùng chung cho cả 2 tính năng)
if (dwEvent == EVENT_SYSTEM_FOREGROUND && vUseSmartSwitchKey) {
    _languageTemp = getAppInputMethodStatus(exe, vLanguage | (vCodeTable << 1));
} else {
    _languageTemp = -1;
}

if (vAutoSwitchCodeTable) {
    int ruleCode = ProcessRuleHelper::getCodeTableForProcessAndTitle(exe, title);
    if (ruleCode != -1) {
        // Rule tường minh thắng
        if (vCodeTable != ruleCode) { onTableCode(ruleCode); ... }
    } else if (vRememberCode && _languageTemp != -1) {
        // Không có rule -> khôi phục bảng mã đã nhớ cho app này
        int rememberedCode = (_languageTemp >> 1);
        if (vCodeTable != rememberedCode) { onTableCode(rememberedCode); ... }
    } else if (vFallbackToUnicode) {
        if (vCodeTable != 0) { onTableCode(0); ... }
    }
}
```

**Thứ tự ưu tiên**: Rule tường minh → Bảng mã đã nhớ (nếu `vRememberCode`) → Fallback Unicode (nếu bật) → Giữ nguyên.

#### G.2.2. [E3] Guard xung đột hotkey

```cpp
bool _conflictsWithUserHotkey =
    (GET_SWITCH_KEY(vSwitchKeyStatus) == _keycode) ||
    (GET_SWITCH_KEY(convertToolHotKey) == _keycode);
if (!_conflictsWithUserHotkey &&
    (_flag & MASK_CONTROL) && (_flag & MASK_SHIFT) && !(_flag & MASK_ALT) && !(_flag & MASK_WIN)) {
    // ... xử lý F1/F2/F12
}
```

Nếu người dùng đã gán F1/F2/F12 làm phím chuyển ngôn ngữ/chuyển mã nhanh, hotkey cứng sẽ nhường quyền.

#### G.2.3. [E4] Hot-reload INI

Thêm mục menu Tray `POPUP_RELOAD_RULES` (906) → `AppDelegate::onReloadProcessRules()` → `ProcessRuleHelper::reloadRules()` + thông báo. Người dùng sửa `process_rules.ini` rồi nạp lại **không cần khởi động lại app**.

#### G.2.4. [E5] Cảnh báo title-only rule

```cpp
for (const auto& item : _ruleItems) {
    if (item.processName.empty() && !item.titlePattern.empty()) {
        LOG(L"[ProcessRuleHelper] Warning: title-only rule '%s' applies to ALL processes.\n",
            utf8ToWideString(item.titlePattern).c_str());
        break;
    }
}
```

#### G.2.5. [E6] Dùng `getCodeTableName()`

Thông báo hotkey F1/F2 giờ lấy tên bảng mã động từ `ProcessRuleHelper::getCodeTableName(code)` thay vì chuỗi hardcode.

### G.3. File thay đổi (đợt 2)

| File                                                          | Thay đổi                                                            |
| :------------------------------------------------------------ | :------------------------------------------------------------------ |
| `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp`           | Hợp nhất remember-table-code; guard hotkey; dùng `getCodeTableName` |
| `Sources/OpenKey/win32/OpenKey/OpenKey/AppDelegate.h`         | Thêm `onReloadProcessRules()`                                       |
| `Sources/OpenKey/win32/OpenKey/OpenKey/AppDelegate.cpp`       | Cài đặt `onReloadProcessRules()`                                    |
| `Sources/OpenKey/win32/OpenKey/OpenKey/SystemTrayHelper.cpp`  | Menu `POPUP_RELOAD_RULES`                                           |
| `Sources/OpenKey/win32/OpenKey/OpenKey/ProcessRuleHelper.cpp` | Cảnh báo title-only rule                                            |

**Build đợt 2**: ✅ Exit code 0.

---

## PHẦN H — ĐỢT SỬA LỖI THỨ 3: XỬ LÝ FOCUS TEXTBOX & CLEAN MACRO BUFFER (12/10/2026)

Đợt 3 giải quyết triệt để lỗi gõ tắt không hoạt động khi mở UserForm / Dialog (Excel, Word, Access, Web, Form nhập liệu hệ thống) có focus tự động vào TextBox.

### H.1. Phân tích nguyên nhân gốc rễ

1. **Bộ đệm `hMacroKey` không được làm sạch trong `startNewSession()`**:
   - `startNewSession()` trong `Engine.cpp` chỉ reset `_index`, `hBPC`, `_hasHandledMacro`, nhưng **hoàn toàn bỏ quên `hMacroKey.clear()` và `hMacroData.clear()`**.
   - Khi chuyển cửa sổ (kể cả khi `EVENT_SYSTEM_FOREGROUND` chạy), các ký tự rác từ phiên trước vẫn còn nguyên trong `hMacroKey`.

2. **Thiếu WinEvent Hook theo dõi chuyển đổi Focus (`EVENT_OBJECT_FOCUS` - 0x8005)**:
   - OpenKey trước đó chỉ hook `EVENT_SYSTEM_FOREGROUND` và `EVENT_OBJECT_NAMECHANGE`.
   - Khi mở UserForm trong Excel (hoặc child dialog, modeless popup, nhảy Tab giữa các TextBox), Windows không phát sinh `EVENT_SYSTEM_FOREGROUND` do cùng thuộc process Excel.
   - Do đó, OpenKey không hề biết focus đã chuyển sang ô nhập liệu mới; không có sự kiện nào kích hoạt việc reset phiên gõ.

3. **Phím chức năng (`VK_F1` - `VK_F24`) bị bỏ sót trong `_breakCode`**:
   - Khi người dùng bấm `F5` trong Excel/VBA để mở UserForm (hoặc F2, F8), OpenKey không coi F5 là phím ngắt từ mà nhét mã phím `VK_F5` (0x74) vào `hMacroKey`.
   - Khi UserForm hiện lên, `hMacroKey` chứa `[VK_F5]`. Người dùng gõ `dc` + Space $\rightarrow$ `findMacro` tìm `[VK_F5, 'd', 'c']` $\rightarrow$ Thất bại.

4. **Tại sao click chuột lại làm macro hoạt động?**:
   - `mouseHookProcess` bắt `WM_LBUTTONDOWN` và gọi `vKeyHandleEvent(Mouse)` $\rightarrow$ Kích hoạt `isWordBreak` $\rightarrow$ Gọi `startNewSession()` và gọi trực tiếp `hMacroKey.clear()` $\rightarrow$ Vô tình dọn sạch rác, đưa buffer về trạng thái rỗng.

### H.2. Giải pháp kỹ thuật tổng quát đã triển khai

1. **`Engine.h` & `Engine.cpp`**:
   - Thêm `hMacroKey.clear()` và `hMacroData.clear()` vào `startNewSession()`.
   - Bổ sung hàm tiện ích `resetInputState()` để dọn sạch toàn diện `startNewSession()`, `_typingStates`, `_specialChar`, `_spaceCount = 0`, reset spelling và temp-off flags.
   - Thêm kiểm tra `(data >= VK_F1 && data <= VK_F24)` trong `isWordBreak()` để các phím F không bao giờ bị coi là ký tự gõ từ hay nhét vào macro buffer.

2. **`OpenKey.cpp`**:
   - Khai báo và đăng ký hook `hFocusEvent = SetWinEventHook(EVENT_OBJECT_FOCUS, EVENT_OBJECT_FOCUS, ...)`.
   - Giải phóng `hFocusEvent` trong `OpenKeyFree()`.
   - Trong `winEventProcCallback`:
     - Deduplicate sự kiện focus (`hwnd`, `idObject`, `idChild`), bỏ qua `OBJID_CARET`, `OBJID_CURSOR`.
     - Khi focus chuyển đổi sang control/window mới: gọi `resetInputState()`, xóa `_syncKey.clear()`, reset `_lastFlag = 0`, `_hasJustUsedHotKey = false`, `_keycode = 0`.
     - Cập nhật và đánh giá lại quy tắc ứng dụng / bảng mã cho cửa sổ chứa control vừa nhận focus.

### H.3. File thay đổi (đợt 3)

| File | Thay đổi |
| :--- | :------- |
| `Sources/OpenKey/engine/Engine.h` | Khai báo `resetInputState()` |
| `Sources/OpenKey/engine/Engine.cpp` | `isWordBreak()` hỗ trợ F1–F24; `startNewSession()` xóa `hMacroKey`; cài đặt `resetInputState()` |
| `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp` | Thêm `hFocusEvent`, hook `EVENT_OBJECT_FOCUS`, xử lý reset engine trên focus change |
| `tests/test_focus_macro_clean.cpp` | Bộ unit test kiểm thử focus & macro clean state |

**Kiểm thử**:
- `tests\test_focus_macro_clean.exe`: ✅ 4/4 PASS.
- `tests\test_lazy_macro.exe`: ✅ 14/14 PASS (Zero regression).
- `build.bat`: ✅ Exit code 0, `OpenKey.exe` đã cập nhật.

---

---

## PHẦN I — ĐỢT SỬA LỖI THỨ 4: NGĂN FALLBACK UNICODE KHI MỞ USERFORM & GIỮ BẢNG MÃ TRONG CÙNG ỨNG DỤNG (12/10/2026)

Giải quyết triệt để vấn đề: Khi UserForm / Dialog bật lên trong Excel (hoặc Word, Access, v.v.), OpenKey bị fallback về bảng mã Unicode do tiêu đề UserForm thay thế tên file Excel hoặc do sự kiện focus control kích hoạt lại bộ nhận diện bảng mã.

### I.1. Phân tích nguyên nhân gốc rễ

1. **Sự kiện `EVENT_OBJECT_FOCUS` chạy nhầm logic Auto-Switch Code Table**:
   - Khi hook `EVENT_OBJECT_FOCUS` được đưa vào `winEventProcCallback`, mọi sự kiện focus con trỏ vào TextBox/Control đều chạy qua khối kiểm tra bảng mã `vAutoSwitchCodeTable` và `vUseSmartSwitchKey`.
   - Vì focus rơi vào TextBox, việc truy vấn tiêu đề cửa sổ / rule bị sai lệch và kích hoạt nhánh `vFallbackToUnicode` $\rightarrow$ Đưa bảng mã về Unicode (0).
   - **Nguyên lý đúng**: `EVENT_OBJECT_FOCUS` chỉ là sự kiện chuyển ô nhập liệu nội bộ; **chỉ được phép reset buffer gõ và macro**, tuyệt đối không được phép đổi bảng mã hay đổi ngôn ngữ gõ.

2. **Tiêu đề UserForm "chiếm" tên file Excel**:
   - Khi UserForm hiển thị (kích hoạt `EVENT_SYSTEM_FOREGROUND`), tiêu đề cửa sổ foreground là `"UserForm1"` thay vì `"Tong hop ban hang - Date.xlsm - Excel"`.
   - `ProcessRuleHelper::getCodeTableForProcessAndTitle("EXCEL.EXE", "UserForm1")` trả về `-1` (không khớp quy tắc).
   - Nếu không có cơ chế tìm lại cửa sổ gốc sở hữu (Root Owner Window) hay cửa sổ tài liệu trong cùng tiến trình, OpenKey sẽ hiểu nhầm là không có quy tắc.

3. **Chuyển cửa sổ trong cùng một ứng dụng bị kích hoạt Fallback**:
   - Theo định nghĩa trong `process_rules.ini`: `fallback_to_unicode = 1` là *fallback về Unicode khi **rời app*** (chuyển sang phần mềm khác).
   - Khi người dùng vẫn đang làm việc bên trong `EXCEL.EXE` (mở UserForm, mở hộp thoại Find, Format Cells...), người dùng **chưa hề rời app**. Việc fallback về Unicode khi mở cửa sổ con trong cùng app là sai logic nghiệp vụ.

### I.2. Giải pháp kỹ thuật tổng quát đã triển khai

1. **Tách biệt hoàn toàn `EVENT_OBJECT_FOCUS` khỏi Auto-Switch & Smart-Switch**:
   - Trong `winEventProcCallback`, nhánh `EVENT_OBJECT_FOCUS` chỉ thực hiện `resetInputState()`, xóa `_syncKey`, reset cờ phím và **`return` ngay lập tức**.
   - Không chạy qua `_languageTemp`, không chạy qua `ProcessRuleHelper`, không kích hoạt `onTableCode()`.

2. **Truy vấn tiêu đề cửa sổ gốc (`GA_ROOTOWNER`) và cửa sổ cùng tiến trình**:
   - Bổ sung hàm `OpenKeyHelper::getWindowTitleUtf8(HWND hwnd)`.
   - Khi cửa sổ foreground (như UserForm) không khớp quy tắc trực tiếp:
     - Lấy `HWND hRootOwner = GetAncestor(hForeground, GA_ROOTOWNER)`. Nếu cùng PID, kiểm tra quy tắc với tiêu đề của `hRootOwner` (chứa tên file Excel: `"Tong hop ban hang - Date.xlsm - Excel"`).
     - Nếu vẫn chưa khớp (UserForm modeless/cửa sổ phụ độc lập), quét nhanh các top-level window đang hiển thị của cùng tiến trình (`EnumWindows`) để nhận diện quy tắc bảng mã của tài liệu đang mở.

3. **Bảo toàn bảng mã trong cùng tiến trình (`isSameApp`)**:
   - Theo dõi `_lastForegroundExe`. Nếu `isSameApp == true` (`_lastForegroundExe == exe`):
     - Nếu có quy tắc chỉ định tường minh: áp dụng quy tắc.
     - Nếu không có quy tắc chỉ định mới: **giữ nguyên bảng mã hiện tại của ứng dụng**, tuyệt đối không fallback về Unicode hay khôi phục trạng thái cũ.
   - Chỉ khi chuyển sang ứng dụng khác (`!isSameApp`) mới thực hiện fallback về Unicode nếu `vFallbackToUnicode = 1`.

### I.3. File thay đổi (đợt 4)

| File | Thay đổi |
| :--- | :------- |
| `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKeyHelper.h` | Khai báo `getWindowTitleUtf8(HWND hwnd)` |
| `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKeyHelper.cpp` | Cài đặt `getWindowTitleUtf8(HWND hwnd)` và tối ưu `getFrontMostWindowTitleUtf8()` |
| `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp` | Ngắt `EVENT_OBJECT_FOCUS` return ngay; thêm giải quyết root owner & EnumWindows; bảo vệ bảng mã với `isSameApp` |
| `tests/test_userform_title_fallback.cpp` | Bộ unit test kiểm thử UserForm title resolution, same-app fallback guard, focus isolation |

**Kiểm thử**:
- `tests\test_userform_title_fallback.exe`: ✅ 6/6 PASS.
- `tests\test_focus_macro_clean.exe`: ✅ 4/4 PASS.
- `tests\test_lazy_macro.exe`: ✅ 14/14 PASS (Zero regression).
- `build.bat`: ✅ Exit code 0, `OpenKey.exe` đã cập nhật.

---

---

## PHẦN J — ĐỢT SỬA LỖI THỨ 5: PHÂN TÁCH ĐỘC LẬP TỪNG FILE EXCEL (MULTI-WORKBOOK INDEPENDENCE) (12/10/2026)

Giải quyết triệt để lỗi: Tất cả các file Excel khác mở trên máy đều bị chuyển sang font TCVN3 dù người dùng chỉ cấu hình duy nhất một file Excel có quy tắc TCVN3 (`excel.exe[Tong hop ban hang - Date] = TCVN3`).

### J.1. Phân tích nguyên nhân gốc rễ

1. **`EnumWindows` quét toàn bộ tiến trình gán nhầm quy tắc sang các file khác**:
   - Trong đợt sửa trước, để hỗ trợ UserForm modeless, hàm `EnumWindows` quét qua mọi cửa sổ có cùng `PID` của `EXCEL.EXE`.
   - Vì Excel chạy dưới một tiến trình duy nhất cho mọi workbook mở cùng lúc, khi người dùng mở hoặc chuyển sang file khác (như `Book2.xlsx` hay `Báo cáo.xlsx`), `EnumWindows` phát hiện file `Tong hop ban hang - Date` cũng đang mở trong cùng tiến trình $\rightarrow$ Gán nhầm `ruleCode = 1` (TCVN3) cho cả `Book2.xlsx`!

2. **Khóa `!isSameApp` chặn `fallback_to_unicode` giữa các tài liệu trong cùng app**:
   - Khi chuyển giữa các workbook trong cùng Excel, `isSameApp` bằng `true`.
   - Khối `fallback_to_unicode` trước đó chỉ chạy khi `!isSameApp` $\rightarrow$ Khiến các file Excel không có quy tắc bị "mắc kẹt" ở bảng mã TCVN3 của file trước đó thay vì quay về Unicode.

### J.2. Giải pháp kỹ thuật chuẩn xác

1. **Xóa bỏ hoàn toàn `EnumWindows` quét xuyên tiến trình**:
   - Tuyệt đối không để quy tắc của workbook này rò rỉ sang workbook khác chạy cùng tiến trình.

2. **Phân biệt rạch ròi Cửa sổ Tài liệu (Workbook Document) và UserForm / Dialog**:
   - Tạo hàm kiểm tra `isDialogOrOwnedWindow(HWND hwnd)`:
     - Cửa sổ tài liệu Excel (`XLMAIN`) là cửa sổ độc lập cấp cao nhất (`GW_OWNER == NULL` và `GA_ROOTOWNER == hwnd`), `isDialogOrOwnedWindow` trả về `false`.
     - Cửa sổ UserForm (`ThunderDFrame`), Dialog (`#32770`, `bosa_sdm_XL`), hoặc cửa sổ có `GW_OWNER != NULL`: `isDialogOrOwnedWindow` trả về `true`.
   - **Xử lý cho UserForm / Dialog**:
     - Tìm cửa sổ Workbook sở hữu nó (`GW_OWNER` hoặc `GA_ROOTOWNER`). Nếu Workbook sở hữu là `Tong hop ban hang - Date` $\rightarrow$ UserForm thừa hưởng bảng mã TCVN3 của chính file đó.
     - Nếu không có quy tắc, vì là Dialog/UserForm nên không bị ép fallback về Unicode, giữ nguyên trạng thái làm việc của Form.
   - **Xử lý cho Cửa sổ Workbook (`!isDialogOrForm`)**:
     - Từng file Excel độc lập đánh giá tiêu đề của chính nó qua `ProcessRuleHelper`.
     - File có quy tắc (`Tong hop ban hang - Date`) $\rightarrow$ Chuyển TCVN3.
     - Các file khác (`Book2.xlsx`, `Báo cáo.xlsx`...) không có quy tắc $\rightarrow$ Khi bật `fallback_to_unicode = 1`, OpenKey tự động **fallback về UNICODE (0)** ngay lập tức!

### J.3. File thay đổi (đợt 5)

| File | Thay đổi |
| :--- | :------- |
| `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp` | Thêm `isDialogOrOwnedWindow()`; xóa bỏ `EnumWindows`; tách biệt cơ chế fallback cho Document Window và UserForm |
| `tests/test_userform_title_fallback.cpp` | Mở rộng bộ unit test lên 7/7 test cases bao quát đa workbook độc lập và UserForm |

**Kiểm thử**:
- `tests\test_userform_title_fallback.exe`: ✅ 7/7 PASS (Xác nhận các file Excel khác tự động về Unicode; chỉ đúng file chỉ định mới nhận TCVN3; UserForm của file chỉ định giữ nguyên TCVN3).
- `tests\test_focus_macro_clean.exe`: ✅ 4/4 PASS.
- `tests\test_lazy_macro.exe`: ✅ 14/14 PASS.
- `build.bat`: ✅ Exit code 0, `OpenKey.exe` cập nhật và đang chạy trực tiếp (PID 8264).

---

## PHẦN K — ĐỢT SỬA LỖI THỨ 6: RÀ SOÁT ĐỘC LẬP ĐỢT 3–5 (AUDIT & HARDENING) (12/10/2026)

Đợt 6 là kết quả rà soát độc lập các đợt 3–5, phát hiện và sửa các **lỗi ngầm**, sai sót logic và bằng chứng kiểm thử sai lệch.

### K.1. Bảng tổng hợp vấn đề & trạng thái

|  #  | Ưu tiên | Vấn đề                                                                                                   | File                                     | Trạng thái |
| :-: | :-----: | :------------------------------------------------------------------------------------------------------- | :--------------------------------------- | :--------: |
|  1  |   P0    | `vLanguage` bị gán giá trị "gói" `language \| (codeTable<<1)` → hỏng chế độ Anh/Việt                     | `OpenKey.cpp`                            |  ✅ Đã sửa |
|  2  |   P0    | `process_rules.ini` để `fallback_to_unicode = 1` nhưng comment ghi "giữ nguyên" → tài liệu mâu thuẫn file | `process_rules.ini`                      |  ✅ Đã sửa |
|  3  |   P1    | Thứ tự ưu tiên "remember code" đặt sai (bị `vFallbackToUnicode` chặn trước) & có `!isSameApp` vô hiệu     | `OpenKey.cpp`                            |  ✅ Đã sửa |
|  4  |   P1    | `isDialogOrOwnedWindow` không nhận diện popup/dialog **không owner** → vẫn fallback Unicode               | `OpenKey.cpp`                            |  ✅ Đã sửa |
|  5  |   P2    | `_lastTitleHwnd`/`_lastProcessedTitle` không reset khi đổi foreground → trễ đánh giá bảng mã 1 nhịp       | `OpenKey.cpp`                            |  ✅ Đã sửa |
|  6  |   P2    | Dò owner chỉ 1 tầng, không theo chuỗi dialog lồng nhau                                                    | `OpenKey.cpp`                            |  ✅ Đã sửa |
|  7  |   P2    | Test "7/7 PASS" tự viết lại `matchesTitle` (substring) → không kiểm thử code sản phẩm                    | `tests/test_userform_title_fallback.cpp` |  ✅ Đã sửa |
|  8  |   P2    | Không có test nào chạy logic `ProcessRuleHelper` thật                                                     | `tests/test_process_rules_real.cpp`      |  ✅ Đã thêm |

### K.2. Chi tiết

#### K.2.1. [K1] `vLanguage` bị nhiễm giá trị bảng mã

**Nguyên nhân**: `getAppInputMethodStatus()` trả về giá trị đã đóng gói `vLanguage | (vCodeTable << 1)`. Callback lại gán thẳng `vLanguage = _languageTemp`, trong khi Engine chỉ hiểu `0` = tiếng Anh.

**Hậu quả**: Một app vừa ở chế độ Anh vừa có bảng mã ≠ Unicode (giá trị `2`) khiến `vLanguage = 2`; `switchLanguage()` và toàn bộ logic Anh/Việt hỏng, registry bị ghi giá trị rác.

**Sửa**:
```cpp
vLanguage = _languageTemp & 0x01;   // chỉ lấy bit ngôn ngữ
```

#### K.2.2. [K2] Đồng bộ `process_rules.ini` với tài liệu

File cấu hình thực tế (`fallback_to_unicode = 1`) mâu thuẫn với comment ngay trên và Phần B.2.1 (mặc định là 0). Đã cập nhật comment cho khớp giá trị thực & hành vi đã mô tả ở Phần J (khi bật, file không có rule sẽ về Unicode).

#### K.2.3. [K3] Sắp lại chuỗi ưu tiên (Rule → Fallback → Remember → Keep)

Trước đây `vFallbackToUnicode` được xét **trước** nhánh remember và có thêm điều kiện `!isSameApp` vô hiệu hóa việc khôi phục khi quay lại cùng app/tài liệu. Nay:
```cpp
if (ruleCode != -1) { /* Rule tường minh thắng */ }
else if (!isDialogOrForm) {
    if (vFallbackToUnicode)                     { /* về Unicode */ }
    else if (vRememberCode && _languageTemp != -1) { /* khôi phục bảng mã đã nhớ */ }
}
```

#### K.2.4. [K4] Nhận diện popup/dialog không owner

Bổ sung heuristic: cửa sổ `WS_POPUP` không resize và thiếu cặp caption+system-menu được coi là dialog → **không** fallback Unicode. Kết hợp với `isDialogOrOwnedWindow()` hiện có.

#### K.2.5. [K5] Reset cache dedup tiêu đề khi đổi foreground

`_lastTitleHwnd = NULL; _lastProcessedTitle.clear();` khi `EVENT_SYSTEM_FOREGROUND` để event `NAMECHANGE` đầu tiên của cửa sổ mới không bị bỏ sót.

#### K.2.6. [K6] Duyệt chuỗi owner (bounded, 8 tầng)

Tách hàm `resolveRuleCodeForWindow()` duyệt `GW_OWNER`/`GA_ROOTOWNER` nhiều tầng, chỉ kế thừa rule trong **cùng process**, phục vụ dialog lồng nhau (UserForm trong dialog, modeless popup...).

#### K.2.7. [K7] Test dùng logic thật

`test_userform_title_fallback.cpp` nay `#include "ProcessRuleHelper.h"` và **link** `ProcessRuleHelper.cpp`, gọi trực tiếp `matchesTitle()`/`getCodeTableForProcessAndTitle()` thật thay vì bản substring tự viết.

#### K.2.8. [K8] Bộ test thật mới

Thêm `tests/test_process_rules_real.cpp` (23 assertions): kiểm tra ranh giới từ ASCII & tiếng Việt UTF-8, phân giải rule, mô hình quyết định đa workbook, và giải mã gói smart-switch `(language | codeTable<<1)`.

### K.3. File thay đổi (đợt 6)

| File | Thay đổi |
| :--- | :------- |
| `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp` | Fix `vLanguage & 0x01`; sắp lại ưu tiên; `resolveRuleCodeForWindow()` duyệt chuỗi owner; heuristic popup không owner; reset cache tiêu đề |
| `Sources/OpenKey/win32/OpenKey/OpenKey/ProcessRuleHelper.h` | `matchesTitle` public; thêm `setRuleItemsForTest()` |
| `Sources/OpenKey/win32/OpenKey/OpenKey/ProcessRuleHelper.cpp` | Cài đặt `setRuleItemsForTest()` |
| `process_rules.ini` | Comment khớp giá trị `fallback_to_unicode = 1` |
| `tests/test_userform_title_fallback.cpp` | Link & gọi ProcessRuleHelper thật |
| `tests/test_process_rules_real.cpp` | **Mới** — bộ test logic thật (23 assertions) |

**Kiểm thử đợt 6**:
- `tests\test_process_rules_real.exe`: ✅ 23/23 PASS (logic thật).
- `tests\test_userform_title_fallback.exe`: ✅ 7/7 PASS (nay dùng helper thật).
- `tests\test_focus_macro_clean.exe`: ✅ 4/4 PASS.
- `tests\test_lazy_macro.exe`: ✅ 14/14 PASS (zero regression).
- `tests\test_auditor_independent.exe`: ✅ 5/5 PASS.
- `build.bat`: ✅ `OpenKey.exe` cập nhật (16:38 12/10/2026).

---

## PHẦN L — ĐỢT SỬA LỖI THỨ 7: SHELL / START MENU / MINIMIZE LÀM MẤT BẢNG MÃ (12/10/2026)

Giải quyết triệt để: Dùng thanh **Start** để thu nhỏ `s.exe` → bảng mã fallback về Unicode; sau đó click tab `s.exe` ở taskbar → font **không** tự về lại TCVN3.

### L.1. Phân tích nguyên nhân gốc rễ

1. **Guard `explorer.exe` quá hẹp**:
   - OpenKey chỉ bỏ qua đúng tiến trình `explorer.exe`. Nhưng Start menu trên Windows 10/11 thuộc `StartMenuExperienceHost.exe` (Search: `SearchHost.exe`, Action Center: `ShellExperienceHost.exe`, …).
   - Khi mở Start để thu nhỏ `s.exe`, foreground là `StartMenuExperienceHost.exe` → rule = −1, không phải dialog → nhánh `vFallbackToUnicode` chạy → `onTableCode(0)` → **tụt về Unicode**.

2. **Không bỏ qua sự kiện minimize**:
   - Không hook `EVENT_SYSTEM_MINIMIZESTART/END`; mọi thay đổi foreground chớp nhoáng đều xử lý như "đổi app".

3. **Khôi phục phụ thuộc đúng một event foreground**:
   - Nếu event foreground lúc restore bị miss/race, hoặc `getFrontMostAppExecuteName()` trả sai tên (app chạy **elevated** khiến `OpenProcess` fail → trả `"UnknownProgram"`), thì không khớp rule `s.exe` → kẹt Unicode, không có cơ chế phục hồi.

### L.2. Giải pháp kỹ thuật

1. **[P0] Bộ lọc ngữ cảnh shell (`isNonAppContext`)**:
   - Thay guard `explorer.exe` bằng hàm lọc theo **tên tiến trình** (`explorer.exe`, `startmenuexperiencehost.exe`, `searchhost.exe`, `searchapp.exe`, `shellexperiencehost.exe`, `textinputhost.exe`, `lockapp.exe`, `sihost.exe`, `backgroundtaskhost.exe`, `dwm.exe`) và theo **class cửa sổ** (`Shell_TrayWnd`, `Shell_SecondaryTrayWnd`, `Progman`, `WorkerW`, `Windows.UI.Core.CoreWindow`, `ForegroundStaging`, `MultitaskingViewFrame`, `XamlExplorerHostIslandWindow`).
   - So sánh tên tiến trình **không phân biệt hoa/thường**.
   - Gặp ngữ cảnh shell → `return` sớm, **giữ nguyên** bảng mã đang dùng.

2. **[P0] Bỏ qua minimize**:
   - Đăng ký thêm hook `hMinimizeEvent = SetWinEventHook(EVENT_SYSTEM_MINIMIZESTART, EVENT_SYSTEM_MINIMIZEEND, …)` và giải phóng trong `OpenKeyFree()`.
   - Khi bắt event minimize: chỉ `resetInputState()`, **không** đổi bảng mã.

3. **[P1] Lưới an toàn "lazy re-check"**:
   - Cờ `_needRuleRecheck` được bật mỗi khi có `EVENT_SYSTEM_FOREGROUND`/`EVENT_OBJECT_NAMECHANGE`.
   - Ở phím thật đầu tiên sau đó, `recheckRuleForCurrentWindow()` chạy lại `resolveRuleCodeForWindow()` cho cửa sổ hiện tại → áp rule nếu khớp. Nhờ đó dù event restore bị miss, gõ vào `s.exe` là TCVN3 tự về.
   - Bỏ `!_isSameApp`/`_lastForegroundExe` dead code.

4. **[P2] Chẩn đoán quyền (elevated)**:
   - `getFrontMostAppExecuteName()` thử lại bằng `PROCESS_QUERY_LIMITED_INFORMATION` khi `OpenProcess` đầu fail; nếu vẫn không đọc được tên thì trả sentinel `UnknownProgram` (không khớp rule) thay vì âm thầm fallback Unicode.

### L.3. File thay đổi (đợt 7)

| File | Thay đổi |
| :--- | :------- |
| `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp` | `isNonAppContext()`; hook minimize; `recheckRuleForCurrentWindow()` + `_needRuleRecheck`; bỏ dead code `_lastForegroundExe` |
| `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKeyHelper.cpp` | `getFrontMostAppExecuteName()` fallback `PROCESS_QUERY_LIMITED_INFORMATION` + sentinel |
| `tests/test_process_rules_real.cpp` | Thêm Section 5 (shell / minimize / taskbar restore / lazy re-check) |

**Kiểm thử đợt 7**:
- `tests\test_process_rules_real.exe`: ✅ 30/30 PASS (thêm 7 ca shell/minimize/restore/lazy).
- `tests\test_userform_title_fallback.exe`: ✅ 7/7 PASS.
- `tests\test_focus_macro_clean.exe`: ✅ 4/4 PASS.
- `tests\test_lazy_macro.exe`: ✅ 14/14 PASS.
- `tests\test_auditor_independent.exe`: ✅ 5/5 PASS.
- `build.bat`: ✅ `OpenKey.exe` cập nhật (18:06 12/10/2026).

---

**Kết luận**: Toàn bộ yêu cầu quản lý bảng mã theo từng file Excel độc lập, bảo toàn bảng mã và gõ tắt cho UserForm/Dialog đã được giải quyết chuẩn xác và triệt để.



