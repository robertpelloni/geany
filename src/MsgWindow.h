/**
 * MsgWindow.h
 *
 * Modern C++ refactor of `src/msgwindow.c`.
 * This class abstracts the bottom panel's message queues (Compiler, Messages, Search Results).
 * It replaces scattered GSList logging and manual string allocations with
 * a robust object-oriented system capable of routing structured messages to agnostic UIs.
 */

#ifndef GEANY_MSG_WINDOW_H
#define GEANY_MSG_WINDOW_H

#include <string>
#include <vector>
#include <mutex>

namespace geany {

// The type of message being logged (determines which tab/color is used).
enum class MsgType {
    Compiler,
    Message,
    Search,
    Status
};

// Represents a single structured log entry in the Message Window.
struct MsgEntry {
    MsgType type;
    std::string text;
    std::string filename; // Optional context for double-click jumping
    int line;             // Optional context (-1 if none)
};

class MsgWindow {
public:
    MsgWindow();
    ~MsgWindow();

    // Prevent copying
    MsgWindow(const MsgWindow&) = delete;
    MsgWindow& operator=(const MsgWindow&) = delete;

    // Appends a message to the specified tab.
    void Log(MsgType type, const std::string& text, const std::string& filename = "", int line = -1);

    // Clears all messages from a specific tab.
    void Clear(MsgType type);

    // Clears all messages across all tabs.
    void ClearAll();

    // Retrieves all messages for a specific tab (useful for UI rendering).
    std::vector<MsgEntry> GetMessages(MsgType type) const;

    // Changes the visibility state of the message window itself.
    void SetVisible(bool visible);
    bool IsVisible() const;

private:
    bool m_visible;
    std::vector<MsgEntry> m_compilerMsgs;
    std::vector<MsgEntry> m_generalMsgs;
    std::vector<MsgEntry> m_searchMsgs;

    // Helper to select the correct vector based on MsgType.
    std::vector<MsgEntry>& GetBuffer(MsgType type);
    const std::vector<MsgEntry>& GetBuffer(MsgType type) const;
};

} // namespace geany

#endif // GEANY_MSG_WINDOW_H
