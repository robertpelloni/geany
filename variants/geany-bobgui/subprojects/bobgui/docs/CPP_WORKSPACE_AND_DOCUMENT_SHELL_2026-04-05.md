# C++ Workspace Actions and Document Shell 2026-04-05

## Summary
This pass pushes the C++ shell layer in two related directions:
- shell-level workspace and panel action helpers
- a document-oriented shell preset

## Workspace and panel action helpers
`bobgui::cpp::AppShell` now provides:
- `add_workspace_command()`
- `add_panel_toggle_command()`

`bobgui::cpp::StudioShell` forwards those same helpers.

## Why this matters
These helpers remove repeated metadata wiring for some of the most common application-shell commands.

They provide stronger defaults for:
- workspace-oriented commands
- panel/dock/view toggles

This helps the C++ API feel more like an application framework and less like a lower-level command-registration layer.

## DocumentShell preset
Added:
- `bobgui::cpp::DocumentShell`

`DocumentShell` builds on top of `AppShell` and provides a document-oriented vocabulary:
- `set_outline_panel()`
- `set_content_view()`
- `set_details_panel()`
- `build_document_toolbar_widget()`
- `build_document_panel_toolbar_widget()`
- `build_document_tools_widget()`
- `build_document_panel_tools_widget()`
- `document_action_count()`

## Why this matters
Not every application is a studio tool.
A document-style preset gives the framework a cleaner path for:
- editors
- note/document apps
- structured-content tools
- detail/outline workflows

## Example effect
The document example now demonstrates:
- panel toggles
- workspace actions
- document commands
- workspace toolbar surfaces
- panel toolbar surfaces
- workspace tool surfaces
- panel tool surfaces
- document action counts exposed through the shell

That gives the preset a much clearer and more concrete usage story.

## Strategic value
This continues the same architectural pattern:
- shared action model
- shell presets
- stronger defaults
- more purposeful domain vocabulary

## Next recommended step
1. continue integrating dock/workspace behavior more deeply into the shell presets
2. consider a dashboard-oriented preset after document/studio shells
3. improve actual compile/build validation when toolchain support is available
4. keep modernizing visible inherited public header surfaces
