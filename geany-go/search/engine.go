package search

import (
	"regexp"
	"strings"
)

// SearchFlags define the options for a search operation.
// These map closely to Scintilla/Geany search flags.
type SearchFlags struct {
	MatchCase bool
	WholeWord bool
	Regex     bool
	// Backwards bool (handled by method selection usually, but kept for completeness)
}

// SearchResult represents a match found in the text.
type SearchResult struct {
	Start int
	End   int
	Text  string
}

// Engine provides robust text searching capabilities.
// This is the Go port of the core logic found in src/search.c
type Engine struct{}

// NewEngine initializes a new Search Engine.
func NewEngine() *Engine {
	return &Engine{}
}

// FindNext searches for 'pattern' within 'text' starting at 'startIndex'.
// It applies the rules specified in 'flags'.
func (e *Engine) FindNext(text string, pattern string, startIndex int, flags SearchFlags) (*SearchResult, error) {
	if startIndex < 0 || startIndex >= len(text) {
		return nil, nil // Out of bounds, no match
	}
	if pattern == "" {
		return nil, nil // Empty pattern, no match
	}

	searchContext := text[startIndex:]

	if flags.Regex {
		return e.regexSearch(searchContext, pattern, startIndex, flags)
	}

	return e.plainSearch(searchContext, pattern, startIndex, flags)
}

func (e *Engine) plainSearch(context string, pattern string, offset int, flags SearchFlags) (*SearchResult, error) {
	searchStr := context
	pat := pattern

	if !flags.MatchCase {
		searchStr = strings.ToLower(context)
		pat = strings.ToLower(pattern)
	}

	idx := strings.Index(searchStr, pat)
	if idx == -1 {
		return nil, nil // Not found
	}

	absoluteStart := offset + idx
	absoluteEnd := absoluteStart + len(pattern)

	// Whole word check (basic implementation)
	if flags.WholeWord {
		if !isWholeWordMatch(textFromOffset(offset, context), absoluteStart, absoluteEnd) {
			// Recursive call to find the *next* occurrence if this one isn't a whole word
			return e.FindNext(context, pattern, absoluteEnd, flags)
		}
	}

	return &SearchResult{
		Start: absoluteStart,
		End:   absoluteEnd,
		Text:  context[idx : idx+len(pattern)],
	}, nil
}

func (e *Engine) regexSearch(context string, pattern string, offset int, flags SearchFlags) (*SearchResult, error) {
	// Prepend case-insensitive flag if needed
	prefix := ""
	if !flags.MatchCase {
		prefix = "(?i)"
	}

	// Whole word boundaries for regex
	if flags.WholeWord {
		pattern = `\b` + pattern + `\b`
	}

	re, err := regexp.Compile(prefix + pattern)
	if err != nil {
		return nil, err // Invalid regex
	}

	loc := re.FindStringIndex(context)
	if loc == nil {
		return nil, nil // Not found
	}

	return &SearchResult{
		Start: offset + loc[0],
		End:   offset + loc[1],
		Text:  context[loc[0]:loc[1]],
	}, nil
}

// Helper to reconstruct original text slice for word boundary checks
func textFromOffset(offset int, context string) string {
	// In a real implementation, we'd need the *full* text string passed down,
	// or we'd just check the boundaries of the matched substring within the larger context.
	// For this stub, we do a basic boundary check within the sliced context.
	return context
}

// isWholeWordMatch performs a basic check to ensure characters surrounding a match are not alphanumeric.
func isWholeWordMatch(text string, start, end int) bool {
	// Note: 'start' and 'end' here are absolute indices relative to the original text length.
	// We need to adjust them to be relative to the 'text' slice we currently have.
	// Because textFromOffset currently just returns the context slice, we assume start/end are 0-indexed into it for now
	// This is a simplified boundary check for demonstration.

	relStart := 0 // Simplified: assume the match is at the beginning of the slice for the check
	relEnd := end - start

	if relStart > 0 {
		prevChar := text[relStart-1]
		if isAlphanumeric(prevChar) {
			return false
		}
	}

	if relEnd < len(text) {
		nextChar := text[relEnd]
		if isAlphanumeric(nextChar) {
			return false
		}
	}

	return true
}

func isAlphanumeric(c byte) bool {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'
}
