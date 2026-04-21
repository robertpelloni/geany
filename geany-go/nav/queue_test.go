package nav

import "testing"

func TestQueueNavigation(t *testing.T) {
	q := NewQueue(5)

	// Test Initial State
	if q.CanGoBack() || q.CanGoForward() {
		t.Error("Empty queue should not allow navigation")
	}

	// Add P1
	q.Add(Position{DocumentID: 1, Line: 10})
	if q.CanGoBack() {
		t.Error("Queue with 1 item should not allow GoBack")
	}

	// Add P2
	q.Add(Position{DocumentID: 1, Line: 20})
	if !q.CanGoBack() {
		t.Error("Expected CanGoBack == true")
	}

	// Go Back to P1
	pos, err := q.GoBack()
	if err != nil || pos.Line != 10 {
		t.Errorf("GoBack failed, expected Line 10, got %d", pos.Line)
	}

	// Can Go Forward now
	if !q.CanGoForward() {
		t.Error("Expected CanGoForward == true after GoBack")
	}

	// Go Forward to P2
	pos, err = q.GoForward()
	if err != nil || pos.Line != 20 {
		t.Errorf("GoForward failed, expected Line 20, got %d", pos.Line)
	}
}

func TestQueueTruncation(t *testing.T) {
	q := NewQueue(5)

	q.Add(Position{Line: 1})
	q.Add(Position{Line: 2})
	q.Add(Position{Line: 3})

	// Go back to Line 2
	q.GoBack()

	// Add Line 4 (should truncate Line 3)
	q.Add(Position{Line: 4})

	if q.CanGoForward() {
		t.Error("Forward history should be truncated")
	}

	pos, _ := q.Current()
	if pos.Line != 4 {
		t.Errorf("Expected current line 4, got %d", pos.Line)
	}

	pos, _ = q.GoBack()
	if pos.Line != 2 {
		t.Errorf("Expected previous line to be 2, got %d", pos.Line)
	}
}

func TestQueueCapacity(t *testing.T) {
	q := NewQueue(2)

	q.Add(Position{Line: 1})
	q.Add(Position{Line: 2})
	q.Add(Position{Line: 3}) // Should push out Line 1

	q.GoBack()
	pos, _ := q.Current()
	if pos.Line != 2 {
		t.Errorf("Capacity breached, expected oldest line to be 2, got %d", pos.Line)
	}

	if q.CanGoBack() {
		t.Error("Should only hold 2 items, Cannot go back further than 2")
	}
}
