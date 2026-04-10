# Bobgui Handoff — 2026-04-05 — C++ Toolbar Semantics Pass

## Overview
This pass kept improving generated action surfaces while making the most common shell-level call paths simpler and more opinionated.

## Implemented

### 1. ToolSurfaceBuilder preset factory
`bobgui/cpp/tool_surface_builder.hpp` now adds:
- `ToolSurfaceBuilder::Options::detailed()`

This gives a cleaner entry point for fuller descriptive tool-panel surfaces.

### 2. ToolbarBuilder semantics and presets
`bobgui/cpp/toolbar_builder.hpp` now adds:
- `prefer_toggle_controls`
- `ToolbarBuilder::Options::compact()`
- `ToolbarBuilder::Options::labeled()`

This lets the toolbar path express stronger semantics for:
- compact quick actions
- labeled command strips
- toggle-oriented controls

### 3. Shell-level preset helpers
`bobgui/cpp/app_shell.hpp` now exposes:
- `build_descriptive_tool_surface_widget()`
- `build_labeled_toolbar_widget()`
- `build_compact_toolbar_widget()`

`bobgui/cpp/studio_shell.hpp` now exposes the same helpers.

This means the shell presets provide more opinionated and easier-to-use defaults for the most common generated-surface styles.

### 4. Example update
`examples/workbench-demo/main.cpp` now uses:
- compact quick-actions toolbar preset in the navigation panel
- labeled command-strip toolbar preset in the inspector panel
- descriptive tool-surface preset in the inspector panel

That makes the demo a stronger showcase of multiple generated surface styles from the same shared action model.

### 5. Documentation
Updated:
- `docs/CPP_APP_FRAMEWORK_LAYER.md`
- `docs/CPP_TOOLBAR_BUILDERS_2026-04-05.md`
- `docs/CPP_VISUAL_POLICY_LAYER_2026-04-05.md`

## Validation
- literal rename audit: clean
- `git diff --check`: clean

## Blockers
Real build validation remains blocked by missing environment tooling:
- `meson`
- Python `mesonbuild`
- `g++`

## Recommended continuation
1. Integrate dock/workspace actions more deeply into the shell path.
2. Continue public-header branding cleanup where safe.
3. Consider a document-app or dashboard preset next.
4. Run full build validation as soon as the environment supports it.
