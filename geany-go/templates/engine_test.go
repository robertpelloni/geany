package templates

import (
	"strings"
	"testing"
	"time"
)

func TestEngineProcessing(t *testing.T) {
	prefs := &Preferences{
		Developer: "Jules AI",
		Initials:  "JAI",
		Mail:      "jules@example.com",
		Company:   "GeanyCorp",
		Version:   "1.0",
	}

	engine := NewEngine(prefs)

	rawTmpl := "/*\n * {filename}\n * \n * Copyright {year} {developer} <{mail}>\n * \n */\n"

	result := engine.Process(rawTmpl, "main.cpp", "GeanyProject")

	// Verification
	if !strings.Contains(result, "main.cpp") {
		t.Error("Failed to replace {filename}")
	}
	if !strings.Contains(result, "Jules AI") {
		t.Error("Failed to replace {developer}")
	}
	if !strings.Contains(result, "jules@example.com") {
		t.Error("Failed to replace {mail}")
	}

	currentYear := time.Now().Format("2006")
	if !strings.Contains(result, currentYear) {
		t.Errorf("Failed to replace {year} with %s", currentYear)
	}

	// Ensure old tags are gone
	if strings.Contains(result, "{developer}") || strings.Contains(result, "{year}") {
		t.Error("Raw tags still present in processed output")
	}
}

func TestEngineObcClass(t *testing.T) {
	engine := NewEngine(nil)
	rawTmpl := "@interface {obc_class} : NSObject"

	result := engine.Process(rawTmpl, "MyController.h", "")

	if !strings.Contains(result, "MyController") {
		t.Errorf("Failed to process {obc_class} from filename, got: %s", result)
	}
	if strings.Contains(result, ".h") {
		t.Error("Objective-C class name should not contain extension")
	}
}

func TestEngineEmpty(t *testing.T) {
	engine := NewEngine(nil)
	if res := engine.Process("", "", ""); res != "" {
		t.Errorf("Expected empty string, got '%s'", res)
	}
}
