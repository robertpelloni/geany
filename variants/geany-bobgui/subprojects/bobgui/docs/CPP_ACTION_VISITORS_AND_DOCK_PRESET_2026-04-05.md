# C++ Action Visitors and Dock Preset 2026-04-05

## Summary
This pass continues the thin C++ framework layer in two important directions:
- action inspection from C++
- early dock-oriented shell convenience

## Action inspection improvements
`bobgui/cpp/action_registry.hpp` now exposes:
- `ActionRegistry::ActionInfo`
- `ActionRegistry::ActionVisitor`
- `ActionRegistry::visit()`

This makes it possible for C++ callers to inspect the shared action model without dropping back into the C callback API.

## Why this matters
A higher-level framework API should not only let callers register actions.
It should also let them reason about the action model in a clean way.

That matters for:
- custom toolbars
- custom menus
- command dashboards
- inspection/debug tooling
- future studio-style wrappers

## Dock-oriented shell convenience
The C++ layer now also includes:
- `bobgui/cpp/dock_manager.hpp`

And `AppShell` now has lazy dock-manager support via:
- `has_dock_manager()`
- `dock_manager()`

## Why this matters
The current C dock manager is still minimal, but exposing it through the shell preset is strategically useful.

It establishes that bobgui's C++ app-shell story is not just:
- window
- actions
- palette

but can grow toward:
- docks
- workspaces
- studio-style shells

## Strategic value
This is another step away from raw wrapper generation and toward a genuinely nicer C++ application-framework surface.

The pattern is becoming clearer:
- stable C core
- ergonomic C++ façade
- stronger defaults for common desktop-app structure

## Next recommended step
1. add C++ helpers for action-driven menu/tool surfaces
2. expand dock/workspace behavior once the C dock layer matures
3. add a more opinionated multi-pane/studio preset on top of `AppShell`
4. validate the full stack with a real build toolchain when available
