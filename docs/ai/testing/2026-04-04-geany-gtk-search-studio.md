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
- Close and reopen it
- Confirm it reopens near the previous position

### Find tab
- Enter a literal string and use Find Next / Find Previous
- Confirm matching works and Find Next / Previous state updates correctly
- Use Count and confirm the status bar reports the count
- Switch mode to Extended and verify escape-sequence processing works for simple cases like `\n`
- Switch mode to Regex and verify multiline / dot-all controls become enabled

### Mark tab
- Enter a search string and click Mark now
- Confirm search indicators are shown
- Enable `Bookmark matching lines` and run again
- Confirm line markers are placed on matching lines
- Enable `Purge existing bookmarks first` and rerun with a different term
- Confirm previous bookmarks are removed before the new set is added
- Click `Clear marks`
- Confirm both indicators and bookmarks are removed

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
