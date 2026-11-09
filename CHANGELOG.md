# Changelog

All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [1.0.0-alpha.29] - BobUI (Qt6) Rich Component Integration
- Implemented `geany-go/ui/bobui/components.go` with `TabWidget` (supporting multi-line vertical/horizontal layout), `StatusBar` (displaying caret coordinates, encoding, filetype), and `SearchBar` (incremental find & replace).
- Integrated components into `bobui.Window` and `bobui.Editor`.
- Added comprehensive component unit tests in `geany-go/ui/bobui/application_test.go`.
- Validated all tests across Go and Rust project modules.

## [1.0.0-alpha.28] - FFI UI Frontend Selection Bridge Integration
- Exported `GeanyGo_UI_SetActiveFrontend` and `GeanyGo_UI_GetActiveFrontend` in `geany-go/main.go` and declared them in `src/Application_C_Bridge.h`.
- Initialized default native UI frontends (`bobgui`, `bobui`, `btk`) in the global `ui.Registry`.
- Validated all Go and Rust tests pass cleanly.

## [1.0.0-alpha.27] - Session Management & Configuration Requirements Specification
- Authored `docs/ai/requirements/2026-04-07-geany-session-management-and-configuration-requirements.md` outlining functional requirements (snapshotting, position persistence, metadata), non-functional requirements (latency bounds, thread safety), and Notepad++ parity comparisons.

## [1.0.0-alpha.26] - Session Management & Configuration Testing Documentation
- Authored `docs/ai/testing/2026-04-07-geany-session-management-and-configuration.md` establishing the test matrix, automated scenarios (round-trip INI serialization, concurrent event notifications, corrupted session recovery), and CI/CD verification commands.

## [1.0.0-alpha.25] - Multi-Toolkit Frontend Adapter Registry
- Implemented `geany-go/ui/registry.go` providing a thread-safe `Registry` for managing and dynamically switching active native UI frontends (`FrontendBobGUI`, `FrontendBobUI`, `FrontendBTK`).
- Added `geany-go/ui/registry_test.go` with mock application implementations to prevent import cycles.
- Validated all tests across Go and Rust project modules.

## [1.0.0-alpha.24] - Session Management & Configuration User/Developer Guide
- Authored `docs/ai/planning/2026-04-07-geany-session-management-user-guide.md` providing end-user instructions, configuration schema examples, CLI options, developer FFI examples, and session recovery procedures.
- Documented cross-toolkit session restoration mechanisms across GTK (`bobgui`), Qt6 (`bobui`), and CopperSpice (`btk`).

## [1.0.0-alpha.23] - Session Management and Configuration Architecture Documentation
- Authored comprehensive architectural design documentation for session management and configuration in `docs/ai/design/2026-04-07-geany-session-management-and-configuration.md`.
- Authored detailed implementation notes in `docs/ai/implementation/2026-04-07-geany-session-management-and-configuration.md`.
- Documented data flow between C++ `ConfigManager`, Go `geany-go/config`, FFI bridges, and UI submodules (`bobgui`, `bobui`, `btk`).
- Specified schema for global preferences, active session indexes, and per-document states (encodings, cursor/selection bounds, bookmarks, folds).

## [1.0.0-alpha.22] - Session Management and Configuration Architecture Design
- Authored initial architectural design documentation for session management and configuration in `docs/ai/design/2026-04-07-geany-session-management-and-configuration.md`.

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

## [1.0.0-alpha.11] - 2026-04-10
### Changed
- Replaced legacy C config file serialization (`src/keyfile.c`) with a modern, memory-safe C++ `ConfigManager`.
- Hooked `ConfigManager` into the primary `Application` orchestration structure.

## [1.0.0-alpha.10] - 2026-04-10
### Changed
- Implemented `geany-go/editor/Selection` struct to manage text buffer highlighting ranges securely.

## [1.0.0-alpha.9] - 2026-04-10
### Changed
- Implemented `SetModified()` and `IsModified()` methods for the `geany-go/editor/Document` struct, mimicking Geany's document state behavior.

## [1.0.0-alpha.8] - 2026-04-10
### Changed
- Implemented robust `Save()` and `Open()` file I/O methods for the `geany-go/editor/Document` struct.
- Added UTF-8 encoding validation and Byte-Order Mark (BOM) preservation across document saves.

## [1.0.0-alpha.7] - 2026-04-10
### Changed
- Bootstrapped `bobgui` submodule to serve as the Native GTK UI frontend.
- Implemented `geany-go/ui` interfaces for `bobgui` (`Window`, `Application`, `EditorWidget`).
- Injected `bobgui` package into `geany-go` core initialization sequence.

## [1.0.0-alpha.6] - 2026-04-10
### Changed
- Wired the newly created C++ `Application` core into the main execution pipeline (`src/main.c`) utilizing a safe C-to-C++ Bridge wrapper.
- The Geany executable now securely bootstraps the new C++ and Go subsystems prior to spinning up the legacy GTK loop.

## [1.0.0-alpha.5] - 2026-04-10
### Changed
- Wired C++ `Application` and Go `scintilla` backend through the CGO FFI bridge via `GeanyGo_Scintilla_Bind`.

## [1.0.0-alpha.4] - 2026-04-10
### Changed
- Created `geany-go/scintilla` package with robust CGO bindings to the native C++ Scintilla widget API.

## [1.0.0-alpha.3] - 2026-04-10
### Changed
- Created robust `geany-go/editor/editor.go` to replace legacy C global document arrays with a modern, thread-safe Go `Editor` struct.
- Created `geany-go/editor/cursor.go` tracking insertion points and text selections.

## [1.0.0-alpha.2] - 2026-04-10
### Changed
- Created modern C++ `PrintManager` and ported to `geany-go/project` interface, abstracting GTK printing APIs.

## [1.0.0-alpha.1] - 2026-04-10
### Changed
- Designed robust `geany-go/engine` package defining the core backend abstractions needed by Native UI text editor widgets.
- Created modern C++ `SearchManager` coordinating search results across `DocumentManager` and logging to `MsgWindow`.
- Implemented thread-safe Go `nav` package replicating IDE cursor navigation history (back/forward).
- Created modern C++ `MsgWindow` class to orchestrate structured logging output (`Compiler`, `Search`, `Message`) via `std::vector`.
- Implemented native Go `macros` recording and playback engine to achieve Notepad++ feature parity.
- Created modern C++ `Application` class orchestrating all newly refactored C++ subsystem managers.
- Implemented robust Go `symbols` package replicating ctags workspace navigation and querying.
- Created modern C++ `ProjectTreeView` data model abstracting legacy GTK sidebar nodes.
- Implemented Go `templates` package mapping legacy {tags} natively.
- Created modern C++ `SyntaxHighlighter` class that orchestrates syntax configurations dynamically between FileTypeManager and ScintillaWrapper.
- Implemented robust Go `filetypes` package for parsing syntax highlighting definitions based on file extensions.
- Created modern C++ `FileTypeManager` class utilizing `std::map` to replace static `GeanyFiletype` global structs.
- Created robust Go `config` package for parsing and generating INI/Keyfile format configuration files.
- Created C++ `KeybindingManager` class to encapsulate keyboard shortcut routing via `std::map` and `std::function`.
- Designed `geany-go/build` execution engine, porting synchronous command execution logic with modern Go context timeouts.
- Created C++ `ProjectManager` class to encapsulate project state and paths safely via `std::unique_ptr`.
- Implemented robust Go `search` package replicating basic find and regex search capabilities.
- Created modern C++ `DocumentManager` class utilizing `std::vector` and `std::unique_ptr` to replace global C document array.
- Designed `geany-go/ui` interface package bridging the Go backend with agnostic UI frontend submodules.
- Created C++ `PluginManager` class replacing bare C constructs (`GSList`) with robust STL containers (`std::map`, `std::unique_ptr`).
- Introduced modern, type-safe C++ `ScintillaWrapper` class to replace bare C Scintilla messages.
- Updated build configurations to include `ScintillaWrapper.cpp`, `PluginManager.cpp`, and `DocumentManager.cpp`.
- Updated `meson.build` and `configure.ac` to dynamically read version from `VERSION.md`.
- Added C++ refactoring plans to `TODO.md` and updated `HANDOFF.md` tracking.
