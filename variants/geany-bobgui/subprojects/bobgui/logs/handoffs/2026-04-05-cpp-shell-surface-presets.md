# Bobgui Handoff — 2026-04-05 — C++ Shell Surface Presets

## Overview
This pass kept improving the generated-surface story while also simplifying the most common call paths through preset helpers.

## Implemented

### 1. ToolSurfaceBuilder preset factory
`bobgui/cpp/tool_surface_builder.hpp` now adds:
- `ToolSurfaceBuilder::Options::detailed()`

This gives a cleaner entry point for fuller descriptive tool-panel surfaces.

### 2. ToolbarBuilder preset factory
`bobgui/cpp/toolbar_builder.hpp` now adds:
- `ToolbarBuilder::Options::compact()`

This gives a cleaner entry point for compact quick-action toolbar surfaces.

### 3. Shell-level preset helpers
`bobgui/cpp/app_shell.hpp` now exposes:
- `build_descriptive_tool_surface_widget()`
- `build_compact_toolbar_widget()`

`bobgui/cpp/studio_shell.hpp` now exposes:
- `build_descriptive_tool_surface_widget()`
- `build_compact_toolbar_widget()`

This means the shell presets now provide more opinionated and easier-to-use defaults for the two most common generated-surface styles.

### 4. Example update
`examples/workbench-demo/main.cpp` now uses the shell-level surface presets rather than manually building up policy objects at the call site.

That makes the C++ demo cleaner and better aligned with the goal of a more straightforward application-framework API.

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
1. Add stronger toolbar-specific semantics beyond generic grouped boxes and labels.
2. Integrate dock/workspace actions more deeply into the shell path.
3. Continue public-header branding cleanup where safe.
4. Run full build validation as soon as the environment supports it.
