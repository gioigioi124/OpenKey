# Tài liệu Kỹ thuật: Phím tắt Đổi Bảng mã, Nhận diện Tiến trình / Tiêu đề File & Tùy chọn Fallback về Unicode (OpenKey)

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
     - Hỗ trợ cả tên file thực tế của người dùng cấu hình trong `process_rules.ini` (ví dụ: `excel.exe[Tong hop ban hang - Date] = TCVN3`).
   - **Thuật toán so khớp ranh giới từ (Word-boundary matching)**:
     - Phân tích thông minh các ký tự phân cách khoảng trắng, dấu chấm, gạch ngang, gạch dưới...
     - Tuyệt đối không nhận diện nhầm file có chứa ký tự đơn `a` hay `b` (ví dụ: file `data.xlsx`, `table.xlsx` sẽ không bị kích hoạt nhầm quy tắc của file `a`).
3. **Tính năng Fallback về Unicode có thể bật/tắt linh hoạt (Configurable Fallback to Unicode)**:
   - **Bật / Tắt trực tiếp tại Menu khay hệ thống (Tray)**: Bổ sung tùy chọn *"Fallback về Unicode khi rời ứng dụng"* có dấu tích chọn (checkmark) trực quan.
   - **Khi TẮT Fallback (Mặc định)**: Các ứng dụng/file KHÔNG có trong quy tắc sẽ **giữ nguyên 100% bảng mã hiện tại**, không tự ý nhảy về Unicode.
   - **Khi BẬT Fallback**: Khi người dùng rời khỏi ứng dụng/file có quy tắc sang một ứng dụng thông thường khác (như Notepad, Word, Browser...), OpenKey sẽ tự động chuyển bảng mã về **Unicode**.
   - Đồng bộ lưu trữ trong Windows Registry (`vFallbackToUnicode`) và hỗ trợ cấu hình qua `process_rules.ini` (`fallback_to_unicode = 0` hoặc `1`).
4. **Cơ chế Khóa / Bật-Tắt Tự động chuyển bảng mã (Lock / Toggle Mechanism)**:
   - **Menu khay hệ thống**: Tùy chọn *"Tự động chuyển bảng mã theo ứng dụng"* có dấu tích chọn (checkmark) để bật hoặc khóa tính năng bất cứ lúc nào.
   - **Phím tắt nhanh**: Nhấn `Ctrl + Shift + F12` để Bật hoặc Khóa nhanh tính năng này.
   - **File cấu hình**: Có thể đặt `enabled = 1` hoặc `enabled = 0` trong `process_rules.ini`.
5. **Bảo tồn giao diện gốc**:
   - 100% giữ nguyên giao diện, bố cục dialog và khay hệ thống nguyên bản của phần mềm OpenKey.

---

## 2. Phân rã nhiệm vụ 3 Agents

### Agent 1: Kiến trúc sư & Lên kế hoạch (Architect & Planner)
- **Nhiệm vụ**: Phân tích kiến trúc mã nguồn của OpenKey Win32, đề xuất giải pháp kỹ thuật, lập câu hỏi làm rõ các điểm mơ hồ với người dùng.
- **Kết quả làm rõ từ người dùng**:
  - *Cấu hình danh sách ứng dụng*: Hỗ trợ đọc file `process_rules.ini` bên ngoài.
  - *Độ ưu tiên*: So khớp chính xác cả Process + Tiêu đề file (ví dụ `excel.exe[a]`), sau đó đến quy tắc Tiêu đề chung (`title:a`), cuối cùng là quy tắc theo Process (`s.exe`).
  - *Tùy chọn Fallback*: Hỗ trợ công tắc bật/tắt Fallback về Unicode ngay trên Menu khay hệ thống (Tray). Mặc định là tắt (giữ nguyên bảng mã).
  - *Cơ chế Khóa*: Cung cấp công tắc khóa để tắt tự động chuyển bảng mã khi cần.
  - *Phản hồi*: Hiển thị thông báo nhỏ (Tray notification / Balloon tooltip) ở góc màn hình khi đổi bảng mã hoặc bật/tắt thiết lập.

### Agent 2: Kỹ sư Triển khai Mã nguồn (Software Engineer)
- **Nhiệm vụ**: Trực tiếp viết code vào hệ thống mã nguồn OpenKey Win32.
- **Các thành phần đã phát triển**:
  1. `ProcessRuleHelper.h` & `ProcessRuleHelper.cpp`: Module quản lý danh sách quy tắc ánh xạ (tiến trình, tiêu đề cửa sổ) $\rightarrow$ bảng mã, tự động sinh và nạp `process_rules.ini`, hỗ trợ tham số `enabled = 1/0` và `fallback_to_unicode = 1/0`.
  2. `OpenKeyHelper.h` & `OpenKeyHelper.cpp`:
     - Bổ sung hàm `OpenKeyHelper::getFrontMostWindowTitleUtf8()` đọc tiêu đề cửa sổ UTF-8 qua `GetWindowTextW`.
  3. `SystemTrayHelper.h` & `SystemTrayHelper.cpp`:
     - Bổ sung mục menu `POPUP_AUTO_SWITCH_CODETABLE` ("Tự động chuyển bảng mã theo ứng dụng") kèm checkmark.
     - Bổ sung mục menu `POPUP_FALLBACK_UNICODE` ("Fallback về Unicode khi rời ứng dụng") kèm checkmark.
     - Hàm `SystemTrayHelper::showNotification` gửi thông báo Balloon qua Windows Shell Notification API.
  4. `AppDelegate.h` & `AppDelegate.cpp`:
     - Bổ sung biến toàn cục `vAutoSwitchCodeTable` và `vFallbackToUnicode` lưu cấu hình vào Registry.
     - Triển khai hàm `onToggleAutoSwitchCodeTable()` và `onToggleFallbackToUnicode()`.
  5. `OpenKey.cpp`:
     - Bắt phím tắt `Ctrl + Shift + F1` (Unicode), `Ctrl + Shift + F2` (TCVN3) và `Ctrl + Shift + F12` (Khóa/Mở khóa tự động).
     - Hook sự kiện đổi cửa sổ (`EVENT_SYSTEM_FOREGROUND`) và đổi tiêu đề (`EVENT_OBJECT_NAMECHANGE`).
     - Triển khai nhánh logic fallback trong `winEventProcCallback`: nếu `ruleCode == -1` và `vFallbackToUnicode == 1` thì chuyển về Unicode; nếu `vFallbackToUnicode == 0` thì giữ nguyên bảng mã hiện tại.
  6. `process_rules.ini`: File cấu hình mẫu với các tham số `enabled`, `fallback_to_unicode`, và các quy tắc mẫu.
  7. `build.bat`: Kịch bản biên dịch tự động chuẩn UTF-8 (codepage 65001).

### Agent 3: Chuyên gia Phản biện, Kiểm thử & Lập tài liệu (Reviewer & QA)
- **Nhiệm vụ**: Đánh giá kiến trúc, kiểm tra các ca biên (edge-cases), xác minh tính tương thích và lập tài liệu kỹ thuật hoàn chỉnh.

---

## 3. Chi tiết triển khai mã nguồn (Implementation Details)

### 3.1. Menu Khay hệ thống: Tùy chọn Fallback về Unicode
Vị trí: `SystemTrayHelper.cpp`

```cpp
#define POPUP_AUTO_SWITCH_CODETABLE 904
#define POPUP_FALLBACK_UNICODE 905

map<UINT, LPCTSTR> menuData = {
    ...
    {POPUP_AUTO_SWITCH_CODETABLE, _T("Tự động chuyển bảng mã theo ứng dụng")},
    {POPUP_FALLBACK_UNICODE, _T("Fallback về Unicode khi rời ứng dụng")},
    ...
};

// Trong WndProc:
case POPUP_FALLBACK_UNICODE:
    AppDelegate::getInstance()->onToggleFallbackToUnicode();
    break;

// Trong createPopupMenu:
AppendMenu(popupMenu, MF_CHECKED, POPUP_AUTO_SWITCH_CODETABLE, menuData[POPUP_AUTO_SWITCH_CODETABLE]);
AppendMenu(popupMenu, MF_UNCHECKED, POPUP_FALLBACK_UNICODE, menuData[POPUP_FALLBACK_UNICODE]);

// Trong updateData:
MODIFY_MENU(popupMenu, POPUP_AUTO_SWITCH_CODETABLE, vAutoSwitchCodeTable);
MODIFY_MENU(popupMenu, POPUP_FALLBACK_UNICODE, vFallbackToUnicode);
```

---

### 3.2. Logic Nhận diện & Fallback trong Callback sự kiện
Vị trí: `winEventProcCallback` trong [`OpenKey.cpp`](Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp)

```cpp
// 1. Kiểm tra quy tắc nhận diện ứng dụng & tiêu đề cửa sổ (chỉ chạy khi không bị khóa)
if (vAutoSwitchCodeTable) {
    string title = OpenKeyHelper::getFrontMostWindowTitleUtf8();
    int ruleCode = ProcessRuleHelper::getCodeTableForProcessAndTitle(exe, title);
    if (ruleCode != -1) {
        if (vCodeTable != ruleCode) {
            AppDelegate::getInstance()->onTableCode(ruleCode);
            SystemTrayHelper::updateData();
        }
    } else if (vFallbackToUnicode) {
        // Nếu không khớp rule và BẬT Fallback -> tự động chuyển về Unicode!
        if (vCodeTable != 0) {
            AppDelegate::getInstance()->onTableCode(0);
            SystemTrayHelper::updateData();
        }
    }
    // Nếu ruleCode == -1 và TẮT Fallback -> GIỮ NGUYÊN bảng mã hiện tại!
}
```

---

### 3.3. File cấu hình `process_rules.ini`

```ini
# ============================================================
# OpenKey - Cau hinh tu dong nhan dien tien trinh & bang ma
#
# Cai dat khoa / bat tinh nang:
#   enabled = 1               (1: Bat tu dong chuyen, 0: Khoa / Tat)
#   fallback_to_unicode = 0   (1: Bat fallback ve Unicode khi roi app, 0: Giu nguyen bang ma)
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
# ============================================================

enabled = 1
fallback_to_unicode = 0

s.exe = TCVN3
excel.exe[Tong hop ban hang - Date] = TCVN3
chrome.exe = UNICODE
zalo.exe = UNICODE
```

---

## 4. Hướng dẫn sử dụng & Kiểm tra
1. **Bật/Tắt tính năng Fallback về Unicode tại Tray**:
   - Nhấp chuột phải vào biểu tượng OpenKey dưới khay hệ thống.
   - Nhấp vào mục **"Fallback về Unicode khi rời ứng dụng"**:
     - Khi có dấu tích $\checkmark$: Khi chuyển sang bất kỳ ứng dụng nào không có trong quy tắc, bảng mã sẽ tự động quay về **Unicode**.
     - Khi không có dấu tích: Khi chuyển sang bất kỳ ứng dụng nào không có trong quy tắc, OpenKey sẽ **giữ nguyên 100% bảng mã bạn đang dùng** (ví dụ vừa dùng TCVN3 thì vẫn là TCVN3).
2. **Khóa / Mở khóa toàn bộ tính năng tự động chuyển bảng mã**:
   - Nhấp vào dòng **"Tự động chuyển bảng mã theo ứng dụng"** ở khay hệ thống, hoặc bấm phím tắt **`Ctrl + Shift + F12`**.
3. **Phím tắt đổi bảng mã nhanh**:
   - `Ctrl + Shift + F1`: Chuyển sang **Unicode**.
   - `Ctrl + Shift + F2`: Chuyển sang **TCVN3 (ABC)**.

---

## 5. Đồng bộ Bảng mã cho Tính năng Gõ tắt (Macro Synchronization)
- **Tài liệu kỹ thuật chi tiết**: Xem tại [`DOCS_MACRO_TABLECODE_SYNC.md`](DOCS_MACRO_TABLECODE_SYNC.md).
- **Tổng kết**: Khi người dùng chuyển đổi bảng mã qua phím tắt (`Ctrl + Shift + F1/F2`), Menu khay hệ thống, Combobox Bảng điều khiển, hoặc cơ chế nhận diện tự động (`ProcessRuleHelper` & Fallback), toàn bộ từ viết tắt trong bộ nhớ RAM được tự động cập nhật ngay lập tức theo bảng mã hiện hành thông qua hàm engine chuẩn `onTableCodeChange()`.
- **Kiến trúc Single Source of Truth**: Tất cả các kênh đổi bảng mã đều quy tụ về `AppDelegate::onTableCode(code)` và đồng bộ toàn diện trạng thái giao diện lẫn dữ liệu engine.

