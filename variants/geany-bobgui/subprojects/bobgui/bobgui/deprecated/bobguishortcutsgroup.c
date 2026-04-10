/* bobguishortcutsgroup.c
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

#include "bobguishortcutsgroup.h"

#include "bobguibox.h"
#include "bobguibuildable.h"
#include "bobguilabel.h"
#include "bobguiorientable.h"
#include "bobguiprivate.h"
#include "bobguishortcutsshortcut.h"
#include "bobguisizegroup.h"
#include "bobguiaccessible.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiShortcutsGroup:
 *
 * A `BobguiShortcutsGroup` represents a group of related keyboard shortcuts
 * or gestures.
 *
 * The group has a title. It may optionally be associated with a view
 * of the application, which can be used to show only relevant shortcuts
 * depending on the application context.
 *
 * This widget is only meant to be used with [class@Bobgui.ShortcutsWindow].
 *
 * The recommended way to construct a `BobguiShortcutsGroup` is with
 * [class@Bobgui.Builder], by using the `<child>` tag to populate a
 * `BobguiShortcutsGroup` with one or more [class@Bobgui.ShortcutsShortcut]
 * instances.
 *
 * If you need to add a shortcut programmatically, use
 * [method@Bobgui.ShortcutsGroup.add_shortcut].
 *
 * Deprecated: 4.18: This widget will be removed in BOBGUI 5
 */

struct _BobguiShortcutsGroup
{
  BobguiBox    parent_instance;

  BobguiLabel *title;
  char     *view;
  guint     height;

  BobguiSizeGroup *accel_size_group;
  BobguiSizeGroup *title_size_group;
};

struct _BobguiShortcutsGroupClass
{
  BobguiBoxClass parent_class;
};

static void bobgui_shortcuts_group_buildable_iface_init (BobguiBuildableIface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiShortcutsGroup, bobgui_shortcuts_group, BOBGUI_TYPE_BOX,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_shortcuts_group_buildable_iface_init))

enum {
  PROP_0,
  PROP_TITLE,
  PROP_VIEW,
  PROP_ACCEL_SIZE_GROUP,
  PROP_TITLE_SIZE_GROUP,
  PROP_HEIGHT,
  LAST_PROP
};

static GParamSpec *properties[LAST_PROP];

static void
bobgui_shortcuts_group_apply_accel_size_group (BobguiShortcutsGroup *group,
                                            BobguiWidget         *child)
{
  if (BOBGUI_IS_SHORTCUTS_SHORTCUT (child))
    g_object_set (child, "accel-size-group", group->accel_size_group, NULL);
}

static void
bobgui_shortcuts_group_apply_title_size_group (BobguiShortcutsGroup *group,
                                            BobguiWidget         *child)
{
  if (BOBGUI_IS_SHORTCUTS_SHORTCUT (child))
    g_object_set (child, "title-size-group", group->title_size_group, NULL);
}

static void
bobgui_shortcuts_group_set_accel_size_group (BobguiShortcutsGroup *group,
                                          BobguiSizeGroup      *size_group)
{
  BobguiWidget *child;

  g_set_object (&group->accel_size_group, size_group);

  for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (group));
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    bobgui_shortcuts_group_apply_accel_size_group (group, child);
}

static void
bobgui_shortcuts_group_set_title_size_group (BobguiShortcutsGroup *group,
                                          BobguiSizeGroup      *size_group)
{
  BobguiWidget *child;

  g_set_object (&group->title_size_group, size_group);

  for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (group));
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    bobgui_shortcuts_group_apply_title_size_group (group, child);
}

static guint
bobgui_shortcuts_group_get_height (BobguiShortcutsGroup *group)
{
  BobguiWidget *child;
  guint height;

  height = 1;

  for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (group));
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      if (!bobgui_widget_get_visible (child))
        continue;
      else if (BOBGUI_IS_SHORTCUTS_SHORTCUT (child))
        height += 1;
    }

  return height;
}

static BobguiBuildableIface *parent_buildable_iface;

static void
bobgui_shortcuts_group_buildable_add_child (BobguiBuildable *buildable,
                                         BobguiBuilder   *builder,
                                         GObject      *child,
                                         const char   *type)
{
  if (BOBGUI_IS_SHORTCUTS_SHORTCUT (child))
    {
      bobgui_shortcuts_group_add_shortcut (BOBGUI_SHORTCUTS_GROUP (buildable),
                                        BOBGUI_SHORTCUTS_SHORTCUT (child));
    }
  else
    parent_buildable_iface->add_child (buildable, builder, child, type);
}

static void
bobgui_shortcuts_group_buildable_iface_init (BobguiBuildableIface *iface)
{
  parent_buildable_iface = g_type_interface_peek_parent (iface);

  iface->add_child = bobgui_shortcuts_group_buildable_add_child;
}

static void
bobgui_shortcuts_group_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  BobguiShortcutsGroup *self = BOBGUI_SHORTCUTS_GROUP (object);

  switch (prop_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, bobgui_label_get_label (self->title));
      break;

    case PROP_VIEW:
      g_value_set_string (value, self->view);
      break;

    case PROP_HEIGHT:
      g_value_set_uint (value, bobgui_shortcuts_group_get_height (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_shortcuts_group_direction_changed (BobguiWidget        *widget,
                                       BobguiTextDirection  previous_dir)
{
  BOBGUI_WIDGET_CLASS (bobgui_shortcuts_group_parent_class)->direction_changed (widget, previous_dir);
  g_object_notify (G_OBJECT (widget), "height");
}

static void
bobgui_shortcuts_group_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  BobguiShortcutsGroup *self = BOBGUI_SHORTCUTS_GROUP (object);

  switch (prop_id)
    {
    case PROP_TITLE:
      bobgui_label_set_label (self->title, g_value_get_string (value));
      break;

    case PROP_VIEW:
      g_free (self->view);
      self->view = g_value_dup_string (value);
      break;

    case PROP_ACCEL_SIZE_GROUP:
      bobgui_shortcuts_group_set_accel_size_group (self, BOBGUI_SIZE_GROUP (g_value_get_object (value)));
      break;

    case PROP_TITLE_SIZE_GROUP:
      bobgui_shortcuts_group_set_title_size_group (self, BOBGUI_SIZE_GROUP (g_value_get_object (value)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_shortcuts_group_finalize (GObject *object)
{
  BobguiShortcutsGroup *self = BOBGUI_SHORTCUTS_GROUP (object);

  g_free (self->view);
  g_set_object (&self->accel_size_group, NULL);
  g_set_object (&self->title_size_group, NULL);

  G_OBJECT_CLASS (bobgui_shortcuts_group_parent_class)->finalize (object);
}

static void
bobgui_shortcuts_group_dispose (GObject *object)
{
  BobguiShortcutsGroup *self = BOBGUI_SHORTCUTS_GROUP (object);

  g_clear_pointer ((BobguiWidget **)&self->title, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_shortcuts_group_parent_class)->dispose (object);
}

static void
bobgui_shortcuts_group_class_init (BobguiShortcutsGroupClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->finalize = bobgui_shortcuts_group_finalize;
  object_class->get_property = bobgui_shortcuts_group_get_property;
  object_class->set_property = bobgui_shortcuts_group_set_property;
  object_class->dispose = bobgui_shortcuts_group_dispose;

  widget_class->direction_changed = bobgui_shortcuts_group_direction_changed;

  /**
   * BobguiShortcutsGroup:title:
   *
   * The title for this group of shortcuts.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  properties[PROP_TITLE] =
    g_param_spec_string ("title", NULL, NULL,
                         "",
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * BobguiShortcutsGroup:view:
   *
   * An optional view that the shortcuts in this group are relevant for.
   *
   * The group will be hidden if the [property@Bobgui.ShortcutsWindow:view-name]
   * property does not match the view of this group.
   *
   * Set this to %NULL to make the group always visible.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  properties[PROP_VIEW] =
    g_param_spec_string ("view", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * BobguiShortcutsGroup:accel-size-group:
   *
   * The size group for the accelerator portion of shortcuts in this group.
   *
   * This is used internally by BOBGUI, and must not be modified by applications.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  properties[PROP_ACCEL_SIZE_GROUP] =
    g_param_spec_object ("accel-size-group", NULL, NULL,
                         BOBGUI_TYPE_SIZE_GROUP,
                         (G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  /**
   * BobguiShortcutsGroup:title-size-group:
   *
   * The size group for the textual portion of shortcuts in this group.
   *
   * This is used internally by BOBGUI, and must not be modified by applications.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  properties[PROP_TITLE_SIZE_GROUP] =
    g_param_spec_object ("title-size-group", NULL, NULL,
                         BOBGUI_TYPE_SIZE_GROUP,
                         (G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  /**
   * BobguiShortcutsGroup:height:
   *
   * A rough measure for the number of lines in this group.
   *
   * This is used internally by BOBGUI, and is not useful for applications.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  properties[PROP_HEIGHT] =
    g_param_spec_uint ("height", NULL, NULL,
                       0, G_MAXUINT, 1,
                       (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, LAST_PROP, properties);

  bobgui_widget_class_set_css_name (widget_class, I_("shortcuts-group"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GROUP);
}

static void
bobgui_shortcuts_group_init (BobguiShortcutsGroup *self)
{
  PangoAttrList *attrs;

  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (self), BOBGUI_ORIENTATION_VERTICAL);
  bobgui_box_set_spacing (BOBGUI_BOX (self), 10);

  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_weight_new (PANGO_WEIGHT_BOLD));
  self->title = g_object_new (BOBGUI_TYPE_LABEL,
                              "accessible-role", BOBGUI_ACCESSIBLE_ROLE_CAPTION,
                              "attributes", attrs,
                              "visible", TRUE,
                              "xalign", 0.0f,
                              NULL);
  pango_attr_list_unref (attrs);

  bobgui_box_append (BOBGUI_BOX (self), BOBGUI_WIDGET (self->title));

  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (self),
                                  BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, self->title, NULL,
                                  -1);
}

/**
 * bobgui_shortcuts_group_add_shortcut:
 * @self: a `BobguiShortcutsGroup`
 * @shortcut: the `BobguiShortcutsShortcut` to add
 *
 * Adds a shortcut to the shortcuts group.
 *
 * This is the programmatic equivalent to using [class@Bobgui.Builder] and a
 * `<child>` tag to add the child. Adding children with other API is not
 * appropriate as `BobguiShortcutsGroup` manages its children internally.
 *
 * Since: 4.14
 *
 * Deprecated: 4.18: This widget will be removed in BOBGUI 5
 */
void
bobgui_shortcuts_group_add_shortcut (BobguiShortcutsGroup    *self,
                                  BobguiShortcutsShortcut *shortcut)
{
  g_return_if_fail (BOBGUI_IS_SHORTCUTS_GROUP (self));
  g_return_if_fail (BOBGUI_IS_SHORTCUTS_SHORTCUT (shortcut));
  g_return_if_fail (bobgui_widget_get_parent (BOBGUI_WIDGET (shortcut)) == NULL);

  BobguiWidget *widget = BOBGUI_WIDGET (shortcut);
  bobgui_box_append (BOBGUI_BOX (self), widget);
  bobgui_shortcuts_group_apply_accel_size_group (self, widget);
  bobgui_shortcuts_group_apply_title_size_group (self, widget);
}
