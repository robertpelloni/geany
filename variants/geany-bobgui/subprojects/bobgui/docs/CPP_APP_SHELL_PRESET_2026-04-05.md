# C++ App Shell Preset 2026-04-05

## Summary
This pass adds a small higher-level C++ preset:
- `bobgui::cpp::AppShell`

The goal is to make the most common bobgui desktop-app setup less repetitive.

## What `AppShell` does
`AppShell` owns and wires together:
- `Workbench`
- `ActionRegistry`
- `CommandPalette`

That means C++ callers no longer have to manually create and connect those three objects for every basic application shell.

## Why this matters
The lower-level pieces are still valuable and should remain available.
But a clean framework also needs a convenient default path.

This is part of making bobgui feel more like a complete application framework:
- less assembly
- fewer repeated setup steps
- more obvious happy path

## Additional C++ cleanup in the same pass
The C++ action layer was also strengthened with:
- `ActionRegistry::ActionOptions`
- wrapper methods for adding actions and toggle actions
- wrapper methods for checked-state updates
- wrapper method for creating a menu model

`Workbench::CommandOptions` is now aligned with the action-options model instead of maintaining a completely separate metadata shape.

## Example effect
The C++ demo can now say:
- create `Application`
- create `AppShell`
- set title/content/sidebar
- pin commands
- add commands with `CommandOptions`
- show menubar/toolbar
- present

That is a much better developer story than manually plumbing all supporting objects at every call site.

## Strategic value
This is an important step toward the real goal:
not just wrapping the C API in C++, but shaping a clearer and more approachable application-framework experience.

## Next recommended step
The strongest continuation would be:
1. add action visiting/iteration helpers to the C++ registry
2. add dock/workspace-oriented shell helpers
3. add more opinionated shell presets for common app layouts
4. validate the entire layer with a real toolchain as soon as the environment supports it
