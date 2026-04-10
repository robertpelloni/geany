package utils

import (
	"testing"
)

func TestIsEmpty(t *testing.T) {
	if !IsEmpty("") {
		t.Error("Expected true for empty string")
	}
	if !IsEmpty("   \t\n") {
		t.Error("Expected true for whitespace string")
	}
	if IsEmpty("text") {
		t.Error("Expected false for non-empty string")
	}
}

func TestStrReplace(t *testing.T) {
	res := StrReplace("hello world", "world", "geany")
	if res != "hello geany" {
		t.Errorf("Expected 'hello geany', got '%s'", res)
	}
}

func TestCapitalize(t *testing.T) {
	if res := Capitalize("gEaNy"); res != "Geany" {
		t.Errorf("Expected 'Geany', got '%s'", res)
	}
}
