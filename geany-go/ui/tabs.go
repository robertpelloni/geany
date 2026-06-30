package ui

import "fmt"

// TabOrientation represents whether tabs are laid out horizontally or vertically.
type TabOrientation int

const (
	Horizontal TabOrientation = iota
	Vertical
)

// TabManager defines the agnostic interface for managing UI document tabs.
type TabManager interface {
	SetOrientation(orientation TabOrientation) error
	GetOrientation() TabOrientation
	AddTab(title string) int
	RemoveTab(index int) error
}

// DefaultTabManager is a stub implementation of the TabManager interface.
type DefaultTabManager struct {
	orientation TabOrientation
	tabs        []string
}

func NewDefaultTabManager() *DefaultTabManager {
	return &DefaultTabManager{
		orientation: Horizontal,
		tabs:        make([]string, 0),
	}
}

func (m *DefaultTabManager) SetOrientation(o TabOrientation) error {
	m.orientation = o
	// Real implementation would propagate this state down to the Qt/GTK frontend.
	fmt.Printf("[TabManager] Setting orientation to %v\n", o)
	return nil
}

func (m *DefaultTabManager) GetOrientation() TabOrientation {
	return m.orientation
}

func (m *DefaultTabManager) AddTab(title string) int {
	m.tabs = append(m.tabs, title)
	fmt.Printf("[TabManager] Added tab: %s\n", title)
	return len(m.tabs) - 1
}

func (m *DefaultTabManager) RemoveTab(index int) error {
	if index < 0 || index >= len(m.tabs) {
		return fmt.Errorf("invalid tab index: %d", index)
	}
	m.tabs = append(m.tabs[:index], m.tabs[index+1:]...)
	fmt.Printf("[TabManager] Removed tab at index %d\n", index)
	return nil
}
