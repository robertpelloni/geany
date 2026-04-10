#include "bobguireport.h"
G_DEFINE_TYPE (BobguiReportDesigner, bobgui_report_designer, G_TYPE_OBJECT)
static void bobgui_report_designer_init (BobguiReportDesigner *self) {}
static void bobgui_report_designer_class_init (BobguiReportDesignerClass *klass) {}
BobguiReportDesigner * bobgui_report_designer_new (void) { return g_object_new (BOBGUI_TYPE_REPORT_DESIGNER, NULL); }

G_DEFINE_TYPE (BobguiReportPreview, bobgui_report_preview, BOBGUI_TYPE_WIDGET)
static void bobgui_report_preview_init (BobguiReportPreview *self) {}
static void bobgui_report_preview_class_init (BobguiReportPreviewClass *klass) {}
BobguiReportPreview * bobgui_report_preview_new (BobguiReportDesigner *d) { return g_object_new (BOBGUI_TYPE_REPORT_PREVIEW, NULL); }
