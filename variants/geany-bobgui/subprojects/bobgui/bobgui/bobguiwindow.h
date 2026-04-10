/* BOBGUI - A modern native application and UI framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Maintained and evolved by the Bobgui contributors.  See the AUTHORS
 * file for contributor credits and repository history for implementation
 * details and ongoing refactor work.
 */

#pragma once


#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiapplication.h>
#include <bobgui/bobguiaccelgroup.h>
#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_WINDOW			(bobgui_window_get_type ())
#define BOBGUI_WINDOW(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_WINDOW, BobguiWindow))
#define BOBGUI_WINDOW_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_WINDOW, BobguiWindowClass))
#define BOBGUI_IS_WINDOW(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_WINDOW))
#define BOBGUI_IS_WINDOW_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_WINDOW))
#define BOBGUI_WINDOW_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_WINDOW, BobguiWindowClass))

typedef struct _BobguiWindowClass        BobguiWindowClass;
typedef struct _BobguiWindowGroup        BobguiWindowGroup;
typedef struct _BobguiWindowGroupClass   BobguiWindowGroupClass;
typedef struct _BobguiWindowGroupPrivate BobguiWindowGroupPrivate;

struct _BobguiWindow
{
  BobguiWidget parent_instance;
};

/**
 * BobguiWindowClass:
 * @parent_class: The parent class.
 * @activate_focus: Activates the current focused widget within the window.
 * @activate_default: Activates the default widget for the window.
 * @keys_changed: Signal gets emitted when the set of accelerators or
 *   mnemonics that are associated with window changes.
 * @enable_debugging: Class handler for the `BobguiWindow::enable-debugging`
 *   keybinding signal.
 */
struct _BobguiWindowClass
{
  BobguiWidgetClass parent_class;

  /*< public >*/

  /* G_SIGNAL_ACTION signals for keybindings */

  void     (* activate_focus)   (BobguiWindow *window);
  void     (* activate_default) (BobguiWindow *window);
  void	   (* keys_changed)     (BobguiWindow *window);
  gboolean (* enable_debugging) (BobguiWindow *window,
                                 gboolean   toggle);
  /**
   * BobguiWindowClass::close_request:
   *
   * Class handler for the [signal@Window::close-request] signal.
   *
   * Returns: Whether the window should be destroyed
   */
  gboolean (* close_request)    (BobguiWindow *window);

  /*< private >*/
  gpointer padding[8];
};

GDK_AVAILABLE_IN_ALL
GType      bobgui_window_get_type                 (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_window_new                      (void);
GDK_AVAILABLE_IN_ALL
void       bobgui_window_set_title                (BobguiWindow           *window,
						const char          *title);
GDK_AVAILABLE_IN_ALL
const char * bobgui_window_get_title             (BobguiWindow           *window);
GDK_AVAILABLE_IN_ALL
void       bobgui_window_set_startup_id           (BobguiWindow           *window,
                                                const char          *startup_id);
GDK_AVAILABLE_IN_ALL
void       bobgui_window_set_focus                (BobguiWindow           *window,
						BobguiWidget           *focus);
GDK_AVAILABLE_IN_ALL
BobguiWidget *bobgui_window_get_focus                (BobguiWindow           *window);
GDK_AVAILABLE_IN_ALL
void       bobgui_window_set_default_widget       (BobguiWindow           *window,
						BobguiWidget           *default_widget);
GDK_AVAILABLE_IN_ALL
BobguiWidget *bobgui_window_get_default_widget       (BobguiWindow           *window);

GDK_AVAILABLE_IN_ALL
void       bobgui_window_set_transient_for        (BobguiWindow           *window,
						BobguiWindow           *parent);
GDK_AVAILABLE_IN_ALL
BobguiWindow *bobgui_window_get_transient_for        (BobguiWindow           *window);
GDK_AVAILABLE_IN_ALL
void       bobgui_window_set_destroy_with_parent  (BobguiWindow           *window,
                                                gboolean             setting);
GDK_AVAILABLE_IN_ALL
gboolean   bobgui_window_get_destroy_with_parent  (BobguiWindow           *window);
GDK_AVAILABLE_IN_ALL
void       bobgui_window_set_hide_on_close        (BobguiWindow           *window,
                                                gboolean             setting);
GDK_AVAILABLE_IN_ALL
gboolean   bobgui_window_get_hide_on_close        (BobguiWindow           *window);
GDK_AVAILABLE_IN_ALL
void       bobgui_window_set_mnemonics_visible    (BobguiWindow           *window,
                                                gboolean             setting);
GDK_AVAILABLE_IN_ALL
gboolean   bobgui_window_get_mnemonics_visible    (BobguiWindow           *window);
GDK_AVAILABLE_IN_ALL
void       bobgui_window_set_focus_visible        (BobguiWindow           *window,
                                                gboolean             setting);
GDK_AVAILABLE_IN_ALL
gboolean   bobgui_window_get_focus_visible        (BobguiWindow           *window);

GDK_AVAILABLE_IN_ALL
void       bobgui_window_set_resizable            (BobguiWindow           *window,
                                                gboolean             resizable);
GDK_AVAILABLE_IN_ALL
gboolean   bobgui_window_get_resizable            (BobguiWindow           *window);

GDK_AVAILABLE_IN_ALL
void	   bobgui_window_set_display              (BobguiWindow	    *window,
						GdkDisplay          *display);

GDK_AVAILABLE_IN_ALL
gboolean   bobgui_window_is_active                (BobguiWindow           *window);

GDK_AVAILABLE_IN_ALL
void       bobgui_window_set_decorated            (BobguiWindow *window,
                                                gboolean   setting);
GDK_AVAILABLE_IN_ALL
gboolean   bobgui_window_get_decorated            (BobguiWindow *window);
GDK_AVAILABLE_IN_ALL
void       bobgui_window_set_deletable            (BobguiWindow *window,
                                                gboolean   setting);
GDK_AVAILABLE_IN_ALL
gboolean   bobgui_window_get_deletable            (BobguiWindow *window);

GDK_AVAILABLE_IN_ALL
void       bobgui_window_set_icon_name                (BobguiWindow   *window,
						    const char *name);
GDK_AVAILABLE_IN_ALL
const char * bobgui_window_get_icon_name             (BobguiWindow  *window);
GDK_AVAILABLE_IN_ALL
void       bobgui_window_set_default_icon_name        (const char *name);
GDK_AVAILABLE_IN_ALL
const char * bobgui_window_get_default_icon_name     (void);

GDK_AVAILABLE_IN_ALL
void       bobgui_window_set_auto_startup_notification (gboolean setting);

/* If window is set modal, input will be grabbed when show and released when hide */
GDK_AVAILABLE_IN_ALL
void       bobgui_window_set_modal      (BobguiWindow *window,
				      gboolean   modal);
GDK_AVAILABLE_IN_ALL
gboolean   bobgui_window_get_modal      (BobguiWindow *window);
GDK_AVAILABLE_IN_ALL
GListModel *bobgui_window_get_toplevels (void);
GDK_AVAILABLE_IN_ALL
GList*     bobgui_window_list_toplevels (void);

GDK_AVAILABLE_IN_ALL
void     bobgui_window_present            (BobguiWindow *window);
GDK_DEPRECATED_IN_4_14_FOR(bobgui_window_present)
void     bobgui_window_present_with_time  (BobguiWindow *window,
				        guint32    timestamp);
GDK_AVAILABLE_IN_ALL
void     bobgui_window_minimize      (BobguiWindow *window);
GDK_AVAILABLE_IN_ALL
void     bobgui_window_unminimize    (BobguiWindow *window);
GDK_AVAILABLE_IN_ALL
void     bobgui_window_maximize      (BobguiWindow *window);
GDK_AVAILABLE_IN_ALL
void     bobgui_window_unmaximize    (BobguiWindow *window);
GDK_AVAILABLE_IN_ALL
void     bobgui_window_fullscreen    (BobguiWindow *window);
GDK_AVAILABLE_IN_ALL
void     bobgui_window_unfullscreen  (BobguiWindow *window);
GDK_AVAILABLE_IN_ALL
void     bobgui_window_fullscreen_on_monitor (BobguiWindow  *window,
                                           GdkMonitor *monitor);
GDK_AVAILABLE_IN_ALL
void     bobgui_window_close         (BobguiWindow *window);

/* Set initial default size of the window (does not constrain user
 * resize operations)
 */
GDK_AVAILABLE_IN_ALL
void     bobgui_window_set_default_size (BobguiWindow   *window,
                                      int          width,
                                      int          height);
GDK_AVAILABLE_IN_ALL
void     bobgui_window_get_default_size (BobguiWindow   *window,
                                      int         *width,
                                      int         *height);

GDK_AVAILABLE_IN_ALL
BobguiWindowGroup *bobgui_window_get_group (BobguiWindow   *window);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_window_has_group        (BobguiWindow   *window);


GDK_AVAILABLE_IN_ALL
BobguiApplication *bobgui_window_get_application      (BobguiWindow          *window);
GDK_AVAILABLE_IN_ALL
void            bobgui_window_set_application      (BobguiWindow          *window,
                                                 BobguiApplication     *application);

GDK_AVAILABLE_IN_ALL
void     bobgui_window_set_child              (BobguiWindow    *window,
                                            BobguiWidget    *child);
GDK_AVAILABLE_IN_ALL
BobguiWidget *bobgui_window_get_child            (BobguiWindow    *window);

GDK_AVAILABLE_IN_ALL
void     bobgui_window_set_titlebar           (BobguiWindow    *window,
                                            BobguiWidget    *titlebar);
GDK_AVAILABLE_IN_ALL
BobguiWidget *bobgui_window_get_titlebar         (BobguiWindow    *window);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_window_is_maximized           (BobguiWindow    *window);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_window_is_fullscreen          (BobguiWindow    *window);

GDK_AVAILABLE_IN_4_12
gboolean bobgui_window_is_suspended           (BobguiWindow    *window);

GDK_AVAILABLE_IN_ALL
void     bobgui_window_destroy                (BobguiWindow    *window);

GDK_AVAILABLE_IN_ALL
void     bobgui_window_set_interactive_debugging (gboolean enable);

GDK_AVAILABLE_IN_4_2
void     bobgui_window_set_handle_menubar_accel (BobguiWindow *window,
                                              gboolean   handle_menubar_accel);
GDK_AVAILABLE_IN_4_2
gboolean bobgui_window_get_handle_menubar_accel (BobguiWindow *window);

/**
 * BobguiWindowGravity:
 * @BOBGUI_WINDOW_GRAVITY_TOP_LEFT: The top left corner
 * @BOBGUI_WINDOW_GRAVITY_TOP: The top edge
 * @BOBGUI_WINDOW_GRAVITY_TOP_RIGHT: The top right corner
 * @BOBGUI_WINDOW_GRAVITY_LEFT: The left edge
 * @BOBGUI_WINDOW_GRAVITY_CENTER: The center pointer
 * @BOBGUI_WINDOW_GRAVITY_RIGHT: The right edge
 * @BOBGUI_WINDOW_GRAVITY_BOTTOM_LEFT: The bottom left corner
 * @BOBGUI_WINDOW_GRAVITY_BOTTOM: the bottom edge
 * @BOBGUI_WINDOW_GRAVITY_BOTTOM_RIGHT: The bottom right corner
 * @BOBGUI_WINDOW_GRAVITY_TOP_START: The top left or top right corner,
 *   depending on the text direction
 * @BOBGUI_WINDOW_GRAVITY_TOP_END: The top right or top left corner,
 *   depending on the text direction
 * @BOBGUI_WINDOW_GRAVITY_START: The left or right edge,
 *   depending on the text direction
 * @BOBGUI_WINDOW_GRAVITY_END: The right or left edge,
 *   depending on the text direction
 * @BOBGUI_WINDOW_GRAVITY_BOTTOM_START: The bottom left or top right corner,
 *   depending on the text direction
 * @BOBGUI_WINDOW_GRAVITY_BOTTOM_END: The bottom right or top left corner,
 *   depending on the text direction
 *
 * Determines which point or edge of a window is meant to remain fixed
 * when a window changes size.
 *
 * Since: 4.20
 */
typedef enum
{
  BOBGUI_WINDOW_GRAVITY_TOP_LEFT,
  BOBGUI_WINDOW_GRAVITY_TOP,
  BOBGUI_WINDOW_GRAVITY_TOP_RIGHT,
  BOBGUI_WINDOW_GRAVITY_LEFT,
  BOBGUI_WINDOW_GRAVITY_CENTER,
  BOBGUI_WINDOW_GRAVITY_RIGHT,
  BOBGUI_WINDOW_GRAVITY_BOTTOM_LEFT,
  BOBGUI_WINDOW_GRAVITY_BOTTOM,
  BOBGUI_WINDOW_GRAVITY_BOTTOM_RIGHT,
  BOBGUI_WINDOW_GRAVITY_TOP_START,
  BOBGUI_WINDOW_GRAVITY_TOP_END,
  BOBGUI_WINDOW_GRAVITY_START,
  BOBGUI_WINDOW_GRAVITY_END,
  BOBGUI_WINDOW_GRAVITY_BOTTOM_START,
  BOBGUI_WINDOW_GRAVITY_BOTTOM_END,
} BobguiWindowGravity;

GDK_AVAILABLE_IN_4_20
BobguiWindowGravity
         bobgui_window_get_gravity              (BobguiWindow        *window);
GDK_AVAILABLE_IN_4_20
void     bobgui_window_set_gravity              (BobguiWindow        *window,
                                              BobguiWindowGravity  gravity);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiWindow, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiWindowGroup, g_object_unref)

G_END_DECLS

