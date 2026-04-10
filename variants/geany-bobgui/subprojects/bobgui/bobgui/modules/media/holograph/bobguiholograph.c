#include "bobguiholograph.h"
G_DEFINE_TYPE (BobguiHologram, bobgui_hologram, G_TYPE_OBJECT)
static void bobgui_hologram_init (BobguiHologram *s) {}
static void bobgui_hologram_class_init (BobguiHologramClass *k) {}
BobguiHologram * bobgui_hologram_new_from_voxels (gpointer d, int w, int h, int dep) { return g_object_new (BOBGUI_TYPE_HOLOGRAM, NULL); }

G_DEFINE_TYPE (BobguiHolographView, bobgui_holograph_view, BOBGUI_TYPE_WIDGET)
static void bobgui_holograph_view_init (BobguiHolographView *s) {}
static void bobgui_holograph_view_class_init (BobguiHolographViewClass *k) {}
BobguiHolographView * bobgui_holograph_view_new (void) { return g_object_new (BOBGUI_TYPE_HOLOGRAPH_VIEW, NULL); }
