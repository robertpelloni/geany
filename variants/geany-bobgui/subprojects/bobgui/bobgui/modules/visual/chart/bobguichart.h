#ifndef BOBGUI_CHART_H
#define BOBGUI_CHART_H

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

typedef enum {
  BOBGUI_CHART_TYPE_LINE,
  BOBGUI_CHART_TYPE_BAR,
  BOBGUI_CHART_TYPE_PIE,
  BOBGUI_CHART_TYPE_SCATTER,
  BOBGUI_CHART_TYPE_RADAR
} BobguiChartType;

#define BOBGUI_TYPE_CHART_VIEW (bobgui_chart_view_get_type ())
G_DECLARE_FINAL_TYPE (BobguiChartView, bobgui_chart_view, BOBGUI, CHART_VIEW, BobguiWidget)

BobguiChartView * bobgui_chart_view_new (BobguiChartType type);

G_END_DECLS

#endif
