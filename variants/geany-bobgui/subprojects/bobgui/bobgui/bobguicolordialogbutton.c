/*
 * BOBGUI - The Bobgui Framework
 * Copyright (C) 2022 Red Hat, Inc.
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "bobguicolordialogbutton.h"

#include "bobguibinlayout.h"
#include "bobguibutton.h"
#include "bobguicolorswatchprivate.h"
#include "bobguidragsource.h"
#include "bobguidroptarget.h"
#include <glib/gi18n-lib.h>
#include "bobguimain.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"
#include "bobguisnapshot.h"
#include "bobguiwidgetprivate.h"
#include "gdk/gdkrgbaprivate.h"


static gboolean drop           (BobguiDropTarget        *dest,
                                const GValue         *value,
                                double                x,
                                double                y,
                                BobguiColorDialogButton *self);
static GdkContentProvider *
                  drag_prepare (BobguiDragSource        *source,
                                double                x,
                                double                y,
                                BobguiColorDialogButton *self);
static void     activated      (BobguiColorDialogButton *self);
static void     button_clicked (BobguiColorDialogButton *self);
static void     update_button_sensitivity
                               (BobguiColorDialogButton *self);

/**
 * BobguiColorDialogButton:
 *
 * Opens a color chooser dialog to select a color.
 *
 * <picture>
 *   <source srcset="color-button-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiColorDialogButton" src="color-button.png">
 * </picture>
 *
 * It is suitable widget for selecting a color in a preference dialog.
 *
 * # CSS nodes
 *
 * ```
 * colorbutton
 * ╰── button.color
 *     ╰── [content]
 * ```
 *
 * `BobguiColorDialogButton` has a single CSS node with name colorbutton which
 * contains a button node. To differentiate it from a plain `BobguiButton`,
 * it gets the .color style class.
 *
 * Since: 4.10
 */

/* {{{ GObject implementation */

struct _BobguiColorDialogButton
{
  BobguiWidget parent_instance;

  BobguiWidget *button;
  BobguiWidget *swatch;

  BobguiColorDialog *dialog;
  GCancellable *cancellable;
  GdkRGBA color;
};

/* Properties */
enum
{
  PROP_DIALOG = 1,
  PROP_RGBA,
  NUM_PROPERTIES
};

/* Signals */
enum
{
  SIGNAL_ACTIVATE = 1,
  NUM_SIGNALS
};

static GParamSpec *properties[NUM_PROPERTIES];

static unsigned int color_dialog_button_signals[NUM_SIGNALS] = { 0 };

G_DEFINE_TYPE (BobguiColorDialogButton, bobgui_color_dialog_button, BOBGUI_TYPE_WIDGET)

static void
bobgui_color_dialog_button_init (BobguiColorDialogButton *self)
{
  PangoLayout *layout;
  PangoRectangle rect;
  BobguiDragSource *source;
  BobguiDropTarget *dest;

  g_signal_connect_swapped (self, "activate", G_CALLBACK (activated), self);

  self->color = GDK_RGBA ("00000000");

  self->button = bobgui_button_new ();
  g_signal_connect_swapped (self->button, "clicked", G_CALLBACK (button_clicked), self);
  bobgui_widget_set_parent (self->button, BOBGUI_WIDGET (self));

  self->swatch = g_object_new (BOBGUI_TYPE_COLOR_SWATCH,
                               "accessible-role", BOBGUI_ACCESSIBLE_ROLE_IMG,
                               "selectable", FALSE,
                               "has-menu", FALSE,
                               "can-drag", FALSE,
                               NULL);
  bobgui_widget_set_can_focus (self->swatch, FALSE);
  bobgui_widget_remove_css_class (self->swatch, "activatable");

  layout = bobgui_widget_create_pango_layout (BOBGUI_WIDGET (self), "Black");
  pango_layout_get_pixel_extents (layout, NULL, &rect);
  g_object_unref (layout);

  bobgui_widget_set_size_request (self->swatch, rect.width, rect.height);

  bobgui_button_set_child (BOBGUI_BUTTON (self->button), self->swatch);

  dest = bobgui_drop_target_new (GDK_TYPE_RGBA, GDK_ACTION_COPY);
  g_signal_connect (dest, "drop", G_CALLBACK (drop), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self->button), BOBGUI_EVENT_CONTROLLER (dest));

  source = bobgui_drag_source_new ();
  g_signal_connect (source, "prepare", G_CALLBACK (drag_prepare), self);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (source),
                                              BOBGUI_PHASE_CAPTURE);
  bobgui_widget_add_controller (self->button, BOBGUI_EVENT_CONTROLLER (source));
  bobgui_widget_add_css_class (self->button, "color");

  bobgui_color_dialog_button_set_rgba (self, &(GdkRGBA) { 0.75, 0.25, 0.25, 1.0 });

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self->button),
                                  BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP, TRUE,
                                  -1);
}

static void
bobgui_color_dialog_button_unroot (BobguiWidget *widget)
{
  BobguiColorDialogButton *self = BOBGUI_COLOR_DIALOG_BUTTON (widget);

  if (self->cancellable)
    {
      g_cancellable_cancel (self->cancellable);
      g_clear_object (&self->cancellable);
      update_button_sensitivity (self);
    }

  BOBGUI_WIDGET_CLASS (bobgui_color_dialog_button_parent_class)->unroot (widget);
}

static void
bobgui_color_dialog_button_set_property (GObject      *object,
                                      unsigned int  param_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  BobguiColorDialogButton *self = BOBGUI_COLOR_DIALOG_BUTTON (object);

  switch (param_id)
    {
    case PROP_DIALOG:
      bobgui_color_dialog_button_set_dialog (self, g_value_get_object (value));
      break;

    case PROP_RGBA:
      bobgui_color_dialog_button_set_rgba (self, g_value_get_boxed (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bobgui_color_dialog_button_get_property (GObject      *object,
                                      unsigned int  param_id,
                                      GValue       *value,
                                      GParamSpec   *pspec)
{
  BobguiColorDialogButton *self = BOBGUI_COLOR_DIALOG_BUTTON (object);

  switch (param_id)
    {
    case PROP_DIALOG:
      g_value_set_object (value, self->dialog);
      break;

    case PROP_RGBA:
      g_value_set_boxed (value, &self->color);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bobgui_color_dialog_button_dispose (GObject *object)
{
  BobguiColorDialogButton *self = BOBGUI_COLOR_DIALOG_BUTTON (object);

  g_clear_pointer (&self->button, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_color_dialog_button_parent_class)->dispose (object);
}

static void
bobgui_color_dialog_button_finalize (GObject *object)
{
  BobguiColorDialogButton *self = BOBGUI_COLOR_DIALOG_BUTTON (object);

  g_assert (self->cancellable == NULL);
  g_clear_object (&self->dialog);

  G_OBJECT_CLASS (bobgui_color_dialog_button_parent_class)->finalize (object);
}

static void
bobgui_color_dialog_button_class_init (BobguiColorDialogButtonClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->get_property = bobgui_color_dialog_button_get_property;
  object_class->set_property = bobgui_color_dialog_button_set_property;
  object_class->dispose = bobgui_color_dialog_button_dispose;
  object_class->finalize = bobgui_color_dialog_button_finalize;

  widget_class->grab_focus = bobgui_widget_grab_focus_child;
  widget_class->focus = bobgui_widget_focus_child;
  widget_class->unroot = bobgui_color_dialog_button_unroot;

  /**
   * BobguiColorDialogButton:dialog:
   *
   * The `BobguiColorDialog` that contains parameters for
   * the color chooser dialog.
   *
   * Since: 4.10
   */
  properties[PROP_DIALOG] =
      g_param_spec_object ("dialog", NULL, NULL,
                           BOBGUI_TYPE_COLOR_DIALOG,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiColorDialogButton:rgba:
   *
   * The selected color.
   *
   * This property can be set to give the button its initial
   * color, and it will be updated to reflect the users choice
   * in the color chooser dialog.
   *
   * Listen to `notify::rgba` to get informed about changes
   * to the buttons color.
   *
   * Since: 4.10
   */
  properties[PROP_RGBA] =
      g_param_spec_boxed ("rgba", NULL, NULL,
                          GDK_TYPE_RGBA,
                          G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

  /**
   * BobguiColorDialogButton::activate:
   * @widget: the object which received the signal
   *
   * Emitted when the color dialog button is activated.
   *
   * The `::activate` signal on `BobguiColorDialogButton` is an action signal
   * and emitting it causes the button to pop up its dialog.
   *
   * Since: 4.14
   */
  color_dialog_button_signals[SIGNAL_ACTIVATE] =
    g_signal_new (I_ ("activate"),
                  G_TYPE_FROM_CLASS (class),
                  G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  bobgui_widget_class_set_activate_signal (widget_class, color_dialog_button_signals[SIGNAL_ACTIVATE]);
  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, "colorbutton");
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GROUP);
}

/* }}} */
/* {{{ Private API, callbacks */

static guint
scale_round (double value,
             double scale)
{
  value = floor (value * scale + 0.5);
  value = CLAMP (value, 0, scale);
  return (guint)value;
}

static char *
accessible_color_name (const GdkRGBA *color)
{
  if (color->alpha < 1.0)
    return g_strdup_printf (_("Red %d%%, Green %d%%, Blue %d%%, Alpha %d%%"),
                            scale_round (color->red, 100),
                            scale_round (color->green, 100),
                            scale_round (color->blue, 100),
                            scale_round (color->alpha, 100));
  else
    return g_strdup_printf (_("Red %d%%, Green %d%%, Blue %d%%"),
                            scale_round (color->red, 100),
                            scale_round (color->green, 100),
                            scale_round (color->blue, 100));
}

static gboolean
drop (BobguiDropTarget        *dest,
      const GValue         *value,
      double                x,
      double                y,
      BobguiColorDialogButton *self)
{
  GdkRGBA *color = g_value_get_boxed (value);

  bobgui_color_dialog_button_set_rgba (self, color);

  return TRUE;
}

static GdkContentProvider *
drag_prepare (BobguiDragSource        *source,
              double                x,
              double                y,
              BobguiColorDialogButton *self)
{
  GdkRGBA color;

  bobgui_color_swatch_get_rgba (BOBGUI_COLOR_SWATCH (self->swatch), &color);

  return gdk_content_provider_new_typed (GDK_TYPE_RGBA, &color);
}

static void
update_button_sensitivity (BobguiColorDialogButton *self)
{
  if (self->button)
    bobgui_widget_set_sensitive (self->button,
                              self->dialog != NULL && self->cancellable == NULL);
}

static void
color_chosen (GObject      *source,
              GAsyncResult *result,
              gpointer      data)
{
  BobguiColorDialog *dialog = BOBGUI_COLOR_DIALOG (source);
  BobguiColorDialogButton *self = data;
  GdkRGBA *color;

  color = bobgui_color_dialog_choose_rgba_finish (dialog, result, NULL);
  if (color)
    {
      bobgui_color_dialog_button_set_rgba (self, color);
      gdk_rgba_free (color);
    }

  g_clear_object (&self->cancellable);
  update_button_sensitivity (self);
}

static void
activated (BobguiColorDialogButton *self)
{
  bobgui_widget_activate (self->button);
}

static void
button_clicked (BobguiColorDialogButton *self)
{
  BobguiRoot *root = bobgui_widget_get_root (BOBGUI_WIDGET (self));
  BobguiWindow *parent = NULL;

  g_assert (self->cancellable == NULL);
  self->cancellable = g_cancellable_new ();

  update_button_sensitivity (self);

  if (BOBGUI_IS_WINDOW (root))
    parent = BOBGUI_WINDOW (root);

  bobgui_color_dialog_choose_rgba (self->dialog, parent, &self->color,
                                self->cancellable, color_chosen, self);
}

/* }}} */
/* {{{ Constructor */

/**
 * bobgui_color_dialog_button_new:
 * @dialog: (nullable) (transfer full): the `BobguiColorDialog` to use
 *
 * Creates a new `BobguiColorDialogButton` with the
 * given `BobguiColorDialog`.
 *
 * You can pass `NULL` to this function and set a `BobguiColorDialog`
 * later. The button will be insensitive until that happens.
 *
 * Returns: the new `BobguiColorDialogButton`
 *
 * Since: 4.10
 */
BobguiWidget *
bobgui_color_dialog_button_new (BobguiColorDialog *dialog)
{
  BobguiWidget *self;

  g_return_val_if_fail (dialog == NULL || BOBGUI_IS_COLOR_DIALOG (dialog), NULL);

  self = g_object_new (BOBGUI_TYPE_COLOR_DIALOG_BUTTON,
                       "dialog", dialog,
                       NULL);

  g_clear_object (&dialog);

  return self;
}

/* }}} */
/* {{{ Getters and setters */

/**
 * bobgui_color_dialog_button_get_dialog:
 * @self: a `BobguiColorDialogButton`
 *
 * Returns the `BobguiColorDialog` of @self.
 *
 * Returns: (transfer none) (nullable): the `BobguiColorDialog`
 *
 * Since: 4.10
 */
BobguiColorDialog *
bobgui_color_dialog_button_get_dialog (BobguiColorDialogButton *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLOR_DIALOG_BUTTON (self), NULL);

  return self->dialog;
}

/**
 * bobgui_color_dialog_button_set_dialog:
 * @self: a `BobguiColorDialogButton`
 * @dialog: the new `BobguiColorDialog`
 *
 * Sets a `BobguiColorDialog` object to use for
 * creating the color chooser dialog that is
 * presented when the user clicks the button.
 *
 * Since: 4.10
 */
void
bobgui_color_dialog_button_set_dialog (BobguiColorDialogButton *self,
                                    BobguiColorDialog       *dialog)
{
  g_return_if_fail (BOBGUI_IS_COLOR_DIALOG_BUTTON (self));
  g_return_if_fail (dialog == NULL || BOBGUI_IS_COLOR_DIALOG (dialog));

  if (!g_set_object (&self->dialog, dialog))
    return;

  update_button_sensitivity (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DIALOG]);
}

/**
 * bobgui_color_dialog_button_get_rgba:
 * @self: a `BobguiColorDialogButton`
 *
 * Returns the color of the button.
 *
 * This function is what should be used to obtain
 * the color that was chosen by the user. To get
 * informed about changes, listen to "notify::rgba".
 *
 * Returns: the color
 *
 * Since: 4.10
 */
const GdkRGBA *
bobgui_color_dialog_button_get_rgba (BobguiColorDialogButton *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLOR_DIALOG_BUTTON (self), NULL);

  return &self->color;
}

/**
 * bobgui_color_dialog_button_set_rgba:
 * @self: a `BobguiColorDialogButton`
 * @color: the new color
 *
 * Sets the color of the button.
 *
 * Since: 4.10
 */
void
bobgui_color_dialog_button_set_rgba (BobguiColorDialogButton *self,
                                  const GdkRGBA        *color)
{
  char *text;

  g_return_if_fail (BOBGUI_IS_COLOR_DIALOG_BUTTON (self));
  g_return_if_fail (color != NULL);

  if (gdk_rgba_equal (&self->color, color))
    return;

  self->color = *color;
  bobgui_color_swatch_set_rgba (BOBGUI_COLOR_SWATCH (self->swatch), color);

  text = accessible_color_name (color);
  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self->swatch),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL, text,
                                  -1);
  g_free (text);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_RGBA]);
}

/* }}} */

/* vim:set foldmethod=marker: */
