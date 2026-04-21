package build

import (
	"testing"
	"time"
)

func TestNewCommand(t *testing.T) {
	cmd := NewCommand("go build -v", "/tmp")

	if cmd.Executable != "go" {
		t.Errorf("Expected executable 'go', got '%s'", cmd.Executable)
	}
	if len(cmd.Args) != 2 || cmd.Args[0] != "build" || cmd.Args[1] != "-v" {
		t.Errorf("Args parsed incorrectly: %v", cmd.Args)
	}
	if cmd.WorkingDir != "/tmp" {
		t.Errorf("WorkingDir set incorrectly: %s", cmd.WorkingDir)
	}
}

func TestExecuteSuccess(t *testing.T) {
	cmd := NewCommand("echo hello", "")
	res := cmd.Execute(2 * time.Second)

	if res.Error != nil {
		t.Errorf("Execution failed: %v", res.Error)
	}
	if res.ExitCode != 0 {
		t.Errorf("Expected exit code 0, got %d", res.ExitCode)
	}
	if res.Stdout != "hello\n" {
		t.Errorf("Expected 'hello\\n', got '%s'", res.Stdout)
	}
}

func TestExecuteTimeout(t *testing.T) {
	cmd := NewCommand("sleep 2", "")
	res := cmd.Execute(10 * time.Millisecond)

	if res.Error == nil {
		t.Error("Expected timeout error, got nil")
	}
}
