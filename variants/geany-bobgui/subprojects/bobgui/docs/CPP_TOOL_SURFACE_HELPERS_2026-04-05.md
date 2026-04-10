# C++ Tool Surface Helpers 2026-04-05

## Summary
This pass extends the C++ shell layer with helpers that are useful for action-driven tool surfaces.

## What changed
### Action registry
`bobgui/cpp/action_registry.hpp` now adds:
- `list_sections()`

This builds grouped action sections from the shared action model using the same basic fallback logic:
- explicit section
- category
- `General`

### App shell
`bobgui/cpp/app_shell.hpp` now adds:
- `list_action_sections()`

### Studio shell
`bobgui/cpp/studio_shell.hpp` now adds:
- `list_tool_sections()`

## Why this matters
A usable application framework should make it easy not only to register actions, but also to derive tool surfaces from them.

Grouped action sections are useful for:
- toolbars
- command dashboards
- workspace side panels
- tool inspectors
- custom action launchers

## Example effect
The C++ demo now reads grouped action-section information back from the shell and uses it to produce an initial status message.

That is a small example, but it proves the shell can now do more than just push actions into the framework. It can also inspect and organize them.

## Strategic value
This is another step toward a more complete C++ application-framework façade:
- shared action model
- shell presets
- menu access
- grouped action inspection
- tool-oriented composition

## Next recommended step
1. add richer toolbar/tool-surface builders on top of grouped action sections
2. deepen dock/workspace helpers once the C dock layer matures
3. continue turning the C++ demo into a canonical tool-style bobgui example
4. validate the whole stack with a real toolchain when available
