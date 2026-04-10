/* bobgui/modules/report/bobguireport.h */
#ifndef BOBGUI_REPORT_H
#define BOBGUI_REPORT_H

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

/* Professional Reporting Engine (Better than Qt Print/PDF) */
#define BOBGUI_TYPE_REPORT_DESIGNER (bobgui_report_designer_get_type ())
G_DECLARE_FINAL_TYPE (BobguiReportDesigner, bobgui_report_designer, BOBGUI, REPORT_DESIGNER, GObject)

BobguiReportDesigner * bobgui_report_designer_new (void);

/* Designer API (Uses BobguiDSL for layout) */
void bobgui_report_designer_set_template (BobguiReportDesigner *self, BobguiWidget *template_root);
void bobgui_report_designer_set_data     (BobguiReportDesigner *self, GList *data_records);

/* High-Performance Export (Hardware Accelerated Rendering) */
void bobgui_report_export_pdf (BobguiReportDesigner *self, const char *path, GAsyncReadyCallback callback);
void bobgui_report_export_svg (BobguiReportDesigner *self, const char *path);
void bobgui_report_print      (BobguiReportDesigner *self, BobguiPrintOperation *op);

/* Preview Widget (Integrated with BobguiScene) */
#define BOBGUI_TYPE_REPORT_PREVIEW (bobgui_report_preview_get_type ())
G_DECLARE_FINAL_TYPE (BobguiReportPreview, bobgui_report_preview, BOBGUI, REPORT_PREVIEW, BobguiWidget)

BobguiReportPreview * bobgui_report_preview_new (BobguiReportDesigner *designer);

G_END_DECLS

#endif /* BOBGUI_REPORT_H */
