/**
 * PrintManager.h
 *
 * Modern C++ refactor of `src/printing.c`.
 * This class abstracts the low-level GTK print dialogs and pagination logic
 * into an object-oriented interface. It interacts closely with the active
 * document's text buffer to generate paginated output.
 */

#ifndef GEANY_PRINT_MANAGER_H
#define GEANY_PRINT_MANAGER_H

#include <string>
#include <vector>

namespace geany {

class DocumentManager;
class MsgWindow;

// Configuration options for a print job
struct PrintOptions {
    bool printLineNumbers;
    bool printHeader;
    bool colorSyntax;
    int tabsToSpaces;
};

class PrintManager {
public:
    PrintManager(DocumentManager* docManager, MsgWindow* msgWindow);
    ~PrintManager();

    // Prevent copying
    PrintManager(const PrintManager&) = delete;
    PrintManager& operator=(const PrintManager&) = delete;

    // Initiates the print dialog and routing for the currently active document.
    // Returns true if the job was spooled successfully, false if cancelled or error.
    bool PrintActiveDocument(const PrintOptions& options) const;

    // Spools a raw string of text directly to the printer using default settings.
    bool PrintRawText(const std::string& text, const std::string& jobName) const;

private:
    DocumentManager* m_docManager;
    MsgWindow* m_msgWindow;

    // Simulates paginating a document for the printer canvas.
    int CalculatePages(const std::string& text) const;
};

} // namespace geany

#endif // GEANY_PRINT_MANAGER_H
