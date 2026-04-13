/**
 * KeybindingManager.cpp
 *
 * Implementation of the C++ KeybindingManager using STL maps and functional callbacks.
 */

#include "KeybindingManager.h"
#include <iostream>

namespace geany {

KeybindingManager::KeybindingManager() {}

KeybindingManager::~KeybindingManager() {
    m_bindings.clear();
    m_keyMap.clear();
}

bool KeybindingManager::Register(const std::string& actionName, const std::string& label, const std::string& defaultKey, std::function<void()> callback) {
    if (m_bindings.find(actionName) != m_bindings.end()) {
        std::cerr << "[KeybindingManager] Error: Action already registered: " << actionName << std::endl;
        return false;
    }

    Keybinding kb;
    kb.actionName = actionName;
    kb.label = label;
    kb.defaultKey = defaultKey;
    kb.callback = std::move(callback);

    m_bindings[actionName] = kb;

    // Map the raw keystring if it exists
    if (!defaultKey.empty()) {
        m_keyMap[defaultKey] = actionName;
    }

    std::cout << "[KeybindingManager] Registered: " << actionName << " [" << defaultKey << "]" << std::endl;
    return true;
}

bool KeybindingManager::Unregister(const std::string& actionName) {
    auto it = m_bindings.find(actionName);
    if (it != m_bindings.end()) {
        // Remove from the raw key map as well
        if (!it->second.defaultKey.empty()) {
            m_keyMap.erase(it->second.defaultKey);
        }
        m_bindings.erase(it);
        std::cout << "[KeybindingManager] Unregistered: " << actionName << std::endl;
        return true;
    }
    return false;
}

bool KeybindingManager::Trigger(const std::string& actionName) const {
    auto it = m_bindings.find(actionName);
    if (it != m_bindings.end() && it->second.callback) {
        std::cout << "[KeybindingManager] Triggering action: " << actionName << std::endl;
        it->second.callback();
        return true;
    }
    return false;
}

bool KeybindingManager::HandleKeyPress(const std::string& keyString) const {
    auto mapIt = m_keyMap.find(keyString);
    if (mapIt != m_keyMap.end()) {
        return Trigger(mapIt->second);
    }
    return false;
}

std::vector<Keybinding> KeybindingManager::GetAllBindings() const {
    std::vector<Keybinding> list;
    list.reserve(m_bindings.size());
    for (const auto& pair : m_bindings) {
        list.push_back(pair.second);
    }
    return list;
}

} // namespace geany
