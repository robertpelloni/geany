#include "bobguifin.h"
G_DEFINE_TYPE (BobguiFinEngine, bobgui_fin_engine, G_TYPE_OBJECT)
static void bobgui_fin_engine_init (BobguiFinEngine *s) {}
static void bobgui_fin_engine_class_init (BobguiFinEngineClass *k) {}
BobguiFinEngine * bobgui_fin_engine_new (const char *u) { return g_object_new (BOBGUI_TYPE_FIN_ENGINE, NULL); }

G_DEFINE_TYPE (BobguiFinChart, bobgui_fin_chart, BOBGUI_TYPE_WIDGET)
static void bobgui_fin_chart_init (BobguiFinChart *s) {}
static void bobgui_fin_chart_class_init (BobguiFinChartClass *k) {}
BobguiFinChart * bobgui_fin_chart_new (void) { return g_object_new (BOBGUI_TYPE_FIN_CHART, NULL); }
