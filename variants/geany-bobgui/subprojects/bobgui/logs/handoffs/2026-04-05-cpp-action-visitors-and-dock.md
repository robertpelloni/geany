# Bobgui Handoff — 2026-04-05 — C++ Action Visitors and Dock Support

## Overview
This pass kept pushing the C++ layer toward a more complete application-framework façade.

## Implemented

### 1. Action inspection in C++
`bobgui/cpp/action_registry.hpp` now exposes:
- `ActionInfo`
- `ActionVisitor`
- `visit()`

This lets C++ callers inspect the shared action model directly instead of dropping down into the C callback API.

### 2. Shared metadata cleanup
`Workbench::CommandOptions` continues to reuse the action-registry options model.
This keeps command metadata and action metadata aligned.

### 3. Dock wrapper and shell extension
Added:
- `bobgui/cpp/dock_manager.hpp`

Extended:
- `bobgui/cpp/app_shell.hpp`

`AppShell` now supports lazy dock-manager creation, giving the shell preset an early path toward dock/workspace-oriented application shells.

### 4. Build/install metadata
- Updated the C++ umbrella header
- Updated Meson header installation for the new dock wrapper

### 5. Documentation
Updated:
- `docs/CPP_APP_FRAMEWORK_LAYER.md`

Added:
- `docs/CPP_ACTION_VISITORS_AND_DOCK_PRESET_2026-04-05.md`

## Validation
- literal rename audit: clean
- `git diff --check`: clean

## Blockers
Real build validation is still blocked by missing toolchain pieces in the environment:
- `meson`
- Python `mesonbuild`
- `g++`

## Recommended continuation
1. Add action-driven menu/tool-surface helpers in the C++ layer.
2. Add a more opinionated studio or multi-pane shell preset.
3. Continue high-visibility branding/header cleanup where safe.
4. Run full build validation as soon as the environment supports it.
