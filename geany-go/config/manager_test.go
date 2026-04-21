package config

import (
	"os"
	"testing"
)

func TestConfigManagerBasic(t *testing.T) {
	mgr := NewManager()

	// Test basic set/get
	mgr.SetString("Geany", "theme", "dark")
	mgr.SetString("Geany", "font", "Monospace 10")

	val := mgr.GetString("Geany", "theme", "light")
	if val != "dark" {
		t.Errorf("Expected 'dark', got '%s'", val)
	}

	fallback := mgr.GetString("Geany", "missing_key", "default_val")
	if fallback != "default_val" {
		t.Errorf("Expected fallback 'default_val', got '%s'", fallback)
	}
}

func TestConfigManagerLoadSave(t *testing.T) {
	mgr := NewManager()
	mgr.SetString("UI", "show_sidebar", "true")
	mgr.SetString("Files", "show_hidden", "false")

	// Create temp file
	f, err := os.CreateTemp("", "geany_config_test_*.ini")
	if err != nil {
		t.Fatal(err)
	}
	tempFile := f.Name()
	f.Close()
	defer os.Remove(tempFile)

	// Save
	err = mgr.Save(tempFile)
	if err != nil {
		t.Fatalf("Failed to save config: %v", err)
	}

	// Load into a new manager
	newMgr := NewManager()
	err = newMgr.Load(tempFile)
	if err != nil {
		t.Fatalf("Failed to load config: %v", err)
	}

	// Verify
	val := newMgr.GetString("UI", "show_sidebar", "false")
	if val != "true" {
		t.Errorf("Expected 'true', got '%s'", val)
	}
}
