#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguitypes.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_LAYOUT_CHILD (bobgui_layout_child_get_type())

GDK_AVAILABLE_IN_ALL
G_DECLARE_DERIVABLE_TYPE (BobguiLayoutChild, bobgui_layout_child, BOBGUI, LAYOUT_CHILD, GObject)

struct _BobguiLayoutChildClass
{
  /*< private >*/
  GObjectClass parent_class;
};

GDK_AVAILABLE_IN_ALL
BobguiLayoutManager *      bobgui_layout_child_get_layout_manager     (BobguiLayoutChild *layout_child);
GDK_AVAILABLE_IN_ALL
BobguiWidget *             bobgui_layout_child_get_child_widget       (BobguiLayoutChild *layout_child);

G_END_DECLS
