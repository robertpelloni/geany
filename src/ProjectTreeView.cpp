/**
 * ProjectTreeView.cpp
 *
 * Implementation of the C++ ProjectTreeView data model.
 */

#include "ProjectTreeView.h"
#include <iostream>

// Stub: For full directory parsing, we would use std::filesystem::directory_iterator
// from C++17. Here we stub it to allow testing the structural logic.
// #include <filesystem>

namespace geany {

// --- ProjectTreeView Implementation ---

ProjectTreeView::ProjectTreeView() : m_rootNode(nullptr) {}

ProjectTreeView::~ProjectTreeView() {
    Clear();
}

bool ProjectTreeView::SetRoot(const std::string& rootPath) {
    if (rootPath.empty()) return false;

    m_rootPath = rootPath;
    m_rootNode = std::make_unique<TreeNode>();
    m_rootNode->name = "ROOT";
    m_rootNode->fullPath = rootPath;
    m_rootNode->isDirectory = true;
    m_rootNode->isExpanded = true;

    std::cout << "[ProjectTreeView] Building project tree from root: " << rootPath << std::endl;
    BuildTree(m_rootNode.get(), rootPath);
    return true;
}

void ProjectTreeView::Refresh() {
    if (!m_rootPath.empty() && m_rootNode) {
        std::cout << "[ProjectTreeView] Refreshing project tree data..." << std::endl;
        m_rootNode->children.clear();
        BuildTree(m_rootNode.get(), m_rootPath);
    }
}

const TreeNode* ProjectTreeView::GetRootNode() const {
    return m_rootNode.get();
}

TreeNode* ProjectTreeView::FindNodeByPath(const std::string& path) const {
    if (!m_rootNode || path.empty()) return nullptr;
    if (m_rootNode->fullPath == path) return m_rootNode.get();

    return FindRecursive(m_rootNode.get(), path);
}

TreeNode* ProjectTreeView::FindRecursive(TreeNode* node, const std::string& path) const {
    for (auto& child : node->children) {
        if (child->fullPath == path) {
            return child.get();
        }
        if (child->isDirectory) {
            TreeNode* found = FindRecursive(child.get(), path);
            if (found) return found;
        }
    }
    return nullptr;
}

void ProjectTreeView::ToggleNode(TreeNode* node) {
    if (node && node->isDirectory) {
        node->isExpanded = !node->isExpanded;
        std::cout << "[ProjectTreeView] Node " << node->name << (node->isExpanded ? " expanded." : " collapsed.") << std::endl;
    }
}

void ProjectTreeView::Clear() {
    m_rootPath.clear();
    m_rootNode.reset();
}

void ProjectTreeView::BuildTree(TreeNode* node, const std::string& currentPath) {
    // Stub implementation.
    // In a real C++17 scenario:
    // for (const auto& entry : std::filesystem::directory_iterator(currentPath)) { ... }

    // Simulate creating some files and a subdirectory.
    if (node->name == "ROOT") {
        auto file1 = std::make_unique<TreeNode>();
        file1->name = "main.cpp";
        file1->fullPath = currentPath + "/main.cpp";
        file1->isDirectory = false;
        file1->parent = node;
        node->children.push_back(std::move(file1));

        auto dir1 = std::make_unique<TreeNode>();
        dir1->name = "src";
        dir1->fullPath = currentPath + "/src";
        dir1->isDirectory = true;
        dir1->isExpanded = false;
        dir1->parent = node;

        // Add a file inside the nested dir
        auto file2 = std::make_unique<TreeNode>();
        file2->name = "helper.h";
        file2->fullPath = dir1->fullPath + "/helper.h";
        file2->isDirectory = false;
        file2->parent = dir1.get();
        dir1->children.push_back(std::move(file2));

        node->children.push_back(std::move(dir1));
    }
}

} // namespace geany
