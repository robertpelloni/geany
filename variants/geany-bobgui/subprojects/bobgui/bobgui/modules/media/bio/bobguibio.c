#include "bobguibio.h"
G_DEFINE_TYPE (BobguiBioManager, bobgui_bio_manager, G_TYPE_OBJECT)
static void bobgui_bio_manager_init (BobguiBioManager *s) {}
static void bobgui_bio_manager_class_init (BobguiBioManagerClass *k) {}
BobguiBioManager * bobgui_bio_manager_get_default (void) {
    static BobguiBioManager *m = NULL;
    if (!m) m = g_object_new (BOBGUI_TYPE_BIO_MANAGER, NULL);
    return m;
}

G_DEFINE_TYPE (BobguiBioViewer, bobgui_bio_viewer, BOBGUI_TYPE_WIDGET)
static void bobgui_bio_viewer_init (BobguiBioViewer *s) {}
static void bobgui_bio_viewer_class_init (BobguiBioViewerClass *k) {}
BobguiBioViewer * bobgui_bio_viewer_new (void) { return g_object_new (BOBGUI_TYPE_BIO_VIEWER, NULL); }
