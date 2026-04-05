# Testing notes: search parity upgrades and BTK variant

## GTK-based Geany search dialog checks

### Find dialog
- Open Find
- Verify new options appear:
  - Wrap around
  - . matches newline
  - Count button in expanded area
- Confirm `. matches newline` is disabled until regex mode is enabled
- Confirm Wrap around mirrors the saved `search_prefs.always_wrap` value when reopening the dialog

### Replace dialog
- Open Replace
- Verify new options appear:
  - Wrap around
  - . matches newline
- Confirm `. matches newline` is disabled until regex mode is enabled

### Regex behavior
Use a multi-line sample document and search with regex enabled:
- pattern: `foo.*bar`
- with `. matches newline` off, confirm newline-separated spans do not match via dot-all semantics
- with `. matches newline` on, confirm newline-separated spans do match

### Count behavior
- Search for a known term with several matches
- Press Count
- Confirm the status bar reports the number of matches
- Confirm selection and indicators are not changed by Count alone

## Preferences theme selector checks
- Open Preferences → Interface
- Verify **Application skin** combo exists
- Change between:
  - Liquid Glass
  - Cyber Glass
  - Aurora Forge
- Confirm the application skin updates immediately
- Reopen Preferences and confirm the active selection persists

## BTK variant checks

### Filesystem
Confirm these exist:
- `variants/geany-btk/README.md`
- `variants/geany-btk/CMakeLists.txt`
- `variants/geany-btk/src/main.cpp`
- `subprojects/btk`

### Content guard
- Confirm `variants/geany-btk/` contains no legacy `bobgui` or `bobui` references
- Confirm the prototype models Find / Replace / Find in Files / Mark tabs
- Confirm a lower navigator exists with tabs for:
  - Activity
  - Results
  - Diff Preview
- Confirm buttons like `Count Session`, `Mark Session`, `Replace in Session`, and preview actions exist in the prototype surface
- Trigger several prototype actions and confirm structured rows are appended in the Results pane
- Select a result row and confirm the Diff Preview pane updates with richer detail text
- Activate both informational and navigable rows and confirm the prototype logs different behaviors in Activity

## Windows/MSVC build validation

### Successful BTK standalone build
Using Visual Studio 18 Insiders MSVC environment plus bundled Ninja:
- configure succeeded for `subprojects/btk`
- build succeeded for `build/btk-ninja-fixes`
- install succeeded to `build/btk-install`

### Successful BTK variant build
Using the standalone BTK install tree as a package source:
- configure succeeded for `build/geany-btk-package3`
- build succeeded for:
  - `build/geany-btk-package3/geany-btk-search-studio.exe`

### Successful BTK variant runtime smoke check
Using the built BTK runtime directory on `PATH`:
- prepended runtime path:
  - `build/btk-install/bin`
- launched executable:
  - `build/geany-btk-package3/geany-btk-search-studio.exe`
- verified process remained running and responsive after startup
- verified main window title resolved as:
  - `geany-btk-search-studio`

### Runtime caveat discovered
- launching the BTK variant executable without the BTK runtime DLL directory on `PATH` exited immediately in this environment
- a convenience launcher was added at:
  - `variants/geany-btk/run-windows-msvc.bat`
- the CMake build now also generates a launcher with the resolved BTK runtime path baked in:
  - `build/geany-btk-package3/run-geany-btk-search-studio.bat`

### Additional regression check after launcher automation
- reconfigure the BTK variant build and confirm the generated launcher exists beside the built executable
- confirm the generated launcher contains the resolved local BTK runtime directory

### Lightweight staged runtime validation
- confirm a staged runtime directory matching `build/geany-btk-package3/runtime-bundle-*` exists after rebuilding the variant
- confirm it contains:
  - `bin/geany-btk-search-studio.exe`
  - BTK runtime DLLs in `bin/`
  - plugin subdirectories such as:
    - `platforms/`
    - `imageformats/`
    - `mediaservices/`
    - `playlistformats/`
    - `printerdrivers/`
    - `sqldrivers/`
  - `runtime-bundle-<timestamp>/run-geany-btk-search-studio.bat`
- confirm the staged runtime layout is trimmed to DLLs needed for runtime rather than also containing BTK `.lib` files, CMake package metadata, or auxiliary BTK developer executables
- launch the generated staged launcher inside the newest `runtime-bundle-*` directory
- confirm the staged executable launches and the process remains responsive without manually editing global `PATH`

### BTK backend-layer validation
- confirm `variants/geany-btk/src/search_studio_backend.h` and `variants/geany-btk/src/search_studio_backend.cpp` exist
- confirm the BTK backend layer now owns request/result models for Find / Replace / Mark / Find in Files flows
- confirm result specs carry action kind / result kind / target scope metadata in addition to row text payloads
- confirm first-wave action specs exist for Find / Replace / Mark / Find in Files execution families in the BTK backend layer
- confirm the BTK backend also defines a first explicit provider seam via `SearchStudioSearchService`, with payload structs such as `SearchStudioDocumentImpact` and `SearchStudioReplacePreviewImpact`
- confirm `SearchStudioBackend::defaultSearchService()` exists and the family execution helpers also expose overloads that accept an injected `SearchStudioSearchService`
- confirm simple summary-oriented Find and Replace button flows still behave correctly after being routed through BTK action specs/execution helpers instead of only ad-hoc UI-local formatting
- confirm several BTK UI actions still append activity lines and structured rows after the backend-layer extraction rather than regressing to blank/no-op behavior

### Runtime package validation
- build the custom target:
  - `geany-btk-runtime-package`
- if desired, delete the newest `build/geany-btk-package3/runtime-bundle-*` directory first and confirm the package target still refreshes the staged runtime tree before archiving
- confirm an archive matching this pattern exists:
  - `build/geany-btk-package3/geany-btk-search-studio-runtime-<timestamp>.zip`
- inspect the zip and confirm it contains:
  - `bin/geany-btk-search-studio.exe`
  - BTK runtime DLLs in `bin/`
  - plugin subdirectories like `platforms/`, `imageformats/`, `mediaservices/`, `playlistformats/`, `printerdrivers/`, and `sqldrivers/`
  - `run-geany-btk-search-studio.bat`
- confirm the zip no longer carries BTK import libraries or CMake package metadata in the runtime payload

### Regressions to watch after the build-enablement pass
- BTK still builds standalone after the `CMAKE_CURRENT_SOURCE_DIR` CMake-module-path fix
- the Geany BTK variant still finds BTK via local install/build-tree hints and no longer depends on `add_subdirectory(...)`
- the BTK variant still compiles against current BTK string APIs (`formatArg` / `formatArgs`, `QString::fromUtf8(...)`) with `CMAKE_CXX_STANDARD 20`
- the BTK backend-shaped helper layer (`src/search_studio_backend.h/.cpp`) still builds cleanly and the UI still consumes its action-result bundles/specs correctly
- iterative rebuild/package flows still work while older staged runtime executables are running because the current staged output now uses timestamped `runtime-bundle-*` directories instead of the earlier fixed runtime path
- repeated `geany-btk-stage-runtime` / `geany-btk-runtime-package` runs can produce fresh bundle/package timestamps without requiring a manual CMake reconfigure in between
- the produced `geany-btk-search-studio.exe` launches and still presents the expected Search Studio tabs and lower navigator panes

## Current environment limitation
- Full compile validation for the main GTK Geany tree still depends on external Meson/native GTK build plumbing.
- Meson was not available in the current environment during this pass, so the production GTK application was still not built here.
