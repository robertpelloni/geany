/* Stack Sidebar
 *
 * BobguiStackSidebar provides an automatic sidebar widget to control
 * navigation of a BobguiStack object. This widget automatically updates
 * its content based on what is presently available in the BobguiStack
 * object, and using the "title" child property to set the display labels.
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>

BobguiWidget *
do_sidebar (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *sidebar;
  BobguiWidget *stack;
  BobguiWidget *box;
  BobguiWidget *widget;
  BobguiWidget *header;
  const char * pages[] = {
    "Welcome to BOBGUI",
    "BobguiStackSidebar Widget",
    "Automatic navigation",
    "Consistent appearance",
    "Scrolling",
    "Page 6",
    "Page 7",
    "Page 8",
    "Page 9",
    NULL
  };
  const char *c = NULL;
  guint i;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_resizable (BOBGUI_WINDOW (window), TRUE);

      header = bobgui_header_bar_new ();
      bobgui_window_set_titlebar (BOBGUI_WINDOW(window), header);
      bobgui_window_set_title (BOBGUI_WINDOW(window), "Stack Sidebar");
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      sidebar = bobgui_stack_sidebar_new ();
      bobgui_box_append (BOBGUI_BOX (box), sidebar);

      stack = bobgui_stack_new ();
      bobgui_stack_set_transition_type (BOBGUI_STACK (stack), BOBGUI_STACK_TRANSITION_TYPE_SLIDE_UP_DOWN);
      bobgui_stack_sidebar_set_stack (BOBGUI_STACK_SIDEBAR (sidebar), BOBGUI_STACK (stack));
      bobgui_widget_set_hexpand (stack, TRUE);

      bobgui_box_append (BOBGUI_BOX (box), stack);

      for (i=0; (c = *(pages+i)) != NULL; i++ )
        {
          if (i == 0)
            {
              widget = bobgui_image_new_from_icon_name ("org.bobgui.Demo4");
              bobgui_widget_add_css_class (widget, "icon-dropshadow");
              bobgui_image_set_pixel_size (BOBGUI_IMAGE (widget), 256);
            }
          else
            {
              widget = bobgui_label_new (c);
            }
          bobgui_stack_add_named (BOBGUI_STACK (stack), widget, c);
          g_object_set (bobgui_stack_get_page (BOBGUI_STACK (stack), widget), "title", c, NULL);
        }

      bobgui_window_set_child (BOBGUI_WINDOW (window), box);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
