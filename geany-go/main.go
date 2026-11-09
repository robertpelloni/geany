package main

/*
#include <stdlib.h>
*/
import "C"

import (
	"fmt"
	"unsafe"
	"github.com/geany/geany-go/config"
	"github.com/geany/geany-go/editor"
	"github.com/geany/geany-go/macros"
	"github.com/geany/geany-go/parser"
	"github.com/geany/geany-go/plugins"
	"github.com/geany/geany-go/project"
	"github.com/geany/geany-go/scintilla"
	"github.com/geany/geany-go/search"
	"github.com/geany/geany-go/symbols"
	"github.com/geany/geany-go/templates"
	"github.com/geany/geany-go/textfx"
	"github.com/geany/geany-go/ui"
	_ "github.com/geany/geany-go/ui/bobgui"
	_ "github.com/geany/geany-go/ui/bobui"
	_ "github.com/geany/geany-go/ui/btk"
)

// Global instances for the FFI bridge
var (
	configMgr    *config.Manager
	macroEngine  *macros.Engine
	pluginMgr    *plugins.Manager
	projectMgr   *project.Manager
	searchEng    *search.Engine
	symbolSpace  *symbols.Workspace
	templateEng  *templates.Engine
	editorMgr    *editor.Editor
	activeScin   *scintilla.ScintillaEditor
	tabMgr       ui.TabManager
	codeParser   *parser.Parser
	uiDispatcher *ui.EventDispatcher
)

//export GeanyGo_Initialize
func GeanyGo_Initialize() {
	fmt.Println("[Geany-Go FFI] Initializing Go ultra-project backend...")

	configMgr = config.NewManager()
	projectMgr = project.NewManager()

	executor := func(a macros.Action) error {
		fmt.Printf("[Geany-Go FFI] Executing Macro Action: %v\n", a)
		return nil
	}
	macroEngine = macros.NewEngine(executor)

	pluginMgr = plugins.NewManager()
	searchEng = search.NewEngine()
	symbolSpace = symbols.NewWorkspace()
	codeParser = parser.NewParser()
	uiDispatcher = ui.NewEventDispatcher()

	prefs := &templates.Preferences{
		Developer: "Geany User",
		Version:   "2.2.0-ultra",
	}
	templateEng = templates.NewEngine(prefs)

	// Initialize the Editor document lifecycle manager
	editorMgr = editor.NewEditor()

	// Initialize UI Tab Manager
	tabMgr = ui.NewDefaultTabManager()
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
	if uiDispatcher != nil {
		uiDispatcher.Dispatch(ui.UIEvent{
			Type:    ui.EventDocOpened,
			Payload: path,
		})
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

//export GeanyGo_FreeString
// GeanyGo_FreeString frees a string allocated by Go via C.CString
func GeanyGo_FreeString(str *C.char) {
	C.free(unsafe.Pointer(str))
}

//export GeanyGo_Project_Create
func GeanyGo_Project_Create(cName *C.char, cFilename *C.char, cBasePath *C.char) int {
	if projectMgr == nil {
		return 0
	}
	name := C.GoString(cName)
	filename := C.GoString(cFilename)
	basePath := C.GoString(cBasePath)

	_, err := projectMgr.Create(name, filename, basePath)
	if err != nil {
		fmt.Printf("[Geany-Go FFI] Error creating project: %v\n", err)
		return 0
	}
	return 1
}

//export GeanyGo_Project_Open
func GeanyGo_Project_Open(cFilename *C.char) int {
	if projectMgr == nil {
		return 0
	}
	filename := C.GoString(cFilename)

	_, err := projectMgr.Load(filename)
	if err != nil {
		fmt.Printf("[Geany-Go FFI] Error opening project: %v\n", err)
		return 0
	}
	return 1
}

//export GeanyGo_Project_Close
func GeanyGo_Project_Close() {
	if projectMgr != nil {
		projectMgr.Close()
	}
}

//export GeanyGo_TextFX_SortLines
func GeanyGo_TextFX_SortLines(cText *C.char, ascending bool, caseSensitive bool) *C.char {
	text := C.GoString(cText)
	sorted := textfx.SortLines(text, ascending, caseSensitive)
	return C.CString(sorted)
}

//export GeanyGo_TextFX_ToProperCase
func GeanyGo_TextFX_ToProperCase(cText *C.char) *C.char {
	text := C.GoString(cText)
	converted := textfx.ToProperCase(text)
	return C.CString(converted)
}

//export GeanyGo_TextFX_ToSentenceCase
func GeanyGo_TextFX_ToSentenceCase(cText *C.char) *C.char {
	text := C.GoString(cText)
	converted := textfx.ToSentenceCase(text)
	return C.CString(converted)
}

//export GeanyGo_TextFX_TrimTrailingWhitespace
func GeanyGo_TextFX_TrimTrailingWhitespace(cText *C.char) *C.char {
	text := C.GoString(cText)
	formatted := textfx.TrimTrailingWhitespace(text)
	return C.CString(formatted)
}

//export GeanyGo_TextFX_RemoveBlankLines
func GeanyGo_TextFX_RemoveBlankLines(cText *C.char) *C.char {
	text := C.GoString(cText)
	cleaned := textfx.RemoveBlankLines(text)
	return C.CString(cleaned)
}

//export GeanyGo_TextFX_RemoveDuplicateLines
func GeanyGo_TextFX_RemoveDuplicateLines(cText *C.char) *C.char {
	text := C.GoString(cText)
	deduped := textfx.RemoveDuplicateLines(text)
	return C.CString(deduped)
}

//export GeanyGo_TextFX_EncodeBase64
func GeanyGo_TextFX_EncodeBase64(cText *C.char) *C.char {
	text := C.GoString(cText)
	encoded := textfx.EncodeBase64(text)
	return C.CString(encoded)
}

//export GeanyGo_TextFX_DecodeBase64
func GeanyGo_TextFX_DecodeBase64(cText *C.char) *C.char {
	text := C.GoString(cText)
	decoded, err := textfx.DecodeBase64(text)
	if err != nil {
		return C.CString("")
	}
	return C.CString(decoded)
}

//export GeanyGo_UI_SetTabOrientation
func GeanyGo_UI_SetTabOrientation(vertical bool) {
	if tabMgr != nil {
		if vertical {
			tabMgr.SetOrientation(ui.Vertical)
		} else {
			tabMgr.SetOrientation(ui.Horizontal)
		}
	}
}

//export GeanyGo_UI_DispatchEvent
func GeanyGo_UI_DispatchEvent(eventType int, payload *C.char) {
	if uiDispatcher == nil {
		return
	}
	var pyld string
	if payload != nil {
		pyld = C.GoString(payload)
	}
	uiDispatcher.Dispatch(ui.UIEvent{
		Type:    ui.EventType(eventType),
		Payload: pyld,
	})
}

//export GeanyGo_Parser_ParseAndIndex
func GeanyGo_Parser_ParseAndIndex(cFilename *C.char, cContent *C.char, cLang *C.char) int {
	if codeParser == nil || symbolSpace == nil {
		return 0
	}
	filename := C.GoString(cFilename)
	content := C.GoString(cContent)
	lang := C.GoString(cLang)

	syms, err := codeParser.ParseSource(filename, content, lang)
	if err != nil {
		fmt.Printf("[Geany-Go Parser] Error parsing %s: %v\n", filename, err)
		return 0
	}

	symbolSpace.AddFileSymbols(filename, syms)

	if uiDispatcher != nil {
		uiDispatcher.Dispatch(ui.UIEvent{
			Type:    ui.EventSymbolsUpdated,
			Payload: filename,
		})
	}
	return len(syms)
}

func main() {}
