# Changelog

All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [1.0.0-alpha.22] - Session Management and Configuration Architecture Documentation
- Authored comprehensive architectural design documentation for session management and configuration in `docs/ai/design/2026-04-07-geany-session-management-and-configuration.md`.
- Documented data flow between C++ `ConfigManager`, Go `geany-go/config`, FFI bridges, and UI submodules (`bobgui`, `bobui`, `btk`).
- Specified schema for global preferences, active session indexes, and per-document states (encodings, cursor/selection bounds, bookmarks, folds).

## [1.0.0-alpha.21] - UI Submodule Integration Testing & Lifecycle Validation
- Added comprehensive unit tests for `geany-go/ui/bobui` (`application_test.go`) validating Qt6 lifecycle, window operations, document bindings, and Notepad++ parity toggles (MultiLineTabs, DocumentMap).
- Added comprehensive unit tests for `geany-go/ui/btk` (`application_test.go`) validating Qt4/CopperSpice lifecycle, window operations, document bindings, and Notepad++ parity toggles.
- Validated test suite across all `geany-go` and `geany-rust` modules.

## [1.0.0-alpha.20] - Go Parser & UI Interaction Port
- Created `geany-go/parser` package (`lexer.go`, `parser.go`) with thread-safe lexical analysis and multi-language symbol extraction (Go, C/C++, Python, generic) feeding `symbols.Workspace`.
- Implemented `geany-go/ui/events.go` with thread-safe `EventDispatcher` coordinating editor and UI lifecycle events (`EventDocOpened`, `EventSymbolsUpdated`, etc.).
- Fixed syntax in `geany-go/main.go` and implemented missing TextFX utilities (`RemoveBlankLines`, `EncodeBase64`, `DecodeBase64`) in `geany-go/textfx`.
- Exported FFI functions `GeanyGo_UI_DispatchEvent` and `GeanyGo_Parser_ParseAndIndex` in `geany-go/main.go`, and declared them in `src/Application_C_Bridge.h` and `src/Application.cpp`.
- Added comprehensive unit tests in `geany-go/parser` and `geany-go/ui`.

## [1.0.0-alpha.19] - Core Go Editor Bootstrapping
- Bootstrapped `geany-go/editor/search.go` to begin handling Find/Replace engine logic.
- Bootstrapped `geany-go/editor/cursor.go` defining the `Selection` logic and Scintilla proxying for caret position.
- Appended final styling and macro details to the `NOTEPAD_PLUS_PARITY_ANALYSIS.md`.

## [1.0.0-alpha.18] - Phase 1 Final Parity Expansion
- Added expanded analysis of core missing plugins, auto-completion architecture, and Folder as Workspace implementations into `NOTEPAD_PLUS_PARITY_ANALYSIS.md`.
- Finalized Phase 1 documentation tasks.

## [1.0.0-alpha.17] - Go Architecture Stabilization
- Fixed build errors in `geany-go/filetypes` module.
- Implemented the `engine.Backend` UI-agnostic abstraction in `geany-go/engine/editor.go` to safely decouple Scintilla from the main backend.
- Created `geany-go/scintilla/scintilla_backend.go` which successfully maps CGO C++ Scintilla pointers into the safe Go `Backend` interface.
- Version bumped to prepare for extensive Phase 3 Go port assimilation.

## [1.0.0-alpha.16] - Pre-Phase 2 Re-Sync
- Validated upstream parity and verified submodules are cleanly merged and initialized.
- Merged local phase 1 progress against upstream.

## [1.0.0-alpha.15] - Go Editor Expansion
- Began implementation of core `Document` logic (`InsertText`, `GetText`) in `geany-go/editor/document_features.go`.
- Incremented version for phase continuation.

## [1.0.0-alpha.14] - Extended TextFX Parity
- Implemented `remove_blank_lines`, `remove_duplicate_lines`, `encode_base64`, and `decode_base64` in `geany-go/textfx`.
- Implemented equivalent functions in `geany-rust/src/textfx.rs` using the `base64` crate.
- Exposed new TextFX functions across the FFI bridge in `Application_C_Bridge.h`.

## [1.0.0-alpha.13] - AI Dev Automation Synchronization
- Performed a comprehensive remote and local git workspace synchronization.
- Consolidated progress from the `jules-8901070248151919093-615e712e` feature branch (which included TextFX and Tab formatting for Rust and Go) into `master`.

## [1.0.0-alpha.12] - 2026-07-02
### Added
- Explicit Phase 3 target documentation tracking in TODO.md.
- Dashboard UI tracking mappings linking features directly to UI elements in `src/DashboardHelper.cpp`.

### Changed
- Mapped explicit GTK code extraction goals to `bobgui` submodule logic.
- Logged finalized Phase 1 submodule state synchronization check to MEMORY.md.
- Initiated \`geany-rust\` port for high-performance functionality.
- Implemented TextFX2 features (Sort Lines, Proper Case, Sentence Case, Formatting) in both Rust and Go ports.
- Implemented foundational Vertical Tabs interface via \`TabManager\` in Rust and Go.
- Connected \`geany-rust\` to the Meson build system as a dynamic/static C dependency.
- Wired Rust and Go TextFX/UI functionalities into the C++ \`geany::Application\` FFI bridge.
