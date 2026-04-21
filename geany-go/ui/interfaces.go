package ui

import "github.com/geany/geany-go/editor"

// Application represents the main entry point and lifecycle manager for any UI frontend.
// The submodules (bobui, btk, bobgui) will implement this interface to bootstrap their respective event loops.
type Application interface {
	Init(args []string) error
	Run() int
	Quit()
	MainWindow() Window
}

// Window represents the primary IDE frame containing menus, toolbars, sidebars, and notebooks.
type Window interface {
	SetTitle(title string)
	Show()
	Hide()
	AddDocument(doc *editor.Document) EditorWidget
	GetActiveDocument() *editor.Document

	// Menu and Toolbar construction
	CreateMenuBar() MenuBar
	CreateToolBar() Toolbar
}

// EditorWidget represents the actual text editing surface (the Scintilla equivalent).
// This isolates the backend text manipulation from the frontend rendering (Qt, GTK).
type EditorWidget interface {
	SetText(text string)
	GetText() string
	AppendText(text string)
	ClearAll()
	SetReadOnly(readOnly bool)
	GetLineCount() int

	// Parity Features (Notepad++)
	EnableMultiLineTabs(enable bool)
	ShowDocumentMap(show bool)
}

// MenuBar and Toolbar represent core navigational elements.
type MenuBar interface {
	AddMenu(title string) Menu
}

type Menu interface {
	AddItem(title string, shortcut string, action func())
	AddSeparator()
}

type Toolbar interface {
	AddButton(icon string, tooltip string, action func())
}
