package engine

import "testing"

// MockBackend simulates a text editor widget (like Scintilla) in memory for unit testing.
type MockBackend struct {
	buffer   string
	readOnly bool
	cursor   int
	selStart int
	selEnd   int
}

func (m *MockBackend) GetText() string { return m.buffer }
func (m *MockBackend) SetText(text string) { m.buffer = text }
func (m *MockBackend) GetTextRange(start, end int) string {
	if start < 0 || end > len(m.buffer) || start > end {
		return ""
	}
	return m.buffer[start:end]
}
func (m *MockBackend) InsertText(pos int, text string) {
	if pos < 0 || pos > len(m.buffer) { return }
	m.buffer = m.buffer[:pos] + text + m.buffer[pos:]
}
func (m *MockBackend) DeleteRange(start, end int) {
	if start < 0 || end > len(m.buffer) || start > end { return }
	m.buffer = m.buffer[:start] + m.buffer[end:]
}
func (m *MockBackend) GetLineCount() int { return 1 } // Simplified stub
func (m *MockBackend) GetLineLength(line int) int { return len(m.buffer) }
func (m *MockBackend) SetReadOnly(ro bool) { m.readOnly = ro }
func (m *MockBackend) IsReadOnly() bool { return m.readOnly }
func (m *MockBackend) GetCursorPos() int { return m.cursor }
func (m *MockBackend) SetCursorPos(pos int) { m.cursor = pos }
func (m *MockBackend) GetSelection() (int, int) { return m.selStart, m.selEnd }
func (m *MockBackend) SetSelection(start, end int) {
	m.selStart = start
	m.selEnd = end
}

func TestDocumentReplaceText(t *testing.T) {
	backend := &MockBackend{buffer: "Hello World"}
	doc := NewDocument(1, "/test.txt", backend)

	// Replace "World" with "Geany"
	err := doc.ReplaceText(6, 11, "Geany")
	if err != nil {
		t.Fatalf("Unexpected error: %v", err)
	}

	if backend.buffer != "Hello Geany" {
		t.Errorf("Expected 'Hello Geany', got '%s'", backend.buffer)
	}
	if !doc.Modified {
		t.Error("Document should be marked modified")
	}

	// Test ReadOnly enforcement
	backend.SetReadOnly(true)
	err = doc.ReplaceText(0, 5, "Hi")
	if err == nil {
		t.Error("Expected error when modifying read-only document")
	}
	if backend.buffer != "Hello Geany" {
		t.Errorf("Buffer should not have changed, got '%s'", backend.buffer)
	}
}

func TestDocumentSelectAll(t *testing.T) {
	backend := &MockBackend{buffer: "12345"}
	doc := NewDocument(1, "", backend)

	doc.SelectAll()
	s, e := backend.GetSelection()
	if s != 0 || e != 5 {
		t.Errorf("Expected selection (0, 5), got (%d, %d)", s, e)
	}
}
