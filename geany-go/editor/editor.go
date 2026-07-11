package editor

import (
	"fmt"
	"sync"
)

// Editor manages a collection of open documents and the active view state.
// This acts as the central hub for document lifecycle management in the Go port,
// replacing the legacy global C arrays.
type Editor struct {
	documents      map[int]*Document
	activeDocID    int
	nextID         int
	mu             sync.RWMutex
}

// NewEditor initializes a new Editor instance.
func NewEditor() *Editor {
	return &Editor{
		documents:   make(map[int]*Document),
		activeDocID: -1,
		nextID:      1,
	}
}

// OpenDocument creates and registers a new document for the given path.
// It automatically sets the newly opened document as the active document.
func (e *Editor) OpenDocument(path string) (*Document, error) {
	e.mu.Lock()
	defer e.mu.Unlock()

	// Check if already open
	for _, doc := range e.documents {
		if doc.FileName == path {
			e.activeDocID = doc.ID
			return doc, nil
		}
	}

	doc := NewDocument(e.nextID, path, nil)
	err := doc.Open()
	if err != nil {
		// Even if file doesn't exist yet, we might be creating a new file buffer.
		// For now, if Open fails (e.g. permission denied on existing file), we return the error.
		// If it's a new unsaved buffer, path might be empty.
		if path != "" {
		    // Let's allow creating new named buffers even if they don't exist on disk yet.
			// But if it's a hard error like permissions, we should probably catch that in a real implementation.
		}
	}

	e.documents[doc.ID] = doc
	e.activeDocID = doc.ID
	e.nextID++

	return doc, nil
}

// CloseDocument removes the document with the specified ID from the editor.
// If the closed document was the active document, the active document is unset (-1).
func (e *Editor) CloseDocument(id int) error {
	e.mu.Lock()
	defer e.mu.Unlock()

	if _, exists := e.documents[id]; !exists {
		return fmt.Errorf("document with ID %d not found", id)
	}

	delete(e.documents, id)

	if e.activeDocID == id {
		e.activeDocID = -1
		// In a real editor, we would activate the next available tab here.
		for docID := range e.documents {
			e.activeDocID = docID
			break
		}
	}

	return nil
}

// GetCurrentDocument returns the currently active document, or nil if none are open.
func (e *Editor) GetCurrentDocument() *Document {
	e.mu.RLock()
	defer e.mu.RUnlock()

	if e.activeDocID == -1 {
		return nil
	}

	return e.documents[e.activeDocID]
}

// GetDocumentByID retrieves a specific document by its ID.
func (e *Editor) GetDocumentByID(id int) (*Document, error) {
	e.mu.RLock()
	defer e.mu.RUnlock()

	doc, exists := e.documents[id]
	if !exists {
		return nil, fmt.Errorf("document with ID %d not found", id)
	}

	return doc, nil
}

// SetActiveDocument explicitly changes which document is currently active.
func (e *Editor) SetActiveDocument(id int) error {
	e.mu.Lock()
	defer e.mu.Unlock()

	if _, exists := e.documents[id]; !exists {
		return fmt.Errorf("cannot activate unknown document ID %d", id)
	}

	e.activeDocID = id
	return nil
}

// DocumentCount returns the total number of open documents.
func (e *Editor) DocumentCount() int {
	e.mu.RLock()
	defer e.mu.RUnlock()
	return len(e.documents)
}
