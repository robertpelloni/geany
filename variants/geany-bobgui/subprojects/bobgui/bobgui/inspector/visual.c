/*
 * Copyright (c) 2014 Red Hat, Inc.
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
#include <glib/gi18n-lib.h>

#include "visual.h"

#include "fpsoverlay.h"
#include "a11yoverlay.h"
#include "updatesoverlay.h"
#include "layoutoverlay.h"
#include "focusoverlay.h"
#include "baselineoverlay.h"
#include "subsurfaceoverlay.h"
#include "window.h"

#include "bobguiadjustment.h"
#include "bobguibox.h"
#include "bobguibutton.h"
#include "bobguidropdown.h"
#include "bobguicssproviderprivate.h"
#include "bobguidebug.h"
#include "bobguiprivate.h"
#include "bobguisettingsprivate.h"
#include "bobguiswitch.h"
#include "bobguiscale.h"
#include "bobguiwindow.h"
#include "bobguilistbox.h"
#include "gskdebugprivate.h"
#include "gskrendererprivate.h"
#include "bobguinative.h"
#include "bobguibinlayout.h"
#include "bobguieditable.h"
#include "bobguientry.h"
#include "bobguistringlist.h"
#include "bobguilabel.h"
#include "bobguitypebuiltins.h"

#ifdef GDK_WINDOWING_X11
#include "x11/gdkx.h"
#endif
#ifdef GDK_WINDOWING_WAYLAND
#include "wayland/gdkwayland.h"
#include "wayland/gdkdisplay-wayland.h"
#endif
#ifdef GDK_WINDOWING_MACOS
#include "macos/gdkmacos.h"
#endif
#ifdef GDK_WINDOWING_BROADWAY
#include "broadway/gdkbroadway.h"
#endif

#include "gdk/gdkdebugprivate.h"

#define EPSILON               1e-10

struct _BobguiInspectorVisual
{
  BobguiWidget widget;

  BobguiWidget *swin;
  BobguiWidget *box;

  BobguiWidget *visual_box;
  BobguiWidget *theme_combo;
  BobguiWidget *color_scheme_combo;
  BobguiWidget *contrast_combo;
  BobguiWidget *motion_combo;
  BobguiWidget *icon_combo;
  BobguiWidget *cursor_combo;
  BobguiWidget *cursor_size_spin;
  BobguiWidget *direction_combo;
  BobguiWidget *font_button;
  BobguiWidget *font_rendering_combo;
  BobguiWidget *animation_switch;
  BobguiWidget *font_scale_entry;
  BobguiAdjustment *font_scale_adjustment;
  BobguiAdjustment *slowdown_adjustment;
  BobguiWidget *slowdown_entry;
  BobguiAdjustment *cursor_size_adjustment;

  BobguiWidget *debug_box;
  BobguiWidget *fps_switch;
  BobguiWidget *updates_switch;
  BobguiWidget *cairo_switch;
  BobguiWidget *baselines_switch;
  BobguiWidget *layout_switch;
  BobguiWidget *focus_switch;
  BobguiWidget *a11y_switch;
  BobguiWidget *subsurface_switch;

  BobguiWidget *misc_box;
  BobguiWidget *touchscreen_switch;

  BobguiInspectorOverlay *fps_overlay;
  BobguiInspectorOverlay *updates_overlay;
  BobguiInspectorOverlay *layout_overlay;
  BobguiInspectorOverlay *focus_overlay;
  BobguiInspectorOverlay *baseline_overlay;
  BobguiInspectorOverlay *a11y_overlay;
  BobguiInspectorOverlay *subsurface_overlay;

  GdkDisplay *display;
  BobguiSettings *settings;
};

typedef struct _BobguiInspectorVisualClass
{
  BobguiWidgetClass parent_class;
} BobguiInspectorVisualClass;

G_DEFINE_TYPE (BobguiInspectorVisual, bobgui_inspector_visual, BOBGUI_TYPE_WIDGET)

static void
fix_direction_recurse (BobguiWidget        *widget,
                       BobguiTextDirection  dir)
{
  BobguiWidget *child;

  g_object_ref (widget);

  bobgui_widget_set_direction (widget, dir);
  for (child = bobgui_widget_get_first_child (widget);
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
     {
        fix_direction_recurse (child, dir);
     }

  g_object_unref (widget);
}

static BobguiTextDirection initial_direction;

static void
fix_direction (BobguiWidget *iw)
{
  fix_direction_recurse (iw, initial_direction);
}

static void
direction_changed (BobguiDropDown *combo)
{
  BobguiWidget *iw;
  guint selected;

  iw = BOBGUI_WIDGET (bobgui_widget_get_root (BOBGUI_WIDGET (combo)));
  if (iw)
    fix_direction (iw);

  selected = bobgui_drop_down_get_selected (combo);

  if (selected == 0)
    bobgui_widget_set_default_direction (BOBGUI_TEXT_DIR_LTR);
  else
    bobgui_widget_set_default_direction (BOBGUI_TEXT_DIR_RTL);
}

static void
init_direction (BobguiInspectorVisual *vis)
{
  initial_direction = bobgui_widget_get_default_direction ();
  if (initial_direction == BOBGUI_TEXT_DIR_LTR)
    bobgui_drop_down_set_selected (BOBGUI_DROP_DOWN (vis->direction_combo), 0);
  else
    bobgui_drop_down_set_selected (BOBGUI_DROP_DOWN (vis->direction_combo), 1);
}

static void
redraw_everything (void)
{
  GList *toplevels;
  toplevels = bobgui_window_list_toplevels ();
  g_list_foreach (toplevels, (GFunc) bobgui_widget_queue_draw, NULL);
  g_list_free (toplevels);
}

static double
get_dpi_ratio (BobguiInspectorVisual *vis)
{
#ifdef GDK_WINDOWING_MACOS
  if (GDK_IS_MACOS_DISPLAY (vis->display))
    return 72.0 * 1024.0;
#endif

  return 96.0 * 1024.0;
}

static double
get_font_scale (BobguiInspectorVisual *vis)
{
  int dpi_int;

  g_object_get (vis->settings, "bobgui-xft-dpi", &dpi_int, NULL);

  return dpi_int / get_dpi_ratio (vis);
}

static void
update_font_scale (BobguiInspectorVisual *vis,
                   double              factor,
                   gboolean            update_adjustment,
                   gboolean            update_entry,
                   gboolean            write_back)
{
  if (write_back)
    g_object_set (vis->settings, "bobgui-xft-dpi", (int)(factor * get_dpi_ratio (vis)), NULL);

  if (update_adjustment)
    bobgui_adjustment_set_value (vis->font_scale_adjustment, factor);

  if (update_entry)
    {
      char *str = g_strdup_printf ("%0.2f", factor);

      bobgui_editable_set_text (BOBGUI_EDITABLE (vis->font_scale_entry), str);
      g_free (str);
    }
}

static void
font_scale_adjustment_changed (BobguiAdjustment      *adjustment,
                               BobguiInspectorVisual *vis)
{
  double factor;

  factor = bobgui_adjustment_get_value (adjustment);
  update_font_scale (vis, factor, FALSE, TRUE, TRUE);
}

static BobguiFontRendering
get_font_rendering (BobguiInspectorVisual *vis)
{
  BobguiFontRendering font_rendering;

  g_object_get (vis->settings, "bobgui-font-rendering", &font_rendering, NULL);

  return font_rendering;
}

static void
update_font_rendering (BobguiInspectorVisual *vis,
                       BobguiFontRendering    font_rendering)
{
  if (get_font_rendering (vis) == font_rendering)
    return;

  g_object_set (vis->settings, "bobgui-font-rendering", font_rendering, NULL);
}

static void
font_scale_entry_activated (BobguiEntry           *entry,
                            BobguiInspectorVisual *vis)
{
  double factor;
  char *err = NULL;

  factor = g_strtod (bobgui_editable_get_text (BOBGUI_EDITABLE (entry)), &err);
  if (err != NULL)
    update_font_scale (vis, factor, TRUE, FALSE, TRUE);
}

static void
fps_activate (BobguiSwitch          *sw,
              GParamSpec         *pspec,
              BobguiInspectorVisual *vis)
{
  BobguiInspectorWindow *iw;
  gboolean fps;

  fps = bobgui_switch_get_active (sw);
  iw = BOBGUI_INSPECTOR_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (vis)));
  if (iw == NULL)
    return;

  if (fps)
    {
      if (vis->fps_overlay == NULL)
        {
          vis->fps_overlay = bobgui_fps_overlay_new ();
          bobgui_inspector_window_add_overlay (iw, vis->fps_overlay);
          g_object_unref (vis->fps_overlay);
        }
    }
  else
    {
      if (vis->fps_overlay != NULL)
        {
          bobgui_inspector_window_remove_overlay (iw, vis->fps_overlay);
          vis->fps_overlay = NULL;
        }
    }

  redraw_everything ();
}

static void
a11y_activate (BobguiSwitch          *sw,
               GParamSpec         *pspec,
               BobguiInspectorVisual *vis)
{
  BobguiInspectorWindow *iw;
  gboolean a11y;

  a11y = bobgui_switch_get_active (sw);
  iw = BOBGUI_INSPECTOR_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (vis)));
  if (iw == NULL)
    return;

  if (a11y)
    {
      if (vis->a11y_overlay == NULL)
        {
          vis->a11y_overlay = bobgui_a11y_overlay_new ();
          bobgui_inspector_window_add_overlay (iw, vis->a11y_overlay);
          g_object_unref (vis->a11y_overlay);
        }
    }
  else
    {
      if (vis->a11y_overlay != NULL)
        {
          bobgui_inspector_window_remove_overlay (iw, vis->a11y_overlay);
          vis->a11y_overlay = NULL;
        }
    }

  redraw_everything ();
}

static void
subsurface_activate (BobguiSwitch          *sw,
                     GParamSpec         *pspec,
                     BobguiInspectorVisual *vis)
{
  BobguiInspectorWindow *iw;
  gboolean subsurface;

  subsurface = bobgui_switch_get_active (sw);
  iw = BOBGUI_INSPECTOR_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (vis)));
  if (iw == NULL)
    return;

  if (subsurface)
    {
      if (vis->subsurface_overlay == NULL)
        {
          vis->subsurface_overlay = bobgui_subsurface_overlay_new ();
          bobgui_inspector_window_add_overlay (iw, vis->subsurface_overlay);
          g_object_unref (vis->subsurface_overlay);
        }
    }
  else
    {
      if (vis->subsurface_overlay != NULL)
        {
          bobgui_inspector_window_remove_overlay (iw, vis->subsurface_overlay);
          vis->subsurface_overlay = NULL;
        }
    }

  redraw_everything ();
}

static void
updates_activate (BobguiSwitch          *sw,
                  GParamSpec         *pspec,
                  BobguiInspectorVisual *vis)
{
  BobguiInspectorWindow *iw;
  gboolean updates;

  updates = bobgui_switch_get_active (sw);
  iw = BOBGUI_INSPECTOR_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (vis)));
  if (iw == NULL)
    return;

  if (updates)
    {
      if (vis->updates_overlay == NULL)
        {
          vis->updates_overlay = bobgui_updates_overlay_new ();
          bobgui_inspector_window_add_overlay (iw, vis->updates_overlay);
          g_object_unref (vis->updates_overlay);
        }
    }
  else
    {
      if (vis->updates_overlay != NULL)
        {
          bobgui_inspector_window_remove_overlay (iw, vis->updates_overlay);
          vis->updates_overlay = NULL;
        }
    }

  redraw_everything ();
}

static void
cairo_activate (BobguiSwitch          *sw,
                GParamSpec         *pspec,
                BobguiInspectorVisual *vis)
{
  BobguiInspectorWindow *iw;
  gboolean active;
  guint flags;
  GList *toplevels, *l;

  active = bobgui_switch_get_active (sw);
  iw = BOBGUI_INSPECTOR_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (vis)));
  if (iw == NULL)
    return;

  flags = gsk_get_debug_flags ();
  if (active)
    flags = flags | GSK_DEBUG_CAIRO;
  else
    flags = flags & ~GSK_DEBUG_CAIRO;
  gsk_set_debug_flags (flags);

  toplevels = bobgui_window_list_toplevels ();
  for (l = toplevels; l; l = l->next)
    {
      BobguiWidget *toplevel = l->data;
      GskRenderer *renderer;

      if ((BobguiRoot *)toplevel == bobgui_widget_get_root (BOBGUI_WIDGET (sw))) /* skip the inspector */
        continue;

      renderer = bobgui_native_get_renderer (BOBGUI_NATIVE (toplevel));
      if (!renderer)
        continue;

      gsk_renderer_set_debug_flags (renderer, flags);
    }
  g_list_free (toplevels);

  redraw_everything ();
}

static void
baselines_activate (BobguiSwitch          *sw,
                    GParamSpec         *pspec,
                    BobguiInspectorVisual *vis)
{
  BobguiInspectorWindow *iw;
  gboolean baselines;

  baselines = bobgui_switch_get_active (sw);
  iw = BOBGUI_INSPECTOR_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (vis)));
  if (iw == NULL)
    return;

  if (baselines)
    {
      if (vis->baseline_overlay == NULL)
        {
          vis->baseline_overlay = bobgui_baseline_overlay_new ();
          bobgui_inspector_window_add_overlay (iw, vis->baseline_overlay);
          g_object_unref (vis->baseline_overlay);
        }
    }
  else
    {
      if (vis->baseline_overlay != NULL)
        {
          bobgui_inspector_window_remove_overlay (iw, vis->baseline_overlay);
          vis->baseline_overlay = NULL;
        }
    }

  redraw_everything ();
}

static void
layout_activate (BobguiSwitch          *sw,
                 GParamSpec         *pspec,
                 BobguiInspectorVisual *vis)
{
  BobguiInspectorWindow *iw;
  gboolean draw_layout;

  draw_layout = bobgui_switch_get_active (sw);
  iw = BOBGUI_INSPECTOR_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (vis)));
  if (iw == NULL)
    return;

  if (draw_layout)
    {
      if (vis->layout_overlay == NULL)
        {
          vis->layout_overlay = bobgui_layout_overlay_new ();
          bobgui_inspector_window_add_overlay (iw, vis->layout_overlay);
          g_object_unref (vis->layout_overlay);
        }
    }
  else
    {
      if (vis->layout_overlay != NULL)
        {
          bobgui_inspector_window_remove_overlay (iw, vis->layout_overlay);
          vis->layout_overlay = NULL;
        }
    }

  redraw_everything ();
}

static void
focus_activate (BobguiSwitch          *sw,
                GParamSpec         *pspec,
                BobguiInspectorVisual *vis)
{
  BobguiInspectorWindow *iw;
  gboolean focus;

  focus = bobgui_switch_get_active (sw);
  iw = BOBGUI_INSPECTOR_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (vis)));
  if (iw == NULL)
    return;

  if (focus)
    {
      if (vis->focus_overlay == NULL)
        {
          vis->focus_overlay = bobgui_focus_overlay_new ();
          bobgui_inspector_window_add_overlay (iw, vis->focus_overlay);
          g_object_unref (vis->focus_overlay);
        }
    }
  else
    {
      if (vis->focus_overlay != NULL)
        {
          bobgui_inspector_window_remove_overlay (iw, vis->focus_overlay);
          vis->focus_overlay = NULL;
        }
    }

  redraw_everything ();
}

static void
fill_bobgui (const char *path,
          GHashTable  *t)
{
  const char *dir_entry;
  GDir *dir = g_dir_open (path, 0, NULL);

  if (!dir)
    return;

  while ((dir_entry = g_dir_read_name (dir)))
    {
      char *filename = g_build_filename (path, dir_entry, "bobgui-4.0", "bobgui.css", NULL);

      if (g_file_test (filename, G_FILE_TEST_IS_REGULAR) &&
          !g_hash_table_contains (t, dir_entry))
        g_hash_table_add (t, g_strdup (dir_entry));

      g_free (filename);
    }

  g_dir_close (dir);
}

static char *
get_data_path (const char *subdir)
{
  char *base_datadir, *full_datadir;
#if defined (GDK_WINDOWING_WIN32) || defined (GDK_WINDOWING_MACOS)
  base_datadir = g_strdup (_bobgui_get_datadir ());
#else
  base_datadir = g_strdup (BOBGUI_DATADIR);
#endif
  full_datadir = g_build_filename (base_datadir, subdir, NULL);
  g_free (base_datadir);
  return full_datadir;
}

static gboolean
theme_to_pos (GBinding *binding,
              const GValue *from,
              GValue *to,
              gpointer user_data)
{
  BobguiStringList *names = user_data;
  const char *theme = g_value_get_string (from);
  guint i, n;

  for (i = 0, n = g_list_model_get_n_items (G_LIST_MODEL (names)); i < n; i++)
    {
      const char *name = bobgui_string_list_get_string (names, i);
      if (g_strcmp0 (name, theme) == 0)
        {
          g_value_set_uint (to, i);
          return TRUE;
        }
    }
  return FALSE;
}

static gboolean
pos_to_theme (GBinding *binding,
              const GValue *from,
              GValue *to,
              gpointer user_data)
{
  BobguiStringList *names = user_data;
  int pos = g_value_get_uint (from);
  g_value_set_string (to, bobgui_string_list_get_string (names, pos));
  return TRUE;
}

static void
init_theme (BobguiInspectorVisual *vis)
{
  GHashTable *t;
  GHashTableIter iter;
  char *theme, *path;
  char **builtin_themes;
  GList *list, *l;
  BobguiStringList *names;
  guint i;
  const char * const *dirs;

  t = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  /* Builtin themes */
  builtin_themes = g_resources_enumerate_children ("/org/bobgui/libbobgui/theme", 0, NULL);
  for (i = 0; builtin_themes[i] != NULL; i++)
    {
      if (g_str_has_suffix (builtin_themes[i], "/"))
        g_hash_table_add (t, g_strndup (builtin_themes[i], strlen (builtin_themes[i]) - 1));
    }
  g_strfreev (builtin_themes);

  path = _bobgui_get_theme_dir ();
  fill_bobgui (path, t);
  g_free (path);

  path = g_build_filename (g_get_user_data_dir (), "themes", NULL);
  fill_bobgui (path, t);
  g_free (path);

  path = g_build_filename (g_get_home_dir (), ".themes", NULL);
  fill_bobgui (path, t);
  g_free (path);

  dirs = g_get_system_data_dirs ();
  for (i = 0; dirs[i]; i++)
    {
      path = g_build_filename (dirs[i], "themes", NULL);
      fill_bobgui (path, t);
      g_free (path);
    }

  list = NULL;
  g_hash_table_iter_init (&iter, t);
  while (g_hash_table_iter_next (&iter, (gpointer *)&theme, NULL))
    list = g_list_insert_sorted (list, theme, (GCompareFunc)strcmp);

  names = bobgui_string_list_new (NULL);
  for (l = list, i = 0; l; l = l->next, i++)
    bobgui_string_list_append (names, (const char *)l->data);

  g_list_free (list);
  g_hash_table_destroy (t);

  bobgui_drop_down_set_model (BOBGUI_DROP_DOWN (vis->theme_combo), G_LIST_MODEL (names));

  g_object_bind_property_full (vis->settings, "bobgui-theme-name",
                               vis->theme_combo, "selected",
                               G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE,
                               theme_to_pos, pos_to_theme, names, (GDestroyNotify)g_object_unref);

  if (g_getenv ("BOBGUI_THEME") != NULL)
    {
      BobguiWidget *row;

      /* theme is hardcoded, nothing we can do */
      row = bobgui_widget_get_parent (vis->theme_combo);
      bobgui_widget_unparent (vis->theme_combo);
      vis->theme_combo = bobgui_label_new ("Set via BOBGUI_THEME");
      bobgui_widget_add_css_class (vis->theme_combo, "dim-label");
      bobgui_box_append (BOBGUI_BOX (row), vis->theme_combo);
    }
}

static void
init_colorscheme (BobguiInspectorVisual *vis)
{
  g_object_bind_property (vis->settings, "bobgui-interface-color-scheme",
                          vis->color_scheme_combo, "selected",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
}

static void
init_contrast (BobguiInspectorVisual *vis)
{
  g_object_bind_property (vis->settings, "bobgui-interface-contrast",
                          vis->contrast_combo, "selected",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
}

static void
init_reduced_motion (BobguiInspectorVisual *vis)
{
  g_object_bind_property (vis->settings, "bobgui-interface-reduced-motion",
                          vis->motion_combo, "selected",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
}

static void
fill_icons (const char *path,
            GHashTable  *t)
{
  const char *dir_entry;
  GDir *dir;

  dir = g_dir_open (path, 0, NULL);
  if (!dir)
    return;

  while ((dir_entry = g_dir_read_name (dir)))
    {
      char *filename = g_build_filename (path, dir_entry, "index.theme", NULL);

      if (g_file_test (filename, G_FILE_TEST_IS_REGULAR) &&
          g_strcmp0 (dir_entry, "hicolor") != 0 &&
          !g_hash_table_contains (t, dir_entry))
        g_hash_table_add (t, g_strdup (dir_entry));

      g_free (filename);
    }

  g_dir_close (dir);
}

static void
init_icons (BobguiInspectorVisual *vis)
{
  GHashTable *t;
  GHashTableIter iter;
  char *theme, *path;
  GList *list, *l;
  int i;
  BobguiStringList *names;
  const char * const *dirs;

  t = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  path = get_data_path ("icons");
  fill_icons (path, t);
  g_free (path);

  path = g_build_filename (g_get_user_data_dir (), "icons", NULL);
  fill_icons (path, t);
  g_free (path);

  dirs = g_get_system_data_dirs ();
  for (i = 0; dirs[i]; i++)
    {
      path = g_build_filename (dirs[i], "icons", NULL);
      fill_icons (path, t);
      g_free (path);
    }

  list = NULL;
  g_hash_table_iter_init (&iter, t);
  while (g_hash_table_iter_next (&iter, (gpointer *)&theme, NULL))
    list = g_list_insert_sorted (list, theme, (GCompareFunc)strcmp);

  names = bobgui_string_list_new (NULL);
  for (l = list, i = 0; l; l = l->next, i++)
    bobgui_string_list_append (names, (const char *)l->data);

  g_hash_table_destroy (t);
  g_list_free (list);

  bobgui_drop_down_set_model (BOBGUI_DROP_DOWN (vis->icon_combo), G_LIST_MODEL (names));

  g_object_bind_property_full (vis->settings, "bobgui-icon-theme-name",
                               vis->icon_combo, "selected",
                               G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE,
                               theme_to_pos, pos_to_theme, names, (GDestroyNotify)g_object_unref);
}

static void
fill_cursors (const char *path,
              GHashTable  *t)
{
  const char *dir_entry;
  GDir *dir;

  dir = g_dir_open (path, 0, NULL);
  if (!dir)
    return;

  while ((dir_entry = g_dir_read_name (dir)))
    {
      char *filename = g_build_filename (path, dir_entry, "cursors", NULL);

      if (g_file_test (filename, G_FILE_TEST_IS_DIR) &&
          !g_hash_table_contains (t, dir_entry))
        g_hash_table_add (t, g_strdup (dir_entry));

      g_free (filename);
    }

  g_dir_close (dir);
}

static void
init_cursors (BobguiInspectorVisual *vis)
{
  GHashTable *t;
  GHashTableIter iter;
  char *theme, *path;
  GList *list, *l;
  BobguiStringList *names;
  int i;
  const char * const *dirs;

  t = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

  path = get_data_path ("icons");
  fill_cursors (path, t);
  g_free (path);

  path = g_build_filename (g_get_user_data_dir (), "icons", NULL);
  fill_cursors (path, t);
  g_free (path);

  dirs = g_get_system_data_dirs ();
  for (i = 0; dirs[i]; i++)
    {
      path = g_build_filename (dirs[i], "icons", NULL);
      fill_cursors (path, t);
      g_free (path);
    }

  list = NULL;
  g_hash_table_iter_init (&iter, t);
  while (g_hash_table_iter_next (&iter, (gpointer *)&theme, NULL))
    list = g_list_insert_sorted (list, theme, (GCompareFunc)strcmp);

  names = bobgui_string_list_new (NULL);
  for (l = list, i = 0; l; l = l->next, i++)
    bobgui_string_list_append (names, (const char *)l->data);

  g_hash_table_destroy (t);
  g_list_free (list);

  bobgui_drop_down_set_model (BOBGUI_DROP_DOWN (vis->cursor_combo), G_LIST_MODEL (names));

  g_object_bind_property_full (vis->settings, "bobgui-cursor-theme-name",
                               vis->cursor_combo, "selected",
                               G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE,
                               theme_to_pos, pos_to_theme, names, (GDestroyNotify)g_object_unref);

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (vis->display) &&
      GDK_WAYLAND_DISPLAY (vis->display)->cursor_shape != NULL)
    {
      BobguiWidget *row;

      row = bobgui_widget_get_parent (vis->cursor_combo);
      bobgui_widget_unparent (vis->cursor_combo);
      vis->cursor_combo = bobgui_label_new ("Set by Compositor");
      bobgui_widget_add_css_class (vis->cursor_combo, "dim-label");
      bobgui_box_append (BOBGUI_BOX (row), vis->cursor_combo);
    }
#endif
}

static void
cursor_size_changed (BobguiAdjustment *adjustment, BobguiInspectorVisual *vis)
{
  int size;

  size = bobgui_adjustment_get_value (adjustment);
  g_object_set (vis->settings, "bobgui-cursor-theme-size", size, NULL);
}

static void
init_cursor_size (BobguiInspectorVisual *vis)
{
  int size;

  g_object_get (vis->settings, "bobgui-cursor-theme-size", &size, NULL);
  if (size == 0)
    size = 32;

  bobgui_adjustment_set_value (vis->cursor_size_adjustment, (double)size);
  g_signal_connect (vis->cursor_size_adjustment, "value-changed",
                    G_CALLBACK (cursor_size_changed), vis);

#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (vis->display) &&
      GDK_WAYLAND_DISPLAY (vis->display)->cursor_shape != NULL)
    {
      BobguiWidget *row;

      row = bobgui_widget_get_parent (vis->cursor_size_spin);
      bobgui_widget_unparent (vis->cursor_size_spin);
      vis->cursor_size_spin = bobgui_label_new ("Set by Compositor");
      bobgui_widget_add_css_class (vis->cursor_size_spin, "dim-label");
      bobgui_box_append (BOBGUI_BOX (row), vis->cursor_size_spin);
    }
#endif
}

static gboolean
name_to_desc (GBinding     *binding,
              const GValue *from_value,
              GValue       *to_value,
              gpointer      user_data)
{
  const char *name;
  PangoFontDescription *desc;

  name = g_value_get_string (from_value);
  desc = pango_font_description_from_string (name);

  g_value_take_boxed (to_value, desc);

  return TRUE;
}

static gboolean
name_from_desc (GBinding     *binding,
                const GValue *from_value,
                GValue       *to_value,
                gpointer      user_data)
{
  const char *name;
  PangoFontDescription *desc;

  desc = g_value_get_boxed (from_value);
  name = pango_font_description_get_family (desc);

  g_value_set_string (to_value, name);

  return TRUE;
}

static void
init_font (BobguiInspectorVisual *vis)
{
  g_object_bind_property_full (vis->settings, "bobgui-font-name",
                               vis->font_button, "font-desc",
                               G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE,
                               name_to_desc,
                               name_from_desc,
                               NULL, NULL);
}

static void
init_font_scale (BobguiInspectorVisual *vis)
{
  double scale;

  scale = get_font_scale (vis);
  update_font_scale (vis, scale, TRUE, TRUE, FALSE);
  g_signal_connect (vis->font_scale_adjustment, "value-changed",
                    G_CALLBACK (font_scale_adjustment_changed), vis);
  g_signal_connect (vis->font_scale_entry, "activate",
                    G_CALLBACK (font_scale_entry_activated), vis);
}

static void
font_rendering_changed (BobguiDropDown        *combo,
                        GParamSpec         *pspec,
                        BobguiInspectorVisual *vis)
{
  update_font_rendering (vis, bobgui_drop_down_get_selected (combo));
}

static void
init_font_rendering (BobguiInspectorVisual *vis)
{
  bobgui_drop_down_set_selected (BOBGUI_DROP_DOWN (vis->font_rendering_combo), get_font_rendering (vis));
}

static void
init_animation (BobguiInspectorVisual *vis)
{
  g_object_bind_property (vis->settings, "bobgui-enable-animations",
                          vis->animation_switch, "active",
                          G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);
}

static void
update_slowdown (BobguiInspectorVisual *vis,
                 double slowdown,
                 gboolean update_adjustment,
                 gboolean update_entry)
{
  _bobgui_set_slowdown (slowdown);

  if (update_adjustment)
    bobgui_adjustment_set_value (vis->slowdown_adjustment,
                              log2 (slowdown));

  if (update_entry)
    {
      char *str = g_strdup_printf ("%0.*f", 2, slowdown);

      bobgui_editable_set_text (BOBGUI_EDITABLE (vis->slowdown_entry), str);
      g_free (str);
    }
}

static void
slowdown_adjustment_changed (BobguiAdjustment *adjustment,
                             BobguiInspectorVisual *vis)
{
  double value = bobgui_adjustment_get_value (adjustment);
  double previous = CLAMP (log2 (_bobgui_get_slowdown ()),
                            bobgui_adjustment_get_lower (adjustment),
                            bobgui_adjustment_get_upper (adjustment));

  if (fabs (value - previous) > EPSILON)
    update_slowdown (vis, exp2 (value), FALSE, TRUE);
}

static void
slowdown_entry_activated (BobguiEntry *entry,
                          BobguiInspectorVisual *vis)
{
  double slowdown;
  char *err = NULL;

  slowdown = g_strtod (bobgui_editable_get_text (BOBGUI_EDITABLE (entry)), &err);
  if (err != NULL)
    update_slowdown (vis, slowdown, TRUE, FALSE);
}

static void
init_slowdown (BobguiInspectorVisual *vis)
{
  update_slowdown (vis, _bobgui_get_slowdown (), TRUE, TRUE);
  g_signal_connect (vis->slowdown_adjustment, "value-changed",
                    G_CALLBACK (slowdown_adjustment_changed), vis);
  g_signal_connect (vis->slowdown_entry, "activate",
                    G_CALLBACK (slowdown_entry_activated), vis);
}

static void
update_touchscreen (BobguiSwitch *sw)
{
  BobguiDebugFlags flags;

  flags = bobgui_get_debug_flags ();

  if (bobgui_switch_get_active (sw))
    flags |= BOBGUI_DEBUG_TOUCHSCREEN;
  else
    flags &= ~BOBGUI_DEBUG_TOUCHSCREEN;

  bobgui_set_debug_flags (flags);
}

static void
init_touchscreen (BobguiInspectorVisual *vis)
{
  bobgui_switch_set_active (BOBGUI_SWITCH (vis->touchscreen_switch), (bobgui_get_debug_flags () & BOBGUI_DEBUG_TOUCHSCREEN) != 0);
  g_signal_connect (vis->touchscreen_switch, "notify::active",
                    G_CALLBACK (update_touchscreen), NULL);
}

static gboolean
keynav_failed (BobguiWidget *widget, BobguiDirectionType direction, BobguiInspectorVisual *vis)
{
  BobguiWidget *next;

  if (direction == BOBGUI_DIR_DOWN &&
      widget == vis->visual_box)
    next = vis->debug_box;
  else if (direction == BOBGUI_DIR_DOWN &&
      widget == vis->debug_box)
    next = vis->misc_box;
  else if (direction == BOBGUI_DIR_UP &&
           widget == vis->debug_box)
    next = vis->visual_box;
  else if (direction == BOBGUI_DIR_UP &&
           widget == vis->misc_box)
    next = vis->debug_box;
  else
    next = NULL;

  if (next)
    {
      bobgui_widget_child_focus (next, direction);
      return TRUE;
    }

  return FALSE;
}

static void
row_activated (BobguiListBox         *box,
               BobguiListBoxRow      *row,
               BobguiInspectorVisual *vis)
{
  if (bobgui_widget_is_ancestor (vis->animation_switch, BOBGUI_WIDGET (row)))
    {
      BobguiSwitch *sw = BOBGUI_SWITCH (vis->animation_switch);
      bobgui_switch_set_active (sw, !bobgui_switch_get_active (sw));
    }
  else if (bobgui_widget_is_ancestor (vis->fps_switch, BOBGUI_WIDGET (row)))
    {
      BobguiSwitch *sw = BOBGUI_SWITCH (vis->fps_switch);
      bobgui_switch_set_active (sw, !bobgui_switch_get_active (sw));
    }
  else if (bobgui_widget_is_ancestor (vis->updates_switch, BOBGUI_WIDGET (row)))
    {
      BobguiSwitch *sw = BOBGUI_SWITCH (vis->updates_switch);
      bobgui_switch_set_active (sw, !bobgui_switch_get_active (sw));
    }
  else if (bobgui_widget_is_ancestor (vis->cairo_switch, BOBGUI_WIDGET (row)))
    {
      BobguiSwitch *sw = BOBGUI_SWITCH (vis->cairo_switch);
      bobgui_switch_set_active (sw, !bobgui_switch_get_active (sw));
    }
  else if (bobgui_widget_is_ancestor (vis->baselines_switch, BOBGUI_WIDGET (row)))
    {
      BobguiSwitch *sw = BOBGUI_SWITCH (vis->baselines_switch);
      bobgui_switch_set_active (sw, !bobgui_switch_get_active (sw));
    }
  else if (bobgui_widget_is_ancestor (vis->layout_switch, BOBGUI_WIDGET (row)))
    {
      BobguiSwitch *sw = BOBGUI_SWITCH (vis->layout_switch);
      bobgui_switch_set_active (sw, !bobgui_switch_get_active (sw));
    }
  else if (bobgui_widget_is_ancestor (vis->focus_switch, BOBGUI_WIDGET (row)))
    {
      BobguiSwitch *sw = BOBGUI_SWITCH (vis->focus_switch);
      bobgui_switch_set_active (sw, !bobgui_switch_get_active (sw));
    }
  else if (bobgui_widget_is_ancestor (vis->touchscreen_switch, BOBGUI_WIDGET (row)))
    {
      BobguiSwitch *sw = BOBGUI_SWITCH (vis->touchscreen_switch);
      bobgui_switch_set_active (sw, !bobgui_switch_get_active (sw));
    }
  else if (bobgui_widget_is_ancestor (vis->a11y_switch, BOBGUI_WIDGET (row)))
    {
      BobguiSwitch *sw = BOBGUI_SWITCH (vis->a11y_switch);
      bobgui_switch_set_active (sw, !bobgui_switch_get_active (sw));
    }
  else if (bobgui_widget_is_ancestor (vis->subsurface_switch, BOBGUI_WIDGET (row)))
    {
      BobguiSwitch *sw = BOBGUI_SWITCH (vis->subsurface_switch);
      bobgui_switch_set_active (sw, !bobgui_switch_get_active (sw));
    }
}

static void
init_gl (BobguiInspectorVisual *vis)
{
}

static void
inspect_inspector (BobguiButton          *button,
                   BobguiInspectorVisual *vis)
{
  BobguiWidget *inspector_window;

  inspector_window = bobgui_inspector_window_get (bobgui_widget_get_display (BOBGUI_WIDGET (button)));
  bobgui_window_present (BOBGUI_WINDOW (inspector_window));
}

static void
bobgui_inspector_visual_init (BobguiInspectorVisual *vis)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (vis));
}

static void
bobgui_inspector_visual_constructed (GObject *object)
{
  BobguiInspectorVisual *vis = BOBGUI_INSPECTOR_VISUAL (object);

  G_OBJECT_CLASS (bobgui_inspector_visual_parent_class)->constructed (object);

  g_signal_connect (vis->visual_box, "keynav-failed", G_CALLBACK (keynav_failed), vis);
  g_signal_connect (vis->debug_box, "keynav-failed", G_CALLBACK (keynav_failed), vis);
  g_signal_connect (vis->misc_box, "keynav-failed", G_CALLBACK (keynav_failed), vis);
  g_signal_connect (vis->visual_box, "row-activated", G_CALLBACK (row_activated), vis);
  g_signal_connect (vis->debug_box, "row-activated", G_CALLBACK (row_activated), vis);
  g_signal_connect (vis->misc_box, "row-activated", G_CALLBACK (row_activated), vis);
}

static void
bobgui_inspector_visual_unroot (BobguiWidget *widget)
{
  BobguiInspectorVisual *vis = BOBGUI_INSPECTOR_VISUAL (widget);
  BobguiInspectorWindow *iw = BOBGUI_INSPECTOR_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (vis)));

  if (vis->layout_overlay)
    {
      bobgui_inspector_window_remove_overlay (iw, vis->layout_overlay);
      vis->layout_overlay = NULL;
    }
  if (vis->updates_overlay)
    {
      bobgui_inspector_window_remove_overlay (iw, vis->updates_overlay);
      vis->updates_overlay = NULL;
    }
  if (vis->fps_overlay)
    {
      bobgui_inspector_window_remove_overlay (iw, vis->fps_overlay);
      vis->fps_overlay = NULL;
    }
  if (vis->focus_overlay)
    {
      bobgui_inspector_window_remove_overlay (iw, vis->focus_overlay);
      vis->focus_overlay = NULL;
    }
  if (vis->a11y_overlay)
    {
      bobgui_inspector_window_remove_overlay (iw, vis->a11y_overlay);
      vis->a11y_overlay = NULL;
    }

  BOBGUI_WIDGET_CLASS (bobgui_inspector_visual_parent_class)->unroot (widget);
}

static void
bobgui_inspector_visual_dispose (GObject *object)
{
  BobguiInspectorVisual *vis = BOBGUI_INSPECTOR_VISUAL (object);

  bobgui_widget_dispose_template (BOBGUI_WIDGET (vis), BOBGUI_TYPE_INSPECTOR_VISUAL);

  G_OBJECT_CLASS (bobgui_inspector_visual_parent_class)->dispose (object);
}

static void
bobgui_inspector_visual_class_init (BobguiInspectorVisualClass *klass)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = bobgui_inspector_visual_constructed;
  object_class->dispose = bobgui_inspector_visual_dispose;

  widget_class->unroot = bobgui_inspector_visual_unroot;

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/inspector/visual.ui");
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, swin);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, direction_combo);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, theme_combo);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, color_scheme_combo);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, contrast_combo);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, motion_combo);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, cursor_combo);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, cursor_size_spin);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, cursor_size_adjustment);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, icon_combo);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, animation_switch);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, slowdown_adjustment);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, slowdown_entry);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, touchscreen_switch);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, visual_box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, font_rendering_combo);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, debug_box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, font_button);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, misc_box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, font_scale_entry);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, font_scale_adjustment);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, fps_switch);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, updates_switch);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, cairo_switch);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, baselines_switch);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, layout_switch);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, focus_switch);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, a11y_switch);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorVisual, subsurface_switch);

  bobgui_widget_class_bind_template_callback (widget_class, fps_activate);
  bobgui_widget_class_bind_template_callback (widget_class, updates_activate);
  bobgui_widget_class_bind_template_callback (widget_class, cairo_activate);
  bobgui_widget_class_bind_template_callback (widget_class, direction_changed);
  bobgui_widget_class_bind_template_callback (widget_class, font_rendering_changed);
  bobgui_widget_class_bind_template_callback (widget_class, baselines_activate);
  bobgui_widget_class_bind_template_callback (widget_class, layout_activate);
  bobgui_widget_class_bind_template_callback (widget_class, focus_activate);
  bobgui_widget_class_bind_template_callback (widget_class, a11y_activate);
  bobgui_widget_class_bind_template_callback (widget_class, subsurface_activate);
  bobgui_widget_class_bind_template_callback (widget_class, inspect_inspector);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
}

void
bobgui_inspector_visual_set_display (BobguiInspectorVisual *vis,
                                  GdkDisplay         *display)
{
  vis->display = display;
  vis->settings = bobgui_settings_get_for_display (display);

  init_direction (vis);
  init_theme (vis);
  init_colorscheme (vis);
  init_contrast (vis);
  init_reduced_motion (vis);
  init_icons (vis);
  init_cursors (vis);
  init_cursor_size (vis);
  init_font (vis);
  init_font_scale (vis);
  init_font_rendering (vis);
  init_animation (vis);
  init_slowdown (vis);
  init_touchscreen (vis);
  init_gl (vis);
}

// vim: set et sw=2 ts=2:
