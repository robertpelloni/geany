# Network Compatibility Consolidation

## Summary
This pass focused on reducing confusion around duplicate module roots for network-related features.

Historically, bobgui contained both:
- grouped network modules under `bobgui/modules/network/...`
- flat compatibility roots under:
  - `bobgui/modules/cloud/`
  - `bobgui/modules/remote/`
  - `bobgui/modules/sync/`
  - `bobgui/modules/web/`

## What changed
The flat compatibility headers were converted into lightweight wrappers that forward to the grouped network headers:
- `modules/cloud/bobguicloud.h` -> `modules/network/cloud/bobguicloud.h`
- `modules/remote/bobguiremote.h` -> `modules/network/remote/bobguiremote.h`
- `modules/sync/bobguisync.h` -> `modules/network/sync/bobguisync.h`
- `modules/web/bobguiweb.h` -> `modules/network/web/bobguiweb.h`

Additionally, the grouped sync header was updated to reference the grouped core state header:
- `#include <bobgui/modules/core/state/bobguistate.h>`

## Why this matters
This improves the codebase by:
1. making grouped module ownership clearer
2. preserving compatibility for legacy include paths
3. reducing the chance of APIs drifting between duplicate header definitions
4. continuing the migration toward a single coherent module hierarchy

## Recommended next step
The next cleanup target should be similar consolidation for any remaining duplicated compatibility roots outside the network family, followed by a deeper implementation pass on one of the grouped network modules.
