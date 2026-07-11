package filetypes

import (
	"path/filepath"
	"strings"
	"sync"
)

// LexerType represents the underlying syntax parsing engine identifier
// (mapping broadly to Scintilla lexer constants).
type LexerType int

const (
	LexerNone LexerType = iota
	LexerCPP
	LexerPython
	LexerGo
	LexerHTML
	LexerMake
)

// FileType represents a specific programming or markup language.
// It maps to the legacy GeanyFiletype struct in src/filetypes.c.
type FileType struct {
	ID          string
	Name        string
	Extensions  []string
	Lexer       LexerType
	Icon        string
	Comments    string
	StringChars string
	WordChars   string
	Settings    map[string]string
}

// Manager holds the central registry of supported languages.
type Manager struct {
	types map[string]*FileType
	mu    sync.RWMutex
}

// NewManager initializes the language registry with common defaults.
func NewManager() *Manager {
	m := &Manager{
		types: make(map[string]*FileType),
	}
	m.registerDefaults()
	return m
}

func (m *Manager) registerDefaults() {
	m.Register(&FileType{
		ID:         "c",
		Name:       "C/C++",
		Extensions: []string{".c", ".cpp", ".h", ".hpp", ".cxx"},
		Lexer:      LexerCPP,
		Comments:   "//",
	})
	m.Register(&FileType{
		ID:         "python",
		Name:       "Python",
		Extensions: []string{".py", ".pyw"},
		Lexer:      LexerPython,
		Comments:   "#",
	})
	m.Register(&FileType{
		ID:         "go",
		Name:       "Go",
		Extensions: []string{".go"},
		Lexer:      LexerGo,
		Comments:   "//",
	})
	m.Register(&FileType{
		ID:         "make",
		Name:       "Make",
		Extensions: []string{"Makefile", "makefile", ".mak"},
		Lexer:      LexerMake,
		Comments:   "#",
	})
	m.Register(&FileType{
		ID:         "none",
		Name:       "None",
		Extensions: []string{".txt", ""},
		Lexer:      LexerNone,
	})
}

// Register adds a new FileType to the system.
func (m *Manager) Register(ft *FileType) {
	m.mu.Lock()
	defer m.mu.Unlock()
	m.types[ft.ID] = ft
}

// GetByExtension retrieves a language definition by its extension.
func (m *Manager) GetByExtension(ext string) *FileType {
	m.mu.RLock()
	defer m.mu.RUnlock()

	ext = strings.ToLower(ext)
	if !strings.HasPrefix(ext, ".") {
		ext = "." + ext
	}

	for _, ft := range m.types {
		for _, definedExt := range ft.Extensions {
			if ext == definedExt {
				return ft
			}
		}
	}

	if ft, ok := m.types["none"]; ok {
		return ft
	}
	return &FileType{Name: "None", Lexer: LexerNone}
}

// DetectType attempts to determine the FileType from a given filename or path.
// It checks explicit extension matches first, then falls back to exact filename matching (e.g., 'Makefile').
func (m *Manager) DetectType(filename string) *FileType {
	m.mu.RLock()
	defer m.mu.RUnlock()

	base := filepath.Base(filename)
	ext := strings.ToLower(filepath.Ext(filename))

	// 1. Check exact matches (e.g., Makefile)
	for _, ft := range m.types {
		for _, definedExt := range ft.Extensions {
			if base == definedExt {
				return ft
			}
		}
	}

	// 2. Check extension matches
	if ext != "" {
		for _, ft := range m.types {
			for _, definedExt := range ft.Extensions {
				if ext == definedExt {
					return ft
				}
			}
		}
	}

	// 3. Fallback
	if ft, ok := m.types["none"]; ok {
		return ft
	}
	return &FileType{Name: "None", Lexer: LexerNone}
}

// GetByID retrieves a language definition by its internal string identifier.
func (m *Manager) GetByID(id string) *FileType {
	m.mu.RLock()
	defer m.mu.RUnlock()

	if ft, ok := m.types[id]; ok {
		return ft
	}
	if ft, ok := m.types["none"]; ok {
		return ft
	}
	return &FileType{Name: "None", Lexer: LexerNone}
}
