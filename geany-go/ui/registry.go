package ui

import (
	"fmt"
	"sync"
)

// FrontendType distinguishes the native UI toolkits.
type FrontendType string

const (
	FrontendBobGUI FrontendType = "bobgui" // GTK-based frontend
	FrontendBobUI  FrontendType = "bobui"  // Qt6-based frontend
	FrontendBTK    FrontendType = "btk"    // Qt4/CopperSpice-based frontend
)

// Registry manages multiple registered frontend applications, allowing the Go engine
// to route lifecycle events and active documents to the chosen native toolkit seamlessly.
type Registry struct {
	mu        sync.RWMutex
	frontends map[FrontendType]Application
	active    FrontendType
}

// NewRegistry initializes an empty frontend adapter registry.
func NewRegistry() *Registry {
	return &Registry{
		frontends: make(map[FrontendType]Application),
	}
}

// Register adds an Application frontend to the registry.
func (r *Registry) Register(fType FrontendType, app Application) {
	r.mu.Lock()
	defer r.mu.Unlock()
	r.frontends[fType] = app
	if r.active == "" {
		r.active = fType
	}
}

// SetActive selects the primary active frontend toolkit.
func (r *Registry) SetActive(fType FrontendType) error {
	r.mu.Lock()
	defer r.mu.Unlock()
	if _, ok := r.frontends[fType]; !ok {
		return fmt.Errorf("frontend %s is not registered", fType)
	}
	r.active = fType
	return nil
}

// GetActive retrieves the currently active Application adapter.
func (r *Registry) GetActive() (FrontendType, Application) {
	r.mu.RLock()
	defer r.mu.RUnlock()
	return r.active, r.frontends[r.active]
}

// Get retrieves a specific Application adapter by its FrontendType.
func (r *Registry) Get(fType FrontendType) (Application, bool) {
	r.mu.RLock()
	defer r.mu.RUnlock()
	app, ok := r.frontends[fType]
	return app, ok
}

// Available returns a list of all registered frontend types.
func (r *Registry) Available() []FrontendType {
	r.mu.RLock()
	defer r.mu.RUnlock()
	types := make([]FrontendType, 0, len(r.frontends))
	for t := range r.frontends {
		types = append(types, t)
	}
	return types
}
