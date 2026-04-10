# C++ Application Lifecycle 2026-04-05

## Summary
This pass introduces a more robust, JUCE and Ultimate++ inspired application lifecycle model to `bobgui::cpp::Application`.

## New Lifecycle Hooks
`Application` now provides distinct phases for application execution:
- `on_startup(LifecycleHandler handler)`: Fired exactly once when the application initializes.
- `on_activate(LifecycleHandler handler)`: Fired when the application becomes active (often triggering the main window creation).
- `on_shutdown(LifecycleHandler handler)`: Fired exactly once as the application tears down, allowing graceful resource cleanup.

## Why this matters
Traditional C-style toolkit usage often relies on a single `activate` signal. However, modern application frameworks (like Qt, JUCE, and Ultimate++) cleanly separate:
1. One-time engine initialization (allocating singletons, reading config)
2. UI activation (presenting the shell)
3. Graceful teardown (saving state, releasing memory)

By bringing these explicit lifecycle hooks into the C++ wrapper, we allow developers to organize their code in a much more structured and familiar way. This is a critical step for reaching and surpassing parity with other major C++ application frameworks.

## Example effect
All three C++ examples (`workbench-demo`, `document-demo`, `dashboard-demo`) have been updated. They now use:
- `on_startup` to perform early startup logic (simulated by console output).
- `on_activate` to actually build and present the UI shell.
- `on_shutdown` to safely release the `unique_ptr` holding the shell and perform final cleanup (simulated by console output).

## Strategic value
This represents the transition from simple event wrapping to true application state management. The `Application` wrapper is now responsible for the entire lifecycle, aligning perfectly with the goal of creating an easy-to-use, cleanly organized C++ application framework.

## Next recommended step
1. Map further application-wide signals (like `open` or `command-line`) into the `Application` wrapper.
2. Consider adding integrated configuration loading to the `on_startup` phase.
3. Continue migrating lower-level mechanics into higher-level, framework-style abstractions.
