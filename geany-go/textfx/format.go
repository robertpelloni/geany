package textfx

import (
	"strings"
)

// TrimTrailingWhitespace removes trailing whitespace from every line in the text.
func TrimTrailingWhitespace(text string) string {
	lines := strings.Split(text, "\n")
	for i, line := range lines {
		lines[i] = strings.TrimRight(line, " \t\r")
	}
	return strings.Join(lines, "\n")
}

// TrimLeadingWhitespace removes leading whitespace from every line in the text.
func TrimLeadingWhitespace(text string) string {
	lines := strings.Split(text, "\n")
	for i, line := range lines {
		lines[i] = strings.TrimLeft(line, " \t\r")
	}
	return strings.Join(lines, "\n")
}

// RemoveDuplicateLines removes consecutive duplicate lines.
func RemoveDuplicateLines(text string) string {
	lines := strings.Split(text, "\n")
	if len(lines) == 0 {
		return text
	}

	var result []string
	result = append(result, lines[0])

	for i := 1; i < len(lines); i++ {
		if lines[i] != lines[i-1] {
			result = append(result, lines[i])
		}
	}

	return strings.Join(result, "\n")
}
