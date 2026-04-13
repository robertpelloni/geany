package plugins

import (
	"fmt"
	"sync"
)

// Info holds metadata about a loaded Go-based Geany plugin.
// This is an idiomatic port mirroring the C PluginInfo struct.
type Info struct {
	Name        string
	Description string
	Version     string
	Author      string
}

// Plugin is the primary interface that any Go extension must implement.
type Plugin interface {
	// Metadata returns the identifying information for the plugin.
	Metadata() Info
	// Init is called when the plugin is enabled.
	Init() error
	// Cleanup is called when the plugin is disabled or the IDE shuts down.
	Cleanup()
}

// Manager serves as the central registry for dynamically loaded Go plugins.
type Manager struct {
	mu      sync.RWMutex
	plugins map[string]Plugin
	active  map[string]bool
}

// NewManager initializes a thread-safe Go plugin registry.
func NewManager() *Manager {
	return &Manager{
		plugins: make(map[string]Plugin),
		active:  make(map[string]bool),
	}
}

// Register statically registers a plugin structure into the manager.
// In a real port using Go plugins (build -buildmode=plugin), this would be wrapped by a dynamic loader.
func (m *Manager) Register(p Plugin) error {
	m.mu.Lock()
	defer m.mu.Unlock()

	info := p.Metadata()
	if info.Name == "" {
		return fmt.Errorf("plugin name cannot be empty")
	}

	if _, exists := m.plugins[info.Name]; exists {
		return fmt.Errorf("plugin %s already registered", info.Name)
	}

	m.plugins[info.Name] = p
	m.active[info.Name] = false
	return nil
}

// Enable activates a registered plugin, triggering its Init() hook.
func (m *Manager) Enable(name string) error {
	m.mu.Lock()
	defer m.mu.Unlock()

	p, exists := m.plugins[name]
	if !exists {
		return fmt.Errorf("plugin %s not found", name)
	}

	if m.active[name] {
		return nil // Already active
	}

	if err := p.Init(); err != nil {
		return fmt.Errorf("failed to init plugin %s: %w", name, err)
	}

	m.active[name] = true
	return nil
}

// Disable deactivates a plugin, triggering its Cleanup() hook.
func (m *Manager) Disable(name string) error {
	m.mu.Lock()
	defer m.mu.Unlock()

	p, exists := m.plugins[name]
	if !exists {
		return fmt.Errorf("plugin %s not found", name)
	}

	if !m.active[name] {
		return nil // Already inactive
	}

	p.Cleanup()
	m.active[name] = false
	return nil
}

// GetActive returns a list of all currently enabled plugin metadata.
func (m *Manager) GetActive() []Info {
	m.mu.RLock()
	defer m.mu.RUnlock()

	var active []Info
	for name, p := range m.plugins {
		if m.active[name] {
			active = append(active, p.Metadata())
		}
	}
	return active
}

// DisableAll safely tears down all running plugins.
func (m *Manager) DisableAll() {
	m.mu.Lock()
	defer m.mu.Unlock()

	for name, p := range m.plugins {
		if m.active[name] {
			p.Cleanup()
			m.active[name] = false
		}
	}
}
