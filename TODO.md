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

## Phase 1 Finalization
- [x] Fix submodule pointers (`bobui`, `btk`, `bobgui`).
- [x] Bootstrap `geany-go` module and CLI.
- [x] Perform Notepad++ Parity Analysis.

## Notepad++ Parity Features (Identified in Analysis)
- [x] Implement native Macro Recording & Playback.
- [ ] Implement Multi-line Tabs (Support vertical stacking and scrolling).
- [ ] Implement Synchronized Scrolling in Split View.
- [ ] Enhance "Mark All" search functionality to match N++ Bookmark/Mark tab.
- [ ] Native Document Map (Minimap) integration.
- [ ] Modernize Style/Theme Configurator (via UI submodules).
- [ ] Native 'Compare Plugin' feature for side-by-side diffing.
- [x] Implement explicit Edit > Line Operations (Sort, Reverse, Split, Join, Duplicate, Delete) in `geany-go/engine`.
- [x] Implement explicit Edit > Blank Operations (Trim, Converters).
- [ ] Native Hex Editor integration.
- [x] **Full PARITY.md Menu Audit**: Catalog every single N++ menu item vs Geany and add to PARITY.md

## C++ Refactoring Plan
- [x] **Scintilla C++ Wrapper**: Refactor `src/scintilla.c` and related Scintilla wrapper functions into a modern, robust C++ `ScintillaWrapper` class.
    - Encapsulate Scintilla message sending (`ScintillaObject`, `SendMessage`).
    - Provide strong typing for Scintilla commands rather than using bare integers and `void*` pointers.
    - Ensure memory safety and RAII patterns are applied to Scintilla resources.

## C++ Refactoring Plan (Next Steps)
- [x] **Document Core**: Refactor `src/document.c` into a clean C++ `DocumentManager` and `Document` class mirroring the `geany-go/editor` interface.
- [x] **Plugin Abstraction**: Wire up the `PluginManager` class into `main.c` so the C codebase utilizes the C++ container instead of `GSList`.

## C++ Refactoring Plan (Next Steps)
- [x] **Tools Abstraction**: Refactor `src/tools.c` into a C++ `ToolsManager`.
- [x] **Search Engine C++**: Wire the C++ `DocumentManager` and `ScintillaWrapper` up to a modernized C++ search handler before fully replacing it with the Go port.

## C++ Refactoring Plan (Next Steps)
- [x] **Filetypes Abstraction**: Refactor `src/filetypes.c` into a modern C++ `FileTypeManager` containing a mapping of languages to syntax highlighting definitions.
- [x] **Configuration Serialization**: Refactor `src/keyfile.c` to use a generic C++ config parser, complementing the Go `config` package.

## C++ Refactoring Plan (Final Phase)
- [ ] **Application Singleton**: Fully deprecate `src/main.c` and `src/libmain.c`, transitioning entirely to `geany::Application::Run()`.
- [ ] **UI Abstraction**: Transition GTK dependencies strictly into the `bobgui` submodule and remove GTK headers from core engine compilation.

## Phase 2: Bobgui UI Overhaul & Dashboard
- [ ] Implement Geany settings panel via Dashboard UI (Bobgui).
- [ ] Connect Go backend Config metrics to Dashboard.
- [x] Thoroughly map all backend `geany-go/engine` features (Line Operations, Formatters, Search) directly into the Dashboard UI.
- [x] Ensure all settings and tools are comprehensively labeled with clear, detailed UI tooltips indicating their behavior.
- [ ] Redesign the Bobgui Dashboard UI layout to condense all subpages and related concepts into a single, intuitive, high-value monolithic page.
- [ ] Ensure every single feature and function exposed in the Dashboard UI is fully annotated with descriptive tooltips and guidance markers.

## Phase 4: Notepad++ Parity Feature Engine Port
- [ ] Begin porting N++ Multi-line tabs core logic to `geany-go/ui`.
- [ ] Map all documented `PARITY.md` gaps (Save a Copy As, Bookmark logic) to Go backend interfaces.
- [ ] Implement Hex Editor plugin backend logic in Go.
- [ ] Implement AST Compare Plugin core diff algorithm in Go.

## C++ Refactoring Plan (Next Steps)
- [x] **Highlighting Engine**: Refactor `src/highlighting.c` into a C++ `SyntaxHighlighter` class that consumes `FileType` definitions from the new `FileTypeManager` and maps them to Scintilla style bytes via `ScintillaWrapper`.

## Main Execution Wiring
- [x] **Wire C++ Application**: In `src/main.c`, instantiate the global `geany::Application` object, pass CLI arguments into `Initialize()`, and delegate to `Run()`.
- [x] **Go FFI Bridge**: Established a CGO interface bridging the C++ `Application` with the Go backend (`geany-go`).

## Native UI Submodule Bootstrapping
- [x] **Bobui Integration (Qt6)**: In `submodules/bobui`, establish a basic `Application` frontend that satisfies the `geany-go/ui` interface schema.
- [x] **Bobgui Integration (GTK)**: Begin refactoring the old Geany GTK calls to funnel through the new C++ managers and `bobgui`.
- [x] Implement `geany-go/project` interface in C++ FFI bridge.
