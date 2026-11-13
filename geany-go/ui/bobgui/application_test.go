package bobgui

import (
	"strings"
	"testing"

	"github.com/geany/geany-go/editor"
	"github.com/geany/geany-go/ui"
)

func TestBobguiApplicationLifecycle(t *testing.T) {
	app := NewApp()
	if err := app.Init([]string{"--test"}); err != nil {
		t.Fatalf("Failed to initialize bobgui app: %v", err)
	}

	win := app.MainWindow()
	if win == nil {
		t.Fatal("MainWindow should not be nil")
	}

	win.SetTitle("Test GTK Window")
	win.Show()

	doc := &editor.Document{
		ID:       1,
		BaseName: "test.txt",
	}

	widget := win.AddDocument(doc)
	if widget == nil {
		t.Fatal("AddDocument returned nil widget")
	}

	if active := win.GetActiveDocument(); active == nil || active.ID != doc.ID {
		t.Errorf("Expected active document ID %d, got %v", doc.ID, active)
	}

	widget.SetText("Hello Bobgui")
	if widget.GetText() != "Hello Bobgui" {
		t.Errorf("GetText mismatch: %s", widget.GetText())
	}

	widget.AppendText(" World")
	widget.EnableMultiLineTabs(true)
	widget.ShowDocumentMap(true)

	if ret := app.Run(); ret != 0 {
		t.Errorf("Run() returned %d", ret)
	}
	app.Quit()
}

func TestBobguiComponents(t *testing.T) {
	app := NewApp()
	bobWin := app.MainWindow().(*Window)

	// TabWidget testing
	tabWidget := bobWin.GetTabWidget()
	if tabWidget.Count() != 0 {
		t.Errorf("Expected 0 initial tabs, got %d", tabWidget.Count())
	}
	tabWidget.AddTab("doc.txt")
	tabWidget.AddTab("notes.md")
	if tabWidget.Count() != 2 {
		t.Errorf("Expected 2 tabs, got %d", tabWidget.Count())
	}
	tabWidget.SetOrientation(ui.Vertical)
	tabWidget.SetMultiLine(true)
	if err := tabWidget.RemoveTab(0); err != nil {
		t.Fatalf("Failed to remove tab: %v", err)
	}
	if tabWidget.Count() != 1 {
		t.Errorf("Expected 1 tab after removal, got %d", tabWidget.Count())
	}

	// StatusBar testing
	statusBar := bobWin.GetStatusBar()
	statusBar.SetPosition(5, 12)
	statusBar.SetEncoding("UTF-8")
	statusBar.SetFileType("Markdown")
	statusBar.SetMessage("Idle")
	statusText := statusBar.GetText()
	if !strings.Contains(statusText, "Ln 5, Col 12") || !strings.Contains(statusText, "Markdown") {
		t.Errorf("Unexpected status text: %s", statusText)
	}

	// SearchBar testing
	searchBar := bobWin.GetSearchBar()
	searchBar.SetQuery("sample")
	content := "sample text and another sample line"
	matches := searchBar.ExecuteSearch(content)
	if matches != 2 {
		t.Errorf("Expected 2 matches for sample, got %d", matches)
	}
	if searchBar.GetResultsCount() != 2 {
		t.Errorf("Expected GetResultsCount() == 2, got %d", searchBar.GetResultsCount())
	}
}
