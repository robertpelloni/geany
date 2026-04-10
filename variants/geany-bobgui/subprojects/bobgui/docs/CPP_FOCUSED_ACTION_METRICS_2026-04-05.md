# C++ Focused Action Metrics 2026-04-05

## Summary
This pass extends the shell-level workspace and panel helpers with lightweight metrics and readiness-oriented queries.

## AppShell additions
`bobgui::cpp::AppShell` now provides:
- `ensure_dock_manager()`
- `workspace_action_count()`
- `panel_action_count()`
- `has_workspace_actions()`
- `has_panel_actions()`

These are built on top of the same shared filtered action-surface model already used for workspace and panel surface generation.

## Why this matters
A higher-level shell should be able to do more than build surfaces.
It should also be able to answer basic questions like:
- do I have workspace actions yet?
- do I have panel actions yet?
- how many focused actions exist in each family?

That makes the shell more useful for:
- status text
- onboarding hints
- empty-state handling
- diagnostics
- future shell dashboards

## Preset integration
The more opinionated presets now expose shell-specific focused counts too:
- `DocumentShell::document_action_count()`
- `DashboardShell::dashboard_action_count()`

## Example effect
The current document and dashboard examples now report focused action counts through their shell APIs.

That demonstrates that the shell family can both:
- build focused surfaces
- summarize focused action state

## Strategic value
This is another step toward making the C++ shell family feel more like an actual application framework rather than only a rendering convenience layer.

## Next recommended step
1. keep deepening dock/workspace behavior across shell presets
2. consider build-wiring multiple C++ examples once toolchain support exists
3. continue refining generated-surface semantics and shell-level summaries
4. continue modernizing visible inherited public header surfaces
