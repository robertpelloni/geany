# C++ Studio Shell Preset 2026-04-05

## Summary
This pass adds a more opinionated C++ shell preset:
- `bobgui::cpp::StudioShell`

`StudioShell` builds on top of `AppShell` and gives tool-style or IDE-style applications a clearer starting point.

## What it provides
`StudioShell` keeps the same underlying pieces:
- workbench
- action registry
- command palette
- dock manager

But it exposes them through more purposeful shell language:
- `set_navigation_panel()`
- `set_document_view()`
- `set_inspector_panel()`

## Why this matters
A framework becomes easier to use when it gives developers names that match their intent.

For many desktop applications, especially:
- editors
- studio tools
- dashboards
- IDE-style shells

it is much clearer to think in terms of:
- navigation
- document
- inspector

than in terms of raw left/center/right pane plumbing.

## Relationship to the underlying architecture
`StudioShell` does not replace `Workbench` or `AppShell`.
It sits above them as a more opinionated convenience layer.

That keeps the stack layered cleanly:
- C core primitives
- workbench shell
- app-shell preset
- studio-shell preset

## Example effect
The C++ demo now reads much closer to the intended high-level story:
- create application
- create studio shell
- set navigation/document/inspector panels
- register actions and commands
- enable menubar and toolbar
- present

## Strategic value
This is another step toward making bobgui feel like a framework with real application-shaping affordances instead of just a toolkit with wrappers.

## Next recommended step
1. add tool-surface helpers driven from the action model
2. deepen dock/workspace behavior once the C dock layer matures
3. add more presets for document apps or dashboards if the shell story keeps stabilizing
4. validate the whole stack with a real toolchain when available
