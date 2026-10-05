# Tài liệu Kỹ thuật: Phím tắt Đổi Bảng mã & Nhận diện Tiến trình Tự động (OpenKey)

## 1. Giới thiệu tổng quan
Tài liệu này ghi lại chi tiết quá trình phân tích, thiết kế, triển khai mã nguồn và phản biện kiểm tra cho 2 tính năng mới được phát triển trên bản fork của **OpenKey** (https://open-key.org/):
1. **Phím tắt chuyển đổi nhanh bảng mã (UniKey-like hotkeys)**:
   - `Ctrl + Shift + F1`: Chuyển sang bảng mã **Unicode** (`vCodeTable = 0`).
   - `Ctrl + Shift + F2`: Chuyển sang bảng mã **TCVN3 (ABC)** (`vCodeTable = 1`).
   - Kèm thông báo Tooltip/Balloon ở khay hệ thống (System Tray) và âm báo nếu bật chế độ âm báo.
2. **Tự động nhận diện phần mềm đang hoạt động (Process-based Auto Encoding Switch)**:
   - Khi chuyển sang cửa sổ tiến trình `s.exe` $\rightarrow$ Tự động chuyển bảng mã về **TCVN3**.
   - Khi chuyển sang cửa sổ tiến trình `chrome.exe` hoặc `zalo.exe` $\rightarrow$ Tự động chuyển bảng mã về **Unicode**.
   - Hỗ trợ file cấu hình bên ngoài `process_rules.ini` để người dùng dễ dàng bổ sung ứng dụng khác mà không phải can thiệp code.
3. **Bảo tồn giao diện gốc**:
   - 100% giữ nguyên giao diện, bố cục dialog và khay hệ thống nguyên bản của phần mềm OpenKey.

---

## 2. Phân rã nhiệm vụ 3 Agents

### Agent 1: Kiến trúc sư & Lên kế hoạch (Architect & Planner)
- **Nhiệm vụ**: Phân tích kiến trúc mã nguồn của OpenKey Win32, đề xuất giải pháp kỹ thuật, lập câu hỏi làm rõ các điểm mơ hồ với người dùng.
- **Kết quả làm rõ từ người dùng**:
  - *Cấu hình danh sách ứng dụng*: Mặc định có sẵn `s.exe` (TCVN3), `chrome.exe`/`zalo.exe` (Unicode), kèm hỗ trợ đọc file `process_rules.ini` bên ngoài.
  - *Độ ưu tiên*: Tự động chuyển bảng mã khi kích hoạt cửa sổ; nếu người dùng ấn phím tắt thủ công trong ứng dụng đó thì vẫn cho phép đổi tạm thời cho phiên đó.
  - *Phản hồi*: Hiển thị thông báo nhỏ (Tray notification / Balloon tooltip) ở góc màn hình khi đổi bảng mã.

### Agent 2: Kỹ sư Triển khai Mã nguồn (Software Engineer)
- **Nhiệm vụ**: Trực tiếp viết code vào hệ thống mã nguồn OpenKey Win32.
- **Các thành phần đã phát triển**:
  1. `ProcessRuleHelper.h` & `ProcessRuleHelper.cpp`: Module quản lý danh sách quy tắc ánh xạ tiến trình $\rightarrow$ bảng mã, tự động sinh và nạp `process_rules.ini`, xử lý chuẩn hóa tên tiến trình không phân biệt hoa thường.
  2. `SystemTrayHelper.h` & `SystemTrayHelper.cpp`: Bổ sung hàm `SystemTrayHelper::showNotification` gửi thông báo Balloon Notification qua Windows Shell Notification API (`Shell_NotifyIcon`).
  3. `OpenKey.cpp`:
     - Bổ sung bắt phím `Ctrl + Shift + F1` và `Ctrl + Shift + F2` trong hàm hook bàn phím `keyboardHookProcess`.
     - Bổ sung cơ chế nhận diện foreground process và áp dụng bảng mã trong `winEventProcCallback`.
     - Khởi tạo quy tắc trong `OpenKeyInit()`.
  4. `OpenKey.vcxproj` & `OpenKey.vcxproj.filters`: Cập nhật cấu hình build Visual Studio để tích hợp `ProcessRuleHelper`.
  5. `process_rules.ini`: File cấu hình mẫu với các giá trị mặc định.

### Agent 3: Chuyên gia Phản biện, Kiểm thử & Lập tài liệu (Reviewer & QA)
- **Nhiệm vụ**: Đánh giá kiến trúc, kiểm tra các ca biên (edge-cases), xác minh tính tương thích và lập tài liệu kỹ thuật hoàn chỉnh.

---

## 3. Chi tiết triển khai mã nguồn (Implementation Details)

### 3.1. Phím tắt `Ctrl + Shift + F1` & `Ctrl + Shift + F2`
Vị trí: `keyboardHookProcess` trong [`OpenKey.cpp`](Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp).

```cpp
// Hotkeys: Ctrl + Shift + F1 (Unicode), Ctrl + Shift + F2 (TCVN3)
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
        return -1; // Ngăn không cho phím F1 gửi tới ứng dụng đích
    } else if (_keycode == VK_F2) {
        AppDelegate::getInstance()->onTableCode(1);
        SystemTrayHelper::updateData();
        SystemTrayHelper::showNotification(_T("OpenKey"), _T("Bảng mã: TCVN3 (ABC)"));
        if (HAS_BEEP(vSwitchKeyStatus)) {
            MessageBeep(MB_OK);
        }
        _hasJustUsedHotKey = true;
        _keycode = 0;
        return -1; // Ngăn không cho phím F2 gửi tới ứng dụng đích
    }
}
```

**Cơ chế hoạt động**:
- Khi nhấn tổ hợp phím, `onTableCode(code)` được gọi:
  - Cập nhật biến `vCodeTable`.
  - Lưu cấu hình vào Windows Registry (`APP_SET_DATA`).
  - Làm mới hộp thoại điều khiển chính nếu đang mở (`mainDialog->fillData()`).
- `SystemTrayHelper::updateData()`: Cập nhật dấu tích chọn (checkmark) trên menu chuột phải ở khay hệ thống.
- `SystemTrayHelper::showNotification(...)`: Hiển thị balloon thông báo trực quan.
- Gán `_hasJustUsedHotKey = true` để tránh nhầm lẫn với hotkey chuyển ngôn ngữ Ctrl+Shift thông thường.
- Trả về `-1` để triệt tiêu sự kiện bàn phím, tránh việc ứng dụng đang dùng nhận được phím `F1` (thường mở Help).

---

### 3.2. Tự động nhận diện ứng dụng (Process Recognition)
Vị trí: `winEventProcCallback` trong [`OpenKey.cpp`](Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp).

```cpp
VOID CALLBACK winEventProcCallback(HWINEVENTHOOK hWinEventHook, DWORD dwEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
    string& exe = OpenKeyHelper::getFrontMostAppExecuteName();
    if (exe.compare("explorer.exe") == 0)
        return;

    // 1. Kiểm tra quy tắc nhận diện ứng dụng
    int ruleCode = ProcessRuleHelper::getCodeTableForProcess(exe);
    if (ruleCode != -1) {
        if (vCodeTable != ruleCode) {
            AppDelegate::getInstance()->onTableCode(ruleCode);
            SystemTrayHelper::updateData();
        }
    }

    // 2. Chức năng nhớ bảng mã & ngôn ngữ (SmartSwitchKey)
    if (vUseSmartSwitchKey || vRememberCode) {
        _languageTemp = getAppInputMethodStatus(exe, vLanguage | (vCodeTable << 1));
        vTempOffEngine(false);
        if (vUseSmartSwitchKey && (_languageTemp & 0x01) != vLanguage) {
            if (_languageTemp != -1) {
                vLanguage = _languageTemp;
                AppDelegate::getInstance()->onInputMethodChangedFromHotKey();
            } else {
                saveSmartSwitchKeyData();
            }
        }
        startNewSession();
        // Nếu ứng dụng nằm trong danh sách quy tắc, quy tắc có độ ưu tiên cao hơn
        if (ruleCode == -1 && vRememberCode && (_languageTemp >> 1) != vCodeTable) {
            if (_languageTemp != -1) {
                AppDelegate::getInstance()->onTableCode(_languageTemp >> 1);
            } else {
                saveSmartSwitchKeyData();
            }
        }
    } else {
        startNewSession();
    }
    ...
}
```

---

### 3.3. File cấu hình `process_rules.ini`
File được tự động tìm kiếm hoặc khởi tạo cùng thư mục với `OpenKey.exe` (hoặc `OpenKey64.exe`):

```ini
# ============================================================
# OpenKey - Cấu hình tự động nhận diện tiến trình & bảng mã
# Định dạng: [tên_tiến_trình] = [bảng_mã]
# Bảng mã hỗ trợ:
#   0 hoặc UNICODE          : Unicode dựng sẵn
#   1 hoặc TCVN3            : TCVN3 (ABC)
#   2 hoặc VNI              : VNI Windows
#   3 hoặc UNICODE_COMPOUND : Unicode tổ hợp
#   4 hoặc VN_LOCALE_1258   : Vietnamese locale CP 1258
# ============================================================

s.exe = TCVN3
chrome.exe = UNICODE
zalo.exe = UNICODE
```

---

## 4. Báo cáo Phản biện & Đánh giá Kiểm thử (Agent 3 Review)

| Tiêu chí phản biện | Phân tích kỹ thuật & Đánh giá | Trạng thái |
| :--- | :--- | :--- |
| **Tránh xung đột phím (Hotkey Collision)** | Điều kiện yêu cầu chính xác `Ctrl + Shift` mà **không** có `Alt` hay `Win`. Khi bấm `Ctrl+Shift+F1/F2`, hook trả về `-1` ngay lập tức để ứng dụng foreground không nhận phím `F1`/`F2` (tránh bật menu Trợ giúp). | ĐẠT |
| **Tránh kích hoạt nhầm chuyển ngôn ngữ** | OpenKey cho phép đổi Anh/Việt bằng `Ctrl+Shift`. Đoạn code đã set cờ `_hasJustUsedHotKey = true`, nhờ đó khi người dùng nhả phím `Shift`/`Ctrl`, OpenKey sẽ không kích hoạt toggle ngôn ngữ. | ĐẠT |
| **Không gây lag gõ phím (Performance)** | Kiểm tra phím tắt trong `keyboardHookProcess` là $O(1)$. Việc nhận diện process chỉ diễn ra một lần duy nhất khi chuyển cửa sổ foreground trong sự kiện hệ thống `EVENT_SYSTEM_FOREGROUND`, hoàn toàn không can thiệp hay đọc file trong quá trình gõ chữ liên tục. | ĐẠT |
| **Phân biệt hoa thường (Case Insensitivity)** | Tên tiến trình trên Windows có thể là `S.EXE`, `s.exe`, `Chrome.exe`. Module `ProcessRuleHelper` chuẩn hóa tất cả về chữ thường (`toLower`) trước khi đối chiếu map, đảm bảo nhận diện chính xác 100%. | ĐẠT |
| **Cô lập phiên gõ chữ (Session Isolation)** | Khi chuyển cửa sổ, hàm `startNewSession()` được gọi để dọn sạch bộ đệm ký tự dở dang từ cửa sổ cũ, ngăn chặn hiện tượng backspace xóa nhầm nội dung trên ứng dụng mới. | ĐẠT |
| **Độ ưu tiên giữa Auto Rule & Hotkey thủ công** | Khi người dùng chuyển vào `s.exe`, bảng mã tự chuyển về TCVN3. Nếu người dùng bấm `Ctrl+Shift+F1` để gõ Unicode trong `s.exe`, hệ thống cho phép giữ Unicode. Khi chuyển ra ngoài rồi quay lại `s.exe`, sự kiện kích hoạt lại chuyển về TCVN3 theo đúng mong muốn. | ĐẠT |
| **Bảo tồn 100% giao diện gốc** | Không thay đổi bất kỳ file `.rc`, dialog hay icon nào trong giao diện đồ họa. Người dùng tùy biến thông qua file `.ini` gọn nhẹ. | ĐẠT |

---

## 5. Hướng dẫn sử dụng
1. **Sử dụng phím tắt**:
   - Nhấn `Ctrl + Shift + F1` bất kỳ lúc nào để chuyển sang bảng mã **Unicode**.
   - Nhấn `Ctrl + Shift + F2` bất kỳ lúc nào để chuyển sang bảng mã **TCVN3 (ABC)**.
   - Một thông báo nhỏ sẽ xuất hiện ở góc phải màn hình xác nhận bảng mã hiện tại.
2. **Sử dụng tự động theo ứng dụng**:
   - Mở phần mềm `s.exe`: Bảng mã sẽ tự động chuyển thành **TCVN3**.
   - Chuyển sang `chrome.exe` hoặc `zalo.exe`: Bảng mã sẽ tự động chuyển thành **Unicode**.
   - Để thêm ứng dụng khác, mở file `process_rules.ini` và thêm dòng mới (ví dụ: `excel.exe = UNICODE` hoặc `foxpro.exe = TCVN3`).
