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

## Current environment limitation
- Full compile validation for both the GTK tree and the BTK variant still depends on external toolchain availability.
- Meson was not available in the current environment during this pass.
