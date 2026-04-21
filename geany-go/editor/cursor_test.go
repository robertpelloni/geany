package editor

import (
	"testing"
)

func TestCursorInitialization(t *testing.T) {
	c := NewCursor()

	if c.Position != 0 {
		t.Errorf("Expected initial Position to be 0, got %d", c.Position)
	}

	if c.Anchor != 0 {
		t.Errorf("Expected initial Anchor to be 0, got %d", c.Anchor)
	}

	if c.HasSelection() {
		t.Errorf("Expected initial Cursor to have no selection")
	}
}

func TestCursorSetPosition(t *testing.T) {
	c := NewCursor()

	c.SetPosition(10)
	if c.Position != 10 || c.Anchor != 10 {
		t.Errorf("SetPosition failed, expected 10/10, got %d/%d", c.Position, c.Anchor)
	}

	if c.HasSelection() {
		t.Errorf("SetPosition should clear selection")
	}

	// Test negative bounds checking
	c.SetPosition(-5)
	if c.Position != 0 || c.Anchor != 0 {
		t.Errorf("SetPosition bounds checking failed, expected 0/0, got %d/%d", c.Position, c.Anchor)
	}
}

func TestCursorMovementAndSelection(t *testing.T) {
	c := NewCursor()

	// Start a selection
	c.SetPosition(5)
	c.MoveTo(15)

	if c.Position != 15 {
		t.Errorf("MoveTo failed, expected Position 15, got %d", c.Position)
	}

	if c.Anchor != 5 {
		t.Errorf("MoveTo should not modify Anchor, expected 5, got %d", c.Anchor)
	}

	if !c.HasSelection() {
		t.Errorf("Expected cursor to have selection")
	}

	start, end := c.Selection()
	if start != 5 || end != 15 {
		t.Errorf("Selection bounds incorrect, expected 5-15, got %d-%d", start, end)
	}

	// Move backwards to test inverted selection
	c.MoveTo(2)

	if c.Anchor != 5 || c.Position != 2 {
		t.Errorf("Backward MoveTo failed, expected anchor 5, pos 2, got %d/%d", c.Anchor, c.Position)
	}

	start, end = c.Selection()
	if start != 2 || end != 5 {
		t.Errorf("Inverted selection bounds incorrect, expected 2-5, got %d-%d", start, end)
	}

	// Clear selection
	c.ClearSelection()
	if c.Anchor != 2 || c.Position != 2 || c.HasSelection() {
		t.Errorf("ClearSelection failed, expected 2/2 and no selection, got %d/%d, has=%v", c.Anchor, c.Position, c.HasSelection())
	}
}

func TestCursorSelectAll(t *testing.T) {
	c := NewCursor()

	c.SetPosition(50) // Random position

	docLength := 100
	c.SelectAll(docLength)

	if c.Anchor != 0 || c.Position != docLength {
		t.Errorf("SelectAll failed, expected 0/%d, got %d/%d", docLength, c.Anchor, c.Position)
	}

	start, end := c.Selection()
	if start != 0 || end != docLength {
		t.Errorf("SelectAll bounds incorrect, expected 0-%d, got %d-%d", docLength, start, end)
	}

	// Test negative length
	c.SelectAll(-10)
	if c.Position != 0 {
		t.Errorf("SelectAll bounds checking failed, expected 0, got %d", c.Position)
	}
}
