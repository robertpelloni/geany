# Handoff Document

This document is used to pass state, analysis, and instructions between different AI models (e.g., Claude, Gemini, GPT) across sessions.

## Current State
- Build files configured to read from dynamic VERSION.md.
- Started porting `src/utils.c` to idiomatic Go in `geany-go/utils`.
- Started porting `GeanyDocument` to idiomatic Go in `geany-go/editor`.
- Began C++ Refactor by introducing `ScintillaWrapper` and `PluginManager` classes.
- Created robust Go `ui` interfaces defining Application, Window, and EditorWidget behavior to prepare for submodule integration.
- Ported text search logic to `geany-go/search` and refactored C++ `DocumentManager`.
- Bootstrapped `geany-go/build` execution engine and refactored C++ `ProjectManager`.
- A Notepad++ parity analysis is pending.

## Instructions for Next Agent
1. Read `VISION.md`, `ROADMAP.md`, `TODO.md`, and `MEMORY.md` to understand the massive scope of this project.
2. Review the `CHANGELOG.md` and `VERSION.md` to see what was just done.
3. Continue executing tasks from `ROADMAP.md` and `TODO.md`.
4. Document your findings in extreme detail. If you generate new ideas, put them in `IDEAS.md`.
5. Before finishing your session, update this `HANDOFF.md` file, the `CHANGELOG.md`, and bump the version in `VERSION.md` if significant changes were made.
