/**
 * DashboardHelper.h
 *
 * Provides C++ level macros, documentation structs, and tooltips mapping engine features
 * specifically required by the Bobgui Dashboard UI interface.
 */
#ifndef GEANY_DASHBOARD_HELPER_H
#define GEANY_DASHBOARD_HELPER_H

#include <string>
#include <map>

namespace geany {

struct DashboardFeatureMetric {
    std::string internalName;
    std::string uiLabel;
    std::string tooltipDesc;
    bool isVisible;
};

class DashboardHelper {
public:
    DashboardHelper();

    // Retrieves a fully documented struct intended for the Dashboard UI rendering loop
    DashboardFeatureMetric GetFeatureMetadata(const std::string& featureKey) const;

private:
    std::map<std::string, DashboardFeatureMetric> m_featureRegistry;

    void InitializeTooltips();
};

}

#endif // GEANY_DASHBOARD_HELPER_H
