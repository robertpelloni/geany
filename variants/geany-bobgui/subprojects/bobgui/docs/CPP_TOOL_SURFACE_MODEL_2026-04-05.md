# C++ Tool Surface Model 2026-04-05

## Summary
This pass adds a small C++ model layer for action-driven tool surfaces:
- `bobgui::cpp::ToolSurfaceModel`

## What it provides
`ToolSurfaceModel` turns grouped action sections into a more tool-oriented structure:
- `ToolSurfaceModel::ToolSection`
- `ToolSurfaceModel::ToolItem`

It also provides convenience queries such as:
- `section_count()`
- `item_count()`
- `empty()`

## Why this matters
At this stage, the C++ layer already had:
- action registration
- action inspection
- grouped action sections
- shell presets

But turning those grouped sections into a directly reusable tool-surface model makes the API more explicit and easier to consume from higher-level code.

## Shell integration
`AppShell` now exposes:
- `tool_surface_model()`

`StudioShell` now exposes:
- `tool_surface_model()`

This means tool-oriented C++ code can ask the shell for a structured action-derived model directly.

## Example effect
The C++ demo now derives a `ToolSurfaceModel` from the shell and uses it to report:
- section count
- total tool item count

That is a small example, but it shows a cleaner read path for future toolbar, palette, dashboard, and studio surfaces.

## Strategic value
This continues the same architecture pattern:
- stable C core
- action-centric shell model
- ergonomic C++ wrappers
- increasingly explicit application-framework data models

## Next recommended step
1. add actual toolbar/tool-surface builder helpers on top of `ToolSurfaceModel`
2. deepen dock/workspace shell behavior as the C dock layer matures
3. continue turning the C++ demo into a canonical studio-style example
4. validate the whole stack with a real toolchain when available
