# Implementation notes: GTK Search Studio

## Summary

This pass adds a new **Search Studio** dialog to the main GTK-based Geany codebase.

It does not remove the classic dialogs. Instead, it creates a denser unified front-end while preserving the existing search implementation.

## Files changed

### Search engine / UI
- `src/search.c`
- `src/search.h`
- `src/callbacks.c`
- `src/callbacks.h`
- `data/geany.glade`

## Main implementation elements

### 1. New Search Studio dialog state
Added a new internal state block:
- `studio_dlg.dialog`
- `studio_dlg.notebook`
- tab page pointers
- saved position

Its window position is persisted through the same stash/session mechanism used by the other search dialogs.

### 2. New public entry point
Added:
- `search_show_search_studio_dialog()`

This creates or presents the Search Studio notebook dialog.

### 3. Search Studio pages
Implemented four page builders:
- `search_studio_create_find_page()`
- `search_studio_create_replace_page()`
- `search_studio_create_fif_page()`
- `search_studio_create_mark_page()`

### 4. Explicit search mode model
Implemented shared mode helpers:
- `search_studio_create_mode_box()`
- `search_studio_get_mode()`
- `search_studio_mode_toggled()`
- `search_studio_whole_word_toggled()`

These map Search Studio modes as follows:
- **Normal** → literal search, no escape transformation
- **Extended** → escape-sequence processing without regex
- **Regex** → regex search, multiline/dot-all controls enabled

This gives Geany a much clearer conceptual mapping to Notepad++'s search modes.

### 5. Shared option extraction
Implemented:
- `search_studio_prepare_find()`
- `search_studio_store_search_data()`

These functions:
- gather text from the tab
- map mode and checkbox state to `GeanyFindFlags`
- validate regex when needed
- apply escape conversion when needed
- update `search_prefs.always_wrap`
- synchronize `search_data` so Find Next / Previous keeps working

### 6. New direct search actions in Search Studio
Implemented GTK callbacks for:
- find next
- find previous
- count
- mark/bookmark
- clear marks

These call Geany's existing search/document primitives rather than inventing a new backend.

### 7. Extended mark behavior
Implemented:
- `search_mark_all_with_options()`
- `search_clear_all_marks()`

This extends Geany's marking behavior beyond highlight-only mode by optionally:
- bookmarking matching lines using marker `1`
- purging existing bookmarks first

This is a deliberate step toward Notepad++'s richer Mark feature set.

### 8. Classic dialog bridges
Implemented page-to-dialog sync helpers:
- `search_studio_sync_find_dialog_from_page()`
- `search_studio_sync_replace_dialog_from_page()`

These let Search Studio open classic dialogs with synced text and option state.

### 9. Menu integration
Added a new Search menu entry:
- `Search Studio...`

Callback:
- `on_search_studio1_activate()`

## What this improves versus previous Geany behavior

### Before
- fragmented search UI
- weaker search mode discoverability
- no unified notebook-based search cockpit
- Mark behavior mostly limited to highlight marking

### After
- unified notebook search cockpit exists in GTK Geany
- Normal / Extended / Regex are explicit
- Count is a first-class action
- marking can optionally bookmark lines
- classic dialogs remain reachable and prefilleable

## Remaining technical debt

1. Replace tab is still a bridge, not yet a full standalone executor.
2. Find in Files tab is still a bridge, not yet a full advanced option surface.
3. Search Studio currently has no integrated results/preview pane.
4. Search Studio state is not yet fully normalized into a reusable frontend-independent model object.

## Why this was implemented this way

A direct rewrite of all search surfaces would have been riskier and harder to validate. This pass instead maximizes value by:
- reusing existing engine paths
- adding a new unified shell
- improving Mark semantics
- creating a practical path for future BobUI parity work
