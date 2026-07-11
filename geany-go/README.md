# Geany Go Backend (`geany-go`)

## Architectural Overview
This directory contains the primary backend orchestrator for the modernized Geany IDE.
As part of the Ultra-Project initiative, all C-based core editor logic (Project Management, Configurations, File I/O, Plugin Management) is methodically being rewritten into idiomatic, memory-safe, thread-safe Go.

## The C++ / CGO FFI Bridge
Because Geany historically relies on an intertwined GTK/C architecture, `geany-go` is compiled using `-buildmode=c-shared`.
This produces a dynamically linked shared library (`libgeanygo.so`) which the modernized `geany::Application` C++ object loads during bootstrap.
The Go orchestrator maintains internal `sync.Mutex` locks over the state of the editor.

## UI Agnosticism
A core philosophical design choice is strict separation of logic and presentation.
The Go backend interfaces with Native UIs exclusively through the abstract contracts defined in `ui/interfaces.go` (such as `ui.Application`, `ui.Window`, and `ui.EditorWidget`).
The native frontends (`bobui` Qt6, `btk` Qt4, `bobgui` GTK) implement these interfaces and register themselves with the backend.
This guarantees that Geany's core functionality behaves perfectly identically regardless of which UI renderer the user launches.

## Building
This backend is natively integrated into both the Autotools (`Makefile.am`) and Meson (`meson.build`) build graphs.

To build manually in isolation for testing:
```bash
go mod tidy
go test ./...
go build -buildmode=c-shared -o libgeanygo.so .
```
