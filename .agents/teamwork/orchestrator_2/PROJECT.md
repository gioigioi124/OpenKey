# Project: OpenKey On-Demand Lazy Conversion for Macro

## Architecture
- **Module**: OpenKey Engine Core (`Macro.h`, `Macro.cpp`, `OpenKey.h`, `OpenKey.cpp`).
- **Old Mechanism**:
  - `MacroData` stores both `macroContent` (UTF-8 string) and `macroContentCode` (`vector<KeyEvent>`).
  - Pre-translates ALL macros into `macroContentCode` at initialization (`initMacroMap`) and when adding (`addMacro`).
  - When switching table code (`vCodeTable`), `onTableCodeChange()` iterates through all macros in RAM to re-translate every entry ($O(N)$ CPU operations and memory reallocations).
- **New Mechanism (On-Demand / Lazy JIT Conversion)**:
  - Eliminate pre-translation of `macroContentCode` in `initMacroMap()` and `addMacro()`.
  - Simplify `MacroData`: eliminate storing pre-translated key event arrays for every macro in RAM.
  - In `findMacro()`: only when a keyword matches and is triggered, perform dynamic Just-In-Time (JIT) conversion of that specific `macroContent` into key events based on current `vCodeTable`.
  - Seamlessly handle `vAutoCapsMacro` (capitalizing initial character dynamically).
  - `onTableCodeChange()` becomes an $O(1)$ 0% CPU operation (no loop, no reallocation, instant table switching).

## Feature Inventory
| # | Feature | Description | Milestone | Source |
|---|---------|-------------|-----------|--------|
| 1 | Planning & JIT Architectural Design | In-depth analysis of Macro.h/cpp, design of JIT conversion flow, test specifications | M1 | User Request, R1, R2 |
| 2 | On-Demand JIT Conversion in `findMacro()` | Dynamically convert `macroContent` to key events for matched macro at trigger time | M2 | R1 |
| 3 | AutoCaps Macro support in JIT conversion | Respect `vAutoCapsMacro` when capitalizing first letter of macro output | M2 | R1 |
| 4 | Remove pre-compilation in `initMacroMap` & `addMacro` | Stop populating `macroContentCode` during startup and dynamic insertion | M2 | R2 |
| 5 | Simplify `MacroData` and RAM storage | Eliminate pre-translated key arrays from RAM for all inactive macros | M2 | R2 |
| 6 | $O(1)$ `onTableCodeChange()` | Make table code change instantaneous with zero CPU loop overhead | M2 | R2 |
| 7 | Build verification | Successfully compile `OpenKey.exe` using `build.bat` | M2, M3 | R3 |
| 8 | Multi-encoding & continuous switching testing | Comprehensive testing of Unicode, TCVN3, VNI macro expansion & rapid switching | M3 | R3 |
| 9 | Technical documentation & performance comparison | Markdown document recording process, performance benchmarks, and architecture | M3 | R3 |

## Milestones
| # | Name | Scope | Dependencies | Status |
|---|------|-------|-------------|--------|
| 1 | Agent 1: Planning & Architectural Design | Khảo sát Macro.h/cpp, thiết kế JIT on-demand conversion, lập kế hoạch chi tiết & test cases | none | DONE |
| 2 | Agent 2: Code Implementation | Chỉnh sửa Macro.h, Macro.cpp, cập nhật findMacro, initMacroMap, addMacro, build.bat | M1 | DONE |
| 3 | Agent 3: Testing & Technical Docs | Kiểm thử gõ tắt trên Unicode, TCVN3, VNI, stress test chuyển bảng mã, viết báo cáo kỹ thuật MD | M2 | DONE |

## Interface Contracts
- `findMacro()`: Given typed input and macro keyword match, translate `macroContent` on-the-fly according to `vCodeTable`, apply `vAutoCapsMacro` if applicable, and return the key events.
- `onTableCodeChange()`: $O(1)$ zero CPU overhead.
- `initMacroMap()` / `addMacro()`: Store keyword and text, no pre-translation into RAM.
