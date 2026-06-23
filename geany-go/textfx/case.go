package textfx

import (
	"strings"
	"unicode"
)

// ToProperCase converts text to Proper Case (Capitalize Every Word).
func ToProperCase(text string) string {
	var result []rune
	capitalizeNext := true

	for _, r := range text {
		if unicode.IsSpace(r) {
			result = append(result, r)
			capitalizeNext = true
		} else if capitalizeNext {
			result = append(result, unicode.ToTitle(r))
			capitalizeNext = false
		} else {
			result = append(result, unicode.ToLower(r))
		}
	}
	return string(result)
}

// ToSentenceCase converts text to Sentence case (Capitalize first word of a sentence).
func ToSentenceCase(text string) string {
	var result []rune
	capitalizeNext := true

	for _, r := range text {
		if capitalizeNext && unicode.IsLetter(r) {
			result = append(result, unicode.ToTitle(r))
			capitalizeNext = false
		} else {
			result = append(result, unicode.ToLower(r))
			if r == '.' || r == '!' || r == '?' {
				capitalizeNext = true
			} else if !unicode.IsSpace(r) {
				capitalizeNext = false // Reset just in case it was true but we hit a non-letter
			}
		}
	}
	return string(result)
}

// ToLowerCase converts the text to fully lowercase.
func ToLowerCase(text string) string {
	return strings.ToLower(text)
}

// ToUpperCase converts the text to fully uppercase.
func ToUpperCase(text string) string {
	return strings.ToUpper(text)
}
