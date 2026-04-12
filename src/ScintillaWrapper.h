/**
 * ScintillaWrapper.h
 *
 * This file introduces a modern, type-safe C++ wrapper around the raw C Scintilla API.
 * The goal is to encapsulate the messy pointer arithmetic and naked integer message IDs
 * into a clean, object-oriented RAII interface.
 *
 * This is a foundational step in the C++ refactoring roadmap (as outlined in VISION.md).
 */

#ifndef SCINTILLA_WRAPPER_H
#define SCINTILLA_WRAPPER_H

#include <string>
#include <vector>

// Forward declare the opaque C struct to avoid bringing in GTK/Scintilla headers unnecessarily here.
typedef struct _ScintillaObject ScintillaObject;

namespace geany {

class ScintillaWrapper {
public:
    // Construct the wrapper around an existing GTK Scintilla widget instance.
    explicit ScintillaWrapper(ScintillaObject* sci_obj);

    // Destructor - note: this wrapper does not currently assume ownership of the widget lifecycle.
    ~ScintillaWrapper();

    // -- Type-safe API methods --

    // Retrieves the entire text of the document.
    std::string GetText() const;

    // Sets the entire text of the document.
    void SetText(const std::string& text);

    // Retrieves the total number of lines in the document.
    int GetLineCount() const;

    // Appends text to the end of the document.
    void AppendText(const std::string& text);

    // Clears the entire document.
    void ClearAll();

    // Retrieves the length of the document in bytes.
    int GetLength() const;

    // Sets whether the document is read-only.
    void SetReadOnly(bool readOnly);

    // Checks if the document is read-only.
    bool GetReadOnly() const;

private:
    ScintillaObject* m_sci; // Raw pointer to the underlying Scintilla instance

    // Internal helper to send generic messages to the underlying widget.
    long SendEditor(unsigned int message, unsigned long wParam = 0, long lParam = 0) const;
};

} // namespace geany

#endif // SCINTILLA_WRAPPER_H
