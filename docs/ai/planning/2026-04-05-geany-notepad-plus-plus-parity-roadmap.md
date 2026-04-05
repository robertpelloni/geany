# Planning: Notepad++ parity roadmap for Geany

## Audit method

Use `../npp/PowerEditor/src/menuCmdID.h` as the first command inventory spine, then cross-check each category against:
- `Notepad_plus.rc`
- command routing in `NppCommands.cpp`, `NppBigSwitch.cpp`, and `Notepad_plus*.cpp`
- subsystem dialogs/panels in `ScintillaComponent/` and `WinControls/`
- auxiliary feature trees such as `../npp/textfx/`

## Phase breakdown

### Phase 1 — inventory and classification
For each Notepad++ command/functionality bucket, classify Geany state as:
- parity present
- partial / weaker
- missing
- intentionally relocated / improved elsewhere

Initial top-level buckets from `menuCmdID.h`:
- File
- Edit
- Search
- View
- Format
- Language
- Settings
- Macro
- Run
- Plugins
- Window / panel management
- Tools / dialogs / specialized panels

### Phase 2 — search-first closure
Because the user explicitly called out search parity, prioritize:
- exact Find / Replace / Find in Files / Mark affordance mapping
- dialog/surface option mapping from `FindReplaceDlg.*`
- marked-line workflows
- result navigation and found-result focus
- replace-preview and count/impact semantics

### Phase 3 — text transformation closure
Use Notepad++ edit commands plus TextFX-related sources to define:
- transforms Geany already has
- transforms that belong in a new richer Geany transform workbench
- transforms that should land in core vs plugin/extension layers

### Phase 4 — workspace/panel/navigation closure
Map Notepad++ panels and workbench surfaces against Geany equivalents:
- folder/workspace tree
- function list / symbol map / document map-like behaviors
- result panes
- clipboard/history/char/browser panels

### Phase 5 — language/theme/extensibility closure
Map:
- language inventory
- user-defined language analogues
- theme/styler coverage
- plugin/admin/settings surfaces

## Immediate execution wave

### Wave A
- preserve and extend current Search Studio work
- keep BTK alternate frontend aligned with backend normalization
- use the Notepad++ search source as the parity checklist for next GTK/BTK search passes

### Wave B
- produce a source-driven parity matrix document with explicit status for every major Notepad++ command bucket
- identify the first highest-value gaps outside search (especially edit/transform workflows)

### Wave C
- implement missing parity features in coherent clusters rather than random one-offs
- document which Geany UI surface exposes each Notepad++ equivalent or improvement

## Non-goal for a single pass

Do not pretend one session can close all Notepad++ parity gaps. The correct process is:
- audit honestly
- close gaps in prioritized waves
- keep implementation/docs/testing synchronized
