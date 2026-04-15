package project

import (
	"testing"
)

func TestProjectManager(t *testing.T) {
	mgr := NewManager()

	if mgr.GetActive() != nil {
		t.Error("Expected no active project initially")
	}

	// Test Create
	p, err := mgr.Create("Geany-Go", "/tmp/geany.geany", "/tmp/")
	if err != nil {
		t.Fatalf("Failed to create project: %v", err)
	}

	if p.Name != "Geany-Go" {
		t.Errorf("Expected name 'Geany-Go', got '%s'", p.Name)
	}

	active := mgr.GetActive()
	if active == nil || active.Name != "Geany-Go" {
		t.Error("Active project not set correctly")
	}

	// Test Build Steps
	err = mgr.AddBuildStep("make", "Make All", "make -j4", "./")
	if err != nil {
		t.Fatalf("Failed to add build step: %v", err)
	}

	if step, ok := mgr.GetActive().BuildSteps["make"]; !ok || step.Command != "make -j4" {
		t.Error("Build step not stored correctly")
	}

	// Test Close
	mgr.Close()
	if mgr.GetActive() != nil {
		t.Error("Expected no active project after close")
	}
}

func TestProjectLoad(t *testing.T) {
	mgr := NewManager()

	p, err := mgr.Load("/home/user/workspace/AwesomeApp.geany")
	if err != nil {
		t.Fatalf("Failed to load project: %v", err)
	}

	if p.Name != "AwesomeApp" {
		t.Errorf("Expected name extracted from file 'AwesomeApp', got '%s'", p.Name)
	}
	if p.BasePath != "/home/user/workspace" {
		t.Errorf("Expected base path '/home/user/workspace', got '%s'", p.BasePath)
	}
}

func TestProjectErrors(t *testing.T) {
	mgr := NewManager()

	_, err := mgr.Create("", "", "")
	if err == nil {
		t.Error("Expected error creating project with empty name")
	}

	err = mgr.AddBuildStep("test", "Test", "go test", "")
	if err == nil {
		t.Error("Expected error adding build step with no active project")
	}
}
