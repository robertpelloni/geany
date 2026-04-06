# Requirements inventory: Notepad++ command buckets from source

## Purpose

This document is the first explicit source-driven inventory spine for the broader Notepad++ parity effort.
It is based on:
- `../npp/PowerEditor/src/menuCmdID.h`

The goal is not to pretend parity is already complete, but to define the command surface that Geany must eventually cover or intentionally surpass.

## Bucket summary from `menuCmdID.h`

The current Notepad++ source exposes at least these major command buckets:

| Bucket | Approx. command count | Notes |
|---|---:|---|
| File | 28 | file lifecycle, session, folder/workspace, reload/restore, close variants |
| Edit | 106 | line transforms, whitespace, clipboard variants, sorting, multiselect, casing, path/file helpers |
| Search | 66 | Find / Replace / Incremental Find / Find in Files / Mark / bookmark navigation / marked-line actions |
| View | 88 | panels, visibility, zoom, wrap, folding, UI surfaces |
| Format | 62 | EOL, encoding, conversion, indentation, wrap/format-adjacent commands |
| Language | 100 | language selection and styling-related entries |
| Macro | 5 | record / stop / playback / save / run multiple |
| Tool | 13 | hash generation and tool-oriented helpers |
| Window | 15 | document ordering, sorting, MRU behavior |
| Settings | 12 | preferences, shortcut mapping, plugin admin, context menu, session memory |

There are also smaller buckets and singletons such as:
- plugin/admin-related commands
- forum/update/about/documentation commands
- tray/document list commands
- project page / pin tab / execute / misc helpers

## Search bucket inventory

The current search-related commands declared in `menuCmdID.h` include:

### Core find/replace/navigation
- `IDM_SEARCH_FIND`
- `IDM_SEARCH_FINDNEXT`
- `IDM_SEARCH_FINDPREV`
- `IDM_SEARCH_REPLACE`
- `IDM_SEARCH_GOTOLINE`
- `IDM_SEARCH_GOTOMATCHINGBRACE`
- `IDM_SEARCH_SELECTMATCHINGBRACES`
- `IDM_SEARCH_FINDINCREMENT`
- `IDM_SEARCH_FINDINFILES`
- `IDM_SEARCH_SETANDFINDNEXT`
- `IDM_SEARCH_SETANDFINDPREV`
- `IDM_SEARCH_VOLATILE_FINDNEXT`
- `IDM_SEARCH_VOLATILE_FINDPREV`
- `IDM_SEARCH_GOTONEXTFOUND`
- `IDM_SEARCH_GOTOPREVFOUND`

### Bookmark and marker navigation
- `IDM_SEARCH_TOGGLE_BOOKMARK`
- `IDM_SEARCH_NEXT_BOOKMARK`
- `IDM_SEARCH_PREV_BOOKMARK`
- `IDM_SEARCH_CLEAR_BOOKMARKS`
- `IDM_SEARCH_GOPREVMARKER1..5`
- `IDM_SEARCH_GOPREVMARKER_DEF`
- `IDM_SEARCH_GONEXTMARKER1..5`
- `IDM_SEARCH_GONEXTMARKER_DEF`

### Mark workflows and mark colors
- `IDM_SEARCH_MARK`
- `IDM_SEARCH_MARKALLEXT1..5`
- `IDM_SEARCH_UNMARKALLEXT1..5`
- `IDM_SEARCH_MARKONEEXT1..5`
- `IDM_SEARCH_CLEARALLMARKS`
- `IDM_SEARCH_INVERSEMARKS`

### Marked-line operations
- `IDM_SEARCH_CUTMARKEDLINES`
- `IDM_SEARCH_COPYMARKEDLINES`
- `IDM_SEARCH_PASTEMARKEDLINES`
- `IDM_SEARCH_DELETEMARKEDLINES`
- `IDM_SEARCH_DELETEUNMARKEDLINES`
- `IDM_SEARCH_MARKEDTOCLIP`

### Search-style extraction / specialized helpers
- `IDM_SEARCH_FINDCHARINRANGE`
- `IDM_SEARCH_STYLE1TOCLIP..STYLE5TOCLIP`
- `IDM_SEARCH_ALLSTYLESTOCLIP`
- `IDM_SEARCH_CHANGED_NEXT`
- `IDM_SEARCH_CHANGED_PREV`
- `IDM_SEARCH_CLEAR_CHANGE_HISTORY`

## Implication for Geany

The current Geany Search Studio already covers an important subset of this space, especially around:
- unified Find / Replace / Find in Files / Mark
- explicit Normal / Extended / Regex modes
- wrap-around exposure
- dot-matches-newline exposure
- count
- session-wide count/mark/replace preview flows
- structured results and preview panes

But the inventory also makes it clear that important NPP-style parity gaps still exist, especially around:
- marked-line edit operations
- colored/extensible mark sets
- some bookmark/marker traversal affordances
- more exact incremental-find/result-focus behavior
- style-to-clipboard and find-char-in-range style helpers

## Requirement conclusion

This inventory must be treated as a living parity checklist.
Geany should not claim “100% Notepad++ parity” until this source-defined command surface has been audited, mapped, and either implemented or deliberately superseded with a documented better equivalent.
