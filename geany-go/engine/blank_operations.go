package engine

import (
	"fmt"
	"strings"
)

// TrimTrailingSpace removes whitespace characters from the end of all lines in the document.
// This fulfills the "Edit > Blank Operations > Trim Trailing Space" Notepad++ parity feature.
func (d *Document) TrimTrailingSpace() error {
	if d.Backend.IsReadOnly() {
		return fmt.Errorf("document is read-only")
	}

	lineCount := d.Backend.GetLineCount()
	startOffset := 0

	for i := 0; i < lineCount; i++ {
		length := d.Backend.GetLineLength(i)
		if length > 0 {
			lineText := d.Backend.GetTextRange(startOffset, startOffset+length)

			// Separate newline chars before trimming so we don't accidentally remove blank lines entirely
			trimmedLine := strings.TrimRight(lineText, " \t")

			// Extract any trailing newlines (e.g. \r\n or \n) to preserve line structure
			newlines := ""
			if strings.HasSuffix(lineText, "\r\n") {
				newlines = "\r\n"
				trimmedLine = strings.TrimRight(lineText[:len(lineText)-2], " \t")
			} else if strings.HasSuffix(lineText, "\n") {
				newlines = "\n"
				trimmedLine = strings.TrimRight(lineText[:len(lineText)-1], " \t")
			}

			newLineText := trimmedLine + newlines

			if newLineText != lineText {
				d.Backend.DeleteRange(startOffset, startOffset+length)
				d.Backend.InsertText(startOffset, newLineText)
				// Adjust length to account for the deletion of spaces
				length = len(newLineText)
				d.Modified = true
			}
		}
		startOffset += length
	}

	return nil
}

// ConvertEOLToSpace converts all End-of-Line characters (\r\n, \n, \r) into a single space.
// This fulfills the "Edit > Blank Operations > EOL to Space" Notepad++ parity feature.
func (d *Document) ConvertEOLToSpace() error {
	if d.Backend.IsReadOnly() {
		return fmt.Errorf("document is read-only")
	}

	text := d.Backend.GetText()

	// Normalize Windows/Mac/Linux newlines
	text = strings.ReplaceAll(text, "\r\n", " ")
	text = strings.ReplaceAll(text, "\n", " ")
	text = strings.ReplaceAll(text, "\r", " ")

	d.Backend.SetText(text)
	d.Modified = true
	return nil
}

// ConvertSpaceToEOL converts instances of single or multiple spaces into newlines.
// This fulfills the "Edit > Blank Operations > Space to EOL" Notepad++ parity feature.
func (d *Document) ConvertSpaceToEOL() error {
	if d.Backend.IsReadOnly() {
		return fmt.Errorf("document is read-only")
	}

	text := d.Backend.GetText()

	// Convert spaces to newlines
	// Note: We might want to collapse multiple spaces into a single newline, but for raw parity,
	// we do exactly what the string says.
	text = strings.ReplaceAll(text, " ", "\n")

	d.Backend.SetText(text)
	d.Modified = true
	return nil
}
