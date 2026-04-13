/**
 * ToolsManager.h
 *
 * Modern C++ refactor of `src/tools.c`.
 * This class encapsulates the various utility tools available in the IDE,
 * such as Word Count, Color Chooser, and Reload Configuration.
 *
 * It acts as an orchestrator that can be invoked via menu items or keybindings.
 */

#ifndef GEANY_TOOLS_MANAGER_H
#define GEANY_TOOLS_MANAGER_H

#include <string>

namespace geany {

// Forward declarations of dependent classes
class DocumentManager;

// Represents the result of a Word Count operation
struct WordCountResult {
    int lines;
    int words;
    int chars;
    int selectionLines;
    int selectionWords;
    int selectionChars;
};

class ToolsManager {
public:
    // Requires a pointer to the DocumentManager to inspect the active document.
    explicit ToolsManager(DocumentManager* docManager);
    ~ToolsManager();

    // Prevent copying
    ToolsManager(const ToolsManager&) = delete;
    ToolsManager& operator=(const ToolsManager&) = delete;

    // Computes the line, word, and character counts for the currently active document.
    // Returns true if a valid document was active and counted.
    bool PerformWordCount(WordCountResult& outResult) const;

    // Spawns a color chooser dialog (UI dependent implementation stub).
    // Returns the selected hex string, or an empty string if canceled.
    std::string ShowColorChooser(const std::string& defaultColor = "#000000") const;

    // Reloads global configurations dynamically.
    void ReloadConfiguration() const;

private:
    DocumentManager* m_docManager;
};

} // namespace geany

#endif // GEANY_TOOLS_MANAGER_H
