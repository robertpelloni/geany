package textfx

import (
	"sort"
	"strings"
)

// SortLines splits text into lines and sorts them.
// ascending dictates the sort direction.
// caseSensitive dictates whether case matters during sorting.
func SortLines(text string, ascending bool, caseSensitive bool) string {
	lines := strings.Split(text, "\n")

	sort.Slice(lines, func(i, j int) bool {
		a := lines[i]
		b := lines[j]

		if !caseSensitive {
			a = strings.ToLower(a)
			b = strings.ToLower(b)
		}

		if ascending {
			return a < b
		}
		return a > b
	})

	return strings.Join(lines, "\n")
}
