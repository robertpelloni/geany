# Implementation notes: multi-toolkit Geany variants

## Submodules added

This pass adds or restores these toolkit submodules:
- `subprojects/btk` → `https://github.com/robertpelloni/btk`
- `subprojects/bobui` → `https://github.com/robertpelloni/bobui`
- `subprojects/bobgui` → `https://github.com/robertpelloni/bobgui`

## Variant directories

### BTK
- existing directory: `variants/geany-btk/`
- remains the most evolved alternate frontend
- continues to use BTK/CopperSpice-style APIs only inside that folder
- still has the most mature backend, runtime staging, and packaging workflow
- during this pass it also gained more explicit provenance-aware run metadata in the backend (`sourceLabel`, `scopeLabel`, `targetLabel`, `scannedRootPath`) so BTK result/status rows can distinguish workspace-backed data from prototype fallback more honestly

### BobUI
- added directory: `variants/geany-bobui/`
- files:
  - `CMakeLists.txt`
  - `README.md`
  - `src/main.cpp`
  - `src/search_studio_backend.h`
  - `src/search_studio_backend.cpp`
- this variant uses Qt6-compatible package discovery intended for BobUI-provided `Qt6` packages
- it keeps the Search Studio shell and backend model in a BobUI-only folder so the code path can evolve independently from BTK

### BobGUI
- added directory: `variants/geany-bobgui/`
- files:
  - `meson.build`
  - `README.md`
  - `src/main.c`
- this variant is intentionally smaller right now: a BobGUI-only Search Studio shell with toolkit-exclusive pages and an activity surface
- it uses BobGUI/GObject-style APIs only

## Build validation performed

### BTK
- rebuilt and repackaged successfully in this environment using the existing Windows/MSVC workflow
- this remains the only fully validated alternate-toolkit runtime/package path in the current environment

### BobUI
- build scaffolding was added
- full local build validation still depends on a usable BobUI / Qt6 package tree being available to CMake
- no such BobUI package/install tree was already validated in this environment during this pass

### BobGUI
- build scaffolding was added with Meson
- full local build validation remains blocked in this environment because Meson / pkg-config / BobGUI dev plumbing are still unavailable here

## Important limitation

The request was to make Geany able to compile a version for each toolkit and build the project. The repo now has explicit toolkit-exclusive variant build definitions for all three toolkits, but empirical build success is still asymmetric:
- BTK: validated here
- BobUI: scaffolding added, local package/toolchain still needed
- BobGUI: scaffolding added, local Meson/pkg-config/toolchain still needed

That is the honest state of the implementation in this environment.
