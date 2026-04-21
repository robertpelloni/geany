/**
 * DocumentManager.cpp
 *
 * Implementation of the C++ DocumentManager using STL vectors.
 */

#include "DocumentManager.h"
#include <iostream>
#include <algorithm>

namespace geany {

// --- Document Implementation ---

Document::Document(int id, const std::string& filename)
    : m_id(id), m_fileName(filename), m_changed(false), m_readOnly(false) {
}

// --- DocumentManager Implementation ---

DocumentManager::DocumentManager() {
    // Reserve some initial capacity to avoid early reallocations
    m_documents.reserve(10);
}

DocumentManager::~DocumentManager() {
    // The unique_ptrs in m_documents will automatically clean up the Document objects.
    m_documents.clear();
}

int DocumentManager::NewDocument() {
    int newId = static_cast<int>(m_documents.size());
    m_documents.push_back(std::make_unique<Document>(newId, "Untitled"));
    std::cout << "[DocumentManager] Created new document (ID: " << newId << ")" << std::endl;
    return newId;
}

int DocumentManager::OpenDocument(const std::string& filename) {
    // Basic deduplication check
    for (const auto& doc : m_documents) {
        if (doc && doc->GetFileName() == filename) {
            std::cout << "[DocumentManager] Document already open: " << filename << std::endl;
            return doc->GetId();
        }
    }

    int newId = static_cast<int>(m_documents.size());
    m_documents.push_back(std::make_unique<Document>(newId, filename));
    std::cout << "[DocumentManager] Opened document: " << filename << " (ID: " << newId << ")" << std::endl;
    return newId;
}

bool DocumentManager::SaveDocument(int id) {
    Document* doc = GetDocument(id);
    if (!doc) return false;

    if (doc->IsReadOnly()) {
        std::cerr << "[DocumentManager] Cannot save read-only document: " << doc->GetFileName() << std::endl;
        return false;
    }

    std::cout << "[DocumentManager] Saving document: " << doc->GetFileName() << std::endl;
    doc->SetChanged(false);
    return true;
}

bool DocumentManager::CloseDocument(int id) {
    if (id < 0 || id >= static_cast<int>(m_documents.size())) return false;

    // In a real implementation, we'd check `IsChanged()` and prompt the user here.
    // For now, we simulate a successful close by clearing the pointer slot.
    m_documents[id].reset();
    std::cout << "[DocumentManager] Closed document ID: " << id << std::endl;
    return true;
}

Document* DocumentManager::GetDocument(int id) const {
    if (id >= 0 && id < static_cast<int>(m_documents.size())) {
        return m_documents[id].get();
    }
    return nullptr;
}

size_t DocumentManager::GetCount() const {
    // Count non-null pointers
    return std::count_if(m_documents.begin(), m_documents.end(), [](const std::unique_ptr<Document>& doc) {
        return doc != nullptr;
    });
}

} // namespace geany
