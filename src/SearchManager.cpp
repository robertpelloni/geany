/**
 * SearchManager.cpp
 *
 * Implementation of the C++ SearchManager.
 */

#include "SearchManager.h"
#include "DocumentManager.h"
#include "MsgWindow.h"
#include <iostream>
#include <stdexcept>

namespace geany {

SearchManager::SearchManager(DocumentManager* docManager, MsgWindow* msgWindow)
    : m_docManager(docManager), m_msgWindow(msgWindow) {
    if (!m_docManager || !m_msgWindow) {
        std::cerr << "[SearchManager] Warning: Initialized with null pointer dependencies." << std::endl;
    }
}

SearchManager::~SearchManager() {}

bool SearchManager::FindNext(const std::string& pattern, const SearchOptions& options) {
    if (pattern.empty() || !m_docManager) return false;

    // Stub: Get current document and its raw text buffer
    // Document* doc = m_docManager->GetActiveDocument();
    std::string currentText = "Stub text buffer for testing search algorithms.";
    int currentOffset = 0; // In reality, fetch from ScintillaWrapper::GetCursorPos()

    std::cout << "[SearchManager] Executing FindNext for: '" << pattern << "'" << std::endl;

    // Call out to the Go engine (stubbed here)
    SearchMatch match = InvokeGoSearch(currentText, pattern, currentOffset, options);

    if (match.startPos != -1) {
        std::cout << "[SearchManager] Match found at [" << match.startPos << ", " << match.endPos << "]" << std::endl;

        // Notify MsgWindow for status updates
        if (m_msgWindow) {
            m_msgWindow->Log(MsgType::Status, "Match found.");
        }

        // Stub: Tell ScintillaWrapper to highlight the selection
        // editor->SetSelection(match.startPos, match.endPos);
        return true;
    }

    if (m_msgWindow) {
        m_msgWindow->Log(MsgType::Status, "Search pattern not found.");
    }
    return false;
}

bool SearchManager::ReplaceNext(const std::string& pattern, const std::string& replacement, const SearchOptions& options) {
    // Stub implementation
    std::cout << "[SearchManager] Executing ReplaceNext: '" << pattern << "' -> '" << replacement << "'" << std::endl;
    return true;
}

int SearchManager::ReplaceAll(const std::string& pattern, const std::string& replacement, const SearchOptions& options) {
    // Stub implementation
    std::cout << "[SearchManager] Executing ReplaceAll: '" << pattern << "' -> '" << replacement << "'" << std::endl;
    return 42; // Replaced 42 instances
}

void SearchManager::FindInFiles(const std::string& directory, const std::string& pattern, const SearchOptions& options) {
    std::cout << "[SearchManager] Executing FindInFiles: '" << pattern << "' in '" << directory << "'" << std::endl;

    // In a real implementation, this would asynchronously invoke the `geany-go/search` package
    // to scan the directory concurrently.

    // For now, we simulate logging results directly to the MsgWindow (Search tab).
    if (m_msgWindow) {
        m_msgWindow->Clear(MsgType::Search);
        m_msgWindow->Log(MsgType::Search, "Searching for '" + pattern + "' in " + directory + "...");
        m_msgWindow->Log(MsgType::Search, "Result 1: int main() {", directory + "/main.cpp", 10);
        m_msgWindow->Log(MsgType::Search, "Result 2: int FindNext(", directory + "/search.cpp", 45);
        m_msgWindow->Log(MsgType::Search, "Search completed: 2 matches found.");
    }
}

SearchMatch SearchManager::InvokeGoSearch(const std::string& text, const std::string& pattern, int startOffset, const SearchOptions& options) const {
    // C++ Stub. Once `libgeanygo.so` exports `GeanyGo_Search_FindNext`, we call it here.
    // For tests, we hardcode a fake positive match if the text contains the pattern.

    SearchMatch m = {-1, -1, ""};
    size_t pos = text.find(pattern, startOffset);
    if (pos != std::string::npos) {
        m.startPos = static_cast<int>(pos);
        m.endPos = m.startPos + static_cast<int>(pattern.length());
        m.matchedText = pattern;
    }
    return m;
}

} // namespace geany
