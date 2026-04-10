/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2007 Red Hat, Inc.
 *
 * Authors:
 * - Bastien Nocera <bnocera@redhat.com>
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

/*
 * Modified by the BOBGUI Team and others 2007.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#include "config.h"

#include "bobguivolumebutton.h"

#include "bobguiadjustment.h"
#include <glib/gi18n-lib.h>
#include "bobguitooltip.h"
#include "bobguiprivate.h"


G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiVolumeButton:
 *
 * `BobguiVolumeButton` is a `BobguiScaleButton` subclass tailored for
 * volume control.
 *
 * <picture>
 *   <source srcset="volumebutton-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiVolumeButton" src="volumebutton.png">
 * </picture>
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */

typedef struct _BobguiVolumeButtonClass  BobguiVolumeButtonClass;

struct _BobguiVolumeButtonClass
{
  BobguiScaleButtonClass parent_class;
};

#define EPSILON (1e-10)

static const char * const icons[] =
{
  "audio-volume-muted",
  "audio-volume-high",
  "audio-volume-low",
  "audio-volume-medium",
  NULL
};

static const char * const icons_symbolic[] =
{
  "audio-volume-muted-symbolic",
  "audio-volume-high-symbolic",
  "audio-volume-low-symbolic",
  "audio-volume-medium-symbolic",
  NULL
};

enum
{
  PROP_0,
  PROP_SYMBOLIC
};

static gboolean cb_query_tooltip (BobguiWidget       *button,
                                  int              x,
                                  int              y,
                                  gboolean         keyboard_mode,
                                  BobguiTooltip      *tooltip,
                                  gpointer         user_data);
static void     cb_value_changed (BobguiVolumeButton *button,
                                  double           value,
                                  gpointer         user_data);

G_DEFINE_TYPE (BobguiVolumeButton, bobgui_volume_button, BOBGUI_TYPE_SCALE_BUTTON)

static gboolean
get_symbolic (BobguiScaleButton *button)
{
  char **icon_list;
  gboolean ret;

  g_object_get (button, "icons", &icon_list, NULL);
  if (icon_list != NULL &&
      icon_list[0] != NULL &&
      g_str_equal (icon_list[0], icons_symbolic[0]))
    ret = TRUE;
  else
    ret = FALSE;
  g_strfreev (icon_list);

  return ret;
}

static void
bobgui_volume_button_set_property (GObject       *object,
                                guint          prop_id,
                                const GValue  *value,
                                GParamSpec    *pspec)
{
  BobguiScaleButton *button = BOBGUI_SCALE_BUTTON (object);

  switch (prop_id)
    {
    case PROP_SYMBOLIC:
      if (get_symbolic (button) != g_value_get_boolean (value))
        {
          if (g_value_get_boolean (value))
            bobgui_scale_button_set_icons (button, (const char **) icons_symbolic);
          else
            bobgui_scale_button_set_icons (button, (const char **) icons);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_volume_button_get_property (GObject     *object,
                                guint        prop_id,
                                GValue      *value,
                                GParamSpec  *pspec)
{
  switch (prop_id)
    {
    case PROP_SYMBOLIC:
      g_value_set_boolean (value, get_symbolic (BOBGUI_SCALE_BUTTON (object)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_volume_button_class_init (BobguiVolumeButtonClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  gobject_class->set_property = bobgui_volume_button_set_property;
  gobject_class->get_property = bobgui_volume_button_get_property;

  /**
   * BobguiVolumeButton:use-symbolic:
   *
   * Whether to use symbolic icons as the icons.
   *
   * Note that if the symbolic icons are not available in your installed
   * theme, then the normal (potentially colorful) icons will be used.
   *
   * Deprecated: 4.10: This widget will be removed in BOBGUI 5
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SYMBOLIC,
                                   g_param_spec_boolean ("use-symbolic", NULL, NULL,
                                                         TRUE,
                                                         BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY));

  /* Bind class to template
   */
  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/ui/bobguivolumebutton.ui");
  bobgui_widget_class_bind_template_callback (widget_class, cb_query_tooltip);
  bobgui_widget_class_bind_template_callback (widget_class, cb_value_changed);
}

static void
bobgui_volume_button_init (BobguiVolumeButton *button)
{
  BobguiWidget *widget = BOBGUI_WIDGET (button);

  bobgui_widget_init_template (widget);
}

/**
 * bobgui_volume_button_new:
 *
 * Creates a `BobguiVolumeButton`.
 *
 * The button has a range between 0.0 and 1.0, with a stepping of 0.02.
 * Volume values can be obtained and modified using the functions from
 * [class@Bobgui.ScaleButton].
 *
 * Returns: a new `BobguiVolumeButton`
 *
 * Deprecated: 4.10: This widget will be removed in BOBGUI 5
 */
BobguiWidget *
bobgui_volume_button_new (void)
{
  GObject *button;
  button = g_object_new (BOBGUI_TYPE_VOLUME_BUTTON, NULL);
  return BOBGUI_WIDGET (button);
}

static gboolean
cb_query_tooltip (BobguiWidget  *button,
                  int         x,
                  int         y,
                  gboolean    keyboard_mode,
                  BobguiTooltip *tooltip,
                  gpointer    user_data)
{
  BobguiScaleButton *scale_button = BOBGUI_SCALE_BUTTON (button);
  BobguiAdjustment *adjustment;
  double val;
  char *str;

  adjustment = bobgui_scale_button_get_adjustment (scale_button);
  val = bobgui_scale_button_get_value (scale_button);

  if (val < (bobgui_adjustment_get_lower (adjustment) + EPSILON))
    {
      str = g_strdup (_("Muted"));
    }
  else if (val >= (bobgui_adjustment_get_upper (adjustment) - EPSILON))
    {
      str = g_strdup (_("Full Volume"));
    }
  else
    {
      int percent;

      percent = (int) (100. * val / (bobgui_adjustment_get_upper (adjustment) - bobgui_adjustment_get_lower (adjustment)) + .5);

      /* Translators: this is the percentage of the current volume,
       * as used in the tooltip, eg. "49 %".
       * Translate the "%d" to "%Id" if you want to use localised digits,
       * or otherwise translate the "%d" to "%d".
       */
      str = g_strdup_printf (C_("volume percentage", "%d %%"), percent);
    }

  bobgui_tooltip_set_text (tooltip, str);
  g_free (str);

  return TRUE;
}

static void
cb_value_changed (BobguiVolumeButton *button, double value, gpointer user_data)
{
  bobgui_widget_trigger_tooltip_query (BOBGUI_WIDGET (button));
}
