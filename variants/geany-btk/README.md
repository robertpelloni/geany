# Geany BTK Variant

This directory hosts an **experimental BTK-based alternate frontend** for Geany.

## Purpose

- explore a BTK-native frontend path without destabilizing the current production tree
- prototype richer, Notepad++-style search UX and broader command surfaces
- give the project a clean place to test alternate UI architecture choices

## Current scope

This variant currently contains a standalone **Search Studio** prototype with:

- Find
- Replace
- Find in Files
- Mark
- lower navigator tabs for:
  - Activity
  - Results
  - Diff Preview

The UI intentionally mirrors the density and speed of Notepad++ while adding room for richer preview, workflow shortcuts, and future command-palette integration.

The current BTK prototype now mirrors several of the matured Search Studio concepts from the main tree:
- structured result rows instead of only static form controls
- lower-pane navigator behavior
- session-aware actions like `Count Session` and `Mark Session`
- preview-oriented replace and impact routing
- informational vs navigable result-row behavior

## Toolchain direction

The prototype is written against BTK's Qt-style widget stack and is expected to be built against the BTK submodule at:

- `../../subprojects/btk`

At the moment, the most reliable Windows/MSVC path is:
1. configure and build BTK standalone from `../../subprojects/btk`
2. configure this variant against the resulting BTK CMake package/build tree

The variant CMake currently looks for BTK in common local install/build-tree locations such as:
- `../../build/btk-install/lib/cmake`
- `../../build/btk-install/cmake`
- `../../build/btk-ninja-fixes`
- `../../build/btk-ninja`
- `../../build/btk`

or any BTK installation discoverable by `find_package(BTK CONFIG ...)`.

## Windows launch notes

After building BTK standalone and the Geany BTK variant, the executable needs BTK runtime DLLs on `PATH`.

The validated local layout in this repo is:
- BTK install/runtime: `../../build/btk-install/bin`
- variant executable: `../../build/geany-btk-package3/geany-btk-search-studio.exe`

A convenience launcher is included for this workflow:
- source-tree helper: `run-windows-msvc.bat`

That launcher prepends the local BTK runtime directory to `PATH` and starts the built variant executable.

The CMake build now also generates a build-directory launcher with the resolved BTK runtime path baked in:
- `../../build/geany-btk-package3/run-geany-btk-search-studio.bat`

In addition, the build now stages a self-contained local runtime layout under:
- `../../build/geany-btk-package3/runtime/`
  - `bin/geany-btk-search-studio.exe`
  - BTK runtime DLLs in `bin/`
  - BTK plugin/runtime-support DLLs in `lib/`
  - `run-geany-btk-search-studio.bat`

The build can also package that staged layout into a zip archive via the custom target:
- `geany-btk-runtime-package`

Validated local package artifact:
- `../../build/geany-btk-package3/geany-btk-search-studio-runtime.zip`

This makes the local Windows/MSVC runtime path less manual after configuration/build succeeds and gives the project an initial deploy-style handoff without requiring a full installer.

## Why a separate variant exists

Geany's main codebase is still strongly shaped by the current toolkit and plugin-facing contracts. A separate BTK variant allows faster experimentation with:

- dockable and tabbed dialogs
- denser search UX
- cross-platform native-feeling widget behavior
- future command-palette and transform-tooling work

without forcing an all-at-once migration of the production application.

## Next steps

1. connect the Search Studio controls to Geany core search services
2. replace prototype result generation with real document/session/search backend data
3. port command-palette and transform tooling into this variant
4. define the boundary between reusable Geany core logic and BTK-native presentation
5. trim the staged BTK runtime/package layout to only the files actually needed by the prototype, or evolve the zip output into a more formal install/release step
