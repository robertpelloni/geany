package parser

import (
	"testing"

	"github.com/geany/geany-go/symbols"
)

func TestParseGoSource(t *testing.T) {
	src := `package main

type Config struct {
	Name string
}

type Runner interface {
	Run() error
}

var GlobalCounter = 0

func CalculateSum(a int, b int) int {
	return a + b
}

func (c *Config) GetName() string {
	return c.Name
}
`
	p := NewParser()
	syms, err := p.ParseSource("main.go", src, "go")
	if err != nil {
		t.Fatalf("Unexpected error parsing Go source: %v", err)
	}

	foundMap := make(map[string]symbols.Kind)
	for _, s := range syms {
		foundMap[s.Name] = s.Kind
	}

	expected := map[string]symbols.Kind{
		"Config":        symbols.KindStruct,
		"Runner":        symbols.KindInterface,
		"GlobalCounter": symbols.KindVariable,
		"CalculateSum":  symbols.KindFunction,
		"GetName":       symbols.KindFunction,
	}

	for name, kind := range expected {
		k, ok := foundMap[name]
		if !ok {
			t.Errorf("Expected symbol %q not found in parsed symbols", name)
		} else if k != kind {
			t.Errorf("Symbol %q kind mismatch: expected %v, got %v", name, kind, k)
		}
	}
}

func TestParseCPPSource(t *testing.T) {
	src := `#include <iostream>

class EditorWindow {
public:
	void Show();
};

struct TabItem {
	int id;
};

void InitializeSystem(int mode) {
	if (mode > 0) {
		std::cout << "Starting\n";
	}
}
`
	p := NewParser()
	syms, err := p.ParseSource("editor.cpp", src, "cpp")
	if err != nil {
		t.Fatalf("Unexpected error parsing C++ source: %v", err)
	}

	foundMap := make(map[string]symbols.Kind)
	for _, s := range syms {
		foundMap[s.Name] = s.Kind
	}

	if k, ok := foundMap["EditorWindow"]; !ok || k != symbols.KindClass {
		t.Errorf("Expected class EditorWindow, got %v", k)
	}
	if k, ok := foundMap["TabItem"]; !ok || k != symbols.KindStruct {
		t.Errorf("Expected struct TabItem, got %v", k)
	}
	if k, ok := foundMap["InitializeSystem"]; !ok || k != symbols.KindFunction {
		t.Errorf("Expected function InitializeSystem, got %v", k)
	}
	// Verify control flow 'if' was not identified as a function
	if _, ok := foundMap["if"]; ok {
		t.Errorf("Control statement 'if' incorrectly identified as a symbol")
	}
}

func TestParsePythonSource(t *testing.T) {
	src := `class DocumentManager:
    def __init__(self):
        pass

def calculate_checksum(data):
    return hash(data)
`
	p := NewParser()
	syms, err := p.ParseSource("manager.py", src, "python")
	if err != nil {
		t.Fatalf("Unexpected error parsing Python source: %v", err)
	}

	foundMap := make(map[string]symbols.Kind)
	for _, s := range syms {
		foundMap[s.Name] = s.Kind
	}

	if k, ok := foundMap["DocumentManager"]; !ok || k != symbols.KindClass {
		t.Errorf("Expected class DocumentManager, got %v", k)
	}
	if k, ok := foundMap["__init__"]; !ok || k != symbols.KindFunction {
		t.Errorf("Expected function __init__, got %v", k)
	}
	if k, ok := foundMap["calculate_checksum"]; !ok || k != symbols.KindFunction {
		t.Errorf("Expected function calculate_checksum, got %v", k)
	}
}

func TestParseMalformedSource(t *testing.T) {
	// Source with incomplete quotes, broken syntax, unbalanced braces
	src := `func BrokenMethod(a, b {
	var "unclosed string
	type Incomplete
	/* unclosed comment
`
	p := NewParser()
	syms, err := p.ParseSource("broken.go", src, "go")
	if err != nil {
		t.Fatalf("Parser should not crash on broken source, got error: %v", err)
	}
	// Check that BrokenMethod was safely identified despite subsequent errors
	found := false
	for _, s := range syms {
		if s.Name == "BrokenMethod" {
			found = true
			break
		}
	}
	if !found {
		t.Errorf("Expected BrokenMethod to be identified even in malformed source")
	}
}
