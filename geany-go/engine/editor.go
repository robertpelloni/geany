package engine

import "fmt"

// Position identifies a specific character index and line in the document.
type Position struct {
	Line   int
	Column int
	Index  int // Absolute byte offset in the buffer
}

// Range defines a span of text within the document.
type Range struct {
	Start Position
	End   Position
}

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

// Document manages the high-level state (file path, undo/redo, modifications)
// and delegates raw text operations to the underlying Backend.
type Document struct {
	ID       int
	FilePath string
	Modified bool
	Backend  Backend
}

// NewDocument initializes a new high-level editor document bound to a specific UI backend.
func NewDocument(id int, path string, backend Backend) *Document {
	return &Document{
		ID:       id,
		FilePath: path,
		Modified: false,
		Backend:  backend,
	}
}

// ReplaceText is a robust operation that handles modifying a specific range,
// tracking the modification state, and updating the backend.
func (d *Document) ReplaceText(start, end int, replacement string) error {
	if d.Backend.IsReadOnly() {
		return fmt.Errorf("document is read-only")
	}

	if start > end {
		start, end = end, start // Ensure proper ordering
	}

	d.Backend.DeleteRange(start, end)
	d.Backend.InsertText(start, replacement)
	d.Modified = true
	return nil
}

// SelectAll highlights the entire document buffer.
func (d *Document) SelectAll() {
	textLen := len(d.Backend.GetText())
	d.Backend.SetSelection(0, textLen)
}

// -----------------------------------------------------------------------------
// Notepad++ Parity: Line Operations
// -----------------------------------------------------------------------------

// DeleteLine removes the text of an entire line (including the newline character).
func (d *Document) DeleteLine(lineIndex int) error {
	if d.Backend.IsReadOnly() {
		return fmt.Errorf("document is read-only")
	}

	lineCount := d.Backend.GetLineCount()
	if lineIndex < 0 || lineIndex >= lineCount {
		return fmt.Errorf("line index out of bounds")
	}

	// This assumes the backend has logic to map line index to absolute byte offset.
	// For this parity bootstrap, we mock the calculation by summing previous line lengths.
	startOffset := 0
	for i := 0; i < lineIndex; i++ {
		startOffset += d.Backend.GetLineLength(i)
	}

	length := d.Backend.GetLineLength(lineIndex)
	d.Backend.DeleteRange(startOffset, startOffset+length)
	d.Modified = true
	return nil
}

// DuplicateLine duplicates the line at the current cursor position.
func (d *Document) DuplicateLine(lineIndex int) error {
	if d.Backend.IsReadOnly() {
		return fmt.Errorf("document is read-only")
	}

	lineCount := d.Backend.GetLineCount()
	if lineIndex < 0 || lineIndex >= lineCount {
		return fmt.Errorf("line index out of bounds")
	}

	startOffset := 0
	for i := 0; i < lineIndex; i++ {
		startOffset += d.Backend.GetLineLength(i)
	}

	length := d.Backend.GetLineLength(lineIndex)
	lineText := d.Backend.GetTextRange(startOffset, startOffset+length)

	// Insert duplicate directly after the current line
	d.Backend.InsertText(startOffset+length, lineText)
	d.Modified = true
	return nil
}
