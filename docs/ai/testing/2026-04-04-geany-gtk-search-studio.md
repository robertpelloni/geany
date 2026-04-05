# Testing notes: GTK Search Studio

## Manual verification

### Menu entry
- Open the Search menu
- Confirm a new item exists:
  - `Search Studio...`

### Dialog presentation
- Open Search Studio
- Confirm it opens a notebook dialog with tabs:
  - Find
  - Replace
  - Find in Files
  - Mark
- Confirm a lower notebook pane is visible with:
  - Activity
  - Results
  - Diff Preview
- Switch tabs and confirm the Activity pane receives page-specific guidance entries
- Confirm the Results pane starts with an initialization row
- Confirm the lower notebook initially focuses the Results tab after initialization rows are appended
- Confirm the Diff Preview pane shows a default instructional message before any row selection
- Close and reopen it
- Confirm it reopens near the previous position

### Find tab
- Enter a literal string and use Find Next / Find Previous
- Confirm matching works and Find Next / Previous state updates correctly
- Confirm the Activity pane records a summary line for the action
- Confirm the Results pane gets a structured row for the action
- For a successful current-document find, activate the newest match row and confirm Geany jumps to the stored match position
- Select a result row and confirm the Diff Preview pane updates with row-specific detail text
- Confirm row selection automatically pivots the lower notebook to the Diff Preview tab
- Confirm generic match rows now show richer line-context preview text instead of only a one-line summary
- Use Count and confirm the status bar reports the count
- Confirm the Activity pane records the count summary
- Confirm the Results pane gets a Count row
- Confirm a count-impact row is appended for the active document and that selecting it shows richer first-hit context in Diff Preview
- Confirm at least some match-detail rows are appended for current-document count operations
- Activate one of the appended match rows and confirm Geany jumps to that match
- Use `Count Session` and confirm the status bar reports an aggregate count across open documents
- Confirm per-document count-impact rows are appended for affected documents
- Select one of those session count-impact rows and confirm the Diff Preview pane updates with richer first-hit context
- Activate one of those session count-impact rows and confirm Geany switches to that document and jumps to the stored first match
- Use `Collect Document Hits` and confirm a broader set of navigable current-document match rows is appended
- Use `Collect Session Hits` and confirm hits from multiple open documents can be appended to the Results pane
- Activate one of the collected session-hit rows for an already-open file and confirm Geany switches to that document and jumps to the match
- Use `Clear Results` and confirm the Results pane is cleared and a clear event is recorded
- Confirm clearing results resets the Diff Preview pane back to its instructional state
- Switch mode to Extended and verify escape-sequence processing works for simple cases like `\n`
- Switch mode to Regex and verify multiline / dot-all controls become enabled

### Mark tab
- Enter a search string and click Mark now
- Confirm search indicators are shown
- Confirm the Activity pane records the mark action
- Confirm the Results pane gets a Mark row
- Confirm at least some match-detail rows are appended for current-document mark operations
- Activate one of the appended match rows and confirm Geany jumps to that match
- Enable `Bookmark matching lines` and run again
- Confirm line markers are placed on matching lines
- Enable `Purge existing bookmarks first` and rerun with a different term
- Confirm previous bookmarks are removed before the new set is added
- Use `Mark Session`
- Confirm search indicators are applied across multiple open documents
- If bookmarking is enabled, confirm bookmark markers are applied across multiple open documents as well
- Confirm session-mark impact rows are appended for affected documents and that selecting one shows richer first-hit context in Diff Preview
- Activate one of the session-mark impact rows and confirm Geany switches to that document and jumps to the first stored match
- Click `Clear marks`
- Confirm both indicators and bookmarks are removed from the active document
- Click `Clear Session Marks`
- Confirm indicators and bookmarks are removed across open documents
- Confirm the clear actions are reflected in the Activity pane
- Confirm the Results pane gets clear-mark summary rows

### Replace tab
- Enter find/replace values
- Verify these actions work directly from Search Studio:
  - Find Next
  - Replace
  - Replace & Find
  - Replace in Document
  - Replace in Selection
  - Replace in Session
  - Preview in Document
  - Preview in Session
- Confirm the Activity pane records a summary for each action
- Confirm the Results pane gets structured rows for each action
- For the Replace tab's Find-style operations, confirm navigable match rows appear when applicable
- For `Replace in Document`, confirm document-impact rows are appended before/alongside the summary row and that selecting one shows richer first-hit context in Diff Preview with resolved replacement text
- For preview actions, confirm candidate rows appear without modifying the document
- Select a replace preview candidate row and confirm the Diff Preview pane shows richer original-line / replacement-line / matched-segment diff text
- In Regex mode, verify a preview using backreferences (for example, find `(foo)(bar)` and replace with `\\2-\\1`) shows the resolved replacement text instead of only echoing the raw payload
- For `Replace in Session`, confirm per-document impact rows are appended for affected open documents and that activating one can navigate to the first affected location in that document
- Confirm classic Replace can still be opened with synced state

### Find in Files tab
- Enter search text and a valid directory
- Verify `Find in Files now` starts a search directly from Search Studio
- Confirm the Activity pane records the launch summary and selected options
- Confirm the Results pane gets a Find in Files row with target/query/summary fields
- After results stream in, confirm additional Find in Files hit rows appear in the Results pane
- Select an ingested Find in Files hit row and confirm the Diff Preview pane updates with richer hit details
- Activate one of the ingested Find in Files hit rows and confirm Geany opens or switches to the corresponding file and jumps near the matched line
- Confirm row activation on non-navigable summary rows does not crash and instead reinforces that the row is informational and should be inspected in Diff Preview
- Verify file pattern validation works when non-`all` modes are selected
- Verify classic Find in Files can still be opened with the text/directory prefilled

### Classic bridge buttons
- From Find tab, click `Open classic Find dialog`
- Confirm the classic Find dialog opens with synced text/settings
- From Replace tab, click `Open classic Replace dialog`
- Confirm the classic Replace dialog opens with synced text/settings
- From Find in Files tab, click `Open full Find in Files`
- Confirm the classic Find in Files dialog opens with the text/directory prefilled

## Regression checks
- Existing Search menu items still work:
  - Find
  - Find Next
  - Find Previous
  - Replace
  - Find in Files
- Existing classic dialogs still open and operate normally
- Structured result routing still works for summary rows, navigable match rows, replace preview rows, count impact rows, mark impact rows, and ingested Find in Files rows after the internal request/result-spec normalization passes
- Active-document Find/Count/Mark/Replace summary rows still target the correct document after the shared action-layer helper passes
- Session-level Count/Mark/Replace Preview/Replace summary rows still report the correct scope/summary text after formatted summary helper extraction
- Session-oriented actions still behave correctly after open-document visitor normalization (no missing affected-doc counts, no missing session rows, and no obvious regression in session-hit/session-mark/session-replace-preview/session-replace-impact/clear-session-mark collection)
- Count/Mark status bar messaging still reports the correct active-document vs open-document scope after shared status-report helper extraction

## Environment limitation
- Full compile validation was not performed here if the required toolchain is unavailable in the current environment.
