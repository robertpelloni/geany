# Project Roadmap

This document outlines the major, long-term structural plans for the project.

## Phase 1: Deep Analysis and Foundation (Current)
- [x] Establish comprehensive documentation suite (VISION, ROADMAP, TODO, MEMORY, DEPLOY, HANDOFF, AGENTS, etc.).
- [x] Add git submodules (`bobui`, `btk`, `bobgui`).
- [x] Bootstrap the Go port (`geany-go`).
- [x] Conduct comprehensive 1:1 feature parity analysis with Notepad++.

## Phase 2: C++ Refactoring & Native UIs
- [ ] Begin systematic refactoring of existing C codebase to modern, clean C++.
- [ ] Integrate `bobui` (Qt6) as a native UI frontend.
- [ ] Integrate `btk` (Qt4) as a native UI frontend.
- [ ] Integrate `bobgui` (GTK) as a native UI frontend.

## Phase 3: The Go Port (`geany-go`)
- [ ] Methodically port all core engine logic to Go.
- [ ] Port UI logic and integrations to Go.
- [ ] Assimilate features from all submodules into the Go ultra-project.

## Phase 4: Notepad++ Parity & Feature Completion
- [ ] Implement all missing features identified in the Notepad++ parity analysis.
- [ ] Ensure 100% UI representation for all features (tooltips, labels, menus).
- [ ] Implement all ideas from IDEAS.md generated during deep repo analysis.

## Phase 5: Polish and Documentation
- [ ] Finalize user manuals and help files.
- [ ] Double and triple check all functions for bugs.
- [ ] Prepare Web UI frontend architecture.
