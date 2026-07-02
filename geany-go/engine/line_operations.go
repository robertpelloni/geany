package engine

import (
	"fmt"
	"sort"
	"strings"
)

// SortLinesLexicographically sorts all lines in the document alphabetically.
// Fulfills the Notepad++ parity feature: Edit > Line Operations > Sort Lines Lexicographically
func (d *Document) SortLinesLexicographically(ascending bool) error {
	if d.Backend.IsReadOnly() {
		return fmt.Errorf("document is read-only")
	}

	text := d.Backend.GetText()

	// Normalize to detect line bounds accurately
	text = strings.ReplaceAll(text, "\r\n", "\n")
	lines := strings.Split(text, "\n")

	// Sort logic
	if ascending {
		sort.Strings(lines)
	} else {
		sort.Slice(lines, func(i, j int) bool {
			return lines[i] > lines[j]
		})
	}

	// Re-join with default unix newlines
	sortedText := strings.Join(lines, "\n")

	d.Backend.SetText(sortedText)
	d.Modified = true
	return nil
}

// ReverseLineOrder flips the order of all lines in the document (bottom to top).
// Fulfills the Notepad++ parity feature: Edit > Line Operations > Reverse Line Order
func (d *Document) ReverseLineOrder() error {
	if d.Backend.IsReadOnly() {
		return fmt.Errorf("document is read-only")
	}

	text := d.Backend.GetText()

	// Normalize newlines
	text = strings.ReplaceAll(text, "\r\n", "\n")
	lines := strings.Split(text, "\n")

	for i, j := 0, len(lines)-1; i < j; i, j = i+1, j-1 {
		lines[i], lines[j] = lines[j], lines[i]
	}

	reversedText := strings.Join(lines, "\n")

	d.Backend.SetText(reversedText)
	d.Modified = true
	return nil
}

// SplitLines breaks long lines down to a specified maximum column width.
// Fulfills Notepad++ Split Lines operation.
func (d *Document) SplitLines(maxWidth int) error {
	if d.Backend.IsReadOnly() {
		return fmt.Errorf("document is read-only")
	}
	if maxWidth <= 0 {
		return fmt.Errorf("invalid max width")
	}

	text := d.Backend.GetText()
	text = strings.ReplaceAll(text, "\r\n", "\n")
	lines := strings.Split(text, "\n")

	var sb strings.Builder

	for i, line := range lines {
		for len(line) > maxWidth {
			// Find the last space before the maxWidth boundary to avoid splitting words
			cutIndex := strings.LastIndex(line[:maxWidth], " ")
			if cutIndex == -1 {
				cutIndex = maxWidth // Hard split if no space is found
			}

			sb.WriteString(line[:cutIndex])
			sb.WriteString("\n")
			line = line[cutIndex:]

			// Trim leading space from the remainder
			line = strings.TrimLeft(line, " ")
		}
		sb.WriteString(line)
		if i < len(lines)-1 {
			sb.WriteString("\n")
		}
	}

	d.Backend.SetText(sb.String())
	d.Modified = true
	return nil
}

// JoinLines merges all selected lines (or all lines in document) into a single continuous line.
// Fulfills Notepad++ Join Lines operation.
func (d *Document) JoinLines() error {
	if d.Backend.IsReadOnly() {
		return fmt.Errorf("document is read-only")
	}

	text := d.Backend.GetText()
	text = strings.ReplaceAll(text, "\r\n", " ")
	text = strings.ReplaceAll(text, "\n", " ")
	text = strings.ReplaceAll(text, "\r", " ")

	// Collapse multiple spaces into one to clean up the join
	for strings.Contains(text, "  ") {
		text = strings.ReplaceAll(text, "  ", " ")
	}

	d.Backend.SetText(text)
	d.Modified = true
	return nil
}
