# Geany BobUI Variant

This directory hosts an **experimental BobUI-based alternate frontend** for Geany.

## Purpose

- explore a BobUI/Qt-native frontend path without destabilizing the GTK-based production tree
- prototype richer, Notepad++-style search UX and broader command surfaces
- give the project a clean place to test alternate UI architecture choices

## Current scope

This variant currently contains a standalone **Search Studio** prototype with:

- Find
- Replace
- Find in Files
- Mark
- lower navigator tabs for:
  - Activity
  - Results
  - Diff Preview

The UI intentionally mirrors the density and speed of Notepad++ while adding room for richer preview, workflow shortcuts, and future command-palette integration.

The current BobUI prototype now mirrors several of the matured GTK Search Studio concepts:
- structured result rows instead of only static form controls
- lower-pane navigator behavior
- session-aware actions like `Count Session` and `Mark Session`
- preview-oriented replace and impact routing
- informational vs navigable result-row behavior

## Toolchain direction

The prototype is written against BobUI's Qt-style widget stack and is expected to be built against the BobUI submodule at:

- `../../subprojects/bobui`

## Why a separate variant exists

Geany's main codebase is still GTK3-oriented and plugin-facing. A separate BobUI variant allows faster experimentation with:

- dockable and tabbed dialogs
- denser search UX
- cross-platform native-feeling widget behavior
- future command-palette and transform-tooling work

without forcing an all-at-once migration of the production application.

## Next steps

1. connect the Search Studio controls to Geany core search services
2. replace prototype result generation with real document/session/search backend data
3. port command-palette and transform tooling into this variant
4. define the boundary between reusable Geany core logic and BobUI-native presentation
