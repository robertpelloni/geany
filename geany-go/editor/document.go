package editor

import (
	"os"
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
	FileObj   *os.File
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



// SetChanged marks the document as having unsaved modifications.
func (d *Document) SetChanged(changed bool) {
	d.Changed = changed
}
