# Design: Geany Search Studio vs Notepad++ search dialog parity

## Source anchors

This comparison is driven by the Notepad++ source, especially:
- `../npp/PowerEditor/src/ScintillaComponent/FindReplaceDlg.h`
- `../npp/PowerEditor/src/ScintillaComponent/FindReplaceDlg.cpp`
- `../npp/PowerEditor/src/ScintillaComponent/FindReplaceDlg.rc`

## Notepad++ search model from source

The Notepad++ search surface is explicitly organized around:
- Find
- Replace
- Find in Files
- Find in Projects
- Mark

It also has explicit search option state in `FindOption` for:
- whole word
- match case
- wrap around
- direction
- search type (`FindNormal`, `FindExtended`, `FindRegex`)
- purge / bookmark line
- in selection
- directory / filters / recursive / hidden-dir
- project panels 1/2/3
- dot matches newline
- line-number matching for find-in-finder

This is a strong source-defined model, not just a UI coincidence.

## Geany current direction

Geany's current GTK Search Studio already aligns with several major Notepad++ search ideas:
- unified notebook surface
- explicit Normal / Extended / Regex modes
- direct Count action
- Find / Replace / Find in Files / Mark pages
- lower Activity / Results / Diff Preview panes
- replace preview / impact routing
- session-oriented count / mark / replace preview workflows

That means Geany is already structurally on the correct path for parity and in some areas is moving beyond classic NPP behavior.

## Exact parity matrix for the main search surface

| Notepad++ source-defined search feature | NPP source evidence | Geany current state | Status |
|---|---|---|---|
| Unified tabbed Find/Replace/Find in Files/Mark surface | `FindReplaceDlg` + tab insertions in `FindReplaceDlg.cpp` | GTK Search Studio has Find / Replace / Find in Files / Mark in one notebook | Present |
| Separate Find in Projects tab | `FINDINPROJECTS_DLG`, project-panel controls in `.rc` and `FindOption` | Geany Search Studio now exposes a first project-aware `Find in Project` action on the Find in Files page, but it still does not yet expose a dedicated Find in Projects tab with project-panel-specific controls | Partial |
| Explicit Normal / Extended / Regex modes | `SearchType` enum + `IDNORMAL`, `IDEXTENDED`, `IDREGEXP` | Present in GTK Search Studio and BTK/BobUI prototypes | Present |
| Dot matches newline toggle | `IDREDOTMATCHNL`, `_dotMatchesNewline`, `SCFIND_REGEXP_DOTMATCHESNL` | Present in Geany dialogs and Search Studio | Present |
| Wrap around | `IDWRAP` / `_isWrapAround` | Present in Geany dialogs and Search Studio | Present |
| Match case | `IDMATCHCASE` / `_isMatchCase` | Present | Present |
| Match whole word only | `IDWHOLEWORD` / `_isWholeWord` | Present | Present |
| Backward direction | `IDC_BACKWARDDIRECTION` / `_whichDirection` | Present in Geany Search Studio surfaces | Present |
| In selection | `IDC_IN_SELECTION_CHECK` / `_isInSelection` | Present in Search Studio replace/document flows | Present / Partial depending on page |
| Count button | `IDCCOUNTALL` | Present in Geany Search Studio | Present |
| Find All in current document | `IDC_FINDALL_CURRENTFILE` | Present via Search Studio collected/current-document results | Present |
| Find All in all opened documents | `IDC_FINDALL_OPENEDFILES` | Present via session/open-document collection flows | Present |
| Replace in all opened documents | `IDC_REPLACE_OPENEDFILES` | Present as session replace flow | Present |
| Mark All | `IDCMARKALL` | Present | Present |
| Clear all marks | `IDC_CLEAR_ALL` | Present | Present |
| Bookmark line | `IDC_MARKLINE_CHECK` / `_doMarkLine` | Present | Present |
| Purge for each search | `IDC_PURGE_CHECK` / `_doPurge` | Present | Present |
| Mark colors/extensions 1..5 | `IDM_SEARCH_MARKALLEXT1..5`, `MARKONEEXT1..5` | Geany has mark/bookmark flows but not the same multi-color mark-set model | Partial |
| Marked-line edit operations | `CUTMARKEDLINES`, `COPYMARKEDLINES`, `PASTEMARKEDLINES`, `DELETEMARKEDLINES`, `DELETEUNMARKEDLINES`, `MARKEDTOCLIP` | Search Studio now exposes Copy/Cut/Delete Marked Lines plus Delete Unmarked Lines in the active document, but does not yet mirror paste-marked-lines or every exact NPP variant | Partial |
| Inverse marks | `IDM_SEARCH_INVERSEMARKS` | Search Studio now exposes `Inverse Marks` in the active document, implemented as line-wise inversion over current search indicator/bookmark coverage | Present / Partial |
| Incremental Find dialog | `IDM_SEARCH_FINDINCREMENT`, `IDD_INCREMENT_FIND` | Geany has search workflows but not the same explicit incremental dialog surface | Partial |
| Transparency controls | `IDC_TRANSPARENT_*` controls in `.rc` | Geany does not currently mirror NPP transparency controls in Search Studio | Missing |
| 2-button mode / split-button find UX | `IDC_2_BUTTONS_MODE`, swap button in `.rc` | Geany does not mirror this exact affordance | Missing |
| Find in Files filters field | `IDD_FINDINFILES_FILTERS_COMBO` | Geany has Find in Files, but Search Studio option density is still slimmer than NPP | Partial |
| Directory browse / set dir from doc | browse buttons in `.rc` | Geany Find in Files is functional, but exact NPP affordance parity is incomplete in Search Studio | Partial |
| Recursive / hidden-dir toggles | `IDD_FINDINFILES_RECURSIVE_CHECK`, `IDD_FINDINFILES_INHIDDENDIR_CHECK` | Recursive-style behavior exists conceptually; hidden-dir parity is not surfaced the same way | Partial |
| Find in Projects explicit project panel toggles | `IDD_FINDINFILES_PROJECT1/2/3_CHECK` | No exact Search Studio parity yet | Missing |
| Found-result focus / next found / prev found | `IDM_FOCUS_ON_FOUND_RESULTS`, `GOTONEXTFOUND`, `GOTOPREVFOUND` | Geany has navigable results and Diff Preview, but exact NPP-style result-focus commands are still not fully mirrored | Partial |
| Find in Finder / search within results | `FindInFinderDlg`, `ProcessFindInFinder` | Geany Search Studio now exposes a first `Find in Results` action that searches existing structured result rows and appends matching result-hit rows, but it does not yet mirror NPP's dedicated Find in Finder dialog and all its line-number/result-only semantics | Partial |

## Strategic conclusion

The search/replace direction is already good enough that Geany should **continue refining Search Studio**, not replace it with a copy-paste imitation of the old fragmented dialogs.

However, the parity matrix makes three next implementation clusters especially clear:

### Cluster 1 — exact search workbench gaps
- full Find in Projects page/controls beyond the new project-aware launch action
- denser Find in Files options matrix
- result-focus navigation commands
- fuller search-within-results / finder-in-finder behavior beyond the new first `Find in Results` action

### Cluster 2 — Mark parity gaps
- finish the remaining marked-line parity beyond the first active-document cluster (especially paste-marked-lines / marked-text-to-clipboard variants)
- richer multi-mark-set / mark-color behavior
- search-within-marks/result-style mark workflows

### Cluster 3 — NPP dialog-specific ergonomics
- transparency / compact behavior
- some swap/2-button affordances
- more exact result-navigation muscle-memory shortcuts

## Design principle going forward

Parity should be **behavior-first and source-driven**:
- preserve Geany's stronger modern result/preview architecture
- add exact NPP affordances where they matter for muscle memory
- exceed NPP where Geany's Search Studio can clearly do better

That is better than a superficial clone and also more honest than claiming parity too early.
