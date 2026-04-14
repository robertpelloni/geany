# Geany Overhaul: Comprehensive Project Memory & Architecture

## 1. Ultimate Project Vision
The overarching goal is to elevate the Geany IDE into the ultimate, "Insanely Great" text editor. This involves two massive concurrent tracks:
*   **C++ Refactor:** Methodically modernizing the existing C codebase into clean, well-structured, robust, and object-oriented C++.
*   **The Go "Ultra-Project":** Simultaneously building a comprehensive, fully-featured port of the entire IDE in Go (`geany-go/`). This port will eventually assimilate the features from all submodules into a unified architecture.

## 2. Feature Parity
*   **Notepad++ as the Benchmark:** The project mandates 100% 1:1 feature and functionality parity with Notepad++. Every menu item, option, setting, and feature found in Notepad++ must be present in Geany, and improved upon wherever possible.
*   **Key Identified Gaps (from Parity Analysis):** Immediate focus areas include native Macro Recording & Playback, Multi-line Tabs, Synchronized Scrolling in Split View, a native Document Map (Minimap), and a modernized Theme/Style Configurator.

## 3. Native UI Submodules
The architecture heavily relies on git submodules to provide multiple native frontend choices, moving away from a strict GTK-only dependency:
*   **Bobui (`submodules/bobui`):** The Qt6 (bobq) native UI frontend.
*   **Btk (`submodules/btk`):** The CopperSpice/Qt4 native UI frontend.
*   **Bobgui (`submodules/bobgui`):** The GTK (bobtk) native UI frontend (an evolution of the current GTK UI).
*   *Future Note:* A Web UI is also planned for the Go port architecture.

## 4. Documentation and State Management
The user demands exhaustive, extreme detail in all documentation. The project state is heavily managed through a suite of Markdown files that must be updated at the start and end of every session:
*   **`VISION.md` & `ROADMAP.md`**: For long-term goals, architectural pillars, and phased tracking.
*   **`TODO.md`**: For granular, short-term tasks and bug fixes.
*   **`HANDOFF.md`**: A critical file for passing state, context, and immediate next steps between different AI models (Claude, Gemini, GPT).
*   **`AGENTS.md`**: Universal instructions for all AI models (read documentation first, remain autonomous, commit often).
*   **`MEMORY.md` & `IDEAS.md`**: For tracking ongoing observations and brainstorming architectural improvements.

## 5. Build System & Versioning
*   **Single Source of Truth:** Version numbers are no longer hardcoded in build scripts. `VERSION.md` contains the raw version string (e.g., `1.0.0-alpha.1`).
*   **Dynamic Resolution:** Both the Autotools (`configure.ac` using `m4_esyscmd_s`) and Meson (`meson.build` using `run_command`) build systems have been configured to dynamically read this text file during build configuration.
*   **Changelog:** Every version bump must be accompanied by an update to `CHANGELOG.md` and explicitly referenced in the Git commit message.

## 6. Code Guidelines
*   **Extreme Comments:** All code must be commented in extreme depth. Explain *what* it is doing, *why* it is there, any side effects, optimizations, and even non-working alternative methods. Bare code is only acceptable if it is completely, undeniably self-explanatory.
*   **Robustness:** The Go port (`geany-go/`) specifically prioritizes idiomatic, heavily tested utilities (like the newly bootstrapped `geany-go/utils` package for string and file manipulation).

## 7. Workflow Methodology
*   **Deep Planning Mode:** Every task must start with deep planning. AI agents are expected to ask clarifying questions until there is absolute certainty about the user's requirements before making code changes.
*   **Autonomy:** Once a plan is approved, the agent must execute autonomously, moving from feature to feature, committing and pushing regularly ("Don't stop the party").