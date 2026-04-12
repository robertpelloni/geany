/**
 * ProjectManager.cpp
 *
 * Implementation of the C++ ProjectManager and Project classes using STL strings and unique_ptr.
 */

#include "ProjectManager.h"
#include <iostream>

namespace geany {

// --- Project Implementation ---

Project::Project() {}

Project::~Project() {}

void Project::AddFilePattern(const std::string& pattern) {
    m_filePatterns.push_back(pattern);
}

void Project::ClearFilePatterns() {
    m_filePatterns.clear();
}

bool Project::IsValid() const {
    return !m_fileName.empty() && !m_name.empty();
}

// --- ProjectManager Implementation ---

ProjectManager::ProjectManager() : m_activeProject(nullptr) {}

ProjectManager::~ProjectManager() {
    CloseProject();
}

Project* ProjectManager::NewProject(const std::string& name, const std::string& filename, const std::string& basePath) {
    if (m_activeProject) {
        std::cout << "[ProjectManager] Closing existing project before creating a new one." << std::endl;
        CloseProject();
    }

    m_activeProject = std::make_unique<Project>();
    m_activeProject->SetName(name);
    m_activeProject->SetFileName(filename);
    m_activeProject->SetBasePath(basePath);

    std::cout << "[ProjectManager] Created new project: " << name << " (" << filename << ")" << std::endl;
    return m_activeProject.get();
}

bool ProjectManager::OpenProject(const std::string& filename) {
    if (m_activeProject) {
        std::cout << "[ProjectManager] Closing existing project before opening a new one." << std::endl;
        CloseProject();
    }

    // Stub: In reality, we'd use a KeyFile/ini parser here to load properties from the disk.
    // For now, we simulate a successful load:
    m_activeProject = std::make_unique<Project>();

    // Naively extract the base name as the generic "name" for the stub
    size_t lastSlash = filename.find_last_of("/\\");
    std::string base = (lastSlash == std::string::npos) ? filename : filename.substr(lastSlash + 1);

    // Strip extension
    size_t lastDot = base.find_last_of('.');
    if (lastDot != std::string::npos) {
        base = base.substr(0, lastDot);
    }

    m_activeProject->SetName(base);
    m_activeProject->SetFileName(filename);

    std::string mockBasePath = (lastSlash == std::string::npos) ? "./" : filename.substr(0, lastSlash + 1);
    m_activeProject->SetBasePath(mockBasePath);

    std::cout << "[ProjectManager] Opened project file: " << filename << std::endl;
    return true;
}

bool ProjectManager::SaveProject() {
    if (!m_activeProject || !m_activeProject->IsValid()) {
        std::cerr << "[ProjectManager] No valid active project to save." << std::endl;
        return false;
    }

    // Stub: Serialize the Project state to the KeyFile/ini format on disk.
    std::cout << "[ProjectManager] Saving active project to: " << m_activeProject->GetFileName() << std::endl;
    return true;
}

void ProjectManager::CloseProject() {
    if (m_activeProject) {
        std::cout << "[ProjectManager] Closing active project: " << m_activeProject->GetName() << std::endl;
        // The unique_ptr automatically cleans up the memory when reset is called.
        m_activeProject.reset();
    }
}

Project* ProjectManager::GetActiveProject() const {
    return m_activeProject.get();
}

} // namespace geany
