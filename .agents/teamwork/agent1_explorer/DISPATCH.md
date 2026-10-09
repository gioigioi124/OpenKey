# Dispatch: Agent 1 - Explorer & Planner

## Identity
- Role: Explorer, Clarifier, and Remediation Planner (Agent 1 in the 3-Agent team)
- Working Directory: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent1_explorer\
- Project Root: C:\Users\Administrator\Desktop\OpenKey

## Objective
Thực hiện vai trò **Agent 1**:
1. Đọc và phân tích kỹ yêu cầu trong `C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\ORIGINAL_REQUEST.md`.
2. Khảo sát chi tiết mã nguồn OpenKey (đặc biệt là bản Win32 và đối chiếu macOS nếu có):
   - `Macro.h`, `Macro.cpp`: Tìm hiểu cấu trúc dữ liệu macro, hàm `onTableCodeChange()`, biến `macroContentCode`, các bảng mã được hỗ trợ (Unicode, TCVN3, VNI, Tổ hợp, CP 1258...), cơ chế chuyển đổi dữ liệu phím tắt.
   - `AppDelegate.h`, `AppDelegate.cpp`: Khảo sát hàm `onTableCode(const int & code)`, luồng xử lý phím tắt nhanh (`Ctrl+Shift+F1/F2...`), menu chuột phải khay hệ thống (Tray menu).
   - `MainControlDialog.h`, `MainControlDialog.cpp`: Khảo sát sự kiện khi người dùng chọn bảng mã trong Combobox. Có đi qua `AppDelegate::onTableCode()` không?
   - `ProcessRuleHelper.h`, `ProcessRuleHelper.cpp`: Khảo sát luồng tự động chuyển đổi bảng mã theo ứng dụng/Excel và fallback về Unicode.
   - Đối chiếu với macOS (nếu có trong repo, e.g. `Sources/macOS` hoặc hàm `OnTableCodeChange()`) để nắm chuẩn thiết kế ban đầu của tác giả.
3. Làm rõ các vấn đề (Clarifications):
   - Đưa ra danh sách các phát hiện thực tế trong codebase.
   - Làm rõ nguyên nhân gốc rễ (Root Cause) vì sao hiện tại macro không được cập nhật bảng mã khi chuyển đổi trên Win32.
   - Ghi nhận các câu hỏi/điểm cần lưu ý (clarifications/questions).
4. Lên kế hoạch sửa chữa chi tiết (Detailed Remediation Plan) cho Agent 2 (Dev) thực hiện:
   - Các file cần chỉnh sửa, vị trí sửa, đoạn code mẫu dự kiến.
   - Đảm bảo độ mượt mà, hiệu năng (zero lag).
   - Kế hoạch biên dịch (`build.bat`) và kịch bản kiểm thử (cho Agent 3 phản biện).
5. Ghi báo cáo đầy đủ vào: `C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent1_explorer\handoff.md`.
6. Gửi tin nhắn hoàn thành về cho Project Orchestrator (Caller ID).


## 2026-10-09T02:27:57Z
Bạn là Agent 1 (Explorer & Planner) trong mô hình 3 Agents theo yêu cầu của người dùng.

Nhiệm vụ của bạn:
1. Đọc kỹ file yêu cầu gốc: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\ORIGINAL_REQUEST.md
2. Đọc file phân công nhiệm vụ: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent1_explorer\DISPATCH.md
3. Khảo sát toàn diện mã nguồn liên quan đến Macro và TableCode:
   - Các file Macro: Macro.h, Macro.cpp (hàm onTableCodeChange, macroContentCode, danh sách macro trong bộ nhớ, cách chuyển đổi bảng mã, etc.)
   - Các file AppDelegate: AppDelegate.h, AppDelegate.cpp (onTableCode, các hotkey Ctrl+Shift+F1..F12, Tray menu, etc.)
   - MainControlDialog.h, MainControlDialog.cpp (sự kiện combobox bảng mã)
   - ProcessRuleHelper.h, ProcessRuleHelper.cpp (tự động đổi bảng mã và fallback về Unicode)
   - Mã nguồn macOS (nếu có trong repo) để xem đối chiếu cách gọi OnTableCodeChange().
4. Làm rõ vấn đề & phân tích nguyên nhân gốc rễ (Root Cause Analysis):
   - Tại sao phím tắt/macro trên Win32 hiện tại không đồng bộ bảng mã khi đổi bảng mã qua Tray, Hotkey, Dialog, ProcessRuleHelper?
   - Cần kích hoạt onTableCodeChange() ở những điểm nào?
   - Cần đảm bảo hiệu năng, tránh trùng lặp hay xung đột như thế nào?
5. Thiết lập kế hoạch sửa chữa chi tiết (Detailed Remediation Plan) để Agent 2 (Dev) có thể lập trình chính xác:
   - Danh sách file cần sửa, hàm cần sửa, logic cần thêm.
   - Chiến lược biên dịch build.bat và kịch bản test để bàn giao cho Agent 2 và Agent 3.
6. Xuất báo cáo hoàn chỉnh ra file: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent1_explorer\handoff.md
7. Gửi thông báo kết quả qua send_message cho Orchestrator (Caller) khi hoàn thành.
