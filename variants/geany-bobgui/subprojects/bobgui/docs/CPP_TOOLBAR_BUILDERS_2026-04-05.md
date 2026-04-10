# C++ Toolbar Builders 2026-04-05

## Summary
This pass adds a toolbar-oriented C++ builder layer:
- `bobgui::cpp::ToolbarBuilder`

## What it does
`ToolbarBuilder` takes:
- an `ActionRegistry`
- a `ToolSurfaceModel`

and builds a toolbar-like widget surface using grouped action data.

## Current options
`ToolbarBuilder::Options` currently supports:
- `show_section_labels`
- `show_button_labels`
- `show_shortcuts`
- `show_checked_prefix`
- `show_tooltips`
- `frame_sections`
- `show_section_separators`
- `prefer_toggle_controls`
- `section_spacing`
- `item_spacing`

This provides a simple policy layer for deciding how compact or descriptive the generated toolbar should be.

## Output shape
The current builder produces:
- a horizontal root container
- per-section grouped boxes
- optional section labels
- action buttons driven by the shared action model

Buttons can:
- show icons
- show text labels when enabled
- show checked state for active toggle actions
- expose tooltip detail
- activate actions through the shared registry

## Shell integration
`AppShell` now adds:
- `build_toolbar_widget()`
- `build_labeled_toolbar_widget()`
- `build_compact_toolbar_widget()`

`StudioShell` now adds:
- `build_toolbar_widget()`
- `build_labeled_toolbar_widget()`
- `build_compact_toolbar_widget()`

This means the higher-level shell presets can now generate both:
- grouped tool surfaces
- toolbar-like action surfaces

from the same shared action state.

## Example effect
The C++ demo now adds a compact icon-oriented "Quick Actions" toolbar-like widget into the navigation panel while also showing a labeled toolbar-style command strip in the inspector panel before the fuller grouped tool surface. The compact surface comes from a dedicated preset path and demonstrates framed section grouping, separators, and tooltip-oriented compact interaction.

That gives the example a stronger story around multiple action-derived surfaces coming from the same shared model.

## Strategic value
This is another important step toward a more complete application-framework façade:
- shared action registry
- grouped sections
- tool-surface model
- tool-surface widget builder
- toolbar-specific builder and policy layer

## Next recommended step
1. keep deepening the visual policy layer for generated toolbar and tool surfaces
2. integrate dock/workspace-oriented actions more deeply into the same shell path
3. continue modernizing high-visibility public header comments
4. validate the whole stack with a real toolchain when available
