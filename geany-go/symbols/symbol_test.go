package symbols

import "testing"

func TestWorkspaceManagement(t *testing.T) {
	ws := NewWorkspace()

	syms1 := []Symbol{
		{Name: "main", File: "main.go", Line: 10, Kind: KindFunction, Signature: "()"},
		{Name: "Config", File: "main.go", Line: 15, Kind: KindStruct},
	}

	syms2 := []Symbol{
		{Name: "InitManager", File: "manager.cpp", Line: 25, Kind: KindFunction},
	}

	ws.AddFileSymbols("main.go", syms1)
	ws.AddFileSymbols("manager.cpp", syms2)

	// Test File retrieval
	mainSyms := ws.GetFileSymbols("main.go")
	if len(mainSyms) != 2 {
		t.Errorf("Expected 2 symbols in main.go, got %d", len(mainSyms))
	}

	// Test exact search
	foundExact := ws.FindByName("Config", true)
	if len(foundExact) != 1 || foundExact[0].Kind != KindStruct {
		t.Errorf("Exact search for 'Config' failed")
	}

	// Test partial/case-insensitive search
	foundPartial := ws.FindByName("init", false)
	if len(foundPartial) != 1 || foundPartial[0].Name != "InitManager" {
		t.Errorf("Partial search for 'init' failed")
	}

	// Test overwrite (reload file)
	syms1New := []Symbol{{Name: "run", File: "main.go", Line: 5, Kind: KindFunction}}
	ws.AddFileSymbols("main.go", syms1New)
	if len(ws.GetFileSymbols("main.go")) != 1 {
		t.Errorf("Overwrite failed, expected 1 symbol")
	}

	// Test clearing
	ws.ClearFile("main.go")
	if len(ws.GetFileSymbols("main.go")) != 0 {
		t.Error("ClearFile failed to remove symbols")
	}

	ws.ClearAll()
	if len(ws.FindByName("InitManager", true)) != 0 {
		t.Error("ClearAll failed to wipe workspace")
	}
}
