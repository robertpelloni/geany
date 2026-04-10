# Bobgui Module Category Guide

## Purpose
This guide documents the grouped module layout that is gradually replacing the older flat `bobgui/modules/*` layout.

## Categories

### core
Foundational runtime and data-oriented capabilities.
Examples:
- reflection
- layout
- stream
- meta
- entity
- autonomous
- realtime
- i18n
- fin

Primary umbrella header:
- `bobgui/modules/core/core.h`

### media
Audio, bio/medical, rendering-adjacent media workflows, and specialized media UI.
Examples:
- audio widgets
- MIDI 2
- plugin host
- bio
- holograph
- physics

Primary umbrella header:
- `bobgui/modules/media/media.h`

### network
Connectivity, web, cloud, sync, and decentralized/networked integrations.
Examples:
- network
- blockchain
- cloud
- remote
- sync
- web

Primary umbrella header:
- `bobgui/modules/network/network.h`

### system
Platform/runtime and device-facing integrations.
Examples:
- input
- ipc
- omni runtime
- virtual OS
- AI vision accessibility

Primary umbrella header:
- `bobgui/modules/system/system.h`

### tools
Developer and workflow tooling.
Examples:
- forge
- report
- studio

Primary umbrella header:
- `bobgui/modules/tools/tools.h`

### visual
Rendering abstraction and future visual/editor-facing subsystems.
Examples:
- RHI

Primary umbrella header:
- `bobgui/modules/visual/visual.h`

## Transitional state
The repository currently contains a mixture of:
- older flat paths
- grouped paths
- duplicated compatibility headers in a few places

The recommended direction is:
1. prefer grouped paths for new work
2. expose grouped umbrella headers for easier adoption
3. retire duplicated flat paths incrementally after build validation

## Recommended include style
For grouped APIs:
- `#include <bobgui/modules/core/core.h>`
- `#include <bobgui/modules/media/media.h>`
- `#include <bobgui/modules/network/network.h>`
- `#include <bobgui/modules/system/system.h>`
- `#include <bobgui/modules/tools/tools.h>`
- `#include <bobgui/modules/visual/visual.h>`

For all grouped APIs together:
- `#include <bobgui/modules/bobguimodules.h>`
