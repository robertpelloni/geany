# Changelog

All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

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
