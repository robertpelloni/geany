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
- replace
- replace and find
- replace in document
- replace in selection
- replace in session
- find in files launch from Search Studio

These call Geany's existing search/document primitives rather than inventing a new backend.

### 7. Integrated activity / preview pane
The Search Studio dialog now includes a lower read-only activity pane.

It is not yet a full search-results viewer, but it provides immediate in-dialog feedback for:
- current page guidance
- find/count summaries
- replace actions
- mark/bookmark actions
- file-search launches
- classic-dialog bridge actions

This gives the dialog a much more cockpit-like feel and starts building the case for a future richer preview/results surface.

### 8. Structured results pane
The lower pane now uses a notebook with separate views for:
- Activity
- Results

The Results tab is backed by a `GtkListStore`/`GtkTreeView` and records structured entries with columns such as:
- Action
- Target
- Query
- Mode
- Summary

Additional hidden metadata columns store navigation context such as:
- filename
- line
- position
- whether the row is navigable

This is now more than a summary-level results surface: for current-document operations it can append concrete match rows and respond to row activation by navigating to the stored match position.

It is not yet a full universal hit-list, but it is a major architectural step because Search Studio now distinguishes between:
- narrative workflow logging
- structured operation records
- navigable match entries

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

### 9. Direct Find in Files execution helpers
Introduced shared helpers:
- `build_grep_options()`
- `execute_find_in_files_request()`

This reduces duplication and makes both the classic Find in Files dialog and Search Studio use the same validation and launch path.

### 10. Direct replace execution helpers
Introduced:
- `search_studio_prepare_replace()`
- `search_studio_replace_action_activate()`

This moved Search Studio Replace beyond simple bridging into direct execution against the active document/session.

### 11. Notebook activity logging
Added helpers such as:
- `search_studio_activity_append()`
- `search_studio_result_append()`
- `search_studio_activity_show_page_hint()`
- `search_studio_notebook_switch_page()`

These record useful summaries directly inside Search Studio so the user can see what happened without relying only on the status bar.

### 12. Structured result capture
Search actions now append structured result records for operations like:
- Find
- Count
- Mark
- Clear marks
- Replace variants
- Find in Files launches
- classic-dialog bridge launches

For Find / Count / Mark / Replace-find on the active document, Search Studio also appends concrete match rows (with capped detail volume) so users can activate a row and jump to that match.

Additionally, the Find tab now supports explicit hit collection actions:
- collect hits from the active document
- collect hits from all open documents
- clear the results pane

This makes Search Studio feel much more like a real search workbench rather than a collection of form controls.

## What this improves versus previous Geany behavior

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

1. Find in Files tab is now executable, but still not as dense as Notepad++ or Geany's classic advanced dialog.
2. Search Studio now has both activity and structured results panes, and the Find tab can collect active-document and open-document hits, but it is still not yet a full universal hit-list / navigation result viewer across every action.
3. Replace operations do not yet offer preview/dry-run mode.
4. Search Studio state is not yet fully normalized into a reusable frontend-independent model object.

## Why this was implemented this way

A direct rewrite of all search surfaces would have been riskier and harder to validate. This pass instead maximizes value by:
- reusing existing engine paths
- adding a new unified shell
- improving Mark semantics
- creating a practical path for future BobUI parity work
