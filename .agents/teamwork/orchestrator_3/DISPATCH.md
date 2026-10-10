# Dispatch Orders

## 2026-10-10T02:06:27Z

You are the Project Orchestrator for the OpenKey Win32 project.

Working directory: c:\Users\03102025\Desktop\OpenKey\.agents\teamwork\orchestrator_3
Original request file: c:\Users\03102025\Desktop\OpenKey\.agents\teamwork\ORIGINAL_REQUEST.md
Project root: c:\Users\03102025\Desktop\OpenKey
Integrity mode: development

Task:
Please implement the latest user request in ORIGINAL_REQUEST.md (under ## 2026-10-10T02:05:02Z):
"Phân rã nhiệm vụ cho 3 Agents: Agent 1 hỏi lại tôi để làm rõ vấn đề rồi lên kế hoạch, Agent 2 viết code, Agent 3 kiểm tra, phản biện và ghi vào file md đã có.

Tự động kiểm tra tiêu đề/tiến trình của cửa sổ cha (Parent / Owner Window) trước khi chuyển đổi bảng mã hoặc fallback về Unicode khi mở UserForm hoặc hộp thoại con (áp dụng cho Excel và tổng quát các ứng dụng) trong OpenKey Win32."

Follow the instructions in ORIGINAL_REQUEST.md strictly:
- R1: Trace parent/root owner window (GA_ROOTOWNER / GW_OWNER) within the same process when opening UserForm / child dialog before table code switching or fallback to Unicode.
- R2: Ensure safety, minimal latency, single source of truth via AppDelegate::getInstance()->onTableCode(ruleCode) and SystemTrayHelper::updateData().
- R3: Decompose into 3 Agents: Agent 1 (Clarify & Plan), Agent 2 (Dev / Implementer), Agent 3 (Review, Test, Critique, update DOCS_AUTO_ENCODING_AND_HOTKEY.md and CHANGELOG.md).
- Acceptance criteria must all be satisfied.

Keep your progress.md and BRIEFING.md updated in your working directory.
When all tasks are complete, deliver your handoff.md and report completion to Sentinel so the independent Victory Auditor can be dispatched.
