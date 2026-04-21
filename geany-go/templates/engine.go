package templates

import (
	"strings"
	"time"
)

// Global constants representing common Geany template tags.
const (
	TagDeveloper   = "{developer}"
	TagInitials    = "{initials}"
	TagMail        = "{mail}"
	TagVersion     = "{version}"
	TagYear        = "{year}"
	TagDate        = "{date}"
	TagTime        = "{time}"
	TagDatetime    = "{datetime}"
	TagCompany     = "{company}"
	TagProject     = "{project}"
	TagFilename    = "{filename}"
	TagDescription = "{description}"
	TagObcClass    = "{obc_class}" // Objective-C class name
)

// Preferences stores the user-defined variables used for template substitution.
// This typically maps directly from the [Templates] section of the Geany config.
type Preferences struct {
	Developer string
	Initials  string
	Mail      string
	Company   string
	Version   string
}

// Engine handles the processing and interpolation of text templates.
// It serves as the Go port for the logic originally found in `src/templates.c`.
type Engine struct {
	prefs *Preferences
}

// NewEngine initializes a template engine with the given user preferences.
func NewEngine(prefs *Preferences) *Engine {
	if prefs == nil {
		prefs = &Preferences{}
	}
	return &Engine{prefs: prefs}
}

// Process takes a raw template string (e.g., a file header or snippet) and replaces
// all known Geany {tags} with their dynamic or user-configured values.
func (e *Engine) Process(templateText string, filename string, projectName string) string {
	if templateText == "" {
		return ""
	}

	now := time.Now()

	// Create a map of substitutions to perform
	subs := map[string]string{
		TagDeveloper: e.prefs.Developer,
		TagInitials:  e.prefs.Initials,
		TagMail:      e.prefs.Mail,
		TagCompany:   e.prefs.Company,
		TagVersion:   e.prefs.Version,
		TagYear:      now.Format("2006"),
		TagDate:      now.Format("2006-01-02"), // ISO format default
		TagTime:      now.Format("15:04:05"),
		TagDatetime:  now.Format("2006-01-02 15:04:05"),
		TagProject:   projectName,
		TagFilename:  filename,
		TagDescription: "TODO: Description", // In Geany this often prompts the user
	}

	// Apply all substitutions
	result := templateText
	for tag, replacement := range subs {
		result = strings.ReplaceAll(result, tag, replacement)
	}

	// Handle specialized logic like Objective-C class names
	// If `{obc_class}` is present, we attempt to derive it from the filename (removing extension).
	if strings.Contains(result, TagObcClass) {
		class := strings.TrimSuffix(filename, ".m")
		class = strings.TrimSuffix(class, ".h")
		if class == "" {
			class = "Untitled"
		}
		result = strings.ReplaceAll(result, TagObcClass, class)
	}

	return result
}
