#ifndef BOBGUI_DOCK_H
#define BOBGUI_DOCK_H

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_DOCK_MANAGER (bobgui_dock_manager_get_type ())
G_DECLARE_FINAL_TYPE (BobguiDockManager, bobgui_dock_manager, BOBGUI, DOCK_MANAGER, GObject)

BobguiDockManager * bobgui_dock_manager_new (BobguiWindow *main_window);

/* Professional Docking (Qt6/JUCE Parity) */
typedef enum {
  BOBGUI_DOCK_LEFT,
  BOBGUI_DOCK_RIGHT,
  BOBGUI_DOCK_TOP,
  BOBGUI_DOCK_BOTTOM,
  BOBGUI_DOCK_FLOATING
} BobguiDockPosition;

void bobgui_dock_manager_add_widget (BobguiDockManager *self, 
                                     BobguiWidget      *widget, 
                                     BobguiDockPosition position, 
                                     const char        *title);

void bobgui_dock_manager_save_layout (BobguiDockManager *self, const char *path);
void bobgui_dock_manager_load_layout (BobguiDockManager *self, const char *path);

G_END_DECLS

#endif
