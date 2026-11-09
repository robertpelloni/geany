package ui

import (
	"testing"
)

// mockApp implements ui.Application for testing the registry without import cycles.
type mockApp struct {
	name string
}

func (m *mockApp) Init(args []string) error { return nil }
func (m *mockApp) Run() int                 { return 0 }
func (m *mockApp) Quit()                    {}
func (m *mockApp) MainWindow() Window       { return nil }

func TestRegistry_Operations(t *testing.T) {
	reg := NewRegistry()

	bobguiApp := &mockApp{name: "bobgui"}
	bobuiApp := &mockApp{name: "bobui"}
	btkApp := &mockApp{name: "btk"}

	reg.Register(FrontendBobGUI, bobguiApp)
	reg.Register(FrontendBobUI, bobuiApp)
	reg.Register(FrontendBTK, btkApp)

	if len(reg.Available()) != 3 {
		t.Fatalf("Expected 3 registered frontends, got %d", len(reg.Available()))
	}

	activeType, activeApp := reg.GetActive()
	if activeType != FrontendBobGUI || activeApp == nil {
		t.Errorf("Expected initial active frontend to be FrontendBobGUI, got %v", activeType)
	}

	// Switch active frontend
	if err := reg.SetActive(FrontendBobUI); err != nil {
		t.Fatalf("Failed to set active frontend: %v", err)
	}

	activeType, activeApp = reg.GetActive()
	if activeType != FrontendBobUI || activeApp == nil {
		t.Errorf("Expected active frontend to be FrontendBobUI, got %v", activeType)
	}

	// Error on nonexistent frontend
	if err := reg.SetActive(FrontendType("nonexistent")); err == nil {
		t.Errorf("Expected error setting unknown frontend, got nil")
	}

	// Retrieve specific
	app, ok := reg.Get(FrontendBTK)
	if !ok || app == nil {
		t.Errorf("Failed to retrieve BTK frontend")
	}
}
