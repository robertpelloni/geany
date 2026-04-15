package main

/*
#include <stdlib.h>
*/
import "C"

import (
	"fmt"
	"github.com/geany/geany-go/config"
	"github.com/geany/geany-go/macros"
	"github.com/geany/geany-go/plugins"
	"github.com/geany/geany-go/search"
	"github.com/geany/geany-go/symbols"
	"github.com/geany/geany-go/templates"
)

// Global instances for the FFI bridge
var (
	configMgr   *config.Manager
	macroEngine *macros.Engine
	pluginMgr   *plugins.Manager
	searchEng   *search.Engine
	symbolSpace *symbols.Workspace
	templateEng *templates.Engine
)

//export GeanyGo_Initialize
func GeanyGo_Initialize() {
	fmt.Println("[Geany-Go FFI] Initializing Go ultra-project backend...")

	configMgr = config.NewManager()

	// Simple stub executor for the macro engine via FFI
	executor := func(a macros.Action) error {
		fmt.Printf("[Geany-Go FFI] Executing Macro Action: %v\n", a)
		return nil
	}
	macroEngine = macros.NewEngine(executor)

	pluginMgr = plugins.NewManager()
	searchEng = search.NewEngine()
	symbolSpace = symbols.NewWorkspace()

	// Default preferences for templates
	prefs := &templates.Preferences{
		Developer: "Geany User",
		Version:   "2.2.0-ultra",
	}
	templateEng = templates.NewEngine(prefs)
}

//export GeanyGo_Shutdown
func GeanyGo_Shutdown() {
	fmt.Println("[Geany-Go FFI] Shutting down Go backend...")
	if pluginMgr != nil {
		pluginMgr.DisableAll()
	}
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

func main() {
	// The main function is required for c-shared buildmode, but execution starts in the C/C++ host.
}
