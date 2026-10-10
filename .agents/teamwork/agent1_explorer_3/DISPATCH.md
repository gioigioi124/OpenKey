## 2026-10-10T02:07:26Z
You are Agent 1 (Clarify & Plan / Explorer) in the 3-Agent workflow for OpenKey Win32.
Working directory: c:\Users\03102025\Desktop\OpenKey\.agents\teamwork\agent1_explorer_3
Original request file: c:\Users\03102025\Desktop\OpenKey\.agents\teamwork\ORIGINAL_REQUEST.md
Scope document: c:\Users\03102025\Desktop\OpenKey\.agents\teamwork\orchestrator_3\SCOPE.md
Project root: c:\Users\03102025\Desktop\OpenKey

Your mission:
Read c:\Users\03102025\Desktop\OpenKey\.agents\teamwork\ORIGINAL_REQUEST.md (under section ## 2026-10-10T02:05:02Z).
Investigate the codebase in c:\Users\03102025\Desktop\OpenKey:
1. Examine OpenKeyHelper.cpp, OpenKeyHelper.h, OpenKey.cpp, ProcessRuleHelper.cpp, ProcessRuleHelper.h.
2. Analyze how window events (EVENT_SYSTEM_FOREGROUND, EVENT_OBJECT_NAMECHANGE) are hooked and handled in winEventProcCallback.
3. Analyze how active window title and process name are currently obtained and matched against process_rules.ini.
4. Determine how to safely trace the parent / owner window (GA_ROOTOWNER via GetAncestor or GW_OWNER via GetWindow) belonging to the same process (verifying PID with GetWindowThreadProcessId).
5. Address all edge cases:
   - What if foreground window is already the main/root window?
   - What if child window is a modal dialog or modeless UserForm (e.g. in Excel with title "UserForm1" or a dialog with empty title)?
   - What if child window has its own specific rule in process_rules.ini vs no rule?
   - What if parent window has rule vs no rule?
   - When should fallback to Unicode occur? (Only if neither child nor parent matches any rule and fallback_to_unicode == 1).
   - How to prevent infinite loops, handle leaks, invalid HWND, cross-process owner windows?
   - How to ensure minimal latency and single source of truth via AppDelegate::getInstance()->onTableCode(ruleCode) and SystemTrayHelper::updateData().
6. Create a concrete, detailed implementation plan for Agent 2 (Dev / Worker), specifying exact function signatures, logic flow, and file locations.
7. Build a comprehensive Test Matrix covering all scenarios and edge cases.
8. Write your full analysis, architecture plan, and test matrix to `c:\Users\03102025\Desktop\OpenKey\.agents\teamwork\agent1_explorer_3\analysis.md` and your handoff to `c:\Users\03102025\Desktop\OpenKey\.agents\teamwork\agent1_explorer_3\handoff.md`.
9. Send a message back to me (the orchestrator) with a summary and the path to handoff.md when done.
