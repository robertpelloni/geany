package bobgui

import (
	"testing"
	"github.com/geany/geany-go/editor"
)

func TestBobguiApplicationLifecycle(t *testing.T) {
	app := NewApp()

	err := app.Init([]string{"geany", "test.txt"})
	if err != nil {
		t.Fatalf("Failed to init bobgui app: %v", err)
	}

	win := app.MainWindow()
	if win == nil {
		t.Fatalf("MainWindow returned nil")
	}

	win.SetTitle("Test GTK Window")
	win.Show()

	doc := editor.NewDocument(1, "/tmp/test.txt", nil)
	widget := win.AddDocument(doc)

	if widget == nil {
		t.Fatalf("AddDocument returned nil widget")
	}

	widget.SetText("Hello BobGUI")
	if widget.GetText() != "Hello BobGUI" {
		t.Errorf("Widget text mismatch")
	}

	widget.EnableMultiLineTabs(true)
	widget.ShowDocumentMap(true)

	activeDoc := win.GetActiveDocument()
	if activeDoc == nil || activeDoc.ID != 1 {
		t.Errorf("Active document mismatch")
	}

	res := app.Run()
	if res != 0 {
		t.Errorf("Expected 0 return code from Run, got %d", res)
	}

	app.Quit()
}
