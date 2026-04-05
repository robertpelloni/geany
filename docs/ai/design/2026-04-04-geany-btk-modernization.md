# Geany Modernization Pass: BTK foundation, vertical tabs, and TextFX parity analysis

## What shipped in this pass

1. **BTK added as a submodule** at `subprojects/btk` with upstream URL `https://github.com/robertpelloni/btk`.
2. **Toolkit include centralization**: direct `#include <gtk/gtk.h>` usage in Geany core sources was routed through `src/gtkcompat.h` so future BTK migration work has a single integration seam.
3. **Vertical editor tabs are now the default** by changing the fallback `tab_pos_editor` from `GTK_POS_TOP` to `GTK_POS_LEFT`.
4. **Application-wide CSS UI themes** were added and wired to a new config key:
   - `liquid-glass` (default)
   - `cyber-glass`
   - `aurora-forge` (custom house theme)
5. **Matching editor colorschemes** were added for the same three themes.

## Configuration

Geany now reads `ui_theme` from the main config group. Supported values:

- `liquid-glass`
- `cyber-glass`
- `aurora-forge`

If the theme is unknown, Geany falls back to `liquid-glass`.

## BTK migration reality check

A full direct replacement of GTK in current Geany is **not a safe one-pass change**. The local `btk` repository is a renamed/forked toolkit with a different API surface and a more modern widget model than the GTK3 API Geany currently targets. Geany still relies heavily on GTK3-era patterns, including:

- classic `GtkBuilder`/widget construction flow
- legacy widgets and APIs (for example old-style notebook, tree, misc/table usage, RC-era behaviors, and deprecated signal patterns)
- direct `gtk_*` types/functions throughout the codebase
- GTK3 CSS expectations and widget naming conventions
- plugin-facing headers that expose GTK-derived types

### Safe migration strategy

The practical path is staged:

1. **Centralize toolkit includes and assumptions** — done in this pass.
2. **Introduce a compatibility facade** for the most common widget/type/signal helpers.
3. **Port the highest-friction widgets first**:
   - notebook/tab system
   - tree/list/sidebar views
   - dialogs and file choosers
   - menu/toolbar/action plumbing
4. **Isolate plugin API exposure** so Geany core can evolve without breaking plugins on every step.
5. **Only then** switch build configuration from GTK3-targeted compilation to BTK-backed compilation.

Attempting to rename every `gtk_*` symbol immediately would create a large uncompilable fork with broken plugins and brittle UI behavior.

## Notepad++ and TextFX deep feature analysis

Source analyzed from the local workspace:

- Notepad++: `../npp`
- TextFX: `../npp/textfx/SRC/NPPTextFX.cpp`

### TextFX functional clusters

The `funcItem[]` command table in `NPPTextFX.cpp` shows that TextFX is not one feature; it is a bundle of many text-power-user systems:

#### 1. Quote and escaping transforms
- convert quotes between `'` and `"`
- drop quotes
- escape/unescape single, double, or both quote styles
- CSV-style doubled-quote transforms

#### 2. Case and character transforms
- upper/lower/proper/sentence/invert case
- zap to spaces
- replace non-printables

#### 3. Search, mark, and brace intelligence
- mark word forward/reverse
- case-sensitive and whole-word variants
- find/mark/delete matching brace pair
- show matching line and line ranges

#### 4. Visibility pipeline (“Viz”)
- show/hide/invert lines by selection or clipboard text
- progressive hide/show sequences
- copy/cut/delete visible vs invisible selections
- clipboard policy toggles for CRLF, binary, UTF-8, Unicode, append mode, cursor retention

#### 5. Column and fill operations
- fill down insert/overwrite
- insert clipboard through multiple lines
- line-up/align text by comma, equals, or clipboard delimiter

#### 6. Indentation and whitespace tools
- reindent C/C++
- leading spaces ↔ tabs
- trim trailing spaces
- sticky indent / surround with braces
- delete blank / surplus blank lines

#### 7. Quote-aware structural transforms
- strip unquoted text (VB/C modes)
- kill unquoted whitespace
- split lines by delimiter while respecting quoting

#### 8. Wrap and unwrap
- unwrap text
- rewrap text to clipboard-configured width or fallback width

#### 9. Encoding and HTML transforms
- encode URI component
- encode HTML entities
- strip HTML tags (tab/non-tab table forms)
- text-to-code conversion helpers

#### 10. Numeric/base conversions
- decimal/binary/octal/hex conversion families
- C-style number parsing
- text ↔ hex blocks
- little-endian byte-run helpers
- ROT13
- EBCDIC/ASCII and KOI8-R/CP1251 conversions

#### 11. Sorting and utility transforms
- case-sensitive and case-insensitive sort at column
- ascending/descending behavior toggle
- unique-lines mode
- insert ASCII chart/ruler/line numbers
- remove line numbers / first word
- UUDecode / Base64 decode
- word count
- add-up-numbers

#### 12. Editor behavior toggles
- block overwrite handling
- wrapped-line home/end behavior
- duplicate block on Ctrl-D
- subclassing / advanced feature toggles
- auto behaviors around brace matching and clipboard capture

### What this means for Geany

Geany already has strong fundamentals:
- Scintilla editor core
- lexer/filetype system
- plugin support
- built-in search/replace, indentation, folding, build tooling, symbol navigation

But TextFX’s advantage is **density of power transforms accessible from one place**.

### Gap assessment

The biggest parity gaps are not syntax highlighting or editing basics. They are:

1. **Batch text transformation discoverability**
2. **Quote-aware / delimiter-aware structured line transforms**
3. **Visibility-driven workflows** (hide/show/copy only visible or invisible line sets)
4. **Dense conversion toolbox** (encoding, numeric bases, byte layouts, data wrangling)
5. **Single-command utility breadth** for programmers doing quick manipulation work

## Better-than-TextFX implementation plan for Geany

A robust Geany implementation should not just copy menu items. It should improve the architecture.

### Proposed subsystem: Geany TextLab

A modern replacement should provide:

- **single searchable command palette** for text transforms
- **preview-before-apply** for destructive operations
- **multi-selection aware commands**
- **undo-grouped transactions**
- **selection / whole document / visible lines / folded lines scopes**
- **quote-aware parser utilities** rather than ad-hoc string scanning per command
- **delimiter-aware table/column transforms**
- **stable sort with locale and numeric modes**
- **stream-safe handling** for large documents
- **property-based tests** for transforms
- **plugin-extensible command registry** so more transforms can be added without touching core UI code

### High-priority parity roadmap

#### Phase 1: command framework
- searchable transform registry
- undo transaction wrapper
- preview dialog / diff summary
- scope selection model

#### Phase 2: text essentials
- quote conversion / escaping family
- case transforms
- trim, blank-line cleanup, tabs/spaces, rewrap/unwrap
- sort + unique + numeric/column modes

#### Phase 3: structured line tools
- align by delimiter
- insert clipboard through lines
- fill-down insert/overwrite
- quoted CSV/TSV-aware split and join

#### Phase 4: conversion pack
- Base64 encode/decode
- URI/HTML encode/decode
- hex/text/byte-order transforms
- numeric base conversion
- checksum/hash helpers where useful

#### Phase 5: visibility workbench
- line filter/hide/show based on regex, selection, clipboard, or markers
- copy visible / copy hidden / operate on folded blocks
- persistent filter state per document

## Why Geany can be better than TextFX

TextFX is powerful, but much of it is implemented as a monolithic plugin with a huge command table and a lot of modeful global state. Geany can surpass it by using:

- better previews
- cleaner command organization
- more reliable undo boundaries
- cross-platform consistency
- extensible plugin-level APIs
- modern tests for edge cases, Unicode, EOL handling, and large buffers

## Next recommended implementation steps

1. Add a first-class **UI theme selector** to Preferences instead of relying on config editing.
2. Build the **TextLab transform registry** and land the first transform family (case, trim, tabs/spaces, sort).
3. Audit plugin-facing GTK assumptions before any BTK API swap.
4. Add build-time backend experiments only after the compatibility facade exists.
