## 2026-10-09T03:52:30Z
You are Agent 2 (Developer / Worker) for the OpenKey Win32 On-Demand / Lazy Conversion Macro project.

Working Directory: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent2_worker_2
Project Root: C:\Users\Administrator\Desktop\OpenKey
Original Request: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\ORIGINAL_REQUEST.md (under section `## 2026-10-09T03:41:32Z`)
Project Specification: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\orchestrator_2\PROJECT.md
Agent 1 Blueprint / Handoff: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent1_explorer_2\handoff.md

MANDATORY INTEGRITY WARNING:
DO NOT CHEAT. All implementations must be genuine. DO NOT hardcode test results, create dummy/facade implementations, or circumvent the intended task. A teamwork_preview_auditor will independently verify your work. Integrity violations WILL be detected and your work WILL be rejected.

### Objective:
Implement On-Demand / Lazy JIT Conversion for Macro engine in C++ according to the blueprint designed by Agent 1, removing all pre-translation into RAM, and verify compilation via `build.bat`.

### Write Ownership:
You own edits to:
- `Sources/OpenKey/engine/Macro.h`
- `Sources/OpenKey/engine/Macro.cpp`

### Detailed Instructions:
1. Read `C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent1_explorer_2\handoff.md` carefully, especially Section 4.1.
2. Edit `Sources/OpenKey/engine/Macro.h`:
   - Simplify `MacroData` by removing `vector<Uint32> macroContentCode;`.
   - Update `onTableCodeChange()` documentation comment.
3. Edit `Sources/OpenKey/engine/Macro.cpp`:
   - In `initMacroMap()`: remove `convert(macroContent, data.macroContentCode);`.
   - In `findMacro()`:
     - On direct match: invoke `convert(it->second.macroContent, macroContentCode);` and return `true`.
     - In `vAutoCapsMacro` branch: normalize key, find in `macroMap`, invoke `convert(itCaps->second.macroContent, macroContentCode);`, apply uppercase loop (`toupper()` for ASCII characters, `modifyCaseUnicode()` for accented codes), and return `true`.
   - In `addMacro()`: remove `convert(macroContent, ...);` calls so insertion does not pre-translate.
   - In `onTableCodeChange()`: clear the loop and make it an $O(1)$ zero-CPU operation (no-op).
4. Run `build.bat` using `cmd.exe /c build.bat` in `C:\Users\Administrator\Desktop\OpenKey`. Verify compilation and linking succeed with exit code 0, producing `OpenKey.exe`.
5. Write your complete handoff report to `C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent2_worker_2\handoff.md` including exact diffs, build output, and send a message back to parent orchestrator.
