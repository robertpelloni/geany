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
- Diff Preview

The Results tab is backed by a `GtkListStore`/`GtkTreeView` and records structured entries with columns such as:
- Action
- Target
- Query
- Mode
- Summary

Additional hidden metadata columns store navigation and preview context such as:
- filename
- line
- position
- whether the row is navigable
- preview title
- preview body

The internal result-routing code is now also partially normalized through a shared `SearchStudioResultSpec` model object in `src/search.c`. This reduces parameter soup in row append helpers and gives the GTK implementation a cleaner stepping stone toward a future frontend-independent Search Studio backend model.

Search request preparation is also starting to normalize through internal query/replace spec helpers (`SearchStudioFindSpec` and `SearchStudioReplaceSpec`). The GTK code still owns execution directly, but it now has a clearer seam between:
- request extraction from the UI
- action execution
- result-row routing

Shared action-layer helpers are now also starting to reduce repeated execution-adjacent glue such as:
- combo-history updates
- active-document target labeling
- summary-row emission for active-document actions
- formatted summary-row emission for session and document actions
- shared statusbar reporting for count/mark families
- open-document iteration for session-oriented actions

This means several Search Studio callbacks now read more like small orchestration functions, while reusable helper paths own the repetitive document/session summary plumbing.

Session-oriented helpers are now also converging around a shared open-document visitor path. Match collection, replace-preview session rows, replace-impact session rows, count session, mark session, and clear-session marks all benefit from that normalization direction.

This is another meaningful backend-boundary step because session-scope work is increasingly described as:
- a shared prepared request
- a reusable open-document traversal helper
- per-document callbacks that emit rows or mutate state
- shared summary/status reporting on top

This is still not a full backend split, but it makes the remaining execution code less repetitive and clarifies which pieces are generic enough to move behind a frontend-independent boundary later.

This is now more than a summary-level results surface: for current-document operations it can append concrete match rows and respond to row activation by navigating to the stored match position. Selection changes can also update the Diff Preview pane with row-specific details.

More result row types now carry preview payloads, including:
- generic current-document/session match rows
- count impact rows
- replace preview rows
- bulk replace impact rows
- session mark impact rows
- ingested Find in Files rows

The lower notebook now also behaves more intentionally:
- appending result rows pulls the user toward the Results tab
- selecting a row pivots to Diff Preview automatically
- activating a non-navigable informational row no longer just beeps; it now reinforces that the row is for inspection in Diff Preview
- clearing results resets the Diff Preview instructional state

Bulk replace operations now also append document-impact rows before mutation-heavy actions run. Those rows capture affected-document counts and first-hit context, which gives Search Studio a better bridge between preview-only workflows and real replace execution.

Replace preview construction is now more semantic as well. Instead of only splicing the raw payload string into line context, preview helpers now resolve the actual replacement text that Geany would apply (including regex backreference expansion) and present an original-line vs replacement-line view plus matched-segment diff text.

The Find and Mark tabs now also have session-wide execution paths beyond simple hit collection:
- `Count Session` aggregates match counts across open documents and emits per-document count impact rows
- `Mark Session` applies highlight/bookmark behavior across open documents
- `Clear Session Marks` removes search indicators and bookmarks across open documents
- affected documents emit navigable count/mark impact rows with first-hit context and option summaries

This pushes Search Studio beyond strict Notepad++ parity in useful areas: Geany can now treat count review and mark/bookmark review as multi-document operations inside the same cockpit.

This means the Diff Preview pane is increasingly becoming a universal inspection surface instead of a replace-preview-only feature, and the lower notebook is starting to behave like a real result navigator rather than three unrelated panes.

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
- `search_studio_replace_preview_document()`
- `search_studio_replace_preview_session()`
- preview-row collection helpers for replace candidates

This moved Search Studio Replace beyond simple bridging into direct execution against the active document/session, and added dry-run groundwork so users can inspect replacement candidate rows before modifying content. Replace preview rows now carry richer preview text so the Diff Preview pane can show a simple before/match/payload/after view.

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

Search Studio-launched Find in Files searches now also feed grep output back into the Results pane through a lightweight capture path. This gives the dialog a first real cross-file results ingestion loop instead of treating Find in Files as a pure fire-and-forget background action.

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

1. Find in Files tab is now executable and can ingest its own grep output into Search Studio results, but it is still not as dense as Notepad++ or Geany's classic advanced dialog.
2. Search Studio now has both activity and structured results panes, and the Find tab can collect active-document/open-document hits while Find in Files can ingest launched results. Count now also has active-document/session impact rows, bulk replace actions emit richer document-impact rows, and Mark can operate across the open-document set with session-mark impact rows. Lower-pane focus and informational-row handling are more navigator-like now, but it is still not yet a full universal hit-list / navigation result viewer across every action.
3. Replace preview/dry-run groundwork exists and now feeds a dedicated Diff Preview pane, and more row types provide richer previews. It now resolves actual replacement text more accurately, but it is still a lightweight text preview rather than a true semantic diff viewer.
4. Search Studio state is not yet fully normalized into a reusable frontend-independent model object, though result-row metadata is now partially normalized through an internal `SearchStudioResultSpec`.

## Why this was implemented this way

A direct rewrite of all search surfaces would have been riskier and harder to validate. This pass instead maximizes value by:
- reusing existing engine paths
- adding a new unified shell
- improving Mark semantics
- creating a practical path for future BTK parity work

That BTK path is now slightly better grounded by first-wave compatibility helpers in `src/gtkcompat.h` for notebook positioning, dialog/widget CSS naming, and CSS-provider seams, which are the kinds of high-touch UI calls Search Studio will likely stress during a future toolkit transition.
