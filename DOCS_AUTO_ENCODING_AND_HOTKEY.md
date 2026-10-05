# Tài liệu Kỹ thuật: Phím tắt Đổi Bảng mã & Nhận diện Tiến trình / Tiêu đề File Tự động (OpenKey)

## 1. Giới thiệu tổng quan
Tài liệu này ghi lại chi tiết quá trình phân tích, thiết kế, triển khai mã nguồn và phản biện kiểm tra cho các tính năng mới được phát triển trên bản fork của **OpenKey** (https://open-key.org/):
1. **Phím tắt chuyển đổi nhanh bảng mã (UniKey-like hotkeys)**:
   - `Ctrl + Shift + F1`: Chuyển sang bảng mã **Unicode** (`vCodeTable = 0`).
   - `Ctrl + Shift + F2`: Chuyển sang bảng mã **TCVN3 (ABC)** (`vCodeTable = 1`).
   - Kèm thông báo Tooltip/Balloon ở khay hệ thống (System Tray) và âm báo (nếu bật tùy chọn âm báo).
2. **Tự động nhận diện phần mềm & tên file / tiêu đề cửa sổ (Process & Window Title Auto Encoding Switch)**:
   - Khi chuyển sang cửa sổ tiến trình `s.exe` $\rightarrow$ Tự động chuyển bảng mã về **TCVN3**.
   - **Nhận diện theo file Excel / tiêu đề cửa sổ**:
     - Mở file Excel tên `"a"` (ví dụ: `a.xlsx`) $\rightarrow$ Tự động chuyển bảng mã về **TCVN3**.
     - Mở file Excel tên `"b"` (ví dụ: `b.xlsx`) $\rightarrow$ Tự động chuyển bảng mã về **Unicode**.
   - **Thuật toán so khớp ranh giới từ (Word-boundary matching)**:
     - Phân tích thông minh các ký tự phân cách khoảng trắng, dấu chấm, gạch ngang, gạch dưới...
     - Tuyệt đối không nhận diện nhầm file có chứa ký tự đơn `a` hay `b` (ví dụ: file `data.xlsx`, `table.xlsx` sẽ không bị kích hoạt nhầm quy tắc của file `a`).
   - **Không tự động fallback về Unicode**: Các ứng dụng và file KHÔNG nằm trong danh sách quy tắc sẽ **giữ nguyên 100% bảng mã hiện tại** của người dùng, không tự ý nhảy về Unicode.
   - Hỗ trợ file cấu hình bên ngoài `process_rules.ini` để người dùng dễ dàng bổ sung ứng dụng và file khác.
3. **Cơ chế Khóa / Bật-Tắt Tự động chuyển bảng mã (Lock / Toggle Mechanism)**:
   - **Menu khay hệ thống**: Bổ sung tùy chọn *"Tự động chuyển bảng mã theo ứng dụng"* có dấu tích chọn (checkmark) trực quan để bật hoặc khóa tính năng bất cứ lúc nào.
   - **Phím tắt nhanh**: Nhấn `Ctrl + Shift + F12` để Bật hoặc Khóa nhanh tính năng này mọi lúc mọi nơi.
   - **File cấu hình**: Có thể đặt `enabled = 1` hoặc `enabled = 0` trong `process_rules.ini`.
4. **Bảo tồn giao diện gốc**:
   - 100% giữ nguyên giao diện, bố cục dialog và khay hệ thống nguyên bản của phần mềm OpenKey.

---

## 2. Phân rã nhiệm vụ 3 Agents

### Agent 1: Kiến trúc sư & Lên kế hoạch (Architect & Planner)
- **Nhiệm vụ**: Phân tích kiến trúc mã nguồn của OpenKey Win32, đề xuất giải pháp kỹ thuật, lập câu hỏi làm rõ các điểm mơ hồ với người dùng.
- **Kết quả làm rõ từ người dùng**:
  - *Cấu hình danh sách ứng dụng*: Hỗ trợ đọc file `process_rules.ini` bên ngoài.
  - *Độ ưu tiên*: So khớp chính xác cả Process + Tiêu đề file (ví dụ `excel.exe[a]`), sau đó đến quy tắc Tiêu đề chung (`title:a`), cuối cùng là quy tắc theo Process (`s.exe`).
  - *Không fallback*: Ứng dụng/file khác không có quy tắc thì giữ nguyên bảng mã đang dùng.
  - *Cơ chế Khóa*: Cung cấp công tắc khóa để tắt tự động chuyển bảng mã khi cần.
  - *Phản hồi*: Hiển thị thông báo nhỏ (Tray notification / Balloon tooltip) ở góc màn hình khi đổi bảng mã.

### Agent 2: Kỹ sư Triển khai Mã nguồn (Software Engineer)
- **Nhiệm vụ**: Trực tiếp viết code vào hệ thống mã nguồn OpenKey Win32.
- **Các thành phần đã phát triển**:
  1. `ProcessRuleHelper.h` & `ProcessRuleHelper.cpp`: Module quản lý danh sách quy tắc ánh xạ (tiến trình, tiêu đề cửa sổ) $\rightarrow$ bảng mã, tự động sinh và nạp `process_rules.ini`, xử lý chuẩn hóa không phân biệt hoa thường, thuật toán tách từ `matchesTitle`, hỗ trợ tham số `enabled = 1/0`.
  2. `OpenKeyHelper.h` & `OpenKeyHelper.cpp`:
     - Bổ sung hàm `OpenKeyHelper::getFrontMostWindowTitleUtf8()` đọc tiêu đề cửa sổ UTF-8 qua `GetWindowTextW`.
  3. `SystemTrayHelper.h` & `SystemTrayHelper.cpp`:
     - Bổ sung hàm `SystemTrayHelper::showNotification` gửi thông báo Balloon qua Windows Shell Notification API.
     - Bổ sung mục menu `POPUP_AUTO_SWITCH_CODETABLE` ("Tự động chuyển bảng mã theo ứng dụng") kèm checkmark.
  4. `AppDelegate.h` & `AppDelegate.cpp`:
     - Bổ sung biến toàn cục `vAutoSwitchCodeTable` lưu cấu hình vào Registry.
     - Triển khai hàm `onToggleAutoSwitchCodeTable()`.
  5. `OpenKey.cpp`:
     - Bổ sung bắt phím `Ctrl + Shift + F1` (Unicode), `Ctrl + Shift + F2` (TCVN3) và `Ctrl + Shift + F12` (Khóa/Mở khóa tự động).
     - Đăng ký 2 WinEventHook: `EVENT_SYSTEM_FOREGROUND` (chuyển cửa sổ) và `EVENT_OBJECT_NAMECHANGE` (đổi tên/mở tài liệu trong cửa sổ hiện tại).
     - Nhận diện cả tiến trình lẫn tiêu đề cửa sổ trong `winEventProcCallback`.
     - Loại bỏ việc ép fallback về Unicode đối với ứng dụng/file không có rule.
     - Khởi tạo quy tắc trong `OpenKeyInit()` và giải phóng hook an toàn trong `OpenKeyFree()`.
  6. `process_rules.ini`: File cấu hình mẫu hỗ trợ cú pháp `process[title] = CODETABLE`.
  7. `build.bat`: Kịch bản biên dịch tự động chuẩn UTF-8 (codepage 65001).

### Agent 3: Chuyên gia Phản biện, Kiểm thử & Lập tài liệu (Reviewer & QA)
- **Nhiệm vụ**: Đánh giá kiến trúc, kiểm tra các ca biên (edge-cases), xác minh tính tương thích và lập tài liệu kỹ thuật hoàn chỉnh.

---

## 3. Chi tiết triển khai mã nguồn (Implementation Details)

### 3.1. Phím tắt `Ctrl + Shift + F1`, `F2` & `F12`
Vị trí: `keyboardHookProcess` trong [`OpenKey.cpp`](Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp).

```cpp
// Hotkeys: Ctrl + Shift + F1 (Unicode), Ctrl + Shift + F2 (TCVN3), Ctrl + Shift + F12 (Khóa/Mở khóa)
if ((_flag & MASK_CONTROL) && (_flag & MASK_SHIFT) && !(_flag & MASK_ALT) && !(_flag & MASK_WIN)) {
    if (_keycode == VK_F1) {
        AppDelegate::getInstance()->onTableCode(0);
        SystemTrayHelper::updateData();
        SystemTrayHelper::showNotification(_T("OpenKey"), _T("Bảng mã: Unicode"));
        if (HAS_BEEP(vSwitchKeyStatus)) {
            MessageBeep(MB_OK);
        }
        _hasJustUsedHotKey = true;
        _keycode = 0;
        return -1;
    } else if (_keycode == VK_F2) {
        AppDelegate::getInstance()->onTableCode(1);
        SystemTrayHelper::updateData();
        SystemTrayHelper::showNotification(_T("OpenKey"), _T("Bảng mã: TCVN3 (ABC)"));
        if (HAS_BEEP(vSwitchKeyStatus)) {
            MessageBeep(MB_OK);
        }
        _hasJustUsedHotKey = true;
        _keycode = 0;
        return -1;
    } else if (_keycode == VK_F12) {
        AppDelegate::getInstance()->onToggleAutoSwitchCodeTable();
        _hasJustUsedHotKey = true;
        _keycode = 0;
        return -1;
    }
}
```

---

### 3.2. Bắt sự kiện chuyển cửa sổ và đổi tên tiêu đề (WinEventHook)
Vị trí: `OpenKeyInit` và `winEventProcCallback` trong [`OpenKey.cpp`](Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp).

```cpp
// Khởi tạo Hook
hSystemEvent = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL, winEventProcCallback, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
hTitleEvent = SetWinEventHook(EVENT_OBJECT_NAMECHANGE, EVENT_OBJECT_NAMECHANGE, NULL, winEventProcCallback, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

// Xử lý callback
VOID CALLBACK winEventProcCallback(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
    if (dwEvent == EVENT_OBJECT_NAMECHANGE) {
        // Chỉ xử lý khi đúng đối tượng cửa sổ và là cửa sổ đang active
        if (idObject != OBJID_WINDOW || hwnd != GetForegroundWindow())
            return;
    }

    string& exe = OpenKeyHelper::getFrontMostAppExecuteName();
    if (exe.compare("explorer.exe") == 0)
        return;

    // 1. Kiểm tra quy tắc nhận diện ứng dụng & tiêu đề cửa sổ (chỉ chạy khi không bị khóa)
    if (vAutoSwitchCodeTable) {
        string title = OpenKeyHelper::getFrontMostWindowTitleUtf8();
        int ruleCode = ProcessRuleHelper::getCodeTableForProcessAndTitle(exe, title);
        if (ruleCode != -1) {
            if (vCodeTable != ruleCode) {
                AppDelegate::getInstance()->onTableCode(ruleCode);
                SystemTrayHelper::updateData();
            }
        }
        // Nếu ruleCode == -1: GIỮ NGUYÊN vCodeTable, KHÔNG fallback về Unicode!
    }

    // 2. Chức năng nhớ ngôn ngữ Anh/Việt (SmartSwitchKey - chỉ chạy khi chuyển cửa sổ)
    if (dwEvent == EVENT_SYSTEM_FOREGROUND && vUseSmartSwitchKey) {
        _languageTemp = getAppInputMethodStatus(exe, vLanguage | (vCodeTable << 1));
        vTempOffEngine(false);
        if ((_languageTemp & 0x01) != vLanguage) {
            if (_languageTemp != -1) {
                vLanguage = _languageTemp;
                AppDelegate::getInstance()->onInputMethodChangedFromHotKey();
            } else {
                saveSmartSwitchKeyData();
            }
        }
    }

    if (dwEvent == EVENT_SYSTEM_FOREGROUND) {
        startNewSession();
        ...
    }
}
```

---

### 3.3. Thuật toán so khớp ranh giới từ (Word-boundary matching)
Vị trí: `matchesTitle` trong [`ProcessRuleHelper.cpp`](Sources/OpenKey/win32/OpenKey/OpenKey/ProcessRuleHelper.cpp).

Giúp phân biệt chính xác file tên `a` (như `a.xlsx - Excel`, `a - Excel`) với các file có chứa chữ `a` nhưng không phải là file `a` (như `data.xlsx`, `table.xlsx`, `sales.xlsx`):

```cpp
bool ProcessRuleHelper::matchesTitle(const std::string& title, const std::string& pattern) {
    if (pattern.empty()) return true;
    if (title.empty()) return false;

    size_t pos = 0;
    while ((pos = title.find(pattern, pos)) != std::string::npos) {
        bool leftBoundary = (pos == 0) || isDelimiter(title[pos - 1]);
        size_t after = pos + pattern.size();
        bool rightBoundary = (after >= title.size()) || isDelimiter(title[after]);

        if (leftBoundary && rightBoundary) {
            return true;
        }
        pos += pattern.size();
    }
    return false;
}
```

---

### 3.4. Cấu hình `process_rules.ini`

```ini
# ============================================================
# OpenKey - Cau hinh tu dong nhan dien tien trinh & bang ma
#
# Cai dat khoa / bat tinh nang:
#   enabled = 1   (1: Bat tu dong chuyen, 0: Khoa / Tat)
#
# Dinh dang quy tac ho tro:
# 1. Theo tien trinh:
#      [ten_tien_trinh] = [bang_ma]
#      Vi du: s.exe = TCVN3
#             chrome.exe = UNICODE
#             zalo.exe = UNICODE
#
# 2. Theo tien trinh + ten file / tieu de cua so:
#      [ten_tien_trinh][ten_file] = [bang_ma]
#      Vi du: excel.exe[a] = TCVN3
#             excel.exe[b] = UNICODE
#
# 3. Theo tieu de / ten file chung:
#      title:[ten_file] = [bang_ma]
#      Vi du: title:a = TCVN3
#             title:b = UNICODE
#
# Bang ma ho tro:
#   0 hoac UNICODE          : Unicode dung san
#   1 hoac TCVN3            : TCVN3 (ABC)
#   2 hoac VNI              : VNI Windows
#   3 hoac UNICODE_COMPOUND : Unicode to hop
#   4 hoac VN_LOCALE_1258   : Vietnamese locale CP 1258
#
# Luu y: Cac phan mem / file KHONG co trong danh sach se
# GIU NGUYEN bang ma hien tai, KHONG tu dong fallback ve Unicode.
# ============================================================

enabled = 1

s.exe = TCVN3
excel.exe[a] = TCVN3
excel.exe[b] = UNICODE
chrome.exe = UNICODE
zalo.exe = UNICODE
```

---

## 4. Hướng dẫn sử dụng & Kiểm tra
1. **Phím tắt đổi bảng mã**:
   - `Ctrl + Shift + F1`: Chuyển sang **Unicode**.
   - `Ctrl + Shift + F2`: Chuyển sang **TCVN3 (ABC)**.
2. **Khóa / Mở khóa tự động chuyển bảng mã**:
   - **Cách 1**: Chuột phải vào biểu tượng OpenKey ở khay hệ thống $\rightarrow$ Nhấp vào dòng **"Tự động chuyển bảng mã theo ứng dụng"** để bỏ dấu tick (Khóa) hoặc bật dấu tick (Mở khóa).
   - **Cách 2**: Bấm phím tắt **`Ctrl + Shift + F12`** bất cứ lúc nào. Thông báo Balloon sẽ hiển thị: *"Đã KHÓA tự động chuyển bảng mã"* hoặc *"Đã BẬT tự động chuyển bảng mã theo ứng dụng"*.
   - **Cách 3**: Đặt `enabled = 0` trong file `process_rules.ini`.
3. **Nhận diện theo tên file Excel (`a` $\rightarrow$ TCVN3, `b` $\rightarrow$ Unicode)**:
   - Mở file Excel tên `a` (ví dụ `a.xlsx`): Bảng mã tự chuyển sang **TCVN3**.
   - Chuyển sang file Excel tên `b` (ví dụ `b.xlsx`): Bảng mã tự chuyển sang **Unicode**.
   - Mở file Excel khác (ví dụ `data.xlsx` hay `baocao.xlsx`): **Bảng mã giữ nguyên**, không bị nhận diện nhầm.
4. **Hoạt động giữ nguyên bảng mã (No Fallback)**:
   - Khi vào `s.exe`: Bảng mã tự chuyển sang **TCVN3**.
   - Khi chuyển sang Notepad, Word, trình duyệt hoặc bất kỳ app nào khác (không có trong rules): **Bảng mã vẫn giữ nguyên là TCVN3** (hoặc bảng mã bạn vừa chọn), tuyệt đối không bị tự động nhảy về Unicode nữa.
