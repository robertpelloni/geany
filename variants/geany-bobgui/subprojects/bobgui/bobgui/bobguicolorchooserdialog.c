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

#include "deprecated/bobguidialog.h"
#include "deprecated/bobguidialogprivate.h"
#include "bobguibutton.h"
#include "bobguibox.h"
#include "bobguiprivate.h"
#include "bobguisettings.h"

#include "deprecated/bobguicolorchooserprivate.h"
#include "deprecated/bobguicolorchooserdialog.h"
#include "deprecated/bobguicolorchooserwidget.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiColorChooserDialog:
 *
 * A dialog for choosing a color.
 *
 * <picture>
 *   <source srcset="colorchooser-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiColorChooserDialog" src="colorchooser.png">
 * </picture>
 *
 * `BobguiColorChooserDialog` implements the [iface@Bobgui.ColorChooser] interface
 * and does not provide much API of its own.
 *
 * To create a `BobguiColorChooserDialog`, use [ctor@Bobgui.ColorChooserDialog.new].
 *
 * To change the initially selected color, use
 * [method@Bobgui.ColorChooser.set_rgba]. To get the selected color use
 * [method@Bobgui.ColorChooser.get_rgba].
 *
 * `BobguiColorChooserDialog` has been deprecated in favor of [class@Bobgui.ColorDialog].
 *
 * ## CSS nodes
 *
 * `BobguiColorChooserDialog` has a single CSS node with the name `window` and style
 * class `.colorchooser`.
 *
 * Deprecated: 4.10: Use [class@Bobgui.ColorDialog] instead
 */

typedef struct _BobguiColorChooserDialogClass   BobguiColorChooserDialogClass;

struct _BobguiColorChooserDialog
{
  BobguiDialog parent_instance;

  BobguiWidget *chooser;
  BobguiWidget *scroller;
};

struct _BobguiColorChooserDialogClass
{
  BobguiDialogClass parent_class;
};

enum
{
  PROP_ZERO,
  PROP_RGBA,
  PROP_USE_ALPHA,
  PROP_SHOW_EDITOR
};

static void bobgui_color_chooser_dialog_iface_init (BobguiColorChooserInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiColorChooserDialog, bobgui_color_chooser_dialog, BOBGUI_TYPE_DIALOG,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_COLOR_CHOOSER,
                                                bobgui_color_chooser_dialog_iface_init))

static void
propagate_notify (GObject               *o,
                  GParamSpec            *pspec,
                  BobguiColorChooserDialog *cc)
{
  g_object_notify (G_OBJECT (cc), pspec->name);
}

static void
save_color (BobguiColorChooserDialog *dialog)
{
  GdkRGBA color;

  /* This causes the color chooser widget to save the
   * selected and custom colors to GSettings.
   */
  bobgui_color_chooser_get_rgba (BOBGUI_COLOR_CHOOSER (dialog), &color);
  bobgui_color_chooser_set_rgba (BOBGUI_COLOR_CHOOSER (dialog), &color);
}

static void
color_activated_cb (BobguiColorChooser *chooser,
                    GdkRGBA         *color,
                    BobguiDialog       *dialog)
{
  save_color (BOBGUI_COLOR_CHOOSER_DIALOG (dialog));
  bobgui_dialog_response (dialog, BOBGUI_RESPONSE_OK);
}

static void
bobgui_color_chooser_dialog_response (BobguiDialog *dialog,
                                   int        response_id,
                                   gpointer   user_data)
{
  if (response_id == BOBGUI_RESPONSE_OK)
    save_color (BOBGUI_COLOR_CHOOSER_DIALOG (dialog));
}

static void
bobgui_color_chooser_dialog_init (BobguiColorChooserDialog *cc)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (cc));
  bobgui_dialog_set_use_header_bar_from_setting (BOBGUI_DIALOG (cc));

  g_signal_connect (cc, "response",
                    G_CALLBACK (bobgui_color_chooser_dialog_response), NULL);
}

static void
bobgui_color_chooser_dialog_unmap (BobguiWidget *widget)
{
  BOBGUI_WIDGET_CLASS (bobgui_color_chooser_dialog_parent_class)->unmap (widget);

  /* We never want the dialog to come up with the editor,
   * even if it was showing the editor the last time it was used.
   */
  g_object_set (widget, "show-editor", FALSE, NULL);
}

static void
bobgui_color_chooser_dialog_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  BobguiColorChooserDialog *cc = BOBGUI_COLOR_CHOOSER_DIALOG (object);

  switch (prop_id)
    {
    case PROP_RGBA:
      {
        GdkRGBA color;

        bobgui_color_chooser_get_rgba (BOBGUI_COLOR_CHOOSER (cc), &color);
        g_value_set_boxed (value, &color);
      }
      break;
    case PROP_USE_ALPHA:
      g_value_set_boolean (value, bobgui_color_chooser_get_use_alpha (BOBGUI_COLOR_CHOOSER (cc->chooser)));
      break;
    case PROP_SHOW_EDITOR:
      {
        gboolean show_editor;
        g_object_get (cc->chooser, "show-editor", &show_editor, NULL);
        g_value_set_boolean (value, show_editor);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_color_chooser_dialog_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  BobguiColorChooserDialog *cc = BOBGUI_COLOR_CHOOSER_DIALOG (object);

  switch (prop_id)
    {
    case PROP_RGBA:
      bobgui_color_chooser_set_rgba (BOBGUI_COLOR_CHOOSER (cc), g_value_get_boxed (value));
      break;
    case PROP_USE_ALPHA:
      if (bobgui_color_chooser_get_use_alpha (BOBGUI_COLOR_CHOOSER (cc->chooser)) != g_value_get_boolean (value))
        {
          bobgui_color_chooser_set_use_alpha (BOBGUI_COLOR_CHOOSER (cc->chooser), g_value_get_boolean (value));
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_SHOW_EDITOR:
      g_object_set (cc->chooser,
                    "show-editor", g_value_get_boolean (value),
                    NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_color_chooser_dialog_dispose (GObject *object)
{
  BobguiColorChooserDialog *cc = BOBGUI_COLOR_CHOOSER_DIALOG (object);

  g_clear_pointer (&cc->scroller, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_color_chooser_dialog_parent_class)->dispose (object);
}

static void
bobgui_color_chooser_dialog_class_init (BobguiColorChooserDialogClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = bobgui_color_chooser_dialog_dispose;
  object_class->get_property = bobgui_color_chooser_dialog_get_property;
  object_class->set_property = bobgui_color_chooser_dialog_set_property;

  widget_class->unmap = bobgui_color_chooser_dialog_unmap;

  g_object_class_override_property (object_class, PROP_RGBA, "rgba");
  g_object_class_override_property (object_class, PROP_USE_ALPHA, "use-alpha");

  /**
   * BobguiColorChooserDialog:show-editor:
   *
   * Whether the color chooser dialog is showing the single-color editor.
   *
   * It can be set to switch the color chooser into single-color editing mode.
   */
  g_object_class_install_property (object_class, PROP_SHOW_EDITOR,
      g_param_spec_boolean ("show-editor", NULL, NULL,
                            FALSE, BOBGUI_PARAM_READWRITE));

  /* Bind class to template
   */
  bobgui_widget_class_set_template_from_resource (widget_class,
					       "/org/bobgui/libbobgui/ui/bobguicolorchooserdialog.ui");
  bobgui_widget_class_bind_template_child (widget_class, BobguiColorChooserDialog, chooser);
  bobgui_widget_class_bind_template_child (widget_class, BobguiColorChooserDialog, scroller);
  bobgui_widget_class_bind_template_callback (widget_class, propagate_notify);
  bobgui_widget_class_bind_template_callback (widget_class, color_activated_cb);
}

static void
bobgui_color_chooser_dialog_get_rgba (BobguiColorChooser *chooser,
                                   GdkRGBA         *color)
{
  BobguiColorChooserDialog *cc = BOBGUI_COLOR_CHOOSER_DIALOG (chooser);

  bobgui_color_chooser_get_rgba (BOBGUI_COLOR_CHOOSER (cc->chooser), color);
}

static void
bobgui_color_chooser_dialog_set_rgba (BobguiColorChooser *chooser,
                                   const GdkRGBA   *color)
{
  BobguiColorChooserDialog *cc = BOBGUI_COLOR_CHOOSER_DIALOG (chooser);

  bobgui_color_chooser_set_rgba (BOBGUI_COLOR_CHOOSER (cc->chooser), color);
}

static void
bobgui_color_chooser_dialog_add_palette (BobguiColorChooser *chooser,
                                      BobguiOrientation   orientation,
                                      int              colors_per_line,
                                      int              n_colors,
                                      GdkRGBA         *colors)
{
  BobguiColorChooserDialog *cc = BOBGUI_COLOR_CHOOSER_DIALOG (chooser);

  bobgui_color_chooser_add_palette (BOBGUI_COLOR_CHOOSER (cc->chooser),
                                 orientation, colors_per_line, n_colors, colors);
}

static void
bobgui_color_chooser_dialog_iface_init (BobguiColorChooserInterface *iface)
{
  iface->get_rgba = bobgui_color_chooser_dialog_get_rgba;
  iface->set_rgba = bobgui_color_chooser_dialog_set_rgba;
  iface->add_palette = bobgui_color_chooser_dialog_add_palette;
}

/**
 * bobgui_color_chooser_dialog_new:
 * @title: (nullable): Title of the dialog
 * @parent: (nullable): Transient parent of the dialog
 *
 * Creates a new `BobguiColorChooserDialog`.
 *
 * Returns: a new `BobguiColorChooserDialog`
 *
 * Deprecated: 4.10: Use [class@Bobgui.ColorDialog] instead
 */
BobguiWidget *
bobgui_color_chooser_dialog_new (const char *title,
                              BobguiWindow   *parent)
{
  return g_object_new (BOBGUI_TYPE_COLOR_CHOOSER_DIALOG,
                       "title", title,
                       "transient-for", parent,
                       NULL);
}
