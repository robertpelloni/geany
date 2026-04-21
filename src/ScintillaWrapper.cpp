/**
 * ScintillaWrapper.cpp
 */

#include "ScintillaWrapper.h"

// Note: In a full integration, we would include "scintilla.h" directly.
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

// Updated signature to match the actual Scintilla API requirements for 64-bit compatibility
extern "C" intptr_t scintilla_send_message(ScintillaObject* sci, unsigned int iMessage, uintptr_t wParam, intptr_t lParam);

namespace geany {

ScintillaWrapper::ScintillaWrapper(ScintillaObject* sci_obj) : m_sci(sci_obj) {}

ScintillaWrapper::~ScintillaWrapper() {}

intptr_t ScintillaWrapper::SendEditor(unsigned int message, uintptr_t wParam, intptr_t lParam) const {
    if (!m_sci) return 0;
    return scintilla_send_message(m_sci, message, wParam, lParam);
}

std::string ScintillaWrapper::GetText() const {
    int length = GetLength();
    if (length == 0) return "";

    std::vector<char> buffer(length + 1);
    SendEditor(SCI_GETTEXT, length + 1, reinterpret_cast<intptr_t>(buffer.data()));
    return std::string(buffer.data());
}

void ScintillaWrapper::SetText(const std::string& text) {
    SendEditor(SCI_SETTEXT, 0, reinterpret_cast<intptr_t>(text.c_str()));
}

int ScintillaWrapper::GetLineCount() const {
    return static_cast<int>(SendEditor(SCI_GETLINECOUNT));
}

void ScintillaWrapper::AppendText(const std::string& text) {
    SendEditor(SCI_APPENDTEXT, text.length(), reinterpret_cast<intptr_t>(text.c_str()));
}

void ScintillaWrapper::ClearAll() {
    SendEditor(SCI_CLEARALL);
}

int ScintillaWrapper::GetLength() const {
    return static_cast<int>(SendEditor(SCI_GETLENGTH));
}

void ScintillaWrapper::SetReadOnly(bool readOnly) {
    SendEditor(SCI_SETREADONLY, readOnly ? 1 : 0);
}

bool ScintillaWrapper::GetReadOnly() const {
    return SendEditor(SCI_GETREADONLY) != 0;
}

} // namespace geany
