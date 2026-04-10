/* Path/Path Explorer
 *
 * This demo lets you explore some of the features of GskPath.
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>

#include "range-editor.h"
#include "path_explorer.h"

static gboolean
text_to_path (GBinding     *binding,
              const GValue *from,
              GValue       *to,
              gpointer      user_data)
{
  const char *text;
  GskPath *path;

  text = g_value_get_string (from);
  path = gsk_path_parse (text);
  g_value_take_boxed (to, path);

  return TRUE;
}

static gboolean
path_to_text (GBinding     *binding,
              const GValue *from,
              GValue       *to,
              gpointer      user_data)
{
  GskPath *path;
  char *text;

  path = g_value_get_boxed (from);
  text = gsk_path_to_string (path);
  g_value_take_string (to, text);

  return TRUE;
}

BobguiWidget *
do_path_explorer_demo (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;
  static BobguiCssProvider *css_provider = NULL;

  if (!css_provider)
    {
      css_provider = bobgui_css_provider_new ();
      bobgui_css_provider_load_from_resource (css_provider, "/path_explorer_demo/path_explorer_demo.css");

      bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                                  BOBGUI_STYLE_PROVIDER (css_provider),
                                                  BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

  if (!window)
    {
      BobguiBuilder *builder;
      PathExplorer *demo;
      BobguiWidget *entry;
      GError *error = NULL;

      g_type_ensure (path_explorer_get_type ());
      g_type_ensure (range_editor_get_type ());

      builder = bobgui_builder_new ();

      bobgui_builder_add_from_resource (builder, "/path_explorer_demo/path_explorer_demo.ui", &error);
      if (error)
        g_error ("%s", error->message);

      window = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "window"));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      demo = PATH_EXPLORER (bobgui_builder_get_object (builder, "demo"));
      entry = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "entry"));
      g_object_bind_property_full (demo, "path",
                                   entry, "text",
                                   G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE,
                                   path_to_text,
                                   text_to_path,
                                   NULL, NULL);

      g_object_unref (builder);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_window_present (BOBGUI_WINDOW (window));
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
