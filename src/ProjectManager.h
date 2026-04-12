/**
 * ProjectManager.h
 *
 * Modern C++ refactor of the core of `src/project.c`.
 * This class encapsulates the global `GeanyProject` struct and provides an object-oriented
 * interface for handling project creation, loading, saving, and querying properties.
 */

#ifndef GEANY_PROJECT_MANAGER_H
#define GEANY_PROJECT_MANAGER_H

#include <string>
#include <memory>
#include <vector>

namespace geany {

// Represents a Geany Project, replacing the old GeanyProject struct.
class Project {
public:
    Project();
    ~Project();

    // Prevent copying
    Project(const Project&) = delete;
    Project& operator=(const Project&) = delete;

    // Getters and Setters
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }

    const std::string& GetFileName() const { return m_fileName; }
    void SetFileName(const std::string& filename) { m_fileName = filename; }

    const std::string& GetBasePath() const { return m_basePath; }
    void SetBasePath(const std::string& basePath) { m_basePath = basePath; }

    const std::string& GetDescription() const { return m_description; }
    void SetDescription(const std::string& description) { m_description = description; }

    // File Patterns
    const std::vector<std::string>& GetFilePatterns() const { return m_filePatterns; }
    void AddFilePattern(const std::string& pattern);
    void ClearFilePatterns();

    // Check if a project is currently open/valid
    bool IsValid() const;

private:
    std::string m_name;
    std::string m_fileName; // The path to the .geany file
    std::string m_basePath;
    std::string m_description;
    std::vector<std::string> m_filePatterns;
};

// Manages the global project state for the IDE instance.
class ProjectManager {
public:
    ProjectManager();
    ~ProjectManager();

    // Prevent copying
    ProjectManager(const ProjectManager&) = delete;
    ProjectManager& operator=(const ProjectManager&) = delete;

    // Creates a new, blank project. Returns a pointer to the new active project.
    Project* NewProject(const std::string& name, const std::string& filename, const std::string& basePath);

    // Opens an existing project file.
    bool OpenProject(const std::string& filename);

    // Saves the currently active project.
    bool SaveProject();

    // Closes the active project, clearing the manager's state.
    void CloseProject();

    // Returns the currently active project, or nullptr if none is open.
    Project* GetActiveProject() const;

private:
    // The currently loaded project (Geany typically only has one active project at a time).
    std::unique_ptr<Project> m_activeProject;
};

} // namespace geany

#endif // GEANY_PROJECT_MANAGER_H
