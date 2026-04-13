# TODO

This file contains individual features, bug fixes, and other fine details that need to be solved/implemented in the short term.

## High Priority
- [x] Create `SUBMODULES.md` detailing the usage and architecture of `bobui`, `btk`, and `bobgui`.
- [x] Run `git submodule add` for the three UI submodules.
- [x] `mkdir geany-go` and run `go mod init github.com/geany/geany-go` (or similar).
- [x] Create `IDEAS.md` containing brainstorming and improvements across the codebase.
- [x] Document the results of the Notepad++ parity analysis here or in a separate file.

## Ongoing
- [ ] Refactor C files to C++.
- [ ] Port C/C++ functionality to Go.
- [ ] Update version strings across the codebase to read from `VERSION.md`.

## Notepad++ Parity Features (Identified in Analysis)
- [ ] Implement native Macro Recording & Playback.
- [ ] Implement Multi-line Tabs.
- [ ] Implement Synchronized Scrolling in Split View.
- [ ] Enhance "Mark All" search functionality to match N++ Bookmark/Mark tab.
- [ ] Native Document Map (Minimap) integration.
- [ ] Modernize Style/Theme Configurator (via UI submodules).

## C++ Refactoring Plan
- [x] **Scintilla C++ Wrapper**: Refactor `src/scintilla.c` and related Scintilla wrapper functions into a modern, robust C++ `ScintillaWrapper` class.
    - Encapsulate Scintilla message sending (`ScintillaObject`, `SendMessage`).
    - Provide strong typing for Scintilla commands rather than using bare integers and `void*` pointers.
    - Ensure memory safety and RAII patterns are applied to Scintilla resources.

## C++ Refactoring Plan (Next Steps)
- [x] **Document Core**: Refactor `src/document.c` into a clean C++ `DocumentManager` and `Document` class mirroring the `geany-go/editor` interface.
- [ ] **Plugin Abstraction**: Wire up the `PluginManager` class into `main.c` so the C codebase utilizes the C++ container instead of `GSList`.

## C++ Refactoring Plan (Next Steps)
- [ ] **Tools Abstraction**: Refactor `src/tools.c` into a C++ `ToolsManager`.
- [ ] **Search Engine C++**: Wire the C++ `DocumentManager` and `ScintillaWrapper` up to a modernized C++ search handler before fully replacing it with the Go port.

## C++ Refactoring Plan (Next Steps)
- [x] **Filetypes Abstraction**: Refactor `src/filetypes.c` into a modern C++ `FileTypeManager` containing a mapping of languages to syntax highlighting definitions.
- [ ] **Configuration Serialization**: Refactor `src/keyfile.c` to use a generic C++ config parser, complementing the Go `config` package.

## C++ Refactoring Plan (Next Steps)
- [x] **Highlighting Engine**: Refactor `src/highlighting.c` into a C++ `SyntaxHighlighter` class that consumes `FileType` definitions from the new `FileTypeManager` and maps them to Scintilla style bytes via `ScintillaWrapper`.

## C++ Refactoring Plan (Next Steps)
- [ ] **Tools Abstraction**: Refactor `src/tools.c` into a C++ `ToolsManager`.
- [ ] **Search Engine C++**: Wire the C++ `DocumentManager` and `ScintillaWrapper` up to a modernized C++ search handler before fully replacing it with the Go port.

## C++ Refactoring Plan (Next Steps)
- [ ] **Main Application Loop**: Refactor `src/main.c` and `src/libmain.c` into a C++ `Application` class, initiating the startup sequence through the new object-oriented managers.
