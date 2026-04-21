package editor

// Cursor represents a single cursor or caret within a text document.
// It tracks the current insertion position and the selection anchor.
// This is the foundation for text navigation and manipulation.
type Cursor struct {
	// Position is the current byte offset of the cursor.
	Position int

	// Anchor is the byte offset where the current selection started.
	// If Anchor == Position, there is no selection.
	Anchor int
}

// NewCursor initializes a new Cursor at the beginning of the document.
func NewCursor() *Cursor {
	return &Cursor{
		Position: 0,
		Anchor:   0,
	}
}

// SetPosition moves the cursor to the specified offset and clears any selection.
func (c *Cursor) SetPosition(offset int) {
	if offset < 0 {
		offset = 0
	}
	c.Position = offset
	c.Anchor = offset
}

// MoveTo moves the cursor to the specified offset, extending the selection
// from the original Anchor to the new Position.
func (c *Cursor) MoveTo(offset int) {
	if offset < 0 {
		offset = 0
	}
	c.Position = offset
}

// ClearSelection clears the current selection by setting the Anchor to the current Position.
func (c *Cursor) ClearSelection() {
	c.Anchor = c.Position
}

// HasSelection returns true if the cursor has an active selection
// (i.e., the Anchor is not equal to the Position).
func (c *Cursor) HasSelection() bool {
	return c.Position != c.Anchor
}

// Selection returns the start and end offsets of the current selection.
// The start is always guaranteed to be less than or equal to the end,
// regardless of the direction the user selected the text.
func (c *Cursor) Selection() (start, end int) {
	if c.Position < c.Anchor {
		return c.Position, c.Anchor
	}
	return c.Anchor, c.Position
}

// SelectAll is a utility method to select the entire document from 0 to the specified length.
func (c *Cursor) SelectAll(documentLength int) {
	if documentLength < 0 {
		documentLength = 0
	}
	c.Anchor = 0
	c.Position = documentLength
}
