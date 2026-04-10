# C++ Tool Surface Builders 2026-04-05

## Summary
This pass moves the C++ layer one step closer to actual UI generation by adding:
- `bobgui::cpp::ToolSurfaceBuilder`

## What it does
`ToolSurfaceBuilder` takes:
- an `ActionRegistry`
- a `ToolSurfaceModel`

and produces an actual bobgui widget tree representing grouped tool sections.

## Output shape
The current builder produces a simple grouped structure:
- vertical root container
- per-section vertical group
- section label
- horizontal row of action buttons

Each button can:
- show an icon when present
- show a checked prefix for active toggle actions
- activate the shared action when clicked

## Shell integration
`AppShell` now adds:
- `build_tool_surface_widget()`

`StudioShell` now adds:
- `build_tool_surface_widget()`

This means the higher-level shell presets can now produce a real tool-surface widget directly from their shared action state.

## Example effect
The C++ demo now appends a built tool surface into the inspector panel.

That is important because it demonstrates a full path:
- register actions
- inspect grouped action structure
- derive a tool-surface model
- build a real widget surface from that model

## Strategic value
This is a meaningful threshold.
The C++ layer is no longer only wrapping objects and returning metadata. It is now beginning to generate actual framework-managed UI surfaces from the shared action model.

## Next recommended step
1. improve visual richness of generated tool surfaces
2. add actual toolbar-specific builders and policies
3. deepen dock/workspace shell behavior once the C dock layer matures
4. validate the whole stack with a real toolchain when available
