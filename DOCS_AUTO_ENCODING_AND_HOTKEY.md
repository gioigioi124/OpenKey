# Tài liệu Kỹ thuật: Phím tắt Đổi Bảng mã & Nhận diện Tiến trình Tự động (OpenKey)

## 1. Giới thiệu tổng quan
Tài liệu này ghi lại chi tiết quá trình phân tích, thiết kế, triển khai mã nguồn và phản biện kiểm tra cho các tính năng mới được phát triển trên bản fork của **OpenKey** (https://open-key.org/):
1. **Phím tắt chuyển đổi nhanh bảng mã (UniKey-like hotkeys)**:
   - `Ctrl + Shift + F1`: Chuyển sang bảng mã **Unicode** (`vCodeTable = 0`).
   - `Ctrl + Shift + F2`: Chuyển sang bảng mã **TCVN3 (ABC)** (`vCodeTable = 1`).
   - Kèm thông báo Tooltip/Balloon ở khay hệ thống (System Tray) và âm báo (nếu bật tùy chọn âm báo).
2. **Tự động nhận diện phần mềm đang hoạt động (Process-based Auto Encoding Switch)**:
   - Khi chuyển sang cửa sổ tiến trình `s.exe` $\rightarrow$ Tự động chuyển bảng mã về **TCVN3**.
   - **Không tự động fallback về Unicode**: Các ứng dụng KHÔNG nằm trong danh sách quy tắc sẽ **giữ nguyên 100% bảng mã hiện tại** của người dùng, không tự ý nhảy về Unicode.
   - Hỗ trợ file cấu hình bên ngoài `process_rules.ini` để người dùng dễ dàng bổ sung ứng dụng khác.
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
  - *Cấu hình danh sách ứng dụng*: Mặc định `s.exe` $\rightarrow$ TCVN3, kèm hỗ trợ đọc file `process_rules.ini` bên ngoài.
  - *Độ ưu tiên*: Tự động chuyển bảng mã khi kích hoạt cửa sổ; nếu người dùng ấn phím tắt thủ công trong ứng dụng đó thì vẫn cho phép đổi tạm thời cho phiên đó.
  - *Không fallback*: Ứng dụng khác không có quy tắc thì giữ nguyên bảng mã đang dùng.
  - *Cơ chế Khóa*: Cung cấp công tắc khóa để tắt tự động chuyển bảng mã khi cần.
  - *Phản hồi*: Hiển thị thông báo nhỏ (Tray notification / Balloon tooltip) ở góc màn hình khi đổi bảng mã.

### Agent 2: Kỹ sư Triển khai Mã nguồn (Software Engineer)
- **Nhiệm vụ**: Trực tiếp viết code vào hệ thống mã nguồn OpenKey Win32.
- **Các thành phần đã phát triển**:
  1. `ProcessRuleHelper.h` & `ProcessRuleHelper.cpp`: Module quản lý danh sách quy tắc ánh xạ tiến trình $\rightarrow$ bảng mã, tự động sinh và nạp `process_rules.ini`, xử lý chuẩn hóa tên tiến trình không phân biệt hoa thường, hỗ trợ tham số `enabled = 1/0`.
  2. `SystemTrayHelper.h` & `SystemTrayHelper.cpp`:
     - Bổ sung hàm `SystemTrayHelper::showNotification` gửi thông báo Balloon qua Windows Shell Notification API.
     - Bổ sung mục menu `POPUP_AUTO_SWITCH_CODETABLE` ("Tự động chuyển bảng mã theo ứng dụng") kèm checkmark.
  3. `AppDelegate.h` & `AppDelegate.cpp`:
     - Bổ sung biến toàn cục `vAutoSwitchCodeTable` lưu cấu hình vào Registry.
     - Triển khai hàm `onToggleAutoSwitchCodeTable()`.
  4. `OpenKey.cpp`:
     - Bổ sung bắt phím `Ctrl + Shift + F1` (Unicode), `Ctrl + Shift + F2` (TCVN3) và `Ctrl + Shift + F12` (Khóa/Mở khóa tự động).
     - Bổ sung cơ chế nhận diện foreground process trong `winEventProcCallback`, loại bỏ việc ép fallback về Unicode đối với ứng dụng không có rule.
     - Khởi tạo quy tắc trong `OpenKeyInit()`.
  5. `OpenKey.vcxproj` & `OpenKey.vcxproj.filters`: Cập nhật cấu hình build Visual Studio để tích hợp `ProcessRuleHelper`.
  6. `process_rules.ini`: File cấu hình mẫu với các giá trị mặc định.
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

### 3.2. Tự động nhận diện ứng dụng (Không fallback về Unicode)
Vị trí: `winEventProcCallback` trong [`OpenKey.cpp`](Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp).

```cpp
VOID CALLBACK winEventProcCallback(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
    string& exe = OpenKeyHelper::getFrontMostAppExecuteName();
    if (exe.compare("explorer.exe") == 0)
        return;

    // 1. Kiểm tra quy tắc nhận diện ứng dụng (chỉ chạy khi không bị khóa)
    if (vAutoSwitchCodeTable) {
        int ruleCode = ProcessRuleHelper::getCodeTableForProcess(exe);
        if (ruleCode != -1) {
            if (vCodeTable != ruleCode) {
                AppDelegate::getInstance()->onTableCode(ruleCode);
                SystemTrayHelper::updateData();
            }
        }
        // Nếu ruleCode == -1: GIỮ NGUYÊN vCodeTable, KHÔNG fallback về Unicode!
    }

    // 2. Chức năng nhớ ngôn ngữ Anh/Việt (SmartSwitchKey)
    if (vUseSmartSwitchKey) {
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

    startNewSession();
    ...
}
```

---

### 3.3. File cấu hình `process_rules.ini`

```ini
# ============================================================
# OpenKey - Cấu hình tự động nhận diện tiến trình & bảng mã
#
# Cài đặt khóa / bật tính năng:
#   enabled = 1   (1: Bật tự động chuyển, 0: Khóa / Tắt)
#
# Định dạng quy tắc: [tên_tiến_trình] = [bảng_mã]
# Bảng mã hỗ trợ:
#   0 hoặc UNICODE          : Unicode dựng sẵn
#   1 hoặc TCVN3            : TCVN3 (ABC)
#   2 hoặc VNI              : VNI Windows
#   3 hoặc UNICODE_COMPOUND : Unicode tổ hợp
#   4 hoặc VN_LOCALE_1258   : Vietnamese locale CP 1258
#
# Lưu ý: Các phần mềm KHÔNG có trong danh sách sẽ GIỮ NGUYÊN
# bảng mã hiện tại, KHÔNG tự động fallback về Unicode.
# ============================================================

enabled = 1

s.exe = TCVN3
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
3. **Hoạt động giữ nguyên bảng mã (No Fallback)**:
   - Khi vào `s.exe`: Bảng mã tự chuyển sang **TCVN3**.
   - Khi chuyển sang Notepad, Word, trình duyệt hoặc bất kỳ app nào khác (không có trong rules): **Bảng mã vẫn giữ nguyên là TCVN3** (hoặc bảng mã bạn vừa chọn), tuyệt đối không bị tự động nhảy về Unicode nữa.
