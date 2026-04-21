package utils

import (
	"os"
	"path/filepath"
)

// FileExists checks if a given file path exists and is not a directory.
func FileExists(path string) bool {
	info, err := os.Stat(path)
	if os.IsNotExist(err) {
		return false
	}
	return !info.IsDir()
}

// DirExists checks if a given path exists and is a directory.
func DirExists(path string) bool {
	info, err := os.Stat(path)
	if os.IsNotExist(err) {
		return false
	}
	return info.IsDir()
}

// GetBaseName returns the last element of path.
// This is a wrapper around filepath.Base.
func GetBaseName(path string) string {
	return filepath.Base(path)
}
