# Bobgui C++ App Framework Layer

## Summary
This pass starts a thin C++ convenience layer on top of the existing bobgui C API instead of attempting a risky full rewrite.

New headers:
- `bobgui/cpp/bobgui.hpp`
- `bobgui/cpp/object_handle.hpp`
- `bobgui/cpp/application.hpp`
- `bobgui/cpp/action_registry.hpp`
- `bobgui/cpp/command_palette.hpp`
- `bobgui/cpp/workbench.hpp`
- `bobgui/cpp/app_shell.hpp`
- `bobgui/cpp/dock_manager.hpp`
- `bobgui/cpp/studio_shell.hpp`
- `bobgui/cpp/document_shell.hpp`
- `bobgui/cpp/dashboard_shell.hpp`
- `bobgui/cpp/tool_surface.hpp`
- `bobgui/cpp/tool_surface_builder.hpp`
- `bobgui/cpp/toolbar_builder.hpp`

New examples:
- `examples/workbench-demo/main.cpp`
- `examples/document-demo/main.cpp`
- `examples/dashboard-demo/main.cpp`

## Why this direction
The project goal is to make bobgui feel easier to use, more coherent, and more application-framework-oriented.

For that, a thin C++ layer is much safer and more realistic than trying to rewrite a very large C codebase all at once.

This follows the same strategic pattern used successfully by frameworks that keep a stable C core while offering higher-level bindings or wrappers.

## Design principles
The C++ layer is intentionally:
- thin
- header-only
- non-invasive
- split into small focused headers instead of one growing monolith
- built on top of the existing C API
- aligned with the new workbench/action/command-palette model

That means the C core remains the source of truth while C++ consumers get a more direct, ergonomic entry point.

## What the wrapper currently provides
The first wrapper pass introduces small C++ objects for:
- `Application`
- `ActionRegistry`
- `CommandPalette`
- `Workbench`
- `AppShell`
- `DockManager`
- `StudioShell`
- `DocumentShell`
- `DashboardShell`
- `ToolSurfaceModel`
- `ToolSurfaceBuilder`
- `ToolbarBuilder`

The current wrapper focuses on:
- ownership of GObject-based instances
- Application lifecycle hooks (`on_startup`, `on_activate`, `on_shutdown`) modeled after JUCE/Ultimate++
- straightforward workbench construction
- action-registry attachment
- command-palette attachment
- section-aware command registration
- option-struct-based command registration to avoid long parameter lists
- action visiting, action listing, grouped-section listing, and menu-model access from C++
- a simple `AppShell` preset that wires workbench + action registry + command palette together
- shell-level workspace/panel action helpers layered on the shared action model
- shell-level workspace/panel toolbar presets derived from the same grouped action state
- shell-level workspace/panel tool-surface presets derived from the same grouped action state
- lazy dock-manager access through the shell preset
- a more opinionated `StudioShell` preset for multi-pane tool-style apps
- a `DocumentShell` preset for outline/content/details document-style apps
- a `DashboardShell` preset for navigation/dashboard/context layouts
- tool-surface modeling on top of grouped actions
- actual tool-surface widget building on top of the model
- toolbar-specific widget building with evolving visual policy options including tooltips, separators, framed section grouping, and toggle-control semantics
- tool-surface widget building with evolving visual policy options including subtitles, shortcuts, tooltips, and framed section grouping
- lambda-friendly command handlers via `std::function`

## Why this is a better step than a rewrite
A rewrite would currently risk:
- losing renamed-path progress
- fragmenting the public API again
- introducing a second incomplete implementation
- breaking the momentum around the shared action/app-shell direction

A thin wrapper keeps the architecture converging instead of splitting.

## Relationship to the action-section work
The new C++ layer directly benefits from the explicit action-section model.

That is important because wrapper APIs need stable semantics for:
- menu sections
- toolbar groups
- command-palette groups
- toggle actions

By making those concepts explicit in the C layer first, the C++ API can stay simple instead of encoding fragile heuristics.

## Example direction
The current C++ examples now cover multiple shell styles and leverage the new application lifecycle hooks.

### Studio-oriented example
The main studio-style example shows a shell that:
- binds to application startup, activation, and shutdown lifecycles
- creates an application
- builds a `StudioShell`
- gets a pre-wired workbench + action registry + command palette stack
- initializes dock support through the preset path
- assigns navigation, document, and inspector panels explicitly
- registers commands through a `Workbench::CommandOptions` struct
- inspects grouped action sections through the shell helper layer
- derives a `ToolSurfaceModel` from grouped actions
- builds an actual tool-surface widget from the model and appends it to the inspector panel
- builds a quick-actions toolbar widget from a dedicated compact preset path
- builds workspace/panel-specific toolbar presets derived from grouped action state
- builds workspace/panel-specific tool-surface presets derived from grouped action state
- demonstrates workspace/panel action helpers on top of the shared action model
- demonstrates different presentation policies from the same shared action model
- demonstrates live panel visibility toggles driven by action state
- enables menubar and toolbar generation
- pins palette commands through the shell convenience layer

### Document-oriented example
The document example shows:
- application lifecycle hooks for structured initialization/teardown
- outline/content/details shell composition
- workspace-oriented commands
- panel toggles
- document-specific toolbar/tool surfaces

### Dashboard-oriented example
The dashboard example shows:
- application lifecycle hooks for structured initialization/teardown
- navigation/dashboard/context shell composition
- dashboard commands
- workspace commands
- panel toggles
- dashboard and panel toolbar/tool surfaces

## Rename audit note
A targeted audit of legacy toolkit spellings in the working tree came back clean during this pass.

That does not guarantee every historical trace is gone in every possible substring form, but it does indicate the visible high-level naming surface is already strongly normalized around `bobgui`.

## Validation status
A real compile-validation pass was attempted after this refactor, but the current environment does not provide Meson or a C++ compiler. The blocker is tool availability, not a decision to skip verification.

## Recommended next steps
1. add richer wrapper coverage around action-driven toolbar/tool surfaces
2. deepen dock/workspace-oriented shell helpers on top of app/studio/document/dashboard shell presets
3. add build-wired C++ examples once the current shell APIs settle a little more
4. continue modernizing the most visible inherited branding/comments in public entry points
