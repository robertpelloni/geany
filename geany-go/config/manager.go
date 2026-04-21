package config

import (
	"bufio"
	"fmt"
	"os"
	"strings"
)

// Section represents a group of key-value pairs, matching the [Section] layout of INI files.
type Section map[string]string

// Manager is responsible for loading, parsing, and writing Geany-style configuration files (.conf/.ini).
// This is an idiomatic Go port of the core of src/keyfile.c and src/prefs.c
type Manager struct {
	sections map[string]Section
}

// NewManager initializes a new, empty configuration manager.
func NewManager() *Manager {
	return &Manager{
		sections: make(map[string]Section),
	}
}

// Load reads an INI-style file from disk and parses it into memory.
func (m *Manager) Load(filename string) error {
	file, err := os.Open(filename)
	if err != nil {
		return err
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	currentSection := ""

	for scanner.Scan() {
		line := strings.TrimSpace(scanner.Text())

		// Skip empty lines and comments (Geany uses # and ; for comments)
		if line == "" || strings.HasPrefix(line, "#") || strings.HasPrefix(line, ";") {
			continue
		}

		// Parse Section headers e.g. [Geany]
		if strings.HasPrefix(line, "[") && strings.HasSuffix(line, "]") {
			currentSection = line[1 : len(line)-1]
			if _, exists := m.sections[currentSection]; !exists {
				m.sections[currentSection] = make(Section)
			}
			continue
		}

		// Parse Key=Value
		parts := strings.SplitN(line, "=", 2)
		if len(parts) == 2 {
			key := strings.TrimSpace(parts[0])
			value := strings.TrimSpace(parts[1])

			if currentSection == "" {
				currentSection = "default" // Fallback for root-level keys
				if _, exists := m.sections[currentSection]; !exists {
					m.sections[currentSection] = make(Section)
				}
			}
			m.sections[currentSection][key] = value
		}
	}

	return scanner.Err()
}

// Save writes the current configuration state back to disk.
func (m *Manager) Save(filename string) error {
	file, err := os.Create(filename)
	if err != nil {
		return err
	}
	defer file.Close()

	writer := bufio.NewWriter(file)

	for secName, section := range m.sections {
		if secName != "default" {
			_, err := writer.WriteString(fmt.Sprintf("[%s]\n", secName))
			if err != nil {
				return err
			}
		}

		for key, val := range section {
			_, err := writer.WriteString(fmt.Sprintf("%s=%s\n", key, val))
			if err != nil {
				return err
			}
		}
		writer.WriteString("\n") // Blank line between sections
	}

	return writer.Flush()
}

// GetString retrieves a string value for a given section and key.
// Returns the provided fallback value if the key or section does not exist.
func (m *Manager) GetString(section, key, fallback string) string {
	if sec, ok := m.sections[section]; ok {
		if val, ok := sec[key]; ok {
			return val
		}
	}
	return fallback
}

// SetString stores a string value under a given section and key.
func (m *Manager) SetString(section, key, value string) {
	if _, exists := m.sections[section]; !exists {
		m.sections[section] = make(Section)
	}
	m.sections[section][key] = value
}
