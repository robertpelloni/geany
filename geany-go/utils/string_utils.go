package utils

import (
	"strings"
	"unicode"
)

// IsEmpty checks if a string is empty or contains only whitespace.
// This is a common utility function in C/Geany that is safer and more robust in Go.
func IsEmpty(s string) bool {
	return strings.TrimSpace(s) == ""
}

// StrReplace replaces all occurrences of search in subject with replace.
// Go's strings.ReplaceAll handles this natively, but this wrapper maintains API similarity if needed.
func StrReplace(subject, search, replace string) string {
	return strings.ReplaceAll(subject, search, replace)
}

// Capitalize capitalizes the first letter of a string and lowercases the rest.
func Capitalize(s string) string {
	if s == "" {
		return ""
	}
	r := []rune(s)
	r[0] = unicode.ToUpper(r[0])
	for i := 1; i < len(r); i++ {
		r[i] = unicode.ToLower(r[i])
	}
	return string(r)
}
