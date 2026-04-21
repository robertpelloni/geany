/**
 * SyntaxHighlighter.cpp
 *
 * Implementation of the C++ SyntaxHighlighter.
 */

#include "SyntaxHighlighter.h"
#include "ScintillaWrapper.h"
#include "FileTypeManager.h" // For FileType definition
#include <iostream>

// Stub Scintilla messages normally found in scintilla.h
#ifndef SCI_SETLEXER
#define SCI_SETLEXER 4001
#define SCI_SETKEYWORDS 4005
#define SCI_STYLESETFORE 2051
#define SCI_STYLESETBACK 2052
#define SCI_STYLESETBOLD 2053
#define SCI_STYLESETITALIC 2054
#endif

namespace geany {

SyntaxHighlighter::SyntaxHighlighter() {
    InitKeywords();
}

SyntaxHighlighter::~SyntaxHighlighter() {}

void SyntaxHighlighter::InitKeywords() {
    // Populate the default generic keyword sets.
    m_keywordSets["c"] = "int float double char struct union enum typedef const static volatile return if else for while do switch case default break continue goto sizeof";
    m_keywordSets["python"] = "def class return if elif else for while break continue try except finally raise assert import from as pass global nonlocal lambda yield with";
    // Go port keywords map exactly to our go parser logic
    m_keywordSets["go"] = "break default func interface select case defer go map struct chan else goto package switch const fallthrough if range type continue for import return var";
}

bool SyntaxHighlighter::ApplyHighlighting(ScintillaWrapper* editor, const FileType* fileType) {
    if (!editor || !fileType) {
        std::cerr << "[SyntaxHighlighter] Error: Null editor or filetype provided." << std::endl;
        return false;
    }

    std::cout << "[SyntaxHighlighter] Applying styling for language: " << fileType->title << " (Lexer ID: " << fileType->lexerId << ")" << std::endl;

    // 1. Tell Scintilla which parser to use
    editor->SendEditor(SCI_SETLEXER, fileType->lexerId, 0);

    // 2. Look up keywords for this language and send them to Scintilla
    auto it = m_keywordSets.find(fileType->id);
    if (it != m_keywordSets.end()) {
        std::cout << "[SyntaxHighlighter] Loading keyword set for " << fileType->id << std::endl;

        // Keyword set 0 is usually primary keywords (e.g. control flow, types)
        editor->SendEditor(SCI_SETKEYWORDS, 0, reinterpret_cast<intptr_t>(it->second.c_str()));
    } else {
        std::cout << "[SyntaxHighlighter] No keyword set defined for " << fileType->id << std::endl;
    }

    return true;
}

void SyntaxHighlighter::SetStyle(ScintillaWrapper* editor, int styleId, const std::string& foreColor, const std::string& backColor, bool bold, bool italic) {
    if (!editor) return;

    // Note: Parsing color hex strings ("#FF0000") into Scintilla integer colors
    // requires a helper function. We stub this for now.
    int foreInt = 0x000000; // Black
    int backInt = 0xFFFFFF; // White

    editor->SendEditor(SCI_STYLESETFORE, styleId, foreInt);
    editor->SendEditor(SCI_STYLESETBACK, styleId, backInt);
    editor->SendEditor(SCI_STYLESETBOLD, styleId, bold ? 1 : 0);
    editor->SendEditor(SCI_STYLESETITALIC, styleId, italic ? 1 : 0);
}

} // namespace geany
