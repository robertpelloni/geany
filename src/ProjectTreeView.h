/**
 * ProjectTreeView.h
 *
 * Modern C++ refactor replacing the hardcoded GTK tree logic in `src/sidebar.c`.
 * This class abstracts the data model of a project's directory structure, allowing
 * different UI submodules (Qt, GTK) to consume the hierarchy without being
 * tightly coupled to a specific GUI toolkit's `GtkTreeModel`.
 */

#ifndef GEANY_PROJECT_TREE_VIEW_H
#define GEANY_PROJECT_TREE_VIEW_H

#include <string>
#include <vector>
#include <memory>

namespace geany {

// Node represents a single file or folder in the project sidebar.
struct TreeNode {
    std::string name;
    std::string fullPath;
    bool isDirectory;
    bool isExpanded; // State for UI consumption

    // For nested directory structures
    std::vector<std::unique_ptr<TreeNode>> children;

    // Optional pointer to parent (helpful for recursive UI updates)
    TreeNode* parent = nullptr;
};

// Orchestrates the internal data model for the "Project" sidebar tab.
class ProjectTreeView {
public:
    ProjectTreeView();
    ~ProjectTreeView();

    // Prevent copying
    ProjectTreeView(const ProjectTreeView&) = delete;
    ProjectTreeView& operator=(const ProjectTreeView&) = delete;

    // Sets the root directory and scans it to build the initial tree structure.
    bool SetRoot(const std::string& rootPath);

    // Re-reads the filesystem and updates the internal tree to match the disk state.
    void Refresh();

    // Returns a raw pointer to the invisible root node containing all top-level files/folders.
    const TreeNode* GetRootNode() const;

    // Retrieves a specific node by its absolute path.
    TreeNode* FindNodeByPath(const std::string& path) const;

    // Simulate expanding or collapsing a node in the UI.
    void ToggleNode(TreeNode* node);

    // Clears the entire tree (e.g., when a project is closed).
    void Clear();

private:
    std::string m_rootPath;
    std::unique_ptr<TreeNode> m_rootNode;

    // Internal helper to build tree nodes recursively
    void BuildTree(TreeNode* node, const std::string& currentPath);

    // Internal recursive search
    TreeNode* FindRecursive(TreeNode* node, const std::string& path) const;
};

} // namespace geany

#endif // GEANY_PROJECT_TREE_VIEW_H
