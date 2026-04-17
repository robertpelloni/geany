package scintilla

import (
	"testing"
)

// We can only test the instantiation without a crash in a headless environment,
// because we don't have a real Scintilla memory block.
func TestScintillaInstantiation(t *testing.T) {
	editor := NewScintillaEditor(0, 0)
	if editor == nil {
		t.Fatalf("Failed to initialize ScintillaEditor struct")
	}

	// Sending a message with a nil function pointer should safely return 0
	// without causing a segfault across the CGO boundary.
	res := editor.SendMessage(0, 0, 0)
	if res != 0 {
		t.Errorf("Expected 0 return for nil function pointer, got %d", res)
	}
}
