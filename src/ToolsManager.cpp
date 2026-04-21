/**
 * ToolsManager.cpp
 *
 * Implementation of the C++ ToolsManager.
 */

#include "ToolsManager.h"
#include "DocumentManager.h"
#include <iostream>

namespace geany {

ToolsManager::ToolsManager(DocumentManager* docManager) : m_docManager(docManager) {
    if (!m_docManager) {
        std::cerr << "[ToolsManager] Warning: Initialized with null DocumentManager pointer." << std::endl;
    }
}

ToolsManager::~ToolsManager() {}

bool ToolsManager::PerformWordCount(WordCountResult& outResult) const {
    if (!m_docManager) return false;

    Document* activeDoc = m_docManager->GetDocument(0);
    // In a real implementation, DocumentManager would have a GetCurrentDocument()
    // For this stub, we'll assume we are getting properties from ScintillaWrapper.

    std::cout << "[ToolsManager] Performing Word Count on active document..." << std::endl;

    // Stub calculation
    outResult.lines = 100;
    outResult.words = 500;
    outResult.chars = 3000;
    outResult.selectionLines = 0;
    outResult.selectionWords = 0;
    outResult.selectionChars = 0;

    return true;
}

std::string ToolsManager::ShowColorChooser(const std::string& defaultColor) const {
    std::cout << "[ToolsManager] Opening native Color Chooser dialog (Default: " << defaultColor << ")" << std::endl;
    // Stub: This would typically invoke a GTK/Qt dialog natively or via the new Go submodules.
    // For now, simulate the user clicking "Cancel" (returning empty) or selecting a color.
    return "#FF00FF"; // Simulate picking Magenta
}

void ToolsManager::ReloadConfiguration() const {
    std::cout << "[ToolsManager] Reloading IDE Configuration files dynamically." << std::endl;
    // Stub: Re-invoke the Go config manager load sequence here.
}

} // namespace geany
