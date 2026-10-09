# Dispatch: Agent 3 - Reviewer, Critic, Challenger & Synthesizer

## Identity
- Role: Reviewer, Critic, Challenger and Technical Documentation Synthesizer (Agent 3 in the 3-Agent team)
- Working Directory: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent3_reviewer\
- Project Root: C:\Users\Administrator\Desktop\OpenKey

## Objective
Thực hiện vai trò **Agent 3 (Reviewer / Critic / Challenger / Synthesizer)**:
1. Đọc kỹ file yêu cầu gốc: `C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\ORIGINAL_REQUEST.md`
2. Đọc báo cáo của Agent 1 (`agent1_explorer/handoff.md`) và Agent 2 (`agent2_worker/handoff.md`).
3. Thực hiện phản biện và rà soát mã nguồn (Code Review & Static Analysis):
   - Kiểm tra `git diff` các tệp đã sửa (`AppDelegate.cpp`, `MainControlDialog.cpp`).
   - Đánh giá tính toàn vẹn (Integrity check): Có gian lận (hardcode/facade) không?
   - Đánh giá kiến trúc: Có thỏa mãn Single Source of Truth không?
   - Đánh giá độ trễ/hiệu năng: `onTableCodeChange()` chạy tốn bao nhiêu thời gian? Có ảnh hưởng hook bàn phím gõ thường không?
   - Đánh giá các luồng kích hoạt bảng mã: Hotkey (F1/F2...), Tray menu (5 bảng mã), Dialog combobox, ProcessRuleHelper tự động chuyển & Fallback Unicode.
4. Kiểm tra biên dịch & kiểm thử độc lập:
   - Chạy lệnh `cmd /c build.bat` để xác minh độc lập khả năng biên dịch sạch sẽ của `OpenKey.exe`.
   - Kiểm thử/đánh giá các kịch bản nghiệm thu (Acceptance Criteria):
     - Unicode hotkey (`Ctrl+Shift+F1`)
     - TCVN3 hotkey (`Ctrl+Shift+F2`)
     - VNI khay hệ thống (System Tray)
     - Combobox trong MainControlDialog
     - Tự động chuyển bảng mã & fallback trong ProcessRuleHelper
     - Thêm/sửa macro trong MacroDialog
     - Chuyển đổi qua lại liên tục nhiều lần (Stress / Rapid switching test)
5. Cập nhật / Biên soạn tài liệu kỹ thuật hoàn chỉnh:
   - Theo yêu cầu R3: "Cập nhật tài liệu kỹ thuật ghi rõ phân công 3 Agents, phân tích nguyên nhân lỗi, giải pháp và kết quả kiểm thử".
   - Cập nhật tài liệu kỹ thuật tại `DOCS_AUTO_ENCODING_AND_HOTKEY.md` (hoặc tạo tài liệu kỹ thuật chi tiết `DOCS_MACRO_TABLECODE_SYNC.md` và dẫn chiếu trong `DOCS_AUTO_ENCODING_AND_HOTKEY.md` / `README.md`).
6. Xuất báo cáo phản biện và tổng hợp bàn giao ra: `C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent3_reviewer\handoff.md`.
7. Gửi thông báo kết quả cho Project Orchestrator (Caller ID).

## 2026-10-09T02:41:37Z
Bạn là Agent 3 (Reviewer / Critic / Challenger / Documentation Synthesizer) trong mô hình 3 Agents theo yêu cầu của người dùng.

Nhiệm vụ của bạn:
1. Đọc kỹ file yêu cầu gốc: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\ORIGINAL_REQUEST.md
2. Đọc file phân công nhiệm vụ: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent3_reviewer\DISPATCH.md
3. Đọc báo cáo của Agent 1 (C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent1_explorer\handoff.md) và Agent 2 (C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent2_worker\handoff.md).
4. Thực hiện phản biện và rà soát mã nguồn (Code Review):
   - Kiểm tra git diff trên 2 tệp AppDelegate.cpp và MainControlDialog.cpp.
   - Kiểm tra tính toàn vẹn (Integrity Forensics): Xác nhận không có gian lận, không hardcode kết quả test, không dummy/facade.
   - Kiểm tra kiến trúc: Single Source of Truth thông qua AppDelegate::onTableCode(code).
   - Kiểm tra hiệu năng: onTableCodeChange() chạy micro-giây trong RAM, hoàn toàn không ảnh hưởng tới keyboardHookProcess.
   - Kiểm tra đầy đủ các luồng: Hotkeys Ctrl+Shift+F1..F12, Tray menu 5 bảng mã, Dialog combobox, ProcessRuleHelper tự động chuyển & Fallback.
5. Kiểm tra biên dịch & kiểm thử độc lập:
   - Chạy lệnh: cmd /c build.bat tại C:\Users\Administrator\Desktop\OpenKey để xác minh độc lập việc biên dịch ra OpenKey.exe với exit code 0.
   - Kiểm tra/đánh giá đầy đủ các kịch bản nghiệm thu (Acceptance Criteria) từ TC-01 đến TC-09.
6. Cập nhật và hoàn thiện tài liệu kỹ thuật theo yêu cầu R3:
   - Soạn thảo/cập nhật tài liệu kỹ thuật hoàn chỉnh tại C:\Users\Administrator\Desktop\OpenKey\DOCS_MACRO_TABLECODE_SYNC.md (và/hoặc bổ sung phần tổng kết vào DOCS_AUTO_ENCODING_AND_HOTKEY.md).
   - Tài liệu kỹ thuật phải ghi rõ:
     + Phân công và vai trò của 3 Agents (Agent 1: Khảo sát & Kế hoạch, Agent 2: Lập trình & Biên dịch, Agent 3: Phản biện, Kiểm thử & Tổng hợp).
     + Phân tích nguyên nhân gốc rễ (Root Cause Analysis).
     + Giải pháp kiến trúc và chi tiết mã nguồn đã thay đổi.
     + Kết quả biên dịch và kết quả kiểm thử các kịch bản nghiệm thu.
7. Xuất báo cáo phản biện và tổng hợp bàn giao ra: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent3_reviewer\handoff.md
8. Gửi tin nhắn thông báo hoàn thành qua send_message cho Project Orchestrator (Caller ID).

