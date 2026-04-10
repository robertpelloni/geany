/* Stack
 *
 * BobguiStack is a container that shows a single child at a time,
 * with nice transitions when the visible child changes.
 *
 * BobguiStackSwitcher adds buttons to control which child is visible.
 */

#include <bobgui/bobgui.h>

BobguiWidget *
do_stack (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiBuilder *builder;

      builder = bobgui_builder_new_from_resource ("/stack/stack.ui");
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
