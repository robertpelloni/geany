# C++ Workspace Surface Presets 2026-04-05

## Summary
This pass deepens shell-level dock/workspace semantics by giving `AppShell` and its higher-level presets more purposeful action-surface helpers.

## New shell-level helpers
`bobgui::cpp::AppShell` now provides:
- `ensure_dock_manager()`
- `workspace_tool_surface_model()`
- `panel_tool_surface_model()`
- `workspace_action_count()`
- `panel_action_count()`
- `has_workspace_actions()`
- `has_panel_actions()`
- `build_workspace_toolbar_widget()`
- `build_panel_toolbar_widget()`
- `build_workspace_toolbar_preset()`
- `build_panel_toolbar_preset()`
- `build_workspace_tool_surface_widget()`
- `build_panel_tool_surface_widget()`
- `build_workspace_tool_surface_preset()`
- `build_panel_tool_surface_preset()`

These helpers derive focused surfaces from the same shared action model instead of forcing callers to manually filter grouped sections.

## Why this matters
This is important because shell-level actions are not all the same.

There is a useful distinction between:
- workspace-oriented commands
- panel/view toggles
- broader general actions

By letting the shell expose those focused surfaces directly, the C++ API becomes more intentional and easier to use.

## Preset integration
Higher-level presets can now reuse those focused surfaces:
- `StudioShell` can expose workspace and panel toolbars/tools
- `DocumentShell` can use workspace and panel toolbar/tool presets for outline/detail flows
- `DashboardShell` can use workspace and panel toolbar/tool presets for navigation/context flows

## Example effect
The C++ demo now renders:
- a workspace-oriented toolbar preset in the navigation panel
- a panel-oriented toolbar preset in the inspector panel
- a panel-oriented tool surface beneath that
- a fuller descriptive tool surface beneath that

The examples also now report focused action counts through the shell-level helpers, which shows that the shell can reason about workspace and panel action families instead of only rendering them.

That gives the example a stronger shell-specific semantics story instead of only showing generic action-surface generation.

## Strategic value
This is another step toward making bobgui feel like a real application framework:
- shared action model
- shell presets
- focused action subsets
- purposeful surface presets

## Next recommended step
1. keep deepening dock/workspace semantics across all shell presets
2. add multiple build-wired C++ examples once toolchain support exists
3. continue refining generated-surface semantics so they feel more intentional and less generic
4. continue modernizing visible inherited public header surfaces
