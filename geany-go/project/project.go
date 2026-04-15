package project

import (
	"errors"
	"path/filepath"
	"sync"
)

// BuildCommand represents a single build step configurable per project.
type BuildCommand struct {
	Label      string
	Command    string
	WorkingDir string
}

// Config encapsulates the data of an active Geany project.
// This is the Go-native implementation corresponding to the GeanyProject C struct.
type Config struct {
	Name         string
	FileName     string
	BasePath     string
	Description  string
	FilePatterns []string
	BuildSteps   map[string]BuildCommand
}

// Manager handles the lifecycle of the IDE's active project.
type Manager struct {
	mu            sync.RWMutex
	activeProject *Config
}

// NewManager initializes the global project state tracker.
func NewManager() *Manager {
	return &Manager{}
}

// Create establishes a new blank project and sets it as active.
func (m *Manager) Create(name, filename, basePath string) (*Config, error) {
	m.mu.Lock()
	defer m.mu.Unlock()

	if name == "" {
		return nil, errors.New("project name cannot be empty")
	}

	m.activeProject = &Config{
		Name:         name,
		FileName:     filename,
		BasePath:     basePath,
		FilePatterns: make([]string, 0),
		BuildSteps:   make(map[string]BuildCommand),
	}

	return m.activeProject, nil
}

// Load reads project settings from disk (stub logic for now).
func (m *Manager) Load(filename string) (*Config, error) {
	m.mu.Lock()
	defer m.mu.Unlock()

	if filename == "" {
		return nil, errors.New("invalid filename")
	}

	// In a real implementation, this would use geany-go/config to parse the .geany ini file.
	// We simulate extracting the base name for the stub.
	base := filepath.Base(filename)
	ext := filepath.Ext(base)
	name := base[:len(base)-len(ext)]
	if name == "" {
		name = "Unnamed"
	}

	m.activeProject = &Config{
		Name:         name,
		FileName:     filename,
		BasePath:     filepath.Dir(filename),
		FilePatterns: []string{"*.*"},
		BuildSteps:   make(map[string]BuildCommand),
	}

	return m.activeProject, nil
}

// Close clears the active project from memory.
func (m *Manager) Close() {
	m.mu.Lock()
	defer m.mu.Unlock()
	m.activeProject = nil
}

// GetActive returns the currently active project configuration, or nil if none exists.
func (m *Manager) GetActive() *Config {
	m.mu.RLock()
	defer m.mu.RUnlock()
	return m.activeProject
}

// AddBuildStep registers a custom build command to the active project.
func (m *Manager) AddBuildStep(id, label, command, workingDir string) error {
	m.mu.Lock()
	defer m.mu.Unlock()

	if m.activeProject == nil {
		return errors.New("no active project")
	}

	if m.activeProject.BuildSteps == nil {
		m.activeProject.BuildSteps = make(map[string]BuildCommand)
	}

	m.activeProject.BuildSteps[id] = BuildCommand{
		Label:      label,
		Command:    command,
		WorkingDir: workingDir,
	}

	return nil
}
