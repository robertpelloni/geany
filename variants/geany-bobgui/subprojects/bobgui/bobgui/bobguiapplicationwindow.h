/*
 * Copyright © 2011 Canonical Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwindow.h>
#include <bobgui/deprecated/bobguishortcutswindow.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_APPLICATION_WINDOW            (bobgui_application_window_get_type ())
#define BOBGUI_APPLICATION_WINDOW(inst)           (G_TYPE_CHECK_INSTANCE_CAST ((inst), \
                                                BOBGUI_TYPE_APPLICATION_WINDOW, BobguiApplicationWindow))
#define BOBGUI_APPLICATION_WINDOW_CLASS(class)    (G_TYPE_CHECK_CLASS_CAST ((class),   \
                                                BOBGUI_TYPE_APPLICATION_WINDOW, BobguiApplicationWindowClass))
#define BOBGUI_IS_APPLICATION_WINDOW(inst)        (G_TYPE_CHECK_INSTANCE_TYPE ((inst), \
                                                BOBGUI_TYPE_APPLICATION_WINDOW))
#define BOBGUI_IS_APPLICATION_WINDOW_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE ((class),   \
                                                BOBGUI_TYPE_APPLICATION_WINDOW))
#define BOBGUI_APPLICATION_WINDOW_GET_CLASS(inst) (G_TYPE_INSTANCE_GET_CLASS ((inst),  \
                                                BOBGUI_TYPE_APPLICATION_WINDOW, BobguiApplicationWindowClass))

typedef struct _BobguiApplicationWindowClass   BobguiApplicationWindowClass;
typedef struct _BobguiApplicationWindow        BobguiApplicationWindow;

struct _BobguiApplicationWindow
{
  BobguiWindow parent_instance;
};

/**
 * BobguiApplicationWindowClass:
 * @parent_class: The parent class.
 */
struct _BobguiApplicationWindowClass
{
  BobguiWindowClass parent_class;

  /**
   * BobguiApplicationWindowClass::save_state:
   * @dict: a dictionary where to store the window's state
   *
   * Class closure for the [signal@ApplicationWindow::save-state] signal.
   *
   * Returns: true to stop stop further handlers from running
   *
   * Since: 4.24
   */
  gboolean     (* save_state)           (BobguiApplicationWindow   *window,
                                         GVariantDict           *dict);

  /*< private >*/
  gpointer padding[7];
};

GDK_AVAILABLE_IN_ALL
GType       bobgui_application_window_get_type          (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget * bobgui_application_window_new               (BobguiApplication      *application);

GDK_AVAILABLE_IN_ALL
void        bobgui_application_window_set_show_menubar (BobguiApplicationWindow *window,
                                                     gboolean              show_menubar);
GDK_AVAILABLE_IN_ALL
gboolean    bobgui_application_window_get_show_menubar (BobguiApplicationWindow *window);

GDK_AVAILABLE_IN_ALL
guint       bobgui_application_window_get_id           (BobguiApplicationWindow *window);

GDK_DEPRECATED_IN_4_18
void        bobgui_application_window_set_help_overlay (BobguiApplicationWindow *window,
                                                     BobguiShortcutsWindow   *help_overlay);
GDK_DEPRECATED_IN_4_18
BobguiShortcutsWindow *
            bobgui_application_window_get_help_overlay (BobguiApplicationWindow *window);


G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiApplicationWindow, g_object_unref)

G_END_DECLS

