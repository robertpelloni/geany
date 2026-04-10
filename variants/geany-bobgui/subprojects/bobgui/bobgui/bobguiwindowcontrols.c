/*
 * Copyright (c) 2020 Alexander Mikhaylenko <alexm@gnome.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#include "bobguiwindowcontrols.h"

#include "bobguiaccessible.h"
#include "bobguiactionable.h"
#include "bobguiboxlayout.h"
#include "bobguibutton.h"
#include "bobguienums.h"
#include "bobguiicontheme.h"
#include "bobguiimage.h"
#include <glib/gi18n-lib.h>
#include "bobguiprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguiwindowprivate.h"

#ifdef GDK_WINDOWING_MACOS
#include "bobguiwindowbuttonsquartzprivate.h"
#endif

/**
 * BobguiWindowControls:
 *
 * Shows window frame controls.
 *
 * Typical window frame controls are minimize, maximize and close buttons,
 * and the window icon.
 *
 * <picture>
 *   <source srcset="windowcontrols-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiWindowControls" src="windowcontrols.png">
 * </picture>
 *
 * `BobguiWindowControls` only displays start or end side of the controls (see
 * [property@Bobgui.WindowControls:side]), so it's intended to be always used
 * in pair with another `BobguiWindowControls` for the opposite side, for example:
 *
 * ```xml
 * <object class="BobguiBox">
 *   <child>
 *     <object class="BobguiWindowControls">
 *       <property name="side">start</property>
 *     </object>
 *   </child>
 *
 *   ...
 *
 *   <child>
 *     <object class="BobguiWindowControls">
 *       <property name="side">end</property>
 *     </object>
 *   </child>
 * </object>
 * ```
 *
 * # CSS nodes
 *
 * ```
 * windowcontrols
 * ├── [image.icon]
 * ├── [button.minimize]
 * ├── [button.maximize]
 * ╰── [button.close]
 * ```
 *
 * A `BobguiWindowControls`' CSS node is called windowcontrols. It contains
 * subnodes corresponding to each title button. Which of the title buttons
 * exist and where they are placed exactly depends on the desktop environment
 * and [property@Bobgui.WindowControls:decoration-layout] value.
 *
 * When [property@Bobgui.WindowControls:empty] is true, it gets the .empty
 * style class.
 *
 * # Accessibility
 *
 * `BobguiWindowControls` uses the [enum@Bobgui.AccessibleRole.group] role.
 */

struct _BobguiWindowControls {
  BobguiWidget parent_instance;

  BobguiPackType side;
  char *decoration_layout;

  gboolean use_native_controls;
  gboolean empty;
};

enum {
  PROP_0,
  PROP_SIDE,
  PROP_DECORATION_LAYOUT,
  PROP_USE_NATIVE_CONTROLS,
  PROP_EMPTY,
  LAST_PROP
};

static GParamSpec *props[LAST_PROP] = { NULL, };

#define WINDOW_ICON_SIZE 16

G_DEFINE_TYPE (BobguiWindowControls, bobgui_window_controls, BOBGUI_TYPE_WIDGET)

static char *
get_layout (BobguiWindowControls *self)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  BobguiRoot *root;
  char *layout_desc, *layout_half;
  char **tokens;

  root = bobgui_widget_get_root (widget);
  if (!root || !BOBGUI_IS_WINDOW (root))
    return NULL;

  if (self->decoration_layout)
    layout_desc = g_strdup (self->decoration_layout);
  else
    g_object_get (bobgui_widget_get_settings (widget),
                  "bobgui-decoration-layout", &layout_desc,
                  NULL);

  if (layout_desc == NULL || layout_desc[0] == '\0')
    {
      g_free (layout_desc);
      return NULL;
    }

  tokens = g_strsplit (layout_desc, ":", 2);

  if (tokens[1] == NULL)
    {
      layout_half = g_strdup (tokens[0]);
    }
  else
    {
      switch (self->side)
        {
        case BOBGUI_PACK_START:
          layout_half = g_strdup (tokens[0]);
          break;

        case BOBGUI_PACK_END:
          layout_half = g_strdup (tokens[1]);
          break;

        default:
          g_assert_not_reached ();
        }
    }

  g_free (layout_desc);
  g_strfreev (tokens);

  return layout_half;
}

static GdkPaintable *
get_default_icon (BobguiWidget *widget)
{
  GdkDisplay *display = bobgui_widget_get_display (widget);
  BobguiIconPaintable *info;
  int scale = bobgui_widget_get_scale_factor (widget);

  info = bobgui_icon_theme_lookup_icon (bobgui_icon_theme_get_for_display (display),
                                     bobgui_window_get_default_icon_name (),
                                     NULL,
                                     WINDOW_ICON_SIZE,
                                     scale,
                                     bobgui_widget_get_direction (widget),
                                     0);

  return GDK_PAINTABLE (info);
}

static gboolean
update_window_icon (BobguiWindow *window,
                    BobguiWidget *icon)
{
  GdkPaintable *paintable;

  if (window)
    paintable = bobgui_window_get_icon_for_size (window, WINDOW_ICON_SIZE);
  else
    paintable = get_default_icon (icon);

  if (paintable)
    {
      bobgui_image_set_from_paintable (BOBGUI_IMAGE (icon), paintable);
      g_object_unref (paintable);
      bobgui_widget_set_visible (icon, TRUE);

      return TRUE;
    }

  return FALSE;
}

static void
set_empty (BobguiWindowControls *self,
           gboolean           empty)
{
  if (empty == self->empty)
    return;

  self->empty = empty;

  if (empty)
    bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "empty");
  else
    bobgui_widget_remove_css_class (BOBGUI_WIDGET (self), "empty");

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_EMPTY]);
}

static void
clear_controls (BobguiWindowControls *self)
{
  BobguiWidget *child = bobgui_widget_get_first_child (BOBGUI_WIDGET (self));

  while (child)
    {
      BobguiWidget *next = bobgui_widget_get_next_sibling (child);

      bobgui_widget_unparent (child);

      child = next;
    }
}

static void
update_window_buttons (BobguiWindowControls *self)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  char *layout;
  char **tokens;
  int i;
  gboolean is_sovereign_window;
  gboolean maximized;
  gboolean resizable;
  gboolean deletable;
  gboolean empty = TRUE;
  BobguiRoot *root;
  BobguiWindow *window = NULL;

  root = bobgui_widget_get_root (widget);
  if (!root || !BOBGUI_IS_WINDOW (root))
    {
      set_empty (self, TRUE);

      return;
    }

#ifdef GDK_WINDOWING_MACOS
  if (self->use_native_controls)
    {
      if (BOBGUI_IS_WINDOW_BUTTONS_QUARTZ (bobgui_widget_get_first_child (widget)))
        return;

      clear_controls (self);

      if (self->side == BOBGUI_PACK_START)
        {
          BobguiWidget *controls = g_object_new (BOBGUI_TYPE_WINDOW_BUTTONS_QUARTZ, NULL);
          g_object_bind_property (self, "decoration-layout",
                                  controls, "decoration-layout",
                                  G_BINDING_SYNC_CREATE);
          bobgui_widget_set_parent (controls, BOBGUI_WIDGET (self));

          bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "native");
          empty = FALSE;
        }

      set_empty (self, empty);

      return;
    }
  else
    {
      bobgui_widget_remove_css_class (BOBGUI_WIDGET (self), "native");
    }
#endif

  clear_controls (self);

  window = BOBGUI_WINDOW (root);
  is_sovereign_window = !bobgui_window_get_modal (window) &&
                         bobgui_window_get_transient_for (window) == NULL;
  maximized = bobgui_window_is_maximized (window);
  resizable = bobgui_window_get_resizable (window);
  deletable = bobgui_window_get_deletable (window);

  layout = get_layout (self);

  if (!layout)
    {
      set_empty (self, TRUE);

      return;
    }

  tokens = g_strsplit (layout, ",", -1);

  for (i = 0; tokens[i]; i++)
    {
      BobguiWidget *button = NULL;
      BobguiWidget *image = NULL;

      if (strcmp (tokens[i], "icon") == 0 &&
          is_sovereign_window)
        {
          /* The icon is not relevant for accessibility purposes */
          button = g_object_new (BOBGUI_TYPE_IMAGE,
                                 "accessible-role", BOBGUI_ACCESSIBLE_ROLE_PRESENTATION,
                                 NULL);
          bobgui_widget_set_valign (button, BOBGUI_ALIGN_CENTER);
          bobgui_widget_add_css_class (button, "icon");

          if (!update_window_icon (window, button))
            {
              g_object_ref_sink (button);
              g_object_unref (button);
              button = NULL;
            }
        }
      else if (strcmp (tokens[i], "minimize") == 0 &&
               is_sovereign_window)
        {
          button = bobgui_button_new ();
          bobgui_widget_add_css_class (button, "minimize");
          /* The icon is not relevant for accessibility purposes */
          image = g_object_new (BOBGUI_TYPE_IMAGE,
                                "accessible-role", BOBGUI_ACCESSIBLE_ROLE_PRESENTATION,
                                "icon-name", "window-minimize-symbolic",
                                "valign", BOBGUI_ALIGN_CENTER,
                                "halign", BOBGUI_ALIGN_CENTER,
                                NULL);
          g_object_set (image, "use-fallback", TRUE, NULL);
          bobgui_button_set_child (BOBGUI_BUTTON (button), image);
          bobgui_widget_set_can_focus (button, FALSE);
          bobgui_actionable_set_action_name (BOBGUI_ACTIONABLE (button),
                                          "window.minimize");
          bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (button),
                                          BOBGUI_ACCESSIBLE_PROPERTY_LABEL, _("Minimize"),
                                          BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION,
                                            _("Minimize the window"),
                                          -1);
        }
      else if (strcmp (tokens[i], "maximize") == 0 &&
               resizable &&
               is_sovereign_window)
        {
          const char *icon_name;

          icon_name = maximized ? "window-restore-symbolic" : "window-maximize-symbolic";
          button = bobgui_button_new ();
          bobgui_widget_add_css_class (button, "maximize");
          /* The icon is not relevant for accessibility purposes */
          image = g_object_new (BOBGUI_TYPE_IMAGE,
                                "accessible-role", BOBGUI_ACCESSIBLE_ROLE_PRESENTATION,
                                "icon-name", icon_name,
                                "valign", BOBGUI_ALIGN_CENTER,
                                "halign", BOBGUI_ALIGN_CENTER,
                                NULL);
          g_object_set (image, "use-fallback", TRUE, NULL);
          bobgui_button_set_child (BOBGUI_BUTTON (button), image);
          bobgui_widget_set_can_focus (button, FALSE);
          bobgui_actionable_set_action_name (BOBGUI_ACTIONABLE (button),
                                          "window.toggle-maximized");
          bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (button),
                                          BOBGUI_ACCESSIBLE_PROPERTY_LABEL, _("Maximize"),
                                          BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION,
                                            _("Maximize the window"),
                                          -1);
        }
      else if (strcmp (tokens[i], "close") == 0 &&
               deletable)
        {
          button = bobgui_button_new ();
          /* The icon is not relevant for accessibility purposes */
          image = g_object_new (BOBGUI_TYPE_IMAGE,
                                "accessible-role", BOBGUI_ACCESSIBLE_ROLE_PRESENTATION,
                                "icon-name", "window-close-symbolic",
                                "valign", BOBGUI_ALIGN_CENTER,
                                "halign", BOBGUI_ALIGN_CENTER,
                                "use-fallback", TRUE,
                                NULL);
          bobgui_widget_add_css_class (button, "close");
          bobgui_button_set_child (BOBGUI_BUTTON (button), image);
          bobgui_widget_set_can_focus (button, FALSE);
          bobgui_actionable_set_action_name (BOBGUI_ACTIONABLE (button),
                                          "window.close");
          bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (button),
                                          BOBGUI_ACCESSIBLE_PROPERTY_LABEL, _("Close"),
                                          BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION,
                                            _("Close the window"),
                                          -1);
        }

      if (button)
        {
          bobgui_widget_set_parent (button, widget);
          empty = FALSE;
        }
    }
  g_free (layout);
  g_strfreev (tokens);

  set_empty (self, empty);
}

static void
window_notify_cb (BobguiWindowControls *self,
                  GParamSpec        *pspec,
                  BobguiWindow         *window)
{
  if (pspec->name == I_("deletable") ||
      pspec->name == I_("icon-name") ||
      pspec->name == I_("maximized") ||
      pspec->name == I_("modal") ||
      pspec->name == I_("resizable") ||
      pspec->name == I_("transient-for"))
    update_window_buttons (self);
}

static void
bobgui_window_controls_root (BobguiWidget *widget)
{
  BobguiSettings *settings;
  BobguiWidget *root;

  BOBGUI_WIDGET_CLASS (bobgui_window_controls_parent_class)->root (widget);

  settings = bobgui_widget_get_settings (widget);
  g_signal_connect_swapped (settings, "notify::bobgui-decoration-layout",
                            G_CALLBACK (update_window_buttons), widget);

  root = BOBGUI_WIDGET (bobgui_widget_get_root (widget));

  if (BOBGUI_IS_WINDOW (root))
    g_signal_connect_swapped (root, "notify",
                              G_CALLBACK (window_notify_cb), widget);

  update_window_buttons (BOBGUI_WINDOW_CONTROLS (widget));
}

static void
bobgui_window_controls_unroot (BobguiWidget *widget)
{
  BobguiSettings *settings;

  settings = bobgui_widget_get_settings (widget);

  g_signal_handlers_disconnect_by_func (settings, update_window_buttons, widget);
  g_signal_handlers_disconnect_by_func (bobgui_widget_get_root (widget), window_notify_cb, widget);

  BOBGUI_WIDGET_CLASS (bobgui_window_controls_parent_class)->unroot (widget);
}

static void
bobgui_window_controls_dispose (GObject *object)
{
  BobguiWindowControls *self = BOBGUI_WINDOW_CONTROLS (object);

  clear_controls (self);

  G_OBJECT_CLASS (bobgui_window_controls_parent_class)->dispose (object);
}

static void
bobgui_window_controls_finalize (GObject *object)
{
  BobguiWindowControls *self = BOBGUI_WINDOW_CONTROLS (object);

  g_free (self->decoration_layout);

  G_OBJECT_CLASS (bobgui_window_controls_parent_class)->finalize (object);
}

static void
bobgui_window_controls_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  BobguiWindowControls *self = BOBGUI_WINDOW_CONTROLS (object);

  switch (prop_id)
    {
    case PROP_SIDE:
      g_value_set_enum (value, bobgui_window_controls_get_side (self));
      break;

    case PROP_DECORATION_LAYOUT:
      g_value_set_string (value, bobgui_window_controls_get_decoration_layout (self));
      break;

    case PROP_USE_NATIVE_CONTROLS:
      g_value_set_boolean (value, bobgui_window_controls_get_use_native_controls (self));
      break;

    case PROP_EMPTY:
      g_value_set_boolean (value, bobgui_window_controls_get_empty (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_window_controls_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  BobguiWindowControls *self = BOBGUI_WINDOW_CONTROLS (object);

  switch (prop_id)
    {
    case PROP_SIDE:
      bobgui_window_controls_set_side (self, g_value_get_enum (value));
      break;

    case PROP_DECORATION_LAYOUT:
      bobgui_window_controls_set_decoration_layout (self, g_value_get_string (value));
      break;

    case PROP_USE_NATIVE_CONTROLS:
      bobgui_window_controls_set_use_native_controls (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_window_controls_class_init (BobguiWindowControlsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = bobgui_window_controls_dispose;
  object_class->finalize = bobgui_window_controls_finalize;
  object_class->get_property = bobgui_window_controls_get_property;
  object_class->set_property = bobgui_window_controls_set_property;

  widget_class->root = bobgui_window_controls_root;
  widget_class->unroot = bobgui_window_controls_unroot;

  /**
   * BobguiWindowControls:side:
   *
   * Whether the widget shows start or end side of the decoration layout.
   *
   * See [property@Bobgui.WindowControls:decoration_layout].
   */
  props[PROP_SIDE] =
      g_param_spec_enum ("side", NULL, NULL,
                         BOBGUI_TYPE_PACK_TYPE,
                         BOBGUI_PACK_START,
                         BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindowControls:decoration-layout:
   *
   * The decoration layout for window buttons.
   *
   * If this property is not set, the
   * [property@Bobgui.Settings:bobgui-decoration-layout] setting is used.
   */
  props[PROP_DECORATION_LAYOUT] =
      g_param_spec_string ("decoration-layout", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);


  /**
   * BobguiWindowControls:use-native-controls:
   *
   * Whether to show platform native close/minimize/maximize buttons.
   *
   * For macOS, the [property@Bobgui.HeaderBar:decoration-layout] property
   * controls the use of native window controls.
   *
   * On other platforms, this option has no effect.
   *
   * See also [Using BOBGUI on Apple macOS](osx.html?native-window-controls).
   *
   * Since: 4.18
   */
  props[PROP_USE_NATIVE_CONTROLS] =
      g_param_spec_boolean ("use-native-controls", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiWindowControls:empty:
   *
   * Whether the widget has any window buttons.
   */
  props[PROP_EMPTY] =
    g_param_spec_boolean ("empty", NULL, NULL,
                          TRUE,
                          BOBGUI_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BOX_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, I_("windowcontrols"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GROUP);
}

static void
bobgui_window_controls_init (BobguiWindowControls *self)
{
  self->decoration_layout = NULL;
  self->side = BOBGUI_PACK_START;
  self->use_native_controls = FALSE;
  self->empty = TRUE;

  bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "empty");
  bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "start");

  bobgui_widget_set_can_focus (BOBGUI_WIDGET (self), FALSE);
}

/**
 * bobgui_window_controls_new:
 * @side: the side
 *
 * Creates a new `BobguiWindowControls`.
 *
 * Returns: a new `BobguiWindowControls`
 **/
BobguiWidget *
bobgui_window_controls_new (BobguiPackType side)
{
  return g_object_new (BOBGUI_TYPE_WINDOW_CONTROLS,
                       "side", side,
                       NULL);
}

/**
 * bobgui_window_controls_get_side:
 * @self: a window controls widget
 *
 * Gets the side to which this window controls widget belongs.
 *
 * Returns: the side
 */
BobguiPackType
bobgui_window_controls_get_side (BobguiWindowControls *self)
{
  g_return_val_if_fail (BOBGUI_IS_WINDOW_CONTROLS (self), BOBGUI_PACK_START);

  return self->side;
}

/**
 * bobgui_window_controls_set_side:
 * @self: a window controls widget
 * @side: a side
 *
 * Determines which part of decoration layout
 * the window controls widget uses.
 *
 * See [property@Bobgui.WindowControls:decoration-layout].
 */
void
bobgui_window_controls_set_side (BobguiWindowControls *self,
                              BobguiPackType        side)
{
  g_return_if_fail (BOBGUI_IS_WINDOW_CONTROLS (self));

  if (self->side == side)
    return;

  self->side = side;

  switch (side)
    {
    case BOBGUI_PACK_START:
      bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "start");
      bobgui_widget_remove_css_class (BOBGUI_WIDGET (self), "end");
      break;

    case BOBGUI_PACK_END:
      bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "end");
      bobgui_widget_remove_css_class (BOBGUI_WIDGET (self), "start");
      break;

    default:
      g_warning ("Unexpected side: %d", side);
      break;
    }

  update_window_buttons (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_SIDE]);
}

/**
 * bobgui_window_controls_get_decoration_layout:
 * @self: a window controls widget
 *
 * Gets the decoration layout of this window controls widget
 *
 * Returns: (nullable): the decoration layout
 */
const char *
bobgui_window_controls_get_decoration_layout (BobguiWindowControls *self)
{
  g_return_val_if_fail (BOBGUI_IS_WINDOW_CONTROLS (self), NULL);

  return self->decoration_layout;
}

/**
 * bobgui_window_controls_set_decoration_layout:
 * @self: a window controls widget
 * @layout: (nullable): a decoration layout, or `NULL` to unset the layout
 *
 * Sets the decoration layout for the title buttons.
 *
 * This overrides the [property@Bobgui.Settings:bobgui-decoration-layout]
 * setting.
 *
 * The format of the string is button names, separated by commas.
 * A colon separates the buttons that should appear on the left
 * from those on the right. Recognized button names are minimize,
 * maximize, close and icon (the window icon).
 *
 * For example, “icon:minimize,maximize,close” specifies a icon
 * on the left, and minimize, maximize and close buttons on the right.
 *
 * If [property@Bobgui.WindowControls:side] value is [enum@Bobgui.PackType.start],
 * @self will display the part before the colon, otherwise after that.
 */
void
bobgui_window_controls_set_decoration_layout (BobguiWindowControls *self,
                                           const char        *layout)
{
  g_return_if_fail (BOBGUI_IS_WINDOW_CONTROLS (self));

  g_set_str (&self->decoration_layout, layout);

  update_window_buttons (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_DECORATION_LAYOUT]);
}


/**
 * bobgui_window_controls_get_use_native_controls:
 * @self: a window controls widget
 *
 * Returns whether platform native window controls are shown.
 *
 * Returns: true if native window controls are shown
 *
 * Since: 4.18
 */
gboolean
bobgui_window_controls_get_use_native_controls (BobguiWindowControls *self)
{
  g_return_val_if_fail (BOBGUI_IS_WINDOW_CONTROLS (self), FALSE);

  return self->use_native_controls;
}

/**
 * bobgui_window_controls_set_use_native_controls:
 * @self: a window_controls widget
 * @setting: true to show native window controls
 *
 * Sets whether platform native window controls are used.
 *
 * This option shows the "stoplight" buttons on macOS.
 * For Linux, this option has no effect.
 *
 * See also [Using BOBGUI on Apple macOS](osx.html?native-window-controls).
 *
 * Since: 4.18
 */
void
bobgui_window_controls_set_use_native_controls (BobguiWindowControls *self,
                                             gboolean           setting)
{
  g_return_if_fail (BOBGUI_IS_WINDOW_CONTROLS (self));

  setting = setting != FALSE;

  if (self->use_native_controls == setting)
    return;

  self->use_native_controls = setting;

  update_window_buttons (self);

  g_object_notify_by_pspec (G_OBJECT (self), props[PROP_USE_NATIVE_CONTROLS]);
}

/**
 * bobgui_window_controls_get_empty:
 * @self: a window controls widget
 *
 * Gets whether the widget has any window buttons.
 *
 * Returns: true if the widget has window buttons
 */
gboolean
bobgui_window_controls_get_empty (BobguiWindowControls *self)
{
  g_return_val_if_fail (BOBGUI_IS_WINDOW_CONTROLS (self), FALSE);

  return self->empty;
}

