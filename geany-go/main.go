package main

/*
#include <stdlib.h>
*/
import "C"

import (
	"fmt"
	"github.com/geany/geany-go/config"
	"github.com/geany/geany-go/editor"
	"github.com/geany/geany-go/macros"
	"github.com/geany/geany-go/plugins"
	"github.com/geany/geany-go/scintilla"
	"github.com/geany/geany-go/search"
	"github.com/geany/geany-go/symbols"
	"github.com/geany/geany-go/templates"
	_ "github.com/geany/geany-go/ui/bobgui"
)

// Global instances for the FFI bridge
var (
	configMgr   *config.Manager
	macroEngine *macros.Engine
	pluginMgr   *plugins.Manager
	searchEng   *search.Engine
	symbolSpace *symbols.Workspace
	templateEng *templates.Engine
	editorMgr   *editor.Editor
	activeScin  *scintilla.ScintillaEditor
)

//export GeanyGo_Initialize
func GeanyGo_Initialize() {
	fmt.Println("[Geany-Go FFI] Initializing Go ultra-project backend...")

	configMgr = config.NewManager()

	executor := func(a macros.Action) error {
		fmt.Printf("[Geany-Go FFI] Executing Macro Action: %v\n", a)
		return nil
	}
	macroEngine = macros.NewEngine(executor)

	pluginMgr = plugins.NewManager()
	searchEng = search.NewEngine()
	symbolSpace = symbols.NewWorkspace()

	prefs := &templates.Preferences{
		Developer: "Geany User",
		Version:   "2.2.0-ultra",
	}
	templateEng = templates.NewEngine(prefs)

	// Initialize the Editor document lifecycle manager
	editorMgr = editor.NewEditor()
}

//export GeanyGo_Shutdown
func GeanyGo_Shutdown() {
	fmt.Println("[Geany-Go FFI] Shutting down Go backend...")
	if pluginMgr != nil {
		pluginMgr.DisableAll()
	}
}

//export GeanyGo_Scintilla_Bind
func GeanyGo_Scintilla_Bind(fnPtr int64, objPtr int64) {
	fmt.Printf("[Geany-Go FFI] Binding Scintilla pointers: fn=%v, obj=%v\n", fnPtr, objPtr)
	activeScin = scintilla.NewScintillaEditor(fnPtr, objPtr)
}

//export GeanyGo_Editor_OpenDocument
func GeanyGo_Editor_OpenDocument(cPath *C.char) int {
	if editorMgr == nil {
		return -1
	}
	path := C.GoString(cPath)
	doc, err := editorMgr.OpenDocument(path)
	if err != nil {
		fmt.Printf("[Geany-Go FFI] Error opening document: %v\n", err)
		return -1
	}
	return doc.ID
}

//export GeanyGo_Config_SetString
func GeanyGo_Config_SetString(cSection *C.char, cKey *C.char, cValue *C.char) {
	if configMgr == nil {
		return
	}
	section := C.GoString(cSection)
	key := C.GoString(cKey)
	value := C.GoString(cValue)
	configMgr.SetString(section, key, value)
}

//export GeanyGo_Config_GetString
func GeanyGo_Config_GetString(cSection *C.char, cKey *C.char, cFallback *C.char) *C.char {
	if configMgr == nil {
		return cFallback
	}
	section := C.GoString(cSection)
	key := C.GoString(cKey)
	fallback := C.GoString(cFallback)

	val := configMgr.GetString(section, key, fallback)
	return C.CString(val) // Caller must free this memory
}

func main() {}
