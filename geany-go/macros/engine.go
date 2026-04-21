package macros

import (
	"fmt"
	"sync"
)

// ActionType represents the type of operation recorded in a macro.
type ActionType int

const (
	ActionTypeUnknown ActionType = iota
	ActionTypeTextInsert
	ActionTypeKeyPress
	ActionTypeCommand // Editor commands like Copy, Paste, DeleteLine
)

// Action represents a single step recorded during macro creation.
type Action struct {
	Type    ActionType
	Payload string // e.g., the text inserted, or the key string "<Ctrl>C"
}

// Macro represents a full sequence of recorded actions.
type Macro struct {
	Name    string
	Actions []Action
}

// Engine handles the recording, playback, and storage of Editor Macros.
// This fulfills a critical feature parity requirement with Notepad++.
type Engine struct {
	mu           sync.RWMutex
	isRecording  bool
	currentMacro []Action
	savedMacros  map[string]Macro

	// Executor is a callback provided by the frontend/editor core
	// to actually apply the actions back to the active document.
	executor func(action Action) error
}

// NewEngine initializes a thread-safe macro recording engine.
func NewEngine(executor func(Action) error) *Engine {
	if executor == nil {
		// Fallback stub executor
		executor = func(a Action) error { return nil }
	}
	return &Engine{
		savedMacros: make(map[string]Macro),
		executor:    executor,
	}
}

// StartRecording clears the current buffer and sets the engine to record mode.
func (e *Engine) StartRecording() error {
	e.mu.Lock()
	defer e.mu.Unlock()

	if e.isRecording {
		return fmt.Errorf("already recording a macro")
	}

	e.isRecording = true
	e.currentMacro = make([]Action, 0)
	return nil
}

// RecordAction captures an event if the engine is currently recording.
func (e *Engine) RecordAction(actionType ActionType, payload string) {
	e.mu.Lock()
	defer e.mu.Unlock()

	if !e.isRecording {
		return
	}

	// Optimize: coalesce rapid text inserts
	if actionType == ActionTypeTextInsert && len(e.currentMacro) > 0 {
		lastIdx := len(e.currentMacro) - 1
		lastAction := &e.currentMacro[lastIdx]

		if lastAction.Type == ActionTypeTextInsert {
			lastAction.Payload += payload
			return
		}
	}

	e.currentMacro = append(e.currentMacro, Action{Type: actionType, Payload: payload})
}

// StopRecording halts the recording process and returns the captured sequence.
func (e *Engine) StopRecording() ([]Action, error) {
	e.mu.Lock()
	defer e.mu.Unlock()

	if !e.isRecording {
		return nil, fmt.Errorf("not currently recording")
	}

	e.isRecording = false
	return e.currentMacro, nil
}

// SaveMacro permanently stores the most recently recorded macro under a given name.
func (e *Engine) SaveMacro(name string) error {
	e.mu.Lock()
	defer e.mu.Unlock()

	if e.isRecording {
		return fmt.Errorf("cannot save while recording")
	}
	if len(e.currentMacro) == 0 {
		return fmt.Errorf("current macro buffer is empty")
	}
	if name == "" {
		return fmt.Errorf("macro name cannot be empty")
	}

	e.savedMacros[name] = Macro{
		Name:    name,
		Actions: append([]Action(nil), e.currentMacro...), // Deep copy
	}
	return nil
}

// PlayReplay triggers the execution of the currently loaded macro buffer.
func (e *Engine) PlayCurrent() error {
	e.mu.RLock()
	actions := append([]Action(nil), e.currentMacro...) // Copy to avoid locking during execution
	isRecording := e.isRecording
	e.mu.RUnlock()

	if isRecording {
		return fmt.Errorf("cannot play macro while recording")
	}
	if len(actions) == 0 {
		return fmt.Errorf("no macro recorded")
	}

	for _, action := range actions {
		if err := e.executor(action); err != nil {
			return fmt.Errorf("macro playback failed at action %v: %w", action, err)
		}
	}
	return nil
}

// PlaySaved triggers the execution of a previously named and saved macro.
func (e *Engine) PlaySaved(name string) error {
	e.mu.RLock()
	macro, exists := e.savedMacros[name]
	isRecording := e.isRecording
	e.mu.RUnlock()

	if isRecording {
		return fmt.Errorf("cannot play macro while recording")
	}
	if !exists {
		return fmt.Errorf("saved macro '%s' not found", name)
	}

	for _, action := range macro.Actions {
		if err := e.executor(action); err != nil {
			return fmt.Errorf("macro playback failed at action %v: %w", action, err)
		}
	}
	return nil
}

// IsRecording returns the current state.
func (e *Engine) IsRecording() bool {
	e.mu.RLock()
	defer e.mu.RUnlock()
	return e.isRecording
}
