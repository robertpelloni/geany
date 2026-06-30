package ui

import "testing"

func TestTabManager(t *testing.T) {
	mgr := NewDefaultTabManager()

	mgr.SetOrientation(Vertical)
	if mgr.GetOrientation() != Vertical {
		t.Errorf("Expected Vertical orientation, got %v", mgr.GetOrientation())
	}

	idx := mgr.AddTab("My Tab")
	if idx != 0 {
		t.Errorf("Expected tab index 0, got %d", idx)
	}

	if err := mgr.RemoveTab(idx); err != nil {
		t.Errorf("Failed to remove tab: %v", err)
	}
}
