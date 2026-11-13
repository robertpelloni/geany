# User & Developer Guide: Geany Ultra Session Management and Configuration

## Introduction

Geany Ultra coordinates state, configuration, and session persistence across three modern layers:
1. **Modern C++ Core (`geany::Application`, `geany::ConfigManager`)**: High-performance orchestrator managing the primary event loop and memory safety.
2. **Go Backend Engine (`geany-go`)**: Houses high-level configuration parsing, text transformation algorithms, syntax symbol indexing, and decoupled UI event routing.
3. **Multi-Toolkit Native Frontends**: Native UI bindings for GTK (`bobgui`), Qt6 (`bobui`), and Qt4/CopperSpice (`btk`).

This guide details configuration schemas, session storage conventions, command-line arguments, and integration guidelines for future developers and plugin authors.

---

## 1. Configuration Storage & Hierarchy

Configuration is persisted in standard INI files under the user's platform configuration directory (e.g., `~/.config/geany/` on Linux/POSIX, `%APPDATA%\Geany\` on Windows).

### Primary Files
- **`geany.conf`**: Global user preferences, UI theme selections, and editor behavior.
- **`session.conf`**: Ephemeral and persistent workspace state, tracking which documents are open, their tab ordering, cursor locations, and active folds.

### INI File Structure Example (`session.conf`)
```ini
[session]
active_document=/home/user/workspace/main.go
document_count=2
split_view=false

[document_0]
path=/home/user/workspace/main.go
encoding=UTF-8
cursor_pos=142
selection_start=0
selection_end=0
bookmarks=10,24,55
folded_lines=30,45

[document_1]
path=/home/user/workspace/config.go
encoding=UTF-8
cursor_pos=88
selection_start=12
selection_end=45
bookmarks=
folded_lines=
```

---

## 2. Command-Line Options

When launching Geany Ultra or one of the variant frontends, session behavior can be influenced via CLI flags:

- `--no-session`: Starts the editor without restoring previous open files or window positions.
- `--session=<path>`: Loads a specific session file instead of the default `session.conf`.
- `--config=<dir>`: Overrides the configuration root directory.
- `[files...]`: Automatically appends provided file paths to the newly opened or active session.

---

## 3. Developer Integration: C++ & Go Bridges

### Reading Configuration in C++
```cpp
#include "ConfigManager.h"

// Access global config manager from geany::Application
auto& config = app.GetConfigManager();
std::string theme = config.GetString("geany", "theme", "default");
bool multiTabs = config.GetBool("geany", "multiline_tabs", false);
```

### Mutating Configuration via FFI in Go
```go
// Exported CGO symbols allow the Go backend to safely update preferences:
// GeanyGo_Config_SetString(section, key, value)
// GeanyGo_Config_GetString(section, key, fallback)
```

### Listening for Session & UI Events
Submodules (`bobgui`, `bobui`, `btk`) and plugins can subscribe to UI events via `ui.EventDispatcher`:
```go
dispatcher := ui.NewEventDispatcher()
dispatcher.Subscribe(ui.EventDocOpened, func(ev ui.UIEvent) {
    fmt.Printf("Document opened: %s\n", ev.Payload)
})
```

---

## 4. Troubleshooting & Session Recovery

- **Corrupt `session.conf`**: If a session file contains invalid non-ASCII characters or unclosed headers, both the Go scanner (`geany-go/config/manager.go`) and C++ loader (`src/ConfigManager.cpp`) skip malformed lines safely, falling back to a clean state without crashing.
- **Missing Files**: If a file listed in `session.conf` has been moved or deleted externally, the loader notes the absence in `MsgWindow` and continues restoring the remaining documents.
