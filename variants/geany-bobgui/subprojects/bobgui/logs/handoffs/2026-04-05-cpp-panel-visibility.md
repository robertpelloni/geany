# Bobgui Handoff — 2026-04-05 — C++ Panel Visibility Toggles

## Overview
This pass wired up the actual UI visibility behavior for the panel toggles that were previously only updating status messages.

## Implemented

### 1. New C API Getters
Added `bobgui_workbench_get_left_sidebar()`, `bobgui_workbench_get_right_sidebar()`, and `bobgui_workbench_get_central()` to the core `BobguiWorkbench` API.

### 2. AppShell visibility helpers
`bobgui/cpp/app_shell.hpp` now provides:
- `set_left_sidebar_visible()`
- `set_right_sidebar_visible()`
- `set_toolbar_visible()`
- `set_menubar_visible()`

These connect directly to the underlying `BobguiWidget` visibility logic.

### 3. Preset integration
The more opinionated presets forward these helpers using domain-specific semantics:
- **StudioShell**: `set_navigation_panel_visible()`, `set_inspector_panel_visible()`
- **DocumentShell**: `set_outline_panel_visible()`, `set_details_panel_visible()`
- **DashboardShell**: `set_navigation_panel_visible()`, `set_context_panel_visible()`

### 4. Example effect
The `examples/workbench-demo/main.cpp`, `examples/document-demo/main.cpp`, and `examples/dashboard-demo/main.cpp` examples were updated.

The panel toggle commands now:
1. read current checked state from `ActionRegistry`
2. toggle state
3. use the shell-level visibility helpers to actually show/hide sidebars
4. update the status bar

### 5. Documentation
Updated `docs/CPP_APP_FRAMEWORK_LAYER.md`.
Added `docs/CPP_PANEL_VISIBILITY_2026-04-05.md`.

## Validation
- literal rename audit: clean
- `git diff --check`: clean

## Blockers
Real build validation remains blocked by missing environment tooling:
- `meson`
- Python `mesonbuild`
- `g++`

## Recommended continuation
1. Deepen dock/workspace-oriented shell helpers on top of app/studio/document/dashboard shell presets.
2. Consider build-wiring multiple C++ examples once toolchain support exists.
3. Continue modernizing visible inherited public header surfaces.
4. Run full build validation as soon as the environment supports it.
