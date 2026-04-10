# Notepad++ vs Geany: Feature Parity Analysis

This document serves to comprehensively evaluate the feature parity between Notepad++ (the benchmark) and Geany, identifying gaps that must be implemented to achieve 100% 1:1 functional parity.

## Core Editing Features
*   **Syntax Highlighting:** Both have extensive language support. *Geany must ensure it matches or exceeds the number of supported languages in Notepad++.*
*   **Auto-completion:** Both support word and function completion. *Geany's needs to be verified for robustness and ease of configuration compared to N++.*
*   **Multi-Document Interface (MDI):** Both use tabs. *Notepad++ allows multi-line tabs; Geany should implement this if not present.*
*   **Multi-View/Split Screen:** Both support splitting the editor. *Notepad++'s synchronized scrolling feature should be verified in Geany.*
*   **Macro Recording & Playback:** Notepad++ has robust macro support. *Geany lacks built-in macro recording (though plugins exist). This must be implemented natively or robustly via a standard, bundled plugin.*

## Search and Replace
*   **Regular Expressions:** Both support PCRE.
*   **Find in Files:** Both have robust directory search.
*   **Incremental Search:** Both support this.
*   **Marking:** Notepad++ has a dedicated "Mark" tab in the find dialog to highlight all occurrences and optionally bookmark lines. *Geany's "Mark All" functionality needs to be as robust and feature-rich.*

## Code Navigation and Structure
*   **Code Folding:** Both support this natively.
*   **Document Map/Minimap:** Notepad++ has a "Document Map". *Geany has a plugin (overview), but it should be evaluated for native integration and 1:1 parity.*
*   **Function List:** Both have a symbol/function list. Geany's is often considered superior due to ctags integration.

## Plugins and Extensibility
*   **Plugin Manager:** Notepad++ has "Plugins Admin". Geany has a plugin manager. *We must ensure Geany's plugin ecosystem is as accessible and robust.*
*   **Scripting:** Notepad++ has plugins like PythonScript. *Geany's scripting capabilities (e.g., GeanyLua or Python plugins) need to be evaluated and potentially brought into the core Go port.*

## UI and Customization
*   **Themes/Style Configurator:** Notepad++ has a comprehensive Style Configurator. *Geany's theme support (often reliant on GTK themes + color schemes) needs to be modernized and made as easy to customize natively. This will be addressed by the `bobui`, `btk`, and `bobgui` integrations.*
*   **Customizable GUI:** Notepad++ allows hiding toolbars, menus, etc. Geany does too. *Ensure 1:1 parity on what can be toggled.*

## Immediate Action Items (Added to TODO)
1.  Implement native Macro Recording & Playback.
2.  Implement Multi-line Tabs.
3.  Implement Synchronized Scrolling in Split View.
4.  Enhance "Mark All" search functionality to match N++ Bookmark/Mark tab.
5.  Native Document Map (Minimap) integration.
6.  Modernize Style/Theme Configurator (to be handled by new UI submodules).
