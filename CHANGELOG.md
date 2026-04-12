# Changelog

All notable changes to this project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [1.0.0-alpha.1] - 2026-04-10
### Changed
- Introduced modern, type-safe C++ `ScintillaWrapper` class to replace bare C Scintilla messages.
- Updated build configurations to include `ScintillaWrapper.cpp`.
- Added Scintilla C++ refactoring plan to `TODO.md`.
- Bootstrapped `geany-go/editor` package with core `Document` struct ported from C.
### Changed
- Introduced modern, type-safe C++ `ScintillaWrapper` class to replace bare C Scintilla messages.
- Updated build configurations to include `ScintillaWrapper.cpp`.
- Updated `meson.build` and `configure.ac` to dynamically read version from `VERSION.md`.
- Bootstrapped `geany-go/utils` package with string and file utility functions ported from C.
### Added
- Initialized comprehensive project documentation suite (VISION, ROADMAP, TODO, MEMORY, DEPLOY, HANDOFF, AGENTS, etc.).
- Prepared architecture for C++ refactor and Go port (`geany-go`).
- Prepared addition of bobui, btk, and bobgui submodules for multiple native UI frontends.
