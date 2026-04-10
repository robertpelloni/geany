#include "bobguiweb.h"
G_DEFINE_TYPE (BobguiWebView, bobgui_web_view, BOBGUI_TYPE_WIDGET)
static void bobgui_web_view_init (BobguiWebView *s) {}
static void bobgui_web_view_class_init (BobguiWebViewClass *k) {}
BobguiWebView * bobgui_web_view_new (void) { return g_object_new (BOBGUI_TYPE_WEB_VIEW, NULL); }
