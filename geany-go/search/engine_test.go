package search

import (
	"testing"
)

func TestPlainSearch(t *testing.T) {
	engine := NewEngine()
	text := "The quick brown fox jumps over the lazy dog."

	// Basic found
	res, err := engine.FindNext(text, "quick", 0, SearchFlags{MatchCase: true})
	if err != nil {
		t.Fatalf("Unexpected error: %v", err)
	}
	if res == nil || res.Start != 4 || res.End != 9 {
		t.Errorf("Expected match at 4-9, got %v", res)
	}

	// Basic not found
	res, err = engine.FindNext(text, "cat", 0, SearchFlags{MatchCase: true})
	if err != nil {
		t.Fatalf("Unexpected error: %v", err)
	}
	if res != nil {
		t.Errorf("Expected no match, got %v", res)
	}

	// Case insensitive
	res, err = engine.FindNext(text, "THE", 0, SearchFlags{MatchCase: false})
	if err != nil {
		t.Fatalf("Unexpected error: %v", err)
	}
	if res == nil || res.Start != 0 || res.End != 3 {
		t.Errorf("Expected match at 0-3, got %v", res)
	}
}

func TestRegexSearch(t *testing.T) {
	engine := NewEngine()
	text := "The year is 2026 and AI is booming."

	res, err := engine.FindNext(text, `\d{4}`, 0, SearchFlags{Regex: true})
	if err != nil {
		t.Fatalf("Unexpected error: %v", err)
	}
	if res == nil || res.Text != "2026" {
		t.Errorf("Expected match '2026', got %v", res)
	}
}
