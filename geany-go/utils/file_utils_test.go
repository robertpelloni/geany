package utils

import (
	"os"
	"testing"
)

func TestFileExists(t *testing.T) {
	// Create temp file
	f, err := os.CreateTemp("", "geany_test_")
	if err != nil {
		t.Fatal(err)
	}
	defer os.Remove(f.Name())
	f.Close()

	if !FileExists(f.Name()) {
		t.Errorf("Expected true for existing file %s", f.Name())
	}
	if FileExists("/does/not/exist/ever") {
		t.Error("Expected false for non-existing file")
	}
}

func TestDirExists(t *testing.T) {
	if !DirExists(os.TempDir()) {
		t.Errorf("Expected true for temp dir %s", os.TempDir())
	}
}
