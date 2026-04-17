package editor

import (
	"fmt"
	"os"
	"unicode/utf8"
)

// Open reads the file from disk into memory.
// It verifies existence, sets the ReadOnly flag based on permissions,
// and determines if the file contains valid UTF-8 text.
func (d *Document) Open() error {
	if d.FileName == "" {
		return fmt.Errorf("cannot open document with no filename")
	}

	info, err := os.Stat(d.FileName)
	if err != nil {
		return err
	}

	// Check read-only status based on permissions (no write bit)
	if info.Mode().Perm()&0200 == 0 {
		d.ReadOnly = true
	}

	// Read entire file content
	content, err := os.ReadFile(d.FileName)
	if err != nil {
		return fmt.Errorf("failed to read file: %v", err)
	}

	// Basic BOM detection (UTF-8)
	if len(content) >= 3 && content[0] == 0xEF && content[1] == 0xBB && content[2] == 0xBF {
		d.HasBOM = true
		content = content[3:] // Strip BOM for internal buffer
	}

	// Verify valid UTF-8 (Geany defaults to UTF-8 internal representation)
	if !utf8.Valid(content) {
		// In a complete implementation, this is where we would invoke the `uchardet`
		// C library or a Go equivalent to detect ISO-8859-1, Windows-1252, etc.
		// For now, we fallback to marking it as raw bytes or failing gracefully.
		d.Encoding = "Unknown"
	} else {
		d.Encoding = "UTF-8"
	}

	// We are intentionally NOT storing `content` directly on the struct right now
	// because the actual text buffer is owned by the C++ Scintilla widget instance.
	// In the final architecture, this Open() method will pipe the `content` directly
	// into the UI Scintilla instance via the geany-go/scintilla package.

	d.Changed = false
	return nil
}

// Save writes the document back to disk.
func (d *Document) Save(content []byte) error {
	if d.ReadOnly {
		return fmt.Errorf("cannot save read-only document")
	}
	if d.FileName == "" {
		return fmt.Errorf("cannot save document with no filename")
	}

	// Re-apply BOM if the document originally had one
	if d.HasBOM && d.Encoding == "UTF-8" {
		bom := []byte{0xEF, 0xBB, 0xBF}
		content = append(bom, content...)
	}

	// Write to disk (0644 default perms)
	err := os.WriteFile(d.FileName, content, 0644)
	if err != nil {
		return fmt.Errorf("failed to write file: %v", err)
	}

	d.Changed = false
	return nil
}
