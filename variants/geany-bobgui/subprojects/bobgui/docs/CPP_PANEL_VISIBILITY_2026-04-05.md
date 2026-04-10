# C++ Panel Visibility Toggles 2026-04-05

## Summary
This pass wires up the actual UI visibility behavior for the panel toggles that were previously only updating status messages.

## AppShell additions
`bobgui::cpp::AppShell` now provides:
- `set_left_sidebar_visible()`
- `set_right_sidebar_visible()`
- `set_toolbar_visible()`
- `set_menubar_visible()`

These are powered by new C-level getters on `BobguiWorkbench`:
- `bobgui_workbench_get_left_sidebar()`
- `bobgui_workbench_get_right_sidebar()`
- `bobgui_workbench_get_central()`

## Preset integration
The shell presets now forward these visibility helpers with domain-specific naming:

### StudioShell
- `set_navigation_panel_visible()`
- `set_inspector_panel_visible()`

### DocumentShell
- `set_outline_panel_visible()`
- `set_details_panel_visible()`

### DashboardShell
- `set_navigation_panel_visible()`
- `set_context_panel_visible()`

## Example effect
The `examples/workbench-demo/main.cpp`, `examples/document-demo/main.cpp`, and `examples/dashboard-demo/main.cpp` examples were updated.

The panel toggle commands now:
1. read their current checked state from the `ActionRegistry`
2. toggle the checked state
3. use the new shell-level visibility helpers to actually show/hide the corresponding sidebar widget
4. update the status bar with the new state

## Why this matters
The C++ layer is now capable of real UI state management, not just static UI generation.
Connecting the shared action model's checked state directly to the workbench's structural visibility proves that the framework abstractions work together smoothly.

## Strategic value
This completes a functional vertical slice for the C++ application framework:
- Action definition
- UI generation from actions
- State inspection
- Real-time structural UI updates based on action state

## Next recommended step
1. deepen dock/workspace-oriented shell helpers on top of app/studio/document/dashboard shell presets
2. add build-wired C++ examples once the current shell APIs settle a little more
3. continue modernizing the most visible inherited branding/comments in public entry points
