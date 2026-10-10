## 2026-10-10T02:17:24Z
DO NOT CHEAT. All implementations must be genuine. DO NOT hardcode test results, create dummy/facade implementations, or circumvent the intended task. A teamwork_preview_auditor will independently verify your work. Integrity violations WILL be detected and your work WILL be rejected.

You are Agent 2 (Developer / Worker) in the 3-Agent workflow for OpenKey Win32.
Working directory: c:\Users\03102025\Desktop\OpenKey\.agents\teamwork\agent2_worker_3
Original request file: c:\Users\03102025\Desktop\OpenKey\.agents\teamwork\ORIGINAL_REQUEST.md
Project root: c:\Users\03102025\Desktop\OpenKey

Inputs:
- Read c:\Users\03102025\Desktop\OpenKey\.agents\teamwork\ORIGINAL_REQUEST.md (under section ## 2026-10-10T02:05:02Z).
- Read Agent 1's analysis report at c:\Users\03102025\Desktop\OpenKey\.agents\teamwork\agent1_explorer_3\analysis.md and handoff report at c:\Users\03102025\Desktop\OpenKey\.agents\teamwork\agent1_explorer_3\handoff.md.

Exclusive Write Ownership:
- Sources/OpenKey/win32/OpenKey/OpenKey/OpenKeyHelper.h
- Sources/OpenKey/win32/OpenKey/OpenKey/OpenKeyHelper.cpp
- Sources/OpenKey/win32/OpenKey/OpenKey/ProcessRuleHelper.h
- Sources/OpenKey/win32/OpenKey/OpenKey/ProcessRuleHelper.cpp
- Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp

Your task:
1. Implement the solution designed by Agent 1:
   - In OpenKeyHelper.h and OpenKeyHelper.cpp:
     - Implement `string getWindowTitleUtf8(HWND hwnd)` to get UTF-8 title of any specified HWND safely.
     - Implement `HWND getProcessRootOwner(HWND hwnd)` using GW_OWNER and GA_ROOTOWNER, verifying PID via GetWindowThreadProcessId, avoiding cross-process boundaries, desktop windows, self-references, and looping at most 10 steps.
     - Refactor `getFrontMostWindowTitleUtf8()` to delegate to `getWindowTitleUtf8(GetForegroundWindow())`.
   - In ProcessRuleHelper.h and ProcessRuleHelper.cpp:
     - Implement `int getCodeTableForTitleOnly(const std::string& processName, const std::string& windowTitle)` (matches title rules Pass 1 and Pass 2, without Pass 3 process fallback).
     - Implement `int getCodeTableForWindow(HWND hwnd, const std::string& processName)` implementing the full priority hierarchy:
       * Priority 1: Check child window title against title rules. If matched, return rule code.
       * Priority 2: If child title has no rule match, find parent/root owner window via `OpenKeyHelper::getProcessRootOwner(hwnd)`. If valid parent found and different from child, get parent's title and check against title rules. If matched, return parent's rule code.
       * Priority 3: If neither child nor parent matches title rules, check process-level rules (Pass 3) using child process name.
       * Priority 4: Return -1 if no rule matches.
   - In OpenKey.cpp:
     - In `winEventProcCallback`, pass the active window handle `hForeground` to `ProcessRuleHelper::getCodeTableForWindow(hForeground, exe)` (or appropriately in `checkActiveWindowRule` / `winEventProcCallback`).
     - Ensure table code switching is executed through `AppDelegate::getInstance()->onTableCode(ruleCode)` and `SystemTrayHelper::updateData()`.
     - Ensure fallback to Unicode (when `vFallbackToUnicode == 1`) occurs ONLY if `ruleCode == -1`.
2. Compile and Verify:
   - Run `build.bat` in project root using run_command.
   - Verify that compilation succeeds and `OpenKey.exe` is generated without errors.
3. Document your changes:
   - Write your progress to `c:\Users\03102025\Desktop\OpenKey\.agents\teamwork\agent2_worker_3\progress.md`.
   - Write your full changes and build results to `c:\Users\03102025\Desktop\OpenKey\.agents\teamwork\agent2_worker_3\handoff.md`.
4. Send a message to orchestrator with your results and path to handoff.md.
