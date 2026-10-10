# Scope: Parent / Owner Window Tracing for Auto Encoding & Fallback in UserForm/Child Dialogs

## Architecture
- OpenKey Win32 event hooks (`EVENT_SYSTEM_FOREGROUND`, `EVENT_OBJECT_NAMECHANGE`) detect window activation and title changes.
- `ProcessRuleHelper` matches process name and window title against rules in `process_rules.ini`.
- When foreground window is a child dialog, popup, or VBA UserForm (e.g. in `excel.exe`, `word.exe`, CAD, etc.):
  1. Check foreground window's own title against rules.
  2. If no rule matches, trace root owner window (`GA_ROOTOWNER` / `GW_OWNER`) belonging to the same process.
  3. If parent/owner window matches a rule, apply parent's table code immediately via `AppDelegate::getInstance()->onTableCode(ruleCode)` and `SystemTrayHelper::updateData()`.
  4. Only fallback to Unicode if neither foreground nor parent window matches any rule (and `fallback_to_unicode == 1`).

## Feature Inventory
| # | Feature | Description | Milestone | Source |
|---|---------|-------------|-----------|--------|
| 1 | Parent/Root Owner Tracing | Trace GA_ROOTOWNER/GW_OWNER within same PID safely | M1, M2 | R1 |
| 2 | Rule Matching Hierarchy | Check child window title first; fallback to parent title if no child rule | M1, M2 | R1 |
| 3 | Single Source of Truth | Switch encoding via AppDelegate::onTableCode & update tray | M2 | R2 |
| 4 | Fallback Isolation | Only fallback to Unicode if both child and parent don't match | M2 | R1 |
| 5 | Edge Cases & Stress Testing | Modal dialog, modeless UserForm, rapid switching, headless window | M3 | R3 |
| 6 | Documentation Update | Update DOCS_AUTO_ENCODING_AND_HOTKEY.md and CHANGELOG.md | M3 | R3 |
| 7 | Forensic Integrity Audit | Independent static/dynamic verification, clean verdict | M4 | Integrity |

## Milestones
| # | Name | Scope | Dependencies | Status |
|---|------|-------|-------------|--------|
| M1 | Agent 1 (Clarify & Plan) | Code analysis, Win32 API trace strategy, Test Matrix | none | DONE |
| M2 | Agent 2 (Dev / Worker) | Implement C++ changes, build via build.bat | M1 | DONE |
| M3 | Agent 3 (Review & Docs) | Review, test cases, critique edge cases, update docs & changelog | M2 | DONE |
| M4 | Forensic Auditor | Full integrity audit | M3 | DONE |

## Interface Contracts
- `ProcessRuleHelper` / `OpenKeyHelper`:
  - Safe root owner retrieval: `HWND getProcessRootOwner(HWND hwnd)`
  - Window title retrieval: verify PID equality (`GetWindowThreadProcessId`)
  - Integration in `winEventProcCallback` / `checkActiveWindowRule`
