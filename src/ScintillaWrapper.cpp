/**
 * ScintillaWrapper.cpp
 *
 * Implementation of the type-safe C++ wrapper for Scintilla.
 * This class abstracts away `scintilla_send_message` logic.
 */

#include "ScintillaWrapper.h"

// In a real build, we would include the actual Scintilla header here:
// #include "scintilla.h"
// #include "SciLexer.h"

// For testing compilation without full GTK/Scintilla headers present in the environment,
// we stub the necessary constants and functions.
#ifndef SCI_GETTEXT
#define SCI_GETTEXT 2182
#define SCI_SETTEXT 2181
#define SCI_GETLINECOUNT 2154
#define SCI_APPENDTEXT 2282
#define SCI_CLEARALL 2004
#define SCI_GETLENGTH 2006
#define SCI_SETREADONLY 2171
#define SCI_GETREADONLY 2140
#endif

// Stub the C function we are wrapping.
extern "C" long scintilla_send_message(ScintillaObject* sci, unsigned int iMessage, unsigned long wParam, long lParam);

namespace geany {

ScintillaWrapper::ScintillaWrapper(ScintillaObject* sci_obj) : m_sci(sci_obj) {}

ScintillaWrapper::~ScintillaWrapper() {
    // We do not destroy the widget here; GTK manages the widget lifecycle.
}

long ScintillaWrapper::SendEditor(unsigned int message, unsigned long wParam, long lParam) const {
    if (!m_sci) return 0;
    return scintilla_send_message(m_sci, message, wParam, lParam);
}

std::string ScintillaWrapper::GetText() const {
    int length = GetLength();
    if (length == 0) return "";

    std::vector<char> buffer(length + 1);
    SendEditor(SCI_GETTEXT, length + 1, reinterpret_cast<long>(buffer.data()));
    return std::string(buffer.data());
}

void ScintillaWrapper::SetText(const std::string& text) {
    SendEditor(SCI_SETTEXT, 0, reinterpret_cast<long>(text.c_str()));
}

int ScintillaWrapper::GetLineCount() const {
    return SendEditor(SCI_GETLINECOUNT);
}

void ScintillaWrapper::AppendText(const std::string& text) {
    SendEditor(SCI_APPENDTEXT, text.length(), reinterpret_cast<long>(text.c_str()));
}

void ScintillaWrapper::ClearAll() {
    SendEditor(SCI_CLEARALL);
}

int ScintillaWrapper::GetLength() const {
    return SendEditor(SCI_GETLENGTH);
}

void ScintillaWrapper::SetReadOnly(bool readOnly) {
    SendEditor(SCI_SETREADONLY, readOnly ? 1 : 0);
}

bool ScintillaWrapper::GetReadOnly() const {
    return SendEditor(SCI_GETREADONLY) != 0;
}

} // namespace geany
