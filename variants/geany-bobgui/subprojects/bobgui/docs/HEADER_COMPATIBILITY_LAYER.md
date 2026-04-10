# Bobgui Header Compatibility Layer

## Purpose
As bobgui transitions from a flat `modules/*` layout to grouped module categories (`core`, `media`, `network`, `system`, `tools`, `visual`), existing include paths can drift.

This session added a small compatibility layer to reduce that drift.

## What was added
Two kinds of headers were added:

### 1. Grouped headers
New grouped headers were added for several modules whose `.c` files already existed but whose local headers were missing, including:
- `modules/core/brain/bobguibrain.h`
- `modules/core/data/bobguidata.h`
- `modules/core/state/bobguistate.h`
- `modules/media/audio/bobguiaudio.h`
- `modules/media/3d/bobgui3d.h`
- `modules/system/live/bobguilive.h`
- `modules/visual/chart/bobguichart.h`
- `modules/visual/dock/bobguidock.h`
- `modules/visual/graph/bobguigraph.h`
- `modules/system/iot/bobguiiot.h`

### 2. Legacy wrapper headers
Legacy flat-path wrappers were added so older include locations still resolve while the tree is being normalized, such as:
- `modules/audio/bobguiaudio.h`
- `modules/brain/bobguibrain.h`
- `modules/data/bobguidata.h`
- `modules/3d/bobgui3d.h`
- `modules/state/bobguistate.h`
- `modules/live/bobguilive.h`
- `modules/chart/bobguichart.h`
- `modules/dock/bobguidock.h`
- `modules/graph/bobguigraph.h`
- `modules/iot/bobguiiot.h`

## Why this matters
This improves the codebase in three ways:
1. makes the grouped hierarchy more internally consistent
2. reduces breakage while old paths still exist in build/install metadata
3. creates a cleaner path for future API stabilization and possible C++ facade work

## Recommended next step
The next step should be a full pass over remaining module headers to:
- add any missing grouped headers
- convert remaining old flat references to grouped ones
- retire legacy wrappers only after build validation confirms the grouped layout is stable
