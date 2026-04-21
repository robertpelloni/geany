package editor

import (
	"path/filepath"
)

// Document represents an open file within the Geany-Go editor.
// This is a direct port and modernization of the GeanyDocument C struct.
type Document struct {
	ID        int
	FileName  string
	BaseName  string
	Changed   bool
	ReadOnly  bool
	Encoding  string // e.g., "UTF-8"
	HasBOM    bool
}

// NewDocument initializes a new Document instance.
func NewDocument(id int, path string) *Document {
	return &Document{
		ID:       id,
		FileName: path,
		BaseName: filepath.Base(path),
		Changed:  false,
		ReadOnly: false,
		Encoding: "UTF-8", // Default
	}
}

// SetModified marks the document's unsaved modification state.
// This triggers UI updates (e.g. adding an asterisk to the tab name).
func (d *Document) SetModified(modified bool) {
	d.Changed = modified
	// In a full implementation, this would emit an event to the geany-go/ui
	// interface so the frontend could update the tab representation.
}

// IsModified returns true if the document has unsaved changes.
func (d *Document) IsModified() bool {
	return d.Changed
}
