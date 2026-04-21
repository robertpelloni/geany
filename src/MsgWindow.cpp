/**
 * MsgWindow.cpp
 *
 * Implementation of the C++ MsgWindow data model.
 */

#include "MsgWindow.h"
#include <iostream>
#include <stdexcept>

namespace geany {

MsgWindow::MsgWindow() : m_visible(true) {}

MsgWindow::~MsgWindow() {
    ClearAll();
}

void MsgWindow::Log(MsgType type, const std::string& text, const std::string& filename, int line) {
    if (text.empty()) return;

    MsgEntry entry;
    entry.type = type;
    entry.text = text;
    entry.filename = filename;
    entry.line = line;

    // Store the message in the correct internal buffer
    GetBuffer(type).push_back(entry);

    // Echo to stdout for terminal users or debugging
    std::string prefix = "";
    switch (type) {
        case MsgType::Compiler: prefix = "[Compiler] "; break;
        case MsgType::Message:  prefix = "[Message] "; break;
        case MsgType::Search:   prefix = "[Search] "; break;
        case MsgType::Status:   prefix = "[Status] "; break;
    }

    std::cout << prefix << text;
    if (!filename.empty() && line >= 0) {
        std::cout << " (" << filename << ":" << line << ")";
    }
    std::cout << std::endl;

    // In a fully integrated environment, we would emit a signal or invoke a callback here
    // to notify the Go UI interface that a new message arrived.
}

void MsgWindow::Clear(MsgType type) {
    GetBuffer(type).clear();
}

void MsgWindow::ClearAll() {
    m_compilerMsgs.clear();
    m_generalMsgs.clear();
    m_searchMsgs.clear();
}

std::vector<MsgEntry> MsgWindow::GetMessages(MsgType type) const {
    // Return a copy of the requested buffer
    return GetBuffer(type);
}

void MsgWindow::SetVisible(bool visible) {
    m_visible = visible;
    std::cout << "[MsgWindow] Visibility set to: " << (visible ? "true" : "false") << std::endl;
}

bool MsgWindow::IsVisible() const {
    return m_visible;
}

// Internal Helpers

std::vector<MsgEntry>& MsgWindow::GetBuffer(MsgType type) {
    switch (type) {
        case MsgType::Compiler: return m_compilerMsgs;
        case MsgType::Search:   return m_searchMsgs;
        case MsgType::Message:
        case MsgType::Status:
        default:                return m_generalMsgs;
    }
}

const std::vector<MsgEntry>& MsgWindow::GetBuffer(MsgType type) const {
    switch (type) {
        case MsgType::Compiler: return m_compilerMsgs;
        case MsgType::Search:   return m_searchMsgs;
        case MsgType::Message:
        case MsgType::Status:
        default:                return m_generalMsgs;
    }
}

} // namespace geany
