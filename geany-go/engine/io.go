package engine

import (
	"fmt"
	"io"
	"os"
)

// ReadFile reads the entire contents of a file into a string using high-throughput Go buffers.
func ReadFile(path string) (string, error) {
	file, err := os.Open(path)
	if err != nil {
		return "", fmt.Errorf("failed to open file: %w", err)
	}
	defer file.Close()

	content, err := io.ReadAll(file)
	if err != nil {
		return "", fmt.Errorf("failed to read file: %w", err)
	}

	return string(content), nil
}

// WriteFile writes string content to a file, overwriting existing contents.
func WriteFile(path string, content string) error {
	file, err := os.Create(path)
	if err != nil {
		return fmt.Errorf("failed to create file: %w", err)
	}
	defer file.Close()

	_, err = file.WriteString(content)
	if err != nil {
		return fmt.Errorf("failed to write to file: %w", err)
	}

	return nil
}

// IsFileReadOnly checks the OS permissions of a file.
func IsFileReadOnly(path string) (bool, error) {
	info, err := os.Stat(path)
	if err != nil {
		return false, fmt.Errorf("failed to stat file: %w", err)
	}

	// Simple check: if we can't write, it's read-only.  More complex checks might be needed for Windows.
	mode := info.Mode()
	return mode.Perm()&(1<<(uint(7))) == 0, nil // Check owner write bit.
}
