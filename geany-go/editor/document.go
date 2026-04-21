package editor

import (
	"fmt"
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

// Open reads the file from disk (stub implementation for now).
func (d *Document) Open() error {
	if d.FileName == "" {
		return fmt.Errorf("cannot open document with no filename")
	}

	// In a real implementation, this would read the file contents into a buffer/Scintilla instance.
	// For now, we just verify the file exists.
	info, err := os.Stat(d.FileName)
	if err != nil {
		return err
	}

	// Check read-only status based on permissions
	if info.Mode().Perm()&0200 == 0 {
		d.ReadOnly = true
	}

	return nil
}

// Save writes the document back to disk (stub implementation).
func (d *Document) Save() error {
	if d.ReadOnly {
		return fmt.Errorf("cannot save read-only document")
	}

	d.Changed = false
	return nil
}

// SetChanged marks the document as having unsaved modifications.
func (d *Document) SetChanged(changed bool) {
	d.Changed = changed
}
