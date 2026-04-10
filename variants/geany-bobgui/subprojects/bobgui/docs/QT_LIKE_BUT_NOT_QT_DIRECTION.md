# Qt-like but not Qt: Bobgui Direction

## Intent
The goal is not to clone Qt mechanically. The goal is to make bobgui better at the things developers actually like about Qt:
- coherent app-shell ergonomics
- discoverable commands and actions
- less manual widget plumbing
- stronger framework-level affordances for real applications

## What has been added in that direction
### Workbench layer
`BobguiWorkbench` provides a higher-level shell abstraction for:
- main application window
- title/header bar
- central widget
- sidebars
- status line
- header actions

### Command palette layer
`BobguiCommandPalette` adds:
- a dedicated command launcher window
- structured command registration
- command activation callbacks
- search-driven filtering over registered commands

### Grouped module umbrella headers
Grouped category headers now make the framework easier to consume at a higher level:
- `core/core.h`
- `media/media.h`
- `network/network.h`
- `system/system.h`
- `tools/tools.h`
- `visual/visual.h`
- `bobguimodules.h`

## Why this matters
This is the difference between:
- a toolkit that merely has widgets
- and a framework that helps developers build complete applications quickly

## Recommended next ergonomic steps
1. central action registry shared by workbench and command palette
2. shortcut-driven palette activation
3. dock integration for workbench panels
4. app template/demo that shows the intended "bobgui way"
5. optional C++ facade layer for easier application authoring without replacing the C core
