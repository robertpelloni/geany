# Requirements: Notepad++ parity audit and migration target

## User goal

Push Geany toward **1:1 functional parity with Notepad++**, and then exceed it, with special emphasis on:
- search / replace parity
- keeping equivalent capabilities visible somewhere in the UI
- ideally placing them in the same or very similar locations when that helps user muscle memory

## Source-of-truth approach

The audit should be driven by the Notepad++ source tree in:
- `../npp/PowerEditor/src/`

At minimum, the following source files and directories act as canonical inventory anchors:
- `menuCmdID.h` for menu command/function inventory
- `Notepad_plus.rc` for dialogs, menus, and UI placement
- `ScintillaComponent/FindReplaceDlg.cpp`
- `ScintillaComponent/FindReplaceDlg.h`
- `ScintillaComponent/FindReplaceDlg.rc`
- `WinControls/` for panels, dialogs, and workbench surfaces
- `functionList.xml`, `langs.model.xml`, `stylers*.xml` for language and styling behavior
- `NppCommands.cpp`, `NppBigSwitch.cpp`, `Notepad_plus*.cpp` for command routing and feature wiring
- `TextFXEngine.h` and `../npp/textfx/` for transform/text-manipulation parity targets

## High-level requirement categories

### 1. File/session/workspace parity
Geany should cover Notepad++ capabilities around:
- file lifecycle actions
- session persistence
- folder/workspace browsing
- reopen/reload/restore flows
- multi-document close variants
- pinned/read-only flows where appropriate

### 2. Edit/transform parity
Geany should cover Notepad++ edit features including, but not limited to:
- line operations
- whitespace conversions and trimming
- case conversions
- sorting families
- duplicate/empty-line handling
- column/block operations
- clipboard/history/char-panel-adjacent tooling
- macro-adjacent editing affordances
- TextFX-style transformations where Geany currently lacks equivalents

### 3. Search parity
Geany should cover Notepad++ search features including, but not limited to:
- Find / Replace / Find in Files / Mark in one unified surface
- modes: Normal / Extended / Regex
- direction, wrapping, in-selection semantics, transparency/compact behavior where appropriate
- count, mark, bookmark-line, clear marks, inverse marks, marked-line operations
- result navigation, found-result focus, and history handling
- replace preview and richer result inspection

### 4. View/navigation parity
Geany should cover:
- document navigation
- symbol/function/document map style affordances
- result panels
- split/multi-view or equivalent workflows where practical
- search-result and workspace-pane ergonomics

### 5. Language/style parity
Geany should cover:
- syntax/language exposure comparable to Notepad++
- UDL/custom language style workflows where practical
- theme/styling configurability
- richer theme and skin ergonomics where Geany can exceed Notepad++

### 6. Macro/automation parity
Geany should cover:
- macro recording/playback concepts where feasible
- repeatable text transformation workflows
- command palette / automation surfaces where they improve on classic menu routing

### 7. Plugin/tool extensibility parity
Geany should cover:
- plugin discoverability
- plugin management ergonomics
- external tool / run / command integration
- extension points for parity gaps too large for core-only work

## Search/replace requirement priority

The search/replace surface is a top-priority parity area.
The current Geany Search Studio direction should be judged against Notepad++'s actual source-defined behavior and UI layout, not just memory.

Priority expectations:
- unify Find / Replace / Find in Files / Mark
- keep familiar modes and options visible
- preserve quick keyboard-heavy workflows
- add result inspection, preview, and session/document scope flows that can exceed Notepad++

## Delivery expectation

The parity effort should proceed systematically:
1. inventory Notepad++ commands/features from source
2. map each feature to current Geany status:
   - present
   - partial
   - missing
   - intentionally reimagined elsewhere
3. define implementation waves
4. keep docs updated as parity advances

## Honesty requirement

Do not claim full parity until the source-driven audit has actually been completed and the missing/partial items have been closed or deliberately addressed.
