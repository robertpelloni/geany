# Bobgui Handoff — 2026-04-05 — C++ Workspace Actions and Document Shell

## Overview
This pass focused on pushing the shell layer toward stronger domain semantics rather than only exposing generic lower-level wrappers.

## Implemented

### 1. AppShell workspace/panel helpers
`bobgui/cpp/app_shell.hpp` now exposes:
- `add_workspace_command()`
- `add_panel_toggle_command()`

These helpers provide stronger defaults for two common command families:
- workspace-oriented commands
- panel/view toggles

They also reduce repeated metadata wiring at call sites.

### 2. StudioShell forwarding helpers
`bobgui/cpp/studio_shell.hpp` now forwards the same workspace/panel helpers.

That keeps the more opinionated tool-shell preset aligned with the same convenience path.

### 3. DocumentShell preset
Added:
- `bobgui/cpp/document_shell.hpp`

This preset provides a more document-oriented vocabulary:
- outline panel
- content view
- details panel
- document toolbar surface
- document tool surface

### 4. Example update
`examples/workbench-demo/main.cpp` now uses the shell-level workspace and panel helpers, which makes the example read more like an app framework and less like manual metadata assembly.

### 5. Documentation
Updated:
- `docs/CPP_APP_FRAMEWORK_LAYER.md`

Added:
- `docs/CPP_WORKSPACE_AND_DOCUMENT_SHELL_2026-04-05.md`

## Validation
- literal rename audit: clean
- `git diff --check`: clean

## Blockers
Real build validation remains blocked by missing environment tooling:
- `meson`
- Python `mesonbuild`
- `g++`

## Recommended continuation
1. Continue deepening dock/workspace behavior in the shell presets.
2. Consider a dashboard-oriented preset after the document and studio presets.
3. Continue public-header branding cleanup where safe.
4. Run full build validation as soon as the environment supports it.
