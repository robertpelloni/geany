package editor

import (
	"os"
	"path/filepath"
	"testing"
)

func TestEditorLifecycle(t *testing.T) {
	ed := NewEditor()

	if ed.DocumentCount() != 0 {
		t.Errorf("Expected 0 documents initially, got %d", ed.DocumentCount())
	}
	if ed.GetCurrentDocument() != nil {
		t.Errorf("Expected nil active document initially")
	}

	// Create a dummy file for testing OpenDocument
	tempDir := t.TempDir()
	testPath := filepath.Join(tempDir, "test.txt")
	os.WriteFile(testPath, []byte("test content"), 0644)

	// Test OpenDocument
	doc1, err := ed.OpenDocument(testPath)
	if err != nil {
		t.Fatalf("Failed to open document: %v", err)
	}

	if ed.DocumentCount() != 1 {
		t.Errorf("Expected 1 document, got %d", ed.DocumentCount())
	}

	active := ed.GetCurrentDocument()
	if active == nil || active.ID != doc1.ID {
		t.Errorf("Expected active document to be doc1")
	}

	// Test Opening the same document again (should return existing)
	doc1Dup, err := ed.OpenDocument(testPath)
	if err != nil {
		t.Fatalf("Failed to open document again: %v", err)
	}
	if doc1Dup.ID != doc1.ID {
		t.Errorf("Expected duplicate open to return same document ID")
	}
	if ed.DocumentCount() != 1 {
		t.Errorf("Expected count to remain 1, got %d", ed.DocumentCount())
	}

	// Test Open second document
	testPath2 := filepath.Join(tempDir, "test2.txt")
	os.WriteFile(testPath2, []byte("test content 2"), 0644)
	doc2, err := ed.OpenDocument(testPath2)
	if err != nil {
		t.Fatalf("Failed to open second document: %v", err)
	}

	if ed.DocumentCount() != 2 {
		t.Errorf("Expected 2 documents, got %d", ed.DocumentCount())
	}

	// newly opened document should be active
	active = ed.GetCurrentDocument()
	if active == nil || active.ID != doc2.ID {
		t.Errorf("Expected active document to be doc2")
	}

	// Test SetActiveDocument
	err = ed.SetActiveDocument(doc1.ID)
	if err != nil {
		t.Errorf("SetActiveDocument failed: %v", err)
	}
	if ed.GetCurrentDocument().ID != doc1.ID {
		t.Errorf("Expected active document to be doc1 after setting")
	}

	// Test CloseDocument
	err = ed.CloseDocument(doc1.ID)
	if err != nil {
		t.Errorf("CloseDocument failed: %v", err)
	}
	if ed.DocumentCount() != 1 {
		t.Errorf("Expected 1 document after close, got %d", ed.DocumentCount())
	}

	// After closing active, it should fallback to another open document
	active = ed.GetCurrentDocument()
	if active == nil || active.ID != doc2.ID {
		t.Errorf("Expected fallback active document to be doc2")
	}

	// Close last document
	ed.CloseDocument(doc2.ID)
	if ed.DocumentCount() != 0 {
		t.Errorf("Expected 0 documents after all closed, got %d", ed.DocumentCount())
	}
	if ed.GetCurrentDocument() != nil {
		t.Errorf("Expected nil active document when empty")
	}
}

func TestEditorErrors(t *testing.T) {
	ed := NewEditor()

	err := ed.CloseDocument(999)
	if err == nil {
		t.Errorf("Expected error closing non-existent document")
	}

	err = ed.SetActiveDocument(999)
	if err == nil {
		t.Errorf("Expected error setting active non-existent document")
	}

	_, err = ed.GetDocumentByID(999)
	if err == nil {
		t.Errorf("Expected error getting non-existent document")
	}
}
