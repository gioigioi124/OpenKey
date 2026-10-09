# BRIEFING — 2026-10-09T03:52:00Z

## Mission
Architectural investigation and design of On-demand / Lazy JIT Conversion for OpenKey Win32 Macro engine.

## 🔒 My Identity
- Archetype: explorer
- Roles: Planner & Architectural Explorer
- Working directory: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent1_explorer_2
- Original parent: c23e947b-4b79-4b4e-8a3f-89e48acc0ccb
- Milestone: M1 (Agent 1: Planning & Architectural Design)

## 🔒 Key Constraints
- Read-only investigation — do NOT implement source changes
- Do not modify files outside `.agents/teamwork/agent1_explorer_2/`
- Deliver self-contained handoff.md with 5 components
- Provide clear implementation guide for Agent 2 and test matrix for Agent 3

## Current Parent
- Conversation ID: c23e947b-4b79-4b4e-8a3f-89e48acc0ccb
- Updated: 2026-10-09T03:52:00Z

## Investigation State
- **Explored paths**: Macro.h, Macro.cpp, Engine.h, Engine.cpp, Vietnamese.h, Vietnamese.cpp, DataType.h, OpenKey.cpp, AppDelegate.cpp, MacroDialog.cpp, build.bat
- **Key findings**:
  1. `MacroData` is only used inside `Macro.h` and `Macro.cpp`. Removing `vector<Uint32> macroContentCode` is 100% safe across the entire repository.
  2. `findMacro()` in `Macro.cpp` is the sole entry point for retrieving macro key event output. Replacing pre-converted vector copying with JIT `convert(it->second.macroContent, macroContentCode)` completely eliminates RAM pre-conversion.
  3. `vAutoCapsMacro` logic in `findMacro()` operates on `macroContentCode` after conversion, perfectly preserving both Title Case and All Caps behavior across all code tables (`_codeTable[vCodeTable]`).
  4. `initMacroMap()` and `addMacro()` can omit calling `convert()` on `macroContent`, making startup and additions instantaneous.
  5. `onTableCodeChange()` becomes an $O(1)$ no-op, eliminating all loop/CPU overhead during table switching.
  6. Baseline build tested via `build.bat` passes cleanly (exit code 0).
- **Unexplored areas**: None. All architectural and implementation questions resolved.

## Key Decisions Made
- `MacroData` will contain only `macroText` and `macroContent`.
- `findMacro` performs JIT `convert(it->second.macroContent, macroContentCode)`.
- `onTableCodeChange()` kept as $O(1)$ no-op to maintain caller compatibility with `AppDelegate.cpp` and `OpenKey.mm`.
- Comprehensive 10-point test matrix created for Agent 3.

## Artifact Index
- DISPATCH.md — record of orchestrator dispatch
- progress.md — liveness heartbeat
- BRIEFING.md — persistent state index
- handoff.md — detailed 5-component handoff report
