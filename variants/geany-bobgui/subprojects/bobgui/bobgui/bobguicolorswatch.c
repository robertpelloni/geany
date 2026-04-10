/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2012 Red Hat, Inc.
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

#include "config.h"

#include "bobguicolorswatchprivate.h"

#include "bobguibox.h"
#include "deprecated/bobguicolorchooserprivate.h"
#include "bobguidragsource.h"
#include "bobguidroptarget.h"
#include "bobguigesturelongpress.h"
#include "bobguigestureclick.h"
#include "bobguigesturesingle.h"
#include "bobguiimage.h"
#include <glib/gi18n-lib.h>
#include "bobguimain.h"
#include "bobguimodelbuttonprivate.h"
#include "bobguipopovermenu.h"
#include "bobguiprivate.h"
#include "bobguisnapshot.h"
#include "bobguiwidgetprivate.h"
#include "bobguieventcontrollerkey.h"
#include "bobguinative.h"

/*
 * BobguiColorSwatch has two CSS nodes, the main one named colorswatch
 * and a subnode named overlay. The main node gets the .light or .dark
 * style classes added depending on the brightness of the color that
 * the swatch is showing.
 *
 * The color swatch has the .activatable style class by default. It can
 * be removed for non-activatable swatches.
 */

typedef struct _BobguiColorSwatchClass   BobguiColorSwatchClass;

struct _BobguiColorSwatch
{
  BobguiWidget parent_instance;

  GdkRGBA color;
  char *icon;
  guint    has_color        : 1;
  guint    use_alpha        : 1;
  guint    selectable       : 1;
  guint    has_menu         : 1;

  BobguiWidget *overlay_widget;

  BobguiWidget *popover;
  BobguiDropTarget *dest;
  BobguiDragSource *source;
};

struct _BobguiColorSwatchClass
{
  BobguiWidgetClass parent_class;

  void ( * activate)  (BobguiColorSwatch *swatch);
  void ( * customize) (BobguiColorSwatch *swatch);
};

enum
{
  PROP_ZERO,
  PROP_RGBA,
  PROP_SELECTABLE,
  PROP_HAS_MENU,
  PROP_CAN_DROP,
  PROP_CAN_DRAG
};

G_DEFINE_TYPE (BobguiColorSwatch, bobgui_color_swatch, BOBGUI_TYPE_WIDGET)

#define INTENSITY(r, g, b) ((r) * 0.30 + (g) * 0.59 + (b) * 0.11)
static void
swatch_snapshot (BobguiWidget   *widget,
                 BobguiSnapshot *snapshot)
{
  BobguiColorSwatch *swatch = BOBGUI_COLOR_SWATCH (widget);
  const int width = bobgui_widget_get_width (widget);
  const int height = bobgui_widget_get_height (widget);
  const GdkRGBA *color;

  color = &swatch->color;
  if (swatch->dest)
    {
      const GValue *value = bobgui_drop_target_get_value (swatch->dest);

      if (value)
        color = g_value_get_boxed (value);
    }

  if (swatch->has_color)
    {
      if (swatch->use_alpha && !gdk_rgba_is_opaque (color))
        {
          _bobgui_color_chooser_snapshot_checkered_pattern (snapshot, width, height);

          bobgui_snapshot_append_color (snapshot,
                                     color,
                                     &GRAPHENE_RECT_INIT (0, 0, width, height));
        }
      else
        {
          GdkRGBA opaque = *color;

          opaque.alpha = 1.0;

          bobgui_snapshot_append_color (snapshot,
                                     &opaque,
                                     &GRAPHENE_RECT_INIT (0, 0, width, height));
        }
    }

  bobgui_widget_snapshot_child (widget, swatch->overlay_widget, snapshot);
}

static gboolean
swatch_drag_drop (BobguiDropTarget  *dest,
                  const GValue   *value,
                  double          x,
                  double          y,
                  BobguiColorSwatch *swatch)

{
  bobgui_color_swatch_set_rgba (swatch, g_value_get_boxed (value));

  return TRUE;
}

void
bobgui_color_swatch_activate (BobguiColorSwatch *swatch)
{
  double red, green, blue, alpha;

  red = swatch->color.red;
  green = swatch->color.green;
  blue = swatch->color.blue;
  alpha = swatch->color.alpha;

  bobgui_widget_activate_action (BOBGUI_WIDGET (swatch),
                              "color.select", "(dddd)", red, green, blue, alpha);
}

void
bobgui_color_swatch_customize (BobguiColorSwatch *swatch)
{
  double red, green, blue, alpha;

  red = swatch->color.red;
  green = swatch->color.green;
  blue = swatch->color.blue;
  alpha = swatch->color.alpha;

  bobgui_widget_activate_action (BOBGUI_WIDGET (swatch),
                              "color.customize", "(dddd)", red, green, blue, alpha);
}

void
bobgui_color_swatch_select (BobguiColorSwatch *swatch)
{
  bobgui_widget_set_state_flags (BOBGUI_WIDGET (swatch), BOBGUI_STATE_FLAG_SELECTED, FALSE);
}

static gboolean
bobgui_color_swatch_is_selected (BobguiColorSwatch *swatch)
{
  return (bobgui_widget_get_state_flags (BOBGUI_WIDGET (swatch)) & BOBGUI_STATE_FLAG_SELECTED) != 0;
}

static gboolean
key_controller_key_pressed (BobguiEventControllerKey *controller,
                            guint                  keyval,
                            guint                  keycode,
                            GdkModifierType        state,
                            BobguiWidget             *widget)
{
  BobguiColorSwatch *swatch = BOBGUI_COLOR_SWATCH (widget);

  if (keyval == GDK_KEY_space ||
      keyval == GDK_KEY_Return ||
      keyval == GDK_KEY_ISO_Enter||
      keyval == GDK_KEY_KP_Enter ||
      keyval == GDK_KEY_KP_Space)
    {
      if (swatch->has_color &&
          swatch->selectable &&
          !bobgui_color_swatch_is_selected (swatch))
        bobgui_color_swatch_select (swatch);
      else
        bobgui_color_swatch_customize (swatch);

      return TRUE;
    }

  return FALSE;
}

static GMenuModel *
bobgui_color_swatch_get_menu_model (BobguiColorSwatch *swatch)
{
  GMenu *menu, *section;
  GMenuItem *item;
  double red, green, blue, alpha;

  menu = g_menu_new ();

  red = swatch->color.red;
  green = swatch->color.green;
  blue = swatch->color.blue;
  alpha = swatch->color.alpha;

  section = g_menu_new ();
  item = g_menu_item_new (_("Customize"), NULL);
  g_menu_item_set_action_and_target_value (item, "color.customize",
                                           g_variant_new ("(dddd)", red, green, blue, alpha));

  g_menu_append_item (section, item);
  g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
  g_object_unref (item);
  g_object_unref (section);

  return G_MENU_MODEL (menu);
}

static void
do_popup (BobguiColorSwatch *swatch)
{
  GMenuModel *model;

  g_clear_pointer (&swatch->popover, bobgui_widget_unparent);

  model = bobgui_color_swatch_get_menu_model (swatch);
  swatch->popover = bobgui_popover_menu_new_from_model (model);
  bobgui_widget_set_parent (swatch->popover, BOBGUI_WIDGET (swatch));
  g_object_unref (model);

  bobgui_popover_popup (BOBGUI_POPOVER (swatch->popover));
}

static gboolean
swatch_primary_action (BobguiColorSwatch *swatch)
{
  if (!swatch->has_color)
    {
      bobgui_color_swatch_customize (swatch);
      return TRUE;
    }
  else if (swatch->selectable &&
           !bobgui_color_swatch_is_selected (swatch))
    {
      bobgui_color_swatch_select (swatch);
      return TRUE;
    }

  return FALSE;
}

static void
hold_action (BobguiGestureLongPress *gesture,
             double               x,
             double               y,
             BobguiColorSwatch      *swatch)
{
  do_popup (swatch);
  bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
}

static void
tap_action (BobguiGestureClick *gesture,
            int              n_press,
            double           x,
            double           y,
            BobguiColorSwatch  *swatch)
{
  guint button;

  button = bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (gesture));

  if (button == GDK_BUTTON_PRIMARY)
    {
      if (n_press == 1)
        swatch_primary_action (swatch);
      else if (n_press > 1)
        bobgui_color_swatch_activate (swatch);
    }
  else if (button == GDK_BUTTON_SECONDARY)
    {
      if (swatch->has_color && swatch->has_menu)
        do_popup (swatch);
    }
}

static void
swatch_size_allocate (BobguiWidget *widget,
                      int        width,
                      int        height,
                      int        baseline)
{
  BobguiColorSwatch *swatch = BOBGUI_COLOR_SWATCH (widget);

  bobgui_widget_size_allocate (swatch->overlay_widget,
                            &(BobguiAllocation) {
                              0, 0,
                              width, height
                            }, -1);

  if (swatch->popover)
    bobgui_popover_present (BOBGUI_POPOVER (swatch->popover));
}

static void
bobgui_color_swatch_measure (BobguiWidget *widget,
                          BobguiOrientation  orientation,
                          int             for_size,
                          int            *minimum,
                          int            *natural,
                          int            *minimum_baseline,
                          int            *natural_baseline)
{
  BobguiColorSwatch *swatch = BOBGUI_COLOR_SWATCH (widget);
  int w, h, min;

  bobgui_widget_measure (swatch->overlay_widget,
                      orientation,
                      -1,
                      minimum, natural,
                      NULL, NULL);

  bobgui_widget_get_size_request (widget, &w, &h);
  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    min = w < 0 ? 48 : w;
  else
    min = h < 0 ? 32 : h;

  *minimum = MAX (*minimum, min);
  *natural = MAX (*natural, min);
}

static void
swatch_popup_menu (BobguiWidget  *widget,
                   const char *action_name,
                   GVariant   *parameters)
{
  do_popup (BOBGUI_COLOR_SWATCH (widget));
}

static void
update_icon (BobguiColorSwatch *swatch)
{
  BobguiImage *image = BOBGUI_IMAGE (swatch->overlay_widget);

  if (swatch->icon)
    bobgui_image_set_from_icon_name (image, swatch->icon);
  else if (bobgui_color_swatch_is_selected (swatch))
    bobgui_image_set_from_icon_name (image, "object-select-symbolic");
  else
    bobgui_image_clear (image);
}

static void
update_accessible_properties (BobguiColorSwatch *swatch)
{
  if (swatch->selectable)
    {
      gboolean selected = bobgui_color_swatch_is_selected (swatch);

      bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (swatch),
                                   BOBGUI_ACCESSIBLE_STATE_CHECKED, selected,
                                   -1);
    }
  else
    {
      bobgui_accessible_reset_state (BOBGUI_ACCESSIBLE (swatch),
                                  BOBGUI_ACCESSIBLE_STATE_CHECKED);
    }
}

static void
swatch_state_flags_changed (BobguiWidget     *widget,
                            BobguiStateFlags  previous_state)
{
  BobguiColorSwatch *swatch = BOBGUI_COLOR_SWATCH (widget);

  update_icon (swatch);
  update_accessible_properties (swatch);

  BOBGUI_WIDGET_CLASS (bobgui_color_swatch_parent_class)->state_flags_changed (widget, previous_state);
}

/* GObject implementation {{{1 */

static void
swatch_get_property (GObject    *object,
                     guint       prop_id,
                     GValue     *value,
                     GParamSpec *pspec)
{
  BobguiColorSwatch *swatch = BOBGUI_COLOR_SWATCH (object);
  GdkRGBA color;

  switch (prop_id)
    {
    case PROP_RGBA:
      bobgui_color_swatch_get_rgba (swatch, &color);
      g_value_set_boxed (value, &color);
      break;
    case PROP_SELECTABLE:
      g_value_set_boolean (value, bobgui_color_swatch_get_selectable (swatch));
      break;
    case PROP_HAS_MENU:
      g_value_set_boolean (value, swatch->has_menu);
      break;
    case PROP_CAN_DROP:
      g_value_set_boolean (value, swatch->dest != NULL);
      break;
    case PROP_CAN_DRAG:
      g_value_set_boolean (value, swatch->source != NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
swatch_set_property (GObject      *object,
                     guint         prop_id,
                     const GValue *value,
                     GParamSpec   *pspec)
{
  BobguiColorSwatch *swatch = BOBGUI_COLOR_SWATCH (object);

  switch (prop_id)
    {
    case PROP_RGBA:
      bobgui_color_swatch_set_rgba (swatch, g_value_get_boxed (value));
      break;
    case PROP_SELECTABLE:
      bobgui_color_swatch_set_selectable (swatch, g_value_get_boolean (value));
      break;
    case PROP_HAS_MENU:
      swatch->has_menu = g_value_get_boolean (value);
      break;
    case PROP_CAN_DROP:
      bobgui_color_swatch_set_can_drop (swatch, g_value_get_boolean (value));
      break;
    case PROP_CAN_DRAG:
      bobgui_color_swatch_set_can_drag (swatch, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
swatch_finalize (GObject *object)
{
  BobguiColorSwatch *swatch = BOBGUI_COLOR_SWATCH (object);

  g_free (swatch->icon);
  bobgui_widget_unparent (swatch->overlay_widget);

  G_OBJECT_CLASS (bobgui_color_swatch_parent_class)->finalize (object);
}

static void
swatch_dispose (GObject *object)
{
  BobguiColorSwatch *swatch = BOBGUI_COLOR_SWATCH (object);

  g_clear_pointer (&swatch->popover, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_color_swatch_parent_class)->dispose (object);
}

static void
bobgui_color_swatch_class_init (BobguiColorSwatchClass *class)
{
  BobguiWidgetClass *widget_class = (BobguiWidgetClass *)class;
  GObjectClass *object_class = (GObjectClass *)class;

  object_class->get_property = swatch_get_property;
  object_class->set_property = swatch_set_property;
  object_class->finalize = swatch_finalize;
  object_class->dispose = swatch_dispose;

  widget_class->measure = bobgui_color_swatch_measure;
  widget_class->snapshot = swatch_snapshot;
  widget_class->size_allocate = swatch_size_allocate;
  widget_class->state_flags_changed = swatch_state_flags_changed;

  g_object_class_install_property (object_class, PROP_RGBA,
      g_param_spec_boxed ("rgba", NULL, NULL,
                          GDK_TYPE_RGBA, BOBGUI_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_SELECTABLE,
      g_param_spec_boolean ("selectable", NULL, NULL,
                            TRUE, BOBGUI_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_HAS_MENU,
      g_param_spec_boolean ("has-menu", NULL, NULL,
                            TRUE, BOBGUI_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_CAN_DROP,
      g_param_spec_boolean ("can-drop", NULL, NULL,
                            FALSE, BOBGUI_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_CAN_DRAG,
      g_param_spec_boolean ("can-drag", NULL, NULL,
                            TRUE, BOBGUI_PARAM_READWRITE));

  /**
   * BobguiColorSwatch|menu.popup:
   *
   * Opens the context menu.
   */
  bobgui_widget_class_install_action (widget_class, "menu.popup", NULL, swatch_popup_menu);

  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_F10, GDK_SHIFT_MASK,
                                       "menu.popup",
                                       NULL);
  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_Menu, 0,
                                       "menu.popup",
                                       NULL);

  bobgui_widget_class_set_css_name (widget_class, I_("colorswatch"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_RADIO);
}

static void
bobgui_color_swatch_init (BobguiColorSwatch *swatch)
{
  BobguiEventController *controller;
  BobguiGesture *gesture;

  swatch->use_alpha = TRUE;
  swatch->selectable = TRUE;
  swatch->has_menu = TRUE;
  swatch->color.red = 0.75;
  swatch->color.green = 0.25;
  swatch->color.blue = 0.25;
  swatch->color.alpha = 1.0;

  bobgui_widget_set_focusable (BOBGUI_WIDGET (swatch), TRUE);
  bobgui_widget_set_overflow (BOBGUI_WIDGET (swatch), BOBGUI_OVERFLOW_HIDDEN);

  gesture = bobgui_gesture_long_press_new ();
  bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (gesture),
                                     TRUE);
  g_signal_connect (gesture, "pressed",
                    G_CALLBACK (hold_action), swatch);
  bobgui_widget_add_controller (BOBGUI_WIDGET (swatch), BOBGUI_EVENT_CONTROLLER (gesture));

  gesture = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture), 0);
  g_signal_connect (gesture, "pressed",
                    G_CALLBACK (tap_action), swatch);
  bobgui_widget_add_controller (BOBGUI_WIDGET (swatch), BOBGUI_EVENT_CONTROLLER (gesture));

  controller = bobgui_event_controller_key_new ();
  g_signal_connect (controller, "key-pressed",
                    G_CALLBACK (key_controller_key_pressed), swatch);
  bobgui_widget_add_controller (BOBGUI_WIDGET (swatch), controller);

  bobgui_color_swatch_set_can_drag (swatch, TRUE);

  bobgui_widget_add_css_class (BOBGUI_WIDGET (swatch), "activatable");

  swatch->overlay_widget = g_object_new (BOBGUI_TYPE_IMAGE,
                                         "accessible-role", BOBGUI_ACCESSIBLE_ROLE_NONE,
                                         "css-name", "overlay",
                                         NULL);
  bobgui_widget_set_parent (swatch->overlay_widget, BOBGUI_WIDGET (swatch));
}

/* Public API {{{1 */

BobguiWidget *
bobgui_color_swatch_new (void)
{
  return (BobguiWidget *) g_object_new (BOBGUI_TYPE_COLOR_SWATCH, NULL);
}

static GdkContentProvider *
bobgui_color_swatch_drag_prepare (BobguiDragSource  *source,
                               double          x,
                               double          y,
                               BobguiColorSwatch *swatch)
{
  return gdk_content_provider_new_typed (GDK_TYPE_RGBA, &swatch->color);
}

void
bobgui_color_swatch_set_rgba (BobguiColorSwatch *swatch,
                           const GdkRGBA  *color)
{
  swatch->has_color = TRUE;
  swatch->color = *color;
  if (swatch->source)
    bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (swatch->source), BOBGUI_PHASE_CAPTURE);

  if (INTENSITY (swatch->color.red, swatch->color.green, swatch->color.blue) > 0.5)
    {
      bobgui_widget_add_css_class (BOBGUI_WIDGET (swatch), "light");
      bobgui_widget_remove_css_class (BOBGUI_WIDGET (swatch), "dark");
    }
  else
    {
      bobgui_widget_add_css_class (BOBGUI_WIDGET (swatch), "dark");
      bobgui_widget_remove_css_class (BOBGUI_WIDGET (swatch), "light");
    }

  bobgui_widget_queue_draw (BOBGUI_WIDGET (swatch));
  g_object_notify (G_OBJECT (swatch), "rgba");
}

gboolean
bobgui_color_swatch_get_rgba (BobguiColorSwatch *swatch,
                           GdkRGBA        *color)
{
  if (swatch->has_color)
    {
      color->red = swatch->color.red;
      color->green = swatch->color.green;
      color->blue = swatch->color.blue;
      color->alpha = swatch->color.alpha;
      return TRUE;
    }
  else
    {
      color->red = 1.0;
      color->green = 1.0;
      color->blue = 1.0;
      color->alpha = 1.0;
      return FALSE;
    }
}

void
bobgui_color_swatch_set_icon (BobguiColorSwatch *swatch,
                           const char     *icon)
{
  swatch->icon = g_strdup (icon);
  update_icon (swatch);
  bobgui_widget_queue_draw (BOBGUI_WIDGET (swatch));
}

void
bobgui_color_swatch_set_can_drop (BobguiColorSwatch *swatch,
                               gboolean        can_drop)
{
  if (can_drop == (swatch->dest != NULL))
    return;

  if (can_drop && !swatch->dest)
    {
      swatch->dest = bobgui_drop_target_new (GDK_TYPE_RGBA, GDK_ACTION_COPY);
      bobgui_drop_target_set_preload (swatch->dest, TRUE);
      g_signal_connect (swatch->dest, "drop", G_CALLBACK (swatch_drag_drop), swatch);
      g_signal_connect_swapped (swatch->dest, "notify::value", G_CALLBACK (bobgui_widget_queue_draw), swatch);
      bobgui_widget_add_controller (BOBGUI_WIDGET (swatch), BOBGUI_EVENT_CONTROLLER (swatch->dest));
    }
  if (!can_drop && swatch->dest)
    {
      bobgui_widget_remove_controller (BOBGUI_WIDGET (swatch), BOBGUI_EVENT_CONTROLLER (swatch->dest));
      swatch->dest = NULL;
    }

  g_object_notify (G_OBJECT (swatch), "can-drop");
}

void
bobgui_color_swatch_set_can_drag (BobguiColorSwatch *swatch,
                               gboolean        can_drag)
{
  if (can_drag == (swatch->source != NULL))
    return;

  if (can_drag && !swatch->source)
    {
      swatch->source = bobgui_drag_source_new ();
      g_signal_connect (swatch->source, "prepare", G_CALLBACK (bobgui_color_swatch_drag_prepare), swatch);
      bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (swatch->source),
                                                  swatch->has_color ? BOBGUI_PHASE_CAPTURE : BOBGUI_PHASE_NONE);
      bobgui_widget_add_controller (BOBGUI_WIDGET (swatch), BOBGUI_EVENT_CONTROLLER (swatch->source));
    }
  if (!can_drag && swatch->source)
    {
      bobgui_widget_remove_controller (BOBGUI_WIDGET (swatch), BOBGUI_EVENT_CONTROLLER (swatch->source));
      swatch->source = NULL;
    }

  g_object_notify (G_OBJECT (swatch), "can-drag");
}

void
bobgui_color_swatch_set_use_alpha (BobguiColorSwatch *swatch,
                                gboolean        use_alpha)
{
  swatch->use_alpha = use_alpha;
  bobgui_widget_queue_draw (BOBGUI_WIDGET (swatch));
}

void
bobgui_color_swatch_set_selectable (BobguiColorSwatch *swatch,
                                 gboolean selectable)
{
  if (selectable == swatch->selectable)
    return;

  swatch->selectable = selectable;

  update_accessible_properties (swatch);
  g_object_notify (G_OBJECT (swatch), "selectable");
}

gboolean
bobgui_color_swatch_get_selectable (BobguiColorSwatch *swatch)
{
  return swatch->selectable;
}

/* vim:set foldmethod=marker: */
