/* bobguishortcutssection.c
 *
 * Copyright (C) 2015 Christian Hergert <christian@hergert.me>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "bobguishortcutssection.h"

#include "bobguibox.h"
#include "bobguibuildable.h"
#include "bobguishortcutsgroup.h"
#include "bobguibutton.h"
#include "bobguilabel.h"
#include "bobguistack.h"
#include "bobguistackswitcher.h"
#include "bobguiorientable.h"
#include "bobguisizegroup.h"
#include "bobguiwidget.h"
#include "bobguiprivate.h"
#include "bobguimarshalers.h"
#include "bobguigesturepan.h"
#include "bobguiwidgetprivate.h"
#include "bobguicenterbox.h"
#include <glib/gi18n-lib.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiShortcutsSection:
 *
 * A `BobguiShortcutsSection` collects all the keyboard shortcuts and gestures
 * for a major application mode.
 *
 * If your application needs multiple sections, you should give each
 * section a unique [property@Bobgui.ShortcutsSection:section-name] and
 * a [property@Bobgui.ShortcutsSection:title] that can be shown in the
 * section selector of the [class@Bobgui.ShortcutsWindow].
 *
 * The [property@Bobgui.ShortcutsSection:max-height] property can be used
 * to influence how the groups in the section are distributed over pages
 * and columns.
 *
 * This widget is only meant to be used with [class@Bobgui.ShortcutsWindow].
 *
 * The recommended way to construct a `BobguiShortcutsSection` is with
 * [class@Bobgui.Builder], by using the `<child>` tag to populate a
 * `BobguiShortcutsSection` with one or more [class@Bobgui.ShortcutsGroup]
 * instances, which in turn contain one or more [class@Bobgui.ShortcutsShortcut]
 * objects.
 *
 * If you need to add a group programmatically, use
 * [method@Bobgui.ShortcutsSection.add_group].
 *
 * # Shortcuts and Gestures
 *
 * Pan gestures allow to navigate between sections.
 *
 * The following signals have default keybindings:
 *
 * - [signal@Bobgui.ShortcutsSection::change-current-page]
 *
 * Deprecated: 4.18: This widget will be removed in BOBGUI 5
 */

struct _BobguiShortcutsSection
{
  BobguiBox            parent_instance;

  char             *name;
  char             *title;
  char             *view_name;
  guint             max_height;

  BobguiStack         *stack;
  BobguiStackSwitcher *switcher;
  BobguiWidget        *show_all;
  BobguiWidget        *footer;
  GList            *groups;

  gboolean          has_filtered_group;
};

struct _BobguiShortcutsSectionClass
{
  BobguiBoxClass parent_class;

  gboolean (* change_current_page) (BobguiShortcutsSection *self,
                                    int                  offset);

};

static void bobgui_shortcuts_section_buildable_iface_init (BobguiBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiShortcutsSection, bobgui_shortcuts_section, BOBGUI_TYPE_BOX,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_shortcuts_section_buildable_iface_init))

enum {
  PROP_0,
  PROP_TITLE,
  PROP_SECTION_NAME,
  PROP_VIEW_NAME,
  PROP_MAX_HEIGHT,
  LAST_PROP
};

enum {
  CHANGE_CURRENT_PAGE,
  LAST_SIGNAL
};

static GParamSpec *properties[LAST_PROP];
static guint signals[LAST_SIGNAL];

static void bobgui_shortcuts_section_set_view_name    (BobguiShortcutsSection *self,
                                                    const char          *view_name);
static void bobgui_shortcuts_section_set_max_height   (BobguiShortcutsSection *self,
                                                    guint                max_height);

static void bobgui_shortcuts_section_show_all         (BobguiShortcutsSection *self);
static void bobgui_shortcuts_section_filter_groups    (BobguiShortcutsSection *self);
static void bobgui_shortcuts_section_reflow_groups    (BobguiShortcutsSection *self);

static gboolean bobgui_shortcuts_section_change_current_page (BobguiShortcutsSection *self,
                                                           int                  offset);

static void bobgui_shortcuts_section_pan_gesture_pan (BobguiGesturePan       *gesture,
                                                   BobguiPanDirection      direction,
                                                   double               offset,
                                                   BobguiShortcutsSection *self);

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_shortcuts_section_buildable_add_child (BobguiBuildable *buildable,
                                           BobguiBuilder   *builder,
                                           GObject      *child,
                                           const char   *type)
{
  if (BOBGUI_IS_SHORTCUTS_GROUP (child))
    bobgui_shortcuts_section_add_group (BOBGUI_SHORTCUTS_SECTION (buildable), BOBGUI_SHORTCUTS_GROUP (child));
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
bobgui_shortcuts_section_buildable_iface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_shortcuts_section_buildable_add_child;
}

static void
map_child (BobguiWidget *child)
{
  if (_bobgui_widget_get_visible (child) &&
      _bobgui_widget_get_child_visible (child) &&
      !_bobgui_widget_get_mapped (child))
    bobgui_widget_map (child);
}

static void
bobgui_shortcuts_section_map (BobguiWidget *widget)
{
  BobguiShortcutsSection *self = BOBGUI_SHORTCUTS_SECTION (widget);

  BOBGUI_WIDGET_CLASS (bobgui_shortcuts_section_parent_class)->map (widget);

  map_child (BOBGUI_WIDGET (self->stack));
  map_child (BOBGUI_WIDGET (self->footer));
}

static void
bobgui_shortcuts_section_unmap (BobguiWidget *widget)
{
  BobguiShortcutsSection *self = BOBGUI_SHORTCUTS_SECTION (widget);

  BOBGUI_WIDGET_CLASS (bobgui_shortcuts_section_parent_class)->unmap (widget);

  bobgui_widget_unmap (BOBGUI_WIDGET (self->footer));
  bobgui_widget_unmap (BOBGUI_WIDGET (self->stack));
}

static void
bobgui_shortcuts_section_dispose (GObject *object)
{
  BobguiShortcutsSection *self = BOBGUI_SHORTCUTS_SECTION (object);

  g_clear_pointer ((BobguiWidget **)&self->stack, bobgui_widget_unparent);
  g_clear_pointer (&self->footer, bobgui_widget_unparent);

  g_list_free (self->groups);
  self->groups = NULL;

  G_OBJECT_CLASS (bobgui_shortcuts_section_parent_class)->dispose (object);
}

static void
bobgui_shortcuts_section_finalize (GObject *object)
{
  BobguiShortcutsSection *self = (BobguiShortcutsSection *)object;

  g_clear_pointer (&self->name, g_free);
  g_clear_pointer (&self->title, g_free);
  g_clear_pointer (&self->view_name, g_free);

  G_OBJECT_CLASS (bobgui_shortcuts_section_parent_class)->finalize (object);
}

static void
bobgui_shortcuts_section_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  BobguiShortcutsSection *self = (BobguiShortcutsSection *)object;

  switch (prop_id)
    {
    case PROP_SECTION_NAME:
      g_value_set_string (value, self->name);
      break;

    case PROP_VIEW_NAME:
      g_value_set_string (value, self->view_name);
      break;

    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;

    case PROP_MAX_HEIGHT:
      g_value_set_uint (value, self->max_height);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_shortcuts_section_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  BobguiShortcutsSection *self = (BobguiShortcutsSection *)object;

  switch (prop_id)
    {
    case PROP_SECTION_NAME:
      g_free (self->name);
      self->name = g_value_dup_string (value);
      break;

    case PROP_VIEW_NAME:
      bobgui_shortcuts_section_set_view_name (self, g_value_get_string (value));
      break;

    case PROP_TITLE:
      g_free (self->title);
      self->title = g_value_dup_string (value);
      break;

    case PROP_MAX_HEIGHT:
      bobgui_shortcuts_section_set_max_height (self, g_value_get_uint (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_shortcuts_section_class_init (BobguiShortcutsSectionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->finalize = bobgui_shortcuts_section_finalize;
  object_class->dispose = bobgui_shortcuts_section_dispose;
  object_class->get_property = bobgui_shortcuts_section_get_property;
  object_class->set_property = bobgui_shortcuts_section_set_property;

  widget_class->map = bobgui_shortcuts_section_map;
  widget_class->unmap = bobgui_shortcuts_section_unmap;

  klass->change_current_page = bobgui_shortcuts_section_change_current_page;

  /**
   * BobguiShortcutsSection:section-name:
   *
   * A unique name to identify this section among the sections
   * added to the `BobguiShortcutsWindow`.
   *
   * Setting the [property@Bobgui.ShortcutsWindow:section-name] property
   * to this string will make this section shown in the `BobguiShortcutsWindow`.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  properties[PROP_SECTION_NAME] =
    g_param_spec_string ("section-name", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * BobguiShortcutsSection:view-name:
   *
   * A view name to filter the groups in this section by.
   *
   * See [property@Bobgui.ShortcutsGroup:view].
   *
   * Applications are expected to use the
   * [property@Bobgui.ShortcutsWindow:view-name] property
   * for this purpose.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  properties[PROP_VIEW_NAME] =
    g_param_spec_string ("view-name", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiShortcutsSection:title:
   *
   * The string to show in the section selector of the `BobguiShortcutsWindow`
   * for this section.
   *
   * If there is only one section, you don't need to set a title,
   * since the section selector will not be shown in this case.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  properties[PROP_TITLE] =
    g_param_spec_string ("title", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * BobguiShortcutsSection:max-height:
   *
   * The maximum number of lines to allow per column.
   *
   * This property can be used to influence how the groups in this
   * section are distributed across pages and columns. The default
   * value of 15 should work in most cases.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  properties[PROP_MAX_HEIGHT] =
    g_param_spec_uint ("max-height", NULL, NULL,
                       0, G_MAXUINT, 15,
                       (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_properties (object_class, LAST_PROP, properties);

  /**
   * BobguiShortcutsSection::change-current-page:
   * @shortcut_section: the shortcut section
   * @offset: the offset
   *
   * Emitted when we change the current page.
   *
   * The default bindings for this signal are
   * <kbd>Ctrl</kbd>+<kbd>PgUp</kbd>, <kbd>PgUp</kbd>,
   * <kbd>Ctrl</kbd>+<kbd>PgDn</kbd>, <kbd>PgDn</kbd>.
   *
   * Returns: whether the page was changed
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  signals[CHANGE_CURRENT_PAGE] =
    g_signal_new (I_("change-current-page"),
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiShortcutsSectionClass, change_current_page),
                  NULL, NULL,
                  _bobgui_marshal_BOOLEAN__INT,
                  G_TYPE_BOOLEAN, 1,
                  G_TYPE_INT);
  g_signal_set_va_marshaller (signals[CHANGE_CURRENT_PAGE],
                              G_TYPE_FROM_CLASS (object_class),
                              _bobgui_marshal_BOOLEAN__INTv);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Page_Up, 0,
                                       "change-current-page",
                                       "(i)", -1);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Page_Down, 0,
                                       "change-current-page",
                                       "(i)", 1);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Page_Up, GDK_CONTROL_MASK,
                                       "change-current-page",
                                       "(i)", -1);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Page_Down, GDK_CONTROL_MASK,
                                       "change-current-page",
                                       "(i)", 1);

  bobgui_widget_class_set_css_name (widget_class, I_("shortcuts-section"));
}

static void
bobgui_shortcuts_section_init (BobguiShortcutsSection *self)
{
  BobguiGesture *gesture;

  self->max_height = 15;

  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (self), BOBGUI_ORIENTATION_VERTICAL);
  bobgui_box_set_homogeneous (BOBGUI_BOX (self), FALSE);
  bobgui_box_set_spacing (BOBGUI_BOX (self), 22);

  self->stack = g_object_new (BOBGUI_TYPE_STACK,
                              "hhomogeneous", TRUE,
                              "vhomogeneous", TRUE,
                              "transition-type", BOBGUI_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT,
                              "vexpand", TRUE,
                              "visible", TRUE,
                              NULL);
  bobgui_box_append (BOBGUI_BOX (self), BOBGUI_WIDGET (self->stack));

  self->switcher = g_object_new (BOBGUI_TYPE_STACK_SWITCHER,
                                 "halign", BOBGUI_ALIGN_CENTER,
                                 "stack", self->stack,
                                 "visible", FALSE,
                                 NULL);

  bobgui_widget_remove_css_class (BOBGUI_WIDGET (self->switcher), "linked");

  self->show_all = bobgui_button_new_with_mnemonic (_("_Show All"));
  bobgui_widget_set_visible (self->show_all, FALSE);
  g_signal_connect_swapped (self->show_all, "clicked",
                            G_CALLBACK (bobgui_shortcuts_section_show_all), self);

  self->footer = bobgui_center_box_new ();
  bobgui_box_append (BOBGUI_BOX (self), BOBGUI_WIDGET (self->footer));

  bobgui_widget_set_hexpand (BOBGUI_WIDGET (self->switcher), TRUE);
  bobgui_widget_set_halign (BOBGUI_WIDGET (self->switcher), BOBGUI_ALIGN_CENTER);
  bobgui_center_box_set_center_widget (BOBGUI_CENTER_BOX (self->footer), BOBGUI_WIDGET (self->switcher));
  bobgui_center_box_set_end_widget (BOBGUI_CENTER_BOX (self->footer), self->show_all);
  bobgui_widget_set_halign (self->show_all, BOBGUI_ALIGN_END);

  gesture = bobgui_gesture_pan_new (BOBGUI_ORIENTATION_HORIZONTAL);
  g_signal_connect (gesture, "pan",
                    G_CALLBACK (bobgui_shortcuts_section_pan_gesture_pan), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self->stack), BOBGUI_EVENT_CONTROLLER (gesture));
}

static void
bobgui_shortcuts_section_set_view_name (BobguiShortcutsSection *self,
                                     const char          *view_name)
{
  if (g_strcmp0 (self->view_name, view_name) == 0)
    return;

  g_free (self->view_name);
  self->view_name = g_strdup (view_name);

  bobgui_shortcuts_section_filter_groups (self);
  bobgui_shortcuts_section_reflow_groups (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VIEW_NAME]);
}

static void
bobgui_shortcuts_section_set_max_height (BobguiShortcutsSection *self,
                                      guint                max_height)
{
  if (self->max_height == max_height)
    return;

  self->max_height = max_height;

  bobgui_shortcuts_section_reflow_groups (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MAX_HEIGHT]);
}

/**
 * bobgui_shortcuts_section_add_group:
 * @self: a `BobguiShortcutsSection`
 * @group: the `BobguiShortcutsGroup` to add
 *
 * Adds a group to the shortcuts section.
 *
 * This is the programmatic equivalent to using [class@Bobgui.Builder] and a
 * `<child>` tag to add the child.
 * 
 * Adding children with the `BobguiBox` API is not appropriate, as
 * `BobguiShortcutsSection` manages its children internally.
 *
 * Since: 4.14
 *
 * Deprecated: 4.18: This widget will be removed in BOBGUI 5
 */
void
bobgui_shortcuts_section_add_group (BobguiShortcutsSection *self,
                                 BobguiShortcutsGroup   *group)
{
  g_return_if_fail (BOBGUI_IS_SHORTCUTS_SECTION (self));
  g_return_if_fail (BOBGUI_IS_SHORTCUTS_GROUP (group));
  g_return_if_fail (bobgui_widget_get_parent (BOBGUI_WIDGET (group)) == NULL);

  BobguiWidget *page, *column;

  page = bobgui_widget_get_last_child (BOBGUI_WIDGET (self->stack));
  if (page == NULL)
    {
      page = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 22);
      bobgui_stack_add_named (self->stack, page, "1");
    }

  column = bobgui_widget_get_last_child (page);
  if (column == NULL)
    {
      column = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 22);
      bobgui_box_append (BOBGUI_BOX (page), column);
    }

  bobgui_box_append (BOBGUI_BOX (column), BOBGUI_WIDGET (group));
  self->groups = g_list_append (self->groups, group);

  bobgui_shortcuts_section_reflow_groups (self);
}

static void
bobgui_shortcuts_section_show_all (BobguiShortcutsSection *self)
{
  bobgui_shortcuts_section_set_view_name (self, NULL);
}

static void
update_group_visibility (BobguiWidget *child, gpointer data)
{
  BobguiShortcutsSection *self = data;

  if (BOBGUI_IS_SHORTCUTS_GROUP (child))
    {
      char *view;
      gboolean match;

      g_object_get (child, "view", &view, NULL);
      match = view == NULL ||
              self->view_name == NULL ||
              strcmp (view, self->view_name) == 0;

      bobgui_widget_set_visible (child, match);
      self->has_filtered_group |= !match;

      g_free (view);
    }
  else
    {
      for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (child));
           child != NULL;
           child = bobgui_widget_get_next_sibling (child))
        update_group_visibility (child, self);
    }
}

static void
bobgui_shortcuts_section_filter_groups (BobguiShortcutsSection *self)
{
  BobguiWidget *child;

  self->has_filtered_group = FALSE;

  for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (self));
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    update_group_visibility (child, self);

  bobgui_widget_set_visible (BOBGUI_WIDGET (self->show_all), self->has_filtered_group);
  bobgui_widget_set_visible (bobgui_widget_get_parent (BOBGUI_WIDGET (self->show_all)),
                          bobgui_widget_get_visible (BOBGUI_WIDGET (self->show_all)) ||
                          bobgui_widget_get_visible (BOBGUI_WIDGET (self->switcher)));
}

static void
bobgui_shortcuts_section_reflow_groups (BobguiShortcutsSection *self)
{
  GList *pages, *p;
  BobguiWidget *page;
  GList *groups, *g;
  guint n_rows;
  guint n_columns;
  guint n_pages;
  BobguiWidget *current_page, *current_column;

  /* collect all groups from the current pages */
  groups = NULL;
  for (page = bobgui_widget_get_first_child (BOBGUI_WIDGET (self->stack));
       page != NULL;
       page = bobgui_widget_get_next_sibling (page))
    {
      BobguiWidget *column;

      for (column = bobgui_widget_get_last_child (page);
           column != NULL;
           column = bobgui_widget_get_prev_sibling (column))
        {
          BobguiWidget *group;

          for (group = bobgui_widget_get_last_child (column);
               group != NULL;
               group = bobgui_widget_get_prev_sibling (group))
            {
              groups = g_list_prepend (groups, group);
            }
        }
    }

  /* create new pages */
  current_page = NULL;
  current_column = NULL;

  pages = NULL;
  n_rows = 0;
  n_columns = 0;
  n_pages = 0;
  for (g = groups; g; g = g->next)
    {
      BobguiShortcutsGroup *group = g->data;
      guint height;
      gboolean visible;

      g_object_get (group,
                    "visible", &visible,
                    "height", &height,
                    NULL);
      if (!visible)
        height = 0;

      if (current_column == NULL || n_rows + height > self->max_height)
        {
          BobguiWidget *column_box;
          BobguiSizeGroup *size_group;

          column_box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 22);

          size_group = bobgui_size_group_new (BOBGUI_SIZE_GROUP_HORIZONTAL);
          g_object_set_data_full (G_OBJECT (column_box), "accel-size-group", size_group, g_object_unref);
          size_group = bobgui_size_group_new (BOBGUI_SIZE_GROUP_HORIZONTAL);
          g_object_set_data_full (G_OBJECT (column_box), "title-size-group", size_group, g_object_unref);

          if (n_columns % 2 == 0)
            {
              page = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 22);

              pages = g_list_append (pages, page);
              current_page = page;
            }

          bobgui_box_append (BOBGUI_BOX (current_page), column_box);
          current_column = column_box;
          n_columns += 1;
          n_rows = 0;
        }

      n_rows += height;

      g_object_set (group,
                    "accel-size-group", g_object_get_data (G_OBJECT (current_column), "accel-size-group"),
                    "title-size-group", g_object_get_data (G_OBJECT (current_column), "title-size-group"),
                    NULL);

      g_object_ref (group);
      bobgui_box_remove (BOBGUI_BOX (bobgui_widget_get_parent (BOBGUI_WIDGET (group))), BOBGUI_WIDGET (group));
      bobgui_box_append (BOBGUI_BOX (current_column), BOBGUI_WIDGET (group));
      g_object_unref (group);
    }

  /* balance the last page */
  if (n_columns % 2 == 1)
    {
      BobguiWidget *column_box;
      BobguiSizeGroup *size_group;
      GList *content;
      BobguiWidget *child;
      guint n;

      column_box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 22);

      size_group = bobgui_size_group_new (BOBGUI_SIZE_GROUP_HORIZONTAL);
      g_object_set_data_full (G_OBJECT (column_box), "accel-size-group", size_group, g_object_unref);
      size_group = bobgui_size_group_new (BOBGUI_SIZE_GROUP_HORIZONTAL);
      g_object_set_data_full (G_OBJECT (column_box), "title-size-group", size_group, g_object_unref);

      bobgui_box_append (BOBGUI_BOX (current_page), column_box);

      content = NULL;
      for (child = bobgui_widget_get_last_child (current_column);
           child != NULL;
           child = bobgui_widget_get_prev_sibling (child))
        content = g_list_prepend (content, child);
      n = 0;

      for (g = g_list_last (content); g; g = g->prev)
        {
          BobguiShortcutsGroup *group = g->data;
          guint height;
          gboolean visible;

          g_object_get (group,
                        "visible", &visible,
                        "height", &height,
                        NULL);
          if (!visible)
            height = 0;

          if (n_rows - height == 0)
            break;
          if (n_rows - n < 2 * height)
            break;

          n_rows -= height;
          n += height;
        }

      g_assert (g);
      for (g = g->next; g; g = g->next)
        {
          BobguiShortcutsGroup *group = g->data;

          g_object_set (group,
                        "accel-size-group", g_object_get_data (G_OBJECT (column_box), "accel-size-group"),
                        "title-size-group", g_object_get_data (G_OBJECT (column_box), "title-size-group"),
                        NULL);

          g_object_ref (group);
          bobgui_box_remove (BOBGUI_BOX (current_column), BOBGUI_WIDGET (group));
          bobgui_box_append (BOBGUI_BOX (column_box), BOBGUI_WIDGET (group));
          g_object_unref (group);
        }

      g_list_free (content);
    }

  /* replace the current pages with the new pages */
  while ((page = bobgui_widget_get_first_child (BOBGUI_WIDGET (self->stack))))
    bobgui_stack_remove (self->stack, page);

  for (p = pages, n_pages = 0; p; p = p->next, n_pages++)
    {
      char *title;

      page = p->data;
      title = g_strdup_printf ("_%u", n_pages + 1);
      bobgui_stack_add_titled (self->stack, page, title, title);
      g_free (title);
    }

  /* fix up stack switcher */
  {
    BobguiWidget *w;

    bobgui_widget_add_css_class (BOBGUI_WIDGET (self->switcher), "circular");

    for (w = bobgui_widget_get_first_child (BOBGUI_WIDGET (self->switcher));
         w != NULL;
         w = bobgui_widget_get_next_sibling (w))
      {
        BobguiWidget *label;

        bobgui_widget_add_css_class (w, "circular");

        label = bobgui_button_get_child (BOBGUI_BUTTON (w));
        bobgui_label_set_use_underline (BOBGUI_LABEL (label), TRUE);
      }

    bobgui_widget_set_visible (BOBGUI_WIDGET (self->switcher), (n_pages > 1));
    bobgui_widget_set_visible (bobgui_widget_get_parent (BOBGUI_WIDGET (self->switcher)),
                            bobgui_widget_get_visible (BOBGUI_WIDGET (self->show_all)) ||
                            bobgui_widget_get_visible (BOBGUI_WIDGET (self->switcher)));
  }

  /* clean up */
  g_list_free (groups);
  g_list_free (pages);
}

static gboolean
bobgui_shortcuts_section_change_current_page (BobguiShortcutsSection *self,
                                           int                  offset)
{
  BobguiWidget *child;

  child = bobgui_stack_get_visible_child (self->stack);

  if (offset == 1)
    child = bobgui_widget_get_next_sibling (child);
  else if (offset == -1)
    child = bobgui_widget_get_prev_sibling (child);
  else
    g_assert_not_reached ();

  if (child)
    bobgui_stack_set_visible_child (self->stack, child);
  else
    bobgui_widget_error_bell (BOBGUI_WIDGET (self));

  return TRUE;
}

static void
bobgui_shortcuts_section_pan_gesture_pan (BobguiGesturePan       *gesture,
                                       BobguiPanDirection      direction,
                                       double               offset,
                                       BobguiShortcutsSection *self)
{
  if (offset < 50)
    return;

  if (direction == BOBGUI_PAN_DIRECTION_LEFT)
    bobgui_shortcuts_section_change_current_page (self, 1);
  else if (direction == BOBGUI_PAN_DIRECTION_RIGHT)
    bobgui_shortcuts_section_change_current_page (self, -1);
  else
    g_assert_not_reached ();

  bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_DENIED);
}
