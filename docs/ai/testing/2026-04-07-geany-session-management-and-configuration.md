# Testing Notes: Geany Session Management & Configuration

## Overview

This document defines testing methodologies, automated test scenarios, validation fixtures, and edge-case verification for session persistence and configuration across Geany Ultra (`geany::ConfigManager`, `geany-go/config`, and UI adapters `bobgui`, `bobui`, `btk`).

---

## 1. Test Matrix & Execution Targets

| Component | Target File | Test Command | Verification Scope |
|---|---|---|---|
| C++ Config Manager | `src/ConfigManager.cpp` | Integrated into `src/Application.cpp` build | Typed accessor safety, whitespace trimming, INI section storage |
| Go Config Manager | `geany-go/config/manager.go` | `cd geany-go && go test -v ./config/...` | Round-trip serialization, missing keys fallback, comments skipping |
| Go UI Event Dispatcher | `geany-go/ui/events.go` | `cd geany-go && go test -v ./ui/...` | Thread concurrency, synchronous dispatch, event subscription |
| Frontend Adapters | `geany-go/ui/bobui`, `btk` | `cd geany-go && go test -v ./ui/bobui/... ./ui/btk/...` | Document lifecycle hooks, window show/hide, parity flags |
| Multi-Toolkit Registry | `geany-go/ui/registry.go` | `cd geany-go && go test -v ./ui/...` | Dynamic switching, registration, error on unknown frontend |

---

## 2. Automated Test Scenarios

### Scenario A: INI File Parsing & Round-Trip Serialization
- **Objective**: Verify that reading an existing configuration file, modifying keys, and saving it back to disk preserves all sections and values without loss or corruption.
- **Fixture**:
  ```ini
  [geany]
  tab_orientation=1
  theme=dark-ultra

  [session]
  active_document=/tmp/test.go
  document_count=1

  [document_0]
  path=/tmp/test.go
  encoding=UTF-8
  cursor_pos=42
  ```
- **Validation**:
  1. Load fixture with `Manager.Load()`.
  2. Confirm `GetString("session", "active_document") == "/tmp/test.go"`.
  3. Mutate `SetString("geany", "theme", "monokai")`.
  4. Save via `Manager.Save()` and verify disk representation.

### Scenario B: Concurrent Configuration Mutation & Event Notification
- **Objective**: Ensure that multiple goroutines can mutate configuration keys while UI event listeners receive updates synchronously without race conditions.
- **Validation**:
  - Validated by `geany-go/ui/events_test.go:TestEventDispatcher_Concurrent` dispatching across 20 concurrent goroutines with 10 listeners.

### Scenario C: Corrupted or Malformed Session Recovery
- **Objective**: Ensure graceful degradation when loading a malformed session file.
- **Test cases**:
  1. **Unclosed section headers**: `[unclosed_section` → parser continues and assigns keys safely to fallback section.
  2. **Missing `=` delimiter**: Bare words or incomplete key-value pairs are skipped without panic.
  3. **Deleted files**: Document files referenced in `session.conf` that do not exist on disk are handled with error logs without crashing the application lifecycle.

---

## 3. CI/CD Automated Verification Commands

Run the complete test validation pipeline:
```bash
# 1. Test all Go modules (config, editor, engine, parser, ui, textfx)
cd geany-go && go test -count=1 ./...

# 2. Test Rust parity algorithms (TextFX, vertical tabs)
cd ../geany-rust && cargo test

# 3. Check working directory status
cd .. && git status
```
