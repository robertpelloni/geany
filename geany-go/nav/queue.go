package nav

import (
	"fmt"
	"sync"
)

// Position represents a specific cursor location in a specific document.
type Position struct {
	DocumentID int
	Line       int
	Column     int
}

// Queue handles the "Navigate Back" and "Navigate Forward" functionality of the IDE.
// This is a direct, thread-safe port of the linked list logic found in src/navqueue.c.
type Queue struct {
	mu           sync.RWMutex
	positions    []Position
	currentIndex int // The current position in history
	maxSize      int
}

// NewQueue creates a new navigation queue capped at the given maximum history size.
func NewQueue(maxSize int) *Queue {
	if maxSize <= 0 {
		maxSize = 100 // Geany default
	}
	return &Queue{
		positions:    make([]Position, 0),
		currentIndex: -1,
		maxSize:      maxSize,
	}
}

// Add records a new position in the navigation history.
// If the user has navigated back and then adds a new position, the forward history is truncated.
func (q *Queue) Add(pos Position) {
	q.mu.Lock()
	defer q.mu.Unlock()

	// If we are not at the end of the queue, truncate the future
	if q.currentIndex < len(q.positions)-1 && len(q.positions) > 0 {
		q.positions = q.positions[:q.currentIndex+1]
	}

	// Avoid adding duplicate consecutive positions
	if len(q.positions) > 0 {
		lastPos := q.positions[len(q.positions)-1]
		if lastPos.DocumentID == pos.DocumentID && lastPos.Line == pos.Line && lastPos.Column == pos.Column {
			return // Ignore duplicate
		}
	}

	q.positions = append(q.positions, pos)

	// Enforce maximum history size
	if len(q.positions) > q.maxSize {
		q.positions = q.positions[1:]
	} else {
		q.currentIndex++
	}
}

// CanGoBack returns whether there is a previous position in history.
func (q *Queue) CanGoBack() bool {
	q.mu.RLock()
	defer q.mu.RUnlock()
	return q.currentIndex > 0
}

// GoBack steps backward in the history and returns the previous position.
func (q *Queue) GoBack() (Position, error) {
	q.mu.Lock()
	defer q.mu.Unlock()

	if q.currentIndex <= 0 {
		return Position{}, fmt.Errorf("no history to navigate back to")
	}

	q.currentIndex--
	return q.positions[q.currentIndex], nil
}

// CanGoForward returns whether there is a forward position in history (user has previously gone back).
func (q *Queue) CanGoForward() bool {
	q.mu.RLock()
	defer q.mu.RUnlock()
	return q.currentIndex < len(q.positions)-1
}

// GoForward steps forward in the history and returns the next position.
func (q *Queue) GoForward() (Position, error) {
	q.mu.Lock()
	defer q.mu.Unlock()

	if q.currentIndex >= len(q.positions)-1 {
		return Position{}, fmt.Errorf("no history to navigate forward to")
	}

	q.currentIndex++
	return q.positions[q.currentIndex], nil
}

// Current returns the current position in the queue without navigating.
func (q *Queue) Current() (Position, error) {
	q.mu.RLock()
	defer q.mu.RUnlock()

	if q.currentIndex < 0 || q.currentIndex >= len(q.positions) {
		return Position{}, fmt.Errorf("queue is empty")
	}

	return q.positions[q.currentIndex], nil
}

// Clear resets the entire navigation history.
func (q *Queue) Clear() {
	q.mu.Lock()
	defer q.mu.Unlock()
	q.positions = make([]Position, 0)
	q.currentIndex = -1
}
