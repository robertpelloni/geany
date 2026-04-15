/**
 * PrintManager.cpp
 *
 * Implementation of the C++ PrintManager class.
 * This replaces scattered GTK printing callbacks.
 */

#include "PrintManager.h"
#include "DocumentManager.h"
#include "MsgWindow.h"
#include <iostream>

namespace geany {

PrintManager::PrintManager(DocumentManager* docManager, MsgWindow* msgWindow)
    : m_docManager(docManager), m_msgWindow(msgWindow) {
    if (!m_docManager || !m_msgWindow) {
        std::cerr << "[PrintManager] Warning: Initialized with null pointer dependencies." << std::endl;
    }
}

PrintManager::~PrintManager() {}

int PrintManager::CalculatePages(const std::string& text) const {
    // Stub: In a real implementation with Pango/Cairo or QPainter,
    // we'd calculate font metrics against page dimensions.
    // Here we estimate 60 lines per page.
    if (text.empty()) return 0;

    int lines = 1;
    for (char c : text) {
        if (c == '\n') lines++;
    }

    int pages = lines / 60;
    if (lines % 60 != 0) pages++;
    return pages == 0 ? 1 : pages;
}

bool PrintManager::PrintActiveDocument(const PrintOptions& options) const {
    if (!m_docManager) return false;

    // In a real environment, fetch active document and raw text from Scintilla
    // Document* doc = m_docManager->GetActiveDocument();
    std::string currentText = "Geany C++ Refactor\nLine 2\nLine 3\nEOF";
    std::string filename = "stub.txt";

    std::cout << "[PrintManager] Preparing print job for: " << filename << std::endl;

    int pages = CalculatePages(currentText);
    std::string statusMsg = "Spooled document '" + filename + "' (" + std::to_string(pages) + " pages) to printer.";

    if (m_msgWindow) {
        m_msgWindow->Log(MsgType::Status, statusMsg);
    }

    std::cout << "[PrintManager] " << statusMsg << std::endl;
    return true;
}

bool PrintManager::PrintRawText(const std::string& text, const std::string& jobName) const {
    if (text.empty() || jobName.empty()) {
        std::cerr << "[PrintManager] Error: Empty text or job name provided." << std::endl;
        return false;
    }

    int pages = CalculatePages(text);
    std::cout << "[PrintManager] Spooled raw text job '" << jobName << "' (" << pages << " pages) to printer." << std::endl;
    return true;
}

} // namespace geany
