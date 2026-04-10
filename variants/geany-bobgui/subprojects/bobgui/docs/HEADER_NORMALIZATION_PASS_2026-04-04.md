# Bobgui Header Normalization Pass — 2026-04-04

## Summary
This pass focused on reducing structural fragility in the evolving grouped module hierarchy.

The primary goal was simple: every module `.c` file under `bobgui/modules/` should be able to resolve its first local `#include "...h"` header from the same grouped directory.

## Why this matters
As bobgui has grown, its module tree has moved from a flat layout toward a categorized hierarchy:
- `core`
- `media`
- `network`
- `system`
- `tools`
- `visual`

However, many grouped implementation files existed before their grouped headers did. That meant the source tree looked organized while still depending on older flat-path compatibility assumptions.

This pass closes part of that gap.

## What was added
Grouped headers were added for the following module directories so local implementation includes now resolve correctly:

### Core
- `modules/core/brain/bobguibrain.h`
- `modules/core/data/bobguidata.h`
- `modules/core/state/bobguistate.h`
- `modules/core/compute/bobguicompute.h`
- `modules/core/quantum/bobguiquantum.h`
- `modules/core/script/bobguiscript.h`

### Media
- `modules/media/3d/bobgui3d.h`
- `modules/media/audio/bobguiaudio.h`
- `modules/media/gis/bobguigis.h`
- `modules/media/shader/bobguishader.h`
- `modules/media/spatial/bobguispatial.h`
- `modules/media/timeline/bobguitimeline.h`

### System
- `modules/system/live/bobguilive.h`
- `modules/system/iot/bobguiiot.h`
- `modules/system/plugin/bobguiplugin.h`
- `modules/system/security/bobguisecurity.h`
- `modules/system/vfs/bobguivfs.h`

### Tools / Visual
- `modules/tools/test/bobguitest.h`
- `modules/visual/chart/bobguichart.h`
- `modules/visual/design/bobguidesign.h`
- `modules/visual/dock/bobguidock.h`
- `modules/visual/graph/bobguigraph.h`
- `modules/visual/imgui/bobguiimgui.h`

## Result
After this pass, the grouped module implementation files no longer have unresolved same-directory header references for their first local include.

That means the tree is now more internally coherent and easier to continue migrating.

## Remaining architectural realities
This does **not** mean the whole build is fully normalized yet.
Important remaining realities:
- some install/public header lists still contain both grouped and legacy compatibility paths
- some compatibility wrappers still exist intentionally during migration
- several modules remain scaffolding/stubs rather than production implementations
- there are still duplicate top-level compatibility directories such as `modules/cloud`, `modules/remote`, `modules/sync`, and `modules/web`

## Recommended next step
The next highest-value cleanup would be:
1. normalize public header lists in `bobgui/meson.build`
2. remove redundant compatibility directories only after confirming grouped paths are stable
3. turn one grouped vertical slice into a truly working subsystem (`audio`, `network`, `layout`, or `dock`)

## Conclusion
This pass improved the internal honesty of the module tree: grouped source files now actually have grouped headers beside them, instead of pretending the migration is further along than it really is.
