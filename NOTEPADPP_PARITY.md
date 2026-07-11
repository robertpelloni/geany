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

## Deep Menu Parity Audit

### File Menu
*   **Notepad++**: New, Open, Open Containing Folder (Explorer/cmd), Reload from Disk, Save, Save As, Save a Copy As, Save All, Rename, Close, Close All, Close Multiple (All BUT this, All to Left, All to Right), Move/Clone to Other View, Print, Print Now, Restore Recent Closed File, Open All Recent Files, Exit.
*   **Geany Gap**: Geany lacks 'Open Containing Folder' natively in File menu, 'Close Multiple' advanced options, 'Move/Clone to Other View' (requires plugin/split view), 'Restore Recent Closed File' (needs to be robustly supported in Go `editor` package), and 'Open All Recent Files'.

### Edit Menu
*   **Notepad++**: Undo, Redo, Cut, Copy, Paste, Delete, Select All, Begin/End Select, Copy to Clipboard (Current Full File path, Filename, Dir Path), Indent/Outdent, Convert Case (UPPER, lower, Proper, Sentence, iNVERT, ranDoM), Line Operations (Duplicate, Split, Join, Move Up/Down, Sort, Remove Empty), Comment/Uncomment, Auto-Completion, EOL Conversion, Blank Operations, Paste Special (HTML/RTF).
*   **Geany Gap**: Geany lacks explicit 'Begin/End Select' markers, 'Paste Special' decoding, 'Convert Case' parity (Random/Invert/Sentence), and advanced 'Line Operations' (Sort, Remove Empty) natively without plugins.

### Search Menu
*   **Notepad++**: Find, Find in Files, Find Next, Find Previous, Select and Find Next/Prev, Replace, Incremental Search, Search Results Window, Go to matching brace, Go to line, Mark, Unmark all, Bookmark (Toggle, Next, Prev, Clear, Cut/Copy/Paste/Remove Bookmarked Lines).
*   **Geany Gap**: Geany's search is robust but Notepad++'s 'Bookmark' sub-menu for manipulating text *based entirely on bookmarks* (e.g. "Cut Bookmarked Lines") is a massive productivity feature missing in native Geany.

### View Menu
*   **Notepad++**: Always on Top, Toggle Full Screen, Post-It, Distraction Free Mode, View Current File in (Chrome/Edge/Firefox), Show Symbol (Space/Tab/Indent/EOL/All), Zoom, Move/Clone Current Document, Tab (Move, Next, Prev, Recent), Word Wrap, Focus on Another View, Hide Lines, Fold/Unfold (Level 1-8).
*   **Geany Gap**: Geany needs 'Post-It' mode, 'Distraction Free Mode', 'Hide Lines' (arbitrary line hiding, not just folding), and native explicit 'View Current File in Browser' without custom build commands.

### Encoding Menu
*   **Notepad++**: ANSI, UTF-8, UTF-8-BOM, UTF-16 BE BOM, UTF-16 LE BOM, Character sets (Arabic, Baltic, Celtic, Cyrillic, etc.), Convert to UTF-8, Convert to ANSI, etc.
*   **Geany Gap**: Geany supports encoding well, but Notepad++ makes it a top-level primary citizen. We must ensure the Go UI bridges replicate this prominence.

## Deep Tools Parity
*   **Compare Plugin:** Notepad++ has a legendary Compare plugin. This is critical for parity. *Geany must integrate a side-by-side AST/Diff view natively.*
*   **Hex Editor:** Built-in capabilities or easily installable plugins in N++. Geany needs a robust Hex view port.

## Immediate Action Items (Added to TODO)
1.  Implement native Macro Recording & Playback.
2.  Implement Multi-line Tabs.
3.  Implement Synchronized Scrolling in Split View.
4.  Enhance "Mark All" search functionality to match N++ Bookmark/Mark tab.
5.  Native Document Map (Minimap) integration.
6.  Modernize Style/Theme Configurator (to be handled by new UI submodules).
7.  Implement explicit Edit > Line Operations (Sort, Reverse, Split, Join) in `geany-go/engine`.
8.  Implement explicit Edit > Blank Operations (Trim, Converters).
9.  Implement Native Compare Plugin logic in Go.
