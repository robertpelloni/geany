/* Theming/Style Classes
 *
 * BOBGUI uses CSS for theming. Style classes can be associated
 * with widgets to inform the theme about intended rendering.
 *
 * This demo shows some common examples where theming features
 * of BOBGUI are used for certain effects: primary toolbars
 * and linked buttons.
 */

#include <bobgui/bobgui.h>

static BobguiWidget *window = NULL;

BobguiWidget *
do_theming_style_classes (BobguiWidget *do_widget)
{
  BobguiWidget *grid;
  BobguiBuilder *builder;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Style Classes");
      bobgui_window_set_resizable (BOBGUI_WINDOW (window), FALSE);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      builder = bobgui_builder_new_from_resource ("/theming_style_classes/theming.ui");

      grid = (BobguiWidget *)bobgui_builder_get_object (builder, "grid");
      bobgui_window_set_child (BOBGUI_WINDOW (window), grid);
      g_object_unref (builder);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
