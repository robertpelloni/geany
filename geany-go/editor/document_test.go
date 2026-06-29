package editor

import (
	"os"
	"testing"
)

func TestNewDocument(t *testing.T) {
	doc := NewDocument(1, "/tmp/test.txt", nil)

	if doc.ID != 1 {
		t.Errorf("Expected ID 1, got %d", doc.ID)
	}
	if doc.FileName != "/tmp/test.txt" {
		t.Errorf("Expected FileName /tmp/test.txt, got %s", doc.FileName)
	}
	if doc.BaseName != "test.txt" {
		t.Errorf("Expected BaseName test.txt, got %s", doc.BaseName)
	}
	if doc.Changed {
		t.Error("Expected Changed to be false")
	}
}

func TestDocumentOpenAndSave(t *testing.T) {
	// Create a temp file
	f, err := os.CreateTemp("", "geany_go_test_")
	if err != nil {
		t.Fatal(err)
	}
	defer os.Remove(f.Name())
	f.Close()

	doc := NewDocument(2, f.Name(), nil)

	err = doc.Open()
	if err != nil {
		t.Errorf("Failed to open document: %v", err)
	}

	doc.SetModified(true)
	if !doc.Changed {
		t.Error("Expected Changed to be true after SetModified")
	}

	err = doc.Save([]byte(""))
	if err != nil {
		t.Errorf("Failed to save document: %v", err)
	}

	if doc.Changed {
		t.Error("Expected Changed to be false after Save")
	}
}
