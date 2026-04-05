# Testing notes: multi-toolkit Geany variants

## Submodule checks

Confirm these submodules exist in `.gitmodules` and `git submodule status`:
- `subprojects/btk`
- `subprojects/bobui`
- `subprojects/bobgui`

## Variant filesystem checks

Confirm these directories exist:
- `variants/geany-btk/`
- `variants/geany-bobui/`
- `variants/geany-bobgui/`

## Toolkit-isolation checks

### BTK
- confirm `variants/geany-btk/` remains BTK/CopperSpice-oriented

### BobUI
- confirm `variants/geany-bobui/` contains only BobUI / Qt-style UI code
- confirm no BTK or BobGUI widget/toolkit dependencies are introduced in that folder

### BobGUI
- confirm `variants/geany-bobgui/` contains only BobGUI / GObject-style UI code
- confirm no BobUI / BTK widget/toolkit code is introduced in that folder

## Build checks

### BTK
- build target:
  - `build/geany-btk-package3/geany-btk-search-studio.exe`
- custom package target:
  - `geany-btk-runtime-package`
- confirm latest staged runtime bundle launches responsively without killing older staged processes
- confirm BTK provenance/status rows continue to surface source/scope/root metadata rather than regressing to anonymous summaries

### BobUI
- configure with CMake against a BobUI-provided Qt6 package tree
- confirm `geany-bobui-search-studio` links against BobUI-provided Qt6-compatible modules
- if no BobUI package tree is available locally, record that as an environment block rather than pretending the variant was validated

### BobGUI
- configure with Meson in an environment with `bobgui4` development packages available
- confirm `meson setup` resolves `dependency('bobgui4')`
- if Meson / pkg-config / BobGUI development packages are missing locally, record that as an environment block rather than pretending the variant was validated
