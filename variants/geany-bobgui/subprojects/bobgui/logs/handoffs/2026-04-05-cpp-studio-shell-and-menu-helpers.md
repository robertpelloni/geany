# Bobgui Handoff — 2026-04-05 — C++ Studio Shell and Menu Helpers

## Overview
This pass continued moving the C++ layer from basic wrappers toward a more opinionated application-framework façade.

## Implemented

### 1. Action listing helper
`bobgui/cpp/action_registry.hpp` now adds:
- `list_actions()`

That complements the earlier visitor support and makes action inspection more natural for C++ callers.

### 2. AppShell menu/tool convenience
`bobgui/cpp/app_shell.hpp` now exposes:
- `menu_model()`
- `visit_actions()`
- `list_actions()`

This helps C++ shell code drive menu/tool surfaces from the shared action model more directly.

### 3. StudioShell preset
Added:
- `bobgui/cpp/studio_shell.hpp`

This preset builds on `AppShell` and gives a more tool-oriented vocabulary:
- navigation panel
- document view
- inspector panel

It also initializes dock-manager support through the preset path.

### 4. Example update
`examples/workbench-demo/main.cpp` now uses `StudioShell`, which makes the example feel more like a real tool-shell starting point.

### 5. Documentation
Updated:
- `docs/CPP_APP_FRAMEWORK_LAYER.md`

Added:
- `docs/CPP_STUDIO_SHELL_PRESET_2026-04-05.md`

## Validation
- literal rename audit: clean
- `git diff --check`: clean

## Blockers
Real build validation remains blocked by missing environment tooling:
- `meson`
- Python `mesonbuild`
- `g++`

## Recommended continuation
1. Add richer action-driven toolbar/tool-surface helpers.
2. Deepen dock/workspace behavior once the C dock layer becomes less skeletal.
3. Continue public-header branding cleanup where safe.
4. Run full build validation as soon as the environment supports it.
