/**
 * KeybindingManager.h
 *
 * Modern C++ refactor of the core of `src/keybindings.c`.
 * This class replaces the global keybinding arrays and static handler functions
 * with a dynamic, object-oriented manager mapping action names to callback handlers.
 */

#ifndef GEANY_KEYBINDING_MANAGER_H
#define GEANY_KEYBINDING_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace geany {

// Defines a specific keyboard shortcut bound to an action.
struct Keybinding {
    std::string actionName;
    std::string label;        // UI Label e.g., "Save File"
    std::string defaultKey;   // e.g., "<Ctrl>S"

    // The callback to execute when the keybinding is triggered.
    std::function<void()> callback;
};

// Manages registration, triggering, and lookup of IDE keybindings.
class KeybindingManager {
public:
    KeybindingManager();
    ~KeybindingManager();

    // Prevent copying
    KeybindingManager(const KeybindingManager&) = delete;
    KeybindingManager& operator=(const KeybindingManager&) = delete;

    // Register a new keybinding into the system.
    bool Register(const std::string& actionName, const std::string& label, const std::string& defaultKey, std::function<void()> callback);

    // Unregister an existing keybinding.
    bool Unregister(const std::string& actionName);

    // Triggers the callback associated with the given action name.
    bool Trigger(const std::string& actionName) const;

    // Simulate a raw key press (e.g. "<Ctrl>S") checking against registered bindings.
    // In a real implementation, this maps GTK/Qt key events to actionNames.
    bool HandleKeyPress(const std::string& keyString) const;

    // Retrieves all currently registered keybindings.
    std::vector<Keybinding> GetAllBindings() const;

private:
    // Safely maps an internal action name to its full binding structure.
    std::map<std::string, Keybinding> m_bindings;

    // Maps raw shortcut strings ("<Ctrl>S") to their corresponding actionNames.
    std::map<std::string, std::string> m_keyMap;
};

} // namespace geany

#endif // GEANY_KEYBINDING_MANAGER_H
