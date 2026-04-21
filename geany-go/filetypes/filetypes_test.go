package filetypes

import "testing"

func TestDetectTypeByExtension(t *testing.T) {
	mgr := NewManager()

	tests := []struct {
		filename string
		expected string // ID
	}{
		{"main.cpp", "c"},
		{"utils.h", "c"},
		{"script.py", "python"},
		{"server.go", "go"},
		{"README.txt", "none"},
		{"unknown.xyz", "none"},
	}

	for _, tt := range tests {
		ft := mgr.DetectType(tt.filename)
		if ft.ID != tt.expected {
			t.Errorf("For %s, expected %s, got %s", tt.filename, tt.expected, ft.ID)
		}
	}
}

func TestDetectTypeByExactName(t *testing.T) {
	mgr := NewManager()

	ft := mgr.DetectType("/usr/src/Makefile")
	if ft.ID != "make" {
		t.Errorf("Expected 'make' for Makefile, got '%s'", ft.ID)
	}
}

func TestGetByID(t *testing.T) {
	mgr := NewManager()

	ft := mgr.GetByID("python")
	if ft == nil || ft.Name != "Python" {
		t.Errorf("Failed to retrieve Python filetype by ID")
	}

	ftNone := mgr.GetByID("invalid_id")
	if ftNone.ID != "none" {
		t.Errorf("Expected fallback to 'none' for invalid ID")
	}
}
