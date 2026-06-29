package ui

import (
	"testing"
	"github.com/geany/geany-go/editor"
)

// MockWindow is a simple mock implementation of the Window interface for unit testing core logic.
type MockWindow struct {
	Title string
	Docs  []*editor.Document
}

func (m *MockWindow) SetTitle(title string) { m.Title = title }
func (m *MockWindow) Show()                 {}
func (m *MockWindow) Hide()                 {}
func (m *MockWindow) AddDocument(doc *editor.Document) EditorWidget {
	m.Docs = append(m.Docs, doc)
	return &MockEditorWidget{Doc: doc}
}
func (m *MockWindow) GetActiveDocument() *editor.Document {
	if len(m.Docs) > 0 {
		return m.Docs[0]
	}
	return nil
}
func (m *MockWindow) CreateMenuBar() MenuBar { return nil }
func (m *MockWindow) CreateToolBar() Toolbar { return nil }

type MockEditorWidget struct {
	Doc *editor.Document
}

func (m *MockEditorWidget) SetText(text string)          {}
func (m *MockEditorWidget) GetText() string              { return "" }
func (m *MockEditorWidget) AppendText(text string)       {}
func (m *MockEditorWidget) ClearAll()                    {}
func (m *MockEditorWidget) SetReadOnly(readOnly bool)    {}
func (m *MockEditorWidget) GetLineCount() int            { return 1 }
func (m *MockEditorWidget) EnableMultiLineTabs(e bool)   {}
func (m *MockEditorWidget) ShowDocumentMap(s bool)       {}

func TestUIAbstraction(t *testing.T) {
	// Verify that the MockWindow properly implements the interface and stores state
	var window Window = &MockWindow{}
	window.SetTitle("Geany Go Mock")

	doc := editor.NewDocument(1, "/mock/test.txt", nil)
	widget := window.AddDocument(doc)

	if window.(*MockWindow).Title != "Geany Go Mock" {
		t.Errorf("Mock window title not set correctly")
	}

	activeDoc := window.GetActiveDocument()
	if activeDoc == nil || activeDoc.FileName != "/mock/test.txt" {
		t.Errorf("Failed to retrieve active document from mock window")
	}

	if widget.GetLineCount() != 1 {
		t.Errorf("Mock widget line count incorrect")
	}
}
