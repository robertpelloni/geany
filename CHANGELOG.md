# Changelog

All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [1.0.0-alpha.13] - AI Dev Automation Synchronization
- Performed a comprehensive remote and local git workspace synchronization.
- Consolidated progress from the `jules-8901070248151919093-615e712e` feature branch (which included TextFX and Tab formatting for Rust and Go) into `master`.

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

### Added
- Initialized comprehensive project documentation suite (VISION, ROADMAP, TODO, MEMORY, DEPLOY, HANDOFF, AGENTS, etc.).
- Bootstrapped `geany-go/editor` package with core `Document` struct ported from C.
- Bootstrapped `geany-go/utils` package with string and file utility functions ported from C.
- Prepared architecture for C++ refactor and Go port (`geany-go`).
- Prepared addition of bobui, btk, and bobgui submodules for multiple native UI frontends.

## 1.0.0-alpha.12
- Initiated \`geany-rust\` port for high-performance functionality.
- Implemented TextFX2 features (Sort Lines, Proper Case, Sentence Case, Formatting) in both Rust and Go ports.
- Implemented foundational Vertical Tabs interface via \`TabManager\` in Rust and Go.
- Connected \`geany-rust\` to the Meson build system as a dynamic/static C dependency.
- Wired Rust and Go TextFX/UI functionalities into the C++ \`geany::Application\` FFI bridge.
