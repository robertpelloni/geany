/* bobguishortcutsshortcut.c
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

#include "bobguishortcutsshortcutprivate.h"

#include "bobguiimage.h"
#include "bobguibox.h"
#include <glib/gi18n-lib.h>
#include "bobguilabel.h"
#include "bobguiprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguishortcutlabel.h"
#include "bobguishortcutswindowprivate.h"
#include "bobguisizegroup.h"
#include "bobguitypebuiltins.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiShortcutsShortcut:
 *
 * A `BobguiShortcutsShortcut` represents a single keyboard shortcut or gesture
 * with a short text.
 *
 * This widget is only meant to be used with `BobguiShortcutsWindow`.
 *
 * Deprecated: 4.18: This widget will be removed in BOBGUI 5
 */

struct _BobguiShortcutsShortcut
{
  BobguiWidget         parent_instance;

  BobguiBox           *box;
  BobguiImage         *image;
  BobguiShortcutLabel *accelerator;
  BobguiLabel         *title;
  BobguiLabel         *subtitle;
  BobguiLabel         *title_box;

  BobguiSizeGroup *accel_size_group;
  BobguiSizeGroup *title_size_group;

  gboolean subtitle_set;
  gboolean icon_set;
  BobguiTextDirection direction;
  char *action_name;
  BobguiShortcutType  shortcut_type;
};

struct _BobguiShortcutsShortcutClass
{
  BobguiWidgetClass parent_class;
};

G_DEFINE_TYPE (BobguiShortcutsShortcut, bobgui_shortcuts_shortcut, BOBGUI_TYPE_WIDGET)

enum {
  PROP_0,
  PROP_ACCELERATOR,
  PROP_ICON,
  PROP_ICON_SET,
  PROP_TITLE,
  PROP_SUBTITLE,
  PROP_SUBTITLE_SET,
  PROP_ACCEL_SIZE_GROUP,
  PROP_TITLE_SIZE_GROUP,
  PROP_DIRECTION,
  PROP_SHORTCUT_TYPE,
  PROP_ACTION_NAME,
  LAST_PROP
};

static GParamSpec *properties[LAST_PROP];

static void
bobgui_shortcuts_shortcut_set_accelerator (BobguiShortcutsShortcut *self,
                                        const char           *accelerator)
{
  bobgui_shortcut_label_set_accelerator (self->accelerator, accelerator);
}

static void
bobgui_shortcuts_shortcut_set_accel_size_group (BobguiShortcutsShortcut *self,
                                             BobguiSizeGroup         *group)
{
  if (self->accel_size_group)
    {
      bobgui_size_group_remove_widget (self->accel_size_group, BOBGUI_WIDGET (self->accelerator));
      bobgui_size_group_remove_widget (self->accel_size_group, BOBGUI_WIDGET (self->image));
    }

  if (group)
    {
      bobgui_size_group_add_widget (group, BOBGUI_WIDGET (self->accelerator));
      bobgui_size_group_add_widget (group, BOBGUI_WIDGET (self->image));
    }

  g_set_object (&self->accel_size_group, group);
}

static void
bobgui_shortcuts_shortcut_set_title_size_group (BobguiShortcutsShortcut *self,
                                             BobguiSizeGroup         *group)
{
  if (self->title_size_group)
    bobgui_size_group_remove_widget (self->title_size_group, BOBGUI_WIDGET (self->title_box));
  if (group)
    bobgui_size_group_add_widget (group, BOBGUI_WIDGET (self->title_box));

  g_set_object (&self->title_size_group, group);
}

static void
update_subtitle_from_type (BobguiShortcutsShortcut *self)
{
  const char *subtitle;

  if (self->subtitle_set)
    return;

  switch (self->shortcut_type)
    {
    case BOBGUI_SHORTCUT_ACCELERATOR:
    case BOBGUI_SHORTCUT_GESTURE:
      subtitle = NULL;
      break;

    case BOBGUI_SHORTCUT_GESTURE_PINCH:
      subtitle = _("Two finger pinch");
      break;

    case BOBGUI_SHORTCUT_GESTURE_STRETCH:
      subtitle = _("Two finger stretch");
      break;

    case BOBGUI_SHORTCUT_GESTURE_ROTATE_CLOCKWISE:
      subtitle = _("Rotate clockwise");
      break;

    case BOBGUI_SHORTCUT_GESTURE_ROTATE_COUNTERCLOCKWISE:
      subtitle = _("Rotate counterclockwise");
      break;

    case BOBGUI_SHORTCUT_GESTURE_TWO_FINGER_SWIPE_LEFT:
      subtitle = _("Two finger swipe left");
      break;

    case BOBGUI_SHORTCUT_GESTURE_TWO_FINGER_SWIPE_RIGHT:
      subtitle = _("Two finger swipe right");
      break;

    case BOBGUI_SHORTCUT_GESTURE_SWIPE_LEFT:
      subtitle = _("Swipe left");
      break;

    case BOBGUI_SHORTCUT_GESTURE_SWIPE_RIGHT:
      subtitle = _("Swipe right");
      break;

    default:
      subtitle = NULL;
      break;
    }

  bobgui_label_set_label (self->subtitle, subtitle);
  bobgui_widget_set_visible (BOBGUI_WIDGET (self->subtitle), subtitle != NULL);
  g_object_notify (G_OBJECT (self), "subtitle");
}

static void
bobgui_shortcuts_shortcut_set_subtitle_set (BobguiShortcutsShortcut *self,
                                         gboolean              subtitle_set)
{
  if (self->subtitle_set != subtitle_set)
    {
      self->subtitle_set = subtitle_set;
      g_object_notify (G_OBJECT (self), "subtitle-set");
    }
  update_subtitle_from_type (self);
}

static void
bobgui_shortcuts_shortcut_set_subtitle (BobguiShortcutsShortcut *self,
                                     const char           *subtitle)
{
  bobgui_label_set_label (self->subtitle, subtitle);
  bobgui_widget_set_visible (BOBGUI_WIDGET (self->subtitle), subtitle && subtitle[0]);
  bobgui_shortcuts_shortcut_set_subtitle_set (self, subtitle && subtitle[0]);

  g_object_notify (G_OBJECT (self), "subtitle");
}

static void
update_icon_from_type (BobguiShortcutsShortcut *self)
{
  GIcon *icon;

  if (self->icon_set)
    return;

  switch (self->shortcut_type)
    {
    case BOBGUI_SHORTCUT_GESTURE_PINCH:
      icon = g_themed_icon_new ("gesture-pinch-symbolic");
      break;

    case BOBGUI_SHORTCUT_GESTURE_STRETCH:
      icon = g_themed_icon_new ("gesture-stretch-symbolic");
      break;

    case BOBGUI_SHORTCUT_GESTURE_ROTATE_CLOCKWISE:
      icon = g_themed_icon_new ("gesture-rotate-clockwise-symbolic");
      break;

    case BOBGUI_SHORTCUT_GESTURE_ROTATE_COUNTERCLOCKWISE:
      icon = g_themed_icon_new ("gesture-rotate-anticlockwise-symbolic");
      break;

    case BOBGUI_SHORTCUT_GESTURE_TWO_FINGER_SWIPE_LEFT:
      icon = g_themed_icon_new ("gesture-two-finger-swipe-left-symbolic");
      break;

    case BOBGUI_SHORTCUT_GESTURE_TWO_FINGER_SWIPE_RIGHT:
      icon = g_themed_icon_new ("gesture-two-finger-swipe-right-symbolic");
      break;

    case BOBGUI_SHORTCUT_GESTURE_SWIPE_LEFT:
      icon = g_themed_icon_new ("gesture-swipe-left-symbolic");
      break;

    case BOBGUI_SHORTCUT_GESTURE_SWIPE_RIGHT:
      icon = g_themed_icon_new ("gesture-swipe-right-symbolic");
      break;

    case BOBGUI_SHORTCUT_ACCELERATOR:
    case BOBGUI_SHORTCUT_GESTURE:
    default:
      icon = NULL;
      break;
    }

  if (icon)
    {
      bobgui_image_set_from_gicon (self->image, icon);
      bobgui_image_set_pixel_size (self->image, 64);
      g_object_unref (icon);
    }
}

static void
bobgui_shortcuts_shortcut_set_icon_set (BobguiShortcutsShortcut *self,
                                     gboolean              icon_set)
{
  if (self->icon_set != icon_set)
    {
      self->icon_set = icon_set;
      g_object_notify (G_OBJECT (self), "icon-set");
    }
  update_icon_from_type (self);
}

static void
bobgui_shortcuts_shortcut_set_icon (BobguiShortcutsShortcut *self,
                                 GIcon                *gicon)
{
  bobgui_image_set_from_gicon (self->image, gicon);
  bobgui_shortcuts_shortcut_set_icon_set (self, gicon != NULL);
  g_object_notify (G_OBJECT (self), "icon");
}

static void
update_visible_from_direction (BobguiShortcutsShortcut *self)
{
  bobgui_widget_set_visible (BOBGUI_WIDGET (self),
                          self->direction == BOBGUI_TEXT_DIR_NONE ||
                          self->direction == bobgui_widget_get_direction (BOBGUI_WIDGET (self)));
}

static void
bobgui_shortcuts_shortcut_set_direction (BobguiShortcutsShortcut *self,
                                      BobguiTextDirection      direction)
{
  if (self->direction == direction)
    return;

  self->direction = direction;

  update_visible_from_direction (self);

  g_object_notify (G_OBJECT (self), "direction");
}

static void
bobgui_shortcuts_shortcut_direction_changed (BobguiWidget        *widget,
                                          BobguiTextDirection  previous_dir)
{
  update_visible_from_direction (BOBGUI_SHORTCUTS_SHORTCUT (widget));

  BOBGUI_WIDGET_CLASS (bobgui_shortcuts_shortcut_parent_class)->direction_changed (widget, previous_dir);
}

static void
bobgui_shortcuts_shortcut_set_type (BobguiShortcutsShortcut *self,
                                 BobguiShortcutType       type)
{
  if (self->shortcut_type == type)
    return;

  self->shortcut_type = type;

  update_subtitle_from_type (self);
  update_icon_from_type (self);

  bobgui_widget_set_visible (BOBGUI_WIDGET (self->accelerator), type == BOBGUI_SHORTCUT_ACCELERATOR);
  bobgui_widget_set_visible (BOBGUI_WIDGET (self->image), type != BOBGUI_SHORTCUT_ACCELERATOR);


  g_object_notify (G_OBJECT (self), "shortcut-type");
}

static void
bobgui_shortcuts_shortcut_set_action_name (BobguiShortcutsShortcut *self,
                                        const char           *action_name)
{
  g_free (self->action_name);
  self->action_name = g_strdup (action_name);

  g_object_notify (G_OBJECT (self), "action-name");
}

static void
bobgui_shortcuts_shortcut_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  BobguiShortcutsShortcut *self = BOBGUI_SHORTCUTS_SHORTCUT (object);

  switch (prop_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, bobgui_label_get_label (self->title));
      break;

    case PROP_SUBTITLE:
      g_value_set_string (value, bobgui_label_get_label (self->subtitle));
      break;

    case PROP_SUBTITLE_SET:
      g_value_set_boolean (value, self->subtitle_set);
      break;

    case PROP_ACCELERATOR:
      g_value_set_string (value, bobgui_shortcut_label_get_accelerator (self->accelerator));
      break;

    case PROP_ICON:
      g_value_set_object (value, bobgui_image_get_gicon (self->image));
      break;

    case PROP_ICON_SET:
      g_value_set_boolean (value, self->icon_set);
      break;

    case PROP_DIRECTION:
      g_value_set_enum (value, self->direction);
      break;

    case PROP_SHORTCUT_TYPE:
      g_value_set_enum (value, self->shortcut_type);
      break;

    case PROP_ACTION_NAME:
      g_value_set_string (value, self->action_name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_shortcuts_shortcut_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  BobguiShortcutsShortcut *self = BOBGUI_SHORTCUTS_SHORTCUT (object);

  switch (prop_id)
    {
    case PROP_ACCELERATOR:
      bobgui_shortcuts_shortcut_set_accelerator (self, g_value_get_string (value));
      break;

    case PROP_ICON:
      bobgui_shortcuts_shortcut_set_icon (self, g_value_get_object (value));
      break;

    case PROP_ICON_SET:
      bobgui_shortcuts_shortcut_set_icon_set (self, g_value_get_boolean (value));
      break;

    case PROP_ACCEL_SIZE_GROUP:
      bobgui_shortcuts_shortcut_set_accel_size_group (self, BOBGUI_SIZE_GROUP (g_value_get_object (value)));
      break;

    case PROP_TITLE:
      bobgui_label_set_label (self->title, g_value_get_string (value));
      break;

    case PROP_SUBTITLE:
      bobgui_shortcuts_shortcut_set_subtitle (self, g_value_get_string (value));
      break;

    case PROP_SUBTITLE_SET:
      bobgui_shortcuts_shortcut_set_subtitle_set (self, g_value_get_boolean (value));
      break;

    case PROP_TITLE_SIZE_GROUP:
      bobgui_shortcuts_shortcut_set_title_size_group (self, BOBGUI_SIZE_GROUP (g_value_get_object (value)));
      break;

    case PROP_DIRECTION:
      bobgui_shortcuts_shortcut_set_direction (self, g_value_get_enum (value));
      break;

    case PROP_SHORTCUT_TYPE:
      bobgui_shortcuts_shortcut_set_type (self, g_value_get_enum (value));
      break;

    case PROP_ACTION_NAME:
      bobgui_shortcuts_shortcut_set_action_name (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_shortcuts_shortcut_finalize (GObject *object)
{
  BobguiShortcutsShortcut *self = BOBGUI_SHORTCUTS_SHORTCUT (object);

  g_clear_object (&self->accel_size_group);
  g_clear_object (&self->title_size_group);
  g_free (self->action_name);
  bobgui_widget_unparent (BOBGUI_WIDGET (self->box));

  G_OBJECT_CLASS (bobgui_shortcuts_shortcut_parent_class)->finalize (object);
}

void
bobgui_shortcuts_shortcut_update_accel (BobguiShortcutsShortcut *self,
                                     BobguiWindow            *window)
{
  BobguiApplication *app;
  char **accels;
  char *str;

  if (self->action_name == NULL)
    return;

  app = bobgui_window_get_application (window);
  if (app == NULL)
    return;

  accels = bobgui_application_get_accels_for_action (app, self->action_name);
  str = g_strjoinv (" ", accels);

  bobgui_shortcuts_shortcut_set_accelerator (self, str);

  g_free (str);
  g_strfreev (accels);
}

static void
bobgui_shortcuts_shortcut_measure (BobguiWidget      *widget,
                                BobguiOrientation  orientation,
                                int            for_size,
                                int           *minimum,
                                int           *natural,
                                int           *minimum_baseline,
                                int           *natural_baseline)
{
  bobgui_widget_measure (BOBGUI_WIDGET (BOBGUI_SHORTCUTS_SHORTCUT (widget)->box),
                      orientation, for_size,
                      minimum, natural,
                      minimum_baseline, natural_baseline);
}

static void
bobgui_shortcuts_shortcut_snapshot (BobguiWidget   *widget,
                                 BobguiSnapshot *snapshot)
{
  bobgui_widget_snapshot_child (widget, BOBGUI_WIDGET (BOBGUI_SHORTCUTS_SHORTCUT (widget)->box), snapshot);
}

static void
bobgui_shortcuts_shortcut_size_allocate (BobguiWidget *widget,
                                      int        width,
                                      int        height,
                                      int        baseline)
{
  BOBGUI_WIDGET_CLASS (bobgui_shortcuts_shortcut_parent_class)->size_allocate (widget, width, height, baseline);

  bobgui_widget_size_allocate (BOBGUI_WIDGET (BOBGUI_SHORTCUTS_SHORTCUT (widget)->box),
                            &(BobguiAllocation) {
                              0, 0,
                              width, height
                            }, -1);
}

static void
bobgui_shortcuts_shortcut_class_init (BobguiShortcutsShortcutClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->finalize = bobgui_shortcuts_shortcut_finalize;
  object_class->get_property = bobgui_shortcuts_shortcut_get_property;
  object_class->set_property = bobgui_shortcuts_shortcut_set_property;

  widget_class->direction_changed = bobgui_shortcuts_shortcut_direction_changed;
  widget_class->measure = bobgui_shortcuts_shortcut_measure;
  widget_class->snapshot = bobgui_shortcuts_shortcut_snapshot;
  widget_class->size_allocate = bobgui_shortcuts_shortcut_size_allocate;

  /**
   * BobguiShortcutsShortcut:accelerator:
   *
   * The accelerator(s) represented by this object.
   *
   * This property is used if [property@Bobgui.ShortcutsShortcut:shortcut-type]
   * is set to %BOBGUI_SHORTCUT_ACCELERATOR.
   *
   * The syntax of this property is (an extension of) the syntax understood
   * by [func@Bobgui.accelerator_parse]. Multiple accelerators can be specified
   * by separating them with a space, but keep in mind that the available width
   * is limited.
   *
   * It is also possible to specify ranges of shortcuts, using `...` between
   * the keys. Sequences of keys can be specified using a `+` or `&` between
   * the keys.
   *
   * Examples:
   *
   * - A single shortcut: `<ctl><alt>delete`
   * - Two alternative shortcuts: `<shift>a Home`
   * - A range of shortcuts: `<alt>1...<alt>9`
   * - Several keys pressed together: `Control_L&Control_R`
   * - A sequence of shortcuts or keys: `<ctl>c+<ctl>x`
   *
   * Use "+" instead of "&" when the keys may (or have to be) pressed
   * sequentially (e.g use "t+t" for 'press the t key twice').
   *
   * Note that `<`, `>` and `&` need to be escaped as `&lt;`, `&gt`; and `&amp`; when used
   * in .ui files.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  properties[PROP_ACCELERATOR] =
    g_param_spec_string ("accelerator", NULL, NULL,
                         NULL,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * BobguiShortcutsShortcut:icon:
   *
   * An icon to represent the shortcut or gesture.
   *
   * This property is used if [property@Bobgui.ShortcutsShortcut:shortcut-type]
   * is set to %BOBGUI_SHORTCUT_GESTURE.
   *
   * For the other predefined gesture types, BOBGUI provides an icon on its own.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  properties[PROP_ICON] =
    g_param_spec_object ("icon", NULL, NULL,
                         G_TYPE_ICON,
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * BobguiShortcutsShortcut:icon-set:
   *
   * %TRUE if an icon has been set.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  properties[PROP_ICON_SET] =
    g_param_spec_boolean ("icon-set", NULL, NULL,
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * BobguiShortcutsShortcut:title:
   *
   * The textual description for the shortcut or gesture represented by
   * this object.
   *
   * This should be a short string that can fit in a single line.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  properties[PROP_TITLE] =
    g_param_spec_string ("title", NULL, NULL,
                         "",
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * BobguiShortcutsShortcut:subtitle:
   *
   * The subtitle for the shortcut or gesture.
   *
   * This is typically used for gestures and should be a short, one-line
   * text that describes the gesture itself. For the predefined gesture
   * types, BOBGUI provides a subtitle on its own.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  properties[PROP_SUBTITLE] =
    g_param_spec_string ("subtitle", NULL, NULL,
                         "",
                         (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * BobguiShortcutsShortcut:subtitle-set:
   *
   * %TRUE if a subtitle has been set.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  properties[PROP_SUBTITLE_SET] =
    g_param_spec_boolean ("subtitle-set", NULL, NULL,
                          FALSE,
                          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * BobguiShortcutsShortcut:accel-size-group:
   *
   * The size group for the accelerator portion of this shortcut.
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
   * BobguiShortcutsShortcut:title-size-group:
   *
   * The size group for the textual portion of this shortcut.
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
   * BobguiShortcutsShortcut:direction:
   *
   * The text direction for which this shortcut is active.
   *
   * If the shortcut is used regardless of the text direction,
   * set this property to %BOBGUI_TEXT_DIR_NONE.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  properties[PROP_DIRECTION] =
    g_param_spec_enum ("direction", NULL, NULL,
                       BOBGUI_TYPE_TEXT_DIRECTION,
                       BOBGUI_TEXT_DIR_NONE,
                       (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiShortcutsShortcut:shortcut-type:
   *
   * The type of shortcut that is represented.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  properties[PROP_SHORTCUT_TYPE] =
    g_param_spec_enum ("shortcut-type", NULL, NULL,
                       BOBGUI_TYPE_SHORTCUT_TYPE,
                       BOBGUI_SHORTCUT_ACCELERATOR,
                       (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiShortcutsShortcut:action-name:
   *
   * A detailed action name.
   *
   * If this is set for a shortcut of type %BOBGUI_SHORTCUT_ACCELERATOR,
   * then BOBGUI will use the accelerators that are associated with the
   * action via [method@Bobgui.Application.set_accels_for_action], and
   * setting [property@Bobgui.ShortcutsShortcut:accelerator] is not necessary.
   *
   * Deprecated: 4.18: This widget will be removed in BOBGUI 5
   */
  properties[PROP_ACTION_NAME] =
    g_param_spec_string ("action-name", NULL, NULL,
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, properties);
  bobgui_widget_class_set_css_name (widget_class, I_("shortcut"));
  /* It is semantically a label, but the label role has such specific meaning in Orca
   * as to be unusable in this context.
   */
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_LABEL);
}

static void
bobgui_shortcuts_shortcut_init (BobguiShortcutsShortcut *self)
{
  bobgui_widget_set_focusable (BOBGUI_WIDGET (self), TRUE);


  self->box = g_object_new (BOBGUI_TYPE_BOX,
                            "orientation", BOBGUI_ORIENTATION_HORIZONTAL,
                            "spacing", 12,
                            NULL);
  bobgui_widget_set_parent (BOBGUI_WIDGET (self->box), BOBGUI_WIDGET (self));

  self->direction = BOBGUI_TEXT_DIR_NONE;
  self->shortcut_type = BOBGUI_SHORTCUT_ACCELERATOR;

  self->image = g_object_new (BOBGUI_TYPE_IMAGE,
                              "visible", FALSE,
                              "valign", BOBGUI_ALIGN_CENTER,
                              "accessible-role", BOBGUI_ACCESSIBLE_ROLE_PRESENTATION,
                              NULL);
  bobgui_box_append (BOBGUI_BOX (self->box), BOBGUI_WIDGET (self->image));

  self->accelerator = g_object_new (BOBGUI_TYPE_SHORTCUT_LABEL,
                                    "visible", TRUE,
                                    "valign", BOBGUI_ALIGN_CENTER,
                                    NULL);
  bobgui_box_append (BOBGUI_BOX (self->box), BOBGUI_WIDGET (self->accelerator));

  self->title_box = g_object_new (BOBGUI_TYPE_BOX,
                                  "visible", TRUE,
                                  "valign", BOBGUI_ALIGN_CENTER,
                                  "hexpand", TRUE,
                                  "orientation", BOBGUI_ORIENTATION_VERTICAL,
                                  NULL);
  bobgui_box_append (BOBGUI_BOX (self->box), BOBGUI_WIDGET (self->title_box));

  self->title = g_object_new (BOBGUI_TYPE_LABEL,
                              "visible", TRUE,
                              "xalign", 0.0f,
                              NULL);
  bobgui_box_append (BOBGUI_BOX (self->title_box), BOBGUI_WIDGET (self->title));

  self->subtitle = g_object_new (BOBGUI_TYPE_LABEL,
                                 "visible", FALSE,
                                 "xalign", 0.0f,
                                 NULL);
  bobgui_widget_add_css_class (BOBGUI_WIDGET (self->subtitle), "dim-label");
  bobgui_box_append (BOBGUI_BOX (self->title_box), BOBGUI_WIDGET (self->subtitle));

#if 0
  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (self),
                                  BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, self->accelerator, self->title, NULL,
                                  -1);
#endif

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                  BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION, "",
                                  -1);
}
