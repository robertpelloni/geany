# Bobgui Handoff — 2026-04-05 — C++ Application Lifecycle

## Overview
This pass introduces JUCE and Ultimate++ inspired application lifecycle hooks to the C++ wrapper, fundamentally shifting how applications manage state and execution phases.

## Implemented

### 1. Application Lifecycle Hooks
`bobgui/cpp/application.hpp` now provides distinct execution phases:
- `on_startup`
- `on_activate`
- `on_shutdown`

This bridges the gap between C-style toolkit signal wrapping and modern C++ framework ergonomics, offering developers explicit, reliable points for initialization and teardown.

### 2. Upgraded C++ Examples
The existing examples (`workbench-demo`, `document-demo`, `dashboard-demo`) were updated to leverage the new lifecycle pattern.
- Early console logging happens in `on_startup`.
- The shell UI is composed and presented in `on_activate`.
- The shell object (`unique_ptr`) is cleanly released in `on_shutdown`.

### 3. Documentation
Updated:
- `docs/CPP_APP_FRAMEWORK_LAYER.md`

Added:
- `docs/CPP_APP_LIFECYCLE_2026-04-05.md`

These documents explain:
- the rationale behind explicit initialization and teardown hooks.
- the migration towards higher-level state management, mirroring tools like Qt and JUCE.

## Validation
- literal rename audit: clean
- `git diff --check`: clean

## Blockers
Real build validation remains blocked by missing environment tooling:
- `meson`
- Python `mesonbuild`
- `g++`

## Recommended continuation
1. Consider bringing file-open and command-line handling into the `Application` wrapper.
2. Move toward implementing Ultimate++ and JUCE feature subsets within the framework.
3. Continue modernizing and refining the C++ wrapper APIs.
4. Run full build validation as soon as the environment supports it.
