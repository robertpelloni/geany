package build

import (
	"context"
	"os/exec"
	"strings"
	"time"
)

// ExecutionResult represents the outcome of running a build/lint command.
type ExecutionResult struct {
	Stdout   string
	Stderr   string
	ExitCode int
	Error    error
	Duration time.Duration
}

// Command encapsulates an external executable call, mirroring Geany's build commands.
type Command struct {
	Executable string
	Args       []string
	WorkingDir string
}

// NewCommand parses a raw command string (like "gcc -c %f") into a Command struct.
// For now, this is a very basic space-delimited parser. A robust port of Geany's
// shell argument parsing would handle quotes and escapes properly.
func NewCommand(rawCmd string, workingDir string) *Command {
	if strings.TrimSpace(rawCmd) == "" {
		return &Command{}
	}

	parts := strings.Fields(rawCmd) // Simplified splitting
	return &Command{
		Executable: parts[0],
		Args:       parts[1:],
		WorkingDir: workingDir,
	}
}

// Execute runs the command synchronously.
// This ports the core of `src/spawn.c` synchronous execution.
func (c *Command) Execute(timeout time.Duration) *ExecutionResult {
	start := time.Now()
	res := &ExecutionResult{ExitCode: -1}

	if c.Executable == "" {
		res.Error = exec.ErrNotFound
		return res
	}

	ctx, cancel := context.WithTimeout(context.Background(), timeout)
	defer cancel()

	cmd := exec.CommandContext(ctx, c.Executable, c.Args...)
	cmd.Dir = c.WorkingDir

	// In a real IDE, we'd pipe this to a terminal/message window asynchronously.
	// For this synchronous port, we capture it.
	outBuf, err := cmd.Output() // Captures stdout
	res.Stdout = string(outBuf)

	if err != nil {
		if exitError, ok := err.(*exec.ExitError); ok {
			res.Stderr = string(exitError.Stderr)
			res.ExitCode = exitError.ExitCode()
		}
		res.Error = err
	} else {
		res.ExitCode = 0
	}

	res.Duration = time.Since(start)
	return res
}
