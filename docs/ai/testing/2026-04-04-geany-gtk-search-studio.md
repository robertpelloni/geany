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
- Switch tabs and confirm the Activity pane receives page-specific guidance entries
- Confirm the Results pane starts with an initialization row
- Close and reopen it
- Confirm it reopens near the previous position

### Find tab
- Enter a literal string and use Find Next / Find Previous
- Confirm matching works and Find Next / Previous state updates correctly
- Confirm the Activity pane records a summary line for the action
- Confirm the Results pane gets a structured row for the action
- For a successful current-document find, activate the newest match row and confirm Geany jumps to the stored match position
- Use Count and confirm the status bar reports the count
- Confirm the Activity pane records the count summary
- Confirm the Results pane gets a Count row
- Confirm at least some match-detail rows are appended for current-document count operations
- Activate one of the appended match rows and confirm Geany jumps to that match
- Use `Collect Document Hits` and confirm a broader set of navigable current-document match rows is appended
- Use `Collect Session Hits` and confirm hits from multiple open documents can be appended to the Results pane
- Activate one of the collected session-hit rows for an already-open file and confirm Geany switches to that document and jumps to the match
- Use `Clear Results` and confirm the Results pane is cleared and a clear event is recorded
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
- Click `Clear marks`
- Confirm both indicators and bookmarks are removed
- Confirm the clear action is reflected in the Activity pane
- Confirm the Results pane gets a clear-mark row

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
- For preview actions, confirm candidate rows appear without modifying the document
- Confirm classic Replace can still be opened with synced state

### Find in Files tab
- Enter search text and a valid directory
- Verify `Find in Files now` starts a search directly from Search Studio
- Confirm the Activity pane records the launch summary and selected options
- Confirm the Results pane gets a Find in Files row with target/query/summary fields
- After results stream in, confirm additional Find in Files hit rows appear in the Results pane
- Activate one of the ingested Find in Files hit rows and confirm Geany opens or switches to the corresponding file and jumps near the matched line
- Confirm row activation on non-navigable summary rows does not crash and simply does nothing useful / may beep
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

## Environment limitation
- Full compile validation was not performed here if the required toolchain is unavailable in the current environment.
