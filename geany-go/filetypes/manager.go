package filetypes

import (
	"sync"
)

type FileType struct {
	Name        string
	Extension   string
	Icon        string
	LexerID     int
	Comments    string
	StringChars string
	WordChars   string
	Settings    map[string]string
}

type Manager struct {
	typesByExtension map[string]FileType
	typesByName      map[string]FileType
	mu               sync.RWMutex
}

func NewManager() *Manager {
	m := &Manager{
		typesByExtension: make(map[string]FileType),
		typesByName:      make(map[string]FileType),
	}
	m.registerDefaults()
	return m
}

func (m *Manager) registerDefaults() {
	m.Register(FileType{Name: "C++", Extension: "cpp", LexerID: 3, Comments: "//"})
	m.Register(FileType{Name: "C", Extension: "c", LexerID: 3, Comments: "//"})
	m.Register(FileType{Name: "Go", Extension: "go", LexerID: 3, Comments: "//"})
}

func (m *Manager) Register(t FileType) {
	m.mu.Lock()
	defer m.mu.Unlock()
	m.typesByExtension[t.Extension] = t
	m.typesByName[t.Name] = t
}

func (m *Manager) GetByExtension(ext string) FileType {
	m.mu.RLock()
	defer m.mu.RUnlock()
	if t, ok := m.typesByExtension[ext]; ok {
		return t
	}
	return FileType{Name: "None", LexerID: 0}
}
