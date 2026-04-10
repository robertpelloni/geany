/* BOBGUI - The Bobgui Framework
 *
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

#include "deprecated/bobguicolorchooserprivate.h"
#include "deprecated/bobguicolorchooserwidget.h"
#include "bobguicolorchooserwidgetprivate.h"
#include "bobguicoloreditorprivate.h"
#include "bobguicolorswatchprivate.h"
#include "bobguigrid.h"
#include "bobguilabel.h"
#include "bobguiorientable.h"
#include "bobguiprivate.h"
#include <glib/gi18n-lib.h>
#include "bobguisizegroup.h"
#include "bobguiboxlayout.h"
#include "bobguiwidgetprivate.h"
#include "gdkrgbaprivate.h"

#include <math.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiColorChooserWidget:
 *
 * The `BobguiColorChooserWidget` widget lets the user select a color.
 *
 * By default, the chooser presents a predefined palette of colors,
 * plus a small number of settable custom colors. It is also possible
 * to select a different color with the single-color editor.
 *
 * To enter the single-color editing mode, use the context menu of any
 * color of the palette, or use the '+' button to add a new custom color.
 *
 * The chooser automatically remembers the last selection, as well
 * as custom colors.
 *
 * To create a `BobguiColorChooserWidget`, use [ctor@Bobgui.ColorChooserWidget.new].
 *
 * To change the initially selected color, use
 * [method@Bobgui.ColorChooser.set_rgba]. To get the selected color use
 * [method@Bobgui.ColorChooser.get_rgba].
 *
 * The `BobguiColorChooserWidget` is used in the [class@Bobgui.ColorChooserDialog]
 * to provide a dialog for selecting colors.
 *
 * # Actions
 *
 * `BobguiColorChooserWidget` defines a set of built-in actions:
 *
 * - `color.customize` activates the color editor for the given color.
 * - `color.select` emits the [signal@Bobgui.ColorChooser::color-activated] signal
 *   for the given color.
 *
 * # CSS names
 *
 * `BobguiColorChooserWidget` has a single CSS node with name colorchooser.
 *
 * Deprecated: 4.10: Direct use of `BobguiColorChooserWidget` is deprecated.
 */

typedef struct _BobguiColorChooserWidgetClass   BobguiColorChooserWidgetClass;

struct _BobguiColorChooserWidget
{
  BobguiWidget parent_instance;

  BobguiWidget *palette;
  BobguiWidget *editor;
  BobguiSizeGroup *size_group;

  BobguiWidget *custom_label;
  BobguiWidget *custom;

  BobguiWidget *button;
  BobguiColorSwatch *current;

  gboolean use_alpha;
  gboolean has_default_palette;

  GSettings *settings;

  int max_custom;
};

struct _BobguiColorChooserWidgetClass
{
  BobguiWidgetClass parent_class;
};

enum
{
  PROP_ZERO,
  PROP_RGBA,
  PROP_USE_ALPHA,
  PROP_SHOW_EDITOR
};

static void bobgui_color_chooser_widget_iface_init (BobguiColorChooserInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiColorChooserWidget, bobgui_color_chooser_widget, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_COLOR_CHOOSER,
                                                bobgui_color_chooser_widget_iface_init))

static void
select_swatch (BobguiColorChooserWidget *cc,
               BobguiColorSwatch        *swatch)
{
  GdkRGBA color;
  double red, green, blue, alpha;

  if (cc->current == swatch)
    return;

  if (cc->current != NULL)
    bobgui_widget_unset_state_flags (BOBGUI_WIDGET (cc->current), BOBGUI_STATE_FLAG_SELECTED);

  bobgui_widget_set_state_flags (BOBGUI_WIDGET (swatch), BOBGUI_STATE_FLAG_SELECTED, FALSE);
  cc->current = swatch;

  bobgui_color_swatch_get_rgba (swatch, &color);

  red = color.red;
  green = color.green;
  blue = color.blue;
  alpha = color.alpha;
  g_settings_set (cc->settings, "selected-color", "(bdddd)",
                  TRUE, red, green, blue, alpha);

  if (bobgui_widget_get_visible (BOBGUI_WIDGET (cc->editor)))
    bobgui_color_chooser_set_rgba (BOBGUI_COLOR_CHOOSER (cc->editor), &color);
  else
    g_object_notify (G_OBJECT (cc), "rgba");
}

static void
swatch_selected (BobguiColorSwatch        *swatch,
                 BobguiStateFlags          previous,
                 BobguiColorChooserWidget *cc)
{
  BobguiStateFlags flags;

  flags = bobgui_widget_get_state_flags (BOBGUI_WIDGET (swatch));
  if ((flags & BOBGUI_STATE_FLAG_SELECTED) != (previous & BOBGUI_STATE_FLAG_SELECTED) &&
      (flags & BOBGUI_STATE_FLAG_SELECTED) != 0)
    select_swatch (cc, swatch);
}

static void
connect_swatch_signals (BobguiWidget *p,
                        gpointer   data)
{
  g_signal_connect (p, "state-flags-changed", G_CALLBACK (swatch_selected), data);
}

static void
connect_button_signals (BobguiWidget *p,
                        gpointer   data)
{
//  g_signal_connect (p, "activate", G_CALLBACK (button_activate), data);
}

static void
save_custom_colors (BobguiColorChooserWidget *cc)
{
  GVariantBuilder builder;
  GVariant *variant;
  GdkRGBA color;
  BobguiWidget *child;
  gboolean first;

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a(dddd)"));

  for (child = bobgui_widget_get_first_child (cc->custom), first = TRUE;
       child != NULL;
       child = bobgui_widget_get_next_sibling (child), first = FALSE)
    {
      if (first)
        continue;

      if (bobgui_color_swatch_get_rgba (BOBGUI_COLOR_SWATCH (child), &color))
        {
          double red, green, blue, alpha;

          red = color.red;
          green = color.green;
          blue = color.blue;
          alpha = color.alpha;
          g_variant_builder_add (&builder, "(dddd)", red, green, blue, alpha);
        }
    }

  variant = g_variant_builder_end (&builder);
  g_settings_set_value (cc->settings, "custom-colors", variant);
}

static void
connect_custom_signals (BobguiWidget *p,
                        gpointer   data)
{
  connect_swatch_signals (p, data);
  g_signal_connect_swapped (p, "notify::rgba",
                            G_CALLBACK (save_custom_colors), data);
}

static void
bobgui_color_chooser_widget_set_use_alpha (BobguiColorChooserWidget *cc,
                                        gboolean               use_alpha)
{
  BobguiWidget *child;
  BobguiWidget *grid;

  if (cc->use_alpha == use_alpha)
    return;

  cc->use_alpha = use_alpha;
  bobgui_color_chooser_set_use_alpha (BOBGUI_COLOR_CHOOSER (cc->editor), use_alpha);

  for (grid = bobgui_widget_get_first_child (cc->palette);
       grid != NULL;
       grid = bobgui_widget_get_next_sibling (grid))
    {
      for (child = bobgui_widget_get_first_child (grid);
           child != NULL;
           child = bobgui_widget_get_next_sibling (child))
        {
          if (BOBGUI_IS_COLOR_SWATCH (child))
            bobgui_color_swatch_set_use_alpha (BOBGUI_COLOR_SWATCH (child), use_alpha);
        }
    }

  bobgui_widget_queue_draw (BOBGUI_WIDGET (cc));
  g_object_notify (G_OBJECT (cc), "use-alpha");
}

static void
bobgui_color_chooser_widget_set_show_editor (BobguiColorChooserWidget *cc,
                                          gboolean               show_editor)
{
  if (show_editor)
    {
      GdkRGBA color = { 0.75, 0.25, 0.25, 1.0 };

      if (cc->current)
        bobgui_color_swatch_get_rgba (cc->current, &color);
      bobgui_color_chooser_set_rgba (BOBGUI_COLOR_CHOOSER (cc->editor), &color);
    }

  bobgui_widget_set_visible (cc->editor, show_editor);
  bobgui_widget_set_visible (cc->palette, !show_editor);
  if (show_editor)
    bobgui_widget_grab_focus (cc->editor);
}

static void
update_from_editor (BobguiColorEditor        *editor,
                    GParamSpec            *pspec,
                    BobguiColorChooserWidget *widget)
{
  if (bobgui_widget_get_visible (BOBGUI_WIDGET (editor)))
    g_object_notify (G_OBJECT (widget), "rgba");
}

/* UI construction {{{1 */

static void
remove_palette (BobguiColorChooserWidget *cc)
{
  GList *children, *l;
  BobguiWidget *widget;

  if (cc->current != NULL &&
      bobgui_widget_get_parent (BOBGUI_WIDGET (cc->current)) != cc->custom)
    cc->current = NULL;

  children = NULL;
  for (widget = bobgui_widget_get_first_child (cc->palette);
       widget != NULL;
       widget = bobgui_widget_get_next_sibling (widget))
    children = g_list_prepend (children, widget);

  for (l = children; l; l = l->next)
    {
      widget = l->data;
      if (widget == cc->custom_label || widget == cc->custom)
        continue;
      bobgui_box_remove (BOBGUI_BOX (cc->palette), widget);
    }
  g_list_free (children);
}

static guint
scale_round (double value,
             double scale)
{
  value = floor (value * scale + 0.5);
  value = MAX (value, 0);
  value = MIN (value, scale);
  return (guint)value;
}

char *
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
add_palette (BobguiColorChooserWidget  *cc,
             BobguiOrientation          orientation,
             int                     colors_per_line,
             int                     n_colors,
             GdkRGBA                *colors,
             const char            **names)
{
  BobguiWidget *grid;
  BobguiWidget *p;
  int line, pos;
  int i;
  int left, right;

  if (colors == NULL)
    {
      remove_palette (cc);
      return;
    }

  grid = bobgui_grid_new ();
  bobgui_widget_set_margin_bottom (grid, 12);
  bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 2);
  bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 4);
  bobgui_box_append (BOBGUI_BOX (cc->palette), grid);

  left = 0;
  right = colors_per_line - 1;
  if (bobgui_widget_get_direction (BOBGUI_WIDGET (cc)) == BOBGUI_TEXT_DIR_RTL)
    {
      i = left;
      left = right;
      right = i;
    }

  for (i = 0; i < n_colors; i++)
    {
      p = bobgui_color_swatch_new ();
      if (names)
        {
          bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (p),
                                          BOBGUI_ACCESSIBLE_PROPERTY_LABEL,
                                          g_dpgettext2 (GETTEXT_PACKAGE, "Color name", names[i]),
                                          -1);
        }
      else
        {
          char *name;
          char *text;

          name = accessible_color_name (&colors[i]);
          text = g_strdup_printf (_("Color: %s"), name);
          bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (p),
                                          BOBGUI_ACCESSIBLE_PROPERTY_LABEL, text,
                                          -1);
          g_free (name);
          g_free (text);
        }
      bobgui_color_swatch_set_rgba (BOBGUI_COLOR_SWATCH (p), &colors[i]);
      connect_swatch_signals (p, cc);

      line = i / colors_per_line;
      pos = i % colors_per_line;

      if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        {
          if (pos == left)
            bobgui_widget_add_css_class (p, "left");
          else if (pos == right)
            bobgui_widget_add_css_class (p, "right");

          bobgui_grid_attach (BOBGUI_GRID (grid), p, pos, line, 1, 1);
        }
      else
        {
          if (pos == 0)
            bobgui_widget_add_css_class (p, "top");
          else if (pos == colors_per_line - 1)
            bobgui_widget_add_css_class (p, "bottom");

          bobgui_grid_attach (BOBGUI_GRID (grid), p, line, pos, 1, 1);
       }
    }

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    cc->max_custom = MAX (cc->max_custom, colors_per_line);
  else
    cc->max_custom = MAX (cc->max_custom, n_colors / colors_per_line);
}

static void
remove_default_palette (BobguiColorChooserWidget *cc)
{
  if (!cc->has_default_palette)
    return;

  remove_palette (cc);
  cc->has_default_palette = FALSE;
  cc->max_custom = 0;
}

static void
add_default_palette (BobguiColorChooserWidget *cc)
{
  GdkRGBA colors[9*5] = {
    GDK_RGBA("99c1f1"), GDK_RGBA("62a0ea"), GDK_RGBA("3584e4"), GDK_RGBA("1c71d8"), GDK_RGBA("1a5fb4"), /* Blue */
    GDK_RGBA("8ff0a4"), GDK_RGBA("57e389"), GDK_RGBA("33d17a"), GDK_RGBA("2ec27e"), GDK_RGBA("26a269"), /* Green */
    GDK_RGBA("f9f06b"), GDK_RGBA("f8e45c"), GDK_RGBA("f6d32d"), GDK_RGBA("f5c211"), GDK_RGBA("e5a50a"), /* Yellow */
    GDK_RGBA("ffbe6f"), GDK_RGBA("ffa348"), GDK_RGBA("ff7800"), GDK_RGBA("e66100"), GDK_RGBA("c64600"), /* Orange */
    GDK_RGBA("f66151"), GDK_RGBA("ed333b"), GDK_RGBA("e01b24"), GDK_RGBA("c01c28"), GDK_RGBA("a51d2d"), /* Red */
    GDK_RGBA("dc8add"), GDK_RGBA("c061cb"), GDK_RGBA("9141ac"), GDK_RGBA("813d9c"), GDK_RGBA("613583"), /* Purple */
    GDK_RGBA("cdab8f"), GDK_RGBA("b5835a"), GDK_RGBA("986a44"), GDK_RGBA("865e3c"), GDK_RGBA("63452c"), /* Brown */
    GDK_RGBA("ffffff"), GDK_RGBA("f6f5f4"), GDK_RGBA("deddda"), GDK_RGBA("c0bfbc"), GDK_RGBA("9a9996"), /* Light */
    GDK_RGBA("77767b"), GDK_RGBA("5e5c64"), GDK_RGBA("3d3846"), GDK_RGBA("241f31"), GDK_RGBA("000000")  /* Dark */
  };
  const char *color_names[] = {
    NC_("Color name", "Very Light Blue"),
    NC_("Color name", "Light Blue"),
    NC_("Color name", "Blue"),
    NC_("Color name", "Dark Blue"),
    NC_("Color name", "Very Dark Blue"),
    NC_("Color name", "Very Light Green"),
    NC_("Color name", "Light Green"),
    NC_("Color name", "Green"),
    NC_("Color name", "Dark Green"),
    NC_("Color name", "Very Dark Green"),
    NC_("Color name", "Very Light Yellow"),
    NC_("Color name", "Light Yellow"),
    NC_("Color name", "Yellow"),
    NC_("Color name", "Dark Yellow"),
    NC_("Color name", "Very Dark Yellow"),
    NC_("Color name", "Very Light Orange"),
    NC_("Color name", "Light Orange"),
    NC_("Color name", "Orange"),
    NC_("Color name", "Dark Orange"),
    NC_("Color name", "Very Dark Orange"),
    NC_("Color name", "Very Light Red"),
    NC_("Color name", "Light Red"),
    NC_("Color name", "Red"),
    NC_("Color name", "Dark Red"),
    NC_("Color name", "Very Dark Red"),
    NC_("Color name", "Very Light Purple"),
    NC_("Color name", "Light Purple"),
    NC_("Color name", "Purple"),
    NC_("Color name", "Dark Purple"),
    NC_("Color name", "Very Dark Purple"),
    NC_("Color name", "Very Light Brown"),
    NC_("Color name", "Light Brown"),
    NC_("Color name", "Brown"),
    NC_("Color name", "Dark Brown"),
    NC_("Color name", "Very Dark Brown"),
    NC_("Color name", "White"),
    NC_("Color name", "Light Gray 1"),
    NC_("Color name", "Light Gray 2"),
    NC_("Color name", "Light Gray 3"),
    NC_("Color name", "Light Gray 4"),
    NC_("Color name", "Dark Gray 1"),
    NC_("Color name", "Dark Gray 2"),
    NC_("Color name", "Dark Gray 3"),
    NC_("Color name", "Dark Gray 4"),
    NC_("Color name", "Black"),
  };

  add_palette (cc, BOBGUI_ORIENTATION_VERTICAL, 5, 9*5, colors, color_names);

  cc->has_default_palette = TRUE;
}

static void
bobgui_color_chooser_widget_activate_color_customize (BobguiWidget  *widget,
                                                   const char *name,
                                                   GVariant   *parameter)
{
  BobguiColorChooserWidget *cc = BOBGUI_COLOR_CHOOSER_WIDGET (widget);
  double red, green, blue, alpha;
  GdkRGBA color;

  g_variant_get (parameter, "(dddd)", &red, &green, &blue, &alpha);
  color.red = red;
  color.green = green;
  color.blue = blue;
  color.alpha = alpha;

  bobgui_color_chooser_set_rgba (BOBGUI_COLOR_CHOOSER (cc->editor), &color);

  bobgui_widget_set_visible (cc->palette, FALSE);
  bobgui_widget_set_visible (cc->editor, TRUE);
  bobgui_widget_grab_focus (cc->editor);
  g_object_notify (G_OBJECT (cc), "show-editor");
}

static void
bobgui_color_chooser_widget_activate_color_select (BobguiWidget  *widget,
                                                const char *name,
                                                GVariant   *parameter)
{
  BobguiColorChooserWidget *cc = BOBGUI_COLOR_CHOOSER_WIDGET (widget);
  GdkRGBA color;
  double red, green, blue, alpha;

  g_variant_get (parameter, "(dddd)", &red, &green, &blue, &alpha);
  color.red = red;
  color.green = green;
  color.blue = blue;
  color.alpha = alpha;

  _bobgui_color_chooser_color_activated (BOBGUI_COLOR_CHOOSER (cc), &color);
}

static void
bobgui_color_chooser_widget_init (BobguiColorChooserWidget *cc)
{
  BobguiWidget *box;
  BobguiWidget *p;
  BobguiWidget *button;
  BobguiWidget *label;
  int i;
  double color[4];
  GdkRGBA rgba;
  GVariant *variant;
  GVariantIter iter;
  gboolean selected;
  char *name;
  char *text;

  cc->use_alpha = TRUE;

  cc->palette = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_widget_set_parent (cc->palette, BOBGUI_WIDGET (cc));

  add_default_palette (cc);

  /* translators: label for the custom section in the color chooser */
  cc->custom_label = label = bobgui_label_new (_("Custom"));
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_box_append (BOBGUI_BOX (cc->palette), label);

  cc->custom = box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 4);
  g_object_set (box, "margin-top", 12, NULL);
  bobgui_box_append (BOBGUI_BOX (cc->palette), box);

  cc->button = button = bobgui_color_swatch_new ();
  bobgui_widget_set_name (button, "add-color-button");
  connect_button_signals (button, cc);
  bobgui_color_swatch_set_icon (BOBGUI_COLOR_SWATCH (button), "list-add-symbolic");
  bobgui_color_swatch_set_selectable (BOBGUI_COLOR_SWATCH (button), FALSE);
  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (button),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL, _("Add Color"),
                                  -1);
  bobgui_box_append (BOBGUI_BOX (box), button);

  cc->settings = g_settings_new ("org.bobgui.bobgui4.Settings.ColorChooser");
  variant = g_settings_get_value (cc->settings, I_("custom-colors"));
  g_variant_iter_init (&iter, variant);
  i = 0;
  p = NULL;
  while (g_variant_iter_loop (&iter, "(dddd)", &color[0], &color[1], &color[2], &color[3]))
    {
      i++;
      p = bobgui_color_swatch_new ();

      rgba.red = color[0];
      rgba.green = color[1];
      rgba.blue = color[2];
      rgba.alpha = color[3];

      bobgui_color_swatch_set_rgba (BOBGUI_COLOR_SWATCH (p), &rgba);

      name = accessible_color_name (&rgba);
      text = g_strdup_printf (_("Custom color %d: %s"), i, name);
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (p),
                                      BOBGUI_ACCESSIBLE_PROPERTY_LABEL, text,
                                      -1);
      g_free (name);
      g_free (text);

      bobgui_color_swatch_set_can_drop (BOBGUI_COLOR_SWATCH (p), TRUE);
      connect_custom_signals (p, cc);
      bobgui_box_append (BOBGUI_BOX (box), p);

      if (i == 8)
        break;
    }
  g_variant_unref (variant);

  cc->editor = bobgui_color_editor_new ();
  bobgui_widget_set_halign (cc->editor, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_hexpand (cc->editor, TRUE);
  g_signal_connect (cc->editor, "notify::rgba",
                    G_CALLBACK (update_from_editor), cc);

  bobgui_widget_set_parent (cc->editor, BOBGUI_WIDGET (cc));

  g_settings_get (cc->settings, I_("selected-color"), "(bdddd)",
                  &selected,
                  &color[0], &color[1], &color[2], &color[3]);
  if (selected)
    {
      rgba.red = color[0];
      rgba.green = color[1];
      rgba.blue = color[2];
      rgba.alpha = color[3];
      bobgui_color_chooser_set_rgba (BOBGUI_COLOR_CHOOSER (cc), &rgba);
    }

  bobgui_widget_set_visible (BOBGUI_WIDGET (cc->editor), FALSE);
}

/* GObject implementation {{{1 */

static void
bobgui_color_chooser_widget_get_property (GObject    *object,
                                       guint       prop_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  BobguiColorChooserWidget *cc = BOBGUI_COLOR_CHOOSER_WIDGET (object);

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
      g_value_set_boolean (value, cc->use_alpha);
      break;
    case PROP_SHOW_EDITOR:
      g_value_set_boolean (value, bobgui_widget_get_visible (cc->editor));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_color_chooser_widget_set_property (GObject      *object,
                                       guint         prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  BobguiColorChooserWidget *cc = BOBGUI_COLOR_CHOOSER_WIDGET (object);

  switch (prop_id)
    {
    case PROP_RGBA:
      bobgui_color_chooser_set_rgba (BOBGUI_COLOR_CHOOSER (cc),
                                  g_value_get_boxed (value));
      break;
    case PROP_USE_ALPHA:
      bobgui_color_chooser_widget_set_use_alpha (cc,
                                              g_value_get_boolean (value));
      break;
    case PROP_SHOW_EDITOR:
      bobgui_color_chooser_widget_set_show_editor (cc,
                                                g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_color_chooser_widget_finalize (GObject *object)
{
  BobguiColorChooserWidget *cc = BOBGUI_COLOR_CHOOSER_WIDGET (object);

  g_object_unref (cc->settings);

  bobgui_widget_unparent (cc->editor);
  bobgui_widget_unparent (cc->palette);

  G_OBJECT_CLASS (bobgui_color_chooser_widget_parent_class)->finalize (object);
}

static void
bobgui_color_chooser_widget_class_init (BobguiColorChooserWidgetClass *class)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->get_property = bobgui_color_chooser_widget_get_property;
  object_class->set_property = bobgui_color_chooser_widget_set_property;
  object_class->finalize = bobgui_color_chooser_widget_finalize;

  widget_class->grab_focus = bobgui_widget_grab_focus_child;
  widget_class->focus = bobgui_widget_focus_child;

  g_object_class_override_property (object_class, PROP_RGBA, "rgba");
  g_object_class_override_property (object_class, PROP_USE_ALPHA, "use-alpha");

  /**
   * BobguiColorChooserWidget:show-editor:
   *
   * %TRUE when the color chooser is showing the single-color editor.
   *
   * It can be set to switch the color chooser into single-color editing mode.
   */
  g_object_class_install_property (object_class, PROP_SHOW_EDITOR,
      g_param_spec_boolean ("show-editor", NULL, NULL,
                            FALSE, BOBGUI_PARAM_READWRITE));

  bobgui_widget_class_set_css_name (widget_class, I_("colorchooser"));
  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BOX_LAYOUT);

  /**
   * BobguiColorChooserWidget|color.select:
   * @red: the red value, between 0 and 1
   * @green: the green value, between 0 and 1
   * @blue: the blue value, between 0 and 1
   * @alpha: the alpha value, between 0 and 1
   *
   * Emits the [signal@Bobgui.ColorChooser::color-activated] signal for
   * the given color.
   */
  bobgui_widget_class_install_action (widget_class, "color.select", "(dddd)",
                                   bobgui_color_chooser_widget_activate_color_select);

  /**
   * BobguiColorChooserWidget|color.customize:
   * @red: the red value, between 0 and 1
   * @green: the green value, between 0 and 1
   * @blue: the blue value, between 0 and 1
   * @alpha: the alpha value, between 0 and 1
   *
   * Activates the color editor for the given color.
   */
  bobgui_widget_class_install_action (widget_class, "color.customize", "(dddd)",
                                   bobgui_color_chooser_widget_activate_color_customize);
}

/* BobguiColorChooser implementation {{{1 */

static void
bobgui_color_chooser_widget_get_rgba (BobguiColorChooser *chooser,
                                   GdkRGBA         *color)
{
  BobguiColorChooserWidget *cc = BOBGUI_COLOR_CHOOSER_WIDGET (chooser);

  if (bobgui_widget_get_visible (cc->editor))
    bobgui_color_chooser_get_rgba (BOBGUI_COLOR_CHOOSER (cc->editor), color);
  else if (cc->current)
    bobgui_color_swatch_get_rgba (cc->current, color);
  else
    {
      color->red = 1.0;
      color->green = 1.0;
      color->blue = 1.0;
      color->alpha = 1.0;
    }

  if (!cc->use_alpha)
    color->alpha = 1.0;
}

static void
add_custom_color (BobguiColorChooserWidget *cc,
                  const GdkRGBA         *color)
{
  BobguiWidget *widget;
  BobguiWidget *p;
  int n;

  n = 0;
  for (widget = bobgui_widget_get_first_child (cc->custom);
       widget != NULL;
       widget = bobgui_widget_get_next_sibling (widget))
    n++;

  while (n >= cc->max_custom)
    {
      BobguiWidget *last = bobgui_widget_get_last_child (cc->custom);

      if (last == (BobguiWidget *)cc->current)
        cc->current = NULL;

      bobgui_box_remove (BOBGUI_BOX (cc->custom), last);
      n--;
    }

  p = bobgui_color_swatch_new ();
  bobgui_color_swatch_set_rgba (BOBGUI_COLOR_SWATCH (p), color);
  bobgui_color_swatch_set_can_drop (BOBGUI_COLOR_SWATCH (p), TRUE);
  connect_custom_signals (p, cc);

  bobgui_box_insert_child_after (BOBGUI_BOX (cc->custom), p, bobgui_widget_get_first_child (cc->custom));

  select_swatch (cc, BOBGUI_COLOR_SWATCH (p));
  save_custom_colors (cc);
}

static void
bobgui_color_chooser_widget_set_rgba (BobguiColorChooser *chooser,
                                   const GdkRGBA   *color)
{
  BobguiColorChooserWidget *cc = BOBGUI_COLOR_CHOOSER_WIDGET (chooser);
  BobguiWidget *swatch;
  BobguiWidget *w;

  GdkRGBA c;

  for (w = bobgui_widget_get_first_child (cc->palette);
       w != NULL;
       w = bobgui_widget_get_next_sibling (w))
    {
      if (!BOBGUI_IS_GRID (w) && !BOBGUI_IS_BOX (w))
        continue;

      for (swatch = bobgui_widget_get_first_child (w);
           swatch != NULL;
           swatch = bobgui_widget_get_next_sibling (swatch))
        {
          bobgui_color_swatch_get_rgba (BOBGUI_COLOR_SWATCH (swatch), &c);
          if (!cc->use_alpha)
            c.alpha = color->alpha;
          if (gdk_rgba_equal (color, &c))
            {
              select_swatch (cc, BOBGUI_COLOR_SWATCH (swatch));
              return;
            }
        }
    }

  add_custom_color (cc, color);
}

static void
bobgui_color_chooser_widget_add_palette (BobguiColorChooser *chooser,
                                      BobguiOrientation   orientation,
                                      int              colors_per_line,
                                      int              n_colors,
                                      GdkRGBA         *colors)
{
  BobguiColorChooserWidget *cc = BOBGUI_COLOR_CHOOSER_WIDGET (chooser);

  remove_default_palette (cc);
  add_palette (cc, orientation, colors_per_line, n_colors, colors, NULL);

  bobgui_box_reorder_child_after (BOBGUI_BOX (cc->palette), cc->custom_label, bobgui_widget_get_last_child (cc->palette));
  bobgui_box_reorder_child_after (BOBGUI_BOX (cc->palette), cc->custom, cc->custom_label);
}

static void
bobgui_color_chooser_widget_iface_init (BobguiColorChooserInterface *iface)
{
  iface->get_rgba = bobgui_color_chooser_widget_get_rgba;
  iface->set_rgba = bobgui_color_chooser_widget_set_rgba;
  iface->add_palette = bobgui_color_chooser_widget_add_palette;
}
 
 /* Public API {{{1 */

/**
 * bobgui_color_chooser_widget_new:
 *
 * Creates a new `BobguiColorChooserWidget`.
 *
 * Returns: a new `BobguiColorChooserWidget`
 */
BobguiWidget *
bobgui_color_chooser_widget_new (void)
{
  return g_object_new (BOBGUI_TYPE_COLOR_CHOOSER_WIDGET, NULL);
}

/* vim:set foldmethod=marker: */
