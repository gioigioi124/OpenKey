# All_fix.md — Tổng hợp Tài liệu Kỹ thuật & Nhật ký Sửa lỗi OpenKey Win32

> **Dự án**: OpenKey Windows (Bộ gõ tiếng Việt mã nguồn mở — fork)
> **Ngày tổng hợp**: 12/10/2026
> **Phạm vi**: Gộp toàn bộ tài liệu kỹ thuật của các tính năng đã phát triển trước đó và ghi nhận đợt rà soát — sửa lỗi mới nhất.
> **Trạng thái build**: ✅ `build.bat` thành công (Exit code 0), `OpenKey.exe` đã cập nhật.

Tài liệu này hợp nhất nội dung của:

- `DOCS_AUTO_ENCODING_AND_HOTKEY.md` — Phím tắt đổi bảng mã, nhận diện tiến trình/tiêu đề, fallback Unicode.
- `DOCS_LAZY_MACRO_CONVERSION.md` — Cơ chế gõ tắt dịch động (On-Demand / Lazy JIT).
- `DOCS_MACRO_TABLECODE_SYNC.md` — Đồng bộ bảng mã cho tính năng gõ tắt.
- **MỚI**: Đợt rà soát & sửa lỗi (Review & Fix) — 12/10/2026.

---

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

**Kết luận**: Toàn bộ lỗi P0/P1/P2 (đợt 1) và 6 vấn đề Phần E (đợt 2) đã được sửa, dự án biên dịch thành công. Tài liệu được hợp nhất vào `All_fix.md` này.
