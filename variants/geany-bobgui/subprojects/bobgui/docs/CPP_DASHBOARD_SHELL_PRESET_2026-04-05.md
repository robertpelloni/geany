# C++ Dashboard Shell Preset 2026-04-05

## Summary
This pass adds another higher-level shell preset:
- `bobgui::cpp::DashboardShell`

The goal is to give bobgui a clearer path for dashboard-style and monitoring-style desktop applications.

## What it provides
`DashboardShell` builds on top of `AppShell` and provides a dashboard-oriented vocabulary:
- `set_navigation_panel()`
- `set_dashboard_view()`
- `set_context_panel()`
- `build_dashboard_toolbar_widget()`
- `build_dashboard_panel_toolbar_widget()`
- `build_dashboard_tools_widget()`
- `build_dashboard_panel_tools_widget()`
- `dashboard_action_count()`

It also exposes shell-level helpers for:
- dashboard commands
- workspace commands
- panel toggles

## Why this matters
Not every application is primarily a studio tool or a document editor.
A dashboard-oriented preset gives the framework a clearer path for:
- analytics tools
- monitoring consoles
- operations dashboards
- admin panels
- overview/detail applications

## Strategic value
With `StudioShell`, `DocumentShell`, and `DashboardShell`, the C++ layer now has a more believable family of application-shell presets instead of one generic abstraction doing everything.

That is important for usability because developers can choose an entry point that already matches their intent.

## Relationship to the shared action model
`DashboardShell` still uses the same shared action/section/surface pipeline:
- register actions
- group them into sections
- derive workspace/panel-specific surface models
- build toolbars and tool surfaces

So the architecture remains unified even as the shell vocabulary becomes more specific.

## Example effect
The dashboard example now demonstrates:
- dashboard commands
- workspace commands
- panel toggles
- dashboard toolbar surfaces
- dashboard tool surfaces
- panel toolbar/tool surfaces
- dashboard action counts exposed through the shell

That gives the preset a much more concrete usage story.

## Next recommended step
1. continue deepening dock/workspace semantics across all shell presets
2. consider build-wiring multiple C++ examples once toolchain support exists
3. keep refining generated surface semantics so the presets feel more intentional
4. continue modernizing visible inherited public header surfaces
