package editor

// Selection represents a contiguous range of text within a document.
// It is defined by a Start and End byte offset, where Start is always <= End.
// This is used for extracting, deleting, or formatting text chunks, separate from
// the directional nature of the Cursor (Anchor/Position).
type Selection struct {
	Start int
	End   int
}

// NewSelection creates a new Selection representing the range [start, end].
// If start > end, the values are automatically swapped to guarantee Start <= End.
// Negative offsets are clamped to 0.
func NewSelection(start, end int) *Selection {
	if start < 0 {
		start = 0
	}
	if end < 0 {
		end = 0
	}

	if start > end {
		return &Selection{Start: end, End: start}
	}
	return &Selection{Start: start, End: end}
}

// SetRange updates the boundaries of the selection.
// If start > end, they are automatically swapped.
// Negative offsets are clamped to 0.
func (s *Selection) SetRange(start, end int) {
	if start < 0 {
		start = 0
	}
	if end < 0 {
		end = 0
	}

	if start > end {
		s.Start = end
		s.End = start
	} else {
		s.Start = start
		s.End = end
	}
}

// Length returns the total number of bytes in the selection.
func (s *Selection) Length() int {
	return s.End - s.Start
}

// IsEmpty returns true if the selection covers 0 bytes (Start == End).
func (s *Selection) IsEmpty() bool {
	return s.Start == s.End
}

// Contains returns true if the given offset falls strictly within the selection bounds [Start, End).
// If the selection is empty, Contains always returns false.
func (s *Selection) Contains(offset int) bool {
	if s.IsEmpty() {
		return false
	}
	return offset >= s.Start && offset < s.End
}
