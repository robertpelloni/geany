/*
 * Copyright (c) 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#include "window.h"
#include "css-editor.h"

#include "bobguialertdialog.h"
#include "bobguicssprovider.h"
#include "bobguifiledialog.h"
#include "bobguilabel.h"
#include "bobguistyleproviderprivate.h"
#include "bobguitextiter.h"
#include "bobguitextview.h"
#include "bobguitogglebutton.h"
#include "bobguiswitch.h"
#include "bobguitooltip.h"
#include "bobguisettings.h"
#include "bobguidropdown.h"

#include "bobgui/css/bobguicss.h"

struct _BobguiInspectorCssEditorPrivate
{
  BobguiWidget *view;
  BobguiTextBuffer *text;
  GdkDisplay *display;
  BobguiCssProvider *provider;
  BobguiSwitch *enable_switch;
  BobguiDropDown *color_scheme;
  BobguiDropDown *contrast;
  BobguiDropDown *reduced_motion;
  guint timeout;
  GList *errors;
  gboolean show_deprecations;
};

typedef struct {
  GError *error;
  BobguiTextIter start;
  BobguiTextIter end;
} CssError;

static void
css_error_free (gpointer data)
{
  CssError *error = data;
  g_error_free (error->error);
  g_free (error);
}

static gboolean
query_tooltip_cb (BobguiWidget             *widget,
                  int                    x,
                  int                    y,
                  gboolean               keyboard_tip,
                  BobguiTooltip            *tooltip,
                  BobguiInspectorCssEditor *ce)
{
  BobguiTextIter iter;
  GList *l;

  if (keyboard_tip)
    {
      int offset;

      g_object_get (ce->priv->text, "cursor-position", &offset, NULL);
      bobgui_text_buffer_get_iter_at_offset (ce->priv->text, &iter, offset);
    }
  else
    {
      int bx, by, trailing;

      bobgui_text_view_window_to_buffer_coords (BOBGUI_TEXT_VIEW (ce->priv->view), BOBGUI_TEXT_WINDOW_TEXT,
                                             x, y, &bx, &by);
      bobgui_text_view_get_iter_at_position (BOBGUI_TEXT_VIEW (ce->priv->view), &iter, &trailing, bx, by);
    }

  for (l = ce->priv->errors; l; l = l->next)
    {
      CssError *css_error = l->data;

      if (g_error_matches (css_error->error,
                           BOBGUI_CSS_PARSER_WARNING,
                           BOBGUI_CSS_PARSER_WARNING_DEPRECATED) &&
          !ce->priv->show_deprecations)
        continue;

      if (bobgui_text_iter_in_range (&iter, &css_error->start, &css_error->end))
        {
          bobgui_tooltip_set_text (tooltip, css_error->error->message);
          return TRUE;
        }
    }

  return FALSE;
}

G_DEFINE_TYPE_WITH_PRIVATE (BobguiInspectorCssEditor, bobgui_inspector_css_editor, BOBGUI_TYPE_BOX)

static char *
get_autosave_path (void)
{
  return g_build_filename (g_get_user_cache_dir (), "bobgui-4.0", "inspector-css-autosave", NULL);
}

static void
set_initial_text (BobguiInspectorCssEditor *ce)
{
  char *initial_text = NULL;
  char *autosave_file = NULL;
  gsize len;

  autosave_file = get_autosave_path ();

  if (g_file_get_contents (autosave_file, &initial_text, &len, NULL))
    bobgui_switch_set_active (BOBGUI_SWITCH (ce->priv->enable_switch), FALSE);
  else
    initial_text = g_strconcat ("/*\n",
                                _("You can type here any CSS rule recognized by BOBGUI."), "\n",
                                _("You can temporarily disable this custom CSS by toggling the switch above."), "\n\n",
                                _("Changes are applied instantly and globally, for the whole application."), "\n",
                                "*/\n\n", NULL);
  bobgui_text_buffer_set_text (BOBGUI_TEXT_BUFFER (ce->priv->text), initial_text, -1);
  g_free (initial_text);
  g_free (autosave_file);
}

static void
autosave_contents (BobguiInspectorCssEditor *ce)
{
  char *autosave_file = NULL;
  char *dir = NULL;
  char *contents;
  BobguiTextIter start, end;

  bobgui_text_buffer_get_bounds (BOBGUI_TEXT_BUFFER (ce->priv->text), &start, &end);
  contents = bobgui_text_buffer_get_text (BOBGUI_TEXT_BUFFER (ce->priv->text), &start, &end, TRUE);
  autosave_file = get_autosave_path ();
  dir = g_path_get_dirname (autosave_file);
  g_mkdir_with_parents (dir, 0755);
  g_file_set_contents (autosave_file, contents, -1, NULL);

  g_free (dir);
  g_free (autosave_file);
  g_free (contents);
}

static void
enable_switch_changed (BobguiSwitch             *sw,
                       GParamSpec            *pspec,
                       BobguiInspectorCssEditor *ce)
{
  if (!ce->priv->display)
    return;

  if (bobgui_switch_get_active (sw))
    bobgui_style_context_add_provider_for_display (ce->priv->display,
                                                BOBGUI_STYLE_PROVIDER (ce->priv->provider),
                                                BOBGUI_STYLE_PROVIDER_PRIORITY_INSPECTOR);
  else
    bobgui_style_context_remove_provider_for_display (ce->priv->display,
                                                   BOBGUI_STYLE_PROVIDER (ce->priv->provider));
}

static void
toggle_deprecations (BobguiToggleButton       *button,
                     BobguiInspectorCssEditor *ce)
{
  BobguiTextTagTable *tags;
  BobguiTextTag *tag;
  PangoUnderline underline;

  if (!ce->priv->display)
    return;

  ce->priv->show_deprecations = bobgui_toggle_button_get_active (button);

  tags = bobgui_text_buffer_get_tag_table (BOBGUI_TEXT_BUFFER (ce->priv->text));
  tag = bobgui_text_tag_table_lookup (tags, "deprecation");
  if (ce->priv->show_deprecations)
    underline = PANGO_UNDERLINE_SINGLE;
  else
    underline = PANGO_UNDERLINE_NONE;

  g_object_set (tag, "underline", underline, NULL);
}

static char *
get_current_text (BobguiTextBuffer *buffer)
{
  BobguiTextIter start, end;

  bobgui_text_buffer_get_start_iter (buffer, &start);
  bobgui_text_buffer_get_end_iter (buffer, &end);
  bobgui_text_buffer_remove_all_tags (buffer, &start, &end);

  return bobgui_text_buffer_get_text (buffer, &start, &end, FALSE);
}

static void
save_to_file (BobguiInspectorCssEditor *ce,
              GFile                 *file)
{
  GError *error = NULL;
  char *text;

  text = get_current_text (ce->priv->text);

  g_file_replace_contents (file, text, strlen (text),
                           NULL,
                           FALSE,
                           G_FILE_CREATE_NONE,
                           NULL,
                           NULL,
                           &error);

  if (error != NULL)
    {
      BobguiAlertDialog *alert;

      alert = bobgui_alert_dialog_new (_("Saving CSS failed"));
      bobgui_alert_dialog_set_detail (alert, error->message);
      bobgui_alert_dialog_show (alert, BOBGUI_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (ce))));
      g_object_unref (alert);
      g_error_free (error);
    }

  g_free (text);
}

static void
save_response (GObject *source,
               GAsyncResult *result,
               gpointer data)
{
  BobguiFileDialog *dialog = BOBGUI_FILE_DIALOG (source);
  BobguiInspectorCssEditor *ce = data;
  GError *error = NULL;
  GFile *file;

  file = bobgui_file_dialog_save_finish (dialog, result, &error);
  if (file)
    {
      save_to_file (ce, file);
      g_object_unref (file);
    }
  else
    {
      g_print ("Error saving css: %s\n", error->message);
      g_error_free (error);
    }
}

static void
save_clicked (BobguiButton             *button,
              BobguiInspectorCssEditor *ce)
{
  BobguiFileDialog *dialog;

  dialog = bobgui_file_dialog_new ();
  bobgui_file_dialog_set_initial_name (dialog, "custom.css");
  bobgui_file_dialog_save (dialog,
                        BOBGUI_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (ce))),
                        NULL,
                        save_response, ce);
  g_object_unref (dialog);
}

static void
settings_changed (BobguiDropDown           *dropdown,
                  GParamSpec            *pspec,
                  BobguiInspectorCssEditor *ce)
{
  BobguiInterfaceColorScheme color_scheme = bobgui_drop_down_get_selected (ce->priv->color_scheme) + 1;
  BobguiInterfaceColorScheme system_color_scheme;
  BobguiInterfaceContrast contrast = bobgui_drop_down_get_selected (ce->priv->contrast) + 1;
  BobguiInterfaceContrast system_contrast;
  BobguiReducedMotion reduced_motion = bobgui_drop_down_get_selected (ce->priv->reduced_motion);
  BobguiReducedMotion system_reduced_motion;

  g_object_get (bobgui_settings_get_for_display (ce->priv->display),
                "bobgui-interface-color-scheme", &system_color_scheme,
                "bobgui-interface-contrast", &system_contrast,
                "bobgui-interface-reduced-motion", &system_reduced_motion,
                NULL);

  if (color_scheme == BOBGUI_INTERFACE_COLOR_SCHEME_DEFAULT)
    color_scheme = system_color_scheme;

  if (contrast == BOBGUI_INTERFACE_CONTRAST_NO_PREFERENCE)
    contrast = system_contrast;

  if (reduced_motion == BOBGUI_REDUCED_MOTION_NO_PREFERENCE)
    reduced_motion = system_reduced_motion;

  g_object_set (ce->priv->provider,
                "prefers-color-scheme", color_scheme,
                "prefers-contrast", contrast,
                "prefers-reduced-motion", reduced_motion,
                NULL);
}

static void
system_settings_changed (BobguiSettings           *settings,
                         GParamSpec            *pspec,
                         BobguiInspectorCssEditor *ce)
{
  settings_changed (NULL, NULL, ce);
}

static void
update_style (BobguiInspectorCssEditor *ce)
{
  char *text;

  g_list_free_full (ce->priv->errors, css_error_free);
  ce->priv->errors = NULL;

  text = get_current_text (ce->priv->text);
  bobgui_css_provider_load_from_string (ce->priv->provider, text);
  g_free (text);
}

static gboolean
update_timeout (gpointer data)
{
  BobguiInspectorCssEditor *ce = data;

  ce->priv->timeout = 0;

  autosave_contents (ce);
  update_style (ce);

  return G_SOURCE_REMOVE;
}

static void
text_changed (BobguiTextBuffer         *buffer,
              BobguiInspectorCssEditor *ce)
{
  if (ce->priv->timeout != 0)
    g_source_remove (ce->priv->timeout);

  ce->priv->timeout = g_timeout_add (100, update_timeout, ce);

  g_list_free_full (ce->priv->errors, css_error_free);
  ce->priv->errors = NULL;
}

static void
show_parsing_error (BobguiCssProvider        *provider,
                    BobguiCssSection         *section,
                    const GError          *error,
                    BobguiInspectorCssEditor *ce)
{
  const char *tag_name;
  BobguiTextBuffer *buffer = BOBGUI_TEXT_BUFFER (ce->priv->text);
  const BobguiCssLocation *start, *end;
  CssError *css_error;

  css_error = g_new (CssError, 1);
  css_error->error = g_error_copy (error);

  start = bobgui_css_section_get_start_location (section);
  bobgui_text_buffer_get_iter_at_line_index (buffer,
                                          &css_error->start,
                                          start->lines,
                                          start->line_bytes);
  end = bobgui_css_section_get_end_location (section);
  bobgui_text_buffer_get_iter_at_line_index (buffer,
                                          &css_error->end,
                                          end->lines,
                                          end->line_bytes);

  if (error->domain == BOBGUI_CSS_PARSER_WARNING)
    {
      if (error->code == BOBGUI_CSS_PARSER_WARNING_DEPRECATED)
        tag_name = "deprecation";
      else
        tag_name = "warning";
    }
  else
    tag_name = "error";

  if (bobgui_text_iter_equal (&css_error->start, &css_error->end))
    bobgui_text_iter_forward_char (&css_error->end);

  bobgui_text_buffer_apply_tag_by_name (buffer, tag_name, &css_error->start, &css_error->end);

  ce->priv->errors = g_list_prepend (ce->priv->errors, css_error);
}

static void
create_provider (BobguiInspectorCssEditor *ce)
{
  ce->priv->provider = bobgui_css_provider_new ();

  g_signal_connect (ce->priv->provider, "parsing-error",
                    G_CALLBACK (show_parsing_error), ce);
}

static void
destroy_provider (BobguiInspectorCssEditor *ce)
{
  g_signal_handlers_disconnect_by_func (ce->priv->provider, show_parsing_error, ce);
  g_clear_object (&ce->priv->provider);
}

static void
add_provider (BobguiInspectorCssEditor *ce,
              GdkDisplay *display)
{
  BobguiSettings *settings = bobgui_settings_get_for_display (display);

  g_signal_connect_object (settings, "notify::bobgui-interface-color-scheme",
                           G_CALLBACK (system_settings_changed), ce, 0);
  g_signal_connect_object (settings, "notify::bobgui-interface-contrast",
                           G_CALLBACK (system_settings_changed), ce, 0);
  g_signal_connect_object (settings, "notify::bobgui-interface-reduced-motion",
                           G_CALLBACK (system_settings_changed), ce, 0);

  system_settings_changed (settings, NULL, ce);

  bobgui_style_context_add_provider_for_display (display,
                                              BOBGUI_STYLE_PROVIDER (ce->priv->provider),
                                              BOBGUI_STYLE_PROVIDER_PRIORITY_USER);
}

static void
remove_provider (BobguiInspectorCssEditor *ce,
                 GdkDisplay *display)
{
  bobgui_style_context_remove_provider_for_display (display,
                                                 BOBGUI_STYLE_PROVIDER (ce->priv->provider));
}

static void
bobgui_inspector_css_editor_init (BobguiInspectorCssEditor *ce)
{
  ce->priv = bobgui_inspector_css_editor_get_instance_private (ce);
  bobgui_widget_init_template (BOBGUI_WIDGET (ce));
}

static void
constructed (GObject *object)
{
  BobguiInspectorCssEditor *ce = BOBGUI_INSPECTOR_CSS_EDITOR (object);

  create_provider (ce);
}

static void
finalize (GObject *object)
{
  BobguiInspectorCssEditor *ce = BOBGUI_INSPECTOR_CSS_EDITOR (object);

  if (ce->priv->timeout != 0)
    g_source_remove (ce->priv->timeout);

  if (ce->priv->display)
    remove_provider (ce, ce->priv->display);
  destroy_provider (ce);

  g_list_free_full (ce->priv->errors, css_error_free);

  G_OBJECT_CLASS (bobgui_inspector_css_editor_parent_class)->finalize (object);
}

static void
bobgui_inspector_css_editor_class_init (BobguiInspectorCssEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->constructed = constructed;
  object_class->finalize = finalize;

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/inspector/css-editor.ui");
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorCssEditor, text);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorCssEditor, view);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorCssEditor, enable_switch);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorCssEditor, color_scheme);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorCssEditor, contrast);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiInspectorCssEditor, reduced_motion);
  bobgui_widget_class_bind_template_callback (widget_class, enable_switch_changed);
  bobgui_widget_class_bind_template_callback (widget_class, toggle_deprecations);
  bobgui_widget_class_bind_template_callback (widget_class, save_clicked);
  bobgui_widget_class_bind_template_callback (widget_class, text_changed);
  bobgui_widget_class_bind_template_callback (widget_class, query_tooltip_cb);
  bobgui_widget_class_bind_template_callback (widget_class, settings_changed);
}

void
bobgui_inspector_css_editor_set_display (BobguiInspectorCssEditor *ce,
                                      GdkDisplay *display)
{
  ce->priv->display = display;
  add_provider (ce, display);
  set_initial_text (ce);
}

// vim: set et sw=2 ts=2:
