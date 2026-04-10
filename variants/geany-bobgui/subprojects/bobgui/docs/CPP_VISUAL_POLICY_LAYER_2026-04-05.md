# C++ Visual Policy Layer 2026-04-05

## Summary
This pass deepens the generated-surface policy model for the C++ layer.

## ToolSurfaceBuilder policy
`bobgui::cpp::ToolSurfaceBuilder::Options` now supports:
- `show_section_labels`
- `show_subtitles`
- `show_shortcuts`
- `show_checked_prefix`
- `show_tooltips`
- `frame_sections`
- `section_spacing`
- `item_spacing`

This lets callers choose whether a generated tool surface should be:
- more descriptive
- more compact
- more metadata-rich
- more toggle-emphasized

## ToolbarBuilder policy
`bobgui::cpp::ToolbarBuilder::Options` now supports:
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

This lets toolbar-like action surfaces become more compact or more explicit depending on the usage context.

## Why this matters
A good framework should not only generate surfaces.
It should also let developers control the presentation policy of those generated surfaces.

That is especially important for:
- tool-oriented applications
- compact navigation areas
- inspector-driven workflows
- future studio and dashboard presets

## Demo effect
The current C++ demo now shows multiple policy styles from the same action model:
- a compact quick-actions surface in the navigation area with framed groups, separators, and tooltip-driven detail
- a labeled command-strip toolbar in the inspector area using stronger toolbar semantics
- a fuller descriptive tool surface in the inspector area with framed groups, subtitles, shortcuts, and toggle emphasis

That is a useful proof that the shared action model can drive multiple distinct presentation styles.

## Strategic value
This is another step toward making bobgui feel like a more mature application framework rather than just a renamed toolkit with wrappers.

## Next recommended step
1. add stronger toolbar-specific semantics beyond generic boxes and labels
2. integrate dock/workspace actions more deeply into the same model
3. continue modernizing visible inherited public header surfaces
4. run real build validation once the environment has toolchain support
