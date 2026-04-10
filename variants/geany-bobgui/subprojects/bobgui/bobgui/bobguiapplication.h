/* BOBGUI - A modern native application and UI framework
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

#include <bobgui/bobguiwidget.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_APPLICATION            (bobgui_application_get_type ())
#define BOBGUI_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_APPLICATION, BobguiApplication))
#define BOBGUI_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_APPLICATION, BobguiApplicationClass))
#define BOBGUI_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_APPLICATION))
#define BOBGUI_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_APPLICATION))
#define BOBGUI_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_APPLICATION, BobguiApplicationClass))

typedef struct _BobguiApplication        BobguiApplication;
typedef struct _BobguiApplicationClass   BobguiApplicationClass;

struct _BobguiApplication
{
  GApplication parent_instance;
};

/**
 * BobguiApplicationClass:
 * @parent_class: The parent class.
 * @window_added: Signal emitted when a `BobguiWindow` is added to
 *    application through bobgui_application_add_window().
 * @window_removed: Signal emitted when a `BobguiWindow` is removed from
 *    application, either as a side-effect of being destroyed or
 *    explicitly through bobgui_application_remove_window().
 */
struct _BobguiApplicationClass
{
  GApplicationClass parent_class;

  /*< public >*/

  void (*window_added)   (BobguiApplication *application,
                          BobguiWindow      *window);
  void (*window_removed) (BobguiApplication *application,
                          BobguiWindow      *window);

  /**
   * BobguiApplicationClass::save_state:
   * @state: a dictionary where to store the application's state
   *
   * Class closure for the [signal@Application::save-state] signal.
   *
   * Returns: true to stop stop further handlers from running
   *
   * Since: 4.24
   */
  gboolean (* save_state)    (BobguiApplication   *application,
                              GVariantDict     *state);

  /**
   * BobguiApplicationClass::restore_state:
   * @reason: the reason for restoring state
   * @state: a dictionary containing the application state to restore
   *
   * Class closure for the [signal@Application::restore-state] signal.
   *
   * Returns: true to stop stop further handlers from running
   *
   * Since: 4.24
   */
  gboolean (* restore_state) (BobguiApplication   *application,
                              BobguiRestoreReason  reason,
                              GVariant         *state);

  /**
   * BobguiApplicationClass::restore_window:
   * @reason: the reason this window is restored
   * @state: (nullable): the state to restore, as saved by a
   *   [signal@Bobgui.ApplicationWindow::save-state] handler
   *
   * Class closure for the [signal@Application::restore-window] signal.
   *
   * Since: 4.24
   */
  void     (*restore_window) (BobguiApplication   *application,
                              BobguiRestoreReason  reason,
                              GVariant         *state);

  /*< private >*/
  gpointer padding[5];
};

GDK_AVAILABLE_IN_ALL
GType            bobgui_application_get_type      (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiApplication * bobgui_application_new           (const char        *application_id,
                                                GApplicationFlags  flags);

GDK_AVAILABLE_IN_ALL
void             bobgui_application_add_window    (BobguiApplication    *application,
                                                BobguiWindow         *window);

GDK_AVAILABLE_IN_ALL
void             bobgui_application_remove_window (BobguiApplication    *application,
                                                BobguiWindow         *window);
GDK_AVAILABLE_IN_ALL
GList *          bobgui_application_get_windows   (BobguiApplication    *application);

GDK_AVAILABLE_IN_ALL
GMenuModel *     bobgui_application_get_menubar   (BobguiApplication    *application);
GDK_AVAILABLE_IN_ALL
void             bobgui_application_set_menubar   (BobguiApplication    *application,
                                                GMenuModel        *menubar);

typedef enum
{
  BOBGUI_APPLICATION_INHIBIT_LOGOUT  = (1 << 0),
  BOBGUI_APPLICATION_INHIBIT_SWITCH  = (1 << 1),
  BOBGUI_APPLICATION_INHIBIT_SUSPEND = (1 << 2),
  BOBGUI_APPLICATION_INHIBIT_IDLE    = (1 << 3)
} BobguiApplicationInhibitFlags;

GDK_AVAILABLE_IN_ALL
guint            bobgui_application_inhibit            (BobguiApplication             *application,
                                                     BobguiWindow                  *window,
                                                     BobguiApplicationInhibitFlags  flags,
                                                     const char                 *reason);
GDK_AVAILABLE_IN_ALL
void             bobgui_application_uninhibit          (BobguiApplication             *application,
                                                     guint                       cookie);

GDK_AVAILABLE_IN_ALL
BobguiWindow *      bobgui_application_get_window_by_id   (BobguiApplication             *application,
                                                     guint                       id);

GDK_AVAILABLE_IN_ALL
BobguiWindow *      bobgui_application_get_active_window  (BobguiApplication             *application);

GDK_AVAILABLE_IN_ALL
char **         bobgui_application_list_action_descriptions        (BobguiApplication       *application);

GDK_AVAILABLE_IN_ALL
char **         bobgui_application_get_accels_for_action           (BobguiApplication       *application,
                                                                  const char           *detailed_action_name);
GDK_AVAILABLE_IN_ALL
char **         bobgui_application_get_actions_for_accel           (BobguiApplication       *application,
                                                                  const char           *accel);


GDK_AVAILABLE_IN_ALL
void             bobgui_application_set_accels_for_action           (BobguiApplication       *application,
                                                                  const char           *detailed_action_name,
                                                                  const char * const  *accels);

GDK_AVAILABLE_IN_ALL
GMenu *          bobgui_application_get_menu_by_id                  (BobguiApplication       *application,
                                                                  const char           *id);

GDK_AVAILABLE_IN_4_24
void             bobgui_application_save                            (BobguiApplication       *application);

GDK_AVAILABLE_IN_4_24
void             bobgui_application_forget                          (BobguiApplication       *application);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiApplication, g_object_unref)

G_END_DECLS

