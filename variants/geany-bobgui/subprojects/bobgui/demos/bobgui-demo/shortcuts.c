/* Shortcuts Window
 *
 * BobguiShortcutsWindow is a window that provides a help overlay
 * for shortcuts and gestures in an application.
 */

#include <bobgui/bobgui.h>

static void
show_shortcuts (BobguiWidget   *window,
                const char *id,
                const char *view)
{
  BobguiBuilder *builder;
  BobguiWidget *overlay;
  char *path;

  path = g_strdup_printf ("/shortcuts/%s.ui", id);
  builder = bobgui_builder_new_from_resource (path);
  g_free (path);
  overlay = BOBGUI_WIDGET (bobgui_builder_get_object (builder, id));
  bobgui_window_set_transient_for (BOBGUI_WINDOW (overlay), BOBGUI_WINDOW (window));
  g_object_set (overlay, "view-name", view, NULL);
  g_object_unref (builder);
  bobgui_window_present (BOBGUI_WINDOW (overlay));
}

G_MODULE_EXPORT void
shortcuts_builder_shortcuts (BobguiWidget *window)
{
  show_shortcuts (window, "shortcuts-builder", NULL);
}

G_MODULE_EXPORT void
shortcuts_gedit_shortcuts (BobguiWidget *window)
{
  show_shortcuts (window, "shortcuts-gedit", NULL);
}

G_MODULE_EXPORT void
shortcuts_clocks_shortcuts (BobguiWidget *window)
{
  show_shortcuts (window, "shortcuts-clocks", NULL);
}

G_MODULE_EXPORT void
shortcuts_clocks_shortcuts_stopwatch (BobguiWidget *window)
{
  show_shortcuts (window, "shortcuts-clocks", "stopwatch");
}

G_MODULE_EXPORT void
shortcuts_boxes_shortcuts (BobguiWidget *window)
{
  show_shortcuts (window, "shortcuts-boxes", NULL);
}

G_MODULE_EXPORT void
shortcuts_boxes_shortcuts_wizard (BobguiWidget *window)
{
  show_shortcuts (window, "shortcuts-boxes", "wizard");
}

G_MODULE_EXPORT void
shortcuts_boxes_shortcuts_display (BobguiWidget *window)
{
  show_shortcuts (window, "shortcuts-boxes", "display");
}

BobguiWidget *
do_shortcuts (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;
  static gboolean icons_added = FALSE;

  if (!icons_added)
    {
      icons_added = TRUE;
      bobgui_icon_theme_add_resource_path (bobgui_icon_theme_get_for_display (bobgui_widget_get_display (do_widget)), "/icons");
    }

  g_type_ensure (G_TYPE_FILE_ICON);

  if (!window)
    {
      BobguiBuilder *builder;

      builder = bobgui_builder_new_from_resource ("/shortcuts/shortcuts.ui");
      window = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "window1"));
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      g_object_unref (builder);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
