package plugins

import (
	"errors"
	"testing"
)

// MockPlugin for unit testing
type MockPlugin struct {
	initCalled    bool
	cleanupCalled bool
}

func (m *MockPlugin) Metadata() Info {
	return Info{Name: "Mock", Author: "AI", Version: "1.0"}
}
func (m *MockPlugin) Init() error {
	m.initCalled = true
	return nil
}
func (m *MockPlugin) Cleanup() {
	m.cleanupCalled = true
}

// ErrorPlugin simulates a failed initialization
type ErrorPlugin struct{}

func (e *ErrorPlugin) Metadata() Info {
	return Info{Name: "ErrorPlugin"}
}
func (e *ErrorPlugin) Init() error {
	return errors.New("simulated init failure")
}
func (e *ErrorPlugin) Cleanup() {}

func TestPluginManager(t *testing.T) {
	mgr := NewManager()
	mock := &MockPlugin{}

	// Test Registration
	if err := mgr.Register(mock); err != nil {
		t.Fatalf("Failed to register: %v", err)
	}

	// Test Duplicate Registration
	if err := mgr.Register(mock); err == nil {
		t.Error("Expected error on duplicate registration")
	}

	// Test Enable
	if err := mgr.Enable("Mock"); err != nil {
		t.Fatalf("Failed to enable: %v", err)
	}
	if !mock.initCalled {
		t.Error("Init() was not called on Enable")
	}

	// Test Active Tracking
	active := mgr.GetActive()
	if len(active) != 1 || active[0].Name != "Mock" {
		t.Errorf("Active plugins list incorrect: %v", active)
	}

	// Test Disable
	if err := mgr.Disable("Mock"); err != nil {
		t.Fatalf("Failed to disable: %v", err)
	}
	if !mock.cleanupCalled {
		t.Error("Cleanup() was not called on Disable")
	}
}

func TestPluginManagerErrors(t *testing.T) {
	mgr := NewManager()
	errPlugin := &ErrorPlugin{}

	mgr.Register(errPlugin)

	if err := mgr.Enable("ErrorPlugin"); err == nil {
		t.Error("Expected error from failing Init(), got nil")
	}

	active := mgr.GetActive()
	if len(active) != 0 {
		t.Error("Failed plugin should not be marked active")
	}
}
