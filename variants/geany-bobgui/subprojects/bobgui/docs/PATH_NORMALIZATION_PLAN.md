# Bobgui Path Normalization Plan

## Goal
Continue the migration from legacy flat module paths to the grouped module hierarchy without breaking developer workflows or installed header compatibility.

## What this pass improved
This pass moved the public header list in `bobgui/meson.build` closer to the grouped layout by replacing legacy flat module header references with grouped paths where grouped headers now exist.

Examples:
- `modules/audio/bobguiaudio.h` -> `modules/media/audio/bobguiaudio.h`
- `modules/state/bobguistate.h` -> `modules/core/state/bobguistate.h`
- `modules/chart/bobguichart.h` -> `modules/visual/chart/bobguichart.h`
- `modules/shell/bobguishell.h` -> `modules/system/shell/bobguishell.h`

## Structural improvements in this pass
New grouped implementation/header pairs were also added for directories that existed but were empty or partially defined:
- `modules/visual/dsl/`
- `modules/system/shell/`
- `modules/system/package/`

This reduces the number of places where Meson referenced files that did not physically exist.

## Remaining migration work
The tree still contains compatibility and duplication that should be addressed incrementally:

### Duplicate compatibility roots
These remain as migration artifacts and should be reviewed later:
- `modules/cloud/`
- `modules/remote/`
- `modules/sync/`
- `modules/web/`
- several legacy flat header wrapper directories such as `modules/audio/`, `modules/brain/`, etc.

### Areas still needing deeper normalization
1. source/include style normalization across module `.c` files
2. installed header policy for grouped vs compatibility wrappers
3. eventual deprecation strategy for flat wrapper includes
4. build validation to ensure grouped paths compile cleanly in isolation

## Recommendation
The next best cleanup is:
1. inventory all duplicate compatibility directories
2. decide which are temporary wrappers vs true source locations
3. add lightweight compile/configure validation for grouped headers
4. choose one subsystem (`audio`, `network`, `dock`, or `layout`) and deepen implementation quality
