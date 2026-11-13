package ui

import (
	"sync"
	"testing"
)

func TestEventDispatcher_Basic(t *testing.T) {
	dispatcher := NewEventDispatcher()

	received := false
	dispatcher.Subscribe(EventDocOpened, func(ev UIEvent) {
		if ev.Payload == "test.txt" {
			received = true
		}
	})

	dispatcher.Dispatch(UIEvent{
		Type:    EventDocOpened,
		Payload: "test.txt",
	})

	if !received {
		t.Errorf("Expected event to be dispatched and received")
	}
}

func TestEventDispatcher_Concurrent(t *testing.T) {
	dispatcher := NewEventDispatcher()
	var wg sync.WaitGroup

	receivedCount := 0
	var mu sync.Mutex

	// Subscribe multiple listeners
	for i := 0; i < 10; i++ {
		dispatcher.Subscribe(EventSymbolsUpdated, func(ev UIEvent) {
			mu.Lock()
			receivedCount++
			mu.Unlock()
		})
	}

	// Dispatch concurrently
	concurrency := 20
	wg.Add(concurrency)
	for i := 0; i < concurrency; i++ {
		go func() {
			defer wg.Done()
			dispatcher.Dispatch(UIEvent{
				Type:    EventSymbolsUpdated,
				Payload: "symbols_update",
			})
		}()
	}

	wg.Wait()

	expected := 10 * concurrency
	if receivedCount != expected {
		t.Errorf("Expected %d total received events, got %d", expected, receivedCount)
	}

	// Test clear
	dispatcher.Clear()
	if count := dispatcher.ListenerCount(EventSymbolsUpdated); count != 0 {
		t.Errorf("Expected 0 listeners after Clear(), got %d", count)
	}
}
