package log

import (
	"bytes"
	"strings"
	"testing"
)

func TestLoggerLevels(t *testing.T) {
	var buf bytes.Buffer
	logger := NewLogger(&buf, LevelInfo)

	logger.Debug("Test", "This is a debug message")
	logger.Info("Test", "This is an info message")
	logger.Error("Core", "Fatal error occurred: %d", 404)

	output := buf.String()

	// Should NOT contain Debug
	if strings.Contains(output, "DEBUG") {
		t.Error("Logger output contained DEBUG message despite LevelInfo minimum")
	}

	// Should contain Info and Error
	if !strings.Contains(output, "[INFO] [Test] This is an info message") {
		t.Errorf("Logger output missing Info message: %s", output)
	}
	if !strings.Contains(output, "[ERROR] [Core] Fatal error occurred: 404") {
		t.Errorf("Logger output missing Error message: %s", output)
	}
}

func TestLoggerHistory(t *testing.T) {
	var buf bytes.Buffer
	logger := NewLogger(&buf, LevelDebug)

	logger.Compiler("gcc", "Compiling main.c")
	logger.Search("grep", "Found 5 matches")

	history := logger.GetHistory()
	if len(history) != 2 {
		t.Fatalf("Expected history length 2, got %d", len(history))
	}

	if history[0].Level != LevelCompiler || history[0].Message != "Compiling main.c" {
		t.Error("First history entry is incorrect")
	}
	if history[1].Level != LevelSearch || history[1].Message != "Found 5 matches" {
		t.Error("Second history entry is incorrect")
	}

	logger.ClearHistory()
	if len(logger.GetHistory()) != 0 {
		t.Error("History not cleared")
	}
}

func TestLoggerCapacity(t *testing.T) {
	var buf bytes.Buffer
	logger := NewLogger(&buf, LevelDebug)
	logger.maxHist = 5 // Override for test

	for i := 0; i < 10; i++ {
		logger.Info("", "Message %d", i)
	}

	history := logger.GetHistory()
	if len(history) != 5 {
		t.Fatalf("Expected history to truncate to 5, got %d", len(history))
	}

	// Should contain messages 5, 6, 7, 8, 9
	if history[0].Message != "Message 5" {
		t.Errorf("Expected oldest message in truncated history to be 'Message 5', got '%s'", history[0].Message)
	}
	if history[4].Message != "Message 9" {
		t.Errorf("Expected newest message in truncated history to be 'Message 9', got '%s'", history[4].Message)
	}
}
