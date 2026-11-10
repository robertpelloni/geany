# Project Vision: The Ultimate IDE

## Ultimate Goal
The ultimate goal of this project is to take the Geany IDE and elevate it into an absolutely outstanding, magnificent, extraordinary, phenomenal, marvelous, INSANELY GREAT text editor and IDE.

## Core Pillars
1. **Clean Architecture & Language**: Refactor the existing C codebase to clean, well-structured, well-thought-out, well-named, well-organized, straightforward, easy-to-use modern C++ (C++17). Replace legacy GLib containers (`GSList`, bare arrays) with robust STL containers (`std::vector`, `std::map`, `std::unique_ptr`) and RAII managers (`Application`, `DocumentManager`, `PluginManager`, `ConfigManager`).
2. **The Go Ultra-Project**: Concurrently port the entire project to Go (`geany-go`). Every feature and functionality possible must be completely ported to Go in a robust and superior implementation. All submodule projects will be analyzed, and their features assimilated and implemented until all projects are merged into one massive, streamlined Go ultra-project.
3. **High-Performance Rust Module**: Utilize `geany-rust` for high-performance memory-safe operations (TextFX2 transformations, cryptographic codecs, Base64 processing, and multi-line tab managers), connected dynamically via FFI bridges.
4. **1:1 Feature Parity with Notepad++**: Deeply analyze Notepad++ and ensure Geany has 100% 1:1 feature parity and functionality parity. Every detail, feature, menu item, button, option, and setting must have an analogous feature in Geany, improved wherever possible (e.g., TextFX2 line and blank operations, vertical and multi-line tabs, minimap document maps, and macro recording/playback).
5. **Multiple Native UI Frontends & Adapters**: The project utilizes modular submodules to provide native UI frontends through a decoupled adapter registry (`geany-go/ui/registry.go`):
   - `bobui` (bobq / Qt6): Modern, hardware-accelerated Qt6 frontend.
   - `btk` (copperspice / Qt4): Lightweight, standards-compliant desktop shell.
   - `bobgui` (bobtk / GTK): Native Linux GObject/GTK shell.
   - Each frontend adapter implements unified UI components (`TabWidget`, `StatusBar`, `SearchBar`) to guarantee consistent feature parity and muscle memory regardless of the rendering backend.
6. **Session Management & Cross-Toolkit State Restoration**: A unified configuration and session pipeline coordinating C++ `ConfigManager` and Go `geany-go/config` to snapshot open files, caret line/column positions, selection offsets, bookmarks, and folded lines. Any session saved under one toolkit (e.g., `bobgui`) can be restored transparently in another (e.g., `bobui`).
7. **Comprehensive Documentation & Quality Governance**: Every single implemented and planned feature must be well represented in full detail in the UI (labels, descriptions, tooltips) and fully documented in manuals, design specifications, requirements, user guides, testing suites, and roadmap/todo lists.

## The "Never Stop" Philosophy
Development should be autonomous, continuous, and highly rigorous. Every feature must be thoroughly tested, documented, and refined. Proceed relentlessly from milestone to milestone.
