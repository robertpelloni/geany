/* BOBGUI - The Bobgui Framework
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
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#include "config.h"

#include "bobguiwindowprivate.h"

#include "bobguiaccessibleprivate.h"
#include "bobguiapplicationprivate.h"
#include "bobguibox.h"
#include "bobguibuildable.h"
#include "bobguibuilderprivate.h"
#include "bobguicheckbutton.h"
#include "bobguicssshadowvalueprivate.h"
#include "bobguidroptargetasync.h"
#include "bobguieventcontrollerlegacy.h"
#include "bobguieventcontrollerkey.h"
#include "bobguieventcontrollermotion.h"
#include "bobguigestureclick.h"
#include "bobguiheaderbar.h"
#include "bobguiicontheme.h"
#include <glib/gi18n-lib.h>
#include "bobguimain.h"
#include "bobguimarshalers.h"
#include "deprecated/bobguimessagedialog.h"
#include "bobguipointerfocusprivate.h"
#include "bobguiprivate.h"
#include "bobguiroot.h"
#include "bobguinativeprivate.h"
#include "bobguisettings.h"
#include "bobguishortcut.h"
#include "bobguishortcutcontrollerprivate.h"
#include "bobguishortcutmanager.h"
#include "bobguishortcuttrigger.h"
#include "bobguisizerequest.h"
#include "bobguisnapshot.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"
#include "bobguiwindowgroup.h"
#include "bobguipopovermenubarprivate.h"
#include "bobguicssboxesimplprivate.h"
#include "bobguitooltipprivate.h"
#include "bobguimenubutton.h"

#include "inspector/window.h"

#include "gdk/gdkprofilerprivate.h"
#include "gdk/gdksurfaceprivate.h"
#include "gdk/gdktextureprivate.h"
#include "gdk/gdktoplevelprivate.h"

#include "gsk/gskrendererprivate.h"
#include "gsk/gskroundedrectprivate.h"

#include <cairo-gobject.h>
#include <errno.h>
#include <graphene.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#ifdef GDK_WINDOWING_X11
#include "x11/gdkx.h"
#endif

#ifdef GDK_WINDOWING_WIN32
#include "win32/gdkwin32.h"
#endif

#ifdef GDK_WINDOWING_WAYLAND
#include "wayland/gdkwayland.h"
#include "wayland/gdkdisplay-wayland.h"
#endif

#ifdef GDK_WINDOWING_MACOS
#include "macos/gdkmacos.h"
#endif

#ifdef GDK_WINDOWING_BROADWAY
#include "broadway/gdkbroadway.h"
#endif

#define GDK_ARRAY_ELEMENT_TYPE BobguiWidget *
#define GDK_ARRAY_TYPE_NAME BobguiWidgetStack
#define GDK_ARRAY_NAME bobgui_widget_stack
#define GDK_ARRAY_FREE_FUNC g_object_unref
#define GDK_ARRAY_PREALLOC 16
#include "gdk/gdkarrayimpl.c"

/**
 * BobguiWindow:
 *
 * A toplevel window which can contain other widgets.
 *
 * <picture>
 *   <source srcset="window-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiWindow" src="window.png">
 * </picture>
 *
 * Windows normally have decorations that are under the control
 * of the windowing system and allow the user to manipulate the window
 * (resize it, move it, close it,...).
 *
 * # BobguiWindow as BobguiBuildable
 *
 * The `BobguiWindow` implementation of the [iface@Bobgui.Buildable] interface supports
 * setting a child as the titlebar by specifying “titlebar” as the “type”
 * attribute of a `<child>` element.
 *
 * # Shortcuts and Gestures
 *
 * `BobguiWindow` supports the following keyboard shortcuts:
 *
 * - <kbd>F10</kbd> activates the menubar, if present.
 * - <kbd>Alt</kbd> makes the mnemonics visible while pressed.
 *
 * The following signals have default keybindings:
 *
 * - [signal@Bobgui.Window::activate-default]
 * - [signal@Bobgui.Window::activate-focus]
 * - [signal@Bobgui.Window::enable-debugging]
 *
 * # Actions
 *
 * `BobguiWindow` defines a set of built-in actions:
 *
 * - `default.activate` activates the default widget.
 * - `window.minimize` minimizes the window.
 * - `window.toggle-maximized` maximizes or restores the window.
 * - `window.close` closes the window.
 *
 * # CSS nodes
 *
 * ```
 * window.background [.csd / .solid-csd / .ssd] [.maximized / .fullscreen / .tiled]
 * ├── <child>
 * ╰── <titlebar child>.titlebar [.default-decoration]
 * ```
 *
 * `BobguiWindow` has a main CSS node with name window and style class .background.
 *
 * Style classes that are typically used with the main CSS node are .csd (when
 * client-side decorations are in use), .solid-csd (for client-side decorations
 * without invisible borders), .ssd (used by mutter when rendering server-side
 * decorations). BobguiWindow also represents window states with the following
 * style classes on the main node: .maximized, .fullscreen, .tiled (when supported,
 * also .tiled-top, .tiled-left, .tiled-right, .tiled-bottom).
 *
 * `BobguiWindow` subclasses often add their own discriminating style classes,
 * such as .dialog, .popup or .tooltip.
 *
 * Generally, some CSS properties don't make sense on the toplevel window node,
 * such as margins or padding. When client-side decorations without invisible
 * borders are in use (i.e. the .solid-csd style class is added to the
 * main window node), the CSS border of the toplevel window is used for
 * resize drags. In the .csd case, the shadow area outside of the window
 * can be used to resize it.
 *
 * `BobguiWindow` adds the .titlebar and .default-decoration style classes to the
 * widget that is added as a titlebar child.
 *
 * # Accessibility
 *
 * `BobguiWindow` uses the [enum@Bobgui.AccessibleRole.window] role.
 *
 * From BOBGUI 4.12 to 4.18, it used the [enum@Bobgui.AccessibleRole.application] role.
 */

#define MENU_BAR_ACCEL GDK_KEY_F10
#define RESIZE_HANDLE_SIZE 12 /* Width of resize borders */
#define RESIZE_HANDLE_CORNER_SIZE 24 /* How resize corners extend */
#define MNEMONICS_DELAY 300 /* ms */
#define NO_CONTENT_CHILD_NAT 200 /* ms */
#define VISIBLE_FOCUS_DURATION 3 /* s */


/* In case the content (excluding header bar and shadows) of the window
 * would be empty, either because there is no visible child widget or only an
 * empty container widget, we use NO_CONTENT_CHILD_NAT as natural width/height
 * instead.
 */

typedef struct _BobguiWindowGeometryInfo BobguiWindowGeometryInfo;

typedef struct
{
  BobguiWidget             *child;

  BobguiWidget             *default_widget;
  BobguiWidget             *focus_widget;
  BobguiWidget             *move_focus_widget;
  BobguiWindow             *transient_parent;
  BobguiWindowGeometryInfo *geometry_info;
  BobguiWindowGroup        *group;
  GdkDisplay            *display;
  BobguiApplication        *application;

  int default_width;
  int default_height;

  char    *startup_id;
  char    *title;

  guint    keys_changed_handler;

  guint32  initial_timestamp;

  guint    mnemonics_display_timeout_id;

  guint    focus_visible_timeout;

  int      scale;

  int title_height;
  BobguiWidget *title_box;
  BobguiWidget *titlebar;
  BobguiWidget *key_press_focus;

  GdkMonitor *initial_fullscreen_monitor;
  guint      edge_constraints;

  GdkToplevelState state;

  /* The following flags are initially TRUE (before a window is mapped).
   * They cause us to compute a configure request that involves
   * default-only parameters. Once mapped, we set them to FALSE.
   * Then we set them to TRUE again on unmap (for position)
   * and on unrealize (for size).
   */
  guint    need_default_size         : 1;

  guint    decorated                 : 1;
  guint    deletable                 : 1;
  guint    destroy_with_parent       : 1;
  guint    minimize_initially        : 1;
  guint    is_active                 : 1;
  guint    mnemonics_visible         : 1;
  guint    focus_visible             : 1;
  guint    modal                     : 1;
  guint    resizable                 : 1;
  guint    transient_parent_group    : 1;
  guint    client_decorated          : 1; /* Decorations drawn client-side */
  guint    use_client_shadow         : 1; /* Decorations use client-side shadows */
  guint    maximized                 : 1;
  guint    is_set_maximized          : 1;
  guint    suspended                 : 1;
  guint    fullscreen                : 1;
  guint    is_set_fullscreen         : 1;
  guint    tiled                     : 1;

  guint    hide_on_close             : 1;
  guint    in_emit_close_request     : 1;
  guint    move_focus                : 1;
  guint    unset_default             : 1;
  guint    in_present                : 1;

  BobguiGesture *click_gesture;
  BobguiEventController *application_shortcut_controller;

  GdkSurface  *surface;
  GskRenderer *renderer;

  GList *foci;

  BobguiConstraintSolver *constraint_solver;

  int surface_width;
  int surface_height;

  BobguiWindowGravity gravity;

  GdkCursor *resize_cursor;

  BobguiEventController *menubar_controller;
} BobguiWindowPrivate;

enum {
  SET_FOCUS,
  ACTIVATE_FOCUS,
  ACTIVATE_DEFAULT,
  KEYS_CHANGED,
  ENABLE_DEBUGGING,
  CLOSE_REQUEST,
  LAST_SIGNAL
};

enum {
  PROP_0,

  /* Normal Props */
  PROP_TITLE,
  PROP_RESIZABLE,
  PROP_MODAL,
  PROP_DEFAULT_WIDTH,
  PROP_DEFAULT_HEIGHT,
  PROP_DESTROY_WITH_PARENT,
  PROP_HIDE_ON_CLOSE,
  PROP_ICON_NAME,
  PROP_DISPLAY,
  PROP_DECORATED,
  PROP_DELETABLE,
  PROP_TRANSIENT_FOR,
  PROP_APPLICATION,
  PROP_DEFAULT_WIDGET,
  PROP_FOCUS_WIDGET,
  PROP_CHILD,
  PROP_TITLEBAR,
  PROP_HANDLE_MENUBAR_ACCEL,
  PROP_GRAVITY,

  /* Readonly properties */
  PROP_IS_ACTIVE,
  PROP_SUSPENDED,

  /* Writeonly properties */
  PROP_STARTUP_ID,

  PROP_MNEMONICS_VISIBLE,
  PROP_FOCUS_VISIBLE,

  PROP_MAXIMIZED,
  PROP_FULLSCREENED,

  LAST_ARG
};

static GParamSpec *window_props[LAST_ARG] = { NULL, };

/* Must be kept in sync with GdkSurfaceEdge ! */
typedef enum
{
  BOBGUI_WINDOW_REGION_EDGE_NW,
  BOBGUI_WINDOW_REGION_EDGE_N,
  BOBGUI_WINDOW_REGION_EDGE_NE,
  BOBGUI_WINDOW_REGION_EDGE_W,
  BOBGUI_WINDOW_REGION_EDGE_E,
  BOBGUI_WINDOW_REGION_EDGE_SW,
  BOBGUI_WINDOW_REGION_EDGE_S,
  BOBGUI_WINDOW_REGION_EDGE_SE,
  BOBGUI_WINDOW_REGION_CONTENT,
} BobguiWindowRegion;

typedef struct
{
  char      *icon_name;
  guint      realized : 1;
  guint      using_default_icon : 1;
  guint      using_themed_icon : 1;
} BobguiWindowIconInfo;

typedef struct {
  GdkGeometry    geometry; /* Last set of geometry hints we set */
  GdkSurfaceHints flags;
  GdkRectangle   configure_request;
} BobguiWindowLastGeometryInfo;

struct _BobguiWindowGeometryInfo
{
  BobguiWindowLastGeometryInfo last;
};

static void bobgui_window_constructed        (GObject           *object);
static void bobgui_window_dispose            (GObject           *object);
static void bobgui_window_finalize           (GObject           *object);
static void bobgui_window_show               (BobguiWidget         *widget);
static void bobgui_window_hide               (BobguiWidget         *widget);
static void bobgui_window_map                (BobguiWidget         *widget);
static void bobgui_window_unmap              (BobguiWidget         *widget);
static void bobgui_window_realize            (BobguiWidget         *widget);
static void bobgui_window_unrealize          (BobguiWidget         *widget);
static void bobgui_window_size_allocate      (BobguiWidget         *widget,
                                           int                width,
                                           int                height,
                                           int                  baseline);
static gboolean bobgui_window_close_request  (BobguiWindow         *window);
static gboolean bobgui_window_handle_focus   (BobguiWidget         *widget,
                                           GdkEvent          *event,
                                           double             x,
                                           double             y);
static gboolean bobgui_window_key_pressed    (BobguiWidget         *widget,
                                           guint              keyval,
                                           guint              keycode,
                                           GdkModifierType    state,
                                           gpointer           data);
static gboolean bobgui_window_key_released   (BobguiWidget         *widget,
                                           guint              keyval,
                                           guint              keycode,
                                           GdkModifierType    state,
                                           gpointer           data);

static void     surface_state_changed     (BobguiWidget          *widget);
static void     surface_size_changed      (BobguiWidget          *widget,
                                           int                 width,
                                           int                 height);
static gboolean surface_render            (GdkSurface         *surface,
                                           cairo_region_t     *region,
                                           BobguiWidget          *widget);
static gboolean surface_event             (GdkSurface         *surface,
                                           GdkEvent           *event,
                                           BobguiWidget          *widget);
static void     after_paint               (GdkFrameClock      *clock,
                                           BobguiWindow          *window);

static int bobgui_window_focus              (BobguiWidget        *widget,
				           BobguiDirectionType  direction);
static void bobgui_window_move_focus         (BobguiWidget         *widget,
                                           BobguiDirectionType   dir);

static void bobgui_window_real_activate_default (BobguiWindow         *window);
static void bobgui_window_real_activate_focus   (BobguiWindow         *window);
static void bobgui_window_keys_changed          (BobguiWindow         *window);
static gboolean bobgui_window_enable_debugging  (BobguiWindow         *window,
                                              gboolean           toggle);
static void bobgui_window_unset_transient_for         (BobguiWindow  *window);
static void bobgui_window_transient_parent_realized   (BobguiWidget  *parent,
						    BobguiWidget  *window);
static void bobgui_window_transient_parent_unrealized (BobguiWidget  *parent,
						    BobguiWidget  *window);

static BobguiWindowGeometryInfo* bobgui_window_get_geometry_info         (BobguiWindow    *window,
                                                                    gboolean      create);

static void     bobgui_window_set_default_size_internal (BobguiWindow    *window,
                                                      gboolean      change_width,
                                                      int           width,
                                                      gboolean      change_height,
                                                      int           height);

static void     update_themed_icon                    (BobguiWindow    *window);
static GList   *icon_list_from_theme                  (BobguiWindow    *window,
						       const char   *name);
static void     bobgui_window_realize_icon               (BobguiWindow    *window);
static void     bobgui_window_unrealize_icon             (BobguiWindow    *window);
static void     update_window_actions                 (BobguiWindow    *window);
static void     get_shadow_width                      (BobguiWindow    *window,
                                                       BobguiBorder    *shadow_width);

static gboolean    bobgui_window_activate_menubar        (BobguiWidget    *widget,
                                                       GVariant     *args,
                                                       gpointer      unused);
#ifdef GDK_WINDOWING_X11
static void        bobgui_window_on_theme_variant_changed (BobguiSettings *settings,
                                                        GParamSpec  *pspec,
                                                        BobguiWindow   *window);
#endif
static void        bobgui_window_set_theme_variant         (BobguiWindow  *window);

static void bobgui_window_activate_default_activate (BobguiWidget *widget,
                                                  const char *action_name,
                                                  GVariant *parameter);
static void bobgui_window_activate_minimize (BobguiWidget  *widget,
                                          const char *action_name,
                                          GVariant   *parameter);
static void bobgui_window_activate_toggle_maximized (BobguiWidget  *widget,
                                                  const char *name,
                                                  GVariant   *parameter);
static void bobgui_window_activate_close (BobguiWidget  *widget,
                                       const char *action_name,
                                       GVariant   *parameter);

static void _bobgui_window_set_is_active (BobguiWindow *window,
			               gboolean   is_active);
static void bobgui_window_present_toplevel (BobguiWindow *window);
static void bobgui_window_update_toplevel (BobguiWindow         *window,
                                        GdkToplevelLayout *layout);

static void bobgui_window_release_application (BobguiWindow *window);

static GListStore  *toplevel_list = NULL;
static guint        window_signals[LAST_SIGNAL] = { 0 };
static char        *default_icon_name = NULL;
static gboolean     disable_startup_notification = FALSE;

static GQuark       quark_bobgui_window_icon_info = 0;

static BobguiBuildableIface *parent_buildable_iface;

static void bobgui_window_set_property (GObject         *object,
				     guint            prop_id,
				     const GValue    *value,
				     GParamSpec      *pspec);
static void bobgui_window_get_property (GObject         *object,
				     guint            prop_id,
				     GValue          *value,
				     GParamSpec      *pspec);

/* BobguiBuildable */
static void     bobgui_window_buildable_interface_init         (BobguiBuildableIface  *iface);
static void     bobgui_window_buildable_add_child              (BobguiBuildable       *buildable,
                                                             BobguiBuilder         *builder,
                                                             GObject            *child,
                                                             const char         *type);

static void             bobgui_window_shortcut_manager_interface_init      (BobguiShortcutManagerInterface *iface);
/* BobguiRoot */
static void             bobgui_window_root_interface_init (BobguiRootInterface *iface);
static void             bobgui_window_native_interface_init  (BobguiNativeInterface  *iface);

static void             bobgui_window_accessible_interface_init (BobguiAccessibleInterface *iface);


static void ensure_state_flag_backdrop (BobguiWidget *widget);
static void unset_titlebar (BobguiWindow *window);

#define INCLUDE_CSD_SIZE 1
#define EXCLUDE_CSD_SIZE -1

static void
bobgui_window_update_csd_size (BobguiWindow *window,
                            int       *width,
                            int       *height,
                            int        apply);

static void unset_fullscreen_monitor (BobguiWindow *window);

G_DEFINE_TYPE_WITH_CODE (BobguiWindow, bobgui_window, BOBGUI_TYPE_WIDGET,
                         G_ADD_PRIVATE (BobguiWindow)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACCESSIBLE,
						bobgui_window_accessible_interface_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
						bobgui_window_buildable_interface_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_NATIVE,
						bobgui_window_native_interface_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_SHORTCUT_MANAGER,
						bobgui_window_shortcut_manager_interface_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ROOT,
						bobgui_window_root_interface_init))

static BobguiAccessibleInterface *parent_accessible_iface;

static gboolean
bobgui_window_accessible_get_platform_state (BobguiAccessible              *self,
                                          BobguiAccessiblePlatformState  state)
{
  switch (state)
    {
    case BOBGUI_ACCESSIBLE_PLATFORM_STATE_FOCUSABLE:
    case BOBGUI_ACCESSIBLE_PLATFORM_STATE_FOCUSED:
      return parent_accessible_iface->get_platform_state (self, state);
    case BOBGUI_ACCESSIBLE_PLATFORM_STATE_ACTIVE:
      return bobgui_window_is_active (BOBGUI_WINDOW (self));
    default:
      g_assert_not_reached ();
    }
}

static void
bobgui_window_accessible_interface_init (BobguiAccessibleInterface *iface)
{
  parent_accessible_iface = g_type_interface_peek_parent (iface);
  iface->get_at_context = parent_accessible_iface->get_at_context;
  iface->get_platform_state = bobgui_window_accessible_get_platform_state;
}

static void
add_tab_bindings (BobguiWidgetClass   *widget_class,
		  GdkModifierType   modifiers,
		  BobguiDirectionType  direction)
{
  BobguiShortcut *shortcut;

  shortcut = bobgui_shortcut_new_with_arguments (
                 bobgui_alternative_trigger_new (bobgui_keyval_trigger_new (GDK_KEY_Tab, modifiers),
                                              bobgui_keyval_trigger_new (GDK_KEY_KP_Tab, modifiers)),
                 bobgui_signal_action_new ("move-focus"),
                 "(i)", direction);

  bobgui_widget_class_add_shortcut (widget_class, shortcut);

  g_object_unref (shortcut);
}

static void
add_arrow_bindings (BobguiWidgetClass   *widget_class,
		    guint             keysym,
		    BobguiDirectionType  direction)
{
  guint keypad_keysym = keysym - GDK_KEY_Left + GDK_KEY_KP_Left;

  bobgui_widget_class_add_binding_signal (widget_class, keysym, 0,
                                       "move-focus",
                                       "(i)",
                                       direction);
  bobgui_widget_class_add_binding_signal (widget_class, keysym, GDK_CONTROL_MASK,
                                       "move-focus",
                                       "(i)",
                                       direction);
  bobgui_widget_class_add_binding_signal (widget_class, keypad_keysym, 0,
                                       "move-focus",
                                       "(i)",
                                       direction);
  bobgui_widget_class_add_binding_signal (widget_class, keypad_keysym, GDK_CONTROL_MASK,
                                       "move-focus",
                                       "(i)",
                                       direction);
}

static guint32
extract_time_from_startup_id (const char * startup_id)
{
  char *timestr = g_strrstr (startup_id, "_TIME");
  guint32 retval = GDK_CURRENT_TIME;

  if (timestr)
    {
      char *end;
      guint32 timestamp;

      /* Skip past the "_TIME" part */
      timestr += 5;

      end = NULL;
      errno = 0;
      timestamp = g_ascii_strtoull (timestr, &end, 0);
      if (errno == 0 && end != timestr)
        retval = timestamp;
    }

  return retval;
}

static gboolean
startup_id_is_fake (const char * startup_id)
{
  return strncmp (startup_id, "_TIME", 5) == 0;
}

static void
bobgui_window_measure (BobguiWidget      *widget,
                    BobguiOrientation  orientation,
                    int             for_size,
                    int            *minimum,
                    int            *natural,
                    int            *minimum_baseline,
                    int            *natural_baseline)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiWidget *child = priv->child;
  gboolean has_size_request = bobgui_widget_has_size_request (widget);
  int title_for_size = for_size;
  int title_min_size = 0;
  int title_nat_size = 0;
  int child_for_size = for_size;
  int child_min_size = 0;
  int child_nat_size = 0;

  if (priv->decorated && !priv->fullscreen)
    {
      if (priv->title_box != NULL &&
          bobgui_widget_get_visible (priv->title_box) &&
          bobgui_widget_get_child_visible (priv->title_box))
        {
          if (orientation == BOBGUI_ORIENTATION_HORIZONTAL && for_size >= 0 &&
              child != NULL && bobgui_widget_get_visible (child))
            {
              BobguiRequestedSize sizes[2];

              bobgui_widget_measure (priv->title_box,
                                  BOBGUI_ORIENTATION_VERTICAL,
                                  -1,
                                  &sizes[0].minimum_size, &sizes[0].natural_size,
                                  NULL, NULL);
              bobgui_widget_measure (child,
                                  BOBGUI_ORIENTATION_VERTICAL,
                                  -1,
                                  &sizes[1].minimum_size, &sizes[1].natural_size,
                                  NULL, NULL);
              for_size -= sizes[0].minimum_size + sizes[1].minimum_size;
              for_size = bobgui_distribute_natural_allocation (for_size, 2, sizes);
              title_for_size = sizes[0].minimum_size;
              child_for_size = sizes[1].minimum_size + for_size;
            }

          bobgui_widget_measure (priv->title_box,
                              orientation,
                              title_for_size,
                              &title_min_size, &title_nat_size,
                              NULL, NULL);
        }
    }

  if (child != NULL && bobgui_widget_get_visible (child))
    {
      bobgui_widget_measure (child,
                          orientation,
                          child_for_size,
                          &child_min_size, &child_nat_size,
                          NULL, NULL);

      if (child_nat_size == 0 && !has_size_request)
        child_nat_size = NO_CONTENT_CHILD_NAT;
    }
  else if (!has_size_request)
    {
      child_nat_size = NO_CONTENT_CHILD_NAT;
    }

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      *minimum = MAX (title_min_size, child_min_size);
      *natural = MAX (title_nat_size, child_nat_size);
    }
  else
    {
      *minimum = title_min_size + child_min_size;
      *natural = title_nat_size + child_nat_size;
    }
}

static void
bobgui_window_compute_expand (BobguiWidget *widget,
                           gboolean  *hexpand,
                           gboolean  *vexpand)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (priv->child)
    {
      *hexpand = bobgui_widget_compute_expand (priv->child, BOBGUI_ORIENTATION_HORIZONTAL);
      *vexpand = bobgui_widget_compute_expand (priv->child, BOBGUI_ORIENTATION_VERTICAL);
    }
  else
    {
      *hexpand = FALSE;
      *vexpand = FALSE;
    }
}

static BobguiSizeRequestMode
bobgui_window_get_request_mode (BobguiWidget *widget)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (priv->child)
    return bobgui_widget_get_request_mode (priv->child);
  else
    return BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;
}

static GdkGravity
get_gdk_gravity (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  switch (priv->gravity)
    {
    case BOBGUI_WINDOW_GRAVITY_TOP_LEFT:
      return GDK_GRAVITY_NORTH_WEST;
    case BOBGUI_WINDOW_GRAVITY_TOP:
      return GDK_GRAVITY_NORTH;
    case BOBGUI_WINDOW_GRAVITY_TOP_RIGHT:
      return GDK_GRAVITY_NORTH_EAST;
    case BOBGUI_WINDOW_GRAVITY_LEFT:
      return GDK_GRAVITY_WEST;
    case BOBGUI_WINDOW_GRAVITY_CENTER:
      return GDK_GRAVITY_CENTER;
    case BOBGUI_WINDOW_GRAVITY_RIGHT:
      return GDK_GRAVITY_EAST;
    case BOBGUI_WINDOW_GRAVITY_BOTTOM_LEFT:
      return GDK_GRAVITY_SOUTH_WEST;
    case BOBGUI_WINDOW_GRAVITY_BOTTOM:
      return GDK_GRAVITY_SOUTH;
    case BOBGUI_WINDOW_GRAVITY_BOTTOM_RIGHT:
      return GDK_GRAVITY_SOUTH_EAST;
    case BOBGUI_WINDOW_GRAVITY_TOP_START:
      if (bobgui_widget_get_direction (BOBGUI_WIDGET (window)) == BOBGUI_TEXT_DIR_RTL)
        return GDK_GRAVITY_NORTH_EAST;
      else
        return GDK_GRAVITY_NORTH_WEST;
    case BOBGUI_WINDOW_GRAVITY_TOP_END:
      if (bobgui_widget_get_direction (BOBGUI_WIDGET (window)) == BOBGUI_TEXT_DIR_RTL)
        return GDK_GRAVITY_NORTH_WEST;
      else
        return GDK_GRAVITY_NORTH_EAST;
    case BOBGUI_WINDOW_GRAVITY_START:
      if (bobgui_widget_get_direction (BOBGUI_WIDGET (window)) == BOBGUI_TEXT_DIR_RTL)
        return GDK_GRAVITY_EAST;
      else
        return GDK_GRAVITY_WEST;
    case BOBGUI_WINDOW_GRAVITY_END:
      if (bobgui_widget_get_direction (BOBGUI_WIDGET (window)) == BOBGUI_TEXT_DIR_RTL)
        return GDK_GRAVITY_WEST;
      else
        return GDK_GRAVITY_EAST;
    case BOBGUI_WINDOW_GRAVITY_BOTTOM_START:
      if (bobgui_widget_get_direction (BOBGUI_WIDGET (window)) == BOBGUI_TEXT_DIR_RTL)
        return GDK_GRAVITY_SOUTH_EAST;
      else
        return GDK_GRAVITY_SOUTH_WEST;
    case BOBGUI_WINDOW_GRAVITY_BOTTOM_END:
      if (bobgui_widget_get_direction (BOBGUI_WIDGET (window)) == BOBGUI_TEXT_DIR_RTL)
        return GDK_GRAVITY_SOUTH_WEST;
      else
        return GDK_GRAVITY_SOUTH_EAST;
    default:
      g_assert_not_reached ();
    }
}

static void
bobgui_window_direction_changed (BobguiWidget        *widget,
                              BobguiTextDirection  previous_direction)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (priv->surface)
    gdk_toplevel_set_gravity (GDK_TOPLEVEL (priv->surface), get_gdk_gravity (window));
}

static void
bobgui_window_class_init (BobguiWindowClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  quark_bobgui_window_icon_info = g_quark_from_static_string ("bobgui-window-icon-info");

  if (toplevel_list == NULL)
    toplevel_list = g_list_store_new (BOBGUI_TYPE_WIDGET);

  gobject_class->constructed = bobgui_window_constructed;
  gobject_class->dispose = bobgui_window_dispose;
  gobject_class->finalize = bobgui_window_finalize;

  gobject_class->set_property = bobgui_window_set_property;
  gobject_class->get_property = bobgui_window_get_property;

  widget_class->show = bobgui_window_show;
  widget_class->hide = bobgui_window_hide;
  widget_class->map = bobgui_window_map;
  widget_class->unmap = bobgui_window_unmap;
  widget_class->realize = bobgui_window_realize;
  widget_class->unrealize = bobgui_window_unrealize;
  widget_class->size_allocate = bobgui_window_size_allocate;
  widget_class->compute_expand = bobgui_window_compute_expand;
  widget_class->get_request_mode = bobgui_window_get_request_mode;
  widget_class->focus = bobgui_window_focus;
  widget_class->move_focus = bobgui_window_move_focus;
  widget_class->measure = bobgui_window_measure;
  widget_class->direction_changed = bobgui_window_direction_changed;

  klass->activate_default = bobgui_window_real_activate_default;
  klass->activate_focus = bobgui_window_real_activate_focus;
  klass->keys_changed = bobgui_window_keys_changed;
  klass->enable_debugging = bobgui_window_enable_debugging;
  klass->close_request = bobgui_window_close_request;

  /**
   * BobguiWindow:title:
   *
   * The title of the window.
   */
  window_props[PROP_TITLE] =
      g_param_spec_string ("title", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE);

  /**
   * BobguiWindow:startup-id:
   *
   * A write-only property for setting window's startup notification identifier.
   */
  window_props[PROP_STARTUP_ID] =
      g_param_spec_string ("startup-id", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_WRITABLE);

  /**
   * BobguiWindow:resizable:
   *
   * If true, users can resize the window.
   */
  window_props[PROP_RESIZABLE] =
      g_param_spec_boolean ("resizable", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:modal:
   *
   * If true, the window is modal.
   */
  window_props[PROP_MODAL] =
      g_param_spec_boolean ("modal", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:default-width:
   *
   * The default width of the window.
   */
  window_props[PROP_DEFAULT_WIDTH] =
      g_param_spec_int ("default-width", NULL, NULL,
                        -1, G_MAXINT,
                        0,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:default-height:
   *
   * The default height of the window.
   */
  window_props[PROP_DEFAULT_HEIGHT] =
      g_param_spec_int ("default-height", NULL, NULL,
                        -1, G_MAXINT,
                        0,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:destroy-with-parent:
   *
   * If this window should be destroyed when the parent is destroyed.
   */
  window_props[PROP_DESTROY_WITH_PARENT] =
      g_param_spec_boolean ("destroy-with-parent", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:hide-on-close:
   *
   * If this window should be hidden instead of destroyed when the user clicks
   * the close button.
   */
  window_props[PROP_HIDE_ON_CLOSE] =
      g_param_spec_boolean ("hide-on-close", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:mnemonics-visible:
   *
   * Whether mnemonics are currently visible in this window.
   *
   * This property is maintained by BOBGUI based on user input,
   * and should not be set by applications.
   */
  window_props[PROP_MNEMONICS_VISIBLE] =
      g_param_spec_boolean ("mnemonics-visible", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:focus-visible:
   *
   * Whether 'focus rectangles' are currently visible in this window.
   *
   * This property is maintained by BOBGUI based on user input
   * and should not be set by applications.
   */
  window_props[PROP_FOCUS_VISIBLE] =
      g_param_spec_boolean ("focus-visible", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:icon-name:
   *
   * Specifies the name of the themed icon to use as the window icon.
   *
   * See [class@Bobgui.IconTheme] for more details.
   */
  window_props[PROP_ICON_NAME] =
      g_param_spec_string ("icon-name", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:display:
   *
   * The display that will display this window.
   */
  window_props[PROP_DISPLAY] =
      g_param_spec_object ("display", NULL, NULL,
                           GDK_TYPE_DISPLAY,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:is-active:
   *
   * Whether the toplevel is the currently active window.
   */
  window_props[PROP_IS_ACTIVE] =
      g_param_spec_boolean ("is-active", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READABLE);

  /**
   * BobguiWindow:decorated:
   *
   * Whether the window should have a frame (also known as *decorations*).
   */
  window_props[PROP_DECORATED] =
      g_param_spec_boolean ("decorated", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:deletable:
   *
   * Whether the window frame should have a close button.
   */
  window_props[PROP_DELETABLE] =
      g_param_spec_boolean ("deletable", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:transient-for:
   *
   * The transient parent of the window.
   */
  window_props[PROP_TRANSIENT_FOR] =
      g_param_spec_object ("transient-for", NULL, NULL,
                           BOBGUI_TYPE_WINDOW,
                           BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:maximized: (getter is_maximized)
   *
   * Whether the window is maximized.
   *
   * Setting this property is the equivalent of calling
   * [method@Bobgui.Window.maximize] or [method@Bobgui.Window.unmaximize];
   * either operation is asynchronous, which means you will need to
   * connect to the ::notify signal in order to know whether the
   * operation was successful.
   */
  window_props[PROP_MAXIMIZED] =
      g_param_spec_boolean ("maximized", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:fullscreened: (getter is_fullscreen)
   *
   * Whether the window is fullscreen.
   *
   * Setting this property is the equivalent of calling
   * [method@Bobgui.Window.fullscreen] or [method@Bobgui.Window.unfullscreen];
   * either operation is asynchronous, which means you will need to
   * connect to the ::notify signal in order to know whether the
   * operation was successful.
   */
  window_props[PROP_FULLSCREENED] =
      g_param_spec_boolean ("fullscreened", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:suspended: (getter is_suspended)
   *
   * Whether the window is suspended.
   *
   * See [method@Bobgui.Window.is_suspended] for details about what suspended means.
   *
   * Since: 4.12
   */
  window_props[PROP_SUSPENDED] =
      g_param_spec_boolean ("suspended", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READABLE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:application:
   *
   * The `BobguiApplication` associated with the window.
   *
   * The application will be kept alive for at least as long as it
   * has any windows associated with it (see g_application_hold()
   * for a way to keep it alive without windows).
   *
   * Normally, the connection between the application and the window
   * will remain until the window is destroyed, but you can explicitly
   * remove it by setting the this property to `NULL`.
   */
  window_props[PROP_APPLICATION] =
      g_param_spec_object ("application", NULL, NULL,
                           BOBGUI_TYPE_APPLICATION,
                           BOBGUI_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:default-widget:
   *
   * The default widget.
   */
  window_props[PROP_DEFAULT_WIDGET] =
      g_param_spec_object ("default-widget", NULL, NULL,
                           BOBGUI_TYPE_WIDGET,
                           BOBGUI_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:focus-widget: (getter get_focus) (setter set_focus)
   *
   * The focus widget.
   */
  window_props[PROP_FOCUS_WIDGET] =
      g_param_spec_object ("focus-widget", NULL, NULL,
                           BOBGUI_TYPE_WIDGET,
                           BOBGUI_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:child:
   *
   * The child widget.
   */
  window_props[PROP_CHILD] =
      g_param_spec_object ("child", NULL, NULL,
                           BOBGUI_TYPE_WIDGET,
                           BOBGUI_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:titlebar:
   *
   * The titlebar widget.
   *
   * Since: 4.6
   */
  window_props[PROP_TITLEBAR] =
      g_param_spec_object ("titlebar", NULL, NULL,
                           BOBGUI_TYPE_WIDGET,
                           BOBGUI_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:handle-menubar-accel:
   *
   * Whether the window frame should handle <kbd>F10</kbd> for activating
   * menubars.
   *
   * Since: 4.2
   */
  window_props[PROP_HANDLE_MENUBAR_ACCEL] =
      g_param_spec_boolean ("handle-menubar-accel", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindow:gravity:
   *
   * The gravity to use when resizing the window programmatically.
   *
   * Gravity describes which point of the window we want to keep
   * fixed (meaning that the window will grow in the opposite direction).
   * For example, a gravity of `BOBGUI_WINDOW_GRAVITY_TOP_RIGHT` means that we
   * want the to fix top right corner of the window.
   *
   * Since: 4.20
   */
  window_props[PROP_GRAVITY] =
      g_param_spec_enum ("gravity", NULL, NULL,
                         BOBGUI_TYPE_WINDOW_GRAVITY,
                         BOBGUI_WINDOW_GRAVITY_TOP_START,
                         BOBGUI_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, LAST_ARG, window_props);

  /**
   * BobguiWindow::activate-focus:
   * @window: the window which emitted the signal
   *
   * Emitted when the user activates the currently focused
   * widget of @window.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default binding for this signal is <kbd>␣</kbd>.
   */
  window_signals[ACTIVATE_FOCUS] =
    g_signal_new (I_("activate-focus"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiWindowClass, activate_focus),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);

  /**
   * BobguiWindow::activate-default:
   * @window: the window which emitted the signal
   *
   * Emitted when the user activates the default widget.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The keybindings for this signal are all forms of the <kbd>Enter</kbd> key.
   */
  window_signals[ACTIVATE_DEFAULT] =
    g_signal_new (I_("activate-default"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiWindowClass, activate_default),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);

  /**
   * BobguiWindow::keys-changed:
   * @window: the window which emitted the signal
   *
   * Emitted when the set of accelerators or mnemonics that
   * are associated with the window changes.
   *
   * Deprecated: 4.10: Use [class@Bobgui.Shortcut] and [class@Bobgui.EventController]
   *   to implement keyboard shortcuts
   */
  window_signals[KEYS_CHANGED] =
    g_signal_new (I_("keys-changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_DEPRECATED,
                  G_STRUCT_OFFSET (BobguiWindowClass, keys_changed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  0);

  /**
   * BobguiWindow::enable-debugging:
   * @window: the window which emitted the signal
   * @toggle: toggle the debugger
   *
   * Emitted when the user enables or disables interactive debugging.
   *
   * When @toggle is true, interactive debugging is toggled on or off,
   * when it is false, the debugger will be pointed at the widget
   * under the pointer.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default bindings for this signal are
   * <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>I</kbd> and
   * <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>D</kbd>.
   *
   * Return: true if the key binding was handled
   */
  window_signals[ENABLE_DEBUGGING] =
    g_signal_new (I_("enable-debugging"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiWindowClass, enable_debugging),
                  NULL, NULL,
                  _bobgui_marshal_BOOLEAN__BOOLEAN,
                  G_TYPE_BOOLEAN,
                  1, G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (window_signals[ENABLE_DEBUGGING],
                              G_TYPE_FROM_CLASS (gobject_class),
                              _bobgui_marshal_BOOLEAN__BOOLEANv);

  /**
   * BobguiWindow::close-request:
   * @window: the window which emitted the signal
   *
   * Emitted when the user clicks on the close button of the window.
   *
   * Return: true to stop other handlers from being invoked for the signal
   */
  window_signals[CLOSE_REQUEST] =
    g_signal_new (I_("close-request"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiWindowClass, close_request),
                  _bobgui_boolean_handled_accumulator, NULL,
                  _bobgui_marshal_BOOLEAN__VOID,
                  G_TYPE_BOOLEAN,
                  0);
  g_signal_set_va_marshaller (window_signals[CLOSE_REQUEST],
                              BOBGUI_TYPE_WINDOW,
                              _bobgui_marshal_BOOLEAN__VOIDv);


  /*
   * Key bindings
   */

  /**
   * BobguiWindow|default.activate:
   *
   * Activate the default widget.
   */
  bobgui_widget_class_install_action (widget_class, "default.activate", NULL,
                                   bobgui_window_activate_default_activate);

  /**
   * BobguiWindow|window.minimize:
   *
   * Minimize the window.
   */
  bobgui_widget_class_install_action (widget_class, "window.minimize", NULL,
                                   bobgui_window_activate_minimize);

  /**
   * BobguiWindow|window.toggle-maximized:
   *
   * Maximize or restore the window.
   */
  bobgui_widget_class_install_action (widget_class, "window.toggle-maximized", NULL,
                                   bobgui_window_activate_toggle_maximized);

  /**
   * BobguiWindow|window.close:
   *
   * Close the window.
   */
  bobgui_widget_class_install_action (widget_class, "window.close", NULL,
                                   bobgui_window_activate_close);

  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_space, 0,
                                       "activate-focus", NULL);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_KP_Space, 0,
                                       "activate-focus", NULL);

  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_Return, 0,
                                       "activate-default", NULL);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_ISO_Enter, 0,
                                       "activate-default", NULL);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_KP_Enter, 0,
                                       "activate-default", NULL);

  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_I, GDK_CONTROL_MASK|GDK_SHIFT_MASK,
                                       "enable-debugging", "(b)", FALSE);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_D, GDK_CONTROL_MASK|GDK_SHIFT_MASK,
                                       "enable-debugging", "(b)", TRUE);

  add_arrow_bindings (widget_class, GDK_KEY_Up, BOBGUI_DIR_UP);
  add_arrow_bindings (widget_class, GDK_KEY_Down, BOBGUI_DIR_DOWN);
  add_arrow_bindings (widget_class, GDK_KEY_Left, BOBGUI_DIR_LEFT);
  add_arrow_bindings (widget_class, GDK_KEY_Right, BOBGUI_DIR_RIGHT);

  add_tab_bindings (widget_class, 0, BOBGUI_DIR_TAB_FORWARD);
  add_tab_bindings (widget_class, GDK_CONTROL_MASK, BOBGUI_DIR_TAB_FORWARD);
  add_tab_bindings (widget_class, GDK_SHIFT_MASK, BOBGUI_DIR_TAB_BACKWARD);
  add_tab_bindings (widget_class, GDK_CONTROL_MASK | GDK_SHIFT_MASK, BOBGUI_DIR_TAB_BACKWARD);

  bobgui_widget_class_set_css_name (widget_class, I_("window"));

  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_WINDOW);
}

/**
 * bobgui_window_is_maximized: (get-property maximized)
 * @window: a window
 *
 * Retrieves the current maximized state of the window.
 *
 * Note that since maximization is ultimately handled by the window
 * manager and happens asynchronously to an application request, you
 * shouldn’t assume the return value of this function changing
 * immediately (or at all), as an effect of calling
 * [method@Bobgui.Window.maximize] or [method@Bobgui.Window.unmaximize].
 *
 * If the window isn't yet mapped, the value returned will whether the
 * initial requested state is maximized.
 *
 * Returns: whether the window is maximized
 */
gboolean
bobgui_window_is_maximized (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), FALSE);

  return priv->maximized;
}

/**
 * bobgui_window_is_fullscreen: (get-property fullscreened)
 * @window: a window
 *
 * Retrieves the current fullscreen state of the window.
 *
 * Note that since fullscreening is ultimately handled by the window
 * manager and happens asynchronously to an application request, you
 * shouldn’t assume the return value of this function changing
 * immediately (or at all), as an effect of calling
 * [method@Bobgui.Window.fullscreen] or [method@Bobgui.Window.unfullscreen].
 *
 * If the window isn't yet mapped, the value returned will whether the
 * initial requested state is fullscreen.
 *
 * Returns: whether the window is fullscreen
 */
gboolean
bobgui_window_is_fullscreen (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), FALSE);

  return priv->fullscreen;
}

/**
 * bobgui_window_is_suspended: (get-property suspended)
 * @window: a window
 *
 * Retrieves the current suspended state of the window.
 *
 * A window being suspended means it's currently not visible
 * to the user, for example by being on a inactive workspace,
 * minimized, obstructed.
 *
 * Returns: whether the window is suspended
 *
 * Since: 4.12
 */
gboolean
bobgui_window_is_suspended (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), FALSE);

  return priv->suspended;
}

void
_bobgui_window_toggle_maximized (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (priv->maximized)
    bobgui_window_unmaximize (window);
  else
    bobgui_window_maximize (window);
}

/**
 * bobgui_window_close:
 * @window: a window
 *
 * Requests that the window is closed.
 *
 * This is similar to what happens when a window manager
 * close button is clicked.
 *
 * This function can be used with close buttons in custom
 * titlebars.
 */
void
bobgui_window_close (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (!_bobgui_widget_get_realized (BOBGUI_WIDGET (window)))
    return;

  if (priv->in_emit_close_request)
    return;

  g_object_ref (window);

  if (!bobgui_window_emit_close_request (window))
    bobgui_window_destroy (window);

  g_object_unref (window);
}

static guint
constraints_for_edge (GdkSurfaceEdge edge)
{
  switch (edge)
    {
    case GDK_SURFACE_EDGE_NORTH_WEST:
      return GDK_TOPLEVEL_STATE_LEFT_RESIZABLE | GDK_TOPLEVEL_STATE_TOP_RESIZABLE;
    case GDK_SURFACE_EDGE_NORTH:
      return GDK_TOPLEVEL_STATE_TOP_RESIZABLE;
    case GDK_SURFACE_EDGE_NORTH_EAST:
      return GDK_TOPLEVEL_STATE_RIGHT_RESIZABLE | GDK_TOPLEVEL_STATE_TOP_RESIZABLE;
    case GDK_SURFACE_EDGE_WEST:
      return GDK_TOPLEVEL_STATE_LEFT_RESIZABLE;
    case GDK_SURFACE_EDGE_EAST:
      return GDK_TOPLEVEL_STATE_RIGHT_RESIZABLE;
    case GDK_SURFACE_EDGE_SOUTH_WEST:
      return GDK_TOPLEVEL_STATE_LEFT_RESIZABLE | GDK_TOPLEVEL_STATE_BOTTOM_RESIZABLE;
    case GDK_SURFACE_EDGE_SOUTH:
      return GDK_TOPLEVEL_STATE_BOTTOM_RESIZABLE;
    case GDK_SURFACE_EDGE_SOUTH_EAST:
      return GDK_TOPLEVEL_STATE_RIGHT_RESIZABLE | GDK_TOPLEVEL_STATE_BOTTOM_RESIZABLE;
    default:
      g_warn_if_reached ();
      return 0;
    }
}

static int
get_number (BobguiCssValue *value)
{
  double d = bobgui_css_number_value_get (value, 100);

  if (d < 1)
    return ceil (d);
  else
    return floor (d);
}

static void
get_box_border (BobguiCssStyle *style,
                BobguiBorder   *border)
{
  border->top = get_number (style->border->border_top_width) + get_number (style->size->padding_top);
  border->left = get_number (style->border->border_left_width) + get_number (style->size->padding_left);
  border->bottom = get_number (style->border->border_bottom_width) + get_number (style->size->padding_bottom);
  border->right = get_number (style->border->border_right_width) + get_number (style->size->padding_right);
}

static int
get_edge_for_coordinates (BobguiWindow *window,
                          double     x,
                          double     y)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  gboolean supports_edge_constraints;
  BobguiBorder handle_size;
  BobguiCssBoxes css_boxes;
  const graphene_rect_t *border_rect;
  float left, top;
  const GskRoundedRect *border;
  graphene_point_t p;
  int resize_handle_size;

#define edge_or_minus_one(edge) ((supports_edge_constraints && (priv->edge_constraints & constraints_for_edge (edge)) != constraints_for_edge (edge)) ? -1 : edge)

  if (!priv->client_decorated ||
      !priv->resizable ||
      priv->fullscreen ||
      priv->maximized)
    return -1;

  supports_edge_constraints = gdk_toplevel_supports_edge_constraints (GDK_TOPLEVEL (priv->surface));

  if (!supports_edge_constraints && priv->tiled)
    return -1;

  bobgui_css_boxes_init (&css_boxes, BOBGUI_WIDGET (window));
  border = bobgui_css_boxes_get_content_box (&css_boxes);
  border_rect = bobgui_css_boxes_get_content_rect (&css_boxes);

  resize_handle_size = RESIZE_HANDLE_CORNER_SIZE;
  for (int i = 0 ; i < 4; i++)
    {
      resize_handle_size = MAX (resize_handle_size, border->corner[i].width);
      resize_handle_size = MAX (resize_handle_size, border->corner[i].height);
    }

  get_box_border (bobgui_css_node_get_style (bobgui_widget_get_css_node (BOBGUI_WIDGET (window))),
                  &handle_size);

  if (priv->use_client_shadow)
    {
      /* We use a maximum of RESIZE_HANDLE_SIZE pixels for the handle size */
      BobguiBorder shadow;

      get_shadow_width (window, &shadow);
      /* This logic is duplicated in update_realized_window_properties() */
      handle_size.left += shadow.left;
      handle_size.top += shadow.top;
      handle_size.right += shadow.right;
      handle_size.bottom += shadow.bottom;
    }

  left = border_rect->origin.x;
  top = border_rect->origin.y;

  if (x < left && x >= left - handle_size.left)
    {
      if (y < top + resize_handle_size && y >= top - handle_size.top)
        return edge_or_minus_one (GDK_SURFACE_EDGE_NORTH_WEST);

      if (y > top + border_rect->size.height - resize_handle_size &&
          y <= top + border_rect->size.height + handle_size.bottom)
        return edge_or_minus_one (GDK_SURFACE_EDGE_SOUTH_WEST);

      return edge_or_minus_one (GDK_SURFACE_EDGE_WEST);
    }
  else if (x > left + border_rect->size.width &&
           x <= left + border_rect->size.width + handle_size.right)
    {
      if (y < top + resize_handle_size  && y >= top - handle_size.top)
        return edge_or_minus_one (GDK_SURFACE_EDGE_NORTH_EAST);

      if (y > top + border_rect->size.height - resize_handle_size &&
          y <= top + border_rect->size.height + handle_size.bottom)
        return edge_or_minus_one (GDK_SURFACE_EDGE_SOUTH_EAST);

      return edge_or_minus_one (GDK_SURFACE_EDGE_EAST);
    }
  else if (y < top && y >= top - handle_size.top)
    {
      if (x < left + resize_handle_size && x >= left - handle_size.left)
        return edge_or_minus_one (GDK_SURFACE_EDGE_NORTH_WEST);

      if (x > left + border_rect->size.width - resize_handle_size &&
          x <= left + border_rect->size.width + handle_size.right)
        return edge_or_minus_one (GDK_SURFACE_EDGE_NORTH_EAST);

      return edge_or_minus_one (GDK_SURFACE_EDGE_NORTH);
    }
  else if (y > top + border_rect->size.height &&
           y <= top + border_rect->size.height + handle_size.bottom)
    {
      if (x < left + resize_handle_size && x >= left - handle_size.left)
        return edge_or_minus_one (GDK_SURFACE_EDGE_SOUTH_WEST);

      if (x > left + border_rect->size.width - resize_handle_size &&
          x <= left + border_rect->size.width + handle_size.right)
        return edge_or_minus_one (GDK_SURFACE_EDGE_SOUTH_EAST);

      return edge_or_minus_one (GDK_SURFACE_EDGE_SOUTH);
    }

  p = GRAPHENE_POINT_INIT (x,y );
  if (!gsk_rounded_rect_contains_point (border, &p))
    {
      if (gsk_rounded_rect_corner_box_contains_point (border, GSK_CORNER_TOP_LEFT, &p))
        return edge_or_minus_one (GDK_SURFACE_EDGE_NORTH_WEST);

      if (gsk_rounded_rect_corner_box_contains_point (border, GSK_CORNER_TOP_RIGHT, &p))
        return edge_or_minus_one (GDK_SURFACE_EDGE_NORTH_EAST);

      if (gsk_rounded_rect_corner_box_contains_point (border, GSK_CORNER_BOTTOM_RIGHT, &p))
        return edge_or_minus_one (GDK_SURFACE_EDGE_SOUTH_EAST);

      if (gsk_rounded_rect_corner_box_contains_point (border, GSK_CORNER_BOTTOM_LEFT, &p))
        return edge_or_minus_one (GDK_SURFACE_EDGE_SOUTH_WEST);
    }

  return -1;
}

static void
click_gesture_pressed_cb (BobguiGestureClick *gesture,
                          int              n_press,
                          double           x,
                          double           y,
                          BobguiWindow       *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  GdkEventSequence *sequence;
  BobguiWindowRegion region;
  GdkEvent *event;
  GdkDevice *device;
  guint button;
  double tx, ty;
  int edge;

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  button = bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (gesture));
  event = bobgui_gesture_get_last_event (BOBGUI_GESTURE (gesture), sequence);
  device = bobgui_gesture_get_device (BOBGUI_GESTURE (gesture));

  if (!event)
    return;

  if (button != GDK_BUTTON_PRIMARY)
    return;

  if (priv->maximized)
    return;

  if (gdk_display_device_is_grabbed (bobgui_widget_get_display (BOBGUI_WIDGET (window)), device))
    return;

  if (!priv->client_decorated)
    return;

  edge = get_edge_for_coordinates (window, x, y);

  if (edge == -1)
    return;

  region = (BobguiWindowRegion)edge;

  bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);

  gdk_event_get_position (event, &tx, &ty);
  gdk_toplevel_begin_resize (GDK_TOPLEVEL (priv->surface),
                             (GdkSurfaceEdge) region,
                             device,
                             GDK_BUTTON_PRIMARY,
                             tx, ty,
                             gdk_event_get_time (event));

  bobgui_event_controller_reset (BOBGUI_EVENT_CONTROLLER (gesture));
}

static void
device_removed_cb (GdkSeat   *seat,
                   GdkDevice *device,
                   gpointer   user_data)
{
  BobguiWindow *window = user_data;
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  GList *l = priv->foci;

  while (l)
    {
      GList *next;
      BobguiPointerFocus *focus = l->data;

      next = l->next;

      if (focus->device == device)
        {
          priv->foci = g_list_delete_link (priv->foci, l);
          bobgui_pointer_focus_unref (focus);
        }

      l = next;
    }
}

static void
bobgui_window_capture_motion (BobguiWidget *widget,
                           double     x,
                           double     y)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  const char *cursor_names[8] = {
    "nw-resize", "n-resize", "ne-resize",
    "w-resize",               "e-resize",
    "sw-resize", "s-resize", "se-resize"
  };
  int edge;

  edge = get_edge_for_coordinates (window, x, y);
  if (edge != -1 &&
      priv->resize_cursor &&
      strcmp (gdk_cursor_get_name (priv->resize_cursor), cursor_names[edge]) == 0)
    return;

  g_clear_object (&priv->resize_cursor);

  if (edge != -1)
    priv->resize_cursor = gdk_cursor_new_from_name (cursor_names[edge], NULL);

  bobgui_window_maybe_update_cursor (window, widget, NULL);
}

static void
bobgui_window_capture_leave (BobguiWidget *widget)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_clear_object (&priv->resize_cursor);
}

static void
bobgui_window_activate_default_activate (BobguiWidget  *widget,
                                      const char *name,
                                      GVariant   *parameter)
{
  bobgui_window_real_activate_default (BOBGUI_WINDOW (widget));
}

static void
bobgui_window_activate_minimize (BobguiWidget  *widget,
                              const char *name,
                              GVariant   *parameter)
{
  bobgui_window_minimize (BOBGUI_WINDOW (widget));
}

static void
bobgui_window_activate_toggle_maximized (BobguiWidget  *widget,
                                      const char *name,
                                      GVariant   *parameter)
{
  _bobgui_window_toggle_maximized (BOBGUI_WINDOW (widget));
}

static void
bobgui_window_activate_close (BobguiWidget  *widget,
                           const char *name,
                           GVariant   *parameter)
{
  bobgui_window_close (BOBGUI_WINDOW (widget));
}

static gboolean
bobgui_window_accept_rootwindow_drop (BobguiDropTargetAsync *self,
                                   GdkDrop            *drop,
                                   double              x,
                                   double              y,
                                   gpointer            unused)
{
  gdk_drop_finish (drop, GDK_ACTION_MOVE);

  return TRUE;
}

static void
bobgui_window_init (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiWidget *widget;
  GdkSeat *seat;
  BobguiEventController *controller;
  BobguiDropTargetAsync *target;
  BobguiShortcut *shortcut;

  widget = BOBGUI_WIDGET (window);

  bobgui_widget_set_overflow (widget, BOBGUI_OVERFLOW_HIDDEN);

  priv->title = NULL;
  priv->geometry_info = NULL;
  priv->focus_widget = NULL;
  priv->default_widget = NULL;
  priv->resizable = TRUE;
  priv->need_default_size = TRUE;
  priv->modal = FALSE;
  priv->decorated = TRUE;
  priv->display = gdk_display_get_default ();

  priv->state = 0;

  priv->deletable = TRUE;
  priv->startup_id = NULL;
  priv->initial_timestamp = GDK_CURRENT_TIME;
  priv->mnemonics_visible = FALSE;
  priv->focus_visible = TRUE;
  priv->initial_fullscreen_monitor = NULL;
  priv->gravity = BOBGUI_WINDOW_GRAVITY_TOP_START;

  g_object_ref_sink (window);

#ifdef GDK_WINDOWING_X11
  g_signal_connect (bobgui_settings_get_for_display (priv->display),
                    "notify::bobgui-application-prefer-dark-theme",
                    G_CALLBACK (bobgui_window_on_theme_variant_changed), window);
#endif

  bobgui_widget_add_css_class (widget, "background");

  target = bobgui_drop_target_async_new (gdk_content_formats_new ((const char*[1]) { "application/x-rootwindow-drop" }, 1),
                                      GDK_ACTION_MOVE);
  bobgui_event_controller_set_static_name (BOBGUI_EVENT_CONTROLLER (target),
                                        "bobgui-window-rootwindow-drop");
  g_signal_connect (target, "drop", G_CALLBACK (bobgui_window_accept_rootwindow_drop), NULL);
  bobgui_widget_add_controller (BOBGUI_WIDGET (window), BOBGUI_EVENT_CONTROLLER (target));

  seat = gdk_display_get_default_seat (bobgui_widget_get_display (widget));
  if (seat)
    g_signal_connect (seat, "device-removed",
                      G_CALLBACK (device_removed_cb), window);

  controller = bobgui_event_controller_motion_new ();
  bobgui_event_controller_set_static_name (controller, "bobgui-window-resize-cursor");
  bobgui_event_controller_set_propagation_phase (controller, BOBGUI_PHASE_CAPTURE);
  bobgui_event_controller_set_propagation_limit (controller, BOBGUI_LIMIT_NONE);
  g_signal_connect_swapped (controller, "enter",
                            G_CALLBACK (bobgui_window_capture_motion), window);
  g_signal_connect_swapped (controller, "motion",
                            G_CALLBACK (bobgui_window_capture_motion), window);
  g_signal_connect_swapped (controller, "leave",
                            G_CALLBACK (bobgui_window_capture_leave), window);
  bobgui_widget_add_controller (widget, controller);

  controller = bobgui_event_controller_key_new ();
  bobgui_event_controller_set_static_name (controller, "bobgui-window-visible-focus");
  bobgui_event_controller_set_propagation_phase (controller, BOBGUI_PHASE_CAPTURE);
  bobgui_event_controller_set_propagation_limit (controller, BOBGUI_LIMIT_NONE);
  g_signal_connect_swapped (controller, "key-pressed",
                            G_CALLBACK (bobgui_window_key_pressed), window);
  g_signal_connect_swapped (controller, "key-released",
                            G_CALLBACK (bobgui_window_key_released), window);
  bobgui_widget_add_controller (widget, controller);

  controller = bobgui_event_controller_legacy_new ();
  bobgui_event_controller_set_static_name (controller, "bobgui-window-toplevel-focus");
  g_signal_connect_swapped (controller, "event",
                            G_CALLBACK (bobgui_window_handle_focus), window);
  bobgui_widget_add_controller (widget, controller);

  controller = bobgui_shortcut_controller_new ();
  bobgui_event_controller_set_propagation_phase (controller, BOBGUI_PHASE_CAPTURE);

  shortcut = bobgui_shortcut_new (bobgui_keyval_trigger_new (MENU_BAR_ACCEL, 0),
                               bobgui_callback_action_new (bobgui_window_activate_menubar, NULL, NULL));
  bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (controller), shortcut);
  bobgui_event_controller_set_static_name (controller, "bobgui-window-menubar-accel");
  bobgui_widget_add_controller (widget, controller);

  priv->menubar_controller = controller;
}

static void
bobgui_window_constructed (GObject *object)
{
  BobguiWindow *window = BOBGUI_WINDOW (object);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  G_OBJECT_CLASS (bobgui_window_parent_class)->constructed (object);

  priv->click_gesture = bobgui_gesture_click_new ();
  bobgui_event_controller_set_static_name (BOBGUI_EVENT_CONTROLLER (priv->click_gesture), "bobgui-window-resize");
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (priv->click_gesture), 0);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (priv->click_gesture),
                                              BOBGUI_PHASE_BUBBLE);
  g_signal_connect (priv->click_gesture, "pressed",
                    G_CALLBACK (click_gesture_pressed_cb), object);
  bobgui_widget_add_controller (BOBGUI_WIDGET (object), BOBGUI_EVENT_CONTROLLER (priv->click_gesture));

  g_list_store_append (toplevel_list, window);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (window),
                               BOBGUI_ACCESSIBLE_STATE_HIDDEN, TRUE,
                               -1);

  g_object_unref (window);
}

static void
bobgui_window_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
  BobguiWindow  *window = BOBGUI_WINDOW (object);

  switch (prop_id)
    {
    case PROP_TITLE:
      bobgui_window_set_title (window, g_value_get_string (value));
      break;
    case PROP_STARTUP_ID:
      bobgui_window_set_startup_id (window, g_value_get_string (value));
      break;
    case PROP_RESIZABLE:
      bobgui_window_set_resizable (window, g_value_get_boolean (value));
      break;
    case PROP_MODAL:
      bobgui_window_set_modal (window, g_value_get_boolean (value));
      break;
    case PROP_DEFAULT_WIDTH:
      bobgui_window_set_default_size_internal (window,
                                            TRUE, g_value_get_int (value),
                                            FALSE, -1);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (window));
      break;
    case PROP_DEFAULT_HEIGHT:
      bobgui_window_set_default_size_internal (window,
                                            FALSE, -1,
                                            TRUE, g_value_get_int (value));
      bobgui_widget_queue_resize (BOBGUI_WIDGET (window));
      break;
    case PROP_DESTROY_WITH_PARENT:
      bobgui_window_set_destroy_with_parent (window, g_value_get_boolean (value));
      break;
    case PROP_HIDE_ON_CLOSE:
      bobgui_window_set_hide_on_close (window, g_value_get_boolean (value));
      break;
    case PROP_ICON_NAME:
      bobgui_window_set_icon_name (window, g_value_get_string (value));
      break;
    case PROP_DISPLAY:
      bobgui_window_set_display (window, g_value_get_object (value));
      break;
    case PROP_DECORATED:
      bobgui_window_set_decorated (window, g_value_get_boolean (value));
      break;
    case PROP_DELETABLE:
      bobgui_window_set_deletable (window, g_value_get_boolean (value));
      break;
    case PROP_TRANSIENT_FOR:
      bobgui_window_set_transient_for (window, g_value_get_object (value));
      break;
    case PROP_APPLICATION:
      bobgui_window_set_application (window, g_value_get_object (value));
      break;
    case PROP_DEFAULT_WIDGET:
      bobgui_window_set_default_widget (window, g_value_get_object (value));
      break;
    case PROP_MNEMONICS_VISIBLE:
      bobgui_window_set_mnemonics_visible (window, g_value_get_boolean (value));
      break;
    case PROP_FOCUS_VISIBLE:
      bobgui_window_set_focus_visible (window, g_value_get_boolean (value));
      break;
    case PROP_MAXIMIZED:
      if (g_value_get_boolean (value))
        bobgui_window_maximize (window);
      else
        bobgui_window_unmaximize (window);
      break;
    case PROP_FULLSCREENED:
      if (g_value_get_boolean (value))
        bobgui_window_fullscreen (window);
      else
        bobgui_window_unfullscreen (window);
      break;
    case PROP_FOCUS_WIDGET:
      bobgui_window_set_focus (window, g_value_get_object (value));
      break;
    case PROP_CHILD:
      bobgui_window_set_child (window, g_value_get_object (value));
      break;
    case PROP_TITLEBAR:
      bobgui_window_set_titlebar (window, g_value_get_object (value));
      break;
    case PROP_HANDLE_MENUBAR_ACCEL:
      bobgui_window_set_handle_menubar_accel (window, g_value_get_boolean (value));
      break;
    case PROP_GRAVITY:
      bobgui_window_set_gravity (window, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_window_get_property (GObject      *object,
			 guint         prop_id,
			 GValue       *value,
			 GParamSpec   *pspec)
{
  BobguiWindow  *window = BOBGUI_WINDOW (object);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  switch (prop_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;
    case PROP_RESIZABLE:
      g_value_set_boolean (value, priv->resizable);
      break;
    case PROP_MODAL:
      g_value_set_boolean (value, priv->modal);
      break;
    case PROP_DEFAULT_WIDTH:
      g_value_set_int (value, priv->default_width);
      break;
    case PROP_DEFAULT_HEIGHT:
      g_value_set_int (value, priv->default_height);
      break;
    case PROP_DESTROY_WITH_PARENT:
      g_value_set_boolean (value, priv->destroy_with_parent);
      break;
    case PROP_HIDE_ON_CLOSE:
      g_value_set_boolean (value, priv->hide_on_close);
      break;
    case PROP_ICON_NAME:
      g_value_set_string (value, bobgui_window_get_icon_name (window));
      break;
    case PROP_DISPLAY:
      g_value_set_object (value, priv->display);
      break;
    case PROP_IS_ACTIVE:
      g_value_set_boolean (value, priv->is_active);
      break;
    case PROP_DECORATED:
      g_value_set_boolean (value, bobgui_window_get_decorated (window));
      break;
    case PROP_DELETABLE:
      g_value_set_boolean (value, bobgui_window_get_deletable (window));
      break;
    case PROP_TRANSIENT_FOR:
      g_value_set_object (value, bobgui_window_get_transient_for (window));
      break;
    case PROP_APPLICATION:
      g_value_set_object (value, bobgui_window_get_application (window));
      break;
    case PROP_DEFAULT_WIDGET:
      g_value_set_object (value, bobgui_window_get_default_widget (window));
      break;
    case PROP_MNEMONICS_VISIBLE:
      g_value_set_boolean (value, priv->mnemonics_visible);
      break;
    case PROP_FOCUS_VISIBLE:
      g_value_set_boolean (value, priv->focus_visible);
      break;
    case PROP_MAXIMIZED:
      g_value_set_boolean (value, bobgui_window_is_maximized (window));
      break;
    case PROP_FULLSCREENED:
      g_value_set_boolean (value, bobgui_window_is_fullscreen (window));
      break;
    case PROP_SUSPENDED:
      g_value_set_boolean (value, bobgui_window_is_suspended (window));
      break;
    case PROP_FOCUS_WIDGET:
      g_value_set_object (value, bobgui_window_get_focus (window));
      break;
    case PROP_CHILD:
      g_value_set_object (value, bobgui_window_get_child (window));
      break;
    case PROP_TITLEBAR:
      g_value_set_object (value, bobgui_window_get_titlebar (window));
      break;
    case PROP_HANDLE_MENUBAR_ACCEL:
      g_value_set_boolean (value, bobgui_window_get_handle_menubar_accel (window));
      break;
    case PROP_GRAVITY:
      g_value_set_enum (value, bobgui_window_get_gravity (window));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_window_buildable_interface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);
  iface->add_child = bobgui_window_buildable_add_child;
}

static void
bobgui_window_buildable_add_child (BobguiBuildable *buildable,
                                BobguiBuilder   *builder,
                                GObject      *child,
                                const char   *type)
{
  if (type && strcmp (type, "titlebar") == 0)
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, "titlebar", "titlebar");
      bobgui_window_set_titlebar (BOBGUI_WINDOW (buildable), BOBGUI_WIDGET (child));
    }
  else if (BOBGUI_IS_WIDGET (child))
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, NULL, "child");
      bobgui_window_set_child (BOBGUI_WINDOW (buildable), BOBGUI_WIDGET (child));
    }
  else
    {
      parent_buildable_iface->add_child (buildable, builder, child, type);
    }
}

static void
bobgui_window_shortcut_manager_interface_init (BobguiShortcutManagerInterface *iface)
{
}

static GdkDisplay *
bobgui_window_root_get_display (BobguiRoot *root)
{
  BobguiWindow *window = BOBGUI_WINDOW (root);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  return priv->display;
}

static GdkSurface *
bobgui_window_native_get_surface (BobguiNative *native)
{
  BobguiWindow *self = BOBGUI_WINDOW (native);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (self);

  return priv->surface;
}

static GskRenderer *
bobgui_window_native_get_renderer (BobguiNative *native)
{
  BobguiWindow *self = BOBGUI_WINDOW (native);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (self);

  return priv->renderer;
}

static BobguiConstraintSolver *
bobgui_window_root_get_constraint_solver (BobguiRoot *root)
{
  BobguiWindow *self = BOBGUI_WINDOW (root);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (self);

  if (!priv->constraint_solver)
    {
      /* Shared constraint solver */
      priv->constraint_solver = bobgui_constraint_solver_new ();
    }

  return priv->constraint_solver;
}

static BobguiWidget *
bobgui_window_root_get_focus (BobguiRoot *root)
{
  BobguiWindow *self = BOBGUI_WINDOW (root);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (self);

  return priv->focus_widget;
}

static void synthesize_focus_change_events (BobguiWindow       *window,
                                            BobguiWidget       *old_focus,
                                            BobguiWidget       *new_focus,
                                            BobguiCrossingType  type);

static void
bobgui_window_root_set_focus (BobguiRoot   *root,
                           BobguiWidget *focus)
{
  BobguiWindow *self = BOBGUI_WINDOW (root);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (self);
  BobguiWidget *old_focus = NULL;

  if (focus && !bobgui_widget_is_sensitive (focus))
    return;

  if (focus == priv->focus_widget)
    {
      if (priv->move_focus &&
          focus && bobgui_widget_is_visible (focus))
        {
          priv->move_focus = FALSE;
          g_clear_object (&priv->move_focus_widget);
        }
      return;
    }

  if (priv->focus_widget)
    old_focus = g_object_ref (priv->focus_widget);
  g_set_object (&priv->focus_widget, NULL);

  if (old_focus)
    bobgui_widget_set_has_focus (old_focus, FALSE);

  synthesize_focus_change_events (self, old_focus, focus, BOBGUI_CROSSING_FOCUS);

  if (focus)
    bobgui_widget_set_has_focus (focus, priv->is_active);

  g_set_object (&priv->focus_widget, focus);

  g_clear_object (&old_focus);

  if (priv->move_focus &&
      focus && bobgui_widget_is_visible (focus))
    {
      priv->move_focus = FALSE;
      g_clear_object (&priv->move_focus_widget);
    }

  g_object_notify (G_OBJECT (self), "focus-widget");
}

static void
bobgui_window_native_get_surface_transform (BobguiNative *native,
                                         double    *x,
                                         double    *y)
{
  BobguiBorder shadow;
  BobguiCssBoxes css_boxes;
  const graphene_rect_t *margin_rect;

  get_shadow_width (BOBGUI_WINDOW (native), &shadow);
  bobgui_css_boxes_init (&css_boxes, BOBGUI_WIDGET (native));
  margin_rect = bobgui_css_boxes_get_margin_rect (&css_boxes);

  *x = shadow.left - margin_rect->origin.x;
  *y = shadow.top  - margin_rect->origin.y;
}

static void
bobgui_window_native_layout (BobguiNative *native,
                          int        width,
                          int        height)
{
  BobguiWindow *window = BOBGUI_WINDOW (native);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiWidget *widget = BOBGUI_WIDGET (native);

  if (priv->surface_width != width || priv->surface_height != height)
    {
      surface_size_changed (widget, width, height);
      priv->surface_width = width;
      priv->surface_height = height;
    }

  /* This fake motion event is needed for getting up to date pointer focus
   * and coordinates when tho pointer didn't move but the layout changed
   * within the window.
   */
  if (bobgui_widget_needs_allocate (widget))
    {
      GdkSeat *seat;

      seat = gdk_display_get_default_seat (bobgui_widget_get_display (widget));
      if (seat)
        {
          GdkDevice *device;
          BobguiWidget *focus;

          device = gdk_seat_get_pointer (seat);
          focus = bobgui_window_lookup_pointer_focus_widget (BOBGUI_WINDOW (widget),
                                                          device, NULL);
          if (focus)
            {
              GdkSurface *surface;

              surface = bobgui_native_get_surface (bobgui_widget_get_native (focus));

              if (surface)
                gdk_surface_request_motion (surface);
            }
        }
    }

  if (bobgui_widget_needs_allocate (widget))
    {
      bobgui_window_update_csd_size (window,
                                  &width, &height,
                                  EXCLUDE_CSD_SIZE);
      bobgui_widget_allocate (widget, width, height, -1, NULL);
    }
  else
    {
      bobgui_widget_ensure_allocate (widget);
    }
}

static void
bobgui_window_root_interface_init (BobguiRootInterface *iface)
{
  iface->get_display = bobgui_window_root_get_display;
  iface->get_constraint_solver = bobgui_window_root_get_constraint_solver;
  iface->get_focus = bobgui_window_root_get_focus;
  iface->set_focus = bobgui_window_root_set_focus;
}

static void
bobgui_window_native_interface_init (BobguiNativeInterface *iface)
{
  iface->get_surface = bobgui_window_native_get_surface;
  iface->get_renderer = bobgui_window_native_get_renderer;
  iface->get_surface_transform = bobgui_window_native_get_surface_transform;
  iface->layout = bobgui_window_native_layout;
}

/**
 * bobgui_window_new:
 *
 * Creates a new `BobguiWindow`.
 *
 * To get an undecorated window (without window borders),
 * use [method@Bobgui.Window.set_decorated].
 *
 * All top-level windows created by this function are stored
 * in an internal top-level window list. This list can be obtained
 * from [func@Bobgui.Window.list_toplevels]. Due to BOBGUI keeping a
 * reference to the window internally, this function does not
 * return a reference to the caller.
 *
 * To delete a `BobguiWindow`, call [method@Bobgui.Window.destroy].
 *
 * Returns: a new `BobguiWindow`
 */
BobguiWidget*
bobgui_window_new (void)
{
  return g_object_new (BOBGUI_TYPE_WINDOW, NULL);
}

/**
 * bobgui_window_set_title:
 * @window: a window
 * @title: (nullable): title of the window
 *
 * Sets the title of the window.
 *
 * The title of a window will be displayed in its title bar; on the
 * X Window System, the title bar is rendered by the window manager
 * so exactly how the title appears to users may vary according to a
 * user’s exact configuration. The title should help a user distinguish
 * this window from other windows they may have open. A good title might
 * include the application name and current document filename, for example.
 *
 * Passing `NULL` does the same as setting the title to an empty string.
 */
void
bobgui_window_set_title (BobguiWindow  *window,
                      const char *title)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  char *new_title;

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  new_title = g_strdup (title);
  g_free (priv->title);
  priv->title = new_title;

  if (_bobgui_widget_get_realized (BOBGUI_WIDGET (window)))
    gdk_toplevel_set_title (GDK_TOPLEVEL (priv->surface), new_title != NULL ? new_title : "");

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (window),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL, priv->title,
                                  -1);

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_TITLE]);
}

/**
 * bobgui_window_get_title:
 * @window: a window
 *
 * Retrieves the title of the window.
 *
 * Returns: (nullable): the title
 */
const char *
bobgui_window_get_title (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), NULL);

  return priv->title;
}

/**
 * bobgui_window_set_default_widget:
 * @window: a window
 * @default_widget: (nullable): widget to be the default
 *
 * Sets the default widget.
 *
 * The default widget is the widget that is activated
 * when the user presses <kbd>Enter</kbd> in a dialog
 * (for example).
 */
void
bobgui_window_set_default_widget (BobguiWindow *window,
                               BobguiWidget *default_widget)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  if (priv->default_widget != default_widget)
    {
      BobguiWidget *old_default_widget = NULL;

      if (default_widget)
	g_object_ref (default_widget);

      if (priv->default_widget)
	{
          old_default_widget = priv->default_widget;

          if (priv->focus_widget != priv->default_widget ||
              !bobgui_widget_get_receives_default (priv->default_widget))
            _bobgui_widget_set_has_default (priv->default_widget, FALSE);

          bobgui_widget_queue_draw (priv->default_widget);
	}

      priv->default_widget = default_widget;

      priv->unset_default = FALSE;

      if (priv->default_widget)
	{
          if (priv->focus_widget == NULL ||
              !bobgui_widget_get_receives_default (priv->focus_widget))
            _bobgui_widget_set_has_default (priv->default_widget, TRUE);

          bobgui_widget_queue_draw (priv->default_widget);
	}

      if (old_default_widget)
	g_object_notify (G_OBJECT (old_default_widget), "has-default");

      if (default_widget)
	{
	  g_object_notify (G_OBJECT (default_widget), "has-default");
	  g_object_unref (default_widget);
	}

      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_DEFAULT_WIDGET]);
    }
}

/**
 * bobgui_window_get_default_widget:
 * @window: a window
 *
 * Returns the default widget for @window.
 *
 * Returns: (nullable) (transfer none): the default widget
 */
BobguiWidget *
bobgui_window_get_default_widget (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), NULL);

  return priv->default_widget;
}

static gboolean
handle_keys_changed (gpointer data)
{
  BobguiWindow *window = BOBGUI_WINDOW (data);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (priv->keys_changed_handler)
    {
      g_source_remove (priv->keys_changed_handler);
      priv->keys_changed_handler = 0;
    }

  if (priv->application_shortcut_controller)
    bobgui_shortcut_controller_update_accels (BOBGUI_SHORTCUT_CONTROLLER (priv->application_shortcut_controller));
  g_signal_emit (window, window_signals[KEYS_CHANGED], 0);

  return FALSE;
}

void
_bobgui_window_notify_keys_changed (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (!priv->keys_changed_handler)
    {
      priv->keys_changed_handler = g_idle_add (handle_keys_changed, window);
      gdk_source_set_static_name_by_id (priv->keys_changed_handler, "[bobgui] handle_keys_changed");
    }
}

/**
 * bobgui_window_get_focus: (get-property focus-widget)
 * @window: a window
 *
 * Retrieves the current focused widget within the window.
 *
 * Note that this is the widget that would have the focus
 * if the toplevel window focused; if the toplevel window
 * is not focused then `bobgui_widget_has_focus (widget)` will
 * not be false for the widget.
 *
 * Returns: (nullable) (transfer none): the currently focused widget
 */
BobguiWidget *
bobgui_window_get_focus (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), NULL);

  return priv->focus_widget;
}

static void
bobgui_window_real_activate_default (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (priv->default_widget && bobgui_widget_is_sensitive (priv->default_widget) &&
      (!priv->focus_widget || !bobgui_widget_get_receives_default (priv->focus_widget)))
    bobgui_widget_activate (priv->default_widget);
  else if (priv->focus_widget && bobgui_widget_is_sensitive (priv->focus_widget))
    bobgui_widget_activate (priv->focus_widget);
}

/**
 * bobgui_window_set_modal:
 * @window: a window
 * @modal: whether the window is modal
 *
 * Sets a window modal or non-modal.
 *
 * Modal windows prevent interaction with other windows in the same
 * application. To keep modal dialogs on top of main application windows,
 * use [method@Bobgui.Window.set_transient_for] to make the dialog transient
 * for the parent; most window managers will then disallow lowering the
 * dialog below the parent.
 */
void
bobgui_window_set_modal (BobguiWindow *window,
                      gboolean   modal)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiWidget *widget;

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  modal = modal != FALSE;
  if (priv->modal == modal)
    return;

  priv->modal = modal;
  widget = BOBGUI_WIDGET (window);

  if (_bobgui_widget_get_realized (widget))
    gdk_toplevel_set_modal (GDK_TOPLEVEL (priv->surface), modal);

  if (bobgui_widget_get_visible (widget))
    {
      if (priv->modal)
        bobgui_grab_add (widget);
      else
        bobgui_grab_remove (widget);
    }

  update_window_actions (window);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (window),
                                  BOBGUI_ACCESSIBLE_PROPERTY_MODAL, modal,
                                  -1);

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_MODAL]);
}

/**
 * bobgui_window_get_modal:
 * @window: a window
 *
 * Returns whether the window is modal.
 *
 * Returns: true if the window is set to be modal and
 *   establishes a grab when shown
 */
gboolean
bobgui_window_get_modal (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), FALSE);

  return priv->modal;
}

/**
 * bobgui_window_get_toplevels:
 *
 * Returns the list of all existing toplevel windows.
 *
 * If you want to iterate through the list and perform actions involving
 * callbacks that might destroy the widgets or add new ones, be aware that
 * the list of toplevels will change and emit the "items-changed" signal.
 *
 * Returns: (transfer none) (attributes element-type=BobguiWindow): the list
 *   of toplevel widgets
 */
GListModel *
bobgui_window_get_toplevels (void)
{
  if (toplevel_list == NULL)
    toplevel_list = g_list_store_new (BOBGUI_TYPE_WIDGET);

  return G_LIST_MODEL (toplevel_list);
}

/**
 * bobgui_window_list_toplevels:
 *
 * Returns the list of all existing toplevel windows.
 *
 * The widgets in the list are not individually referenced.
 * If you want to iterate through the list and perform actions
 * involving callbacks that might destroy the widgets, you must
 * call `g_list_foreach (result, (GFunc)g_object_ref, NULL)` first,
 * and then unref all the widgets afterwards.
 *
 * Returns: (element-type BobguiWidget) (transfer container): list of
 *   toplevel widgets
 */
GList *
bobgui_window_list_toplevels (void)
{
  GListModel *toplevels;
  GList *list = NULL;
  guint i;

  toplevels = bobgui_window_get_toplevels ();

  for (i = 0; i < g_list_model_get_n_items (toplevels); i++)
    {
      gpointer item = g_list_model_get_item (toplevels, i);
      list = g_list_prepend (list, item);
      g_object_unref (item);
    }

  return list;
}

static void
bobgui_window_dispose (GObject *object)
{
  BobguiWindow *window = BOBGUI_WINDOW (object);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  bobgui_window_release_application (window);

  if (priv->transient_parent)
    bobgui_window_set_transient_for (window, NULL);

  if (priv->group)
    bobgui_window_group_remove_window (priv->group, window);

  unset_fullscreen_monitor (window);

  g_list_free_full (priv->foci, (GDestroyNotify) bobgui_pointer_focus_unref);
  priv->foci = NULL;

  g_clear_object (&priv->move_focus_widget);
  bobgui_window_set_focus (window, NULL);
  bobgui_window_set_default_widget (window, NULL);

  g_clear_pointer (&priv->child, bobgui_widget_unparent);
  unset_titlebar (window);

  G_OBJECT_CLASS (bobgui_window_parent_class)->dispose (object);
}

static void
bobgui_window_transient_parent_destroyed (BobguiWindow *parent,
                                       BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (BOBGUI_WINDOW (window));

  bobgui_window_unset_transient_for (window);

  if (priv->destroy_with_parent)
    bobgui_window_destroy (window);
}

static void
bobgui_window_transient_parent_realized (BobguiWidget *parent,
                                      BobguiWidget *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (BOBGUI_WINDOW (window));
  BobguiWindowPrivate *parent_priv = bobgui_window_get_instance_private (BOBGUI_WINDOW (parent));
  if (_bobgui_widget_get_realized (window))
    gdk_toplevel_set_transient_for (GDK_TOPLEVEL (priv->surface), parent_priv->surface);
}

static void
bobgui_window_transient_parent_unrealized (BobguiWidget *parent,
                                        BobguiWidget *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (BOBGUI_WINDOW (window));
  if (_bobgui_widget_get_realized (window))
    gdk_toplevel_set_transient_for (GDK_TOPLEVEL (priv->surface), NULL);
}

static void
bobgui_window_transient_parent_display_changed (BobguiWindow  *parent,
                                             GParamSpec *pspec,
                                             BobguiWindow  *window)
{
  BobguiWindowPrivate *parent_priv = bobgui_window_get_instance_private (parent);

  bobgui_window_set_display (window, parent_priv->display);
}

static void
bobgui_window_unset_transient_for (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (priv->transient_parent)
    {
      g_signal_handlers_disconnect_by_func (priv->transient_parent,
                                            bobgui_window_transient_parent_realized,
                                            window);
      g_signal_handlers_disconnect_by_func (priv->transient_parent,
                                            bobgui_window_transient_parent_unrealized,
                                            window);
      g_signal_handlers_disconnect_by_func (priv->transient_parent,
                                            bobgui_window_transient_parent_display_changed,
                                            window);
      g_signal_handlers_disconnect_by_func (priv->transient_parent,
                                            bobgui_window_transient_parent_destroyed,
                                            window);

      priv->transient_parent = NULL;

      if (priv->transient_parent_group)
        {
          priv->transient_parent_group = FALSE;
          bobgui_window_group_remove_window (priv->group, window);
        }
    }
}

/**
 * bobgui_window_set_transient_for:
 * @window: a window
 * @parent: (nullable): parent window
 *
 * Sets a transient parent for the window.
 *
 * Dialog windows should be set transient for the main application
 * window they were spawned from. This allows window managers to e.g.
 * keep the dialog on top of the main window, or center the dialog
 * over the main window. [ctor@Bobgui.Dialog.new_with_buttons] and other
 * convenience functions in BOBGUI will sometimes call this function on
 * your behalf.
 *
 * Passing `NULL` for @parent unsets the current transient window.
 *
 * On Windows, this function puts the child window on top of the parent,
 * much as the window manager would have done on X.
 */
void
bobgui_window_set_transient_for (BobguiWindow *window,
                              BobguiWindow *parent)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (BOBGUI_IS_WINDOW (window));
  g_return_if_fail (parent == NULL || BOBGUI_IS_WINDOW (parent));
  g_return_if_fail (window != parent);

  if (priv->transient_parent)
    {
      if (_bobgui_widget_get_realized (BOBGUI_WIDGET (window)) &&
          _bobgui_widget_get_realized (BOBGUI_WIDGET (priv->transient_parent)) &&
          (!parent || !_bobgui_widget_get_realized (BOBGUI_WIDGET (parent))))
        bobgui_window_transient_parent_unrealized (BOBGUI_WIDGET (priv->transient_parent),
                                                BOBGUI_WIDGET (window));
      bobgui_window_unset_transient_for (window);
    }

  priv->transient_parent = parent;

  if (parent)
    {
      BobguiWindowPrivate *parent_priv = bobgui_window_get_instance_private (parent);
      g_signal_connect (parent, "realize",
                        G_CALLBACK (bobgui_window_transient_parent_realized), window);
      g_signal_connect (parent, "unrealize",
                        G_CALLBACK (bobgui_window_transient_parent_unrealized), window);
      g_signal_connect (parent, "notify::display",
                        G_CALLBACK (bobgui_window_transient_parent_display_changed), window);
      g_signal_connect (parent, "destroy",
                        G_CALLBACK (bobgui_window_transient_parent_destroyed), window);

      bobgui_window_set_display (window, parent_priv->display);


      if (_bobgui_widget_get_realized (BOBGUI_WIDGET (window)) &&
          _bobgui_widget_get_realized (BOBGUI_WIDGET (parent)))
        bobgui_window_transient_parent_realized (BOBGUI_WIDGET (parent), BOBGUI_WIDGET (window));

      if (parent_priv->group)
        {
          bobgui_window_group_add_window (parent_priv->group, window);
          priv->transient_parent_group = TRUE;
        }
    }

  update_window_actions (window);

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_TRANSIENT_FOR]);
}

/**
 * bobgui_window_get_transient_for:
 * @window: a window
 *
 * Fetches the transient parent for this window.
 *
 * Returns: (nullable) (transfer none): the transient parent
 */
BobguiWindow *
bobgui_window_get_transient_for (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), NULL);

  return priv->transient_parent;
}

/**
 * bobgui_window_get_application:
 * @window: a window
 *
 * Gets the application object associated with the window.
 *
 * Returns: (nullable) (transfer none): the application
 */
BobguiApplication *
bobgui_window_get_application (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), NULL);

  return priv->application;
}

static void
bobgui_window_release_application (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (priv->application)
    {
      BobguiApplication *application;

      /* steal reference into temp variable */
      application = priv->application;
      priv->application = NULL;
      bobgui_widget_remove_controller (BOBGUI_WIDGET (window),
                                    priv->application_shortcut_controller);
      priv->application_shortcut_controller = NULL;

      bobgui_application_remove_window (application, window);
      g_object_unref (application);
    }
}

/**
 * bobgui_window_set_application:
 * @window: a window
 * @application: (nullable): a `BobguiApplication`
 *
 * Sets or unsets the application object associated with the window.
 *
 * The application will be kept alive for at least as long as it has
 * any windows associated with it (see [method@Gio.Application.hold]
 * for a way to keep it alive without windows).
 *
 * Normally, the connection between the application and the window will
 * remain until the window is destroyed, but you can explicitly remove
 * it by setting the @application to %NULL.
 *
 * This is equivalent to calling [method@Bobgui.Application.remove_window]
 * and/or [method@Bobgui.Application.add_window] on the old/new applications
 * as relevant.
 */
void
bobgui_window_set_application (BobguiWindow      *window,
                            BobguiApplication *application)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  if (priv->application != application)
    {
      bobgui_window_release_application (window);

      priv->application = application;

      if (priv->application != NULL)
        {
          BobguiApplicationAccels *app_accels;

          g_object_ref (priv->application);

          bobgui_application_add_window (priv->application, window);

          app_accels = bobgui_application_get_application_accels (priv->application);
          priv->application_shortcut_controller = bobgui_shortcut_controller_new_for_model (bobgui_application_accels_get_shortcuts (app_accels));
          bobgui_event_controller_set_static_name (priv->application_shortcut_controller, "bobgui-application-shortcuts");
          bobgui_event_controller_set_propagation_phase (priv->application_shortcut_controller, BOBGUI_PHASE_CAPTURE);
          bobgui_shortcut_controller_set_scope (BOBGUI_SHORTCUT_CONTROLLER (priv->application_shortcut_controller), BOBGUI_SHORTCUT_SCOPE_GLOBAL);
          bobgui_widget_add_controller (BOBGUI_WIDGET (window), priv->application_shortcut_controller);
        }

      _bobgui_widget_update_parent_muxer (BOBGUI_WIDGET (window));

      _bobgui_window_notify_keys_changed (window);

      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_APPLICATION]);
    }
}

/**
 * bobgui_window_set_destroy_with_parent:
 * @window: a window
 * @setting: whether to destroy the window with its transient parent
 *
 * Sets whether to destroy the window when the transient parent is destroyed.
 *
 * This is useful for dialogs that shouldn’t persist beyond the lifetime
 * of the main window they are associated with, for example.
 */
void
bobgui_window_set_destroy_with_parent (BobguiWindow *window,
                                    gboolean   setting)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  if (priv->destroy_with_parent == (setting != FALSE))
    return;

  priv->destroy_with_parent = setting;

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_DESTROY_WITH_PARENT]);
}

/**
 * bobgui_window_get_destroy_with_parent:
 * @window: a window
 *
 * Returns whether the window will be destroyed with its transient parent.
 *
 * Returns: true if the window will be destroyed with its transient parent
 */
gboolean
bobgui_window_get_destroy_with_parent (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), FALSE);

  return priv->destroy_with_parent;
}

/**
 * bobgui_window_set_hide_on_close:
 * @window: a window
 * @setting: whether to hide the window when it is closed
 *
 * Sets whether clicking the close button will hide the window instead
 * of destroying it.
 */
void
bobgui_window_set_hide_on_close (BobguiWindow *window,
                              gboolean   setting)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  if (priv->hide_on_close == setting)
    return;

  priv->hide_on_close = setting;

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_HIDE_ON_CLOSE]);
}

/**
 * bobgui_window_get_hide_on_close:
 * @window: a window
 *
 * Returns whether the window will be hidden instead of destroyed when the close
 * button is clicked.
 *
 * Returns: true if the window will be hidden
 */
gboolean
bobgui_window_get_hide_on_close (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), FALSE);

  return priv->hide_on_close;
}

static BobguiWindowGeometryInfo*
bobgui_window_get_geometry_info (BobguiWindow *window,
			      gboolean   create)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiWindowGeometryInfo *info;

  info = priv->geometry_info;
  if (!info && create)
    {
      info = g_new0 (BobguiWindowGeometryInfo, 1);

      info->last.configure_request.x = 0;
      info->last.configure_request.y = 0;
      info->last.configure_request.width = -1;
      info->last.configure_request.height = -1;
      priv->geometry_info = info;
    }

  return info;
}

static void
unset_titlebar (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (priv->title_box != NULL)
    {
      bobgui_widget_unparent (priv->title_box);
      priv->title_box = NULL;
      priv->titlebar = NULL;
    }
}

static gboolean
bobgui_window_is_composited (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  GdkDisplay *display;

  display = priv->display;

  return gdk_display_is_rgba (display) && gdk_display_is_composited (display);
}

static gboolean
bobgui_window_supports_client_shadow (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  GdkDisplay *display;

  display = priv->display;

  return gdk_display_supports_shadow_width (display);
}

static void
bobgui_window_enable_csd (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiWidget *widget = BOBGUI_WIDGET (window);

  /* We need a visual with alpha for rounded corners */
  if (bobgui_window_is_composited (window))
    {
      bobgui_widget_add_css_class (widget, "csd");
    }
  else
    {
      bobgui_widget_add_css_class (widget, "solid-csd");
    }

  priv->client_decorated = TRUE;
}

/**
 * bobgui_window_set_titlebar:
 * @window: a window
 * @titlebar: (nullable): the widget to use as titlebar
 *
 * Sets a custom titlebar for the window.
 *
 * A typical widget used here is [class@Bobgui.HeaderBar], as it
 * provides various features expected of a titlebar while allowing
 * the addition of child widgets to it.
 *
 * If you set a custom titlebar, BOBGUI will do its best to convince
 * the window manager not to put its own titlebar on the window.
 * Depending on the system, this function may not work for a window
 * that is already visible, so you set the titlebar before calling
 * [method@Bobgui.Widget.show].
 */
void
bobgui_window_set_titlebar (BobguiWindow *window,
                         BobguiWidget *titlebar)
{
  BobguiWidget *widget = BOBGUI_WIDGET (window);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  gboolean was_mapped;

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  if (priv->titlebar == titlebar)
    return;

  if ((!priv->title_box && titlebar) || (priv->title_box && !titlebar))
    {
      was_mapped = _bobgui_widget_get_mapped (widget);
      if (_bobgui_widget_get_realized (widget))
        {
          g_warning ("bobgui_window_set_titlebar() called on a realized window");
          bobgui_widget_unrealize (widget);
        }
    }
  else
    was_mapped = FALSE;

  unset_titlebar (window);

  if (titlebar == NULL)
    {
      /* these are updated in realize() */
      priv->client_decorated = FALSE;
      bobgui_widget_remove_css_class (widget, "csd");
      bobgui_widget_remove_css_class (widget, "solid-csd");
    }
  else
    {
      priv->use_client_shadow = bobgui_window_supports_client_shadow (window);
      bobgui_window_enable_csd (window);

      priv->titlebar = titlebar;
      priv->title_box = titlebar;
      bobgui_widget_insert_before (priv->title_box, widget, NULL);

      bobgui_widget_add_css_class (titlebar, "titlebar");
    }

  if (was_mapped)
    bobgui_widget_map (widget);

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_TITLEBAR]);
}

/**
 * bobgui_window_get_titlebar:
 * @window: a window
 *
 * Returns the titlebar that has been set with
 * [method@Bobgui.Window.set_titlebar].
 *
 * Returns: (nullable) (transfer none): the titlebar
 */
BobguiWidget *
bobgui_window_get_titlebar (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), NULL);

  return priv->titlebar;
}

/**
 * bobgui_window_set_decorated:
 * @window: a window
 * @setting: true to decorate the window
 *
 * Sets whether the window should be decorated.
 *
 * By default, windows are decorated with a title bar, resize
 * controls, etc. Some window managers allow BOBGUI to disable these
 * decorations, creating a borderless window. If you set the decorated
 * property to false using this function, BOBGUI will do its best to
 * convince the window manager not to decorate the window. Depending on
 * the system, this function may not have any effect when called on a
 * window that is already visible, so you should call it before calling
 * [method@Bobgui.Widget.show].
 *
 * On Windows, this function always works, since there’s no window manager
 * policy involved.
 */
void
bobgui_window_set_decorated (BobguiWindow *window,
                          gboolean   setting)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  setting = setting != FALSE;

  if (setting == priv->decorated)
    return;

  priv->decorated = setting;

  if (priv->surface)
    gdk_toplevel_set_decorated (GDK_TOPLEVEL (priv->surface), priv->decorated && !priv->client_decorated);

  update_window_actions (window);
  bobgui_widget_queue_resize (BOBGUI_WIDGET (window));

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_DECORATED]);
}

/**
 * bobgui_window_get_decorated:
 * @window: a window
 *
 * Returns whether the window has been set to have decorations.
 *
 * Returns: true if the window has been set to have decorations
 */
gboolean
bobgui_window_get_decorated (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), TRUE);

  return priv->decorated;
}

/**
 * bobgui_window_set_deletable:
 * @window: a window
 * @setting: true to decorate the window as deletable
 *
 * Sets whether the window should be deletable.
 *
 * By default, windows have a close button in the window frame.
 * Some  window managers allow BOBGUI to disable this button. If you
 * set the deletable property to false using this function, BOBGUI
 * will do its best to convince the window manager not to show a
 * close button. Depending on the system, this function may not
 * have any effect when called on a window that is already visible,
 * so you should call it before calling [method@Bobgui.Widget.show].
 *
 * On Windows, this function always works, since there’s no window
 * manager policy involved.
 */
void
bobgui_window_set_deletable (BobguiWindow *window,
			  gboolean   setting)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  setting = setting != FALSE;

  if (setting == priv->deletable)
    return;

  priv->deletable = setting;

  if (priv->surface)
    gdk_toplevel_set_deletable (GDK_TOPLEVEL (priv->surface), priv->deletable);

  update_window_actions (window);

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_DELETABLE]);
}

/**
 * bobgui_window_get_deletable:
 * @window: a window
 *
 * Returns whether the window has been set to have a close button.
 *
 * Returns: true if the window has been set to have a close button
 */
gboolean
bobgui_window_get_deletable (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), TRUE);

  return priv->deletable;
}

static BobguiWindowIconInfo*
get_icon_info (BobguiWindow *window)
{
  return g_object_get_qdata (G_OBJECT (window), quark_bobgui_window_icon_info);
}

static void
free_icon_info (BobguiWindowIconInfo *info)
{
  g_free (info->icon_name);
  g_free (info);
}


static BobguiWindowIconInfo*
ensure_icon_info (BobguiWindow *window)
{
  BobguiWindowIconInfo *info;

  info = get_icon_info (window);

  if (info == NULL)
    {
      info = g_new0 (BobguiWindowIconInfo, 1);
      g_object_set_qdata_full (G_OBJECT (window),
                              quark_bobgui_window_icon_info,
                              info,
                              (GDestroyNotify)free_icon_info);
    }

  return info;
}

static int
icon_size_compare (GdkTexture *a,
                   GdkTexture *b)
{
  int area_a, area_b;

  area_a = gdk_texture_get_width (a) * gdk_texture_get_height (a);
  area_b = gdk_texture_get_width (b) * gdk_texture_get_height (b);

  return area_a - area_b;
}

static GdkTexture *
render_paintable_to_texture (GdkPaintable *paintable)
{
  BobguiSnapshot *snapshot;
  GskRenderNode *node;
  int width, height;
  cairo_surface_t *surface;
  cairo_t *cr;
  GdkTexture *texture;

  width = gdk_paintable_get_intrinsic_width (paintable);
  height = gdk_paintable_get_intrinsic_height (paintable);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);

  snapshot = bobgui_snapshot_new ();
  gdk_paintable_snapshot (paintable, snapshot, width, height);
  node = bobgui_snapshot_free_to_node (snapshot);

  cr = cairo_create (surface);
  gsk_render_node_draw (node, cr);
  cairo_destroy (cr);

  gsk_render_node_unref (node);

  texture = gdk_texture_new_for_surface (surface);
  cairo_surface_destroy (surface);

  return texture;
}

static GList *
icon_list_from_theme (BobguiWindow   *window,
		      const char *name)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  GList *list;
  BobguiIconTheme *icon_theme;
  BobguiIconPaintable *info;
  GdkTexture *texture;
  int *sizes;
  int i;
  int scale;

  icon_theme = bobgui_icon_theme_get_for_display (priv->display);

  sizes = bobgui_icon_theme_get_icon_sizes (icon_theme, name);

  scale = bobgui_widget_get_scale_factor (BOBGUI_WIDGET (window));

  list = NULL;
  for (i = 0; sizes[i]; i++)
    {
      /* FIXME
       * We need an EWMH extension to handle scalable icons
       * by passing their name to the WM. For now just use a
       * fixed size of 48.
       */
      if (sizes[i] == -1)
        info = bobgui_icon_theme_lookup_icon (icon_theme, name, NULL,
                                           48, scale,
                                           bobgui_widget_get_direction (BOBGUI_WIDGET (window)),
                                           0);
      else
        info = bobgui_icon_theme_lookup_icon (icon_theme, name, NULL,
                                           sizes[i], scale,
                                           bobgui_widget_get_direction (BOBGUI_WIDGET (window)),
                                           0);

      texture = render_paintable_to_texture (GDK_PAINTABLE (info));
      list = g_list_insert_sorted (list, texture, (GCompareFunc) icon_size_compare);
      g_object_unref (info);
    }

  g_free (sizes);

  return list;
}

static void
bobgui_window_realize_icon (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiWindowIconInfo *info;
  GList *icon_list = NULL;

  g_return_if_fail (priv->surface != NULL);

  info = ensure_icon_info (window);

  if (info->realized)
    return;

  info->using_default_icon = FALSE;
  info->using_themed_icon = FALSE;

  /* Look up themed icon */
  if (icon_list == NULL && info->icon_name)
    {
      icon_list = icon_list_from_theme (window, info->icon_name);
      if (icon_list)
        info->using_themed_icon = TRUE;
    }

  /* Look up themed icon */
  if (icon_list == NULL && default_icon_name)
    {
      icon_list = icon_list_from_theme (window, default_icon_name);
      info->using_default_icon = TRUE;
      info->using_themed_icon = TRUE;
    }

  info->realized = TRUE;

  gdk_toplevel_set_icon_list (GDK_TOPLEVEL (priv->surface), icon_list);

  if (info->using_themed_icon)
    g_list_free_full (icon_list, g_object_unref);
}

GdkPaintable *
bobgui_window_get_icon_for_size (BobguiWindow *window,
                              int        size)
{
  const char *name;
  BobguiIconPaintable *info;
  int scale;

  name = bobgui_window_get_icon_name (window);

  if (!name)
    name = default_icon_name;
  if (!name)
    return NULL;

  scale = bobgui_widget_get_scale_factor (BOBGUI_WIDGET (window));

  info = bobgui_icon_theme_lookup_icon (bobgui_icon_theme_get_for_display (bobgui_widget_get_display (BOBGUI_WIDGET (window))),
                                     name, NULL, size, scale,
                                     bobgui_widget_get_direction (BOBGUI_WIDGET (window)),
                                     0);
  if (info == NULL)
    return NULL;

  return GDK_PAINTABLE (info);
}

static void
bobgui_window_unrealize_icon (BobguiWindow *window)
{
  BobguiWindowIconInfo *info;

  info = get_icon_info (window);

  if (info == NULL)
    return;

  /* We don't clear the properties on the window, just figure the
   * window is going away.
   */

  info->realized = FALSE;

}

static void
update_themed_icon (BobguiWindow *window)
{
  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_ICON_NAME]);

  bobgui_window_unrealize_icon (window);

  if (_bobgui_widget_get_realized (BOBGUI_WIDGET (window)))
    bobgui_window_realize_icon (window);
}

/**
 * bobgui_window_set_icon_name:
 * @window: a window
 * @name: (nullable): the name of the themed icon
 *
 * Sets the icon for the window from a named themed icon.
 *
 * See the docs for [class@Bobgui.IconTheme] for more details.
 * On some platforms, the window icon is not used at all.
 *
 * Note that this has nothing to do with the WM_ICON_NAME
 * property which is mentioned in the ICCCM.
 */
void
bobgui_window_set_icon_name (BobguiWindow  *window,
			  const char *name)
{
  BobguiWindowIconInfo *info;
  char *tmp;

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  info = ensure_icon_info (window);

  if (g_strcmp0 (info->icon_name, name) == 0)
    return;

  tmp = info->icon_name;
  info->icon_name = g_strdup (name);
  g_free (tmp);

  update_themed_icon (window);

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_ICON_NAME]);
}

/**
 * bobgui_window_get_icon_name:
 * @window: a window
 *
 * Returns the name of the themed icon for the window.
 *
 * Returns: (nullable): the icon name
 */
const char *
bobgui_window_get_icon_name (BobguiWindow *window)
{
  BobguiWindowIconInfo *info;

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), NULL);

  info = ensure_icon_info (window);

  return info->icon_name;
}

/**
 * bobgui_window_set_default_icon_name:
 * @name: the name of the themed icon
 *
 * Sets an icon to be used as fallback.
 *
 * The fallback icon is used for windows that
 * haven't had [method@Bobgui.Window.set_icon_name]
 * called on them.
 */
void
bobgui_window_set_default_icon_name (const char *name)
{
  GList *tmp_list;
  GList *toplevels;

  g_free (default_icon_name);
  default_icon_name = g_strdup (name);

  /* Update all toplevels */
  toplevels = bobgui_window_list_toplevels ();
  tmp_list = toplevels;
  while (tmp_list != NULL)
    {
      BobguiWindowIconInfo *info;
      BobguiWindow *w = tmp_list->data;

      info = get_icon_info (w);
      if (info && info->using_default_icon && info->using_themed_icon)
        {
          bobgui_window_unrealize_icon (w);
          if (_bobgui_widget_get_realized (BOBGUI_WIDGET (w)))
            bobgui_window_realize_icon (w);
        }

      tmp_list = tmp_list->next;
    }
  g_list_free (toplevels);
}

/**
 * bobgui_window_get_default_icon_name:
 *
 * Returns the fallback icon name for windows.
 *
 * The returned string is owned by BOBGUI and should not
 * be modified. It is only valid until the next call to
 * [func@Bobgui.Window.set_default_icon_name].
 *
 * Returns: (nullable): the fallback icon name for windows
 */
const char *
bobgui_window_get_default_icon_name (void)
{
  return default_icon_name;
}

static void
bobgui_window_update_csd_size (BobguiWindow *window,
                            int       *width,
                            int       *height,
                            int        apply)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiBorder window_border = { 0 };
  int w, h;

  if (!priv->decorated ||
      priv->fullscreen)
    return;

  get_shadow_width (window, &window_border);
  w = *width + apply * (window_border.left + window_border.right);
  h = *height + apply * (window_border.top + window_border.bottom);

  /* Make sure the size remains acceptable */
  if (w < 1)
    w = 1;
  if (h < 1)
    h = 1;

  /* Only update given size if not negative */
  if (*width > -1)
    *width = w;
  if (*height > -1)
    *height = h;
}

static void
bobgui_window_set_default_size_internal (BobguiWindow    *window,
                                      gboolean      change_width,
                                      int           width,
                                      gboolean      change_height,
                                      int           height)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (change_width == FALSE || width >= -1);
  g_return_if_fail (change_height == FALSE || height >= -1);

  g_object_freeze_notify (G_OBJECT (window));

  if (change_width)
    {
      if (priv->default_width != width)
        {
          priv->default_width = width;
          g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_DEFAULT_WIDTH]);
        }
    }

  if (change_height)
    {
      if (priv->default_height != height)
        {
          priv->default_height = height;
          g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_DEFAULT_HEIGHT]);
        }
    }

  g_object_thaw_notify (G_OBJECT (window));
}

/**
 * bobgui_window_set_default_size:
 * @window: a window
 * @width: width in pixels, or -1 to unset the default width
 * @height: height in pixels, or -1 to unset the default height
 *
 * Sets the default size of a window.
 *
 * The default size of a window is the size that will be used
 * if no other constraints apply.
 *
 * The default size will be updated whenever the window is resized
 * to reflect the new size, unless the window is forced to a size,
 * like when it is maximized or fullscreened.
 *
 * If the window’s minimum size request is larger than
 * the default, the default will be ignored.
 *
 * Setting the default size to a value <= 0 will cause it to be
 * ignored and the natural size request will be used instead. It
 * is possible to do this while the window is showing to "reset"
 * it to its initial size.
 *
 * Unlike [method@Bobgui.Widget.set_size_request], which sets a size
 * request for a widget and thus would keep users from shrinking
 * the window, this function only sets the initial size, just as
 * if the user had resized the window themselves. Users can still
 * shrink the window again as they normally would. Setting a default
 * size of -1 means to use the “natural” default size (the size request
 * of the window).
 *
 * If you use this function to reestablish a previously saved window size,
 * note that the appropriate size to save is the one returned by
 * [method@Bobgui.Window.get_default_size]. Using the window allocation
 * directly will not work in all circumstances and can lead to growing
 * or shrinking windows.
 */
void
bobgui_window_set_default_size (BobguiWindow   *window,
			     int          width,
			     int          height)
{
  g_return_if_fail (BOBGUI_IS_WINDOW (window));
  g_return_if_fail (width >= -1);
  g_return_if_fail (height >= -1);

  bobgui_window_set_default_size_internal (window, TRUE, width, TRUE, height);
  bobgui_widget_queue_resize (BOBGUI_WIDGET (window));
}

/**
 * bobgui_window_get_default_size:
 * @window: a window
 * @width: (out) (optional): location to store the default width
 * @height: (out) (optional): location to store the default height
 *
 * Gets the default size of the window.
 *
 * A value of 0 for the width or height indicates that a default
 * size has not been explicitly set for that dimension, so the
 * “natural” size of the window will be used.
 *
 * This function is the recommended way for [saving window state
 * across restarts of applications](https://developer.gnome.org/documentation/tutorials/save-state.html).
 */
void
bobgui_window_get_default_size (BobguiWindow *window,
			     int       *width,
			     int       *height)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  if (width != NULL)
    *width = priv->default_width;

  if (height != NULL)
    *height = priv->default_height;
}

static gboolean
bobgui_window_close_request (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (priv->hide_on_close)
    {
      bobgui_widget_set_visible (BOBGUI_WIDGET (window), FALSE);
      return TRUE;
    }

  return FALSE;
}

gboolean
bobgui_window_emit_close_request (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  gboolean handled;

  /* Avoid re-entrancy issues when calling bobgui_window_close from a
   * close-request handler */
  if (priv->in_emit_close_request)
    return TRUE;

  priv->in_emit_close_request = TRUE;
  g_signal_emit (window, window_signals[CLOSE_REQUEST], 0, &handled);
  priv->in_emit_close_request = FALSE;

  return handled;
}

static void
bobgui_window_finalize (GObject *object)
{
  BobguiWindow *window = BOBGUI_WINDOW (object);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  GdkSeat *seat;

  g_free (priv->title);
  bobgui_window_release_application (window);

  if (priv->geometry_info)
    {
      g_free (priv->geometry_info);
    }

  if (priv->keys_changed_handler)
    {
      g_source_remove (priv->keys_changed_handler);
      priv->keys_changed_handler = 0;
    }

  seat = gdk_display_get_default_seat (priv->display);
  if (seat)
    g_signal_handlers_disconnect_by_func (seat, device_removed_cb, window);

#ifdef GDK_WINDOWING_X11
  g_signal_handlers_disconnect_by_func (bobgui_settings_get_for_display (priv->display),
                                        bobgui_window_on_theme_variant_changed,
                                        window);
#endif

  g_free (priv->startup_id);

  if (priv->mnemonics_display_timeout_id)
    {
      g_source_remove (priv->mnemonics_display_timeout_id);
      priv->mnemonics_display_timeout_id = 0;
    }

  if (priv->focus_visible_timeout)
    {
      g_source_remove (priv->focus_visible_timeout);
      priv->focus_visible_timeout = 0;
    }

  g_clear_object (&priv->constraint_solver);
  g_clear_object (&priv->renderer);
  g_clear_object (&priv->resize_cursor);

  G_OBJECT_CLASS (bobgui_window_parent_class)->finalize (object);
}

static gboolean
update_csd_visibility (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  gboolean visible;

  if (priv->title_box == NULL)
    return FALSE;

  visible = !priv->fullscreen &&
            priv->decorated;

  bobgui_widget_set_child_visible (priv->title_box, visible);

  return visible;
}

static void
update_window_actions (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  gboolean is_sovereign_window = !priv->modal && !priv->transient_parent;
  GdkToplevelCapabilities capabilities;

  if (priv->surface)
    capabilities = gdk_toplevel_get_capabilities (GDK_TOPLEVEL (priv->surface));
  else
    capabilities = GDK_TOPLEVEL_CAPABILITIES_MINIMIZE |
                   GDK_TOPLEVEL_CAPABILITIES_MAXIMIZE;

  bobgui_widget_action_set_enabled (BOBGUI_WIDGET (window), "window.minimize",
                                 is_sovereign_window && (capabilities & GDK_TOPLEVEL_CAPABILITIES_MINIMIZE));
  bobgui_widget_action_set_enabled (BOBGUI_WIDGET (window), "window.toggle-maximized",
                                 is_sovereign_window && priv->resizable && (capabilities & GDK_TOPLEVEL_CAPABILITIES_MAXIMIZE));
  bobgui_widget_action_set_enabled (BOBGUI_WIDGET (window), "window.close",
                                 priv->deletable);

  update_csd_visibility (window);
}

static gboolean
bobgui_window_should_use_csd (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  const char *csd_env;

  if (!priv->decorated)
    return FALSE;

  csd_env = g_getenv ("BOBGUI_CSD");

#ifdef GDK_WINDOWING_BROADWAY
  if (GDK_IS_BROADWAY_DISPLAY (bobgui_widget_get_display (BOBGUI_WIDGET (window))))
    return TRUE;
#endif

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (bobgui_widget_get_display (BOBGUI_WIDGET (window))))
    {
      GdkDisplay *gdk_display = bobgui_widget_get_display (BOBGUI_WIDGET (window));
      return !gdk_wayland_display_prefers_ssd (gdk_display);
    }
#endif

#ifdef GDK_WINDOWING_WIN32
  if (g_strcmp0 (csd_env, "0") != 0 &&
      GDK_IS_WIN32_DISPLAY (bobgui_widget_get_display (BOBGUI_WIDGET (window))))
    return TRUE;
#endif

  return (g_strcmp0 (csd_env, "1") == 0);
}

static void
bobgui_window_show (BobguiWidget *widget)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (!g_list_store_find (toplevel_list, window, NULL))
    g_warning ("A window is shown after it has been destroyed. This will leave the window in an inconsistent state.");

  _bobgui_widget_set_visible_flag (widget, TRUE);

  bobgui_css_node_validate (bobgui_widget_get_css_node (widget));

  bobgui_widget_realize (widget);
  bobgui_widget_realize_at_context (widget);

  bobgui_window_present_toplevel (window);

  bobgui_widget_map (widget);

  if (!priv->focus_widget)
    bobgui_window_move_focus (widget, BOBGUI_DIR_TAB_FORWARD);

  if (priv->modal)
    bobgui_grab_add (widget);
}

static void
bobgui_window_hide (BobguiWidget *widget)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  _bobgui_widget_set_visible_flag (widget, FALSE);
  bobgui_widget_unmap (widget);

  if (priv->modal)
    bobgui_grab_remove (widget);
}

static GdkToplevelLayout *
bobgui_window_compute_base_layout (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  GdkToplevelLayout *layout;

  layout = gdk_toplevel_layout_new ();

  gdk_toplevel_layout_set_resizable (layout, priv->resizable);

  return layout;
}

static void
bobgui_window_present_toplevel (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  GdkToplevelLayout *layout;

  layout = bobgui_window_compute_base_layout (window);
  if (priv->is_set_maximized)
    gdk_toplevel_layout_set_maximized (layout, priv->maximized);
  if (priv->is_set_fullscreen)
    gdk_toplevel_layout_set_fullscreen (layout, priv->fullscreen,
                                        priv->initial_fullscreen_monitor);
  gdk_toplevel_present (GDK_TOPLEVEL (priv->surface), layout);
  gdk_toplevel_layout_unref (layout);
}

static void
bobgui_window_update_toplevel (BobguiWindow         *window,
                            GdkToplevelLayout *layout)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (_bobgui_widget_get_mapped (BOBGUI_WIDGET (window)))
    gdk_toplevel_present (GDK_TOPLEVEL (priv->surface), layout);
  gdk_toplevel_layout_unref (layout);
}

static void
bobgui_window_notify_startup (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (!disable_startup_notification)
    {
      /* Do we have a custom startup-notification id? */
      if (priv->startup_id != NULL)
        {
          /* Make sure we have a "real" id */
          if (!startup_id_is_fake (priv->startup_id))
            gdk_toplevel_set_startup_id (GDK_TOPLEVEL (priv->surface), priv->startup_id);

          g_free (priv->startup_id);
          priv->startup_id = NULL;
        }
      else
        gdk_toplevel_set_startup_id (GDK_TOPLEVEL (priv->surface), NULL);
    }
}

static void
bobgui_window_map (BobguiWidget *widget)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiWidget *child = priv->child;

  BOBGUI_WIDGET_CLASS (bobgui_window_parent_class)->map (widget);

  if (child != NULL && bobgui_widget_get_visible (child))
    bobgui_widget_map (child);

  if (priv->title_box != NULL &&
      bobgui_widget_get_visible (priv->title_box) &&
      bobgui_widget_get_child_visible (priv->title_box))
    bobgui_widget_map (priv->title_box);

  bobgui_widget_realize_at_context (widget);

  bobgui_window_present_toplevel (window);

  if (priv->minimize_initially)
    gdk_toplevel_minimize (GDK_TOPLEVEL (priv->surface));

  bobgui_window_set_theme_variant (window);

  if (!priv->in_present)
    bobgui_window_notify_startup (window);

  /* inherit from transient parent, so that a dialog that is
   * opened via keynav shows focus initially
   */
  if (priv->transient_parent)
    bobgui_window_set_focus_visible (window, bobgui_window_get_focus_visible (priv->transient_parent));
  else
    bobgui_window_set_focus_visible (window, FALSE);

  if (priv->application)
    bobgui_application_handle_window_map (priv->application, window);
}

static void
bobgui_window_unmap (BobguiWidget *widget)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiWidget *child = priv->child;

  BOBGUI_WIDGET_CLASS (bobgui_window_parent_class)->unmap (widget);
  gdk_surface_hide (priv->surface);

  _bobgui_window_set_is_active (window, FALSE);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (window),
                               BOBGUI_ACCESSIBLE_STATE_HIDDEN, TRUE,
                               -1);

  bobgui_widget_unrealize_at_context (widget);

  if (priv->title_box != NULL)
    bobgui_widget_unmap (priv->title_box);

  if (child != NULL)
    bobgui_widget_unmap (child);
}

static void
get_shadow_width (BobguiWindow *window,
                  BobguiBorder *shadow_width)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiCssStyle *style;

  if (!priv->decorated)
    goto out;

  if (!priv->client_decorated ||
      !priv->use_client_shadow)
    goto out;

  if (priv->maximized ||
      priv->fullscreen)
    goto out;

  style = bobgui_css_node_get_style (bobgui_widget_get_css_node (BOBGUI_WIDGET (window)));

  /* Calculate the size of the drop shadows ... */
  bobgui_css_shadow_value_get_extents (style->used->box_shadow, shadow_width);

  shadow_width->left = MAX (shadow_width->left, RESIZE_HANDLE_SIZE);
  shadow_width->top = MAX (shadow_width->top, RESIZE_HANDLE_SIZE);
  shadow_width->bottom = MAX (shadow_width->bottom, RESIZE_HANDLE_SIZE);
  shadow_width->right = MAX (shadow_width->right, RESIZE_HANDLE_SIZE);

  return;

out:
  *shadow_width = (BobguiBorder) {0, 0, 0, 0};
}

static void
update_realized_window_properties (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  GdkRectangle rect;
  BobguiCssBoxes css_boxes;
  const graphene_rect_t *border_rect;
  double native_x, native_y;

  if (!priv->client_decorated || !priv->use_client_shadow)
    return;

  bobgui_native_get_surface_transform (BOBGUI_NATIVE (window), &native_x, &native_y);

  /* update the input shape, which makes it so that clicks
   * outside the border windows go through. */
  bobgui_css_boxes_init (&css_boxes, BOBGUI_WIDGET (window));
  border_rect = bobgui_css_boxes_get_border_rect (&css_boxes);

  /* This logic is duplicated in get_edge_for_coordinates() */
  rect.x = native_x + border_rect->origin.x - RESIZE_HANDLE_SIZE;
  rect.y = native_y + border_rect->origin.y - RESIZE_HANDLE_SIZE;
  rect.width = border_rect->size.width + 2 * RESIZE_HANDLE_SIZE;
  rect.height = border_rect->size.height + 2 * RESIZE_HANDLE_SIZE;

  if (rect.width > 0 && rect.height > 0)
    {
      cairo_region_t *region = cairo_region_create_rectangle (&rect);

      gdk_surface_set_input_region (priv->surface, region);
      cairo_region_destroy (region);
    }
}

/* NB: When orientation is VERTICAL, width/height are flipped.
 * The code uses the terms nonetheless to make it more intuitive
 * to understand.
 */
static void
bobgui_window_compute_min_size (BobguiWidget      *window,
                             BobguiOrientation  orientation,
                             int             current_width,
                             int             current_height,
                             int            *min_width,
                             int            *min_height)
{
  double current_ratio = (double) current_width / current_height;
  int start_width, end_width, start_height, end_height;

  /*
   * We'd want to do
   *     bobgui_widget_measure (window, orientation, current_height,
   *                         &start_width, NULL, NULL, NULL);
   * to find the smallest acceptable width for the 'start_width' bound
   * here; but it could be expensive to measure the whole window against
   * its preferred mode.  Instead, since we're doing a binary search
   * anyway, start with the overall minimum width, and reject widths
   * that would require a larger height in the loop below.
   */
  bobgui_widget_measure (window, orientation, -1, &start_width, NULL, NULL, NULL);
  bobgui_widget_measure (window, OPPOSITE_ORIENTATION (orientation),
                      start_width, &start_height, NULL, NULL, NULL);
  end_width = current_width;
  bobgui_widget_measure (window, OPPOSITE_ORIENTATION (orientation),
                      current_width, &end_height, NULL, NULL, NULL);

  if (end_height == current_height)
    {
      /* The current height is the minimum height for this width.  Don't
       * run the search, just find the minimum width for this height. */
      *min_height = current_height;
      bobgui_widget_measure (window, orientation, current_height,
                          min_width, NULL, NULL, NULL);
      return;
    }

  /* The width we're looking for is always between start_width and
   * end_width, inclusive.  We stop the search either when we discover
   * equal heights at both ends, or when there are just two options left
   * on either sides of the current ratio.
   */
  while (start_height > end_height && start_width + 1 < end_width)
    {
      int mid_width, mid_height;
      double mid_ratio;

      mid_width = (start_width + end_width) / 2;
      bobgui_widget_measure (window, OPPOSITE_ORIENTATION (orientation),
                          mid_width, &mid_height, NULL, NULL, NULL);

      mid_ratio = (double) mid_width / mid_height;
      if (mid_ratio == current_ratio)
        {
          *min_width = mid_width;
          *min_height = mid_height;
          return;
        }
      else if (mid_ratio < current_ratio)
        {
          /* This includes the case where mid_height > current_height */
          start_width = mid_width;
          start_height = mid_height;
        }
      else
        {
          end_width = mid_width;
          end_height = mid_height;
        }
    }

  /* Between the 'start' and 'end' points, whose widths could either be
   * the same or differ by one, pick 'end' because it's guaranteed
   * to be <= 'current' for both width and height, while 'start' may
   * have a larger height.
   */

  *min_height = end_height;

  if (start_height > end_height)
    {
      /* Must have broken out of the loop because we saw start_width and
       * end_width differ by exactly one.  This means end_width must be
       * the minimum width for end_height, so no need to measure the
       * widget here again, against its preferred size request mode.
       */
      g_assert (start_width + 1 == end_width);
      *min_width = end_width;
    }
  else
    bobgui_widget_measure (window, orientation, end_height,
                        min_width, NULL, NULL, NULL);
}

static void
bobgui_window_compute_default_size (BobguiWindow *window,
                                 int        cur_width,
                                 int        cur_height,
                                 int        max_width,
                                 int        max_height,
                                 int       *min_width,
                                 int       *min_height,
                                 int       *width,
                                 int       *height)
{
  BobguiWidget *widget = BOBGUI_WIDGET (window);
  BobguiSizeRequestMode request_mode = bobgui_widget_get_request_mode (widget);

  if (request_mode == BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT)
    {
      int minimum, natural;

      bobgui_widget_measure (widget, BOBGUI_ORIENTATION_VERTICAL, -1,
                          &minimum, &natural,
                          NULL, NULL);
      *min_height = minimum;
      if (cur_height <= 0)
        cur_height = natural;
      *height = MAX (minimum, MIN (max_height, cur_height));

      bobgui_widget_measure (widget, BOBGUI_ORIENTATION_HORIZONTAL,
                          *height,
                          &minimum, &natural,
                          NULL, NULL);
      *min_width = minimum;
      if (cur_width <= 0)
        cur_width = natural;
      *width = MAX (minimum, MIN (max_width, cur_width));

      bobgui_window_compute_min_size (widget, BOBGUI_ORIENTATION_VERTICAL,
                                   *height, *width,
                                   min_height, min_width);
    }
  else /* BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH or CONSTANT_SIZE */
    {
      int minimum, natural;

      bobgui_widget_measure (widget, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                          &minimum, &natural,
                          NULL, NULL);
      *min_width = minimum;
      if (cur_width <= 0)
        cur_width = natural;
      *width = MAX (minimum, MIN (max_width, cur_width));

      bobgui_widget_measure (widget, BOBGUI_ORIENTATION_VERTICAL,
                          *width,
                          &minimum, &natural,
                          NULL, NULL);
      *min_height = minimum;
      if (cur_height <= 0)
        cur_height = natural;

      *height = MAX (minimum, MIN (max_height, cur_height));

      if (request_mode != BOBGUI_SIZE_REQUEST_CONSTANT_SIZE)
        bobgui_window_compute_min_size (widget, BOBGUI_ORIENTATION_HORIZONTAL,
                                     *width, *height,
                                     min_width, min_height);
    }
}

static gboolean
should_remember_size (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (!priv->resizable)
    return FALSE;

  return !(priv->state & (GDK_TOPLEVEL_STATE_FULLSCREEN |
                          GDK_TOPLEVEL_STATE_MAXIMIZED |
                          GDK_TOPLEVEL_STATE_TILED |
                          GDK_TOPLEVEL_STATE_TOP_TILED |
                          GDK_TOPLEVEL_STATE_RIGHT_TILED |
                          GDK_TOPLEVEL_STATE_BOTTOM_TILED |
                          GDK_TOPLEVEL_STATE_LEFT_TILED |
                          GDK_TOPLEVEL_STATE_MINIMIZED));
}

static void
toplevel_compute_size (GdkToplevel     *toplevel,
                       GdkToplevelSize *size,
                       BobguiWidget       *widget)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  int width, height;
  BobguiBorder shadow;
  int bounds_width, bounds_height;
  int min_width, min_height;

  gdk_toplevel_size_get_bounds (size, &bounds_width, &bounds_height);

  bobgui_window_compute_default_size (window,
                                   priv->default_width, priv->default_height,
                                   bounds_width, bounds_height,
                                   &min_width, &min_height,
                                   &width, &height);

  if (width < min_width)
    width = min_width;
  if (height < min_height)
    height = min_height;

  if (should_remember_size (window))
    bobgui_window_set_default_size_internal (window, TRUE, width, TRUE, height);

  bobgui_window_update_csd_size (window,
                              &width, &height,
                              INCLUDE_CSD_SIZE);
  bobgui_window_update_csd_size (window,
                              &min_width, &min_height,
                              INCLUDE_CSD_SIZE);

  gdk_toplevel_size_set_min_size (size, min_width, min_height);

  gdk_toplevel_size_set_size (size, width, height);

  if (priv->use_client_shadow)
    {
      get_shadow_width (window, &shadow);
      gdk_toplevel_size_set_shadow_width (size,
                                          shadow.left, shadow.right,
                                          shadow.top, shadow.bottom);
    }

  bobgui_widget_clear_resize_queued (widget);
}

static void
bobgui_window_realize (BobguiWidget *widget)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  GdkSurface *surface;
  GdkFrameClock *frame_clock;
  int old_scale;

  old_scale = bobgui_widget_get_scale_factor (widget);

  /* Create default title bar */
  if (!priv->client_decorated && bobgui_window_should_use_csd (window))
    {
      if (bobgui_window_is_composited (window))
        {
          priv->use_client_shadow = bobgui_window_supports_client_shadow (window);
          bobgui_window_enable_csd (window);

          if (priv->title_box == NULL)
            {
              priv->title_box = bobgui_header_bar_new ();
              bobgui_widget_add_css_class (priv->title_box, "titlebar");
              bobgui_widget_add_css_class (priv->title_box, "default-decoration");

              bobgui_widget_insert_before (priv->title_box, widget, NULL);
            }

          update_window_actions (window);
        }
      else
        priv->use_client_shadow = FALSE;
    }

  surface = gdk_surface_new_toplevel (bobgui_widget_get_display (widget));
  priv->surface = surface;
  gdk_surface_set_widget (surface, widget);

  if (priv->renderer == NULL)
    priv->renderer = gsk_renderer_new_for_surface_full (surface, TRUE);

  g_signal_connect_swapped (surface, "notify::state", G_CALLBACK (surface_state_changed), widget);
  g_signal_connect_swapped (surface, "notify::mapped", G_CALLBACK (surface_state_changed), widget);
  g_signal_connect_swapped (surface, "notify::capabilities", G_CALLBACK (update_window_actions), widget);
  g_signal_connect (surface, "render", G_CALLBACK (surface_render), widget);
  g_signal_connect (surface, "event", G_CALLBACK (surface_event), widget);
  g_signal_connect (surface, "compute-size", G_CALLBACK (toplevel_compute_size), widget);

  frame_clock = gdk_surface_get_frame_clock (surface);
  g_signal_connect (frame_clock, "after-paint", G_CALLBACK (after_paint), widget);

  BOBGUI_WIDGET_CLASS (bobgui_window_parent_class)->realize (widget);

  bobgui_root_start_layout (BOBGUI_ROOT (window));

  if (priv->transient_parent &&
      _bobgui_widget_get_realized (BOBGUI_WIDGET (priv->transient_parent)))
    {
      BobguiWindowPrivate *parent_priv = bobgui_window_get_instance_private (priv->transient_parent);
      gdk_toplevel_set_transient_for (GDK_TOPLEVEL (surface), parent_priv->surface);
    }

  if (priv->title)
    gdk_toplevel_set_title (GDK_TOPLEVEL (surface), priv->title);

  gdk_toplevel_set_decorated (GDK_TOPLEVEL (surface), priv->decorated && !priv->client_decorated);
  gdk_toplevel_set_deletable (GDK_TOPLEVEL (surface), priv->deletable);
  gdk_toplevel_set_modal (GDK_TOPLEVEL (surface), priv->modal);
  gdk_toplevel_set_gravity (GDK_TOPLEVEL (surface), get_gdk_gravity (window));

#ifdef GDK_WINDOWING_X11

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

  if (priv->startup_id)
    {
      if (GDK_IS_X11_SURFACE (surface))
        {
          guint32 timestamp = extract_time_from_startup_id (priv->startup_id);
          if (timestamp != GDK_CURRENT_TIME)
            gdk_x11_surface_set_user_time (surface, timestamp);
        }
    }

  if (priv->initial_timestamp != GDK_CURRENT_TIME)
    {
      if (GDK_IS_X11_SURFACE (surface))
        gdk_x11_surface_set_user_time (surface, priv->initial_timestamp);
    }

G_GNUC_END_IGNORE_DEPRECATIONS
#endif

  update_realized_window_properties (window);

  if (priv->application)
    bobgui_application_handle_window_realize (priv->application, window);

  bobgui_window_realize_icon (window);

  if (old_scale != bobgui_widget_get_scale_factor (widget))
    _bobgui_widget_scale_changed (widget);

  bobgui_native_realize (BOBGUI_NATIVE (window));

  update_window_actions (window);
}

static void
bobgui_window_unrealize (BobguiWidget *widget)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiWindowGeometryInfo *info;
  GdkSurface *surface;
  GdkFrameClock *frame_clock;

  bobgui_native_unrealize (BOBGUI_NATIVE (window));

  /* On unrealize, we reset the size of the window such
   * that we will re-apply the default sizing stuff
   * next time we show the window.
   *
   * Default positioning is reset on unmap, instead of unrealize.
   */
  priv->need_default_size = TRUE;
  info = bobgui_window_get_geometry_info (window, FALSE);
  if (info)
    {
      info->last.configure_request.x = 0;
      info->last.configure_request.y = 0;
      info->last.configure_request.width = -1;
      info->last.configure_request.height = -1;
      /* be sure we reset geom hints on re-realize */
      info->last.flags = 0;
    }

  gsk_renderer_unrealize (priv->renderer);

  /* Icons */
  bobgui_window_unrealize_icon (window);

  if (priv->title_box)
    bobgui_widget_unrealize (priv->title_box);

  if (priv->child)
    bobgui_widget_unrealize (priv->child);

  g_clear_object (&priv->renderer);

  surface = priv->surface;

  g_signal_handlers_disconnect_by_func (surface, surface_state_changed, widget);
  g_signal_handlers_disconnect_by_func (surface, update_window_actions, widget);
  g_signal_handlers_disconnect_by_func (surface, surface_render, widget);
  g_signal_handlers_disconnect_by_func (surface, surface_event, widget);
  g_signal_handlers_disconnect_by_func (surface, toplevel_compute_size, widget);

  frame_clock = gdk_surface_get_frame_clock (surface);

  g_signal_handlers_disconnect_by_func (frame_clock, after_paint, widget);

  bobgui_root_stop_layout (BOBGUI_ROOT (window));

  BOBGUI_WIDGET_CLASS (bobgui_window_parent_class)->unrealize (widget);

  gdk_surface_set_widget (surface, NULL);
  g_clear_pointer (&priv->surface, gdk_surface_destroy);

  priv->use_client_shadow = FALSE;
}

static inline void
add_or_remove_class (BobguiWidget  *widget,
                     gboolean    add,
                     const char *class)
{
  if (add)
    bobgui_widget_add_css_class (widget, class);
  else
    bobgui_widget_remove_css_class (widget, class);
}

static void
update_window_style_classes (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiWidget *widget = BOBGUI_WIDGET (window);
  guint constraints;

  constraints = priv->edge_constraints;

  if (!constraints)
    {
      bobgui_widget_remove_css_class (widget, "tiled-top");
      bobgui_widget_remove_css_class (widget, "tiled-right");
      bobgui_widget_remove_css_class (widget, "tiled-bottom");
      bobgui_widget_remove_css_class (widget, "tiled-left");

      add_or_remove_class (widget, priv->tiled, "tiled");
    }
  else
    {
      bobgui_widget_remove_css_class (widget, "tiled");
      add_or_remove_class (widget, constraints & GDK_TOPLEVEL_STATE_TOP_TILED, "tiled-top");
      add_or_remove_class (widget, constraints & GDK_TOPLEVEL_STATE_RIGHT_TILED, "tiled-right");
      add_or_remove_class (widget, constraints & GDK_TOPLEVEL_STATE_BOTTOM_TILED, "tiled-bottom");
      add_or_remove_class (widget, constraints & GDK_TOPLEVEL_STATE_LEFT_TILED, "tiled-left");
      add_or_remove_class (widget, constraints & GDK_TOPLEVEL_STATE_TOP_RESIZABLE, "resizable-top");
      add_or_remove_class (widget, constraints & GDK_TOPLEVEL_STATE_RIGHT_RESIZABLE, "resizable-right");
      add_or_remove_class (widget, constraints & GDK_TOPLEVEL_STATE_BOTTOM_RESIZABLE, "resizable-bottom");
      add_or_remove_class (widget, constraints & GDK_TOPLEVEL_STATE_LEFT_RESIZABLE, "resizable-left");
    }

  add_or_remove_class (widget, priv->maximized, "maximized");
  add_or_remove_class (widget, priv->fullscreen, "fullscreen");
}

/* _bobgui_window_set_allocation:
 * @window: a `BobguiWindow`
 * @width: the original width for the window
 * @height: the original height for the window
 * @allocation_out: (out): @allocation taking decorations
 * into consideration
 *
 * This function is like bobgui_widget_set_allocation()
 * but does the necessary extra work to update
 * the resize grip positioning, etc.
 *
 * Call this instead of bobgui_widget_set_allocation()
 * when overriding ::size_allocate in a BobguiWindow
 * subclass without chaining up.
 *
 * The @allocation parameter will be adjusted to
 * reflect any internal decorations that the window
 * may have. That revised allocation will then be
 * returned in the @allocation_out parameter.
 */
void
_bobgui_window_set_allocation (BobguiWindow           *window,
                            int                  width,
                            int                  height,
                            BobguiAllocation       *allocation_out)
{
  BobguiWidget *widget = (BobguiWidget *)window;
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiAllocation child_allocation;

  g_assert (allocation_out != NULL);

  child_allocation.x = 0;
  child_allocation.y = 0;
  child_allocation.width = width;
  child_allocation.height = height;

  if (_bobgui_widget_get_realized (widget))
    {
      update_realized_window_properties (window);
    }

  priv->title_height = 0;

  if (priv->title_box != NULL &&
      bobgui_widget_get_visible (priv->title_box) &&
      bobgui_widget_get_child_visible (priv->title_box) &&
      priv->decorated &&
      !priv->fullscreen)
    {
      BobguiAllocation title_allocation;

      title_allocation.x = 0;
      title_allocation.y = 0;
      title_allocation.width = width;

      bobgui_widget_measure (priv->title_box, BOBGUI_ORIENTATION_VERTICAL,
                          title_allocation.width,
                          NULL, &priv->title_height,
                          NULL, NULL);

      title_allocation.height = priv->title_height;

      bobgui_widget_size_allocate (priv->title_box, &title_allocation, -1);
    }

  if (priv->decorated &&
      !priv->fullscreen)
    {
      child_allocation.y += priv->title_height;
      child_allocation.height -= priv->title_height;
    }

  *allocation_out = child_allocation;
}

static void
bobgui_window_size_allocate (BobguiWidget *widget,
                          int        width,
                          int        height,
                          int        baseline)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiWidget *child = priv->child;
  BobguiAllocation child_allocation;

  _bobgui_window_set_allocation (window, width, height, &child_allocation);

  if (child && bobgui_widget_get_visible (child))
    bobgui_widget_size_allocate (child, &child_allocation, -1);

  bobgui_tooltip_maybe_allocate (BOBGUI_NATIVE (widget));
}

static void
update_edge_constraints (BobguiWindow      *window,
                         GdkToplevelState  state)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  priv->edge_constraints = (state & GDK_TOPLEVEL_STATE_TOP_TILED) |
                           (state & GDK_TOPLEVEL_STATE_TOP_RESIZABLE) |
                           (state & GDK_TOPLEVEL_STATE_RIGHT_TILED) |
                           (state & GDK_TOPLEVEL_STATE_RIGHT_RESIZABLE) |
                           (state & GDK_TOPLEVEL_STATE_BOTTOM_TILED) |
                           (state & GDK_TOPLEVEL_STATE_BOTTOM_RESIZABLE) |
                           (state & GDK_TOPLEVEL_STATE_LEFT_TILED) |
                           (state & GDK_TOPLEVEL_STATE_LEFT_RESIZABLE);

  priv->tiled = (state & GDK_TOPLEVEL_STATE_TILED) ? 1 : 0;
}

static void
surface_state_changed (BobguiWidget *widget)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  GdkToplevelState new_surface_state;
  GdkToplevelState changed_mask;

  new_surface_state = gdk_toplevel_get_state (GDK_TOPLEVEL (priv->surface));
  changed_mask = new_surface_state ^ priv->state;
  priv->state = new_surface_state;

  if (changed_mask & GDK_TOPLEVEL_STATE_FOCUSED)
    {
      gboolean focused = new_surface_state & GDK_TOPLEVEL_STATE_FOCUSED;

      ensure_state_flag_backdrop (widget);

      if (!focused)
        bobgui_window_set_mnemonics_visible (window, FALSE);
    }

  if (changed_mask & GDK_TOPLEVEL_STATE_FULLSCREEN)
    {
      priv->fullscreen = (new_surface_state & GDK_TOPLEVEL_STATE_FULLSCREEN) ? TRUE : FALSE;

      g_object_notify_by_pspec (G_OBJECT (widget), window_props[PROP_FULLSCREENED]);
    }

  if (changed_mask & GDK_TOPLEVEL_STATE_MAXIMIZED)
    {
      priv->maximized = (new_surface_state & GDK_TOPLEVEL_STATE_MAXIMIZED) ? TRUE : FALSE;

      g_object_notify_by_pspec (G_OBJECT (widget), window_props[PROP_MAXIMIZED]);
    }

  if (changed_mask & GDK_TOPLEVEL_STATE_SUSPENDED)
    {
      priv->suspended = (new_surface_state & GDK_TOPLEVEL_STATE_SUSPENDED) ? TRUE : FALSE;

      g_object_notify_by_pspec (G_OBJECT (widget), window_props[PROP_SUSPENDED]);
    }

  update_edge_constraints (window, new_surface_state);

  if (changed_mask & (GDK_TOPLEVEL_STATE_FULLSCREEN |
                      GDK_TOPLEVEL_STATE_MAXIMIZED |
                      GDK_TOPLEVEL_STATE_TILED |
                      GDK_TOPLEVEL_STATE_TOP_TILED |
                      GDK_TOPLEVEL_STATE_RIGHT_TILED |
                      GDK_TOPLEVEL_STATE_BOTTOM_TILED |
                      GDK_TOPLEVEL_STATE_LEFT_TILED |
                      GDK_TOPLEVEL_STATE_MINIMIZED))
    {
      update_window_style_classes (window);
      update_window_actions (window);
      bobgui_widget_queue_resize (widget);
    }
}

static void
surface_size_changed (BobguiWidget *widget,
                      int        width,
                      int        height)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);

  if (should_remember_size (window))
    {
      int width_to_remember;
      int height_to_remember;

      width_to_remember = width;
      height_to_remember = height;
      bobgui_window_update_csd_size (window,
                                  &width_to_remember, &height_to_remember,
                                  EXCLUDE_CSD_SIZE);
      bobgui_window_set_default_size_internal (window,
                                            TRUE, width_to_remember,
                                            TRUE, height_to_remember);
    }

  bobgui_widget_queue_allocate (widget);
}

static void
maybe_unset_focus_and_default (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (priv->move_focus)
    {
      BobguiWidget *parent;

      parent = _bobgui_widget_get_parent (priv->move_focus_widget);

      while (parent)
        {
          if (_bobgui_widget_get_visible (parent))
            {
              if (bobgui_widget_grab_focus (parent))
                break;
            }

          parent = _bobgui_widget_get_parent (parent);
        }

      if (!parent)
        bobgui_widget_child_focus (BOBGUI_WIDGET (window), BOBGUI_DIR_TAB_FORWARD);

      priv->move_focus = FALSE;
      g_clear_object (&priv->move_focus_widget);
    }

  if (priv->unset_default)
    bobgui_window_set_default_widget (window, NULL);
}

static gboolean
surface_render (GdkSurface     *surface,
                cairo_region_t *region,
                BobguiWidget      *widget)
{
  bobgui_widget_render (widget, surface, region);

  return TRUE;
}

static void
after_paint (GdkFrameClock *clock,
             BobguiWindow     *window)
{
  maybe_unset_focus_and_default (window);
}

static gboolean
surface_event (GdkSurface *surface,
               GdkEvent   *event,
               BobguiWidget  *widget)
{
  return bobgui_main_do_event (event);
}

static void
bobgui_window_real_activate_focus (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (priv->focus_widget && bobgui_widget_is_sensitive (priv->focus_widget))
    bobgui_widget_activate (priv->focus_widget);
}

static gboolean
bobgui_window_has_mnemonic_modifier_pressed (BobguiWindow *window)
{
  GList *seats, *s;
  gboolean retval = FALSE;

  seats = gdk_display_list_seats (bobgui_widget_get_display (BOBGUI_WIDGET (window)));

  for (s = seats; s; s = s->next)
    {
      GdkDevice *dev = gdk_seat_get_keyboard (s->data);
      GdkModifierType mask;

      mask = gdk_device_get_modifier_state (dev);
      if ((mask & bobgui_accelerator_get_default_mod_mask ()) == GDK_ALT_MASK)
        {
          retval = TRUE;
          break;
        }
    }

  g_list_free (seats);

  return retval;
}

static gboolean
bobgui_window_handle_focus (BobguiWidget *widget,
                         GdkEvent  *event,
                         double     x,
                         double     y)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);

  if (gdk_event_get_event_type (event) != GDK_FOCUS_CHANGE)
    return FALSE;

  if (gdk_focus_event_get_in (event))
    {
      _bobgui_window_set_is_active (window, TRUE);

      if (bobgui_window_has_mnemonic_modifier_pressed (window))
        _bobgui_window_schedule_mnemonics_visible (window);
    }
  else
    {
      _bobgui_window_set_is_active (window, FALSE);

      bobgui_window_set_mnemonics_visible (window, FALSE);
    }

  return TRUE;
}

static void
update_mnemonics_visible (BobguiWindow       *window,
                          guint            keyval,
                          GdkModifierType  state,
                          gboolean         visible)
{
  if ((keyval == GDK_KEY_Alt_L || keyval == GDK_KEY_Alt_R) &&
      ((state & (bobgui_accelerator_get_default_mod_mask ()) & ~(GDK_ALT_MASK)) == 0))
    {
      if (visible)
        _bobgui_window_schedule_mnemonics_visible (window);
      else
        bobgui_window_set_mnemonics_visible (window, FALSE);
    }
}

void
_bobgui_window_update_focus_visible (BobguiWindow       *window,
                                  guint            keyval,
                                  GdkModifierType  state,
                                  gboolean         visible)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (visible)
    {
      if (priv->focus_visible)
        priv->key_press_focus = NULL;
      else
        priv->key_press_focus = priv->focus_widget;

      if ((keyval == GDK_KEY_Alt_L || keyval == GDK_KEY_Alt_R) &&
               ((state & (bobgui_accelerator_get_default_mod_mask ()) & ~(GDK_ALT_MASK)) == 0))
        bobgui_window_set_focus_visible (window, TRUE);
    }
  else
    {
      if (priv->key_press_focus == priv->focus_widget)
        bobgui_window_set_focus_visible (window, FALSE);
      else
        bobgui_window_set_focus_visible (window, TRUE);

      priv->key_press_focus = NULL;
    }
}

static gboolean
bobgui_window_key_pressed (BobguiWidget       *widget,
                        guint            keyval,
                        guint            keycode,
                        GdkModifierType  state,
                        gpointer         data)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);

  _bobgui_window_update_focus_visible (window, keyval, state, TRUE);
  update_mnemonics_visible (window, keyval, state, TRUE);

  return FALSE;
}

static gboolean
bobgui_window_key_released (BobguiWidget       *widget,
                         guint            keyval,
                         guint            keycode,
                         GdkModifierType  state,
                         gpointer         data)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);

  _bobgui_window_update_focus_visible (window, keyval, state, FALSE);
  update_mnemonics_visible (window, keyval, state, FALSE);

  return FALSE;
}

static gboolean
bobgui_window_focus (BobguiWidget        *widget,
                  BobguiDirectionType  direction)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiWidget *child;
  BobguiWidget *old_focus_child;
  BobguiWidget *parent;

  old_focus_child = bobgui_widget_get_focus_child (widget);

  /* We need a special implementation here to deal properly with wrapping
   * around in the tab chain without the danger of going into an
   * infinite loop.
   */
  if (old_focus_child)
    {
      if (bobgui_widget_child_focus (old_focus_child, direction))
        return TRUE;
    }

  if (priv->focus_widget)
    {
      if (direction == BOBGUI_DIR_LEFT ||
          direction == BOBGUI_DIR_RIGHT ||
          direction == BOBGUI_DIR_UP ||
          direction == BOBGUI_DIR_DOWN)
        {
          return FALSE;
        }

      /* Wrapped off the end, clear the focus setting for the toplpevel */
      parent = _bobgui_widget_get_parent (priv->focus_widget);
      while (parent)
        {
          bobgui_widget_set_focus_child (parent, NULL);
          parent = _bobgui_widget_get_parent (parent);
        }

      bobgui_window_set_focus (window, NULL);
    }

  /* Now try to focus the first widget in the window,
   * taking care to hook titlebar widgets into the
   * focus chain.
  */
  if (priv->title_box != NULL &&
      old_focus_child != NULL &&
      priv->title_box != old_focus_child)
    child = priv->title_box;
  else
    child = priv->child;

  if (child)
    {
      if (bobgui_widget_child_focus (child, direction))
        return TRUE;
      else if (priv->title_box != NULL &&
               priv->title_box != child &&
               bobgui_widget_child_focus (priv->title_box, direction))
        return TRUE;
      else if (priv->title_box == child &&
               bobgui_widget_child_focus (priv->child, direction))
        return TRUE;
    }

  return FALSE;
}

static void
bobgui_window_move_focus (BobguiWidget        *widget,
                       BobguiDirectionType  dir)
{
  bobgui_widget_child_focus (widget, dir);

  if (!bobgui_widget_get_focus_child (widget))
    bobgui_window_set_focus (BOBGUI_WINDOW (widget), NULL);
}

void
check_crossing_invariants (BobguiWidget       *widget,
                           BobguiCrossingData *crossing)
{
#ifdef G_ENABLE_DEBUG
  if (crossing->old_target == NULL)
    {
      g_assert (crossing->old_descendent == NULL);
    }
  else if (crossing->old_descendent == NULL)
    {
      g_assert (crossing->old_target == widget || !bobgui_widget_is_ancestor (crossing->old_target, widget));
    }
  else
    {
      g_assert (bobgui_widget_get_parent (crossing->old_descendent) == widget);
      g_assert (crossing->old_target == crossing->old_descendent || bobgui_widget_is_ancestor (crossing->old_target, crossing->old_descendent));
    }
  if (crossing->new_target == NULL)
    {
      g_assert (crossing->new_descendent == NULL);
    }
  else if (crossing->new_descendent == NULL)
    {
      g_assert (crossing->new_target == widget || !bobgui_widget_is_ancestor (crossing->new_target, widget));
    }
  else
    {
      g_assert (bobgui_widget_get_parent (crossing->new_descendent) == widget);
      g_assert (crossing->new_target == crossing->new_descendent || bobgui_widget_is_ancestor (crossing->new_target, crossing->new_descendent));
    }
#endif
}

static void
synthesize_focus_change_events (BobguiWindow       *window,
                                BobguiWidget       *old_focus,
                                BobguiWidget       *new_focus,
                                BobguiCrossingType  type)
{
  BobguiCrossingData crossing;
  BobguiWidget *ancestor;
  BobguiWidget *widget, *focus_child;
  BobguiStateFlags flags;
  BobguiWidget *prev;
  gboolean seen_ancestor;
  BobguiWidgetStack focus_array;
  int i;

  if (old_focus == new_focus)
    return;

  if (old_focus && new_focus)
    ancestor = bobgui_widget_common_ancestor (old_focus, new_focus);
  else
    ancestor = NULL;

  flags = BOBGUI_STATE_FLAG_FOCUSED | BOBGUI_STATE_FLAG_FOCUS_WITHIN;
  if (bobgui_window_get_focus_visible (BOBGUI_WINDOW (window)))
    flags |= BOBGUI_STATE_FLAG_FOCUS_VISIBLE;

  crossing.type = type;
  crossing.mode = GDK_CROSSING_NORMAL;
  crossing.old_target = old_focus ? g_object_ref (old_focus) : NULL;
  crossing.old_descendent = NULL;
  crossing.new_target = new_focus ? g_object_ref (new_focus) : NULL;
  crossing.new_descendent = NULL;

  crossing.direction = BOBGUI_CROSSING_OUT;

  prev = NULL;
  seen_ancestor = FALSE;
  widget = old_focus;
  while (widget)
    {
      crossing.old_descendent = prev;
      if (seen_ancestor)
        {
          crossing.new_descendent = new_focus ? prev : NULL;
        }
      else if (widget == ancestor)
        {
          BobguiWidget *w;

          crossing.new_descendent = NULL;
          for (w = new_focus; w != ancestor; w = bobgui_widget_get_parent (w))
            crossing.new_descendent = w;

          seen_ancestor = TRUE;
        }
      else
        {
          crossing.new_descendent = NULL;
        }

      check_crossing_invariants (widget, &crossing);
      bobgui_widget_handle_crossing (widget, &crossing, 0, 0);
      bobgui_widget_unset_state_flags (widget, flags);
      bobgui_widget_set_focus_child (widget, NULL);
      prev = widget;
      widget = bobgui_widget_get_parent (widget);

      flags = flags & ~BOBGUI_STATE_FLAG_FOCUSED;
    }

  flags = BOBGUI_STATE_FLAG_FOCUS_WITHIN;
  if (bobgui_window_get_focus_visible (BOBGUI_WINDOW (window)))
    flags |= BOBGUI_STATE_FLAG_FOCUS_VISIBLE;

  bobgui_widget_stack_init (&focus_array);
  for (widget = new_focus; widget; widget = _bobgui_widget_get_parent (widget))
    bobgui_widget_stack_append (&focus_array, g_object_ref (widget));

  crossing.direction = BOBGUI_CROSSING_IN;

  seen_ancestor = FALSE;
  for (i = bobgui_widget_stack_get_size (&focus_array) - 1; i >= 0; i--)
    {
      widget = bobgui_widget_stack_get (&focus_array, i);

      if (i > 0)
        focus_child = bobgui_widget_stack_get (&focus_array, i - 1);
      else
        focus_child = NULL;

      crossing.new_descendent = focus_child;
      if (seen_ancestor)
        {
          crossing.old_descendent = NULL;
        }
      else if (widget == ancestor)
        {
          BobguiWidget *w;

          crossing.old_descendent = NULL;
          for (w = old_focus; w != ancestor; w = bobgui_widget_get_parent (w))
            {
              crossing.old_descendent = w;
            }

          seen_ancestor = TRUE;
        }
      else
        {
          crossing.old_descendent = (old_focus && ancestor) ? focus_child : NULL;
        }

      check_crossing_invariants (widget, &crossing);
      bobgui_widget_handle_crossing (widget, &crossing, 0, 0);

      if (i == 0)
        flags = flags | BOBGUI_STATE_FLAG_FOCUSED;

      bobgui_widget_set_state_flags (widget, flags, FALSE);
      bobgui_widget_set_focus_child (widget, focus_child);
    }

  g_clear_object (&crossing.old_target);
  g_clear_object (&crossing.new_target);

  bobgui_widget_stack_clear (&focus_array);
}

/**
 * bobgui_window_set_focus: (set-property focus-widget)
 * @window: a window
 * @focus: (nullable): the new focus widget
 *
 * Sets the focus widget.
 *
 * If @focus is not the current focus widget, and is focusable,
 * sets it as the focus widget for the window. If @focus is %NULL,
 * unsets the focus widget for this window. To set the focus to a
 * particular widget in the toplevel, it is usually more convenient
 * to use [method@Bobgui.Widget.grab_focus] instead of this function.
 */
void
bobgui_window_set_focus (BobguiWindow *window,
                      BobguiWidget *focus)
{
  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  if (focus)
    bobgui_widget_grab_focus (focus);
  else
    bobgui_window_root_set_focus (BOBGUI_ROOT (window), NULL);
}

/*
 * _bobgui_window_unset_focus_and_default:
 * @window: a window
 * @widget: a widget inside of @window
 *
 * Checks whether the focus and default widgets of @window are
 * @widget or a descendent of @widget, and if so, unset them.
 */
void
_bobgui_window_unset_focus_and_default (BobguiWindow *window,
                                     BobguiWidget *widget)

{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiWidget *child;

  child = priv->focus_widget;
  if (child && (child == widget || bobgui_widget_is_ancestor (child, widget)))
    {
      priv->move_focus_widget = g_object_ref (widget);
      priv->move_focus = TRUE;
    }

  child = priv->default_widget;
  if (child && (child == widget || bobgui_widget_is_ancestor (child, widget)))
    priv->unset_default = TRUE;

  if ((priv->move_focus || priv->unset_default) &&
      priv->surface != NULL)
    {
      GdkFrameClock *frame_clock;

      frame_clock = gdk_surface_get_frame_clock (priv->surface);
      gdk_frame_clock_request_phase (frame_clock,
                                     GDK_FRAME_CLOCK_PHASE_AFTER_PAINT);
    }
}

#undef INCLUDE_CSD_SIZE
#undef EXCLUDE_CSD_SIZE

static void
_bobgui_window_present (BobguiWindow *window,
                     guint32    timestamp)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiWidget *widget = BOBGUI_WIDGET (window);

  if (bobgui_widget_get_visible (widget))
    {
      /* Translate a timestamp of GDK_CURRENT_TIME appropriately */
      if (timestamp == GDK_CURRENT_TIME)
        {
#ifdef GDK_WINDOWING_X11

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

          if (GDK_IS_X11_SURFACE (priv->surface))
            {
              GdkDisplay *display = bobgui_widget_get_display (widget);
              timestamp = gdk_x11_display_get_user_time (display);
            }
          else

G_GNUC_END_IGNORE_DEPRECATIONS

#endif
            timestamp = bobgui_get_current_event_time ();
        }
    }
  else
    {
      priv->initial_timestamp = timestamp;
      priv->in_present = TRUE;
      bobgui_widget_set_visible (widget, TRUE);
      priv->in_present = FALSE;
    }

  gdk_toplevel_focus (GDK_TOPLEVEL (priv->surface), timestamp);
  bobgui_window_notify_startup (window);
}

/**
 * bobgui_window_set_startup_id:
 * @window: a window
 * @startup_id: a string with startup-notification identifier
 *
 * Sets the startup notification ID.
 *
 * Startup notification identifiers are used by desktop environment
 * to track application startup, to provide user feedback and other
 * features. This function changes the corresponding property on the
 * underlying `GdkSurface`.
 *
 * Normally, startup identifier is managed automatically and you should
 * only use this function in special cases like transferring focus from
 * other processes. You should use this function before calling
 * [method@Bobgui.Window.present] or any equivalent function generating
 * a window map event.
 *
 * This function is only useful on Wayland or X11, not with other GDK
 * backends.
 */
void
bobgui_window_set_startup_id (BobguiWindow   *window,
                           const char *startup_id)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiWidget *widget;

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  widget = BOBGUI_WIDGET (window);

  g_free (priv->startup_id);
  priv->startup_id = g_strdup (startup_id);

  if (_bobgui_widget_get_realized (widget))
    {
      guint32 timestamp = extract_time_from_startup_id (priv->startup_id);

#ifdef GDK_WINDOWING_X11

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

      if (timestamp != GDK_CURRENT_TIME && GDK_IS_X11_SURFACE (priv->surface))
        gdk_x11_surface_set_user_time (priv->surface, timestamp);

G_GNUC_END_IGNORE_DEPRECATIONS

#endif

      /* Here we differentiate real and "fake" startup notification IDs,
       * constructed on purpose just to pass interaction timestamp
       */
      if (startup_id_is_fake (priv->startup_id))
        _bobgui_window_present (window, timestamp);
      else
        {
          /* If window is mapped, terminate the startup-notification */
          if (_bobgui_widget_get_mapped (widget) && !disable_startup_notification)
            gdk_toplevel_set_startup_id (GDK_TOPLEVEL (priv->surface), priv->startup_id);
        }
    }

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_STARTUP_ID]);
}

/**
 * bobgui_window_present:
 * @window: a window
 *
 * Presents a window to the user.
 *
 * This may mean raising the window in the stacking order,
 * unminimizing it, moving it to the current desktop and/or
 * giving it the keyboard focus (possibly dependent on the user’s
 * platform, window manager and preferences).
 *
 * If @window is hidden, this function also makes it visible.
 */
void
bobgui_window_present (BobguiWindow *window)
{
  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  _bobgui_window_present (window, GDK_CURRENT_TIME);
}

/**
 * bobgui_window_present_with_time:
 * @window: a window
 * @timestamp: the timestamp of the user interaction (typically a
 *   button or key press event) which triggered this call
 *
 * Presents a window to the user in response to an user interaction.
 *
 * See [method@Bobgui.Window.present] for more details.
 *
 * The timestamp should be gathered when the window was requested
 * to be shown (when clicking a link for example), rather than once
 * the window is ready to be shown.
 *
 * Deprecated: 4.14: Use [method@Bobgui.Window.present]
 */
void
bobgui_window_present_with_time (BobguiWindow *window,
                              guint32    timestamp)
{
  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  _bobgui_window_present (window, timestamp);
}

/**
 * bobgui_window_minimize:
 * @window: a window
 *
 * Asks to minimize the window.
 *
 * Note that you shouldn’t assume the window is definitely minimized
 * afterward, because the windowing system might not support this
 * functionality; other entities (e.g. the user or the window manager)
 * could unminimize it again, or there may not be a window manager in
 * which case minimization isn’t possible, etc.
 *
 * It’s permitted to call this function before showing a window,
 * in which case the window will be minimized before it ever appears
 * onscreen.
 *
 * You can track result of this operation via the
 * [property@Gdk.Toplevel:state] property.
 */
void
bobgui_window_minimize (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  priv->minimize_initially = TRUE;

  if (priv->surface)
    gdk_toplevel_minimize (GDK_TOPLEVEL (priv->surface));
}

/**
 * bobgui_window_unminimize:
 * @window: a window
 *
 * Asks to unminimize the window.
 *
 * Note that you shouldn’t assume the window is definitely unminimized
 * afterward, because the windowing system might not support this
 * functionality; other entities (e.g. the user or the window manager)
 * could minimize it again, or there may not be a window manager in
 * which case minimization isn’t possible, etc.
 *
 * You can track result of this operation via the
 * [property@Gdk.Toplevel:state] property.
 */
void
bobgui_window_unminimize (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  priv->minimize_initially = FALSE;

  bobgui_window_update_toplevel (window,
                              bobgui_window_compute_base_layout (window));
}

/**
 * bobgui_window_maximize:
 * @window: a window
 *
 * Asks to maximize the window, so that it fills the screen.
 *
 * Note that you shouldn’t assume the window is definitely maximized
 * afterward, because other entities (e.g. the user or window manager)
 * could unmaximize it again, and not all window managers support
 * maximization.
 *
 * It’s permitted to call this function before showing a window,
 * in which case the window will be maximized when it appears onscreen
 * initially.
 *
 * If a window is not explicitly maximized or unmaximized before it is
 * shown, the initial state is at the window managers discretion. For
 * example, it might decide to maximize a window that almost fills the
 * screen.
 *
 * You can track the result of this operation via the
 * [property@Gdk.Toplevel:state] property, or by listening to
 * notifications on the [property@Bobgui.Window:maximized]
 * property.
 */
void
bobgui_window_maximize (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  priv->is_set_maximized = TRUE;
  if (_bobgui_widget_get_mapped (BOBGUI_WIDGET (window)))
    {
      GdkToplevelLayout *layout;

      layout = bobgui_window_compute_base_layout (window);
      gdk_toplevel_layout_set_maximized (layout, TRUE);
      bobgui_window_update_toplevel (window, layout);
    }
  else if (!priv->maximized)
    {
      priv->maximized = TRUE;
      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_MAXIMIZED]);
    }
}

/**
 * bobgui_window_unmaximize:
 * @window: a window
 *
 * Asks to unmaximize the window.
 *
 * Note that you shouldn’t assume the window is definitely unmaximized
 * afterward, because other entities (e.g. the user or window manager)
 * maximize it again, and not all window managers honor requests to
 * unmaximize.
 *
 * If a window is not explicitly maximized or unmaximized before it is
 * shown, the initial state is at the window managers discretion. For
 * example, it might decide to maximize a window that almost fills the
 * screen.
 *
 * You can track the result of this operation via the
 * [property@Gdk.Toplevel:state] property, or by listening to
 * notifications on the [property@Bobgui.Window:maximized] property.
 */
void
bobgui_window_unmaximize (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  priv->is_set_maximized = TRUE;
  if (_bobgui_widget_get_mapped (BOBGUI_WIDGET (window)))
    {
      GdkToplevelLayout *layout;

      layout = bobgui_window_compute_base_layout (window);
      gdk_toplevel_layout_set_maximized (layout, FALSE);
      bobgui_window_update_toplevel (window, layout);
    }
  else if (priv->maximized)
    {
      priv->maximized = FALSE;
      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_MAXIMIZED]);
    }
}

static void
unset_fullscreen_monitor (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (priv->initial_fullscreen_monitor)
    {
      g_signal_handlers_disconnect_by_func (priv->initial_fullscreen_monitor, unset_fullscreen_monitor, window);
      g_object_unref (priv->initial_fullscreen_monitor);
      priv->initial_fullscreen_monitor = NULL;
    }
}

/**
 * bobgui_window_fullscreen:
 * @window: a window
 *
 * Asks to place the window in the fullscreen state.
 *
 * Note that you shouldn’t assume the window is definitely fullscreen
 * afterward, because other entities (e.g. the user or window manager)
 * unfullscreen it again, and not all window managers honor requests
 * to fullscreen windows.
 *
 * If a window is not explicitly fullscreened or unfullscreened before
 * it is shown, the initial state is at the window managers discretion.
 *
 * You can track the result of this operation via the
 * [property@Gdk.Toplevel:state] property, or by listening to
 * notifications of the [property@Bobgui.Window:fullscreened] property.
 */
void
bobgui_window_fullscreen (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  unset_fullscreen_monitor (window);

  priv->is_set_fullscreen = TRUE;
  if (_bobgui_widget_get_mapped (BOBGUI_WIDGET (window)))
    {
      GdkToplevelLayout *layout;

      layout = bobgui_window_compute_base_layout (window);
      gdk_toplevel_layout_set_fullscreen (layout, TRUE, NULL);
      bobgui_window_update_toplevel (window, layout);
    }
  else if (!priv->fullscreen)
    {
      priv->fullscreen = TRUE;
      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_FULLSCREENED]);
    }
}

/**
 * bobgui_window_fullscreen_on_monitor:
 * @window: a window
 * @monitor: which monitor to go fullscreen on
 *
 * Asks to place the window in the fullscreen state on the given monitor.
 *
 * Note that you shouldn't assume the window is definitely fullscreen
 * afterward, or that the windowing system allows fullscreen windows on
 * any given monitor.
 *
 * You can track the result of this operation via the
 * [property@Gdk.Toplevel:state] property, or by listening to
 * notifications of the [property@Bobgui.Window:fullscreened] property.
 */
void
bobgui_window_fullscreen_on_monitor (BobguiWindow  *window,
                                  GdkMonitor *monitor)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (BOBGUI_IS_WINDOW (window));
  g_return_if_fail (GDK_IS_MONITOR (monitor));
  g_return_if_fail (gdk_monitor_is_valid (monitor));

  bobgui_window_set_display (window, gdk_monitor_get_display (monitor));

  unset_fullscreen_monitor (window);
  priv->initial_fullscreen_monitor = monitor;
  g_signal_connect_swapped (priv->initial_fullscreen_monitor, "invalidate",
                            G_CALLBACK (unset_fullscreen_monitor), window);
  g_object_ref (priv->initial_fullscreen_monitor);

  priv->is_set_fullscreen = TRUE;
  if (_bobgui_widget_get_mapped (BOBGUI_WIDGET (window)))
    {
      GdkToplevelLayout *layout;

      layout = bobgui_window_compute_base_layout (window);
      gdk_toplevel_layout_set_fullscreen (layout, TRUE, monitor);
      bobgui_window_update_toplevel (window, layout);
    }
  else if (!priv->fullscreen)
    {
      priv->fullscreen = TRUE;
      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_FULLSCREENED]);
    }
}

/**
 * bobgui_window_unfullscreen:
 * @window: a window
 *
 * Asks to remove the fullscreen state for the window, and return to
 * its previous state.
 *
 * Note that you shouldn’t assume the window is definitely not
 * fullscreen afterward, because other entities (e.g. the user or
 * window manager) could fullscreen it again, and not all window
 * managers honor requests to unfullscreen windows; normally the
 * window will end up restored to its normal state. Just don’t
 * write code that crashes if not.
 *
 * If a window is not explicitly fullscreened or unfullscreened before
 * it is shown, the initial state is at the window managers discretion.
 *
 * You can track the result of this operation via the
 * [property@Gdk.Toplevel:state] property, or by listening to
 * notifications of the [property@Bobgui.Window:fullscreened] property.
 */
void
bobgui_window_unfullscreen (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  unset_fullscreen_monitor (window);

  priv->is_set_fullscreen = TRUE;
  if (_bobgui_widget_get_mapped (BOBGUI_WIDGET (window)))
    {
      GdkToplevelLayout *layout;

      layout = bobgui_window_compute_base_layout (window);
      gdk_toplevel_layout_set_fullscreen (layout, FALSE, NULL);
      bobgui_window_update_toplevel (window, layout);
    }
  else if (priv->fullscreen)
    {
      priv->fullscreen = FALSE;
      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_FULLSCREENED]);
    }
}

/**
 * bobgui_window_set_resizable:
 * @window: a window
 * @resizable: true if the user can resize this window
 *
 * Sets whether the user can resize a window.
 *
 * Windows are user resizable by default.
 **/
void
bobgui_window_set_resizable (BobguiWindow *window,
                          gboolean   resizable)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  resizable = (resizable != FALSE);

  if (priv->resizable != resizable)
    {
      priv->resizable = resizable;

      update_window_actions (window);

      bobgui_window_update_toplevel (window,
                                  bobgui_window_compute_base_layout (window));

      bobgui_widget_queue_resize (BOBGUI_WIDGET (window));

      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_RESIZABLE]);
    }
}

/**
 * bobgui_window_get_resizable:
 * @window: a window
 *
 * Gets whether the user can resize the window.
 *
 * Returns: true if the user can resize the window
 **/
gboolean
bobgui_window_get_resizable (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), FALSE);

  return priv->resizable;
}

/**
 * bobgui_window_set_display:
 * @window: a window
 * @display: a display
 *
 * Sets the display where the window is displayed.
 *
 * If the window is already mapped, it will be unmapped,
 * and then remapped on the new display.
 */
void
bobgui_window_set_display (BobguiWindow  *window,
		        GdkDisplay *display)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiWidget *widget;
  gboolean was_mapped;
  int old_scale;

  g_return_if_fail (BOBGUI_IS_WINDOW (window));
  g_return_if_fail (GDK_IS_DISPLAY (display));

  if (display == priv->display)
    return;

  old_scale = bobgui_widget_get_scale_factor (BOBGUI_WIDGET (window));

  /* reset initial_fullscreen_monitor since they are relative to the screen */
  unset_fullscreen_monitor (window);

  widget = BOBGUI_WIDGET (window);

  was_mapped = _bobgui_widget_get_mapped (widget);

  if (was_mapped)
    bobgui_widget_unmap (widget);
  if (_bobgui_widget_get_realized (widget))
    bobgui_widget_unrealize (widget);

  if (priv->transient_parent && bobgui_widget_get_display (BOBGUI_WIDGET (priv->transient_parent)) != display)
    bobgui_window_set_transient_for (window, NULL);

#ifdef GDK_WINDOWING_X11
  g_signal_handlers_disconnect_by_func (bobgui_settings_get_for_display (priv->display),
                                        bobgui_window_on_theme_variant_changed, window);
  g_signal_connect (bobgui_settings_get_for_display (display),
                    "notify::bobgui-application-prefer-dark-theme",
                    G_CALLBACK (bobgui_window_on_theme_variant_changed), window);
#endif

  bobgui_widget_unroot (widget);
  priv->display = display;

  bobgui_widget_root (widget);

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_DISPLAY]);

  if (was_mapped)
    bobgui_widget_map (widget);

  if (old_scale != bobgui_widget_get_scale_factor (BOBGUI_WIDGET (window)))
    _bobgui_widget_scale_changed (BOBGUI_WIDGET (window));

  bobgui_widget_system_setting_changed (BOBGUI_WIDGET (window), BOBGUI_SYSTEM_SETTING_DISPLAY);
}

static void
bobgui_window_set_theme_variant (BobguiWindow *window)
{
#ifdef GDK_WINDOWING_X11
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  gboolean   dark_theme_requested;

  g_object_get (bobgui_settings_get_for_display (priv->display),
                "bobgui-application-prefer-dark-theme", &dark_theme_requested,
                NULL);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

  if (GDK_IS_X11_SURFACE (priv->surface))
    gdk_x11_surface_set_theme_variant (priv->surface,
                                       dark_theme_requested ? "dark" : NULL);

G_GNUC_END_IGNORE_DEPRECATIONS

#endif
}

#ifdef GDK_WINDOWING_X11
static void
bobgui_window_on_theme_variant_changed (BobguiSettings *settings,
                                     GParamSpec  *pspec,
                                     BobguiWindow   *window)
{
  bobgui_window_set_theme_variant (window);
}
#endif

/**
 * bobgui_window_is_active:
 * @window: a window
 *
 * Returns whether the window is part of the current active toplevel.
 *
 * The active toplevel is the window receiving keystrokes.
 *
 * The return value is %TRUE if the window is active toplevel itself.
 * You might use this function if you wanted to draw a widget
 * differently in an active window from a widget in an inactive window.
 *
 * Returns: true if the window part of the current active window.
 */
gboolean
bobgui_window_is_active (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), FALSE);

  return priv->is_active;
}

/**
 * bobgui_window_get_group:
 * @window: (nullable): a window
 *
 * Returns the group for the window.
 *
 * If the window has no group, then the default group is returned.
 *
 * Returns: (transfer none): the window group for @window
 *   or the default group
 */
BobguiWindowGroup *
bobgui_window_get_group (BobguiWindow *window)
{
  static BobguiWindowGroup *default_group = NULL;

  if (window)
    {
      BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

      if (priv->group)
        return priv->group;
    }

  if (!default_group)
    default_group = bobgui_window_group_new ();

  return default_group;
}

/**
 * bobgui_window_has_group:
 * @window: a window
 *
 * Returns whether the window has an explicit window group.
 *
 * Returns: true if @window has an explicit window group
 */
gboolean
bobgui_window_has_group (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), FALSE);

  return priv->group != NULL;
}

BobguiWindowGroup *
_bobgui_window_get_window_group (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  return priv->group;
}

void
_bobgui_window_set_window_group (BobguiWindow      *window,
                              BobguiWindowGroup *group)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  priv->group = group;
}

static gboolean
bobgui_window_activate_menubar (BobguiWidget *widget,
                             GVariant  *args,
                             gpointer   unused)
{
  BobguiWindow *window = BOBGUI_WINDOW (widget);
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  GList *tmp_menubars, *l;
  GPtrArray *menubars;
  BobguiWidget *focus;
  BobguiWidget *first;

  tmp_menubars = bobgui_popover_menu_bar_get_viewable_menu_bars (window);
  if (tmp_menubars == NULL)
    {
      focus = bobgui_window_get_focus (window);
      return priv->title_box != NULL &&
             (focus == NULL || !bobgui_widget_is_ancestor (focus, priv->title_box)) &&
             bobgui_widget_child_focus (priv->title_box, BOBGUI_DIR_TAB_FORWARD);
    }

  menubars = g_ptr_array_sized_new (g_list_length (tmp_menubars));;
  for (l = tmp_menubars; l; l = l->next)
    g_ptr_array_add (menubars, l->data);

  g_list_free (tmp_menubars);

  bobgui_widget_focus_sort (BOBGUI_WIDGET (window), BOBGUI_DIR_TAB_FORWARD, menubars);

  first = g_ptr_array_index (menubars, 0);
  if (BOBGUI_IS_POPOVER_MENU_BAR (first))
    bobgui_popover_menu_bar_select_first (BOBGUI_POPOVER_MENU_BAR (first));
  else if (BOBGUI_IS_MENU_BUTTON (first))
    bobgui_menu_button_popup (BOBGUI_MENU_BUTTON (first));

  g_ptr_array_free (menubars, TRUE);

  return TRUE;
}

static void
bobgui_window_keys_changed (BobguiWindow *window)
{
}

/*
 * _bobgui_window_set_is_active:
 * @window: a window
 * @is_active: true if the window is in the currently active toplevel
 *
 * Internal function that sets whether the window is part
 * of the currently active toplevel window (taking into account
 * inter-process embedding).
 */
static void
_bobgui_window_set_is_active (BobguiWindow *window,
                           gboolean   is_active)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (priv->is_active == is_active)
    return;

  priv->is_active = is_active;

  if (priv->focus_widget)
    {
      BobguiWidget *focus;

      focus = g_object_ref (priv->focus_widget);

      if (is_active)
        {
          synthesize_focus_change_events (window, NULL, focus, BOBGUI_CROSSING_ACTIVE);
          bobgui_widget_set_has_focus (focus, TRUE);
        }
      else
        {
          synthesize_focus_change_events (window, focus, NULL, BOBGUI_CROSSING_ACTIVE);
          bobgui_widget_set_has_focus (focus, FALSE);
        }

      g_object_unref (focus);
    }

  bobgui_accessible_update_platform_state (BOBGUI_ACCESSIBLE (window),
                                        BOBGUI_ACCESSIBLE_PLATFORM_STATE_ACTIVE);

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_IS_ACTIVE]);
}

/**
 * bobgui_window_set_auto_startup_notification:
 * @setting: true to automatically do startup notification
 *
 * Sets whether the window should request startup notification.
 *
 * By default, after showing the first window, BOBGUI calls
 * [method@Gdk.Toplevel.set_startup_id]. Call this function
 * to disable the automatic startup notification. You might do this
 * if your first window is a splash screen, and you want to delay
 * notification until after your real main window has been shown,
 * for example.
 *
 * In that example, you would disable startup notification
 * temporarily, show your splash screen, then re-enable it so that
 * showing the main window would automatically result in notification.
 */
void
bobgui_window_set_auto_startup_notification (gboolean setting)
{
  disable_startup_notification = !setting;
}

/**
 * bobgui_window_get_mnemonics_visible:
 * @window: a window
 *
 * Gets whether mnemonics are supposed to be visible.
 *
 * Returns: true if mnemonics are supposed to be visible
 *   in this window
 */
gboolean
bobgui_window_get_mnemonics_visible (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), FALSE);

  return priv->mnemonics_visible;
}

/**
 * bobgui_window_set_mnemonics_visible:
 * @window: a window
 * @setting: the new value
 *
 * Sets whether mnemonics are supposed to be visible.
 *
 * This property is maintained by BOBGUI based on user input,
 * and should not be set by applications.
 */
void
bobgui_window_set_mnemonics_visible (BobguiWindow *window,
                                  gboolean   setting)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  setting = setting != FALSE;

  if (priv->mnemonics_visible != setting)
    {
      priv->mnemonics_visible = setting;
      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_MNEMONICS_VISIBLE]);
    }

  if (priv->mnemonics_display_timeout_id)
    {
      g_source_remove (priv->mnemonics_display_timeout_id);
      priv->mnemonics_display_timeout_id = 0;
    }
}

static gboolean
schedule_mnemonics_visible_cb (gpointer data)
{
  BobguiWindow *window = data;
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  priv->mnemonics_display_timeout_id = 0;

  bobgui_window_set_mnemonics_visible (window, TRUE);

  return FALSE;
}

void
_bobgui_window_schedule_mnemonics_visible (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  if (priv->mnemonics_display_timeout_id)
    return;

  priv->mnemonics_display_timeout_id =
    g_timeout_add (MNEMONICS_DELAY, schedule_mnemonics_visible_cb, window);
  gdk_source_set_static_name_by_id (priv->mnemonics_display_timeout_id, "[bobgui] schedule_mnemonics_visible_cb");
}

/**
 * bobgui_window_get_focus_visible:
 * @window: a window
 *
 * Gets whether “focus rectangles” are supposed to be visible.
 *
 * Returns: true if “focus rectangles” are supposed to be visible
 *   in this window
 */
gboolean
bobgui_window_get_focus_visible (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), FALSE);

  return priv->focus_visible;
}

static gboolean
unset_focus_visible (gpointer data)
{
  BobguiWindow *window = data;
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  priv->focus_visible_timeout = 0;

  bobgui_window_set_focus_visible (window, FALSE);

  return G_SOURCE_REMOVE;
}

/**
 * bobgui_window_set_focus_visible:
 * @window: a window
 * @setting: the new value
 *
 * Sets whether “focus rectangles” are supposed to be visible.
 *
 * This property is maintained by BOBGUI based on user input,
 * and should not be set by applications.
 */
void
bobgui_window_set_focus_visible (BobguiWindow *window,
                              gboolean   setting)
{
  gboolean changed;

  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  changed = priv->focus_visible != setting;

  priv->focus_visible = setting;

  if (priv->focus_visible_timeout)
    {
      g_source_remove (priv->focus_visible_timeout);
      priv->focus_visible_timeout = 0;
    }

  if (priv->focus_visible)
    {
      priv->focus_visible_timeout = g_timeout_add_seconds (VISIBLE_FOCUS_DURATION, unset_focus_visible, window);
      gdk_source_set_static_name_by_id (priv->focus_visible_timeout, "[bobgui] unset_focus_visible");
    }

  if (changed)
    {
      if (priv->focus_widget)
        {
          BobguiWidget *widget;

          for (widget = priv->focus_widget; widget; widget = bobgui_widget_get_parent (widget))
            {
              if (priv->focus_visible)
                bobgui_widget_set_state_flags (widget, BOBGUI_STATE_FLAG_FOCUS_VISIBLE, FALSE);
              else
                bobgui_widget_unset_state_flags (widget, BOBGUI_STATE_FLAG_FOCUS_VISIBLE);
            }
        }
      g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_FOCUS_VISIBLE]);
    }
}

static void
ensure_state_flag_backdrop (BobguiWidget *widget)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (BOBGUI_WINDOW (widget));
  gboolean surface_focused = TRUE;

  surface_focused = gdk_toplevel_get_state (GDK_TOPLEVEL (priv->surface)) & GDK_TOPLEVEL_STATE_FOCUSED;

  if (!surface_focused)
    bobgui_widget_set_state_flags (widget, BOBGUI_STATE_FLAG_BACKDROP, FALSE);
  else
    bobgui_widget_unset_state_flags (widget, BOBGUI_STATE_FLAG_BACKDROP);
}

static void set_warn_again (gboolean warn);
static void bobgui_window_set_debugging (GdkDisplay *display,
                                      gboolean    enable,
                                      gboolean    toggle,
                                      gboolean    select,
                                      gboolean    warn);

static void
warn_response (BobguiDialog *dialog,
               int        response)
{
  BobguiWidget *check;
  gboolean remember;
  BobguiWidget *inspector_window;
  GdkDisplay *display;

  inspector_window = BOBGUI_WIDGET (bobgui_window_get_transient_for (BOBGUI_WINDOW (dialog)));
  display = bobgui_inspector_window_get_inspected_display (BOBGUI_INSPECTOR_WINDOW (inspector_window));

  check = g_object_get_data (G_OBJECT (dialog), "check");
  remember = bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (check));

  bobgui_window_destroy (BOBGUI_WINDOW (dialog));
  g_object_set_data (G_OBJECT (inspector_window), "warning_dialog", NULL);

  if (response == BOBGUI_RESPONSE_NO)
    bobgui_window_set_debugging (display, FALSE, FALSE, FALSE, FALSE);
  else
    set_warn_again (!remember);
}

static void
bobgui_window_set_debugging (GdkDisplay *display,
                          gboolean    enable,
                          gboolean    toggle,
                          gboolean    select,
                          gboolean    warn)
{
  BobguiWidget *dialog = NULL;
  BobguiWidget *area;
  BobguiWidget *check;
  BobguiWidget *inspector_window;
  gboolean was_debugging;

  was_debugging = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (display), "-bobgui-debugging-enabled"));

  if (toggle)
    enable = !was_debugging;

  g_object_set_data (G_OBJECT (display), "-bobgui-debugging-enabled", GINT_TO_POINTER (enable));

  if (enable)
    {
      inspector_window = bobgui_inspector_window_get (display);

      bobgui_window_present (BOBGUI_WINDOW (inspector_window));

      if (warn)
        {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          dialog = bobgui_message_dialog_new (BOBGUI_WINDOW (inspector_window),
                                           BOBGUI_DIALOG_MODAL|BOBGUI_DIALOG_DESTROY_WITH_PARENT,
                                           BOBGUI_MESSAGE_QUESTION,
                                           BOBGUI_BUTTONS_NONE,
                                           _("Do you want to use BOBGUI Inspector?"));
          bobgui_message_dialog_format_secondary_text (BOBGUI_MESSAGE_DIALOG (dialog),
              _("BOBGUI Inspector is an interactive debugger that lets you explore and "
                "modify the internals of any BOBGUI application. Using it may cause the "
                "application to break or crash."));

          area = bobgui_message_dialog_get_message_area (BOBGUI_MESSAGE_DIALOG (dialog));
          check = bobgui_check_button_new_with_label (_("Don’t show this message again"));
          bobgui_widget_set_margin_start (check, 10);
          bobgui_box_append (BOBGUI_BOX (area), check);
          g_object_set_data (G_OBJECT (dialog), "check", check);
          bobgui_dialog_add_button (BOBGUI_DIALOG (dialog), _("_Cancel"), BOBGUI_RESPONSE_NO);
          bobgui_dialog_add_button (BOBGUI_DIALOG (dialog), _("_OK"), BOBGUI_RESPONSE_YES);
          g_signal_connect (dialog, "response", G_CALLBACK (warn_response), inspector_window);
          g_object_set_data (G_OBJECT (inspector_window), "warning_dialog", dialog);

          bobgui_window_present (BOBGUI_WINDOW (dialog));
G_GNUC_END_IGNORE_DEPRECATIONS
        }

      if (select)
        bobgui_inspector_window_select_widget_under_pointer (BOBGUI_INSPECTOR_WINDOW (inspector_window));
    }
  else if (was_debugging)
    {
      inspector_window = bobgui_inspector_window_get (display);

      bobgui_widget_set_visible (inspector_window, FALSE);
    }
}

/**
 * bobgui_window_set_interactive_debugging:
 * @enable: true to enable interactive debugging
 *
 * Opens or closes the [interactive debugger](running.html#interactive-debugging).
 *
 * The debugger offers access to the widget hierarchy of the application
 * and to useful debugging tools.
 *
 * This function allows applications that already use
 * <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>I</kbd>
 * (or <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>D</kbd>)
 * for their own key shortcuts to add a different shortcut to open the Inspector.
 *
 * If you are not overriding the default key shortcuts for the Inspector,
 * you should not use this function.
 */
void
bobgui_window_set_interactive_debugging (gboolean enable)
{
  GdkDisplay *display = gdk_display_get_default ();

  bobgui_window_set_debugging (display, enable, FALSE, FALSE, FALSE);
}

static gboolean
inspector_keybinding_enabled (gboolean *warn)
{
  GSettingsSchema *schema;
  GSettings *settings;
  gboolean enabled;

  enabled = TRUE;
  *warn = TRUE;

  schema = g_settings_schema_source_lookup (g_settings_schema_source_get_default (),
                                            "org.bobgui.bobgui4.Settings.Debug",
                                            TRUE);

  if (schema)
    {
      settings = g_settings_new_full (schema, NULL, NULL);
      enabled = g_settings_get_boolean (settings, "enable-inspector-keybinding");
      *warn = g_settings_get_boolean (settings, "inspector-warning");
      g_object_unref (settings);
      g_settings_schema_unref (schema);
    }

  return enabled;
}

static void
set_warn_again (gboolean warn)
{
  GSettingsSchema *schema;
  GSettings *settings;

  schema = g_settings_schema_source_lookup (g_settings_schema_source_get_default (),
                                            "org.bobgui.bobgui4.Settings.Debug",
                                            TRUE);

  if (schema)
    {
      settings = g_settings_new_full (schema, NULL, NULL);
      g_settings_set_boolean (settings, "inspector-warning", warn);
      g_object_unref (settings);
      g_settings_schema_unref (schema);
    }
}

static gboolean
bobgui_window_enable_debugging (BobguiWindow *window,
                             gboolean   toggle)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  gboolean warn;

  if (!inspector_keybinding_enabled (&warn))
    return FALSE;

  bobgui_window_set_debugging (priv->display, TRUE, toggle, !toggle, warn);

  return TRUE;
}

typedef struct {
  BobguiWindow *window;
  BobguiWindowHandleExported callback;
  gpointer user_data;
} ExportHandleData;

static char *
prefix_handle (GdkDisplay *display,
               char       *handle)
{
#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (display))
    return g_strconcat ("wayland:", handle, NULL);
  else
#endif
#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY (display))
    return g_strconcat ("x11:", handle, NULL);
  else
#endif
    return NULL;
}

static const char *
unprefix_handle (const char *handle)
{
  if (g_str_has_prefix (handle, "wayland:"))
    return handle + strlen ("wayland:");
  else if (g_str_has_prefix (handle, "x11:"))
    return handle + strlen ("x11:");
  else
    return handle;
}

static void
export_handle_done (GObject      *source,
                    GAsyncResult *result,
                    void         *user_data)
{
  ExportHandleData *data = (ExportHandleData *)user_data;
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (data->window);
  char *handle;

  handle = gdk_toplevel_export_handle_finish (GDK_TOPLEVEL (priv->surface), result, NULL);
  if (handle)
    {
      char *prefixed;

      prefixed = prefix_handle (priv->display, handle);
      data->callback (data->window, prefixed, data->user_data);
      g_free (prefixed);
      g_free (handle);
    }
  else
    data->callback (data->window, NULL, data->user_data);


  g_free (data);
}

gboolean
bobgui_window_export_handle (BobguiWindow               *window,
                          BobguiWindowHandleExported  callback,
                          gpointer                 user_data)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  ExportHandleData *data;

  if (G_UNLIKELY (!priv->surface))
    return FALSE;

  data = g_new (ExportHandleData, 1);
  data->window = window;
  data->callback = callback;
  data->user_data = user_data;

  gdk_toplevel_export_handle (GDK_TOPLEVEL (priv->surface), NULL, export_handle_done, data);

  return TRUE;
}

void
bobgui_window_unexport_handle (BobguiWindow  *window,
                            const char *handle)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  if (G_LIKELY (priv->surface))
    gdk_toplevel_unexport_handle (GDK_TOPLEVEL (priv->surface), unprefix_handle (handle));
}

static BobguiPointerFocus *
bobgui_window_lookup_pointer_focus (BobguiWindow        *window,
                                 GdkDevice        *device,
                                 GdkEventSequence *sequence)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  GList *l;

  for (l = priv->foci; l; l = l->next)
    {
      BobguiPointerFocus *focus = l->data;

      if (focus->device == device && focus->sequence == sequence)
        return focus;
    }

  return NULL;
}

BobguiWidget *
bobgui_window_lookup_pointer_focus_widget (BobguiWindow        *window,
                                        GdkDevice        *device,
                                        GdkEventSequence *sequence)
{
  BobguiPointerFocus *focus;

  focus = bobgui_window_lookup_pointer_focus (window, device, sequence);
  return focus ? bobgui_pointer_focus_get_target (focus) : NULL;
}

BobguiWidget *
bobgui_window_lookup_effective_pointer_focus_widget (BobguiWindow        *window,
                                                  GdkDevice        *device,
                                                  GdkEventSequence *sequence)
{
  BobguiPointerFocus *focus;

  focus = bobgui_window_lookup_pointer_focus (window, device, sequence);
  return focus ? bobgui_pointer_focus_get_effective_target (focus) : NULL;
}

BobguiWidget *
bobgui_window_lookup_pointer_focus_implicit_grab (BobguiWindow        *window,
                                               GdkDevice        *device,
                                               GdkEventSequence *sequence)
{
  BobguiPointerFocus *focus;

  focus = bobgui_window_lookup_pointer_focus (window, device, sequence);
  return focus ? bobgui_pointer_focus_get_implicit_grab (focus) : NULL;
}

static void
set_widget_active_state (BobguiWidget *widget,
                         BobguiWidget *topmost,
                         gboolean   active)
{
  BobguiWidget *w = widget;

  while (w)
    {
      bobgui_widget_set_active_state (w, active);
      if (w == topmost)
        break;
      w = _bobgui_widget_get_parent (w);
    }
}

void
bobgui_window_update_pointer_focus (BobguiWindow        *window,
                                 GdkDevice        *device,
                                 GdkEventSequence *sequence,
                                 BobguiWidget        *target,
                                 double            x,
                                 double            y)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiPointerFocus *focus;

  focus = bobgui_window_lookup_pointer_focus (window, device, sequence);
  if (focus)
    {
      bobgui_pointer_focus_ref (focus);

      if (target)
        {
          bobgui_pointer_focus_set_target (focus, target);
          bobgui_pointer_focus_set_coordinates (focus, x, y);
        }
      else
        {
          GList *pos;

          pos = g_list_find (priv->foci, focus);
          if (pos)
            {
              if (focus->grab_widget)
                {
                  set_widget_active_state (focus->grab_widget, NULL, FALSE);
                  bobgui_pointer_focus_set_implicit_grab (focus, NULL);
                }

              priv->foci = g_list_remove (priv->foci, focus);
              bobgui_pointer_focus_unref (focus);
            }
        }

      bobgui_pointer_focus_unref (focus);
    }
  else if (target)
    {
      focus = bobgui_pointer_focus_new (window, target, device, sequence, x, y);
      priv->foci = g_list_prepend (priv->foci, focus);
    }
}

void
bobgui_window_update_pointer_focus_on_state_change (BobguiWindow *window,
                                                 BobguiWidget *widget)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  GList *l = priv->foci;

  while (l)
    {
      GList *next;

      BobguiPointerFocus *focus = l->data;

      next = l->next;

      bobgui_pointer_focus_ref (focus);

      if (focus->grab_widget &&
          (focus->grab_widget == widget ||
           bobgui_widget_is_ancestor (focus->grab_widget, widget)))
        {
          set_widget_active_state (focus->grab_widget, widget, FALSE);
          bobgui_pointer_focus_set_implicit_grab (focus,
                                               bobgui_widget_get_parent (widget));
        }

      if (BOBGUI_WIDGET (focus->toplevel) == widget)
        {
          /* Unmapping the toplevel, remove pointer focus */
          priv->foci = g_list_remove_link (priv->foci, l);
          bobgui_pointer_focus_unref (focus);
          g_list_free (l);
        }
      else if (focus->target == widget ||
               bobgui_widget_is_ancestor (focus->target, widget))
        {
          BobguiWidget *old_target;

          old_target = g_object_ref (focus->target);
          bobgui_pointer_focus_repick_target (focus);
          if (bobgui_widget_get_native (focus->target) == bobgui_widget_get_native (old_target))
            {
              bobgui_synthesize_crossing_events (BOBGUI_ROOT (window),
                                              BOBGUI_CROSSING_POINTER,
                                              old_target, focus->target,
                                              focus->x, focus->y,
                                              GDK_CROSSING_NORMAL,
                                              NULL);
            }
          g_object_unref (old_target);
        }

      bobgui_pointer_focus_unref (focus);

      l = next;
    }
}

void
bobgui_window_maybe_revoke_implicit_grab (BobguiWindow *window,
                                       GdkDevice *device,
                                       BobguiWidget *grab_widget)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  GList *l = priv->foci;

  while (l)
    {
      BobguiPointerFocus *focus = l->data;

      l = l->next;

      if (focus->toplevel != window)
        continue;

      if ((!device || focus->device == device) &&
          focus->target != grab_widget &&
          !bobgui_widget_is_ancestor (focus->target, grab_widget))
        bobgui_window_set_pointer_focus_grab (window,
                                           focus->device,
                                           focus->sequence,
                                           NULL);
    }
}

void
bobgui_window_set_pointer_focus_grab (BobguiWindow        *window,
                                   GdkDevice        *device,
                                   GdkEventSequence *sequence,
                                   BobguiWidget        *grab_widget)
{
  BobguiPointerFocus *focus;
  BobguiWidget *current;

  focus = bobgui_window_lookup_pointer_focus (window, device, sequence);
  if (!focus && !grab_widget)
    return;
  g_assert (focus != NULL);

  current = bobgui_pointer_focus_get_implicit_grab (focus);

  if (current == grab_widget)
    return;

  if (current)
    set_widget_active_state (current, NULL, FALSE);

  bobgui_pointer_focus_set_implicit_grab (focus, grab_widget);

  if (grab_widget)
    set_widget_active_state (grab_widget, NULL, TRUE);
}

static void
update_cursor (BobguiWindow *toplevel,
               GdkDevice *device,
               BobguiWidget *grab_widget,
               BobguiWidget *target)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (toplevel);
  GdkCursor *cursor = NULL;
  BobguiNative *native;
  GdkSurface *surface;

  native = bobgui_widget_get_native (target);
  surface = bobgui_native_get_surface (native);

  if (grab_widget && !bobgui_widget_is_ancestor (target, grab_widget) && target != grab_widget)
    {
      /* Outside the grab widget, cursor stays to whatever the grab
       * widget says.
       */
      if (bobgui_widget_get_native (grab_widget) == native)
        cursor = bobgui_widget_get_cursor (grab_widget);
      else
        cursor = NULL;
    }
  else
    {
      /* Inside the grab widget or in absence of grabs, allow walking
       * up the hierarchy to find out the cursor.
       */
      while (target)
        {
          /* Don't inherit cursors across surfaces */
          if (native != bobgui_widget_get_native (target))
            break;

          if (target == BOBGUI_WIDGET (toplevel) && priv->resize_cursor != NULL)
            cursor = priv->resize_cursor;
          else
            cursor = bobgui_widget_get_cursor (target);

          if (cursor)
            break;

          if (grab_widget && target == grab_widget)
            break;

          target = _bobgui_widget_get_parent (target);
        }
    }

  gdk_surface_set_device_cursor (surface, device, cursor);
}

void
bobgui_window_maybe_update_cursor (BobguiWindow *window,
                                BobguiWidget *widget,
                                GdkDevice *device)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  GList *l;

  for (l = priv->foci; l; l = l->next)
    {
      BobguiPointerFocus *focus = l->data;
      BobguiWidget *grab_widget, *target;
      BobguiWindowGroup *group;

      if (focus->sequence)
        continue;
      if (device && device != focus->device)
        continue;

      group = bobgui_window_get_group (window);

      grab_widget = bobgui_window_group_get_current_grab (group);
      if (!grab_widget)
        grab_widget = bobgui_pointer_focus_get_implicit_grab (focus);

      target = bobgui_pointer_focus_get_target (focus);

      if (widget)
        {
          /* Check whether the changed widget affects the current cursor
           * lookups.
           */
          if (grab_widget && grab_widget != widget &&
              !bobgui_widget_is_ancestor (widget, grab_widget))
            continue;
          if (target != widget &&
              !bobgui_widget_is_ancestor (target, widget))
            continue;
        }

      update_cursor (focus->toplevel, focus->device, grab_widget, target);

      if (device)
        break;
    }
}

/**
 * bobgui_window_set_child:
 * @window: a window
 * @child: (nullable): the child widget
 *
 * Sets the child widget of the window.
 */
void
bobgui_window_set_child (BobguiWindow *window,
                      BobguiWidget *child)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (BOBGUI_IS_WINDOW (window));
  g_return_if_fail (child == NULL || priv->child == child || bobgui_widget_get_parent (child) == NULL);

  if (priv->child == child)
    return;

  g_clear_pointer (&priv->child, bobgui_widget_unparent);

  if (child)
    {
      priv->child = child;
      bobgui_widget_insert_before (child, BOBGUI_WIDGET (window), priv->title_box);
    }

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_CHILD]);
}

/**
 * bobgui_window_get_child:
 * @window: a window
 *
 * Gets the child widget of the window.
 *
 * Returns: (nullable) (transfer none): the child widget of @window
 */
BobguiWidget *
bobgui_window_get_child (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), NULL);

  return priv->child;
}

/**
 * bobgui_window_destroy:
 * @window: the window to destroy
 *
 * Drops the internal reference BOBGUI holds on toplevel windows.
 */
void
bobgui_window_destroy (BobguiWindow *window)
{
  guint i;

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  /* If bobgui_window_destroy() has been called before. Can happen
   * when destroying a dialog manually in a ::close handler for example.
   */
  if (!g_list_store_find (toplevel_list, window, &i))
    return;

  g_object_ref (window);

  bobgui_tooltip_unset_surface (BOBGUI_NATIVE (window));

  bobgui_window_hide (BOBGUI_WIDGET (window));

  g_list_store_remove (toplevel_list, i);

  bobgui_window_release_application (window);

  bobgui_widget_unrealize (BOBGUI_WIDGET (window));

  g_object_unref (window);
}

GdkDevice**
bobgui_window_get_foci_on_widget (BobguiWindow *window,
                               BobguiWidget *widget,
                               guint     *n_devices)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  GPtrArray *array = g_ptr_array_new ();
  GList *l;

  for (l = priv->foci; l; l = l->next)
    {
      BobguiPointerFocus *focus = l->data;
      BobguiWidget *target;

      target = bobgui_pointer_focus_get_effective_target (focus);

      if (target == widget || bobgui_widget_is_ancestor (target, widget))
        g_ptr_array_add (array, focus->device);
    }

  if (n_devices)
    *n_devices = array->len;

  return (GdkDevice**) g_ptr_array_free (array, FALSE);
}

static void
bobgui_synthesize_grab_crossing (BobguiWidget *child,
                              GdkDevice *device,
                              BobguiWidget *new_grab_widget,
                              BobguiWidget *old_grab_widget,
                              gboolean   from_grab,
                              gboolean   was_shadowed,
                              gboolean   is_shadowed)
{
  g_object_ref (child);

  if (is_shadowed)
    {
      if (!was_shadowed &&
          bobgui_widget_is_sensitive (child))
        _bobgui_widget_synthesize_crossing (child,
                                         new_grab_widget,
                                         device,
                                         GDK_CROSSING_BOBGUI_GRAB);
    }
  else
    {
      if (was_shadowed &&
          bobgui_widget_is_sensitive (child))
        _bobgui_widget_synthesize_crossing (old_grab_widget, child,
                                         device,
                                         from_grab ? GDK_CROSSING_BOBGUI_GRAB :
                                         GDK_CROSSING_BOBGUI_UNGRAB);
    }

  g_object_unref (child);
}

static void
bobgui_window_propagate_grab_notify (BobguiWindow *window,
                                  BobguiWidget *target,
                                  GdkDevice *device,
                                  BobguiWidget *old_grab_widget,
                                  BobguiWidget *new_grab_widget,
                                  gboolean   from_grab)
{
  GList *l, *widgets = NULL;
  gboolean was_grabbed = FALSE, is_grabbed = FALSE;

  while (target)
    {
      if (target == old_grab_widget)
        was_grabbed = TRUE;
      if (target == new_grab_widget)
        is_grabbed = TRUE;
      widgets = g_list_prepend (widgets, g_object_ref (target));
      target = bobgui_widget_get_parent (target);
    }

  widgets = g_list_reverse (widgets);

  for (l = widgets; l; l = l->next)
    {
      gboolean was_shadowed, is_shadowed;

      was_shadowed = old_grab_widget && !was_grabbed;
      is_shadowed = new_grab_widget && !is_grabbed;

      if (l->data == old_grab_widget)
        was_grabbed = FALSE;
      if (l->data == new_grab_widget)
        is_grabbed = FALSE;

      if (was_shadowed == is_shadowed)
        break;

      bobgui_synthesize_grab_crossing (l->data,
                                    device,
                                    old_grab_widget,
                                    new_grab_widget,
                                    from_grab,
                                    was_shadowed,
                                    is_shadowed);

      bobgui_widget_reset_controllers (l->data);
    }

  g_list_free_full (widgets, g_object_unref);
}

void
bobgui_window_grab_notify (BobguiWindow *window,
                        BobguiWidget *old_grab_widget,
                        BobguiWidget *new_grab_widget,
                        gboolean   from_grab)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  GList *l;

  for (l = priv->foci; l; l = l->next)
    {
      BobguiPointerFocus *focus = l->data;

      bobgui_window_propagate_grab_notify (window,
                                        bobgui_pointer_focus_get_effective_target (focus),
                                        focus->device,
                                        old_grab_widget,
                                        new_grab_widget,
                                        from_grab);
    }
}

/**
 * bobgui_window_set_handle_menubar_accel:
 * @window: a window
 * @handle_menubar_accel: true to make @window handle <kbd>F10</kbd>
 *
 * Sets whether this window should react to <kbd>F10</kbd>
 * presses by activating a menubar it contains.
 *
 * Since: 4.2
 */
void
bobgui_window_set_handle_menubar_accel (BobguiWindow *window,
                                     gboolean   handle_menubar_accel)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiPropagationPhase phase;

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  phase = handle_menubar_accel ? BOBGUI_PHASE_CAPTURE : BOBGUI_PHASE_NONE;

  if (bobgui_event_controller_get_propagation_phase (priv->menubar_controller) == phase)
    return;

  bobgui_event_controller_set_propagation_phase (priv->menubar_controller, phase);

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_HANDLE_MENUBAR_ACCEL]);
}

/**
 * bobgui_window_get_handle_menubar_accel:
 * @window: a window
*
* Returns whether this window reacts to <kbd>F10</kbd>
* presses by activating a menubar it contains.
 *
 * Returns: true if the window handles <kbd>F10</kbd>
 *
 * Since: 4.2
 */
gboolean
bobgui_window_get_handle_menubar_accel (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);
  BobguiPropagationPhase phase;

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), TRUE);

  phase = bobgui_event_controller_get_propagation_phase (priv->menubar_controller);

  return phase == BOBGUI_PHASE_CAPTURE;
}

/**
 * bobgui_window_get_gravity:
 * @window: a window
 *
 * Returns the gravity that is used when changing the window size programmatically.
 *
 * Returns: the gravity
 *
 * Since: 4.20
 */
BobguiWindowGravity
bobgui_window_get_gravity (BobguiWindow *window)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_val_if_fail (BOBGUI_IS_WINDOW (window), BOBGUI_WINDOW_GRAVITY_TOP_START);

  return priv->gravity;
}

/**
 * bobgui_window_set_gravity:
 * @window: a window
 * @gravity: the new gravity
 *
 * Sets the gravity that is used when changing the window size programmatically.
 *
 * Since: 4.20
 */
void
bobgui_window_set_gravity (BobguiWindow        *window,
                        BobguiWindowGravity  gravity)
{
  BobguiWindowPrivate *priv = bobgui_window_get_instance_private (window);

  g_return_if_fail (BOBGUI_IS_WINDOW (window));

  if (priv->gravity == gravity)
    return;

  priv->gravity = gravity;

  if (priv->surface)
    gdk_toplevel_set_gravity (GDK_TOPLEVEL (priv->surface), get_gdk_gravity (window));

  g_object_notify_by_pspec (G_OBJECT (window), window_props[PROP_GRAVITY]);
}
