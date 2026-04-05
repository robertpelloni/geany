# Testing notes: Geany modernization pass

## Manual verification checklist

### Submodule
- `git submodule status` shows `subprojects/btk`
- `.gitmodules` points at `https://github.com/robertpelloni/btk`

### Vertical tabs
- Start Geany with a fresh config
- Confirm editor tabs default to the left side
- Confirm message window tabs still default to the left side as before

### UI themes
Set `ui_theme` in Geany config and restart:
- `liquid-glass`
- `cyber-glass`
- `aurora-forge`

Verify:
- no startup warnings for valid theme names
- invalid theme names fall back to `liquid-glass`
- notebook tabs remain readable in vertical orientation
- menus, toolbars, entries, buttons, and statusbar are styled consistently

### Colorschemes
- Open the colorscheme chooser
- Confirm the following appear:
  - Liquid Glass
  - Cyber Glass
  - Aurora Forge
- Verify syntax colors remain readable across at least C, Python, HTML, and diff files

## Recommended automated checks

- Meson configure + compile smoke test
- Grep check to ensure direct `#include <gtk/gtk.h>` use in core sources stays centralized via `gtkcompat.h`
- Grep check to ensure recently normalized seams (tab-position application and CSS-provider attachment) continue flowing through `gtkcompat.h` helpers rather than growing fresh raw toolkit calls in parallel
- Config load/save test for `ui_theme`

## Known limitation in this pass

This pass does **not** complete a BTK-native build. It prepares the repository for a staged migration and documents the safer path instead of forcing an uncompilable one-shot swap.
