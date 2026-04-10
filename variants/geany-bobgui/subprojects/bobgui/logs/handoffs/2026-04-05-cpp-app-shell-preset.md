# Bobgui Handoff — 2026-04-05 — C++ App Shell Preset

## Overview
This pass focused on moving the C++ layer from simple wrappers toward a more usable application-framework surface.

## Implemented

### 1. ActionRegistry wrapper expansion
`bobgui/cpp/action_registry.hpp` now supports:
- action metadata via `ActionOptions`
- adding regular actions
- adding toggle actions
- checked-state updates
- checked-state reads
- action activation
- menu-model creation

This is important because a usable C++ app framework needs more than object ownership. It needs a practical command/action surface.

### 2. Shared metadata model
`Workbench::CommandOptions` now reuses the action-registry metadata model instead of duplicating it.

That keeps the C++ API cleaner and reduces divergence between:
- shell commands
- shared actions
- menu-oriented metadata

### 3. AppShell preset
Added:
- `bobgui/cpp/app_shell.hpp`

`AppShell` is a convenience preset that owns and wires together:
- `Workbench`
- `ActionRegistry`
- `CommandPalette`

This gives C++ users a better default path for building a bobgui desktop shell.

### 4. Example update
`examples/workbench-demo/main.cpp` now uses:
- `AppShell`
- `Workbench::CommandOptions`

This makes the example more representative of the intended C++ story.

### 5. Documentation
Updated:
- `docs/CPP_APP_FRAMEWORK_LAYER.md`

Added:
- `docs/CPP_APP_SHELL_PRESET_2026-04-05.md`

## Validation
- literal rename audit: clean
- `git diff --check`: clean

## Blockers
Real build validation is still blocked by missing toolchain components in the environment:
- `meson`
- Python `mesonbuild`
- `g++`

## Recommended continuation
1. Add action visiting/iteration helpers in the C++ wrapper.
2. Add dock/workspace-oriented shell presets.
3. Continue public-header branding cleanup in the highest-visibility surfaces.
4. Run full build validation as soon as the environment supports it.
