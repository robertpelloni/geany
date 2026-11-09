# Requirements: Geany Session Management & Configuration Architecture

## 1. User & Functional Goals

Geany Ultra aims to achieve complete **1:1 functional parity with Notepad++** and modernize Geany's legacy C configuration system (`src/keyfile.c`, `src/prefs.c`) into clean, type-safe C++ and Go modules.

### Functional Requirements (FR)
- **FR-1 (Session Snapshotting)**: The system must automatically capture open documents, tab display order, active/focused tab index, and split-view orientation on clean application exit or periodic auto-save.
- **FR-2 (Buffer Position Persistence)**: For each tracked document, the system must persist:
  - Exact caret line and column position.
  - Text selection start and end byte offsets.
  - Visible line scroll offset / first visible line index.
  - Active line bookmark indicators.
  - Collapsed/folded code blocks.
- **FR-3 (Encoding & File Metadata)**: Document character encoding (UTF-8, UTF-16, ISO-8859), newline format (CRLF, LF, CR), and Byte-Order Mark (BOM) state must be captured to ensure zero data alteration upon reload.
- **FR-4 (Multi-Toolkit Session Replay)**: Any session saved in one toolkit frontend (e.g., `bobgui` GTK) must be transparently loadable by another (e.g., `bobui` Qt6 or `btk` CopperSpice).
- **FR-5 (Named Session Files)**: Users must be able to save and load explicit session snapshots (e.g., `Save Session As...`, `Load Session...`) mirroring Notepad++'s File > Session commands.

---

## 2. Non-Functional Requirements (NFR)

- **NFR-1 (Performance & Latency)**: Session file reading and writing must complete in under 50ms for workspaces containing up to 100 open files, ensuring zero UI hang on launch or exit.
- **NFR-2 (Thread Safety & Non-Blocking I/O)**: Background session serialization and configuration access must be thread-safe (utilizing `sync.RWMutex` in Go and mutexes/RAII in C++), preventing deadlocks or race conditions during rapid editor mutations.
- **NFR-3 (Resilience to Corruption)**: Malformed lines, non-ASCII garbage, or corrupted section headers must be ignored safely without throwing unhandled exceptions or crashing the editor.
- **NFR-4 (Memory Footprint)**: In-memory configuration structures (`geany::ConfigManager`, `geany-go/config.Manager`) must not exceed 2MB for typical workspaces.

---

## 3. Notepad++ Parity Comparison

| Feature | Notepad++ Behavior | Geany Ultra Target | Status |
|---|---|---|---|
| Auto-save session on exit | `session.xml` snapshot | `session.conf` INI snapshot | Specified & Tested |
| Bookmark restoration | Restored via Scintilla markers | Restored via `bookmarks` section key | Specified |
| Fold state restoration | Restored per fold level | Restored via `folded_lines` array | Specified |
| Multi-line tab preferences | Preserved in `config.xml` | Preserved in `geany.conf` (`multiline_tabs`) | Implemented |
| Tab orientation (Vertical) | TextFX2 vertical tab list | `tab_orientation` in Go/Rust/C++ FFI | Implemented |
| Custom Named Sessions | `.session` XML file load/save | `.conf` INI file load/save | Specified |

---

## 4. Verification & Acceptance Criteria

1. All Go unit tests in `geany-go/config`, `geany-go/editor`, `geany-go/ui` pass cleanly.
2. Rust parity unit tests in `geany-rust` pass cleanly.
3. INI round-trip tests confirm identical file output upon re-serialization without data corruption.
