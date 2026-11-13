package textfx

import (
	"encoding/base64"
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

// RemoveBlankLines strips lines that are entirely empty or consist only of whitespace characters.
func RemoveBlankLines(text string) string {
	lines := strings.Split(text, "\n")
	var result []string

	for _, line := range lines {
		if strings.TrimSpace(line) != "" {
			result = append(result, line)
		}
	}

	return strings.Join(result, "\n")
}

// EncodeBase64 encodes the input string into standard base64 format.
func EncodeBase64(text string) string {
	return base64.StdEncoding.EncodeToString([]byte(text))
}

// DecodeBase64 decodes a standard base64 string into plain text.
func DecodeBase64(text string) (string, error) {
	bytes, err := base64.StdEncoding.DecodeString(strings.TrimSpace(text))
	if err != nil {
		return "", err
	}
	return string(bytes), nil
}
