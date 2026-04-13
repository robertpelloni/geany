/**
 * FileTypeManager.h
 *
 * Modern C++ refactor of `src/filetypes.c`.
 * Replaces the global, static `GeanyFiletype` arrays with a dynamic registry
 * backed by STL containers for mapping extensions and identifiers to language lexers.
 */

#ifndef GEANY_FILETYPE_MANAGER_H
#define GEANY_FILETYPE_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace geany {

// Core C++ representation of a language definition.
struct FileType {
    std::string id;          // Internal name, e.g., "cpp"
    std::string title;       // Display name, e.g., "C++"
    std::string defaultExt;  // e.g., ".cpp"
    std::vector<std::string> patterns; // e.g., {"*.cpp", "*.cxx", "*.h"}
    int lexerId;             // Scintilla lexer constant
};

class FileTypeManager {
public:
    FileTypeManager();
    ~FileTypeManager();

    // Prevent copying
    FileTypeManager(const FileTypeManager&) = delete;
    FileTypeManager& operator=(const FileTypeManager&) = delete;

    // Registers a new filetype into the IDE.
    void Register(const FileType& ft);

    // Retrieves a filetype definition by its internal ID (e.g. "python").
    const FileType* GetById(const std::string& id) const;

    // Determines the filetype based on the given filename's extension or exact match.
    const FileType* Detect(const std::string& filename) const;

    // Returns a list of all currently registered filetypes.
    std::vector<const FileType*> GetAllTypes() const;

private:
    std::map<std::string, std::unique_ptr<FileType>> m_fileTypes;

    // Internal initialization of default builtin languages (C, Python, None, etc.)
    void InitDefaults();
};

} // namespace geany

#endif // GEANY_FILETYPE_MANAGER_H
