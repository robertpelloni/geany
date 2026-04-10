/* List Box/Controls
 *
 * BobguiListBox is well-suited for creating “button strips” — lists of
 * controls for use in preference dialogs or settings panels. To create
 * this style of list, use the .rich-list style class.
 */

#include <bobgui/bobgui.h>

static BobguiWidget *window;
static BobguiWidget *switch_widget;
static BobguiWidget *check;
static BobguiWidget *image;

static void
row_activated (BobguiListBox    *list,
               BobguiListBoxRow *row)
{
  if (bobgui_widget_is_ancestor (switch_widget, BOBGUI_WIDGET (row)))
    {
      bobgui_switch_set_active (BOBGUI_SWITCH (switch_widget),
                             !bobgui_switch_get_active (BOBGUI_SWITCH (switch_widget)));
    }
  else if (bobgui_widget_is_ancestor (check, BOBGUI_WIDGET (row)))
    {
      bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (check),
                                   !bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (check)));
    }
  else if (bobgui_widget_is_ancestor (image, BOBGUI_WIDGET (row)))
    {
      bobgui_widget_set_opacity (image,
                              1.0 - bobgui_widget_get_opacity (image));
    }
}

BobguiWidget *
do_listbox_controls (BobguiWidget *do_widget)
{
  if (!window)
    {
      BobguiBuilderScope *scope;
      BobguiBuilder *builder;

      scope = bobgui_builder_cscope_new ();
      bobgui_builder_cscope_add_callback (scope, row_activated);
      builder = bobgui_builder_new ();
      bobgui_builder_set_scope (builder, scope);

      bobgui_builder_add_from_resource (builder, "/listbox_controls/listbox_controls.ui", NULL);

      window = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "window"));
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      switch_widget = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "switch"));
      check = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "check"));
      image = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "image"));

      g_object_unref (builder);
      g_object_unref (scope);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
