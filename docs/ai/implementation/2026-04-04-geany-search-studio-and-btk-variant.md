# Implementation notes: Search Studio parity work and BTK alternate variant

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

## BTK alternate variant

A new experimental alternate frontend folder was added at:

- `variants/geany-btk`

### Why it exists
The BTK stack is a better fit for a **greenfield alternate frontend** than for an all-at-once replacement inside current GTK-oriented Geany.

This variant gives the project a place to explore:
- tabbed search UX
- denser command surfaces
- richer previews
- BTK-native layout behavior
- a future high-end Geany frontend without destabilizing the production tree

### What is in the variant now
- `README.md` explaining the role of the variant
- `CMakeLists.txt` wiring against BTK CMake package/config discovery, with local hints for standalone BTK build/install trees
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

A Windows/MSVC build-enablement pass also landed for the BTK path:
- BTK was updated to newer upstream MSVC-oriented fixes
- a BTK-side CMake fix was applied so BTK resolves its own `cmake/modules` from its current source directory instead of assuming it is always the top-level source tree
- the Geany BTK variant no longer tries to consume BTK via `add_subdirectory(...)`; it now prefers a standalone BTK build/install tree via `find_package(BTK CONFIG ...)`
- the BTK variant itself was updated for the current BTK string API (`formatArg` / `formatArgs`, `QString::fromUtf8(...)`, and C++20)

This produced a successful standalone BTK build and a successful BTK variant executable build on Windows/MSVC, while the main GTK Geany tree remains blocked by missing Meson/native GTK build plumbing in this environment.

A runtime smoke pass after the build also clarified one more Windows concern: the variant executable currently depends on BTK runtime DLLs being discoverable on `PATH`. Launching the executable directly without the BTK runtime directory available caused immediate process exit in this environment, while launching it with `build/btk-install/bin` prepended to `PATH` successfully produced a responsive `geany-btk-search-studio` window. A small Windows launcher batch file was added to make that local workflow repeatable.

To reduce that manual runtime step further, the variant CMake now derives the BTK runtime directory from the discovered BTK package location and generates a build-directory launcher batch file automatically. That keeps the package-based BTK integration model while making the common local Windows/MSVC run path much more direct.

That launcher work has now been pushed one step further into a lightweight local deploy-style layout. The BTK variant build stages:
- the variant executable into `runtime/bin/`
- BTK runtime DLLs into `runtime/bin/`
- plugin DLLs into deployment-shaped subdirectories such as:
  - `runtime/platforms/`
  - `runtime/imageformats/`
  - `runtime/mediaservices/`
  - `runtime/playlistformats/`
  - `runtime/printerdrivers/`
  - `runtime/sqldrivers/`
- a launcher batch file into `runtime/`

This staging was also tightened so the local runtime/package layout now carries only the executable plus relevant runtime/plugin DLLs rather than also dragging along BTK import libraries, CMake package metadata, and extra build-time helper binaries.

On top of that, the variant CMake now exposes a lightweight runtime package target (`geany-btk-runtime-package`) which archives the staged runtime tree into a zip file. In the validated local workflow this produces:
- `build/geany-btk-package3/geany-btk-search-studio-runtime.zip`

The package target now also depends on an explicit runtime-refresh target, so packaging still reconstructs the staged runtime tree even if `runtime/` was deleted between builds.

This is intentionally still a developer-facing staging layout, not a polished package, but it is a meaningful step because it turns the successful build into a more portable local runtime artifact instead of only a build-tree executable plus PATH instructions.

### Prototype goals
The prototype intentionally models the **control density** and **workflow rhythm** of Notepad++ while leaving room for “better than N++” capabilities:
- preview panes
- richer action summaries
- future command-palette integration
- future transform-tooling integration

This evolution is important because it tests the emerging frontend contract before engine wiring is complete. The BTK variant can now validate that the intended cockpit is not just tabbed, but also navigator-driven.

## BTK submodule

A second UI toolkit submodule was added:
- `subprojects/btk`

This is separate from the earlier Bob toolkit exploratory path and allows the repo to compare:
- staged compatibility migration in the GTK production tree
- greenfield alternate UI exploration in a BTK-native folder

## Recommendation for next pass

### GTK tree
1. Add a **unified Search Studio dialog** in the main Geany tree
2. Add explicit **Normal / Extended / Regex** mode presentation
3. Add **Mark** options beyond highlight-only behavior
4. Add preview/count summaries for replace-all operations

### BTK variant
1. Wire the prototype to real Geany search logic
2. Replace prototype-generated result rows with backend-driven document/session/search rows
3. Add command palette entry points
4. Add transform actions inspired by TextFX / TextLab
