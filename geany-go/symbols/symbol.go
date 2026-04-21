package symbols

import (
	"strings"
	"sync"
)

// Kind represents the type of symbol found (e.g., Class, Function, Variable).
// Corresponds to ctags categories or Geany's internal bitmasks.
type Kind int

const (
	KindUnknown Kind = iota
	KindClass
	KindFunction
	KindVariable
	KindInterface
	KindStruct
	KindMacro
)

// Symbol represents a parsed source code tag (ctag).
type Symbol struct {
	Name      string
	File      string
	Line      int
	Kind      Kind
	Signature string // Optional context, like function parameters
}

// Workspace manages all symbols parsed across multiple files in a project.
// This is the Go equivalent of the global tag manager in src/symbols.c and src/tagmanager/
type Workspace struct {
	mu      sync.RWMutex
	symbols map[string][]Symbol // Mapped by filename
}

// NewWorkspace initializes a thread-safe registry for code symbols.
func NewWorkspace() *Workspace {
	return &Workspace{
		symbols: make(map[string][]Symbol),
	}
}

// AddFileSymbols registers a list of parsed symbols for a specific file.
// If symbols already exist for this file, they are replaced (representing a file reload).
func (w *Workspace) AddFileSymbols(filename string, syms []Symbol) {
	w.mu.Lock()
	defer w.mu.Unlock()
	w.symbols[filename] = syms
}

// ClearFile removes all symbols associated with a given filename (e.g., when a file is closed).
func (w *Workspace) ClearFile(filename string) {
	w.mu.Lock()
	defer w.mu.Unlock()
	delete(w.symbols, filename)
}

// ClearAll safely wipes the entire symbol workspace.
func (w *Workspace) ClearAll() {
	w.mu.Lock()
	defer w.mu.Unlock()
	w.symbols = make(map[string][]Symbol)
}

// FindByName searches the entire workspace for symbols matching the given name.
// For Go's idiomatic port, we allow exact matches or partial case-insensitive matches.
func (w *Workspace) FindByName(name string, exact bool) []Symbol {
	w.mu.RLock()
	defer w.mu.RUnlock()

	var results []Symbol
	searchName := name
	if !exact {
		searchName = strings.ToLower(name)
	}

	for _, fileSyms := range w.symbols {
		for _, sym := range fileSyms {
			match := false
			if exact {
				match = (sym.Name == searchName)
			} else {
				match = strings.Contains(strings.ToLower(sym.Name), searchName)
			}

			if match {
				results = append(results, sym)
			}
		}
	}
	return results
}

// GetFileSymbols returns a slice of all symbols contained within a specific file.
func (w *Workspace) GetFileSymbols(filename string) []Symbol {
	w.mu.RLock()
	defer w.mu.RUnlock()

	if syms, ok := w.symbols[filename]; ok {
		return syms
	}
	return nil
}
