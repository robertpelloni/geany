# Implementation notes: Search Studio parity work and BobUI alternate variant

## Search dialog modernization work landed in the GTK-based Geany tree

This pass focused on **high-value, low-risk parity upgrades** inspired by Notepad++'s search surface without breaking the existing Geany search engine.

### 1. Search parity features added directly to Geany

#### Wrap around surfaced directly inside Find / Replace
Notepad++ keeps wrap behavior visible in the search surface. Geany previously had wrap behavior controlled primarily through preferences and the wrap prompt path.

What changed:
- Added a **Wrap around** checkbox directly to the shared Find / Replace option area.
- The checkbox is wired to `search_prefs.always_wrap`.
- The checkbox state is synchronized whenever the Find or Replace dialog opens.

Why this matters:
- reduces hidden state
- matches user expectation from dense search UIs like Notepad++
- lowers friction for quick one-off search mode changes

#### Regex “dot matches newline” support
Notepad++ exposes a specific regex option for whether `.` matches newline.

Geany previously had:
- regex mode
- multi-line matching

But it did **not** expose a true dot-all style switch.

What changed:
- added `GEANY_FIND_DOTALL`
- added persisted settings for find/replace:
  - `find_regexp_dot_matches_newline`
  - `replace_regexp_dot_matches_newline`
- added a shared checkbox:
  - `. matches newline`
- wired regex compilation to `G_REGEX_DOTALL`

Why this matters:
- brings Geany closer to Notepad++ regex ergonomics
- separates **multi-line search behavior** from **dot-all regex behavior**, which are related but not the same
- removes a real parity gap

#### Count command in Find dialog
Notepad++ offers strong search-result introspection. Counting matches is part of that “dense utility” workflow.

What changed:
- added a **Count** action in the Find dialog's expanded action row
- implemented `search_count_matches()` to count results without adding indicators or mutating selection state

Why this matters:
- quick quantitative feedback
- useful before replace operations
- good building block for future Search Studio result summaries

## Search parity work not yet complete

Notepad++ still has a richer integrated search surface than current Geany in several areas:

### Remaining parity gaps
1. **Unified notebook dialog** with Find / Replace / Find in Files / Mark in one surface
2. **Mark tab workflow** with bookmark-line and purge semantics
3. **Visibility / transparency / compact mode** preferences for search windows
4. **Richer incremental search state and count summaries**
5. **Swap/find-replace ergonomics** and denser action placement
6. **Find in Projects panel mapping**
7. **Search mode radio group** presented more explicitly as:
   - Normal
   - Extended
   - Regular expression

### Safer migration strategy for search UX
Instead of replacing the current dialogs in one pass, the recommended path is:

1. Keep current search engine contracts stable
2. Introduce shared search-option models
3. Build a unified tabbed Search Studio frontend on top of those contracts
4. Add preview/result panes and richer scope controls
5. Retire old dialogs only when parity is comfortably exceeded

## BobUI alternate variant

A new experimental alternate frontend folder was added at:

- `variants/geany-bobui`

### Why it exists
The BobUI stack is a better fit for a **greenfield alternate frontend** than for an all-at-once replacement inside current GTK-oriented Geany.

This variant gives the project a place to explore:
- tabbed search UX
- denser command surfaces
- richer previews
- Qt/BobUI-native layout behavior
- a future high-end Geany frontend without destabilizing the production tree

### What is in the variant now
- `README.md` explaining the role of the variant
- `CMakeLists.txt` wiring against `subprojects/bobui`
- `src/main.cpp` implementing a **Search Studio prototype** with tabs:
  - Find
  - Replace
  - Find in Files
  - Mark

The prototype no longer stops at four static forms. It now mirrors more of the matured GTK Search Studio model by adding:
- a lower `Activity / Results / Diff Preview` navigator
- structured result rows via a Qt tree widget
- preview updates on row selection
- informational vs navigable row behavior on row activation
- prototype session-aware actions such as `Count Session` and `Mark Session`
- prototype replace preview / impact rows with richer preview text

### Prototype goals
The prototype intentionally models the **control density** and **workflow rhythm** of Notepad++ while leaving room for “better than N++” capabilities:
- preview panes
- richer action summaries
- future command-palette integration
- future transform-tooling integration

This evolution is important because it tests the emerging frontend contract before engine wiring is complete. The BobUI variant can now validate that the intended cockpit is not just tabbed, but also navigator-driven.

## BobUI submodule

A second UI toolkit submodule was added:
- `subprojects/bobui`

This is separate from the BobGUI exploratory path and allows the repo to compare:
- staged compatibility migration in the GTK production tree
- greenfield alternate UI exploration in a BobUI-native folder

## Recommendation for next pass

### GTK tree
1. Add a **unified Search Studio dialog** in the main Geany tree
2. Add explicit **Normal / Extended / Regex** mode presentation
3. Add **Mark** options beyond highlight-only behavior
4. Add preview/count summaries for replace-all operations

### BobUI variant
1. Wire the prototype to real Geany search logic
2. Replace prototype-generated result rows with backend-driven document/session/search rows
3. Add command palette entry points
4. Add transform actions inspired by TextFX / TextLab
