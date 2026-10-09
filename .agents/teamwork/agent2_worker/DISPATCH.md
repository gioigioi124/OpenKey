# Dispatch: Agent 2 - Developer & Implementation Worker

## Identity
- Role: Developer / Implementation Worker (Agent 2 in the 3-Agent team)
- Working Directory: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent2_worker\
- Project Root: C:\Users\Administrator\Desktop\OpenKey

## Mandatory Integrity Warning
DO NOT CHEAT. All implementations must be genuine. DO NOT hardcode test results, create dummy/facade implementations, or circumvent the intended task. A reviewer/auditor will independently verify your work. Integrity violations WILL be detected and your work WILL be rejected.

## Objective
Thực hiện vai trò **Agent 2 (Dev)** theo kế hoạch của Agent 1:
1. Đọc kỹ file yêu cầu gốc: `C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\ORIGINAL_REQUEST.md`
2. Đọc kỹ báo cáo khảo sát và kế hoạch chi tiết của Agent 1 tại: `C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent1_explorer\handoff.md`
3. Tiến hành chỉnh sửa mã nguồn:
   - File 1: `Sources/OpenKey/win32/OpenKey/OpenKey/AppDelegate.cpp`
     - Trong `AppDelegate::onTableCode(const int & code)`:
       Bổ sung lệnh gọi `onTableCodeChange();` và `SystemTrayHelper::updateData();`.
     - Trong `AppDelegate::onDefaultConfig()`:
       Bổ sung lệnh gọi `onTableCodeChange();` sau khi đặt `APP_SET_DATA(vCodeTable, 0);`.
   - File 2: `Sources/OpenKey/win32/OpenKey/OpenKey/MainControlDialog.cpp`
     - Trong `MainControlDialog::onComboBoxSelected(...)`:
       Khi `hCombobox == comboBoxTableCode`: chuẩn hóa bằng cách lấy `code = (int)SendMessage(hCombobox, CB_GETCURSEL, 0, 0);` và gọi `AppDelegate::getInstance()->onTableCode(code);`.
4. Đảm bảo mã nguồn sạch sẽ, không có lỗi cú pháp, tuân thủ phong cách lập trình của dự án.
5. Thực thi lệnh biên dịch `cmd /c build.bat` để biên dịch hoàn chỉnh `OpenKey.exe`.
6. Kiểm tra mã thoát (Exit Code 0), xác nhận file `OpenKey.exe` được cập nhật và không có lỗi biên dịch.
7. Viết báo cáo bàn giao chi tiết (Handoff Report) tại `C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent2_worker\handoff.md` ghi nhận:
   - Các file đã chỉnh sửa, dòng thay đổi cụ thể.
   - Kết quả biên dịch `build.bat` (lệnh, đầu ra, exit code, timestamp/kích thước `OpenKey.exe`).
   - Kết quả tự kiểm tra/xác minh tính đúng đắn.
8. Gửi thông báo kết quả cho Project Orchestrator (Caller) qua `send_message`.


## 2026-10-09T02:36:01Z
Bạn là Agent 2 (Developer / Implementation Worker) trong mô hình 3 Agents theo yêu cầu của người dùng.

MANDATORY INTEGRITY WARNING:
DO NOT CHEAT. All implementations must be genuine. DO NOT hardcode test results, create dummy/facade implementations, or circumvent the intended task. A teamwork_preview_auditor will independently verify your work. Integrity violations WILL be detected and your work WILL be rejected.

Nhiệm vụ của bạn:
1. Đọc file yêu cầu gốc: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\ORIGINAL_REQUEST.md
2. Đọc file phân công nhiệm vụ: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent2_worker\DISPATCH.md
3. Đọc chi tiết phân tích và kế hoạch sửa chữa của Agent 1 tại: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent1_explorer\handoff.md
4. Thực hiện chỉnh sửa mã nguồn (bạn sở hữu quyền ghi trên các file này):
   - File 1: Sources/OpenKey/win32/OpenKey/OpenKey/AppDelegate.cpp
     - Trong AppDelegate::onTableCode(const int & code): Thêm lệnh gọi onTableCodeChange() và SystemTrayHelper::updateData().
     - Trong AppDelegate::onDefaultConfig(): Thêm lệnh gọi onTableCodeChange() sau khi gán APP_SET_DATA(vCodeTable, 0).
   - File 2: Sources/OpenKey/win32/OpenKey/OpenKey/MainControlDialog.cpp
     - Trong MainControlDialog::onComboBoxSelected: Khi hCombobox == comboBoxTableCode, lấy code từ SendMessage(hCombobox, CB_GETCURSEL, 0, 0) và gọi AppDelegate::getInstance()->onTableCode(code).
5. Thực thi lệnh biên dịch để build file OpenKey.exe:
   Chạy lệnh: cmd /c build.bat tại thư mục C:\Users\Administrator\Desktop\OpenKey
   Kiểm tra đầu ra biên dịch, đảm bảo Exit code 0, không có lỗi.
6. Xác minh file OpenKey.exe đã được tạo/cập nhật thành công.
7. Xuất báo cáo bàn giao chi tiết ra: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent2_worker\handoff.md
8. Gửi thông báo kết quả qua send_message cho Project Orchestrator (Caller ID).
