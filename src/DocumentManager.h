/**
 * DocumentManager.h
 *
 * Modern C++ refactor of the core of `src/document.c`.
 * This class replaces the global `documents_array` and provides a clean,
 * object-oriented interface for managing open Geany documents.
 */

#ifndef GEANY_DOCUMENT_MANAGER_H
#define GEANY_DOCUMENT_MANAGER_H

#include <string>
#include <vector>
#include <memory>

namespace geany {

// Forward declaration of the C++ Document representation.
class Document;

class DocumentManager {
public:
    DocumentManager();
    ~DocumentManager();

    // Prevent copying
    DocumentManager(const DocumentManager&) = delete;
    DocumentManager& operator=(const DocumentManager&) = delete;

    // Create a new empty document. Returns the internal index/ID.
    int NewDocument();

    // Open an existing file. Returns the internal index/ID, or -1 on failure.
    int OpenDocument(const std::string& filename);

    // Save a document by its ID.
    bool SaveDocument(int id);

    // Close a document by its ID. Returns true if successful (e.g., user didn't cancel save prompt).
    bool CloseDocument(int id);

    // Retrieve a pointer to the document by ID. Returns nullptr if invalid.
    Document* GetDocument(int id) const;

    // Get the total number of open documents.
    size_t GetCount() const;

private:
    // Safely manages the lifecycle of all open documents.
    std::vector<std::unique_ptr<Document>> m_documents;
};

// Represents a single open file within Geany.
// This mirrors the `geany-go/editor/Document` structure.
class Document {
public:
    Document(int id, const std::string& filename);
    ~Document() = default;

    int GetId() const { return m_id; }
    const std::string& GetFileName() const { return m_fileName; }

    bool IsChanged() const { return m_changed; }
    void SetChanged(bool changed) { m_changed = changed; }

    bool IsReadOnly() const { return m_readOnly; }
    void SetReadOnly(bool readOnly) { m_readOnly = readOnly; }

private:
    int m_id;
    std::string m_fileName;
    bool m_changed;
    bool m_readOnly;
};

} // namespace geany

#endif // GEANY_DOCUMENT_MANAGER_H
