#include "DashboardHelper.h"

namespace geany {

DashboardHelper::DashboardHelper() {
    InitializeTooltips();
}

void DashboardHelper::InitializeTooltips() {
    // ---------------------------------------------------------
    // Phase 2: Bobgui UI Overhaul & Dashboard - Tooltip Mapping
    // ---------------------------------------------------------

    // Line Operations
    m_featureRegistry["Edit.Line.Sort"] = {
        "Edit.Line.Sort",
        "Sort Lines Lexicographically",
        "Sorts the currently selected lines (or the entire document) in alphabetical order.",
        true
    };

    m_featureRegistry["Edit.Line.Reverse"] = {
        "Edit.Line.Reverse",
        "Reverse Line Order",
        "Flips the order of the selected lines. The bottom line becomes the top line.",
        true
    };

    m_featureRegistry["Edit.Line.Duplicate"] = {
        "Edit.Line.Duplicate",
        "Duplicate Current Line",
        "Copies the line where the cursor is currently positioned and inserts it immediately below.",
        true
    };

    m_featureRegistry["Edit.Line.Delete"] = {
        "Edit.Line.Delete",
        "Delete Current Line",
        "Removes the entire line at the cursor's current position.",
        true
    };

    // Blank Operations
    m_featureRegistry["Edit.Blank.Trim"] = {
        "Edit.Blank.Trim",
        "Trim Trailing Space",
        "Removes any invisible whitespace or tabs at the very end of every line.",
        true
    };

    m_featureRegistry["Edit.Blank.EOLToSpace"] = {
        "Edit.Blank.EOLToSpace",
        "EOL to Space",
        "Removes all line breaks and replaces them with a single space, merging the document into one block.",
        true
    };

    // Dashboard Monolithic Operations (Phase 2 UI Redesign)
    m_featureRegistry["Dashboard.Monolithic"] = {
        "Dashboard.Monolithic",
        "Geany Ultra Main Command Center",
        "A singular, unified dashboard consolidating Editor Metrics, UI Preferences, Telemetry, and Configuration overrides into one prominent, highly-accessible view.",
        true
    };

    m_featureRegistry["Dashboard.Config"] = {
        "Dashboard.Config",
        "Geany Configuration Metrics",
        "Provides a real-time monolithic overview of all application preferences and settings. (Merged from Preferences subpage)",
        false // Now embedded in Monolithic
    };

    m_featureRegistry["Dashboard.Stats"] = {
        "Dashboard.Stats",
        "Application Telemetry",
        "Displays real-time file I/O operations, text editing metrics, and line manipulations. (Merged from Statistics subpage)",
        false // Now embedded in Monolithic
    };

    // N++ Parity - Compare Plugin & Hex View
    m_featureRegistry["Plugin.Compare"] = {
        "Plugin.Compare",
        "AST Diff / Side-by-Side Compare",
        "Launch the native Geany-Go diff engine to inspect side-by-side deviations of the current document.",
        true
    };
}

DashboardFeatureMetric DashboardHelper::GetFeatureMetadata(const std::string& featureKey) const {
    auto it = m_featureRegistry.find(featureKey);
    if (it != m_featureRegistry.end()) {
        return it->second;
    }
    return {"Unknown", "Unknown Feature", "No tooltip available for this feature.", false};
}

}
