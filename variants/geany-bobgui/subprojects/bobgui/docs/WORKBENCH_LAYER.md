# Bobgui Workbench Layer

## Goal
A practical way to make bobgui feel more like a modern application framework—closer to Qt's `QMainWindow` ergonomics—without becoming Qt.

## Why this was added
One of Qt's strongest advantages is not just widgets, but the ease of building a professional application shell:
- top-level main window
- central content area
- left/right side panels
- header / toolbar region
- bottom status area
- straightforward app-shell composition

Lower-level toolkit-style APIs are powerful but often feel assembly-oriented for app-shell work. The new `BobguiWorkbench` layer is intended to reduce that friction.

## What `BobguiWorkbench` provides
`BobguiWorkbench` offers a higher-level application shell abstraction with:
- application window ownership
- header bar management
- central widget placement
- left sidebar support
- right sidebar support
- simple status line updates
- easy header action buttons

## Key API
- `bobgui_workbench_new()`
- `bobgui_workbench_set_title()`
- `bobgui_workbench_set_central()`
- `bobgui_workbench_set_left_sidebar()`
- `bobgui_workbench_set_right_sidebar()`
- `bobgui_workbench_set_status()`
- `bobgui_workbench_add_header_action()`
- `bobgui_workbench_add_header_action_for_command()`
- `bobgui_workbench_set_command_palette()`
- `bobgui_workbench_set_action_registry()`
- `bobgui_workbench_enable_menubar()`
- `bobgui_workbench_enable_toolbar()`
- `bobgui_workbench_add_command_sectioned_visual()`
- `bobgui_workbench_add_command_visual()`
- `bobgui_workbench_add_command()`
- `bobgui_workbench_add_toggle_command_sectioned_visual()`
- `bobgui_workbench_add_toggle_command_visual()`
- `bobgui_workbench_add_toggle_command()`
- `bobgui_workbench_present()`

## Current usability improvements
This pass adds a more Qt-like interaction loop for command-driven apps:
- a workbench can own a command palette directly
- a workbench can own an action registry
- commands can be registered through the workbench API
- commands can now carry explicit section metadata for cleaner menu/toolbar/palette grouping
- the palette can be opened with `Ctrl+Shift+P`
- header buttons can be bound directly to registered commands
- menubar generation can come from the shared action model
- toolbar buttons can be generated from the same action model
- toggle-style commands can be registered through the same shell layer

That starts to move bobgui toward a more opinionated application framework experience instead of only a raw widget toolkit.

## Why this is "more like Qt but not Qt"
This is intentionally not a copy of `QMainWindow`.
Instead, it follows a bobgui-native direction:
- still built from ordinary bobgui widgets
- still composable using paned/box/headerbar primitives
- no separate object model or meta compiler
- integrates naturally with grouped module headers and future DSL work

## Strategic value
`BobguiWorkbench` is a good example of the next phase bobgui needs:
- fewer abstract module stubs
- more developer-facing ergonomics
- more cohesive vertical slices that directly improve usability

## Recommended next step
The most valuable follow-up would be:
1. improve toolbar presentation beyond plain labels into clearer grouped surfaces with stronger toggle/icon affordances
2. add workbench-managed dock registration
3. integrate status updates with shell/progress APIs
4. provide both C and thin C++ demo apps using the workbench shell
