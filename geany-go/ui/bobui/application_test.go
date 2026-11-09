package bobui

import (
	"testing"

	"github.com/geany/geany-go/editor"
)

func TestBobuiApplicationLifecycle(t *testing.T) {
	app := NewApp()
	if err := app.Init([]string{"--test"}); err != nil {
		t.Fatalf("Failed to initialize bobui app: %v", err)
	}

	win := app.MainWindow()
	if win == nil {
		t.Fatal("MainWindow should not be nil")
	}

	win.SetTitle("Geany Ultra Qt6 Test Window")
	win.Show()
	win.Hide()

	doc := &editor.Document{
		ID:       1,
		BaseName: "test_qt.cpp",
	}

	widget := win.AddDocument(doc)
	if widget == nil {
		t.Fatal("AddDocument returned nil widget")
	}

	if active := win.GetActiveDocument(); active == nil || active.ID != doc.ID {
		t.Errorf("Expected active document ID %d, got %v", doc.ID, active)
	}

	widget.SetText("int main() { return 0; }")
	if widget.GetText() != "int main() { return 0; }" {
		t.Errorf("GetText mismatch: %s", widget.GetText())
	}

	widget.AppendText("\n// comment")
	if widget.GetLineCount() < 1 {
		t.Errorf("Invalid line count: %d", widget.GetLineCount())
	}

	widget.EnableMultiLineTabs(true)
	widget.ShowDocumentMap(true)
	widget.ClearAll()
	if widget.GetText() != "" {
		t.Errorf("Expected empty text after ClearAll")
	}

	menuBar := win.CreateMenuBar()
	if menuBar == nil {
		t.Fatal("CreateMenuBar returned nil")
	}
	menu := menuBar.AddMenu("File")
	menu.AddItem("Save", "Ctrl+S", func() {})
	menu.AddSeparator()

	toolBar := win.CreateToolBar()
	if toolBar == nil {
		t.Fatal("CreateToolBar returned nil")
	}
	toolBar.AddButton("save-icon", "Save Document", func() {})

	if ret := app.Run(); ret != 0 {
		t.Errorf("Run() returned %d", ret)
	}
	app.Quit()
}
