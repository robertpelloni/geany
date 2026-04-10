/* widget-factory: a collection of widgets, for easy theme testing
 *
 * Copyright (C) 2011 Canonical Ltd
 *
 * This  library is free  software; you can  redistribute it and/or
 * modify it  under  the terms  of the  GNU Lesser  General  Public
 * License  as published  by the Free  Software  Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed  in the hope that it will be useful,
 * but  WITHOUT ANY WARRANTY; without even  the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by Andrea Cimitan <andrea.cimitan@canonical.com>
 */

#include "config.h"

#include <stdlib.h>

#include <glib/gi18n.h>
#include <gmodule.h>
#include <bobgui/bobgui.h>

#include "profile_conf.h"

static void
change_dark_state (GSimpleAction *action,
                    GVariant      *state,
                    gpointer       user_data)
{
  BobguiSettings *settings = bobgui_settings_get_default ();
  BobguiInterfaceColorScheme color_scheme;

  if (g_variant_get_boolean (state))
    color_scheme = BOBGUI_INTERFACE_COLOR_SCHEME_DARK;
  else
    color_scheme = BOBGUI_INTERFACE_COLOR_SCHEME_LIGHT;

  g_object_set (G_OBJECT (settings),
                "bobgui-interface-color-scheme", color_scheme,
                NULL);

  g_simple_action_set_state (action, state);
}

static void
change_theme_state (GSimpleAction *action,
                    GVariant      *state,
                    gpointer       user_data)
{
  BobguiSettings *settings = bobgui_settings_get_default ();
  BobguiInterfaceColorScheme color_scheme;
  BobguiInterfaceContrast contrast;
  const char *s;

  s = g_variant_get_string (state, NULL);

  g_simple_action_set_state (action, state);

  if (strcmp (s, "default") == 0)
    {
      color_scheme = BOBGUI_INTERFACE_COLOR_SCHEME_LIGHT;
      contrast = BOBGUI_INTERFACE_CONTRAST_NO_PREFERENCE;
    }
  else if (strcmp (s, "dark") == 0)
    {
      color_scheme = BOBGUI_INTERFACE_COLOR_SCHEME_DARK;
      contrast = BOBGUI_INTERFACE_CONTRAST_NO_PREFERENCE;
    }
  else if (strcmp (s, "hc") == 0)
    {
      color_scheme = BOBGUI_INTERFACE_COLOR_SCHEME_LIGHT;
      contrast = BOBGUI_INTERFACE_CONTRAST_MORE;
    }
  else if (strcmp (s, "hc-dark") == 0)
    {
      color_scheme = BOBGUI_INTERFACE_COLOR_SCHEME_DARK;
      contrast = BOBGUI_INTERFACE_CONTRAST_MORE;
    }
  else
    return;

  g_object_set (settings,
                "bobgui-interface-color-scheme", color_scheme,
                "bobgui-interface-contrast", contrast,
                NULL);
}

static void
change_fullscreen (GSimpleAction *action,
                   GVariant      *state,
                   gpointer       user_data)
{
  BobguiWindow *window = user_data;

  if (g_variant_get_boolean (state))
    bobgui_window_fullscreen (window);
  else
    bobgui_window_unfullscreen (window);

  g_simple_action_set_state (action, state);
}

static BobguiWidget *page_stack;

static void
transition_speed_changed (BobguiRange *range,
                          gpointer  data)
{
  double value;

  value = bobgui_range_get_value (range);
  bobgui_stack_set_transition_duration (BOBGUI_STACK (page_stack), (int)value);
}

static void
change_transition_state (GSimpleAction *action,
                         GVariant      *state,
                         gpointer       user_data)
{
  BobguiStackTransitionType transition;

  if (g_variant_get_boolean (state))
    transition = BOBGUI_STACK_TRANSITION_TYPE_CROSSFADE;
  else
    transition = BOBGUI_STACK_TRANSITION_TYPE_NONE;

  bobgui_stack_set_transition_type (BOBGUI_STACK (page_stack), transition);

  g_simple_action_set_state (action, state);
}

static gboolean
get_idle (gpointer data)
{
  BobguiWidget *window = data;
  BobguiApplication *app = bobgui_window_get_application (BOBGUI_WINDOW (window));

  bobgui_widget_set_sensitive (window, TRUE);
  gdk_surface_set_cursor (bobgui_native_get_surface (BOBGUI_NATIVE (window)), NULL);
  g_application_unmark_busy (G_APPLICATION (app));

  return G_SOURCE_REMOVE;
}

static void
get_busy (GSimpleAction *action,
          GVariant      *parameter,
          gpointer       user_data)
{
  BobguiWidget *window = user_data;
  GdkCursor *cursor;
  BobguiApplication *app = bobgui_window_get_application (BOBGUI_WINDOW (window));

  g_application_mark_busy (G_APPLICATION (app));
  cursor = gdk_cursor_new_from_name ("wait", NULL);
  gdk_surface_set_cursor (bobgui_native_get_surface (BOBGUI_NATIVE (window)), cursor);
  g_object_unref (cursor);
  g_timeout_add (5000, get_idle, window);

  bobgui_widget_set_sensitive (window, FALSE);
}

static int current_page = 0;
static gboolean
on_page (int i)
{
  return current_page == i;
}

static void
activate_search (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  BobguiWidget *window = user_data;
  BobguiWidget *searchbar;

  if (!on_page (2))
    return;

  searchbar = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (window), "searchbar"));
  bobgui_search_bar_set_search_mode (BOBGUI_SEARCH_BAR (searchbar), TRUE);
}

static void
activate_delete (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  BobguiWidget *window = user_data;
  BobguiWidget *infobar;

  g_print ("Activate action delete\n");

  if (!on_page (2))
    return;

  infobar = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (window), "infobar"));
  bobgui_widget_set_visible (infobar, TRUE);
}

static void populate_flowbox (BobguiWidget *flowbox);

static void
activate_background (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
  BobguiWidget *window = user_data;
  BobguiWidget *dialog;
  BobguiWidget *flowbox;

  if (!on_page (2))
    return;

  dialog = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (window), "selection_dialog"));
  flowbox = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (window), "selection_flowbox"));

  bobgui_widget_set_visible (dialog, TRUE);
  populate_flowbox (flowbox);
}

static void
file_chooser_response (GObject *source,
                       GAsyncResult *result,
                       void *user_data)
{
  BobguiFileDialog *dialog = BOBGUI_FILE_DIALOG (source);
  GFile *file;

  file = bobgui_file_dialog_open_finish (dialog, result, NULL);
  if (file)
    {
      g_print ("File selected: %s", g_file_peek_path (file));
      g_object_unref (file);
    }
}

static void
activate_open_file (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  BobguiFileDialog *dialog;

  dialog = bobgui_file_dialog_new ();
  bobgui_file_dialog_open (dialog, NULL, NULL, file_chooser_response, NULL);
  g_object_unref (dialog);
}

static void
activate_open (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  BobguiWidget *window = user_data;
  BobguiWidget *button;

  if (!on_page (3))
    return;

  button = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (window), "open_menubutton"));
  g_signal_emit_by_name (button, "clicked");
}

static void
activate_record (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  BobguiWidget *window = user_data;
  BobguiWidget *button;

  if (!on_page (3))
    return;

  button = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (window), "record_button"));
  g_signal_emit_by_name (button, "clicked");
}

static void
activate_lock (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  BobguiWidget *window = user_data;
  BobguiWidget *button;

  if (!on_page (3))
    return;

  button = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (window), "lockbutton"));
  g_signal_emit_by_name (button, "clicked");
}

static void
activate_about (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  BobguiApplication *app = user_data;
  BobguiWindow *window;
  BobguiWidget *button;
  char *version;
  char *os_name;
  char *os_version;
  GString *s;
  BobguiWidget *dialog;
  GFile *logo_file;
  BobguiIconPaintable *logo;

  s = g_string_new ("");

  window = bobgui_application_get_active_window (app);
  button = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (window), "open_menubutton"));
  bobgui_menu_button_popdown (BOBGUI_MENU_BUTTON (button));

  os_name = g_get_os_info (G_OS_INFO_KEY_NAME);
  os_version = g_get_os_info (G_OS_INFO_KEY_VERSION_ID);
  if (os_name && os_version)
    g_string_append_printf (s, "OS\t%s %s\n\n", os_name, os_version);
  g_string_append (s, "System libraries\n");
  g_string_append_printf (s, "\tGLib\t%d.%d.%d\n",
                          glib_major_version,
                          glib_minor_version,
                          glib_micro_version);
  g_string_append_printf (s, "\tPango\t%s\n",
                          pango_version_string ());
  g_string_append_printf (s, "\tBOBGUI \t%d.%d.%d\n",
                          bobgui_get_major_version (),
                          bobgui_get_minor_version (),
                          bobgui_get_micro_version ());
  g_string_append_printf (s, "\nA link can appear here: <http://www.bobgui.org>");

  version = g_strdup_printf ("%s%s%s\nRunning against BOBGUI %d.%d.%d",
                             PACKAGE_VERSION,
                             g_strcmp0 (PROFILE, "devel") == 0 ? "-" : "",
                             g_strcmp0 (PROFILE, "devel") == 0 ? VCS_TAG : "",
                             bobgui_get_major_version (),
                             bobgui_get_minor_version (),
                             bobgui_get_micro_version ());

  logo_file = g_file_new_for_uri ("resource:///org/bobgui/WidgetFactory4/icons/scalable/apps/org.bobgui.WidgetFactory4.svg");
  logo = bobgui_icon_paintable_new_for_file (logo_file, 64, 1);
  dialog = g_object_new (BOBGUI_TYPE_ABOUT_DIALOG,
                         "transient-for", bobgui_application_get_active_window (app),
                         "modal", TRUE,
                         "program-name", g_strcmp0 (PROFILE, "devel") == 0
                                         ? "BOBGUI Widget Factory (Development)"
                                         : "BOBGUI Widget Factory",
                         "version", version,
                         "copyright", "© 1997—2024 The BOBGUI Team",
                         "license-type", BOBGUI_LICENSE_LGPL_2_1,
                         "website", "http://www.bobgui.org",
                         "comments", "Program to demonstrate BOBGUI themes and widgets",
                         "authors", (const char *[]) { "Andrea Cimitan", "Cosimo Cecchi", NULL },
                         "logo", logo,
                         "title", "About BOBGUI Widget Factory",
                         "system-information", s->str,
                         NULL);
  g_object_unref (logo);
  g_object_unref (logo_file);

  bobgui_about_dialog_add_credit_section (BOBGUI_ABOUT_DIALOG (dialog),
                                       _("Maintained by"), (const char *[]) { "The BOBGUI Team", NULL });

  bobgui_window_present (BOBGUI_WINDOW (dialog));

  g_string_free (s, TRUE);
  g_free (version);
  g_free (os_name);
  g_free (os_version);
}

static void
activate_shortcuts_window (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
{
  BobguiApplication *app = user_data;
  BobguiWindow *window;
  BobguiWidget *button;

  window = bobgui_application_get_active_window (app);
  button = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (window), "open_menubutton"));
  bobgui_menu_button_popdown (BOBGUI_MENU_BUTTON (button));
  bobgui_widget_activate_action (BOBGUI_WIDGET (window), "win.show-help-overlay", NULL);
}

static void
activate_quit (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  BobguiApplication *app = user_data;
  BobguiWidget *win;
  GList *list, *next;

  list = bobgui_application_get_windows (app);
  while (list)
    {
      win = list->data;
      next = list->next;

      bobgui_window_destroy (BOBGUI_WINDOW (win));

      list = next;
    }
}

static void
activate_inspector (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  bobgui_window_set_interactive_debugging (TRUE);
}

static void
print_operation_done (BobguiPrintOperation       *op,
                      BobguiPrintOperationResult  res,
                      gpointer                 data)
{
  GError *error = NULL;

  switch (res)
    {
    case BOBGUI_PRINT_OPERATION_RESULT_ERROR:
      bobgui_print_operation_get_error (op, &error);
      g_print ("Printing failed: %s\n", error->message);
      g_clear_error (&error);
      break;
    case BOBGUI_PRINT_OPERATION_RESULT_APPLY:
      break;
    case BOBGUI_PRINT_OPERATION_RESULT_CANCEL:
      g_print ("Printing was canceled\n");
      break;
    case BOBGUI_PRINT_OPERATION_RESULT_IN_PROGRESS:
      return;
    default:
      g_assert_not_reached ();
      break;
    }

  g_object_unref (op);
}

static void
print_operation_begin (BobguiPrintOperation *op,
                       BobguiPrintContext   *context,
                       gpointer           data)
{
  bobgui_print_operation_set_n_pages (op, 1);
}

static void
print_operation_page (BobguiPrintOperation *op,
                      BobguiPrintContext   *context,
                      int                page,
                      gpointer           data)
{
  cairo_t *cr;
  double width;
  double aspect_ratio;
  GdkSnapshot *snapshot;
  GdkPaintable *paintable;
  GskRenderNode *node;

  g_print ("Save the trees!\n");

  cr = bobgui_print_context_get_cairo_context (context);
  width = bobgui_print_context_get_width (context);

  snapshot = bobgui_snapshot_new ();
  paintable = bobgui_widget_paintable_new (BOBGUI_WIDGET (data));
  aspect_ratio = gdk_paintable_get_intrinsic_aspect_ratio (paintable);
  gdk_paintable_snapshot (paintable, snapshot, width, width / aspect_ratio);
  node = bobgui_snapshot_free_to_node (snapshot);

  gsk_render_node_draw (node, cr);

  gsk_render_node_unref (node);

  g_object_unref (paintable);
}

static void
activate_print (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  BobguiWindow *window = BOBGUI_WINDOW (user_data);
  BobguiPrintOperation *op;
  BobguiPrintOperationResult res;

  op = bobgui_print_operation_new ();
  bobgui_print_operation_set_allow_async (op, TRUE);
  g_signal_connect (op, "begin-print", G_CALLBACK (print_operation_begin), NULL);
  g_signal_connect (op, "draw-page", G_CALLBACK (print_operation_page), window);
  g_signal_connect (op, "done", G_CALLBACK (print_operation_done), NULL);

  bobgui_print_operation_set_embed_page_setup (op, TRUE);

  res = bobgui_print_operation_run (op, BOBGUI_PRINT_OPERATION_ACTION_PRINT_DIALOG, window, NULL);

  if (res == BOBGUI_PRINT_OPERATION_RESULT_IN_PROGRESS)
    return;

  print_operation_done (op, res, NULL);
}

static void
spin_value_changed (BobguiAdjustment *adjustment, BobguiWidget *label)
{
  BobguiWidget *w;
  int v;
  char *text;

  v = (int)bobgui_adjustment_get_value (adjustment);

  if ((v % 3) == 0)
    {
      text = g_strdup_printf ("%d is a multiple of 3", v);
      bobgui_label_set_label (BOBGUI_LABEL (label), text);
      g_free (text);
    }

  w = bobgui_widget_get_ancestor (label, BOBGUI_TYPE_REVEALER);
  bobgui_revealer_set_reveal_child (BOBGUI_REVEALER (w), (v % 3) == 0);
}

static void
dismiss (BobguiWidget *button)
{
  BobguiWidget *w;

  w = bobgui_widget_get_ancestor (button, BOBGUI_TYPE_REVEALER);
  bobgui_revealer_set_reveal_child (BOBGUI_REVEALER (w), FALSE);
}

static void
spin_value_reset (BobguiWidget *button, BobguiAdjustment *adjustment)
{
  bobgui_adjustment_set_value (adjustment, 50.0);
  dismiss (button);
}

static int pulse_time = 250;
static int pulse_entry_mode = 0;

static void
remove_pulse (gpointer pulse_id)
{
  g_source_remove (GPOINTER_TO_UINT (pulse_id));
}

static gboolean
pulse_it (BobguiWidget *widget)
{
  guint pulse_id;

  if (BOBGUI_IS_ENTRY (widget))
    bobgui_entry_progress_pulse (BOBGUI_ENTRY (widget));
  else
    bobgui_progress_bar_pulse (BOBGUI_PROGRESS_BAR (widget));

  pulse_id = g_timeout_add (pulse_time, (GSourceFunc)pulse_it, widget);
  g_object_set_data_full (G_OBJECT (widget), "pulse_id", GUINT_TO_POINTER (pulse_id), remove_pulse);

  return G_SOURCE_REMOVE;
}

static void
update_pulse_time (BobguiAdjustment *adjustment, BobguiWidget *widget)
{
  double value;
  guint pulse_id;

  value = bobgui_adjustment_get_value (adjustment);

  pulse_id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (widget), "pulse_id"));

  /* vary between 50 and 450 */
  pulse_time = 50 + 4 * value;

  if (value == 100)
    {
      g_object_set_data (G_OBJECT (widget), "pulse_id", NULL);
    }
  else if (value < 100)
    {
      if (pulse_id == 0 && (BOBGUI_IS_PROGRESS_BAR (widget) || pulse_entry_mode % 3 == 2))
        {
          pulse_id = g_timeout_add (pulse_time, (GSourceFunc)pulse_it, widget);
          g_object_set_data_full (G_OBJECT (widget), "pulse_id", GUINT_TO_POINTER (pulse_id), remove_pulse);
        }
    }
}

static void
on_entry_icon_release (BobguiEntry            *entry,
                       BobguiEntryIconPosition icon_pos,
                       gpointer             user_data)
{
  BobguiSvg *paintable;

  if (icon_pos != BOBGUI_ENTRY_ICON_SECONDARY)
    return;

  pulse_entry_mode++;

  if (pulse_entry_mode % 3 == 0)
    {
      g_object_set_data (G_OBJECT (entry), "pulse_id", NULL);
      bobgui_entry_set_progress_fraction (entry, 0);
    }
  else if (pulse_entry_mode % 3 == 1)
    {
      bobgui_entry_set_progress_fraction (entry, 0.25);
    }
  else if (pulse_entry_mode % 3 == 2)
    {
      if (pulse_time - 50 < 400)
        {
          bobgui_entry_set_progress_pulse_step (entry, 0.1);
          pulse_it (BOBGUI_WIDGET (entry));
        }
    }

  g_object_get (entry, "secondary-icon-paintable", &paintable, NULL);
  bobgui_svg_set_state (paintable, pulse_entry_mode % 3);
  g_object_unref (paintable);
}

#define EPSILON (1e-10)

static void
on_scale_button_value_changed (BobguiScaleButton *button,
                               double          value,
                               gpointer        user_data)
{
  BobguiAdjustment *adjustment;
  double val;
  char *str;

  adjustment = bobgui_scale_button_get_adjustment (button);
  val = bobgui_scale_button_get_value (button);

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

      str = g_strdup_printf (C_("volume percentage", "%d %%"), percent);
    }

  bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (button), str);

  g_free (str);
}

static void
on_record_button_toggled (BobguiToggleButton *button,
                          gpointer         user_data)
{

  if (bobgui_toggle_button_get_active (button))
    bobgui_widget_remove_css_class (BOBGUI_WIDGET (button), "destructive-action");
  else
    bobgui_widget_add_css_class (BOBGUI_WIDGET (button), "destructive-action");
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static void
on_page_combo_changed (BobguiComboBox *combo,
                       gpointer     user_data)
{
  BobguiWidget *from;
  BobguiWidget *to;
  BobguiWidget *print;

  from = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (combo), "range_from_spin"));
  to = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (combo), "range_to_spin"));
  print = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (combo), "print_button"));

  switch (bobgui_combo_box_get_active (combo))
    {
    case 0: /* Range */
      bobgui_widget_set_sensitive (from, TRUE);
      bobgui_widget_set_sensitive (to, TRUE);
      bobgui_widget_set_sensitive (print, TRUE);
      break;
    case 1: /* All */
      bobgui_widget_set_sensitive (from, FALSE);
      bobgui_widget_set_sensitive (to, FALSE);
      bobgui_spin_button_set_value (BOBGUI_SPIN_BUTTON (from), 1);
      bobgui_spin_button_set_value (BOBGUI_SPIN_BUTTON (to), 99);
      bobgui_widget_set_sensitive (print, TRUE);
      break;
    case 2: /* Current */
      bobgui_widget_set_sensitive (from, FALSE);
      bobgui_widget_set_sensitive (to, FALSE);
      bobgui_spin_button_set_value (BOBGUI_SPIN_BUTTON (from), 7);
      bobgui_spin_button_set_value (BOBGUI_SPIN_BUTTON (to), 7);
      bobgui_widget_set_sensitive (print, TRUE);
      break;
    case 4:
      bobgui_widget_set_sensitive (from, FALSE);
      bobgui_widget_set_sensitive (to, FALSE);
      bobgui_widget_set_sensitive (print, FALSE);
      break;
    default:;
    }
}
G_GNUC_END_IGNORE_DEPRECATIONS

static void
on_range_from_changed (BobguiSpinButton *from)
{
  BobguiSpinButton *to;
  int v1, v2;

  to = BOBGUI_SPIN_BUTTON (g_object_get_data (G_OBJECT (from), "range_to_spin"));

  v1 = bobgui_spin_button_get_value_as_int (from);
  v2 = bobgui_spin_button_get_value_as_int (to);

  if (v1 > v2)
    bobgui_spin_button_set_value (to, v1);
}

static void
on_range_to_changed (BobguiSpinButton *to)
{
  BobguiSpinButton *from;
  int v1, v2;

  from = BOBGUI_SPIN_BUTTON (g_object_get_data (G_OBJECT (to), "range_from_spin"));

  v1 = bobgui_spin_button_get_value_as_int (from);
  v2 = bobgui_spin_button_get_value_as_int (to);

  if (v1 > v2)
    bobgui_spin_button_set_value (from, v2);
}

static GdkContentProvider *
on_picture_drag_prepare (BobguiDragSource *source,
                         double         x,
                         double         y,
                         gpointer       unused)
{
  BobguiWidget *picture;

  picture = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (source));

  return gdk_content_provider_new_typed (GDK_TYPE_TEXTURE, bobgui_picture_get_paintable (BOBGUI_PICTURE (picture)));
}

static gboolean
on_picture_drop (BobguiDropTarget *dest,
                 const GValue  *value,
                 double         x,
                 double         y,
                 gpointer       unused)
{
  BobguiWidget *picture;
  GdkPaintable *paintable;

  picture = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (dest));
  paintable = g_value_get_object (value);

  bobgui_picture_set_paintable (BOBGUI_PICTURE (picture), paintable);

  return TRUE;
}

static void
info_bar_response (BobguiWidget *infobar, int response_id)
{
  if (response_id == BOBGUI_RESPONSE_CLOSE)
    bobgui_widget_set_visible (infobar, FALSE);
}

static void
show_dialog (BobguiWidget *button, BobguiWidget *dialog)
{
  bobgui_widget_set_visible (dialog, TRUE);
}

static void
close_dialog (BobguiWidget *dialog)
{
  bobgui_widget_set_visible (dialog, FALSE);
}

static void
set_needs_attention (BobguiWidget *page, gboolean needs_attention)
{
  BobguiWidget *stack;

  stack = bobgui_widget_get_parent (page);
  g_object_set (bobgui_stack_get_page (BOBGUI_STACK (stack), page),
                           "needs-attention", needs_attention,
                           NULL);
}

static gboolean
demand_attention (gpointer stack)
{
  BobguiWidget *page;

  page = bobgui_stack_get_child_by_name (BOBGUI_STACK (stack), "page3");
  set_needs_attention (page, TRUE);

  return G_SOURCE_REMOVE;
}

static void
action_dialog_button_clicked (BobguiButton *button, BobguiWidget *page)
{
  g_timeout_add (1000, demand_attention, page);
}

static void
page_changed_cb (BobguiWidget *stack, GParamSpec *pspec, gpointer data)
{
  const char *name;
  BobguiWidget *window;
  BobguiWidget *page;

  if (bobgui_widget_in_destruction (stack))
    return;

  name = bobgui_stack_get_visible_child_name (BOBGUI_STACK (stack));

  window = bobgui_widget_get_ancestor (stack, BOBGUI_TYPE_APPLICATION_WINDOW);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  g_object_set (bobgui_application_window_get_help_overlay (BOBGUI_APPLICATION_WINDOW (window)),
                "view-name", name,
                NULL);
G_GNUC_END_IGNORE_DEPRECATIONS

  if (g_str_equal (name, "page1"))
    current_page = 1;
  else if (g_str_equal (name, "page2"))
    current_page = 2;
  if (g_str_equal (name, "page3"))
    {
      current_page = 3;
      page = bobgui_stack_get_visible_child (BOBGUI_STACK (stack));
      set_needs_attention (BOBGUI_WIDGET (page), FALSE);
    }
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static void
populate_model (BobguiTreeStore *store)
{
  BobguiTreeIter iter, parent0, parent1, parent2, parent3;

  bobgui_tree_store_append (store, &iter, NULL);
  bobgui_tree_store_set (store, &iter,
                      0, "Charlemagne",
                      1, "742",
                      2, "814",
                      -1);
  parent0 = iter;
  bobgui_tree_store_append (store, &iter, &parent0);
  bobgui_tree_store_set (store, &iter,
                      0, "Pepin the Short",
                      1, "714",
                      2, "768",
                      -1);
  parent1 = iter;
  bobgui_tree_store_append (store, &iter, &parent1);
  bobgui_tree_store_set (store, &iter,
                      0, "Charles Martel",
                      1, "688",
                      2, "741",
                      -1);
  parent2 = iter;
  bobgui_tree_store_append (store, &iter, &parent2);
  bobgui_tree_store_set (store, &iter,
                      0, "Pepin of Herstal",
                      1, "635",
                      2, "714",
                      -1);
  parent3 = iter;
  bobgui_tree_store_append (store, &iter, &parent3);
  bobgui_tree_store_set (store, &iter,
                      0, "Ansegisel",
                      1, "602 or 610",
                      2, "murdered before 679",
                      -1);
  bobgui_tree_store_append (store, &iter, &parent3);
  bobgui_tree_store_set (store, &iter,
                      0, "Begga",
                      1, "615",
                      2, "693",
                      -1);
  bobgui_tree_store_append (store, &iter, &parent2);
  bobgui_tree_store_set (store, &iter,
                      0, "Alpaida",
                      -1);
  bobgui_tree_store_append (store, &iter, &parent1);
  bobgui_tree_store_set (store, &iter,
                      0, "Rotrude",
                      -1);
  parent2 = iter;
  bobgui_tree_store_append (store, &iter, &parent2);
  bobgui_tree_store_set (store, &iter,
                      0, "Liévin de Trèves",
                      -1);
  parent3 = iter;
  bobgui_tree_store_append (store, &iter, &parent3);
  bobgui_tree_store_set (store, &iter,
                      0, "Guérin",
                      -1);
  bobgui_tree_store_append (store, &iter, &parent3);
  bobgui_tree_store_set (store, &iter,
                      0, "Gunza",
                      -1);
  bobgui_tree_store_append (store, &iter, &parent2);
  bobgui_tree_store_set (store, &iter,
                      0, "Willigarde de Bavière",
                      -1);
  bobgui_tree_store_append (store, &iter, &parent0);
  bobgui_tree_store_set (store, &iter,
                      0, "Bertrada of Laon",
                      1, "710",
                      2, "783",
                      -1);
  parent1 = iter;
  bobgui_tree_store_append (store, &iter, &parent1);
  bobgui_tree_store_set (store, &iter,
                      0, "Caribert of Laon",
                      2, "before 762",
                      -1);
  parent2 = iter;
  bobgui_tree_store_append (store, &iter, &parent2);
  bobgui_tree_store_set (store, &iter,
                      0, "Unknown",
                      -1);
  bobgui_tree_store_append (store, &iter, &parent2);
  bobgui_tree_store_set (store, &iter,
                      0, "Bertrada of Prüm",
                      1, "ca. 670",
                      2, "after 721",
                      -1);
  bobgui_tree_store_append (store, &iter, &parent1);
  bobgui_tree_store_set (store, &iter,
                      0, "Gisele of Aquitaine",
                      -1);
  bobgui_tree_store_append (store, &iter, NULL);
  bobgui_tree_store_set (store, &iter, 3, TRUE, -1);
  bobgui_tree_store_append (store, &iter, NULL);
  bobgui_tree_store_set (store, &iter,
                      0, "Attila the Hun",
                      1, "ca. 390",
                      2, "453",
                      -1);
}

static gboolean
row_separator_func (BobguiTreeModel *model, BobguiTreeIter *iter, gpointer data)
{
  gboolean is_sep;

  bobgui_tree_model_get (model, iter, 3, &is_sep, -1);

  return is_sep;
}
G_GNUC_END_IGNORE_DEPRECATIONS

static void
update_title_header (BobguiListBoxRow *row,
                     BobguiListBoxRow *before,
                     gpointer       data)
{
  BobguiWidget *header;
  char *title;

  header = bobgui_list_box_row_get_header (row);
  title = (char *)g_object_get_data (G_OBJECT (row), "title");
  if (!header && title)
    {
      title = g_strdup_printf ("<b>%s</b>", title);

      header = bobgui_label_new (title);
      bobgui_label_set_use_markup (BOBGUI_LABEL (header), TRUE);
      bobgui_widget_set_halign (header, BOBGUI_ALIGN_START);
      bobgui_widget_set_margin_top (header, 12);
      bobgui_widget_set_margin_start (header, 6);
      bobgui_widget_set_margin_end (header, 6);
      bobgui_widget_set_margin_bottom (header, 6);
      bobgui_widget_set_visible (header, TRUE);

      bobgui_list_box_row_set_header (row, header);

      g_free (title);
    }
}

static void
overshot (BobguiScrolledWindow *sw, BobguiPositionType pos, BobguiWidget *widget)
{
  BobguiWidget *box, *row, *label, *swatch;
  GdkRGBA rgba;
  const char *color;
  char *text;
  BobguiWidget *silver;
  BobguiWidget *gold;

  silver = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (widget), "Silver"));
  gold = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (widget), "Gold"));

  if (pos == BOBGUI_POS_TOP)
    {
      if (silver)
        {
          bobgui_list_box_remove (BOBGUI_LIST_BOX (widget), silver);
          g_object_set_data (G_OBJECT (widget), "Silver", NULL);
        }
      if (gold)
        {
          bobgui_list_box_remove (BOBGUI_LIST_BOX (widget), gold);
          g_object_set_data (G_OBJECT (widget), "Gold", NULL);
        }

      return;
    }


  if (gold)
    return;
  else if (silver)
    color = "Gold";
  else
    color = "Silver";

  row = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 20);
  text = g_strconcat ("<b>", color, "</b>", NULL);
  label = bobgui_label_new (text);
  g_free (text);
  g_object_set (label,
                "use-markup", TRUE,
                "halign", BOBGUI_ALIGN_START,
                "valign", BOBGUI_ALIGN_CENTER,
                "hexpand", TRUE,
                "margin-start", 6,
                "margin-end", 6,
                "margin-top", 6,
                "margin-bottom", 6,
                "xalign", 0.0,
                NULL);
  bobgui_box_append (BOBGUI_BOX (row), label);
  gdk_rgba_parse (&rgba, color);
  swatch = g_object_new (g_type_from_name ("BobguiColorSwatch"),
                         "rgba", &rgba,
                         "can-focus", FALSE,
                         "selectable", FALSE,
                         "halign", BOBGUI_ALIGN_END,
                         "valign", BOBGUI_ALIGN_CENTER,
                         "margin-start", 6,
                         "margin-end", 6,
                         "margin-top", 6,
                         "margin-bottom", 6,
                         "height-request", 24,
                         NULL);
  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_box_append (BOBGUI_BOX (box), swatch);
  bobgui_box_append (BOBGUI_BOX (row), box);
  bobgui_list_box_insert (BOBGUI_LIST_BOX (widget), row, -1);
  row = bobgui_widget_get_parent (row);
  bobgui_list_box_row_set_activatable (BOBGUI_LIST_BOX_ROW (row), FALSE);
  g_object_set_data (G_OBJECT (widget), color, row);
  g_object_set_data (G_OBJECT (row), "color", (gpointer)color);
}

static void
rgba_changed (BobguiColorChooser *chooser, GParamSpec *pspec, BobguiListBox *box)
{
  bobgui_list_box_select_row (box, NULL);
}

static void
set_color (BobguiListBox *box, BobguiListBoxRow *row, BobguiColorChooser *chooser)
{
  const char *color;
  GdkRGBA rgba;

  if (!row)
    return;

  color = (const char *)g_object_get_data (G_OBJECT (row), "color");

  if (!color)
    return;

  if (gdk_rgba_parse (&rgba, color))
    {
      g_signal_handlers_block_by_func (chooser, rgba_changed, box);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      bobgui_color_chooser_set_rgba (chooser, &rgba);
G_GNUC_END_IGNORE_DEPRECATIONS
      g_signal_handlers_unblock_by_func (chooser, rgba_changed, box);
    }
}

static void
populate_colors (BobguiWidget *widget, BobguiWidget *chooser)
{
  struct { const char *name; const char *color; const char *title; } colors[] = {
    { "2.5", "#C8828C", "Red" },
    { "5", "#C98286", NULL },
    { "7.5", "#C9827F", NULL },
    { "10", "#C98376", NULL },
    { "2.5", "#C8856D", "Red/Yellow" },
    { "5", "#C58764", NULL },
    { "7.5", "#C1895E", NULL },
    { "10", "#BB8C56", NULL },
    { "2.5", "#B58F4F", "Yellow" },
    { "5", "#AD924B", NULL },
    { "7.5", "#A79548", NULL },
    { "10", "#A09749", NULL },
    { "2.5", "#979A4E", "Yellow/Green" },
    { "5", "#8D9C55", NULL },
    { "7.5", "#7F9F62", NULL },
    { "10", "#73A06E", NULL },
    { "2.5", "#65A27C", "Green" },
    { "5", "#5CA386", NULL },
    { "7.5", "#57A38D", NULL },
    { "10", "#52A394", NULL },
    { "2.5", "#4EA39A", "Green/Blue" },
    { "5", "#49A3A2", NULL },
    { "7.5", "#46A2AA", NULL },
    { "10", "#46A1B1", NULL },
    { "2.5", "#49A0B8", "Blue" },
    { "5", "#529EBD", NULL },
    { "7.5", "#5D9CC1", NULL },
    { "10", "#689AC3", NULL },
    { "2.5", "#7597C5", "Blue/Purple" },
    { "5", "#8095C6", NULL },
    { "7.5", "#8D91C6", NULL },
    { "10", "#988EC4", NULL },
    { "2.5", "#A08CC1", "Purple" },
    { "5", "#A88ABD", NULL },
    { "7.5", "#B187B6", NULL },
    { "10", "#B786B0", NULL },
    { "2.5", "#BC84A9", "Purple/Red" },
    { "5", "#C183A0", NULL },
    { "7.5", "#C48299", NULL },
    { "10", "#C68292", NULL }
  };
  int i;
  BobguiWidget *row, *box, *label, *swatch;
  BobguiWidget *sw;
  GdkRGBA rgba;

  bobgui_list_box_set_header_func (BOBGUI_LIST_BOX (widget), update_title_header, NULL, NULL);

  for (i = 0; i < G_N_ELEMENTS (colors); i++)
    {
      row = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 20);
      label = bobgui_label_new (colors[i].name);
      g_object_set (label,
                    "halign", BOBGUI_ALIGN_START,
                    "valign", BOBGUI_ALIGN_CENTER,
                    "margin-start", 6,
                    "margin-end", 6,
                    "margin-top", 6,
                    "margin-bottom", 6,
                    "hexpand", TRUE,
                    "xalign", 0.0,
                    NULL);
      bobgui_box_append (BOBGUI_BOX (row), label);
      gdk_rgba_parse (&rgba, colors[i].color);
      swatch = g_object_new (g_type_from_name ("BobguiColorSwatch"),
                             "rgba", &rgba,
                             "selectable", FALSE,
                             "can-focus", FALSE,
                             "halign", BOBGUI_ALIGN_END,
                             "valign", BOBGUI_ALIGN_CENTER,
                             "margin-start", 6,
                             "margin-end", 6,
                             "margin-top", 6,
                             "margin-bottom", 6,
                             "height-request", 24,
                             NULL);
      box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_box_append (BOBGUI_BOX (box), swatch);
      bobgui_box_append (BOBGUI_BOX (row), box);
      bobgui_list_box_insert (BOBGUI_LIST_BOX (widget), row, -1);
      row = bobgui_widget_get_parent (row);
      bobgui_list_box_row_set_activatable (BOBGUI_LIST_BOX_ROW (row), FALSE);
      g_object_set_data (G_OBJECT (row), "color", (gpointer)colors[i].color);
      if (colors[i].title)
        g_object_set_data (G_OBJECT (row), "title", (gpointer)colors[i].title);
    }

  g_signal_connect (widget, "row-selected", G_CALLBACK (set_color), chooser);

  bobgui_list_box_invalidate_headers (BOBGUI_LIST_BOX (widget));

  sw = bobgui_widget_get_ancestor (widget, BOBGUI_TYPE_SCROLLED_WINDOW);
  g_signal_connect (sw, "edge-overshot", G_CALLBACK (overshot), widget);
}

typedef struct {
  BobguiWidget *flowbox;
  char *filename;
} BackgroundData;

static void
add_background (BobguiWidget  *flowbox,
                const char *filename,
                GdkTexture *texture,
                gboolean    is_resource)
{
  BobguiWidget *child;

  child = bobgui_picture_new_for_paintable (GDK_PAINTABLE (texture));
  bobgui_widget_set_size_request (child, 110, 70);
  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (flowbox), child, -1);
  child = bobgui_widget_get_parent (child);
  g_object_set_data_full (G_OBJECT (child), "filename", g_strdup (filename), g_free);
  if (is_resource)
    g_object_set_data (G_OBJECT (child), "is-resource", GINT_TO_POINTER (1));
}

static void
background_loaded_cb (GObject      *source,
                      GAsyncResult *res,
                      gpointer      data)
{
  BackgroundData *bd = data;
  GdkPixbuf *pixbuf;
  GdkTexture *texture;
  GError *error = NULL;

  pixbuf = gdk_pixbuf_new_from_stream_finish (res, &error);
  if (error)
    {
      g_warning ("Error loading '%s': %s", bd->filename, error->message);
      g_error_free (error);
      return;
    }

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  texture = gdk_texture_new_for_pixbuf (pixbuf);
G_GNUC_END_IGNORE_DEPRECATIONS
  add_background (bd->flowbox, bd->filename, texture, FALSE);

  g_object_unref (texture);
  g_object_unref (pixbuf);
  g_free (bd->filename);
  g_free (bd);
}

static void
populate_flowbox (BobguiWidget *flowbox)
{
  const char *location;
  GDir *dir;
  GError *error = NULL;
  const char *name;
  char *filename;
  GFile *file;
  GInputStream *stream;
  BackgroundData *bd;
  GdkPixbuf *pixbuf;
  GdkTexture *texture;
  BobguiWidget *child;
  guchar *data;
  GBytes *bytes;
  int i;
  const char *resources[] = {
    "sunset.jpg", "portland-rose.jpg", "beach.jpg", "nyc.jpg"
  };

  if (GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (flowbox), "populated")))
    return;

  g_object_set_data (G_OBJECT (flowbox), "populated", GUINT_TO_POINTER (1));

  data = g_malloc (4 * 110  * 70);
  memset (data, 0xff, 4 * 110  * 70);
  bytes = g_bytes_new_take (data, 4 * 110 * 70);
  texture = gdk_memory_texture_new (110, 70, GDK_MEMORY_DEFAULT, bytes, 4 * 110);
  child = bobgui_picture_new_for_paintable (GDK_PAINTABLE (texture));
  g_object_unref (texture);
  g_bytes_unref (bytes);

  bobgui_widget_add_css_class (child, "frame");
  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (flowbox), child, -1);

  for (i = 0; i < G_N_ELEMENTS (resources); i++)
    {
      filename = g_strconcat ("/org/bobgui/WidgetFactory4/", resources[i], NULL);
      pixbuf = gdk_pixbuf_new_from_resource_at_scale (filename, 110, 110, TRUE, NULL);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      texture = gdk_texture_new_for_pixbuf (pixbuf);
G_GNUC_END_IGNORE_DEPRECATIONS
      add_background (flowbox, filename, texture, TRUE);
      g_object_unref (texture);
      g_object_unref (pixbuf);
    }

  location = "/usr/share/backgrounds/gnome";
  dir = g_dir_open (location, 0, &error);
  if (error)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
      return;
    }

  while ((name = g_dir_read_name (dir)) != NULL)
    {
      if (g_str_has_suffix (name, ".xml"))
        continue;

      filename = g_build_filename (location, name, NULL);
      file = g_file_new_for_path (filename);
      stream = G_INPUT_STREAM (g_file_read (file, NULL, &error));
      if (error)
        {
          g_warning ("%s", error->message);
          g_clear_error (&error);
          g_free (filename);
        }
      else
        {
          bd = g_new (BackgroundData, 1);
          bd->flowbox = flowbox;
          bd->filename = filename;
          gdk_pixbuf_new_from_stream_at_scale_async (stream, 110, 110, TRUE, NULL,
                                                     background_loaded_cb, bd);
        }

      g_object_unref (file);
      g_object_unref (stream);
    }

  g_dir_close (dir);

}

static void
row_activated (BobguiListBox *box, BobguiListBoxRow *row)
{
  BobguiImage *image;
  BobguiWidget *dialog;

  image = (BobguiImage *) g_object_get_data (G_OBJECT (row), "image");
  dialog = (BobguiWidget *) g_object_get_data (G_OBJECT (row), "dialog");

  if (image)
    {
      BobguiSvg *paintable;

      paintable = BOBGUI_SVG (bobgui_image_get_paintable (image));
      if (bobgui_svg_get_state (paintable) == 0)
        bobgui_svg_set_state (paintable, 63);
      else
        bobgui_svg_set_state (paintable, 0);
    }
  else if (dialog)
    {
      bobgui_window_present (BOBGUI_WINDOW (dialog));
    }
}

typedef struct
{
  BobguiTextView tv;
  GdkTexture *texture;
  BobguiAdjustment *adjustment;
} MyTextView;

typedef BobguiTextViewClass MyTextViewClass;

static GType my_text_view_get_type (void);
G_DEFINE_TYPE (MyTextView, my_text_view, BOBGUI_TYPE_TEXT_VIEW)

static void
my_text_view_init (MyTextView *tv)
{
}

static void
my_tv_snapshot_layer (BobguiTextView      *widget,
                      BobguiTextViewLayer  layer,
                      BobguiSnapshot      *snapshot)
{
  MyTextView *tv = (MyTextView *)widget;
  double opacity;
  double scale;

  opacity = bobgui_adjustment_get_value (tv->adjustment) / 100.0;

  if (layer == BOBGUI_TEXT_VIEW_LAYER_BELOW_TEXT && tv->texture)
    {
      scale = bobgui_widget_get_width (BOBGUI_WIDGET (widget)) / (double)gdk_texture_get_width (tv->texture);
      bobgui_snapshot_push_opacity (snapshot, opacity);
      bobgui_snapshot_scale (snapshot, scale, scale);
      bobgui_snapshot_append_texture (snapshot,
                                   tv->texture,
                                   &GRAPHENE_RECT_INIT(
                                     0, 0,
                                     gdk_texture_get_width (tv->texture),
                                     gdk_texture_get_height (tv->texture)
                                   ));
      bobgui_snapshot_scale (snapshot, 1/scale, 1/scale);
      bobgui_snapshot_pop (snapshot);
    }
}

static void
my_tv_finalize (GObject *object)
{
  MyTextView *tv = (MyTextView *)object;

  g_clear_object (&tv->texture);

  G_OBJECT_CLASS (my_text_view_parent_class)->finalize (object);
}

static void
my_text_view_class_init (MyTextViewClass *class)
{
  BobguiTextViewClass *tv_class = BOBGUI_TEXT_VIEW_CLASS (class);
  GObjectClass *o_class = G_OBJECT_CLASS (class);

  o_class->finalize = my_tv_finalize;
  tv_class->snapshot_layer = my_tv_snapshot_layer;
}

static void
my_text_view_set_background (MyTextView *tv, const char *filename, gboolean is_resource)
{
  GError *error = NULL;
  GFile *file;

  g_clear_object (&tv->texture);

  if (filename == NULL)
    return;

  if (is_resource)
    tv->texture = gdk_texture_new_from_resource (filename);
  else
    {
      file = g_file_new_for_path (filename);
      tv->texture = gdk_texture_new_from_file (file, &error);
      g_object_unref (file);
    }

  if (error)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
      return;
    }

  bobgui_widget_queue_draw (BOBGUI_WIDGET (tv));
}

static void
value_changed (BobguiAdjustment *adjustment, MyTextView *tv)
{
  bobgui_widget_queue_draw (BOBGUI_WIDGET (tv));
}

static void
my_text_view_set_adjustment (MyTextView *tv, BobguiAdjustment *adjustment)
{
  g_set_object (&tv->adjustment, adjustment);
  g_signal_connect (tv->adjustment, "value-changed", G_CALLBACK (value_changed), tv);
}

static void
close_selection_dialog (BobguiWidget *dialog, int response, BobguiWidget *tv)
{
  BobguiWidget *box;
  BobguiWidget *child;
  GList *children;
  const char *filename;
  gboolean is_resource;

  bobgui_widget_set_visible (dialog, FALSE);

  if (response == BOBGUI_RESPONSE_CANCEL)
    return;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  box = bobgui_widget_get_first_child (bobgui_dialog_get_content_area (BOBGUI_DIALOG (dialog)));
G_GNUC_END_IGNORE_DEPRECATIONS
  g_assert (BOBGUI_IS_FLOW_BOX (box));
  children = bobgui_flow_box_get_selected_children (BOBGUI_FLOW_BOX (box));

  if (!children)
    return;

  child = children->data;
  filename = (const char *)g_object_get_data (G_OBJECT (child), "filename");
  is_resource = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (child), "is-resource"));

  g_list_free (children);

  my_text_view_set_background ((MyTextView *)tv, filename, is_resource);
}

static void
toggle_selection_mode (BobguiSwitch  *sw,
                       GParamSpec *pspec,
                       BobguiListBox *listbox)
{
  if (bobgui_switch_get_active (sw))
    bobgui_list_box_set_selection_mode (listbox, BOBGUI_SELECTION_SINGLE);
  else
    bobgui_list_box_set_selection_mode (listbox, BOBGUI_SELECTION_NONE);

  bobgui_list_box_set_activate_on_single_click (listbox, !bobgui_switch_get_active (sw));
}

static void
handle_insert (BobguiWidget *button, BobguiWidget *textview)
{
  BobguiTextBuffer *buffer;
  const char *id;
  const char *text;

  id = bobgui_buildable_get_buildable_id (BOBGUI_BUILDABLE (button));

  if (strcmp (id, "toolbutton1") == 0)
    text = "⌘";
  else if (strcmp (id, "toolbutton2") == 0)
    text = "⚽";
  else if (strcmp (id, "toolbutton3") == 0)
    text = "⤢";
  else if (strcmp (id, "toolbutton4") == 0)
    text = "☆";
  else
    text = "";

  buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (textview));
  bobgui_text_buffer_insert_at_cursor (buffer, text, -1);
}

static void
handle_cutcopypaste (BobguiWidget *button, BobguiWidget *textview)
{
  BobguiTextBuffer *buffer;
  GdkClipboard *clipboard;
  const char *id;

  clipboard = bobgui_widget_get_clipboard (textview);
  buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (textview));
  id = bobgui_buildable_get_buildable_id (BOBGUI_BUILDABLE (button));

  if (strcmp (id, "cutbutton") == 0)
    bobgui_text_buffer_cut_clipboard (buffer, clipboard, TRUE);
  else if (strcmp (id, "copybutton") == 0)
    bobgui_text_buffer_copy_clipboard (buffer, clipboard);
  else if (strcmp (id, "pastebutton") == 0)
    bobgui_text_buffer_paste_clipboard (buffer, clipboard, NULL, TRUE);
  else if (strcmp (id, "deletebutton") == 0)
    bobgui_text_buffer_delete_selection (buffer, TRUE, TRUE);
}

static void
clipboard_formats_notify (GdkClipboard *clipboard, GdkEvent *event, BobguiWidget *button)
{
  const char *id;
  gboolean has_text;

  id = bobgui_buildable_get_buildable_id (BOBGUI_BUILDABLE (button));
  has_text = gdk_content_formats_contain_gtype (gdk_clipboard_get_formats (clipboard), BOBGUI_TYPE_TEXT_BUFFER);

  if (strcmp (id, "pastebutton") == 0)
    bobgui_widget_set_sensitive (button, has_text);
}

static void
textbuffer_notify_selection (GObject *object, GParamSpec *pspec, BobguiWidget *button)
{
  const char *id;
  gboolean has_selection;

  id = bobgui_buildable_get_buildable_id (BOBGUI_BUILDABLE (button));
  has_selection = bobgui_text_buffer_get_has_selection (BOBGUI_TEXT_BUFFER (object));

  if (strcmp (id, "cutbutton") == 0 ||
      strcmp (id, "copybutton") == 0 ||
      strcmp (id, "deletebutton") == 0)
    bobgui_widget_set_sensitive (button, has_selection);
}

static gboolean
osd_frame_pressed (BobguiGestureClick *gesture,
                   int              press,
                   double           x,
                   double           y,
                   gpointer         data)
{
  BobguiWidget *frame = data;
  BobguiWidget *osd;
  gboolean visible;

  osd = g_object_get_data (G_OBJECT (frame), "osd");
  visible = bobgui_widget_get_visible (osd);
  bobgui_widget_set_visible (osd, !visible);

  return GDK_EVENT_STOP;
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static gboolean
page_combo_separator_func (BobguiTreeModel *model,
                           BobguiTreeIter  *iter,
                           gpointer      data)
{
  char *text;
  gboolean res;

  bobgui_tree_model_get (model, iter, 0, &text, -1);
  res = g_strcmp0 (text, "-") == 0;
  g_free (text);

  return res;
}
G_GNUC_END_IGNORE_DEPRECATIONS

static void
toggle_format (GSimpleAction *action,
               GVariant      *value,
               gpointer       user_data)
{
  BobguiTextView *text_view = user_data;
  BobguiTextIter start, end;
  const char *name;

  name = g_action_get_name (G_ACTION (action));

  g_simple_action_set_state (action, value);

  bobgui_text_buffer_get_selection_bounds (bobgui_text_view_get_buffer (text_view), &start, &end);
  if (g_variant_get_boolean (value))
    bobgui_text_buffer_apply_tag_by_name (bobgui_text_view_get_buffer (text_view), name, &start, &end);
  else
    bobgui_text_buffer_remove_tag_by_name (bobgui_text_view_get_buffer (text_view), name, &start, &end);
}

static GActionGroup *actions;

static void
text_changed (BobguiTextBuffer *buffer)
{
  GAction *bold;
  GAction *italic;
  GAction *underline;
  BobguiTextIter iter;
  BobguiTextTagTable *tags;
  BobguiTextTag *bold_tag, *italic_tag, *underline_tag;
  gboolean all_bold, all_italic, all_underline;
  BobguiTextIter start, end;
  gboolean has_selection;

  bold = g_action_map_lookup_action (G_ACTION_MAP (actions), "bold");
  italic = g_action_map_lookup_action (G_ACTION_MAP (actions), "italic");
  underline = g_action_map_lookup_action (G_ACTION_MAP (actions), "underline");

  has_selection = bobgui_text_buffer_get_selection_bounds (buffer, &start, &end);
  g_simple_action_set_enabled (G_SIMPLE_ACTION (bold), has_selection);
  g_simple_action_set_enabled (G_SIMPLE_ACTION (italic), has_selection);
  g_simple_action_set_enabled (G_SIMPLE_ACTION (underline), has_selection);
  if (!has_selection)
    return;

  tags = bobgui_text_buffer_get_tag_table (buffer);
  bold_tag = bobgui_text_tag_table_lookup (tags, "bold");
  italic_tag = bobgui_text_tag_table_lookup (tags, "italic");
  underline_tag = bobgui_text_tag_table_lookup (tags, "underline");
  all_bold = TRUE;
  all_italic = TRUE;
  all_underline = TRUE;
  bobgui_text_iter_assign (&iter, &start);
  while (!bobgui_text_iter_equal (&iter, &end))
    {
      all_bold &= bobgui_text_iter_has_tag (&iter, bold_tag);
      all_italic &= bobgui_text_iter_has_tag (&iter, italic_tag);
      all_underline &= bobgui_text_iter_has_tag (&iter, underline_tag);
      bobgui_text_iter_forward_char (&iter);
    }

  g_simple_action_set_state (G_SIMPLE_ACTION (bold), g_variant_new_boolean (all_bold));
  g_simple_action_set_state (G_SIMPLE_ACTION (italic), g_variant_new_boolean (all_italic));
  g_simple_action_set_state (G_SIMPLE_ACTION (underline), g_variant_new_boolean (all_underline));
}

static void
text_view_add_to_context_menu (BobguiTextView *text_view)
{
  GMenu *menu;
  GActionEntry entries[] = {
    { "bold", NULL, NULL, "false", toggle_format },
    { "italic", NULL, NULL, "false", toggle_format },
    { "underline", NULL, NULL, "false", toggle_format },
  };
  GMenuItem *item;
  GAction *action;

  actions = G_ACTION_GROUP (g_simple_action_group_new ());
  g_action_map_add_action_entries (G_ACTION_MAP (actions), entries, G_N_ELEMENTS (entries), text_view);

  action = g_action_map_lookup_action (G_ACTION_MAP (actions), "bold");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);
  action = g_action_map_lookup_action (G_ACTION_MAP (actions), "italic");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);
  action = g_action_map_lookup_action (G_ACTION_MAP (actions), "underline");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);

  bobgui_widget_insert_action_group (BOBGUI_WIDGET (text_view), "format", G_ACTION_GROUP (actions));

  menu = g_menu_new ();
  item = g_menu_item_new (_("Bold"), "format.bold");
  g_menu_item_set_attribute (item, "touch-icon", "s", "format-text-bold-symbolic");
  g_menu_append_item (G_MENU (menu), item);
  g_object_unref (item);
  item = g_menu_item_new (_("Italics"), "format.italic");
  g_menu_item_set_attribute (item, "touch-icon", "s", "format-text-italic-symbolic");
  g_menu_append_item (G_MENU (menu), item);
  g_object_unref (item);
  item = g_menu_item_new (_("Underline"), "format.underline");
  g_menu_item_set_attribute (item, "touch-icon", "s", "format-text-underline-symbolic");
  g_menu_append_item (G_MENU (menu), item);
  g_object_unref (item);

  bobgui_text_view_set_extra_menu (text_view, G_MENU_MODEL (menu));
  g_object_unref (menu);

  g_signal_connect (bobgui_text_view_get_buffer (text_view), "changed", G_CALLBACK (text_changed), NULL);
  g_signal_connect (bobgui_text_view_get_buffer (text_view), "mark-set", G_CALLBACK (text_changed), NULL);
}

static void
open_popover_text_changed (BobguiEntry *entry, GParamSpec *pspec, BobguiWidget *button)
{
  const char *text;

  text = bobgui_editable_get_text (BOBGUI_EDITABLE (entry));
  bobgui_widget_set_sensitive (button, strlen (text) > 0);
}

static gboolean
show_page_again (gpointer data)
{
  bobgui_widget_set_visible (BOBGUI_WIDGET (data), TRUE);
  return G_SOURCE_REMOVE;
}

static void
tab_close_cb (BobguiWidget *page)
{
  bobgui_widget_set_visible (page, FALSE);
  g_timeout_add (2500, show_page_again, page);
}

typedef struct _GTestPermission GTestPermission;
typedef struct _GTestPermissionClass GTestPermissionClass;

struct _GTestPermission
{
  GPermission parent;
};

struct _GTestPermissionClass
{
  GPermissionClass parent_class;
};

static GType g_test_permission_get_type (void);
G_DEFINE_TYPE (GTestPermission, g_test_permission, G_TYPE_PERMISSION)

static void
g_test_permission_init (GTestPermission *test)
{
  g_permission_impl_update (G_PERMISSION (test), TRUE, TRUE, TRUE);
}

static gboolean
update_allowed (GPermission *permission,
                gboolean     allowed)
{
  g_permission_impl_update (permission, allowed, TRUE, TRUE);

  return TRUE;
}

static gboolean
acquire (GPermission   *permission,
         GCancellable  *cancellable,
         GError       **error)
{
  return update_allowed (permission, TRUE);
}

static void
acquire_async (GPermission         *permission,
               GCancellable        *cancellable,
               GAsyncReadyCallback  callback,
               gpointer             user_data)
{
  GTask *task;

  task = g_task_new ((GObject*)permission, NULL, callback, user_data);
  g_task_return_boolean (task, update_allowed (permission, TRUE));
  g_object_unref (task);
}

static gboolean
acquire_finish (GPermission   *permission,
                GAsyncResult  *res,
                GError       **error)
{
  return g_task_propagate_boolean (G_TASK (res), error);
}

static gboolean
release (GPermission   *permission,
         GCancellable  *cancellable,
         GError       **error)
{
  return update_allowed (permission, FALSE);
}

static void
release_async (GPermission         *permission,
               GCancellable        *cancellable,
               GAsyncReadyCallback  callback,
               gpointer             user_data)
{
  GTask *task;

  task = g_task_new ((GObject*)permission, NULL, callback, user_data);
  g_task_return_boolean (task, update_allowed (permission, FALSE));
  g_object_unref (task);
}

static gboolean
release_finish (GPermission   *permission,
                GAsyncResult  *result,
                GError       **error)
{
  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
g_test_permission_class_init (GTestPermissionClass *class)
{
  GPermissionClass *permission_class = G_PERMISSION_CLASS (class);

  permission_class->acquire = acquire;
  permission_class->acquire_async = acquire_async;
  permission_class->acquire_finish = acquire_finish;

  permission_class->release = release;
  permission_class->release_async = release_async;
  permission_class->release_finish = release_finish;
}

static void
update_buttons (BobguiWidget *iv, BobguiIconSize size)
{
  BobguiWidget *button;

  button = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (iv), "increase_button"));
  bobgui_widget_set_sensitive (button, size != BOBGUI_ICON_SIZE_LARGE);
  button = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (iv), "decrease_button"));
  bobgui_widget_set_sensitive (button, size != BOBGUI_ICON_SIZE_NORMAL);
  button = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (iv), "reset_button"));
  bobgui_widget_set_sensitive (button, size != BOBGUI_ICON_SIZE_INHERIT);
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static void
increase_icon_size (BobguiWidget *iv)
{
  GList *cells;
  BobguiCellRendererPixbuf *cell;

  cells = bobgui_cell_layout_get_cells (BOBGUI_CELL_LAYOUT (iv));
  cell = cells->data;
  g_list_free (cells);

  g_object_set (cell, "icon-size", BOBGUI_ICON_SIZE_LARGE, NULL);

  update_buttons (iv, BOBGUI_ICON_SIZE_LARGE);

  bobgui_widget_queue_resize (iv);
}

static void
decrease_icon_size (BobguiWidget *iv)
{
  GList *cells;
  BobguiCellRendererPixbuf *cell;

  cells = bobgui_cell_layout_get_cells (BOBGUI_CELL_LAYOUT (iv));
  cell = cells->data;
  g_list_free (cells);

  g_object_set (cell, "icon-size", BOBGUI_ICON_SIZE_NORMAL, NULL);

  update_buttons (iv, BOBGUI_ICON_SIZE_NORMAL);

  bobgui_widget_queue_resize (iv);
}

static void
reset_icon_size (BobguiWidget *iv)
{
  GList *cells;
  BobguiCellRendererPixbuf *cell;

  cells = bobgui_cell_layout_get_cells (BOBGUI_CELL_LAYOUT (iv));
  cell = cells->data;
  g_list_free (cells);

  g_object_set (cell, "icon-size", BOBGUI_ICON_SIZE_INHERIT, NULL);

  update_buttons (iv, BOBGUI_ICON_SIZE_INHERIT);

  bobgui_widget_queue_resize (iv);
}
G_GNUC_END_IGNORE_DEPRECATIONS

static char *
scale_format_value_blank (BobguiScale *scale, double value, gpointer user_data)
{
  return g_strdup (" ");
}

static char *
scale_format_value (BobguiScale *scale, double value, gpointer user_data)
{
  return g_strdup_printf ("%0.*f", 1, value);
}

static void
adjustment3_value_changed (BobguiAdjustment *adj, BobguiProgressBar *pbar)
{
  double fraction;

  fraction = bobgui_adjustment_get_value (adj) / (bobgui_adjustment_get_upper (adj) - bobgui_adjustment_get_lower (adj));

  bobgui_progress_bar_set_fraction (pbar, fraction);
}

static void
clicked_cb (BobguiGesture *gesture,
            int         n_press,
            double      x,
            double      y,
            BobguiPopover *popover)
{
  GdkRectangle rect;

  rect.x = x;
  rect.y = y;
  rect.width = 1;
  rect.height = 1;
  bobgui_popover_set_pointing_to (popover, &rect);
  bobgui_popover_popup (popover);
}

static void
set_up_context_popover (BobguiWidget *widget,
                        GMenuModel *model)
{
  BobguiWidget *popover = bobgui_popover_menu_new_from_model (model);
  BobguiGesture *gesture;

  bobgui_widget_set_parent (popover, widget);
  bobgui_popover_set_has_arrow (BOBGUI_POPOVER (popover), FALSE);
  gesture = bobgui_gesture_click_new ();
  bobgui_event_controller_set_name (BOBGUI_EVENT_CONTROLLER (gesture), "widget-factory-context-click");
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture), GDK_BUTTON_SECONDARY);
  g_signal_connect (gesture, "pressed", G_CALLBACK (clicked_cb), popover);
  bobgui_widget_add_controller (widget, BOBGUI_EVENT_CONTROLLER (gesture));
}

static void
age_entry_changed (BobguiEntry   *entry,
                   GParamSpec *pspec,
                   gpointer    data)
{
  const char *text;
  guint64 age;
  GError *error = NULL;

  text = bobgui_editable_get_text (BOBGUI_EDITABLE (entry));

  if (strlen (text) > 0 &&
      !g_ascii_string_to_unsigned (text, 10, 16, 666, &age, &error))
    {
      bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (entry), error->message);
      bobgui_widget_add_css_class (BOBGUI_WIDGET (entry), "error");
      g_error_free (error);
    }
  else
    {
      bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (entry), "");
      bobgui_widget_remove_css_class (BOBGUI_WIDGET (entry), "error");
    }
}

static void
validate_more_details (BobguiEntry   *entry,
                       GParamSpec *pspec,
                       BobguiEntry   *details)
{
  if (strlen (bobgui_editable_get_text (BOBGUI_EDITABLE (entry))) > 0 &&
      strlen (bobgui_editable_get_text (BOBGUI_EDITABLE (details))) == 0)
    {
      bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (entry), "Must have details first");
      bobgui_widget_add_css_class (BOBGUI_WIDGET (entry), "error");
      bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (entry),
                                   BOBGUI_ACCESSIBLE_STATE_INVALID, BOBGUI_ACCESSIBLE_INVALID_TRUE,
                                   -1);
    }
  else
    {
      bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (entry), "");
      bobgui_widget_remove_css_class (BOBGUI_WIDGET (entry), "error");
      bobgui_accessible_reset_state (BOBGUI_ACCESSIBLE (entry),
                                  BOBGUI_ACCESSIBLE_STATE_INVALID);
    }
}

static gboolean
mode_switch_state_set (BobguiSwitch *sw, gboolean state)
{
  BobguiWidget *dialog = bobgui_widget_get_ancestor (BOBGUI_WIDGET (sw), BOBGUI_TYPE_DIALOG);
  BobguiWidget *scale = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (dialog), "level_scale"));
  BobguiWidget *label = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (dialog), "error_label"));

  if (!state ||
      (bobgui_range_get_value (BOBGUI_RANGE (scale)) > 50))
    {
      bobgui_widget_set_visible (label, FALSE);
      bobgui_switch_set_state (sw, state);
      bobgui_accessible_reset_state (BOBGUI_ACCESSIBLE (sw),
                                  BOBGUI_ACCESSIBLE_STATE_INVALID);
      bobgui_accessible_reset_relation (BOBGUI_ACCESSIBLE (sw),
                                     BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE);
    }
  else
    {
      bobgui_widget_set_visible (label, TRUE);
      bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (sw),
                                   BOBGUI_ACCESSIBLE_STATE_INVALID, BOBGUI_ACCESSIBLE_INVALID_TRUE,
                                   -1);
      bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (sw),
                                      BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE, label, NULL,
                                      -1);
    }

  return TRUE;
}

static void
level_scale_value_changed (BobguiRange *range)
{
  BobguiWidget *dialog = bobgui_widget_get_ancestor (BOBGUI_WIDGET (range), BOBGUI_TYPE_DIALOG);
  BobguiWidget *sw = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (dialog), "mode_switch"));
  BobguiWidget *label = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (dialog), "error_label"));

  if (bobgui_switch_get_active (BOBGUI_SWITCH (sw)) &&
      !bobgui_switch_get_state (BOBGUI_SWITCH (sw)) &&
      (bobgui_range_get_value (range) > 50))
    {
      bobgui_widget_set_visible (label, FALSE);
      bobgui_switch_set_state (BOBGUI_SWITCH (sw), TRUE);
      bobgui_accessible_reset_state (BOBGUI_ACCESSIBLE (sw),
                                  BOBGUI_ACCESSIBLE_STATE_INVALID);
      bobgui_accessible_reset_relation (BOBGUI_ACCESSIBLE (sw),
                                     BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE);
    }
  else if (bobgui_switch_get_state (BOBGUI_SWITCH (sw)) &&
          (bobgui_range_get_value (range) <= 50))
    {
      bobgui_switch_set_state (BOBGUI_SWITCH (sw), FALSE);
    }
}

static void
hide_widget (BobguiWidget *widget)
{
  bobgui_widget_set_visible (widget, FALSE);
}

static void
load_texture_thread (GTask *task,
                     gpointer source_object,
                     gpointer task_data,
                     GCancellable *cancellable)
{
  const char *resource_path = (const char *) task_data;
  GBytes *bytes;
  GdkTexture *texture;
  GError *error = NULL;

  bytes = g_resources_lookup_data (resource_path, 0, &error);
  if (!bytes)
    {
      g_task_return_error (task, error);
      return;
    }

  texture = gdk_texture_new_from_bytes (bytes, &error);
  g_bytes_unref (bytes);

  if (!texture)
    {
      g_task_return_error (task, error);
      return;
    }

  g_task_return_pointer (task, texture, g_object_unref);
}

static void
load_texture_done (GObject *source,
                   GAsyncResult *result,
                   gpointer data)
{
  BobguiWidget *picture = BOBGUI_WIDGET (source);
  GdkTexture *texture;
  GError *error = NULL;

  texture = g_task_propagate_pointer (G_TASK (result), &error);
  if (!texture)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
      return;
    }

  bobgui_picture_set_paintable (BOBGUI_PICTURE (picture), GDK_PAINTABLE (texture));
  g_object_unref (texture);
}

static void
load_texture_in_thread (BobguiWidget  *picture,
                        const char *resource_path)
{
  GTask *task = g_task_new (picture, NULL, load_texture_done, NULL);
  g_task_set_task_data (task, (gpointer)resource_path, NULL);
  g_task_run_in_thread (task, load_texture_thread);
  g_object_unref (task);
}

static GFile *
resource_file_new (const char *resource_path)
{
  char *uri;
  GFile *file;

  uri = g_strconcat ("resource://", resource_path, NULL);
  file = g_file_new_for_uri (uri);
  g_free (uri);

  return file;
}

static void
builder_add_symbolic (BobguiBuilder *builder,
                      const char *icon_name,
                      const char *resource_path)
{
  GFile *file;
  BobguiIconPaintable *paintable;

  file = resource_file_new (resource_path);
  paintable = bobgui_icon_paintable_new_for_file (file, 16, 1);

  bobgui_builder_expose_object (builder, icon_name, G_OBJECT (paintable));

  g_object_unref (paintable);
  g_object_unref (file);
}

static void
activate (GApplication *app)
{
  GList *list;
  BobguiBuilder *builder;
  BobguiBuilderScope *scope;
  BobguiWindow *window;
  BobguiWidget *widget;
  BobguiWidget *widget2;
  BobguiWidget *widget3;
  BobguiWidget *widget4;
  BobguiWidget *stack;
  BobguiWidget *dialog;
  BobguiAdjustment *adj;
  BobguiCssProvider *provider;
  GMenuModel *model;
  static GActionEntry win_entries[] = {
    { "dark", NULL, NULL, "false", change_dark_state },
    { "theme", NULL, "s", "'current'", change_theme_state },
    { "transition", NULL, NULL, "true", change_transition_state },
    { "search", activate_search, NULL, NULL, NULL },
    { "delete", activate_delete, NULL, NULL, NULL },
    { "busy", get_busy, NULL, NULL, NULL },
    { "fullscreen", NULL, NULL, "false", change_fullscreen },
    { "background", activate_background, NULL, NULL, NULL },
    { "open", activate_open, NULL, NULL, NULL },
    { "record", activate_record, NULL, NULL, NULL },
    { "lock", activate_lock, NULL, NULL, NULL },
    { "print", activate_print, NULL, NULL, NULL },
  };
  struct {
    const char *action_and_target;
    const char *accelerators[2];
  } accels[] = {
    { "app.about", { "F1", NULL } },
    { "app.shortcuts", { "<Control>question", NULL } },
    { "app.quit", { "<Control>q", NULL } },
    { "app.open-in", { "<Control>n", NULL } },
    { "win.dark", { "<Control>d", NULL } },
    { "win.search", { "<Control>s", NULL } },
    { "win.background", { "<Control>b", NULL } },
    { "win.open", { "<Control>o", NULL } },
    { "win.record", { "<Control>r", NULL } },
    { "win.lock", { "<Control>l", NULL } },
    { "win.fullscreen", { "F11", NULL } },
  };
  struct {
    const char *action_and_target;
    const char *accelerators[2];
  } late_accels[] = {
    { "app.cut", { "<Control>x", NULL } },
    { "app.copy", { "<Control>c", NULL } },
    { "app.paste", { "<Control>v", NULL } },
    { "win.delete", { "Delete", NULL } },
  };
  int i;
  GPermission *permission;
  GAction *action;
  GError *error = NULL;
  BobguiEventController *controller;

  g_type_ensure (my_text_view_get_type ());

  if ((list = bobgui_application_get_windows (BOBGUI_APPLICATION (app))) != NULL)
    {
      bobgui_window_present (BOBGUI_WINDOW (list->data));
      return;
    }

  provider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_resource (provider, "/org/bobgui/WidgetFactory4/widget-factory.css");
  bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                              BOBGUI_STYLE_PROVIDER (provider),
                                              BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);
  g_object_unref (provider);

  builder = bobgui_builder_new ();
  scope = bobgui_builder_cscope_new ();
  bobgui_builder_cscope_add_callback (scope, on_entry_icon_release);
  bobgui_builder_cscope_add_callback (scope, on_scale_button_value_changed);
  bobgui_builder_cscope_add_callback (scope, on_record_button_toggled);
  bobgui_builder_cscope_add_callback (scope, on_page_combo_changed);
  bobgui_builder_cscope_add_callback (scope, on_range_from_changed);
  bobgui_builder_cscope_add_callback (scope, on_range_to_changed);
  bobgui_builder_cscope_add_callback (scope, on_picture_drag_prepare);
  bobgui_builder_cscope_add_callback (scope, on_picture_drop);
  bobgui_builder_cscope_add_callback (scope, tab_close_cb);
  bobgui_builder_cscope_add_callback (scope, increase_icon_size);
  bobgui_builder_cscope_add_callback (scope, decrease_icon_size);
  bobgui_builder_cscope_add_callback (scope, osd_frame_pressed);
  bobgui_builder_cscope_add_callback (scope, age_entry_changed);
  bobgui_builder_cscope_add_callback (scope, validate_more_details);
  bobgui_builder_cscope_add_callback (scope, mode_switch_state_set);
  bobgui_builder_cscope_add_callback (scope, level_scale_value_changed);
  bobgui_builder_cscope_add_callback (scope, transition_speed_changed);
  bobgui_builder_cscope_add_callback (scope, reset_icon_size);
  bobgui_builder_set_scope (builder, scope);

  builder_add_symbolic (builder, "open-menu-symbolic", "/org/bobgui/libbobgui/icons/open-menu-symbolic.svg");
  builder_add_symbolic (builder, "view-refresh-symbolic", "/org/bobgui/libbobgui/icons/view-refresh-symbolic.svg");
  builder_add_symbolic (builder, "window-close-symbolic", "/org/bobgui/libbobgui/icons/window-close-symbolic.svg");
  builder_add_symbolic (builder, "emblem-system-symbolic", "/org/bobgui/libbobgui/icons/emblem-system-symbolic.svg");
  builder_add_symbolic (builder, "object-select-symbolic", "/org/bobgui/libbobgui/icons/object-select-symbolic.svg");
  builder_add_symbolic (builder, "appointment-soon-symbolic", "/org/bobgui/WidgetFactory4/icons/scalable/status/appointment-soon-symbolic.svg");
  builder_add_symbolic (builder, "document-new-symbolic", "/org/bobgui/WidgetFactory4/icons/scalable/actions/document-new-symbolic.svg");
  builder_add_symbolic (builder, "document-save-symbolic", "/org/bobgui/libbobgui/icons/document-save-symbolic.svg");
  builder_add_symbolic (builder, "edit-find-symbolic", "/org/bobgui/libbobgui/icons/edit-find-symbolic.svg");
  builder_add_symbolic (builder, "insert-image-symbolic", "/org/bobgui/libbobgui/icons/insert-image-symbolic.svg");
  builder_add_symbolic (builder, "zoom-out-symbolic", "/org/bobgui/WidgetFactory4/icons/scalable/actions/zoom-out-symbolic.svg");
  builder_add_symbolic (builder, "zoom-in-symbolic", "/org/bobgui/WidgetFactory4/icons/scalable/actions/zoom-in-symbolic.svg");
  builder_add_symbolic (builder, "zoom-original-symbolic", "/org/bobgui/WidgetFactory4/icons/scalable/actions/zoom-original-symbolic.svg");
  builder_add_symbolic (builder, "media-record-symbolic", "/org/bobgui/libbobgui/icons/media-record-symbolic.svg");
  builder_add_symbolic (builder, "view-grid-symbolic", "/org/bobgui/libbobgui/icons/view-grid-symbolic.svg");
  builder_add_symbolic (builder, "view-list-symbolic", "/org/bobgui/libbobgui/icons/view-list-symbolic.svg");
  builder_add_symbolic (builder, "view-more-symbolic", "/org/bobgui/libbobgui/icons/view-more-symbolic.svg");
  builder_add_symbolic (builder, "document-open-symbolic", "/org/bobgui/libbobgui/icons/document-open-symbolic.svg");
  builder_add_symbolic (builder, "send-to-symbolic", "/org/bobgui/WidgetFactory4/icons/scalable/actions/send-to-symbolic.svg");
  builder_add_symbolic (builder, "view-fullscreen-symbolic", "/org/bobgui/WidgetFactory4/icons/scalable/actions/view-fullscreen-symbolic.svg");
  builder_add_symbolic (builder, "start-new-symbolic", "/org/bobgui/WidgetFactory4/icons/scalable/actions/star-new-symbolic.svg");
  builder_add_symbolic (builder, "edit-cut-symbolic", "/org/bobgui/libbobgui/icons/edit-cut-symbolic.svg");
  builder_add_symbolic (builder, "edit-copy-symbolic", "/org/bobgui/libbobgui/icons/edit-copy-symbolic.svg");
  builder_add_symbolic (builder, "edit-paste-symbolic", "/org/bobgui/libbobgui/icons/edit-paste-symbolic.svg");
  builder_add_symbolic (builder, "edit-delete-symbolic", "/org/bobgui/libbobgui/icons/edit-delete-symbolic.svg");
  builder_add_symbolic (builder, "go-previous-symbolic", "/org/bobgui/libbobgui/icons/go-previous-symbolic.svg");
  builder_add_symbolic (builder, "go-next-symbolic", "/org/bobgui/libbobgui/icons/go-next-symbolic.svg");
  builder_add_symbolic (builder, "emblem-important-symbolic", "/org/bobgui/libbobgui/icons/emblem-important-symbolic.svg");

  g_object_unref (scope);
  if (!bobgui_builder_add_from_resource (builder, "/org/bobgui/WidgetFactory4/widget-factory.ui", &error))
    {
      g_critical ("%s", error->message);
      g_clear_error (&error);
    }

  window = (BobguiWindow *)bobgui_builder_get_object (builder, "window");

  load_texture_in_thread ((BobguiWidget *)bobgui_builder_get_object (builder, "notebook_sunset"),
                          "/org/bobgui/WidgetFactory4/sunset.jpg");
  load_texture_in_thread ((BobguiWidget *)bobgui_builder_get_object (builder, "notebook_nyc"),
                          "/org/bobgui/WidgetFactory4/nyc.jpg");
  load_texture_in_thread ((BobguiWidget *)bobgui_builder_get_object (builder, "notebook_beach"),
                          "/org/bobgui/WidgetFactory4/beach.jpg");

  if (g_strcmp0 (PROFILE, "devel") == 0)
    bobgui_widget_add_css_class (BOBGUI_WIDGET (window), "devel");

  bobgui_application_add_window (BOBGUI_APPLICATION (app), window);
  g_action_map_add_action_entries (G_ACTION_MAP (window),
                                   win_entries, G_N_ELEMENTS (win_entries),
                                   window);

  controller = bobgui_shortcut_controller_new ();
  bobgui_event_controller_set_static_name (controller, "widget-factory-late-accels");
  bobgui_event_controller_set_propagation_phase (controller, BOBGUI_PHASE_BUBBLE);

  for (i = 0; i < G_N_ELEMENTS (late_accels); i++)
    {
      guint key;
      GdkModifierType mods;
      BobguiShortcutTrigger *trigger;
      BobguiShortcutAction *ac;

      bobgui_accelerator_parse (late_accels[i].accelerators[0], &key, &mods);
      trigger = bobgui_keyval_trigger_new (key, mods);
      ac = bobgui_named_action_new (late_accels[i].action_and_target);
      bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (controller),
                                            bobgui_shortcut_new (trigger, ac));
    }
  bobgui_widget_add_controller (BOBGUI_WIDGET (window), controller);

  for (i = 0; i < G_N_ELEMENTS (accels); i++)
    bobgui_application_set_accels_for_action (BOBGUI_APPLICATION (app), accels[i].action_and_target, accels[i].accelerators);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "statusbar");
  bobgui_statusbar_push (BOBGUI_STATUSBAR (widget), 0, "All systems are operating normally.");
  action = G_ACTION (g_property_action_new ("statusbar", widget, "visible"));
  g_action_map_add_action (G_ACTION_MAP (window), action);
  g_object_unref (G_OBJECT (action));
G_GNUC_END_IGNORE_DEPRECATIONS

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "toolbar");
  action = G_ACTION (g_property_action_new ("toolbar", widget, "visible"));
  g_action_map_add_action (G_ACTION_MAP (window), action);
  g_object_unref (G_OBJECT (action));

  adj = (BobguiAdjustment *)bobgui_builder_get_object (builder, "adjustment1");

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "progressbar3");
  g_signal_connect (adj, "value-changed", G_CALLBACK (update_pulse_time), widget);
  update_pulse_time (adj, widget);

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "entry1");
  g_signal_connect (adj, "value-changed", G_CALLBACK (update_pulse_time), widget);
  update_pulse_time (adj, widget);

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "page2reset");
  adj = (BobguiAdjustment *) bobgui_builder_get_object (builder, "adjustment2");
  g_signal_connect (widget, "clicked", G_CALLBACK (spin_value_reset), adj);

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "page2dismiss");
  g_signal_connect (widget, "clicked", G_CALLBACK (dismiss), NULL);

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "page2note");
  adj = (BobguiAdjustment *) bobgui_builder_get_object (builder, "adjustment2");
  g_signal_connect (adj, "value-changed", G_CALLBACK (spin_value_changed), widget);

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "listbox");
  g_signal_connect (widget, "row-activated", G_CALLBACK (row_activated), NULL);

  widget2 = (BobguiWidget *)bobgui_builder_get_object (builder, "listboxrow1switch");
  g_signal_connect (widget2, "notify::active", G_CALLBACK (toggle_selection_mode), widget);

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "listboxrow3");
  widget2 = (BobguiWidget *)bobgui_builder_get_object (builder, "listboxrow3image");
  g_object_set_data (G_OBJECT (widget), "image", widget2);

  widget2 = (BobguiWidget *)bobgui_builder_get_object (builder, "info_dialog");
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "listboxrow7");
  g_object_set_data (G_OBJECT (widget), "dialog", widget2);
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "listboxrow8");
  g_object_set_data (G_OBJECT (widget), "dialog", widget2);

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "listboxrow5button");
  widget2 = (BobguiWidget *)bobgui_builder_get_object (builder, "action_dialog");
  g_signal_connect_swapped (widget, "clicked", G_CALLBACK (bobgui_window_present), widget2);

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "toolbar");
  g_object_set_data (G_OBJECT (window), "toolbar", widget);

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "searchbar");
  g_object_set_data (G_OBJECT (window), "searchbar", widget);

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "infobar");
  g_signal_connect (widget, "response", G_CALLBACK (info_bar_response), NULL);
  g_object_set_data (G_OBJECT (window), "infobar", widget);

  dialog = (BobguiWidget *)bobgui_builder_get_object (builder, "info_dialog");
  g_signal_connect (dialog, "response", G_CALLBACK (close_dialog), NULL);
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "info_dialog_button");
  g_signal_connect (widget, "clicked", G_CALLBACK (show_dialog), dialog);

  dialog = (BobguiWidget *)bobgui_builder_get_object (builder, "action_dialog");
  g_signal_connect (dialog, "response", G_CALLBACK (close_dialog), NULL);
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "action_dialog_button");
  g_signal_connect (widget, "clicked", G_CALLBACK (show_dialog), dialog);

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "act_action_dialog");
  stack = (BobguiWidget *)bobgui_builder_get_object (builder, "toplevel_stack");
  g_signal_connect (widget, "clicked", G_CALLBACK (action_dialog_button_clicked), stack);
  g_signal_connect (stack, "notify::visible-child-name", G_CALLBACK (page_changed_cb), NULL);
  page_changed_cb (stack, NULL, NULL);

  page_stack = stack;

  dialog = (BobguiWidget *)bobgui_builder_get_object (builder, "preference_dialog");
  g_signal_connect (dialog, "response", G_CALLBACK (close_dialog), NULL);
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "preference_dialog_button");
  g_signal_connect (widget, "clicked", G_CALLBACK (show_dialog), dialog);
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "circular_button");
  g_signal_connect (widget, "clicked", G_CALLBACK (show_dialog), dialog);

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "level_scale");
  g_object_set_data (G_OBJECT (dialog), "level_scale", widget);
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "mode_switch");
  g_object_set_data (G_OBJECT (dialog), "mode_switch", widget);
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "error_label");
  g_object_set_data (G_OBJECT (dialog), "error_label", widget);

  dialog = (BobguiWidget *)bobgui_builder_get_object (builder, "selection_dialog");
  g_object_set_data (G_OBJECT (window), "selection_dialog", dialog);
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "text3");
  g_signal_connect (dialog, "response", G_CALLBACK (close_selection_dialog), widget);
  widget2 = (BobguiWidget *)bobgui_builder_get_object (builder, "opacity");
  my_text_view_set_adjustment ((MyTextView *)widget, bobgui_range_get_adjustment (BOBGUI_RANGE (widget2)));
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "selection_dialog_button");
  g_signal_connect (widget, "clicked", G_CALLBACK (show_dialog), dialog);

  widget2 = (BobguiWidget *)bobgui_builder_get_object (builder, "selection_flowbox");
  g_object_set_data (G_OBJECT (window), "selection_flowbox", widget2);
  g_signal_connect_swapped (widget, "clicked", G_CALLBACK (populate_flowbox), widget2);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "charletree");
  populate_model ((BobguiTreeStore *)bobgui_tree_view_get_model (BOBGUI_TREE_VIEW (widget)));
  bobgui_tree_view_set_row_separator_func (BOBGUI_TREE_VIEW (widget), row_separator_func, NULL, NULL);
  bobgui_tree_view_expand_all (BOBGUI_TREE_VIEW (widget));
G_GNUC_END_IGNORE_DEPRECATIONS

  widget = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "munsell"));
  widget2 = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "cchooser"));

  populate_colors (widget, widget2);
  g_signal_connect (widget2, "notify::rgba", G_CALLBACK (rgba_changed), widget);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "page_combo");
  bobgui_combo_box_set_row_separator_func (BOBGUI_COMBO_BOX (widget), page_combo_separator_func, NULL, NULL);
  widget2 = (BobguiWidget *)bobgui_builder_get_object (builder, "range_from_spin");
  widget3 = (BobguiWidget *)bobgui_builder_get_object (builder, "range_to_spin");
  widget4 = (BobguiWidget *)bobgui_builder_get_object (builder, "print_button");
  g_object_set_data (G_OBJECT (widget), "range_from_spin", widget2);
  g_object_set_data (G_OBJECT (widget3), "range_from_spin", widget2);
  g_object_set_data (G_OBJECT (widget), "range_to_spin", widget3);
  g_object_set_data (G_OBJECT (widget2), "range_to_spin", widget3);
  g_object_set_data (G_OBJECT (widget), "print_button", widget4);
G_GNUC_END_IGNORE_DEPRECATIONS

  widget2 = (BobguiWidget *)bobgui_builder_get_object (builder, "tooltextview");

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "toolbutton1");
  g_signal_connect (widget, "clicked", G_CALLBACK (handle_insert), widget2);
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "toolbutton2");
  g_signal_connect (widget, "clicked", G_CALLBACK (handle_insert), widget2);
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "toolbutton3");
  g_signal_connect (widget, "clicked", G_CALLBACK (handle_insert), widget2);
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "toolbutton4");
  g_signal_connect (widget, "clicked", G_CALLBACK (handle_insert), widget2);
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "cutbutton");
  g_signal_connect (widget, "clicked", G_CALLBACK (handle_cutcopypaste), widget2);
  g_signal_connect (bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (widget2)), "notify::has-selection",
                    G_CALLBACK (textbuffer_notify_selection), widget);
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "copybutton");
  g_signal_connect (widget, "clicked", G_CALLBACK (handle_cutcopypaste), widget2);
  g_signal_connect (bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (widget2)), "notify::has-selection",
                    G_CALLBACK (textbuffer_notify_selection), widget);
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "deletebutton");
  g_signal_connect (widget, "clicked", G_CALLBACK (handle_cutcopypaste), widget2);
  g_signal_connect (bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (widget2)), "notify::has-selection",
                    G_CALLBACK (textbuffer_notify_selection), widget);
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "pastebutton");
  g_signal_connect (widget, "clicked", G_CALLBACK (handle_cutcopypaste), widget2);
  g_signal_connect_object (bobgui_widget_get_clipboard (widget2), "notify::formats",
                           G_CALLBACK (clipboard_formats_notify), widget, 0);

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "osd_frame");
  widget2 = (BobguiWidget *)bobgui_builder_get_object (builder, "totem_like_osd");
  g_object_set_data (G_OBJECT (widget), "osd", widget2);

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "textview1");
  text_view_add_to_context_menu (BOBGUI_TEXT_VIEW (widget));

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "open_popover");
  widget2 = (BobguiWidget *)bobgui_builder_get_object (builder, "open_popover_entry");
  widget3 = (BobguiWidget *)bobgui_builder_get_object (builder, "open_popover_button");
  bobgui_popover_set_default_widget (BOBGUI_POPOVER (widget), widget3);
  g_signal_connect (widget2, "notify::text", G_CALLBACK (open_popover_text_changed), widget3);
  g_signal_connect_swapped (widget3, "clicked", G_CALLBACK (hide_widget), widget);
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "open_menubutton");
  g_object_set_data (G_OBJECT (window), "open_menubutton", widget);
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "record_button");
  g_object_set_data (G_OBJECT (window), "record_button", widget);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "lockbox");
  widget2 = (BobguiWidget *)bobgui_builder_get_object (builder, "lockbutton");
  g_object_set_data (G_OBJECT (window), "lockbutton", widget2);
  permission = g_object_new (g_test_permission_get_type (), NULL);
  g_object_bind_property (permission, "allowed",
                          widget, "sensitive",
                          G_BINDING_SYNC_CREATE);
  action = g_action_map_lookup_action (G_ACTION_MAP (window), "open");
  g_object_bind_property (permission, "allowed",
                          action, "enabled",
                          G_BINDING_SYNC_CREATE);
  action = g_action_map_lookup_action (G_ACTION_MAP (window), "record");
  g_object_bind_property (permission, "allowed",
                          action, "enabled",
                          G_BINDING_SYNC_CREATE);
  bobgui_lock_button_set_permission (BOBGUI_LOCK_BUTTON (widget2), permission);
  g_object_unref (permission);
G_GNUC_END_IGNORE_DEPRECATIONS

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "iconview1");
  widget2 = (BobguiWidget *)bobgui_builder_get_object (builder, "increase_button");
  g_object_set_data (G_OBJECT (widget), "increase_button", widget2);
  widget2 = (BobguiWidget *)bobgui_builder_get_object (builder, "decrease_button");
  g_object_set_data (G_OBJECT (widget), "decrease_button", widget2);
  widget2 = (BobguiWidget *)bobgui_builder_get_object (builder, "reset_button");
  g_object_set_data (G_OBJECT (widget), "reset_button", widget2);
  reset_icon_size (widget);

  adj = (BobguiAdjustment *)bobgui_builder_get_object (builder, "adjustment3");
  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "progressbar1");
  widget2 = (BobguiWidget *)bobgui_builder_get_object (builder, "progressbar2");
  g_signal_connect (adj, "value-changed", G_CALLBACK (adjustment3_value_changed), widget);
  g_signal_connect (adj, "value-changed", G_CALLBACK (adjustment3_value_changed), widget2);

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "extra_info_entry");
  g_timeout_add (100, (GSourceFunc)pulse_it, widget);

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "scale3");
  bobgui_scale_set_format_value_func (BOBGUI_SCALE (widget), scale_format_value, NULL, NULL);

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "scale4");
  bobgui_scale_set_format_value_func (BOBGUI_SCALE (widget), scale_format_value_blank, NULL, NULL);

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "box_for_context");
  model = (GMenuModel *)bobgui_builder_get_object (builder, "new_style_context_menu_model");
  set_up_context_popover (widget, model);

  widget = (BobguiWidget *)bobgui_builder_get_object (builder, "video");

  GFile *file;
  GInputStream *input_stream;
  BobguiMediaStream *media_stream;

  file = g_file_new_for_uri ("resource:///org/bobgui/WidgetFactory4/bobgui-logo.webm");
  input_stream = G_INPUT_STREAM (g_file_read (file, NULL, NULL));
  media_stream = bobgui_media_file_new_for_input_stream (input_stream);
  bobgui_video_set_media_stream (BOBGUI_VIDEO (widget), media_stream);
  g_object_unref (media_stream);
  g_object_unref (input_stream);
  g_object_unref (file);

  bobgui_window_present (window);

  g_object_unref (builder);
}

static void
activate_action (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  g_print ("Activate action %s\n", g_action_get_name (G_ACTION (action)));
}

static void
select_action (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  g_print ("Select action %s value %s\n",
           g_action_get_name (G_ACTION (action)),
           g_variant_get_string (parameter, NULL));

  g_simple_action_set_state (action, parameter);
}

static void
toggle_action (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  GVariant *state = g_action_get_state (G_ACTION (action));

  g_print ("Toggle action %s to %s\n",
           g_action_get_name (G_ACTION (action)),
           g_variant_get_boolean (state) ? "false" : "true");

  g_simple_action_set_state (action,
                             g_variant_new_boolean (!g_variant_get_boolean (state)));
}

static gboolean
quit_timeout (gpointer data)
{
  exit (0);
  return G_SOURCE_REMOVE;
}

G_MODULE_EXPORT
int
main (int argc, char *argv[])
{
  BobguiApplication *app;
  GAction *action;
  static GActionEntry app_entries[] = {
    { "about", activate_about, NULL, NULL, NULL },
    { "shortcuts", activate_shortcuts_window, NULL, NULL, NULL },
    { "quit", activate_quit, NULL, NULL, NULL },
    { "inspector", activate_inspector, NULL, NULL, NULL },
    { "main", NULL, "s", "'steak'", NULL },
    { "wine", NULL, NULL, "false", NULL },
    { "beer", NULL, NULL, "false", NULL },
    { "water", NULL, NULL, "true", NULL },
    { "dessert", NULL, "s", "'bars'", NULL },
    { "pay", NULL, "s", NULL, NULL },
    { "share", activate_action, NULL, NULL, NULL },
    { "labels", activate_action, NULL, NULL, NULL },
    { "new", activate_action, NULL, NULL, NULL },
    { "open", activate_open_file, NULL, NULL, NULL },
    { "open-in", activate_action, NULL, NULL, NULL },
    { "open-tab", activate_action, NULL, NULL, NULL },
    { "open-window", activate_action, NULL, NULL, NULL },
    { "save", activate_action, NULL, NULL, NULL },
    { "save-as", activate_action, NULL, NULL, NULL },
    { "cut", activate_action, NULL, NULL, NULL },
    { "copy", activate_action, NULL, NULL, NULL },
    { "paste", activate_action, NULL, NULL, NULL },
    { "pin", toggle_action, NULL, "true", NULL },
    { "size", select_action, "s", "'medium'", NULL },
    { "berk", toggle_action, NULL, "true", NULL },
    { "broni", toggle_action, NULL, "true", NULL },
    { "drutt", toggle_action, NULL, "true", NULL },
    { "upstairs", toggle_action, NULL, "true", NULL },
    { "option-a", activate_action, NULL, NULL, NULL },
    { "option-b", activate_action, NULL, NULL, NULL },
    { "option-c", activate_action, NULL, NULL, NULL },
    { "option-d", activate_action, NULL, NULL, NULL },
    { "check-on", NULL, NULL, "true", NULL },
    { "check-off", NULL, NULL, "false", NULL },
    { "radio-x", NULL, "s", "'x'", NULL },
    { "check-on-disabled", NULL, NULL, "true", NULL },
    { "check-off-disabled", NULL, NULL, "false", NULL },
    { "radio-x-disabled", NULL, "s", "'x'", NULL },
  };
  int status;
  char version[80];

  app = bobgui_application_new ("org.bobgui.WidgetFactory4", G_APPLICATION_NON_UNIQUE);

  g_snprintf (version, sizeof (version), "%s%s%s\n",
              PACKAGE_VERSION,
              g_strcmp0 (PROFILE, "devel") == 0 ? "-" : "",
              g_strcmp0 (PROFILE, "devel") == 0 ? VCS_TAG : "");
  g_application_set_version (G_APPLICATION (app), version);

  g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   app);
  action = g_action_map_lookup_action (G_ACTION_MAP (app), "wine");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);
  action = g_action_map_lookup_action (G_ACTION_MAP (app), "check-on-disabled");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);
  action = g_action_map_lookup_action (G_ACTION_MAP (app), "check-off-disabled");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);
  action = g_action_map_lookup_action (G_ACTION_MAP (app), "radio-x-disabled");
  g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);

  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  if (g_getenv ("BOBGUI_DEBUG_AUTO_QUIT"))
    g_timeout_add (500, quit_timeout, NULL);

  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
