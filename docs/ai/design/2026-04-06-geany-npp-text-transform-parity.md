# Design: Geany vs Notepad++ Text Transformation Parity

## Source anchors
- `../npp/PowerEditor/src/menuCmdID.h` (IDM_EDIT range)
- `../npp/PowerEditor/src/TextFXEngine.h`
- `../npp/PowerEditor/src/NppCommands.cpp`

## Purpose
Inventory Notepad++'s advanced text manipulation features and track Geany's parity status.

## Transformation Parity Matrix

| NPP Command / TextFX Feature | NPP Command ID | Geany Search Studio | Geany Core / Menu | Status |
|---|---|---|---|---|
| Delete Blank Lines | `IDM_EDIT_REMOVEEMPTYLINES` | `Transform -> Delete Blank Lines` | `Edit -> Format -> Strip trailing spaces` (Partial) | Present |
| Delete Surplus Blank Lines | `IDM_EDIT_REMOVEEMPTYLINESWITHBLANK` | `Transform -> Delete Surplus Blank Lines` | N/A | Present |
| Zap Non-Printable Characters | `TextFX -> Zap Non-Printable` | `Transform -> Zap Non-Printable` | N/A | Present |
| Invert Case | `IDM_EDIT_INVERTCASE` | `Transform -> Invert Case` | `Edit -> Format -> Toggle Case` (Close) | Present |
| Proper Case | `IDM_EDIT_PROPERCASE_FORCE` | N/A (Planned) | `Edit -> Format -> Title Case` | Partial |
| Sentence Case | `IDM_EDIT_SENTENCECASE_FORCE` | N/A (Planned) | N/A | Missing |
| Random Case | `IDM_EDIT_RANDOMCASE` | N/A | N/A | Missing |
| Duplicate Line | `IDM_EDIT_DUP_LINE` | N/A | `Edit -> Duplicate Line` | Present |
| Split Lines | `IDM_EDIT_SPLIT_LINES` | N/A | `Edit -> Format -> Split Lines` | Present |
| Join Lines | `IDM_EDIT_JOIN_LINES` | N/A | `Edit -> Format -> Join Lines` | Present |
| Line Up / Down | `IDM_EDIT_LINE_UP/DOWN` | N/A | `Edit -> Move Line(s) Up/Down` | Present |
| Redact Selection | `IDM_EDIT_REDACT_SELECTION` | `Transform -> Redact Selection` | N/A | Present |
| Trim Trailing Space | `IDM_EDIT_TRIMTRAILING` | N/A | `Edit -> Format -> Strip trailing spaces` | Present |
| Trim Leading Space | `IDM_EDIT_TRIMLINEHEAD` | N/A | N/A | Missing |
| Trim Both | `IDM_EDIT_TRIM_BOTH` | N/A | N/A | Missing |
| EOL to Space | `IDM_EDIT_EOL2WS` | N/A | N/A | Missing |
| Tab to Space / Space to Tab | `IDM_EDIT_TAB2SW` / `SW2TAB` | N/A | `Document -> Replace Tabs by Spaces` | Present |
| Sort Lines Lexicographic | `IDM_EDIT_SORTLINES_LEXICOGRAPHIC_ASC` | N/A | `Edit -> Format -> Send Selection to -> sort` | Partial |

## Strategic Conclusion
Geany has many of the core "Edit" features in the `Edit` menu, but NPP's specialized "Blank Operations" and `TextFX` transforms are more discoverable and powerful for systematic cleaning.

The new **Search Studio -> Transform** page serves as the dedicated home for these "Cockpit" style systematic modifications, whereas the `Edit` menu remains for day-to-day editing shortcuts.

## Wave 1 Implementation (Completed)
- Delete Blank Lines
- Delete Surplus Blank Lines
- Zap Non-Printable
- Invert Case
- Redact Selection

## Wave 2 Implementation (Planned)
- Proper Case / Sentence Case refinements
- Trim variants
- EOL to Space
- Line Numbering toggles (NPP style)
