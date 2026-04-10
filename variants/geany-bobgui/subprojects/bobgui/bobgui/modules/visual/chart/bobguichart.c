#include "bobguichart.h"
G_DEFINE_TYPE (BobguiChartView, bobgui_chart_view, BOBGUI_TYPE_WIDGET)
static void bobgui_chart_view_init (BobguiChartView *s) {}
static void bobgui_chart_view_class_init (BobguiChartViewClass *k) {}
BobguiChartView * bobgui_chart_view_new (BobguiChartType t) { return g_object_new (BOBGUI_TYPE_CHART_VIEW, NULL); }
