/*
 * BOBGUI - The Bobgui Framework
 * Copyright (C) 1998, 1999 Red Hat, Inc.
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
/* Color picker button for GNOME
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 *
 * Modified by the BOBGUI Team and others 2003.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#include "config.h"

#include "bobguicolorbutton.h"

#include "bobguibinlayout.h"
#include "bobguibutton.h"
#include "bobguicolorchooser.h"
#include "bobguicolorchooserprivate.h"
#include "bobguicolorchooserdialog.h"
#include "bobguicolorswatchprivate.h"
#include "bobguidragsource.h"
#include "bobguidroptarget.h"
#include <glib/gi18n-lib.h>
#include "bobguimain.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"
#include "bobguisnapshot.h"
#include "bobguiwidgetprivate.h"


G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiColorButton:
 *
 * The `BobguiColorButton` allows to open a color chooser dialog to change
 * the color.
 *
 * <picture>
 *   <source srcset="color-button-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiColorButton" src="color-button.png">
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
 * `BobguiColorButton` has a single CSS node with name colorbutton which
 * contains a button node. To differentiate it from a plain `BobguiButton`,
 * it gets the .color style class.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ColorDialogButton] instead
 */

typedef struct _BobguiColorButtonClass     BobguiColorButtonClass;

struct _BobguiColorButton {
  BobguiWidget parent_instance;

  BobguiWidget *button;

  BobguiWidget *swatch;    /* Widget where we draw the color sample */
  BobguiWidget *cs_dialog; /* Color selection dialog */

  char *title;         /* Title for the color chooser dialog */
  GdkRGBA rgba;

  guint use_alpha   : 1;  /* Use alpha or not */
  guint show_editor : 1;
  guint modal       : 1;
};

struct _BobguiColorButtonClass {
  BobguiWidgetClass parent_class;

  void (* color_set) (BobguiColorButton *cp);
  void (* activate)  (BobguiColorButton *self);
};

/* Properties */
enum
{
  PROP_0,
  PROP_USE_ALPHA,
  PROP_TITLE,
  PROP_RGBA,
  PROP_SHOW_EDITOR,
  PROP_MODAL
};

/* Signals */
enum
{
  COLOR_SET,
  ACTIVATE,
  LAST_SIGNAL
};

/* gobject signals */
static void bobgui_color_button_finalize      (GObject          *object);
static void bobgui_color_button_set_property  (GObject          *object,
                                            guint             param_id,
                                            const GValue     *value,
                                            GParamSpec       *pspec);
static void bobgui_color_button_get_property  (GObject          *object,
                                            guint             param_id,
                                            GValue           *value,
                                            GParamSpec       *pspec);

static void bobgui_color_button_root          (BobguiWidget        *widget);

/* bobguibutton signals */
static void bobgui_color_button_clicked       (BobguiButton        *button,
                                            gpointer          user_data);


static guint color_button_signals[LAST_SIGNAL] = { 0 };

static void bobgui_color_button_iface_init (BobguiColorChooserInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiColorButton, bobgui_color_button, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_COLOR_CHOOSER,
                                                bobgui_color_button_iface_init))

static void
bobgui_color_button_activate (BobguiColorButton *self)
{
  bobgui_widget_activate (self->button);
}

static void
bobgui_color_button_class_init (BobguiColorButtonClass *klass)
{
  GObjectClass *gobject_class;
  BobguiWidgetClass *widget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  widget_class = BOBGUI_WIDGET_CLASS (klass);

  gobject_class->get_property = bobgui_color_button_get_property;
  gobject_class->set_property = bobgui_color_button_set_property;
  gobject_class->finalize = bobgui_color_button_finalize;

  widget_class->grab_focus = bobgui_widget_grab_focus_child;
  widget_class->focus = bobgui_widget_focus_child;
  widget_class->root = bobgui_color_button_root;

  klass->color_set = NULL;
  klass->activate = bobgui_color_button_activate;

  g_object_class_override_property (gobject_class, PROP_RGBA, "rgba");
  g_object_class_override_property (gobject_class, PROP_USE_ALPHA, "use-alpha");

  /**
   * BobguiColorButton:title:
   *
   * The title of the color chooser dialog
   */
  g_object_class_install_property (gobject_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title", NULL, NULL,
                                                        _("Pick a Color"),
                                                        BOBGUI_PARAM_READWRITE));


  /**
   * BobguiColorButton::color-set:
   * @widget: the object which received the signal.
   *
   * Emitted when the user selects a color.
   *
   * When handling this signal, use [method@Bobgui.ColorChooser.get_rgba]
   * to find out which color was just selected.
   *
   * Note that this signal is only emitted when the user changes the color.
   * If you need to react to programmatic color changes as well, use
   * the notify::rgba signal.
   */
  color_button_signals[COLOR_SET] = g_signal_new (I_("color-set"),
                                                  G_TYPE_FROM_CLASS (gobject_class),
                                                  G_SIGNAL_RUN_FIRST,
                                                  G_STRUCT_OFFSET (BobguiColorButtonClass, color_set),
                                                  NULL, NULL,
                                                  NULL,
                                                  G_TYPE_NONE, 0);

  /**
   * BobguiColorButton::activate:
   * @widget: the object which received the signal.
   *
   * Emitted to when the color button is activated.
   *
   * The `::activate` signal on `BobguiMenuButton` is an action signal and
   * emitting it causes the button to pop up its dialog.
   *
   * Since: 4.4
   */
  color_button_signals[ACTIVATE] =
      g_signal_new (I_ ("activate"),
                    G_OBJECT_CLASS_TYPE (gobject_class),
                    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                    G_STRUCT_OFFSET (BobguiColorButtonClass, activate),
                    NULL, NULL,
                    NULL,
                    G_TYPE_NONE, 0);

  bobgui_widget_class_set_activate_signal (widget_class, color_button_signals[ACTIVATE]);

  /**
   * BobguiColorButton:show-editor:
   *
   * Whether the color chooser should open in editor mode.
   *
   * This property should be used in cases where the palette
   * in the editor would be redundant, such as when the color
   * button is already part of a palette.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SHOW_EDITOR,
                                   g_param_spec_boolean ("show-editor", NULL, NULL,
                                                         FALSE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  /**
   * BobguiColorButton:modal:
   *
   * Whether the color chooser dialog should be modal.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_MODAL,
                                   g_param_spec_boolean ("modal", NULL, NULL,
                                                         TRUE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));


  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, "colorbutton");
}

static gboolean
bobgui_color_button_drop (BobguiDropTarget  *dest,
                       const GValue   *value,
                       double          x,
                       double          y,
                       BobguiColorButton *button)
{
  GdkRGBA *color = g_value_get_boxed (value);

  bobgui_color_chooser_set_rgba (BOBGUI_COLOR_CHOOSER (button), color);
  return TRUE;
}

static GdkContentProvider *
bobgui_color_button_drag_prepare (BobguiDragSource  *source,
                               double          x,
                               double          y,
                               BobguiColorButton *button)
{
  return gdk_content_provider_new_typed (GDK_TYPE_RGBA, &button->rgba);
}

static void
bobgui_color_button_init (BobguiColorButton *button)
{
  PangoLayout *layout;
  PangoRectangle rect;
  BobguiDragSource *source;
  BobguiDropTarget *dest;

  button->button = bobgui_button_new ();
  g_signal_connect (button->button, "clicked", G_CALLBACK (bobgui_color_button_clicked), button);
  g_object_bind_property (button, "focus-on-click",
                          button->button, "focus-on-click",
                          0);
  bobgui_widget_set_parent (button->button, BOBGUI_WIDGET (button));

  button->swatch = g_object_new (BOBGUI_TYPE_COLOR_SWATCH,
                                 "accessible-role", BOBGUI_ACCESSIBLE_ROLE_IMG,
                                 "selectable", FALSE,
                                 "has-menu", FALSE,
                                 "can-drag", FALSE,
                                 NULL);
  bobgui_widget_set_can_focus (button->swatch, FALSE);
  bobgui_widget_remove_css_class (button->swatch, "activatable");
  layout = bobgui_widget_create_pango_layout (BOBGUI_WIDGET (button), "Black");
  pango_layout_get_pixel_extents (layout, NULL, &rect);
  g_object_unref (layout);

  bobgui_widget_set_size_request (button->swatch, rect.width, rect.height);

  bobgui_button_set_child (BOBGUI_BUTTON (button->button), button->swatch);

  button->title = g_strdup (_("Pick a Color")); /* default title */

  /* Start with opaque black, alpha disabled */
  button->rgba.red = 0;
  button->rgba.green = 0;
  button->rgba.blue = 0;
  button->rgba.alpha = 1;
  button->use_alpha = FALSE;
  button->modal = TRUE;

  dest = bobgui_drop_target_new (GDK_TYPE_RGBA, GDK_ACTION_COPY);
  g_signal_connect (dest, "drop", G_CALLBACK (bobgui_color_button_drop), button);
  bobgui_widget_add_controller (BOBGUI_WIDGET (button), BOBGUI_EVENT_CONTROLLER (dest));

  source = bobgui_drag_source_new ();
  g_signal_connect (source, "prepare", G_CALLBACK (bobgui_color_button_drag_prepare), button);
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (source), BOBGUI_PHASE_CAPTURE);
  bobgui_widget_add_controller (button->button, BOBGUI_EVENT_CONTROLLER (source));

  bobgui_widget_add_css_class (button->button, "color");
}

static void
bobgui_color_button_finalize (GObject *object)
{
  BobguiColorButton *button = BOBGUI_COLOR_BUTTON (object);

  if (button->cs_dialog != NULL)
    bobgui_window_destroy (BOBGUI_WINDOW (button->cs_dialog));

  g_free (button->title);
  bobgui_widget_unparent (button->button);

  G_OBJECT_CLASS (bobgui_color_button_parent_class)->finalize (object);
}


/**
 * bobgui_color_button_new:
 *
 * Creates a new color button.
 *
 * This returns a widget in the form of a small button containing
 * a swatch representing the current selected color. When the button
 * is clicked, a color chooser dialog will open, allowing the user
 * to select a color. The swatch will be updated to reflect the new
 * color when the user finishes.
 *
 * Returns: a new color button
 *
 * Deprecated: 4.10: Use [class@Bobgui.ColorDialogButton] instead
 */
BobguiWidget *
bobgui_color_button_new (void)
{
  return g_object_new (BOBGUI_TYPE_COLOR_BUTTON, NULL);
}

/**
 * bobgui_color_button_new_with_rgba:
 * @rgba: A `GdkRGBA` to set the current color with
 *
 * Creates a new color button showing the given color.
 *
 * Returns: a new color button
 */
BobguiWidget *
bobgui_color_button_new_with_rgba (const GdkRGBA *rgba)
{
  return g_object_new (BOBGUI_TYPE_COLOR_BUTTON, "rgba", rgba, NULL);
}

static gboolean
dialog_destroy (BobguiWidget *widget,
                gpointer   data)
{
  BobguiColorButton *button = BOBGUI_COLOR_BUTTON (data);

  button->cs_dialog = NULL;

  return FALSE;
}

static void
dialog_response (BobguiDialog *dialog,
                 int        response,
                 gpointer   data)
{
  if (response == BOBGUI_RESPONSE_CANCEL)
    bobgui_widget_hide (BOBGUI_WIDGET (dialog));
  else if (response == BOBGUI_RESPONSE_OK)
    {
      BobguiColorButton *button = BOBGUI_COLOR_BUTTON (data);

      bobgui_color_chooser_get_rgba (BOBGUI_COLOR_CHOOSER (dialog), &button->rgba);
      bobgui_color_swatch_set_rgba (BOBGUI_COLOR_SWATCH (button->swatch), &button->rgba);

      bobgui_widget_hide (BOBGUI_WIDGET (dialog));
      g_object_ref (button);
      g_signal_emit (button, color_button_signals[COLOR_SET], 0);

      g_object_freeze_notify (G_OBJECT (button));
      g_object_notify (G_OBJECT (button), "rgba");
      g_object_thaw_notify (G_OBJECT (button));
      g_object_unref (button);
    }
}

/* Create the dialog and connects its buttons */
static void
ensure_dialog (BobguiColorButton *button)
{
  BobguiWidget *parent, *dialog;

  if (button->cs_dialog != NULL)
    return;

  parent = BOBGUI_WIDGET (bobgui_widget_get_root (BOBGUI_WIDGET (button)));

  button->cs_dialog = dialog = bobgui_color_chooser_dialog_new (button->title, NULL);
  bobgui_window_set_hide_on_close (BOBGUI_WINDOW (dialog), TRUE);
  bobgui_window_set_modal (BOBGUI_WINDOW (dialog), button->modal);

  if (BOBGUI_IS_WINDOW (parent))
    {
      if (BOBGUI_WINDOW (parent) != bobgui_window_get_transient_for (BOBGUI_WINDOW (dialog)))
        bobgui_window_set_transient_for (BOBGUI_WINDOW (dialog), BOBGUI_WINDOW (parent));

      if (bobgui_window_get_modal (BOBGUI_WINDOW (parent)))
        bobgui_window_set_modal (BOBGUI_WINDOW (dialog), TRUE);
    }

  g_signal_connect (dialog, "response",
                    G_CALLBACK (dialog_response), button);
  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (dialog_destroy), button);
}

static void
bobgui_color_button_root (BobguiWidget *widget)
{
  BobguiColorButton *button = BOBGUI_COLOR_BUTTON (widget);
  BobguiWidget *parent;

  BOBGUI_WIDGET_CLASS (bobgui_color_button_parent_class)->root (widget);

  if (!button->cs_dialog)
    return;

  parent = BOBGUI_WIDGET (bobgui_widget_get_root (BOBGUI_WIDGET (button)));
  if (BOBGUI_IS_WINDOW (parent))
    {
      if (BOBGUI_WINDOW (parent) != bobgui_window_get_transient_for (BOBGUI_WINDOW (button->cs_dialog)))
        bobgui_window_set_transient_for (BOBGUI_WINDOW (button->cs_dialog), BOBGUI_WINDOW (parent));

      if (bobgui_window_get_modal (BOBGUI_WINDOW (parent)))
        bobgui_window_set_modal (BOBGUI_WINDOW (button->cs_dialog), TRUE);
    }
}

static void
bobgui_color_button_clicked (BobguiButton *b,
                          gpointer   user_data)
{
  BobguiColorButton *button = user_data;

  /* if dialog already exists, make sure it's shown and raised */
  ensure_dialog (button);

  g_object_set (button->cs_dialog, "show-editor", button->show_editor, NULL);

  bobgui_color_chooser_set_use_alpha (BOBGUI_COLOR_CHOOSER (button->cs_dialog), button->use_alpha);

  bobgui_color_chooser_set_rgba (BOBGUI_COLOR_CHOOSER (button->cs_dialog), &button->rgba);

  bobgui_window_present (BOBGUI_WINDOW (button->cs_dialog));
}

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

static void
bobgui_color_button_set_rgba (BobguiColorChooser *chooser,
                           const GdkRGBA   *rgba)
{
  BobguiColorButton *button = BOBGUI_COLOR_BUTTON (chooser);
  char *text;

  g_return_if_fail (BOBGUI_IS_COLOR_BUTTON (chooser));
  g_return_if_fail (rgba != NULL);

  button->rgba = *rgba;
  bobgui_color_swatch_set_rgba (BOBGUI_COLOR_SWATCH (button->swatch), &button->rgba);

  text = accessible_color_name (rgba);
  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (button->swatch),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL, text,
                                  -1);
  g_free (text);

  g_object_notify (G_OBJECT (chooser), "rgba");
}

static void
bobgui_color_button_get_rgba (BobguiColorChooser *chooser,
                           GdkRGBA        *rgba)
{
  BobguiColorButton *button = BOBGUI_COLOR_BUTTON (chooser);

  g_return_if_fail (BOBGUI_IS_COLOR_BUTTON (chooser));
  g_return_if_fail (rgba != NULL);

  *rgba = button->rgba;
}

static void
set_use_alpha (BobguiColorButton *button,
               gboolean        use_alpha)
{
  use_alpha = (use_alpha != FALSE);

  if (button->use_alpha != use_alpha)
    {
      button->use_alpha = use_alpha;

      bobgui_color_swatch_set_use_alpha (BOBGUI_COLOR_SWATCH (button->swatch), use_alpha);

      g_object_notify (G_OBJECT (button), "use-alpha");
    }
}

/**
 * bobgui_color_button_set_title:
 * @button: a `BobguiColorButton`
 * @title: String containing new window title
 *
 * Sets the title for the color chooser dialog.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ColorDialogButton] instead
 */
void
bobgui_color_button_set_title (BobguiColorButton *button,
                            const char     *title)
{
  char *old_title;

  g_return_if_fail (BOBGUI_IS_COLOR_BUTTON (button));

  old_title = button->title;
  button->title = g_strdup (title);
  g_free (old_title);

  if (button->cs_dialog)
    bobgui_window_set_title (BOBGUI_WINDOW (button->cs_dialog), button->title);

  g_object_notify (G_OBJECT (button), "title");
}

/**
 * bobgui_color_button_get_title:
 * @button: a `BobguiColorButton`
 *
 * Gets the title of the color chooser dialog.
 *
 * Returns: An internal string, do not free the return value
 *
 * Deprecated: 4.10: Use [class@Bobgui.ColorDialogButton] instead
 */
const char *
bobgui_color_button_get_title (BobguiColorButton *button)
{
  g_return_val_if_fail (BOBGUI_IS_COLOR_BUTTON (button), NULL);

  return button->title;
}

/**
 * bobgui_color_button_set_modal:
 * @button: a `BobguiColorButton`
 * @modal: %TRUE to make the dialog modal
 *
 * Sets whether the dialog should be modal.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ColorDialogButton] instead
 */
void
bobgui_color_button_set_modal (BobguiColorButton *button,
                            gboolean        modal)
{
  g_return_if_fail (BOBGUI_IS_COLOR_BUTTON (button));

  if (button->modal == modal)
    return;

  button->modal = modal;

  if (button->cs_dialog)
    bobgui_window_set_modal (BOBGUI_WINDOW (button->cs_dialog), button->modal);

  g_object_notify (G_OBJECT (button), "modal");
}

/**
 * bobgui_color_button_get_modal:
 * @button: a `BobguiColorButton`
 *
 * Gets whether the dialog is modal.
 *
 * Returns: %TRUE if the dialog is modal
 *
 * Deprecated: 4.10: Use [class@Bobgui.ColorDialogButton] instead
 */
gboolean
bobgui_color_button_get_modal (BobguiColorButton *button)
{
  g_return_val_if_fail (BOBGUI_IS_COLOR_BUTTON (button), FALSE);

  return button->modal;
}

static void
bobgui_color_button_set_property (GObject      *object,
                               guint         param_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  BobguiColorButton *button = BOBGUI_COLOR_BUTTON (object);

  switch (param_id)
    {
    case PROP_USE_ALPHA:
      set_use_alpha (button, g_value_get_boolean (value));
      break;
    case PROP_TITLE:
      bobgui_color_button_set_title (button, g_value_get_string (value));
      break;
    case PROP_RGBA:
      bobgui_color_chooser_set_rgba (BOBGUI_COLOR_CHOOSER (button), g_value_get_boxed (value));
      break;
    case PROP_SHOW_EDITOR:
      {
        gboolean show_editor = g_value_get_boolean (value);
        if (button->show_editor != show_editor)
          {
            button->show_editor = show_editor;
            g_object_notify (object, "show-editor");
          }
      }
      break;
    case PROP_MODAL:
      bobgui_color_button_set_modal (button, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bobgui_color_button_get_property (GObject    *object,
                               guint       param_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  BobguiColorButton *button = BOBGUI_COLOR_BUTTON (object);

  switch (param_id)
    {
    case PROP_USE_ALPHA:
      g_value_set_boolean (value, button->use_alpha);
      break;
    case PROP_TITLE:
      g_value_set_string (value, bobgui_color_button_get_title (button));
      break;
    case PROP_RGBA:
      {
        GdkRGBA rgba;

        bobgui_color_chooser_get_rgba (BOBGUI_COLOR_CHOOSER (button), &rgba);
        g_value_set_boxed (value, &rgba);
      }
      break;
    case PROP_SHOW_EDITOR:
      g_value_set_boolean (value, button->show_editor);
      break;
    case PROP_MODAL:
      g_value_set_boolean (value, button->modal);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
bobgui_color_button_add_palette (BobguiColorChooser *chooser,
                              BobguiOrientation   orientation,
                              int              colors_per_line,
                              int              n_colors,
                              GdkRGBA         *colors)
{
  BobguiColorButton *button = BOBGUI_COLOR_BUTTON (chooser);

  ensure_dialog (button);

  bobgui_color_chooser_add_palette (BOBGUI_COLOR_CHOOSER (button->cs_dialog),
                                 orientation, colors_per_line, n_colors, colors);
}

static void
bobgui_color_button_iface_init (BobguiColorChooserInterface *iface)
{
  iface->get_rgba = bobgui_color_button_get_rgba;
  iface->set_rgba = bobgui_color_button_set_rgba;
  iface->add_palette = bobgui_color_button_add_palette;
}
