package bobui

import (
	"strings"
	"testing"

	"github.com/geany/geany-go/editor"
	"github.com/geany/geany-go/ui"
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

func TestBobuiComponents(t *testing.T) {
	app := NewApp()
	bobWin := app.MainWindow().(*Window)

	// TabWidget testing
	tabWidget := bobWin.GetTabWidget()
	if tabWidget.Count() != 0 {
		t.Errorf("Expected 0 initial tabs, got %d", tabWidget.Count())
	}
	tabWidget.AddTab("main.cpp")
	tabWidget.AddTab("utils.h")
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
	statusBar.SetPosition(42, 10)
	statusBar.SetEncoding("UTF-8")
	statusBar.SetFileType("C++")
	statusBar.SetMessage("Ready")
	statusText := statusBar.GetText()
	if !strings.Contains(statusText, "Ln 42, Col 10") || !strings.Contains(statusText, "C++") {
		t.Errorf("Unexpected status text: %s", statusText)
	}

	// SearchBar testing
	searchBar := bobWin.GetSearchBar()
	searchBar.SetQuery("test")
	content := "This is a test content with test cases."
	matches := searchBar.ExecuteSearch(content)
	if matches != 2 {
		t.Errorf("Expected 2 search matches, got %d", matches)
	}
	if searchBar.GetResultsCount() != 2 {
		t.Errorf("Expected GetResultsCount() == 2, got %d", searchBar.GetResultsCount())
	}
}
