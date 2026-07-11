package editor

// InsertText inserts text into the document at a given offset.
func (d *Document) InsertText(offset int, text string) error {
	// Stub: This will communicate with the Scintilla engine backend
	// via geany-go/scintilla CGO bindings in the future.
	d.SetModified(true)
	return nil
}

// GetText retrieves the entire text of the document.
func (d *Document) GetText() string {
	// Stub
	return ""
}
