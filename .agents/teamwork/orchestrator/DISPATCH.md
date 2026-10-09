## 2026-10-09T02:26:29Z

You are the Project Orchestrator for the OpenKey Win32 Macro table code synchronization fix.

Your working directory is: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\orchestrator\
The project root directory is: C:\Users\Administrator\Desktop\OpenKey
The original user request is recorded in: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\ORIGINAL_REQUEST.md

Please review the user request carefully:
"Sửa lỗi phím tắt/gõ tắt (Macro) trong OpenKey Win32: khi chuyển đổi bảng mã (qua Tray menu, phím tắt nhanh Ctrl+Shift+F1/F2, hộp thoại cài đặt hoặc tự động nhận diện), nội dung phím tắt phải được tự động chuyển đổi theo bảng mã hiện hành theo đúng cơ chế gốc của phần mềm (`onTableCodeChange()`), và tự động biên dịch hoàn chỉnh `OpenKey.exe`.

Working directory: C:\Users\Administrator\Desktop\OpenKey
Integrity mode: development

Phân rã nhiệm vụ cho 3 Agents: Agent 1 hỏi lại tôi để làm rõ vấn đề và lên kế hoạch sửa chữa, Agent 2 đóng vai trò dev viết code, Agent 3 phản biện, kiểm tra và tổng hợp lại toàn bộ.

## Requirements
### R1. Đồng bộ bảng mã cho tính năng gõ tắt (Macro) khi chuyển bảng mã
Khi người dùng chuyển đổi bảng mã bằng bất kỳ hình thức nào:
1. Phím tắt nhanh: `Ctrl + Shift + F1` (Unicode), `Ctrl + Shift + F2` (TCVN3)...
2. Menu chuột phải ở khay hệ thống (System Tray) cho tất cả các bảng mã (Unicode, TCVN3, VNI, Unicode Tổ hợp, CP 1258).
3. Hộp thoại Bảng điều khiển chính (combobox Bảng mã trong `MainControlDialog`).
4. Tự động chuyển bảng mã theo tiến trình / file Excel (`ProcessRuleHelper`) và fallback về Unicode.
Toàn bộ dữ liệu mã phím tắt (`macroContentCode`) trong bộ nhớ phải được chuyển đổi và cập nhật lại tương ứng với bảng mã vừa chọn.

### R2. Áp dụng chuẩn cơ chế xử lý gốc của OpenKey Engine
- OpenKey engine đã có sẵn hàm `onTableCodeChange()` trong `Macro.h` và `Macro.cpp` (bản macOS đã gọi hàm này trong `OnTableCodeChange()`).
- Tích hợp chuẩn lệnh gọi `onTableCodeChange()` vào `AppDelegate::onTableCode(const int & code)` trong Win32.
- Chuẩn hóa luồng chọn bảng mã trong `MainControlDialog.cpp` (khi chọn combobox bảng mã) để đi qua `AppDelegate::onTableCode()` hoặc kích hoạt `onTableCodeChange()`.
- Đảm bảo hiệu năng nhanh chóng, không gây giật lag khi gõ phím hay khi chuyển đổi cửa sổ.

### R3. Biên dịch và Cập nhật Tài liệu
- Thực thi kịch bản `build.bat` biên dịch thành công file thực thi `OpenKey.exe` không có lỗi.
- Cập nhật tài liệu kỹ thuật ghi rõ phân công 3 Agents, phân tích nguyên nhân lỗi, giải pháp và kết quả kiểm thử.
