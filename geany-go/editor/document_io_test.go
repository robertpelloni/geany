package editor

import (
	"os"
	"path/filepath"
	"testing"
)

func TestDocumentIO(t *testing.T) {
	tempDir := t.TempDir()
	testPath := filepath.Join(tempDir, "test_io.txt")

	// 1. Test Save
	doc := NewDocument(1, testPath)
	content := []byte("Hello Go Document I/O")
	err := doc.Save(content)
	if err != nil {
		t.Fatalf("Save failed: %v", err)
	}

	// Verify file on disk
	diskContent, _ := os.ReadFile(testPath)
	if string(diskContent) != "Hello Go Document I/O" {
		t.Errorf("Disk content mismatch, got: %s", string(diskContent))
	}

	// 2. Test Open
	doc2 := NewDocument(2, testPath)
	err = doc2.Open()
	if err != nil {
		t.Fatalf("Open failed: %v", err)
	}

	if doc2.Encoding != "UTF-8" {
		t.Errorf("Expected UTF-8 encoding, got %s", doc2.Encoding)
	}
	if doc2.HasBOM {
		t.Errorf("Expected no BOM")
	}

	// 3. Test BOM Handling
	bomPath := filepath.Join(tempDir, "bom.txt")
	os.WriteFile(bomPath, []byte{0xEF, 0xBB, 0xBF, 'A', 'B', 'C'}, 0644)

	doc3 := NewDocument(3, bomPath)
	doc3.Open()

	if !doc3.HasBOM {
		t.Errorf("Expected BOM to be detected")
	}

	// Save should preserve BOM
	doc3.Save([]byte{'X', 'Y', 'Z'})
	diskBOM, _ := os.ReadFile(bomPath)
	if len(diskBOM) != 6 || diskBOM[0] != 0xEF {
		t.Errorf("BOM was not preserved on save, got %v", diskBOM)
	}
}

func TestDocumentIOErrors(t *testing.T) {
	doc := NewDocument(1, "")
	err := doc.Open()
	if err == nil {
		t.Errorf("Expected error opening empty path")
	}

	err = doc.Save([]byte("test"))
	if err == nil {
		t.Errorf("Expected error saving empty path")
	}

	tempDir := t.TempDir()
	roPath := filepath.Join(tempDir, "ro.txt")
	os.WriteFile(roPath, []byte("ro"), 0444) // Read-only

	docRO := NewDocument(2, roPath)
	docRO.Open()
	if !docRO.ReadOnly {
		t.Errorf("Expected document to be marked read-only")
	}

	err = docRO.Save([]byte("test"))
	if err == nil {
		t.Errorf("Expected error saving read-only document")
	}
}
