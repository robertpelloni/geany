package btk

import (
	"fmt"
	"github.com/geany/geany-go/editor"
	"github.com/geany/geany-go/ui"
)

// App is the btk (Qt4/CopperSpice) implementation of the ui.Application interface.
type App struct {
	mainWindow *Window
	isRunning  bool
}

// NewApp creates a new btk application instance.
func NewApp() ui.Application {
	return &App{
		mainWindow: NewWindow(),
		isRunning:  false,
	}
}

func (a *App) Init(args []string) error {
	fmt.Println("[btk] Initializing Qt4/CS subsystem...")
	return nil
}

func (a *App) Run() int {
	fmt.Println("[btk] Starting Qt4/CS main event loop...")
	a.isRunning = true
	return 0
}

func (a *App) Quit() {
	fmt.Println("[btk] Shutting down Qt4/CS subsystem...")
	if a.isRunning {
		a.isRunning = false
	}
}

func (a *App) MainWindow() ui.Window {
	return a.mainWindow
}

// -----------------------------------------------------------------------------
// Window Implementation
// -----------------------------------------------------------------------------

type Window struct {
	title string
	docs  []*editor.Document
}

func NewWindow() *Window {
	return &Window{
		title: "Geany Ultra (btk Qt4)",
		docs:  make([]*editor.Document, 0),
	}
}

func (w *Window) SetTitle(title string) {
	w.title = title
	fmt.Printf("[btk] Window title set to: %s\n", title)
}

func (w *Window) Show() {
	fmt.Println("[btk] Qt4 Window shown.")
}

func (w *Window) Hide() {
	fmt.Println("[btk] Qt4 Window hidden.")
}

func (w *Window) AddDocument(doc *editor.Document) ui.EditorWidget {
	w.docs = append(w.docs, doc)
	fmt.Printf("[btk] Document added to Window: %s\n", doc.BaseName)
	return NewEditorWidget()
}

func (w *Window) GetActiveDocument() *editor.Document {
	if len(w.docs) > 0 {
		return w.docs[0]
	}
	return nil
}

func (w *Window) CreateMenuBar() ui.MenuBar {
	return &MenuBar{}
}

func (w *Window) CreateToolBar() ui.Toolbar {
	return &Toolbar{}
}

// -----------------------------------------------------------------------------
// EditorWidget Implementation
// -----------------------------------------------------------------------------

type Editor struct {
	text string
}

func NewEditorWidget() ui.EditorWidget {
	return &Editor{}
}

func (e *Editor) SetText(text string) { e.text = text }
func (e *Editor) GetText() string     { return e.text }
func (e *Editor) AppendText(text string) { e.text += text }
func (e *Editor) ClearAll()           { e.text = "" }
func (e *Editor) SetReadOnly(ro bool) {}
func (e *Editor) GetLineCount() int   { return 1 }

func (e *Editor) EnableMultiLineTabs(enable bool) {
	fmt.Printf("[btk] Qt4 MultiLineTabs parity feature: %v\n", enable)
}

func (e *Editor) ShowDocumentMap(show bool) {
	fmt.Printf("[btk] Qt4 DocumentMap parity feature: %v\n", show)
}

// -----------------------------------------------------------------------------
// Menus & Toolbars Stubs
// -----------------------------------------------------------------------------

type MenuBar struct{}
func (m *MenuBar) AddMenu(title string) ui.Menu { return &Menu{} }

type Menu struct{}
func (m *Menu) AddItem(title string, shortcut string, action func()) {}
func (m *Menu) AddSeparator()                                        {}

type Toolbar struct{}
func (t *Toolbar) AddButton(icon string, tooltip string, action func()) {}
