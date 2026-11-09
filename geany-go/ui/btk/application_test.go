package btk

import (
	"testing"

	"github.com/geany/geany-go/editor"
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
