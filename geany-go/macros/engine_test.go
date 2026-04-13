package macros

import (
	"testing"
)

func TestMacroRecordingAndPlayback(t *testing.T) {
	var executed []Action
	executor := func(a Action) error {
		executed = append(executed, a)
		return nil
	}

	engine := NewEngine(executor)

	// Test Start
	if err := engine.StartRecording(); err != nil {
		t.Fatalf("Failed to start recording: %v", err)
	}
	if !engine.IsRecording() {
		t.Error("Expected IsRecording to be true")
	}

	// Test Coalescing text inserts
	engine.RecordAction(ActionTypeTextInsert, "H")
	engine.RecordAction(ActionTypeTextInsert, "e")
	engine.RecordAction(ActionTypeTextInsert, "l")
	engine.RecordAction(ActionTypeTextInsert, "lo")

	// Test other actions
	engine.RecordAction(ActionTypeKeyPress, "<Enter>")
	engine.RecordAction(ActionTypeCommand, "CMD_DELETE_LINE")

	// Test Stop
	actions, err := engine.StopRecording()
	if err != nil {
		t.Fatalf("Failed to stop recording: %v", err)
	}
	if engine.IsRecording() {
		t.Error("Expected IsRecording to be false")
	}

	// Verify coalescing
	if len(actions) != 3 {
		t.Fatalf("Expected 3 actions after coalescing, got %d", len(actions))
	}
	if actions[0].Payload != "Hello" {
		t.Errorf("Expected coalesced payload 'Hello', got '%s'", actions[0].Payload)
	}

	// Test Playback
	if err := engine.PlayCurrent(); err != nil {
		t.Fatalf("Failed to play current macro: %v", err)
	}

	if len(executed) != 3 {
		t.Fatalf("Expected 3 actions executed, got %d", len(executed))
	}
	if executed[0].Payload != "Hello" {
		t.Errorf("Executor received wrong payload: %s", executed[0].Payload)
	}
}

func TestMacroSaving(t *testing.T) {
	engine := NewEngine(nil)

	engine.StartRecording()
	engine.RecordAction(ActionTypeTextInsert, "Test")
	engine.StopRecording()

	if err := engine.SaveMacro("MyMacro"); err != nil {
		t.Fatalf("Failed to save macro: %v", err)
	}

	if err := engine.PlaySaved("MyMacro"); err != nil {
		t.Fatalf("Failed to play saved macro: %v", err)
	}

	if err := engine.PlaySaved("NonExistent"); err == nil {
		t.Error("Expected error playing non-existent macro")
	}
}

func TestEngineStateErrors(t *testing.T) {
	engine := NewEngine(nil)

	// Cannot stop if not recording
	if _, err := engine.StopRecording(); err == nil {
		t.Error("Expected error stopping when not recording")
	}

	engine.StartRecording()

	// Cannot double start
	if err := engine.StartRecording(); err == nil {
		t.Error("Expected error double starting recording")
	}

	// Cannot play while recording
	if err := engine.PlayCurrent(); err == nil {
		t.Error("Expected error playing while recording")
	}

	// Cannot save while recording
	if err := engine.SaveMacro("Test"); err == nil {
		t.Error("Expected error saving while recording")
	}
}
