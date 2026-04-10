# C++ Example Presets 2026-04-05

## Summary
This pass adds more example coverage for the growing C++ shell family.

## Added examples
- `examples/workbench-demo/main.cpp`
- `examples/document-demo/main.cpp`
- `examples/dashboard-demo/main.cpp`

## Why this matters
As the shell family becomes more specialized, examples become more important.

Different applications want different starting points:
- studio/tool apps
- document editors
- dashboards and monitoring consoles

Giving each of those a concrete example makes the framework easier to understand and easier to adopt.

## What each example demonstrates
### Workbench / studio-style example
- navigation/document/inspector composition
- workspace and panel action helpers
- compact and descriptive action surfaces

### Document example
- outline/content/details composition
- document-oriented toolbar and tool surfaces
- panel toggle and workspace command helpers
- document command helpers feeding document-specific surfaces
- document action counts exposed through the shell

### Dashboard example
- navigation/dashboard/context composition
- dashboard-specific action helpers
- panel toggle and workspace command helpers
- dashboard and panel surface generation
- dashboard-specific tools built from dashboard action sections
- dashboard action counts exposed through the shell

## Strategic value
Examples are part of API design.

When examples become clearer, shorter, and more aligned with the shell presets, the framework itself becomes easier to reason about.

## Next recommended step
1. build-wire the C++ examples once toolchain support exists
2. continue refining shell-specific surface semantics
3. consider a shared examples README for the shell family
4. keep modernizing visible inherited public header surfaces
