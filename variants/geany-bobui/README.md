# Geany BobUI Variant

This directory hosts an **experimental BobUI-based alternate frontend** for Geany.

## Purpose

- explore a BobUI / Qt6-fork frontend path without destabilizing the production tree
- keep this variant toolkit-exclusive to BobUI / Qt-style APIs
- validate a dense Search Studio shell on a BobUI-native widget stack

## Current scope

This variant currently builds a standalone **Search Studio** prototype using only BobUI / Qt-style APIs for:

- Find
- Replace
- Find in Files
- Mark
- lower navigator tabs for:
  - Activity
  - Results
  - Diff Preview

The source layout mirrors the current experimental Search Studio variant shape, but this folder is BobUI-only:
- `src/main.cpp`
- `src/search_studio_backend.h`
- `src/search_studio_backend.cpp`

## Toolchain direction

The variant is expected to build against the BobUI submodule at:
- `../../subprojects/bobui`

The local CMake file looks for Qt6-compatible package metadata in common BobUI build/install locations such as:
- `../../build/bobui-install/lib/cmake`
- `../../build/bobui-install`
- `../../build/bobui`

or any BobUI-provided `Qt6` package visible to `find_package(Qt6 CONFIG ...)`.

## Current validation status

- variant scaffolding and BobUI-exclusive sources were added in this repo
- in this environment, full BobUI variant build validation still depends on a usable BobUI / Qt6 package tree being available to CMake

## Next steps

1. build/install BobUI standalone and point this variant at that package tree
2. validate the BobUI Search Studio executable end-to-end
3. continue evolving the backend seam so BobUI consumes real Geany search/document services later
