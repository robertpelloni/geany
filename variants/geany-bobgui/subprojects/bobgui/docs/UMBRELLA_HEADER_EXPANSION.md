# Bobgui Umbrella Header Expansion

## Summary
This pass expanded the grouped umbrella headers so they behave more like real public entry points rather than partial examples.

The goal is to make bobgui easier to consume at a category level, similar to how larger frameworks expose curated entry headers.

## What changed

### Core umbrella
`bobgui/modules/core/core.h` now includes a broader set of grouped core APIs:
- autonomous
- brain
- translate
- compute
- data
- entity
- fin
- i18n
- layout
- meta
- quantum
- realtime
- reflect
- script
- state
- stream

### Media umbrella
`bobgui/modules/media/media.h` now includes:
- 3d
- audio base
- audio widgets
- midi2
- plugin host
- bio
- gis
- holograph
- physics
- shader
- spatial
- timeline

### System umbrella
`bobgui/modules/system/system.h` now includes:
- input
- iot
- ipc
- live
- omni
- os
- package
- plugin
- security
- shell
- vfs
- vision

### Tools umbrella
`bobgui/modules/tools/tools.h` now includes:
- forge
- report
- studio
- test

### Visual umbrella
`bobgui/modules/visual/visual.h` now includes:
- chart
- design
- dock
- dsl
- graph
- imgui
- rhi

## Why this matters
This improves bobgui in several ways:
1. easier category-based adoption
2. fewer manual includes for common workflows
3. stronger alignment between the grouped module hierarchy and the developer-facing API
4. a better foundation for eventual C++ facade work or Qt-like curated entry points

## Recommended next step
The next useful cleanup would be to create installation/include tests for:
- `bobgui/modules/core/core.h`
- `bobgui/modules/media/media.h`
- `bobgui/modules/network/network.h`
- `bobgui/modules/system/system.h`
- `bobgui/modules/tools/tools.h`
- `bobgui/modules/visual/visual.h`
- `bobgui/modules/bobguimodules.h`

That would make the migration measurable instead of purely structural.
