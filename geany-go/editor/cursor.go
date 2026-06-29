package editor

// Cursor handles caret positioning and navigation history.
type Cursor struct {
	Position int
    Anchor int
}

func NewCursor() *Cursor {
	return &Cursor{}
}

func (c *Cursor) HasSelection() bool {
    return c.Position != c.Anchor
}

func (c *Cursor) SetPosition(pos int) {
    if pos < 0 {
        pos = 0
    }
    c.Position = pos
    c.Anchor = pos
}

func (c *Cursor) MoveTo(pos int) {
    if pos < 0 {
        pos = 0
    }
    c.Position = pos
}

func (c *Cursor) Selection() (int, int) {
    if c.Position < c.Anchor {
        return c.Position, c.Anchor
    }
    return c.Anchor, c.Position
}

func (c *Cursor) ClearSelection() {
    c.Anchor = c.Position
}

func (c *Cursor) SelectAll(docLength int) {
    if docLength < 0 {
        docLength = 0
    }
    c.Anchor = 0
    c.Position = docLength
}

func (d *Document) GetCursorPosition() int {
	if d.Backend != nil {
		return d.Backend.GetCursorPos()
	}
	return 0
}

func (d *Document) SetCursorPosition(pos int) {
	if d.Backend != nil {
		d.Backend.SetCursorPos(pos)
	}
}

func (d *Document) GetSelection() Selection {
	if d.Backend != nil {
		start, end := d.Backend.GetSelection()
		return Selection{Start: start, End: end}
	}
	return Selection{0, 0}
}

func (d *Document) SetSelection(sel Selection) {
	if d.Backend != nil {
		d.Backend.SetSelection(sel.Start, sel.End)
	}
}
