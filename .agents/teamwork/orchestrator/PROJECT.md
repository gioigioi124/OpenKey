# Project: OpenKey Win32 Macro Table Code Synchronization

## Architecture
- **Engine Core**: `Macro.h`, `Macro.cpp` handles macro definitions, search, replacement, and table code conversion via `onTableCodeChange()`.
- **Win32 App Coordination**: `AppDelegate.cpp`, `AppDelegate.h` receives table code changes from hotkeys, system tray menu, dialog, and process rules, and dispatches them to engine components via `AppDelegate::onTableCode(code)` (Single Source of Truth).
- **UI Components**:
  - `MainControlDialog.cpp`: UI combobox for table code selection delegates directly to `AppDelegate::onTableCode(code)`.
  - Tray Menu / Hotkeys: System tray items and hotkey events (Ctrl+Shift+F1/F2...) route through `AppDelegate::onTableCode(code)`.
  - `ProcessRuleHelper`: Auto-switch table code by foreground window / Excel title with Unicode fallback routes through `AppDelegate::onTableCode(code)`.

## Feature Inventory
| # | Feature | Description | Milestone | Source |
|---|---------|-------------|-----------|--------|
| 1 | Hotkey macro sync | Macro content synced when switching via Ctrl+Shift+F1/F2... | DONE | R1.1 |
| 2 | System Tray macro sync | Macro content synced when switching via System Tray right-click menu | DONE | R1.2 |
| 3 | MainControlDialog macro sync | Macro content synced when changing combobox in main control dialog | DONE | R1.3 |
| 4 | ProcessRuleHelper macro sync | Macro content synced when auto-switching via process/title rule or fallback | DONE | R1.4 |
| 5 | Engine onTableCodeChange integration | Hook `onTableCodeChange()` cleanly into Win32 `AppDelegate::onTableCode()` | DONE | R2 |
| 6 | MacroDialog compatibility | Ensure adding/editing macros in MacroDialog continues to work properly | DONE | AC |
| 7 | Clean build via build.bat | Compile `OpenKey.exe` successfully without warnings/errors | DONE | R3.1 |
| 8 | Technical documentation | Comprehensive technical report detailing 3-agent roles, root cause, solution, tests | DONE | R3.2 |

## Milestones (3-Agent Pipeline)
| # | Name | Scope | Dependencies | Status |
|---|------|-------|-------------|--------|
| M1 | Agent 1: Investigation & Plan | Code analysis, root cause diagnosis, clarifications & remediation plan | none | DONE |
| M2 | Agent 2: Dev & Implementation | Implement engine hooks & UI sync paths, compile via build.bat | M1 | DONE |
| M3 | Agent 3: Review & Synthesis | Code review, challenge testing, verification & technical documentation | M2 | DONE |

## Interface Contracts
- `AppDelegate::onTableCode(const int & code)`:
  - Calls `APP_SET_DATA(vCodeTable, code)`.
  - Calls `onTableCodeChange()` to reload all `macroContentCode` across the macro table.
  - Updates `mainDialog->fillData()` and `SystemTrayHelper::updateData()`.
  - Updates `setAppInputMethodStatus` if `vRememberCode` is set.
