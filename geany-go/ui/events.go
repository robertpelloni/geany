package ui

import (
	"sync"
)

// EventType enumerates all high-level UI and editor lifecycle events.
type EventType int

const (
	EventDocOpened EventType = iota + 1
	EventDocSaved
	EventDocClosed
	EventCursorMoved
	EventSelectionChanged
	EventSymbolsUpdated
	EventTabChanged
	EventThemeChanged
)

// UIEvent carries contextual metadata for a triggered UI interaction.
type UIEvent struct {
	Type    EventType
	Payload string
}

// EventListener is a callback function invoked when a matching UI event occurs.
type EventListener func(event UIEvent)

// EventDispatcher coordinates decoupled UI notifications across frontend submodules
// and the C++ application core. It is fully thread-safe.
type EventDispatcher struct {
	mu        sync.RWMutex
	listeners map[EventType][]EventListener
}

// NewEventDispatcher creates a new initialized dispatcher.
func NewEventDispatcher() *EventDispatcher {
	return &EventDispatcher{
		listeners: make(map[EventType][]EventListener),
	}
}

// Subscribe registers a listener callback for a given EventType.
func (d *EventDispatcher) Subscribe(eventType EventType, listener EventListener) {
	d.mu.Lock()
	defer d.mu.Unlock()
	d.listeners[eventType] = append(d.listeners[eventType], listener)
}

// Dispatch broadcasts an event synchronously to all registered listeners.
func (d *EventDispatcher) Dispatch(event UIEvent) {
	d.mu.RLock()
	defer d.mu.RUnlock()

	if list, ok := d.listeners[event.Type]; ok {
		for _, listener := range list {
			listener(event)
		}
	}
}

// ListenerCount returns the number of active listeners for a given event type.
func (d *EventDispatcher) ListenerCount(eventType EventType) int {
	d.mu.RLock()
	defer d.mu.RUnlock()
	return len(d.listeners[eventType])
}

// Clear removes all registered event listeners.
func (d *EventDispatcher) Clear() {
	d.mu.Lock()
	defer d.mu.Unlock()
	d.listeners = make(map[EventType][]EventListener)
}
