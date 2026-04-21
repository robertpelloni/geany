/**
 * SyntaxHighlighter.h
 *
 * Modern C++ refactor of `src/highlighting.c`.
 * This class bridges the gap between the `FileTypeManager` (which defines languages)
 * and the `ScintillaWrapper` (which executes the styling commands).
 *
 * It manages setting keywords, lexer IDs, and style properties dynamically.
 */

#ifndef GEANY_SYNTAX_HIGHLIGHTER_H
#define GEANY_SYNTAX_HIGHLIGHTER_H

#include <string>
#include <map>

namespace geany {

// Forward declarations
class ScintillaWrapper;
struct FileType;

class SyntaxHighlighter {
public:
    SyntaxHighlighter();
    ~SyntaxHighlighter();

    // Prevent copying
    SyntaxHighlighter(const SyntaxHighlighter&) = delete;
    SyntaxHighlighter& operator=(const SyntaxHighlighter&) = delete;

    // Initializes syntax highlighting for a specific Scintilla instance
    // based on the provided language FileType definition.
    bool ApplyHighlighting(ScintillaWrapper* editor, const FileType* fileType);

    // Sets a specific style (fore/back colors, font, bold/italic) for a given style ID.
    // e.g. styleId = SCE_C_COMMENTLINE
    void SetStyle(ScintillaWrapper* editor, int styleId, const std::string& foreColor, const std::string& backColor, bool bold, bool italic);

private:
    // A robust mapping of language IDs to specific keyword sets.
    std::map<std::string, std::string> m_keywordSets;

    // Setup internal keyword mapping
    void InitKeywords();
};

} // namespace geany

#endif // GEANY_SYNTAX_HIGHLIGHTER_H
