package editor

import (
	"path/filepath"
)

// Backend defines the low-level text manipulation interface that any UI frontend
// (e.g., Scintilla in GTK, QTextEdit in Qt) must implement to be controlled by the Go core.
// This is the Go-native abstraction mirroring the C++ ScintillaWrapper.
type Backend interface {
	GetText() string
	SetText(text string)
	GetTextRange(start, end int) string
	InsertText(pos int, text string)
	DeleteRange(start, end int)
	GetLineCount() int
	GetLineLength(line int) int
	SetReadOnly(ro bool)
	IsReadOnly() bool

	// Cursor and Selection
	GetCursorPos() int
	SetCursorPos(pos int)
	GetSelection() (int, int)
	SetSelection(start, end int)
}

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
    Backend   Backend
}

// NewDocument initializes a new Document instance.
func NewDocument(id int, path string, backend Backend) *Document {
	return &Document{
		ID:       id,
		FileName: path,
		BaseName: filepath.Base(path),
		Changed:  false,
		ReadOnly: false,
		Encoding: "UTF-8", // Default
        Backend: backend,
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
