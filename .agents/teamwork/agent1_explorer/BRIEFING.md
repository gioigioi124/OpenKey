# BRIEFING — 2026-10-09T02:35:10Z

## Mission
Investigate OpenKey Win32 codebase for Macro table code synchronization issue and produce detailed remediation plan.

## 🔒 My Identity
- Archetype: explorer
- Roles: Explorer, Clarifier, Remediation Planner
- Working directory: C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent1_explorer
- Original parent: 658ac2ea-0e4c-4f15-acdd-165a0246e2d6
- Milestone: Investigation & Planning

## 🔒 Key Constraints
- Read-only investigation — do NOT implement
- Analyze problems, synthesize findings, produce structured reports
- Write only to C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent1_explorer\

## Current Parent
- Conversation ID: 658ac2ea-0e4c-4f15-acdd-165a0246e2d6
- Updated: not yet

## Investigation State
- **Explored paths**:
  - `Sources/OpenKey/engine/Macro.h`, `Macro.cpp`
  - `Sources/OpenKey/engine/Engine.h`, `Engine.cpp`
  - `Sources/OpenKey/engine/Vietnamese.h`, `Vietnamese.cpp`
  - `Sources/OpenKey/engine/SmartSwitchKey.h`, `SmartSwitchKey.cpp`
  - `Sources/OpenKey/macOS/ModernKey/AppDelegate.m`, `OpenKey.mm`
  - `Sources/OpenKey/win32/OpenKey/OpenKey/AppDelegate.h`, `AppDelegate.cpp`
  - `Sources/OpenKey/win32/OpenKey/OpenKey/MainControlDialog.h`, `MainControlDialog.cpp`
  - `Sources/OpenKey/win32/OpenKey/OpenKey/OpenKey.cpp`
  - `Sources/OpenKey/win32/OpenKey/OpenKey/SystemTrayHelper.cpp`
  - `Sources/OpenKey/win32/OpenKey/OpenKey/ProcessRuleHelper.cpp`
  - `build.bat`
- **Key findings**:
  1. Engine has `onTableCodeChange()` in `Macro.cpp` which regenerates `macroContentCode` from pristine UTF-8 `macroContent` for `_codeTable[vCodeTable]`.
  2. macOS implementation explicitly invokes `onTableCodeChange()` inside `onCodeTableChanged` / `OnTableCodeChange()`.
  3. Win32 `AppDelegate::onTableCode(const int& code)` never invoked `onTableCodeChange()`, leaving `macroContentCode` fixed in startup encoding.
  4. `MainControlDialog::onComboBoxSelected` bypassed `AppDelegate::onTableCode()`, directly writing to `vCodeTable`.
  5. `AppDelegate::onDefaultConfig()` reset `vCodeTable` to 0 without synchronizing macros.
  6. Tested baseline build `build.bat` passes with exit code 0.
- **Unexplored areas**: None. Call chains and impact points across Win32 and Engine are completely mapped.

## Key Decisions Made
- Standardize all table code transitions through `AppDelegate::onTableCode(const int& code)`.
- Trigger `onTableCodeChange()` inside `AppDelegate::onTableCode` and `AppDelegate::onDefaultConfig()`.
- Standardize `MainControlDialog::onComboBoxSelected` to delegate to `AppDelegate::onTableCode(code)`.
- Ensure `SystemTrayHelper::updateData()` is called in `AppDelegate::onTableCode` to keep tray menu synchronized.

## Artifact Index
- `C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent1_explorer\handoff.md` — Complete 5-component handoff report.
- `C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent1_explorer\progress.md` — Liveness & status tracking.
- `C:\Users\Administrator\Desktop\OpenKey\.agents\teamwork\agent1_explorer\DISPATCH.md` — Task history and instructions.
