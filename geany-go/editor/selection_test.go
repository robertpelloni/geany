package editor

import (
	"testing"
)

func TestSelectionInit(t *testing.T) {
	s := NewSelection(10, 20)
	if s.Start != 10 || s.End != 20 {
		t.Errorf("Expected 10-20, got %d-%d", s.Start, s.End)
	}

	// Test swap
	s2 := NewSelection(30, 5)
	if s2.Start != 5 || s2.End != 30 {
		t.Errorf("Expected swap 5-30, got %d-%d", s2.Start, s2.End)
	}

	// Test negative clamps
	s3 := NewSelection(-5, 10)
	if s3.Start != 0 || s3.End != 10 {
		t.Errorf("Expected clamp 0-10, got %d-%d", s3.Start, s3.End)
	}
}

func TestSelectionSetRange(t *testing.T) {
	s := NewSelection(0, 0)

	s.SetRange(15, 25)
	if s.Start != 15 || s.End != 25 {
		t.Errorf("Expected 15-25, got %d-%d", s.Start, s.End)
	}

	s.SetRange(50, 40)
	if s.Start != 40 || s.End != 50 {
		t.Errorf("Expected 40-50, got %d-%d", s.Start, s.End)
	}
}

func TestSelectionLengthAndEmpty(t *testing.T) {
	s := NewSelection(10, 20)
	if s.Length() != 10 {
		t.Errorf("Expected length 10, got %d", s.Length())
	}
	if s.IsEmpty() {
		t.Errorf("Expected not empty")
	}

	s2 := NewSelection(15, 15)
	if s2.Length() != 0 {
		t.Errorf("Expected length 0, got %d", s2.Length())
	}
	if !s2.IsEmpty() {
		t.Errorf("Expected empty")
	}
}

func TestSelectionContains(t *testing.T) {
	s := NewSelection(10, 20)

	if s.Contains(9) {
		t.Errorf("9 should not be contained")
	}
	if !s.Contains(10) {
		t.Errorf("10 (start) should be contained")
	}
	if !s.Contains(15) {
		t.Errorf("15 should be contained")
	}
	if s.Contains(20) {
		t.Errorf("20 (end exclusive) should not be contained")
	}
	if s.Contains(21) {
		t.Errorf("21 should not be contained")
	}

	sEmpty := NewSelection(10, 10)
	if sEmpty.Contains(10) {
		t.Errorf("Empty selection should not contain anything")
	}
}
