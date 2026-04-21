/**
 * SearchManager.h
 *
 * Modern C++ refactor of `src/search.c`.
 * This class coordinates search operations (Find Next, Replace, Find in Files).
 * It replaces the messy global search state and bridges the gap between the IDE's UI
 * and the actual search engine logic (which is now being ported to Go).
 */

#ifndef GEANY_SEARCH_MANAGER_H
#define GEANY_SEARCH_MANAGER_H

#include <string>
#include <vector>

namespace geany {

// Forward declarations
class DocumentManager;
class MsgWindow;

// Search configuration flags
struct SearchOptions {
    bool matchCase;
    bool wholeWord;
    bool useRegex;
    bool searchBackwards;
    bool wrapAround;
};

// Represents a single search match
struct SearchMatch {
    int startPos;
    int endPos;
    std::string matchedText;
};

class SearchManager {
public:
    // Requires pointers to the active Document Manager and the Message Window (for routing "Find in Files" results).
    SearchManager(DocumentManager* docManager, MsgWindow* msgWindow);
    ~SearchManager();

    // Prevent copying
    SearchManager(const SearchManager&) = delete;
    SearchManager& operator=(const SearchManager&) = delete;

    // Executes a "Find Next" operation in the currently active document.
    // Returns true if a match was found and selected, false otherwise.
    bool FindNext(const std::string& pattern, const SearchOptions& options);

    // Executes a "Replace Next" operation.
    bool ReplaceNext(const std::string& pattern, const std::string& replacement, const SearchOptions& options);

    // Replaces all occurrences in the active document.
    // Returns the number of replacements made.
    int ReplaceAll(const std::string& pattern, const std::string& replacement, const SearchOptions& options);

    // Simulates "Find in Files" - logging results directly to the MsgWindow.
    void FindInFiles(const std::string& directory, const std::string& pattern, const SearchOptions& options);

private:
    DocumentManager* m_docManager;
    MsgWindow* m_msgWindow;

    // Helper to invoke the Go search engine via FFI
    // Currently stubbed in C++ until the FFI bridge exposes the search package.
    SearchMatch InvokeGoSearch(const std::string& text, const std::string& pattern, int startOffset, const SearchOptions& options) const;
};

} // namespace geany

#endif // GEANY_SEARCH_MANAGER_H
