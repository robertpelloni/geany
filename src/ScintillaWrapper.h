/**
 * ScintillaWrapper.h
 *
 * Modern C++ wrapper around the raw C Scintilla API.
 */

#ifndef SCINTILLA_WRAPPER_H
#define SCINTILLA_WRAPPER_H

#include <string>
#include <vector>
#include <cstdint>

typedef struct _ScintillaObject ScintillaObject;

namespace geany {

class ScintillaWrapper {
public:
    explicit ScintillaWrapper(ScintillaObject* sci_obj);
    ~ScintillaWrapper();

    std::string GetText() const;
    void SetText(const std::string& text);
    int GetLineCount() const;
    void AppendText(const std::string& text);
    void ClearAll();
    int GetLength() const;
    void SetReadOnly(bool readOnly);
    bool GetReadOnly() const;

    // Make SendEditor public for SyntaxHighlighter to use
    intptr_t SendEditor(unsigned int message, uintptr_t wParam = 0, intptr_t lParam = 0) const;

private:
    ScintillaObject* m_sci;
};

} // namespace geany

#endif // SCINTILLA_WRAPPER_H
