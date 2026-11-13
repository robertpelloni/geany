package btk

import (
	"strings"
	"testing"

	"github.com/geany/geany-go/editor"
	"github.com/geany/geany-go/ui"
)

func TestBtkApplicationLifecycle(t *testing.T) {
	app := NewApp()
	if err := app.Init([]string{"--test"}); err != nil {
		t.Fatalf("Failed to initialize btk app: %v", err)
	}

	win := app.MainWindow()
	if win == nil {
		t.Fatal("MainWindow should not be nil")
	}

	win.SetTitle("Geany Ultra Qt4/CopperSpice Test Window")
	win.Show()
	win.Hide()

	doc := &editor.Document{
		ID:       2,
		BaseName: "test_btk.c",
	}

	widget := win.AddDocument(doc)
	if widget == nil {
		t.Fatal("AddDocument returned nil widget")
	}

	if active := win.GetActiveDocument(); active == nil || active.ID != doc.ID {
		t.Errorf("Expected active document ID %d, got %v", doc.ID, active)
	}

	widget.SetText("#include <stdio.h>")
	if widget.GetText() != "#include <stdio.h>" {
		t.Errorf("GetText mismatch: %s", widget.GetText())
	}

	widget.AppendText("\nint x = 42;")
	if widget.GetLineCount() < 1 {
		t.Errorf("Invalid line count: %d", widget.GetLineCount())
	}

	widget.EnableMultiLineTabs(false)
	widget.ShowDocumentMap(false)
	widget.ClearAll()
	if widget.GetText() != "" {
		t.Errorf("Expected empty text after ClearAll")
	}

	menuBar := win.CreateMenuBar()
	if menuBar == nil {
		t.Fatal("CreateMenuBar returned nil")
	}
	menu := menuBar.AddMenu("Edit")
	menu.AddItem("Copy", "Ctrl+C", func() {})
	menu.AddSeparator()

	toolBar := win.CreateToolBar()
	if toolBar == nil {
		t.Fatal("CreateToolBar returned nil")
	}
	toolBar.AddButton("edit-copy", "Copy Text", func() {})

	if ret := app.Run(); ret != 0 {
		t.Errorf("Run() returned %d", ret)
	}
	app.Quit()
}

func TestBtkComponents(t *testing.T) {
	app := NewApp()
	btkWin := app.MainWindow().(*Window)

	// TabWidget testing
	tabWidget := btkWin.GetTabWidget()
	if tabWidget.Count() != 0 {
		t.Errorf("Expected 0 initial tabs, got %d", tabWidget.Count())
	}
	tabWidget.AddTab("buffer.c")
	tabWidget.AddTab("buffer.h")
	if tabWidget.Count() != 2 {
		t.Errorf("Expected 2 tabs, got %d", tabWidget.Count())
	}
	tabWidget.SetOrientation(ui.Horizontal)
	tabWidget.SetMultiLine(false)
	if err := tabWidget.RemoveTab(1); err != nil {
		t.Fatalf("Failed to remove tab: %v", err)
	}
	if tabWidget.Count() != 1 {
		t.Errorf("Expected 1 tab after removal, got %d", tabWidget.Count())
	}

	// StatusBar testing
	statusBar := btkWin.GetStatusBar()
	statusBar.SetPosition(12, 5)
	statusBar.SetEncoding("ISO-8859-1")
	statusBar.SetFileType("C")
	statusBar.SetMessage("Modified")
	statusText := statusBar.GetText()
	if !strings.Contains(statusText, "Ln 12, Col 5") || !strings.Contains(statusText, "ISO-8859-1") {
		t.Errorf("Unexpected status text: %s", statusText)
	}

	// SearchBar testing
	searchBar := btkWin.GetSearchBar()
	searchBar.SetQuery("printf")
	content := "printf(\"hello\");\nprintf(\"world\");\n"
	matches := searchBar.ExecuteSearch(content)
	if matches != 2 {
		t.Errorf("Expected 2 matches for printf, got %d", matches)
	}
	if searchBar.GetResultsCount() != 2 {
		t.Errorf("Expected GetResultsCount() == 2, got %d", searchBar.GetResultsCount())
	}
}
