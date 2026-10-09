## 2026-10-09T03:44:02Z
From: c23e947b-4b79-4b4e-8a3f-89e48acc0ccb (orchestrator_2)
Content:
You are Agent 1 (Planner & Architectural Explorer) for the OpenKey Win32 On-Demand / Lazy Conversion Macro project.

Working Directory: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent1_explorer_2
Project Root: C:\Users\Administrator\Desktop\OpenKey
Original Request: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\ORIGINAL_REQUEST.md (Specifically the new request under `## 2026-10-09T03:41:32Z`)
Project Specification: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\orchestrator_2\PROJECT.md

### Objective:
Investigate and design the architectural transformation of OpenKey Win32 Macro engine to On-demand / Lazy JIT Conversion.
Remove pre-translation and RAM storage of `macroContentCode` for all macros. Replace with JIT conversion of only the triggered macro according to active `vCodeTable` during `findMacro()`.

### Detailed Tasks:
1. Examine `Macro.h`, `Macro.cpp`, `OpenKey.h`, `OpenKey.cpp`, `AppDelegate.cpp`, `MacroDialog.cpp`, and any other files referencing `MacroData`, `macroContentCode`, `macroMap`, `findMacro`, `onTableCodeChange`.
2. Trace how strings are converted to `KeyEvent`s in OpenKey (e.g., `stringToKeyEvents`, table code lookups, character encodings for Unicode, TCVN3, VNI, etc.).
3. Analyze `vAutoCapsMacro`: how does capitalization work when typing a macro keyword with an uppercase first letter? How should JIT conversion preserve this behavior?
4. Analyze `MacroData` data structure: can `macroContentCode` be completely removed or converted to an optional/cached field? How should `initMacroMap()` and `addMacro()` be simplified so no pre-translation occurs?
5. Analyze `onTableCodeChange()`: how to make it $O(1)$ 0% CPU overhead while keeping consistency.
6. Verify all call sites of `MacroData` across the entire solution so no compiler errors occur.
7. Formulate a concrete, step-by-step implementation specification for Agent 2 (Developer), with exact code diff previews / pseudo-code / helper functions needed.
8. Formulate a comprehensive test matrix (TC-01 .. TC-XX) for Agent 3 (Tester) covering Unicode, TCVN3, VNI, AutoCaps, continuous rapid table code switching, and edge cases.
9. Write your detailed handoff report to `C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent1_explorer_2\handoff.md` and communicate completion via `send_message` to your parent orchestrator.
