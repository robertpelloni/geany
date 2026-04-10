/* Layout Manager/Transition
 * #Keywords: BobguiLayoutManager
 *
 * This demo shows a simple example of a custom layout manager
 * and a widget using it. The layout manager places the children
 * of the widget in a grid or a circle.
 *
 * The widget is animating the transition between the two layouts.
 *
 * Click to start the transition.
 */

#include <bobgui/bobgui.h>

#include "demowidget.h"
#include "demochild.h"


BobguiWidget *
do_layoutmanager (BobguiWidget *parent)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *widget;
      BobguiWidget *child;
      const char *color[] = {
        "red", "orange", "yellow", "green",
        "blue", "grey", "magenta", "lime",
        "yellow", "firebrick", "aqua", "purple",
        "tomato", "pink", "thistle", "maroon"
      };
      int i;

      window = bobgui_window_new ();
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Layout Manager — Transition");
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 600, 600);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      widget = demo_widget_new ();

      for (i = 0; i < 16; i++)
        {
          child = demo_child_new (color[i]);
          bobgui_widget_set_margin_start (child, 4);
          bobgui_widget_set_margin_end (child, 4);
          bobgui_widget_set_margin_top (child, 4);
          bobgui_widget_set_margin_bottom (child, 4);
          demo_widget_add_child (DEMO_WIDGET (widget), child);
        }

      bobgui_window_set_child (BOBGUI_WINDOW (window), widget);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;

}
