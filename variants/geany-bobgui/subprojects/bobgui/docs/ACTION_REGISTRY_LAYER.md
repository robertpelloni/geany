# Bobgui Action Registry Layer

## Summary
This pass adds a shared action model to strengthen bobgui's movement toward a real application framework.

## Why this matters
A polished desktop framework usually has a central action concept that can be reused by:
- menus
- toolbars
- command palettes
- workbench shells
- future dock/workspace actions

Without that, every feature ends up inventing its own callback list.

This also matters for a more C++-friendly layer because wrappers need one stable command model to project into higher-level application objects.

## What was added
### New module
- `bobgui/modules/visual/actionregistry/bobguiactionregistry.h`
- `bobgui/modules/visual/actionregistry/bobguiactionregistry.c`

### Core API
- `bobgui_action_registry_new()`
- `bobgui_action_registry_add()`
- `bobgui_action_registry_add_detailed()`
- `bobgui_action_registry_add_sectioned()`
- `bobgui_action_registry_add_toggle()`
- `bobgui_action_registry_set_checked()`
- `bobgui_action_registry_get_checked()`
- `bobgui_action_registry_activate()`
- `bobgui_action_registry_create_menu_model()`
- `bobgui_action_registry_visit()`
- `bobgui_action_registry_populate_palette()`

## Workbench integration
`BobguiWorkbench` was extended with:
- `bobgui_workbench_set_action_registry()`
- improved `bobgui_workbench_add_command()` behavior
- `bobgui_workbench_add_toggle_command()`
- `bobgui_workbench_add_header_action_for_command()`
- `bobgui_workbench_enable_menubar()`
- `bobgui_workbench_enable_toolbar()`

If a workbench has an action registry and a command palette, adding commands through the workbench now updates the shared action model and repopulates the palette from that source. Header buttons can also be bound directly to action identifiers. In this pass, workbench command registration also installs application actions and can export the action registry into a real application menubar.

## Strategic value
This is an important architecture step because it turns workbench + command palette from a set of connected widgets into a more reusable application-command system.

## Recommended next steps
1. dock/workspace actions should register through the same model
2. add recent/pinned actions on top of the same registry
3. expand menu structure from sections into richer submenus where appropriate
4. support icons and richer checked/toggle presentation across all action surfaces
5. keep the action registry as the semantic core for the thin C++ wrapper layer
