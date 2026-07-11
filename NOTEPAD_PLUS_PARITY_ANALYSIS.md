# Notepad++ 1:1 Feature Parity Analysis

This document identifies all features from Notepad++ that need to be explicitly implemented, ported, or enhanced in the Geany codebase to ensure complete 1:1 parity and superiority.

## 1. File and UI Management
*   **Multi-line Tabs**: Ensure the UI Tab Manager (`geany-go/ui` & `geany-rust/src/ui.rs`) can break tabs into multiple stacked rows when horizontal space runs out.
*   **Vertical Tabs**: Implemented!
*   **Synchronized Scrolling**: Dual views (split screen) must have a toggle to synchronize vertical/horizontal scrolling simultaneously.
*   **Document Map (Minimap)**: A miniature graphical overview of the entire file with scroll tracking.
*   **Ghost Typing**: An easter egg/accessibility feature simulating automated typing from a script.

## 2. Search and Replace
*   **Mark All (Bookmarks)**: "Mark" dialog tab that highlights all matches and explicitly drops a line-level bookmark for each match.
*   **Find in Files (Directory)**: Ensure recursive directory search supports explicit exclusion masks (e.g., `*.tmp; .git`).
*   **Incremental Search**: Real-time highlighting of search matches as characters are typed.

## 3. TextFX & Formatting (Implemented)
*   **Sorting**: Ascending/Descending (Implemented)
*   **Case Conversion**: Proper Case, Sentence Case (Implemented)
*   **Whitespace Trimming**: Trailing/Leading (Implemented)
*   **Line Operations**: Blank Line Removal, Duplicate Line Removal (Implemented)
*   **Encoding**: Base64 Encode/Decode (Implemented)

## 4. Advanced Line & Column Operations
*   **Column/Block Mode Selection**: `Alt + Mouse Drag` or `Alt + Shift + Arrows` to select arbitrary rectangles of text.
*   **Column Editor**: Insert a number sequence (e.g., 1 to 100 with zero padding) down a selected column.
*   **Split/Join Lines**: Hard wrap lines at an arbitrary column width (e.g., 80 chars), or join multiple lines into a single continuous string.
*   **Move Lines Up/Down**: Hotkeys (`Ctrl+Shift+Up/Down`) to shift entire lines without cutting/pasting.

## 5. View and Styling
*   **Style Configurator**: A unified, modernized UI (via `bobui`/`bobgui`) to customize all Scintilla Lexer styles, fonts, and colors visually without editing INI files manually.
*   **Word Wrap**: Toggle visual wrapping of long lines to the window width.
*   **Show Symbol**: Toggle visible markers for Whitespace, Tabs, and Line Endings (CRLF).

## 6. Execution & Macros
*   **Macro Recording & Playback**: Core engine implemented. Needs full UI wiring.
*   **Run/Launch Dialog**: Customizable command execution with saved presets.

## 7. Language and Encoding Subsystems
*   **Auto-Completion & Call Tips**: Notepad++ has extensive language-specific auto-completion mapping. Must ensure `geany-go/editor` triggers `ScintillaWrapper` AutoC functionality using defined `geany-go/filetypes` dictionaries.
*   **Folder as Workspace**: Must implement a tree-view panel that dynamically tracks a system directory without requiring a rigid `.geany` project configuration file.
*   **Encoding Autodetection**: Ensure robust detection of Shift-JIS, Windows-1252, and Mac-Roman encodings natively within `geany-go` to match Notepad++'s aggressive localization tracking.

## 8. Plugins & Architecture
*   **Hex Editor**: Notepad++ provides Hex viewer functionality via plugins. We should integrate a native Hex View UI binding directly into the editor tabs via Go.
*   **Markdown Viewer**: A live rendering HTML pane adjacent to the text editor.

## Conclusion
The fundamental backend logic for several of these features exists inside Scintilla, but the C++ architectural wrappers (`ScintillaWrapper`) and the Go backend (`geany-go/editor`) must expose explicit, safe APIs to interact with them, and the `bobui/bobgui` frontends must wire them to explicit, localized buttons and menus.

## 9. Extensibility and Core UI
*   **Split View Synchronization**: Ensure scrolling in dual panels remains perfectly synchronized (essential for diffing).
*   **Native Theming & Styling**: A robust native theme configurator matching Notepad++'s `stylers.xml` UI where language colors can be specifically overridden via an intuitive GUI.
*   **Macro Complex Playback**: Multi-action macro repetition executing `N` times until EOF.
