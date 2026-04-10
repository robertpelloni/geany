/*
 * Copyright (c) 2013 Red Hat, Inc.
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
 *
 */

#include "config.h"

#include "bobguiheaderbarprivate.h"

#include "bobguibinlayout.h"
#include "bobguibox.h"
#include "bobguibuildable.h"
#include "bobguicenterbox.h"
#include "bobguilabel.h"
#include "bobguiprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"
#include "bobguiwindowcontrols.h"
#include "bobguiwindowhandle.h"
#include "bobguibuilderprivate.h"

#include <string.h>

/**
 * BobguiHeaderBar:
 *
 * Creates a custom titlebar for a window.
 *
 * <picture>
 *   <source srcset="headerbar-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiHeaderBar" src="headerbar.png">
 * </picture>
 *
 * `BobguiHeaderBar` is similar to a horizontal `BobguiCenterBox`. It allows
 * children to be placed at the start or the end. In addition, it allows
 * the window title to be displayed. The title will be centered with respect
 * to the width of the box, even if the children at either side take up
 * different amounts of space.
 *
 * `BobguiHeaderBar` can add typical window frame controls, such as minimize,
 * maximize and close buttons, or the window icon.
 *
 * For these reasons, `BobguiHeaderBar` is the natural choice for use as the
 * custom titlebar widget of a `BobguiWindow` (see [method@Bobgui.Window.set_titlebar]),
 * as it gives features typical of titlebars while allowing the addition of
 * child widgets.
 *
 * ## BobguiHeaderBar as BobguiBuildable
 *
 * The `BobguiHeaderBar` implementation of the `BobguiBuildable` interface supports
 * adding children at the start or end sides by specifying “start” or “end” as
 * the “type” attribute of a `<child>` element, or setting the title widget by
 * specifying “title” value.
 *
 * By default the `BobguiHeaderBar` uses a `BobguiLabel` displaying the title of the
 * window it is contained in as the title widget, equivalent to the following
 * UI definition:
 *
 * ```xml
 * <object class="BobguiHeaderBar">
 *   <property name="title-widget">
 *     <object class="BobguiLabel">
 *       <property name="label" translatable="yes">Label</property>
 *       <property name="single-line-mode">True</property>
 *       <property name="ellipsize">end</property>
 *       <property name="width-chars">5</property>
 *       <style>
 *         <class name="title"/>
 *       </style>
 *     </object>
 *   </property>
 * </object>
 * ```
 *
 * # CSS nodes
 *
 * ```
 * headerbar
 * ╰── windowhandle
 *     ╰── box
 *         ├── box.start
 *         │   ├── windowcontrols.start
 *         │   ╰── [other children]
 *         ├── [Title Widget]
 *         ╰── box.end
 *             ├── [other children]
 *             ╰── windowcontrols.end
 * ```
 *
 * A `BobguiHeaderBar`'s CSS node is called `headerbar`. It contains a `windowhandle`
 * subnode, which contains a `box` subnode, which contains two `box` subnodes at
 * the start and end of the header bar, as well as a center node that represents
 * the title.
 *
 * Each of the boxes contains a `windowcontrols` subnode, see
 * [class@Bobgui.WindowControls] for details, as well as other children.
 *
 * # Accessibility
 *
 * `BobguiHeaderBar` uses the [enum@Bobgui.AccessibleRole.group] role.
 */

#define MIN_TITLE_CHARS 5

struct _BobguiHeaderBar
{
  BobguiWidget container;

  BobguiWidget *handle;
  BobguiWidget *center_box;
  BobguiWidget *start_box;
  BobguiWidget *end_box;

  BobguiWidget *title_label;
  BobguiWidget *title_widget;

  BobguiWidget *start_window_controls;
  BobguiWidget *end_window_controls;

  char *decoration_layout;

  guint show_title_buttons : 1;
  guint use_native_controls : 1;
  guint track_default_decoration : 1;
};

typedef struct _BobguiHeaderBarClass BobguiHeaderBarClass;

struct _BobguiHeaderBarClass
{
  BobguiWidgetClass parent_class;
};

enum {
  PROP_0,
  PROP_TITLE_WIDGET,
  PROP_SHOW_TITLE_BUTTONS,
  PROP_DECORATION_LAYOUT,
  PROP_USE_NATIVE_CONTROLS,
  LAST_PROP
};

static GParamSpec *header_bar_props[LAST_PROP] = { NULL, };

static void bobgui_header_bar_buildable_init (BobguiBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiHeaderBar, bobgui_header_bar, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_header_bar_buildable_init));

static void
create_window_controls (BobguiHeaderBar *bar)
{
  BobguiWidget *controls;

  controls = bobgui_window_controls_new (BOBGUI_PACK_START);
  g_object_bind_property (bar, "decoration-layout",
                          controls, "decoration-layout",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (bar, "use-native-controls",
                          controls, "use-native-controls",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (controls, "empty",
                          controls, "visible",
                          G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);
  bobgui_box_prepend (BOBGUI_BOX (bar->start_box), controls);
  bar->start_window_controls = controls;

  controls = bobgui_window_controls_new (BOBGUI_PACK_END);
  g_object_bind_property (bar, "decoration-layout",
                          controls, "decoration-layout",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (bar, "use-native-controls",
                          controls, "use-native-controls",
                          G_BINDING_SYNC_CREATE);
  g_object_bind_property (controls, "empty",
                          controls, "visible",
                          G_BINDING_SYNC_CREATE | G_BINDING_INVERT_BOOLEAN);
  bobgui_box_append (BOBGUI_BOX (bar->end_box), controls);
  bar->end_window_controls = controls;
}

static void
update_default_decoration (BobguiHeaderBar *bar)
{
  gboolean have_children = FALSE;

  /* Check whether we have any child widgets that we didn't add ourselves */
  if (bobgui_center_box_get_center_widget (BOBGUI_CENTER_BOX (bar->center_box)) != NULL)
    {
      have_children = TRUE;
    }
  else
    {
      BobguiWidget *w;

      for (w = _bobgui_widget_get_first_child (bar->start_box);
           w != NULL;
           w = _bobgui_widget_get_next_sibling (w))
        {
          if (w != bar->start_window_controls)
            {
              have_children = TRUE;
              break;
            }
        }

      if (!have_children)
        for (w = _bobgui_widget_get_first_child (bar->end_box);
             w != NULL;
             w = _bobgui_widget_get_next_sibling (w))
          {
            if (w != bar->end_window_controls)
              {
                have_children = TRUE;
                break;
              }
          }
    }

  if (have_children || bar->title_widget != NULL)
    bobgui_widget_remove_css_class (BOBGUI_WIDGET (bar), "default-decoration");
  else
    bobgui_widget_add_css_class (BOBGUI_WIDGET (bar), "default-decoration");
}

void
_bobgui_header_bar_track_default_decoration (BobguiHeaderBar *bar)
{
  g_assert (BOBGUI_IS_HEADER_BAR (bar));

  bar->track_default_decoration = TRUE;

  update_default_decoration (bar);
}

static void
update_title (BobguiHeaderBar *bar)
{
  BobguiRoot *root;
  const char *title = NULL;

  if (!bar->title_label)
    return;

  root = bobgui_widget_get_root (BOBGUI_WIDGET (bar));

  if (BOBGUI_IS_WINDOW (root))
    title = bobgui_window_get_title (BOBGUI_WINDOW (root));

  if (!title)
    title = g_get_application_name ();

  if (!title)
    title = g_get_prgname ();

  bobgui_label_set_text (BOBGUI_LABEL (bar->title_label), title);
}

static void
construct_title_label (BobguiHeaderBar *bar)
{
  BobguiWidget *label;

  g_assert (bar->title_label == NULL);

  label = bobgui_label_new (NULL);
  bobgui_widget_add_css_class (label, "title");
  bobgui_widget_set_valign (label, BOBGUI_ALIGN_CENTER);
  bobgui_label_set_wrap (BOBGUI_LABEL (label), FALSE);
  bobgui_label_set_single_line_mode (BOBGUI_LABEL (label), TRUE);
  bobgui_label_set_ellipsize (BOBGUI_LABEL (label), PANGO_ELLIPSIZE_END);
  bobgui_label_set_width_chars (BOBGUI_LABEL (label), MIN_TITLE_CHARS);
  bobgui_center_box_set_center_widget (BOBGUI_CENTER_BOX (bar->center_box), label);

  bar->title_label = label;

  update_title (bar);
}

/**
 * bobgui_header_bar_set_title_widget:
 * @bar: a header bar
 * @title_widget: (nullable): a widget to use for a title
 *
 * Sets the title for the header bar.
 *
 * When set to `NULL`, the headerbar will display the title of
 * the window it is contained in.
 *
 * The title should help a user identify the current view.
 * To achieve the same style as the builtin title, use the
 * “title” style class.
 *
 * You should set the title widget to `NULL`, for the window
 * title label to be visible again.
 */
void
bobgui_header_bar_set_title_widget (BobguiHeaderBar *bar,
                                 BobguiWidget    *title_widget)
{
  g_return_if_fail (BOBGUI_IS_HEADER_BAR (bar));
  g_return_if_fail (title_widget == NULL || bar->title_widget == title_widget || bobgui_widget_get_parent (title_widget) == NULL);

  /* No need to do anything if the title widget stays the same */
  if (bar->title_widget == title_widget)
    return;

  bobgui_center_box_set_center_widget (BOBGUI_CENTER_BOX (bar->center_box), NULL);
  bar->title_widget = NULL;

  if (title_widget != NULL)
    {
      bar->title_widget = title_widget;

      bobgui_center_box_set_center_widget (BOBGUI_CENTER_BOX (bar->center_box), title_widget);

      bar->title_label = NULL;
    }
  else
    {
      if (bar->title_label == NULL)
        construct_title_label (bar);
    }

  g_object_notify_by_pspec (G_OBJECT (bar), header_bar_props[PROP_TITLE_WIDGET]);
}

/**
 * bobgui_header_bar_get_title_widget:
 * @bar: a header bar
 *
 * Retrieves the title widget of the header bar.
 *
 * See [method@Bobgui.HeaderBar.set_title_widget].
 *
 * Returns: (nullable) (transfer none): the title widget
 */
BobguiWidget *
bobgui_header_bar_get_title_widget (BobguiHeaderBar *bar)
{
  g_return_val_if_fail (BOBGUI_IS_HEADER_BAR (bar), NULL);

  return bar->title_widget;
}

static void
bobgui_header_bar_root (BobguiWidget *widget)
{
  BobguiWidget *root;

  BOBGUI_WIDGET_CLASS (bobgui_header_bar_parent_class)->root (widget);

  root = BOBGUI_WIDGET (bobgui_widget_get_root (widget));

  if (BOBGUI_IS_WINDOW (root))
    g_signal_connect_swapped (root, "notify::title",
                              G_CALLBACK (update_title), widget);

  update_title (BOBGUI_HEADER_BAR (widget));
}

static void
bobgui_header_bar_unroot (BobguiWidget *widget)
{
  g_signal_handlers_disconnect_by_func (bobgui_widget_get_root (widget),
                                        update_title, widget);

  BOBGUI_WIDGET_CLASS (bobgui_header_bar_parent_class)->unroot (widget);
}

static void
bobgui_header_bar_dispose (GObject *object)
{
  BobguiHeaderBar *bar = BOBGUI_HEADER_BAR (object);

  bar->title_widget = NULL;
  bar->title_label = NULL;
  bar->start_box = NULL;
  bar->end_box = NULL;

  g_clear_pointer (&bar->handle, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_header_bar_parent_class)->dispose (object);
}

static void
bobgui_header_bar_finalize (GObject *object)
{
  BobguiHeaderBar *bar = BOBGUI_HEADER_BAR (object);

  g_free (bar->decoration_layout);

  G_OBJECT_CLASS (bobgui_header_bar_parent_class)->finalize (object);
}

static void
bobgui_header_bar_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  BobguiHeaderBar *bar = BOBGUI_HEADER_BAR (object);

  switch (prop_id)
    {
    case PROP_TITLE_WIDGET:
      g_value_set_object (value, bar->title_widget);
      break;

    case PROP_SHOW_TITLE_BUTTONS:
      g_value_set_boolean (value, bobgui_header_bar_get_show_title_buttons (bar));
      break;

    case PROP_DECORATION_LAYOUT:
      g_value_set_string (value, bobgui_header_bar_get_decoration_layout (bar));
      break;

    case PROP_USE_NATIVE_CONTROLS:
      g_value_set_boolean (value, bobgui_header_bar_get_use_native_controls (bar));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_header_bar_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  BobguiHeaderBar *bar = BOBGUI_HEADER_BAR (object);

  switch (prop_id)
    {
    case PROP_TITLE_WIDGET:
      bobgui_header_bar_set_title_widget (bar, g_value_get_object (value));
      break;

    case PROP_SHOW_TITLE_BUTTONS:
      bobgui_header_bar_set_show_title_buttons (bar, g_value_get_boolean (value));
      break;

    case PROP_DECORATION_LAYOUT:
      bobgui_header_bar_set_decoration_layout (bar, g_value_get_string (value));
      break;

    case PROP_USE_NATIVE_CONTROLS:
      bobgui_header_bar_set_use_native_controls (bar, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_header_bar_pack (BobguiHeaderBar *bar,
                     BobguiWidget    *widget,
                     BobguiPackType   pack_type)
{
  g_return_if_fail (bobgui_widget_get_parent (widget) == NULL);

  if (pack_type == BOBGUI_PACK_START)
    {
      bobgui_box_append (BOBGUI_BOX (bar->start_box), widget);
    }
  else if (pack_type == BOBGUI_PACK_END)
    {
      bobgui_box_prepend (BOBGUI_BOX (bar->end_box), widget);
    }

  if (bar->track_default_decoration)
    update_default_decoration (bar);
}

/**
 * bobgui_header_bar_remove:
 * @bar: a header bar
 * @child: the child to remove
 *
 * Removes a child from the header bar.
 *
 * The child must have been added with
 * [method@Bobgui.HeaderBar.pack_start],
 * [method@Bobgui.HeaderBar.pack_end] or
 * [method@Bobgui.HeaderBar.set_title_widget].
 */
void
bobgui_header_bar_remove (BobguiHeaderBar *bar,
                       BobguiWidget    *child)
{
  BobguiWidget *parent;
  gboolean removed = FALSE;

  parent = bobgui_widget_get_parent (child);

  if (parent == bar->start_box)
    {
      bobgui_box_remove (BOBGUI_BOX (bar->start_box), child);
      removed = TRUE;
    }
  else if (parent == bar->end_box)
    {
      bobgui_box_remove (BOBGUI_BOX (bar->end_box), child);
      removed = TRUE;
    }
  else if (parent == bar->center_box)
    {
      bobgui_center_box_set_center_widget (BOBGUI_CENTER_BOX (bar->center_box), NULL);
      removed = TRUE;
    }

  if (removed && bar->track_default_decoration)
    update_default_decoration (bar);
}

static BobguiSizeRequestMode
bobgui_header_bar_get_request_mode (BobguiWidget *widget)
{
  BobguiWidget *w;
  int wfh = 0, hfw = 0;

  for (w = bobgui_widget_get_first_child (widget);
       w != NULL;
       w = bobgui_widget_get_next_sibling (w))
    {
      BobguiSizeRequestMode mode = bobgui_widget_get_request_mode (w);

      switch (mode)
        {
        case BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH:
          hfw ++;
          break;
        case BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT:
          wfh ++;
          break;
        case BOBGUI_SIZE_REQUEST_CONSTANT_SIZE:
        default:
          break;
        }
    }

  if (hfw == 0 && wfh == 0)
    return BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;
  else
    return wfh > hfw ?
        BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT :
        BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
bobgui_header_bar_class_init (BobguiHeaderBarClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = bobgui_header_bar_dispose;
  object_class->finalize = bobgui_header_bar_finalize;
  object_class->get_property = bobgui_header_bar_get_property;
  object_class->set_property = bobgui_header_bar_set_property;

  widget_class->root = bobgui_header_bar_root;
  widget_class->unroot = bobgui_header_bar_unroot;
  widget_class->get_request_mode = bobgui_header_bar_get_request_mode;

  /**
   * BobguiHeaderBar:title-widget:
   *
   * The title widget to display.
   */
  header_bar_props[PROP_TITLE_WIDGET] =
      g_param_spec_object ("title-widget", NULL, NULL,
                           BOBGUI_TYPE_WIDGET,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiHeaderBar:show-title-buttons:
   *
   * Whether to show title buttons like close, minimize, maximize.
   *
   * Which buttons are actually shown and where is determined
   * by the [property@Bobgui.HeaderBar:decoration-layout] property,
   * and by the state of the window (e.g. a close button will not
   * be shown if the window can't be closed).
   */
  header_bar_props[PROP_SHOW_TITLE_BUTTONS] =
      g_param_spec_boolean ("show-title-buttons", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiHeaderBar:decoration-layout:
   *
   * The decoration layout for buttons.
   *
   * If this property is not set, the
   * [property@Bobgui.Settings:bobgui-decoration-layout] setting is used.
   */
  header_bar_props[PROP_DECORATION_LAYOUT] =
      g_param_spec_string ("decoration-layout", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE);

  /**
   * BobguiHeaderBar:use-native-controls:
   *
   * Whether to show platform native close/minimize/maximize buttons.
   *
   * For macOS, the [property@Bobgui.HeaderBar:decoration-layout] property
   * can be used to enable/disable controls.
   *
   * On Linux, this option has no effect.
   *
   * See also [Using BOBGUI on Apple macOS](osx.html?native-window-controls).
   *
   * Since: 4.18
   */
  header_bar_props[PROP_USE_NATIVE_CONTROLS] =
      g_param_spec_boolean ("use-native-controls", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, header_bar_props);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, I_("headerbar"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GROUP);
}

static void
bobgui_header_bar_init (BobguiHeaderBar *bar)
{
  bar->title_widget = NULL;
  bar->decoration_layout = NULL;
  bar->show_title_buttons = TRUE;
  bar->use_native_controls = FALSE;

  bar->handle = bobgui_window_handle_new ();
  bobgui_widget_set_parent (bar->handle, BOBGUI_WIDGET (bar));

  bar->center_box = bobgui_center_box_new ();
  bobgui_window_handle_set_child (BOBGUI_WINDOW_HANDLE (bar->handle), bar->center_box);

  bar->start_box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_add_css_class (bar->start_box, "start");
  bobgui_center_box_set_start_widget (BOBGUI_CENTER_BOX (bar->center_box), bar->start_box);

  bar->end_box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_add_css_class (bar->end_box, "end");
  bobgui_center_box_set_end_widget (BOBGUI_CENTER_BOX (bar->center_box), bar->end_box);

  construct_title_label (bar);
  create_window_controls (bar);
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_header_bar_buildable_add_child (BobguiBuildable *buildable,
                                    BobguiBuilder   *builder,
                                    GObject      *child,
                                    const char   *type)
{
  if (g_strcmp0 (type, "title") == 0)
    {
      bobgui_buildable_child_deprecation_warning (buildable, builder, "title", "title-widget");
      bobgui_header_bar_set_title_widget (BOBGUI_HEADER_BAR (buildable), BOBGUI_WIDGET (child));
    }
  else if (g_strcmp0 (type, "start") == 0)
    {
      bobgui_header_bar_pack_start (BOBGUI_HEADER_BAR (buildable), BOBGUI_WIDGET (child));
    }
  else if (g_strcmp0 (type, "end") == 0)
    {
      bobgui_header_bar_pack_end (BOBGUI_HEADER_BAR (buildable), BOBGUI_WIDGET (child));
    }
  else if (type == NULL && BOBGUI_IS_WIDGET (child))
    {
      bobgui_header_bar_pack_start (BOBGUI_HEADER_BAR (buildable), BOBGUI_WIDGET (child));
    }
  else
    {
      parent_buildable_iface->add_child (buildable, builder, child, type);
    }
}

static void
bobgui_header_bar_buildable_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_header_bar_buildable_add_child;
}

/**
 * bobgui_header_bar_pack_start:
 * @bar: A header bar
 * @child: the widget to be added to @bar
 *
 * Adds a child to the header bar, packed with reference to the start.
 */
void
bobgui_header_bar_pack_start (BobguiHeaderBar *bar,
                           BobguiWidget    *child)
{
  bobgui_header_bar_pack (bar, child, BOBGUI_PACK_START);
}

/**
 * bobgui_header_bar_pack_end:
 * @bar: A header bar
 * @child: the widget to be added to @bar
 *
 * Adds a child to the header bar, packed with reference to the end.
 */
void
bobgui_header_bar_pack_end (BobguiHeaderBar *bar,
                         BobguiWidget    *child)
{
  bobgui_header_bar_pack (bar, child, BOBGUI_PACK_END);
}

/**
 * bobgui_header_bar_new:
 *
 * Creates a new `BobguiHeaderBar` widget.
 *
 * Returns: a new `BobguiHeaderBar`
 */
BobguiWidget *
bobgui_header_bar_new (void)
{
  return BOBGUI_WIDGET (g_object_new (BOBGUI_TYPE_HEADER_BAR, NULL));
}

/**
 * bobgui_header_bar_get_show_title_buttons:
 * @bar: a header bar
 *
 * Returns whether this header bar shows the standard window
 * title buttons.
 *
 * Returns: true if title buttons are shown
 */
gboolean
bobgui_header_bar_get_show_title_buttons (BobguiHeaderBar *bar)
{
  g_return_val_if_fail (BOBGUI_IS_HEADER_BAR (bar), FALSE);

  return bar->show_title_buttons;
}

/**
 * bobgui_header_bar_set_show_title_buttons:
 * @bar: a header bar
 * @setting: true to show standard title buttons
 *
 * Sets whether this header bar shows the standard window
 * title buttons.
 */
void
bobgui_header_bar_set_show_title_buttons (BobguiHeaderBar *bar,
                                       gboolean      setting)
{
  g_return_if_fail (BOBGUI_IS_HEADER_BAR (bar));

  setting = setting != FALSE;

  if (bar->show_title_buttons == setting)
    return;

  bar->show_title_buttons = setting;

  if (setting)
    create_window_controls (bar);
  else
    {
      if (bar->start_box && bar->start_window_controls)
        {
          bobgui_box_remove (BOBGUI_BOX (bar->start_box), bar->start_window_controls);
          bar->start_window_controls = NULL;
        }

      if (bar->end_box && bar->end_window_controls)
        {
          bobgui_box_remove (BOBGUI_BOX (bar->end_box), bar->end_window_controls);
          bar->end_window_controls = NULL;
        }
    }

  g_object_notify_by_pspec (G_OBJECT (bar), header_bar_props[PROP_SHOW_TITLE_BUTTONS]);
}

/**
 * bobgui_header_bar_set_decoration_layout:
 * @bar: a header bar
 * @layout: (nullable): a decoration layout
 *
 * Sets the decoration layout for this header bar.
 *
 * This property overrides the
 * [property@Bobgui.Settings:bobgui-decoration-layout] setting.
 *
 * There can be valid reasons for overriding the setting, such
 * as a header bar design that does not allow for buttons to take
 * room on the right, or only offers room for a single close button.
 * Split header bars are another example for overriding the setting.
 *
 * The format of the string is button names, separated by commas.
 * A colon separates the buttons that should appear on the left
 * from those on the right. Recognized button names are minimize,
 * maximize, close and icon (the window icon).
 *
 * For example, “icon:minimize,maximize,close” specifies an icon
 * on the left, and minimize, maximize and close buttons on the right.
 */
void
bobgui_header_bar_set_decoration_layout (BobguiHeaderBar *bar,
                                      const char   *layout)
{
  g_return_if_fail (BOBGUI_IS_HEADER_BAR (bar));

  g_free (bar->decoration_layout);
  bar->decoration_layout = g_strdup (layout);

  g_object_notify_by_pspec (G_OBJECT (bar), header_bar_props[PROP_DECORATION_LAYOUT]);
}

/**
 * bobgui_header_bar_get_decoration_layout:
 * @bar: a header bar
 *
 * Gets the decoration layout of the header bar.
 *
 * Returns: (nullable): the decoration layout
 */
const char *
bobgui_header_bar_get_decoration_layout (BobguiHeaderBar *bar)
{
  g_return_val_if_fail (BOBGUI_IS_HEADER_BAR (bar), NULL);

  return bar->decoration_layout;
}

/**
 * bobgui_header_bar_get_use_native_controls:
 * @bar: a header bar
 *
 * Returns whether this header bar shows platform
 * native window controls.
 *
 * Returns: true if native window controls are shown
 *
 * Since: 4.18
 */
gboolean
bobgui_header_bar_get_use_native_controls (BobguiHeaderBar *bar)
{
  return bar->use_native_controls;
}

/**
 * bobgui_header_bar_set_use_native_controls:
 * @bar: a header bar
 * @setting: true to show native window controls
 *
 * Sets whether this header bar shows native window controls.
 *
 * This option shows the "stoplight" buttons on macOS.
 * For Linux, this option has no effect.
 *
 * See also [Using BOBGUI on Apple macOS](osx.html?native-window-controls).
 *
 * Since: 4.18
 */
void
bobgui_header_bar_set_use_native_controls (BobguiHeaderBar *bar,
                                         gboolean      setting)
{
  g_return_if_fail (BOBGUI_IS_HEADER_BAR (bar));

  setting = setting != FALSE;

  if (bar->use_native_controls == setting)
    return;

  bar->use_native_controls = setting;

  g_object_notify_by_pspec (G_OBJECT (bar), header_bar_props[PROP_USE_NATIVE_CONTROLS]);
}
