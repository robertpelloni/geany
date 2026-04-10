#include <bobgui/bobgui.h>

static void
update_offset (GObject    *object,
               GParamSpec *pspec,
               BobguiWidget  *widget)
{
  if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (object)))
    bobgui_popover_set_offset (BOBGUI_POPOVER (widget), 12, 12);
  else
    bobgui_popover_set_offset (BOBGUI_POPOVER (widget), 0, 0);
}

static void
update_shadow (GObject    *object,
               GParamSpec *pspec,
               BobguiWidget  *widget)
{
  const char *classes[] = {
    "no-shadow",
    "shadow1",
    "shadow2",
    "shadow3",
    "shadow4",
  };
  guint selected;

  selected = bobgui_drop_down_get_selected (BOBGUI_DROP_DOWN (object));
  g_assert (selected < G_N_ELEMENTS (classes));

  for (int i = 0; i < G_N_ELEMENTS (classes); i++)
    bobgui_widget_remove_css_class (widget, classes[i]);

  bobgui_widget_add_css_class (widget, classes[selected]);
}

static const char css[] =
 "popover.no-shadow > contents { box-shadow: none; }\n"
 "popover.shadow1 > contents { box-shadow: 6px 6px rgba(128,0,255,0.5); }\n"
 "popover.shadow2 > contents { box-shadow: -6px -6px rgba(255,0,0,0.5), 6px 6px rgba(128,0,255,0.5); }\n"
 "popover.shadow3 > contents { box-shadow: -6px -6px rgba(255,0,0,0.5), 18px 18px rgba(128,0,255,0.5); }\n"
 "popover.shadow4 > contents { box-shadow: -18px -18px rgba(255,0,0,0.5), 18px 18px rgba(128,0,255,0.5); }\n";

int
main (int argc, char *argv[])
{
  BobguiWidget *window;
  BobguiWidget *box;
  BobguiWidget *button;
  BobguiWidget *popover;
  BobguiWidget *box2;
  BobguiWidget *box3;
  BobguiWidget *checkbox;
  BobguiWidget *checkbox2;
  BobguiWidget *dropdown;
  BobguiWidget *dropdown2;
  BobguiCssProvider *provider;

  bobgui_init ();

  provider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_string (provider, css);

  bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                              BOBGUI_STYLE_PROVIDER (provider),
                                              800);

  window = bobgui_window_new ();
  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  g_object_set (box,
                "margin-top", 20,
                "margin-bottom", 20,
                "margin-start", 20,
                "margin-end", 20,
                NULL);

  button = bobgui_menu_button_new ();

  bobgui_widget_set_halign (button, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (button, BOBGUI_ALIGN_CENTER);

  bobgui_box_append (BOBGUI_BOX (box), button);

  popover = bobgui_popover_new ();
  box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
  bobgui_popover_set_child (BOBGUI_POPOVER (popover), box2);

  bobgui_box_append (BOBGUI_BOX (box2), bobgui_label_new ("First item"));
  bobgui_box_append (BOBGUI_BOX (box2), bobgui_label_new ("Second item"));
  bobgui_box_append (BOBGUI_BOX (box2), bobgui_label_new ("Third item"));

  bobgui_menu_button_set_popover (BOBGUI_MENU_BUTTON (button), popover);

  box3 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
  dropdown = bobgui_drop_down_new_from_strings ((const char*[]){ "Left", "Right", "Top", "Bottom", NULL });
  bobgui_drop_down_set_selected (BOBGUI_DROP_DOWN (dropdown), 3);

  checkbox = bobgui_check_button_new_with_label ("Arrow");

  checkbox2 = bobgui_check_button_new_with_label ("Offset");

  dropdown2 = bobgui_drop_down_new_from_strings ((const char*[]){ "No Shadow", "Shadow 1", "Shadow 2", "Shadow 3", "Shadow 4", NULL });

  bobgui_box_append (BOBGUI_BOX (box3), checkbox);
  bobgui_box_append (BOBGUI_BOX (box3), checkbox2);
  bobgui_box_append (BOBGUI_BOX (box3), dropdown);
  bobgui_box_append (BOBGUI_BOX (box3), dropdown2);

  bobgui_box_append (BOBGUI_BOX (box), box3);

  g_object_bind_property (checkbox, "active",
                          popover, "has-arrow",
                          G_BINDING_SYNC_CREATE);
  g_signal_connect (checkbox2, "notify::active",
                    G_CALLBACK (update_offset), popover);
  g_object_bind_property (dropdown, "selected",
                          popover, "position",
                          G_BINDING_SYNC_CREATE);
  g_signal_connect (dropdown2, "notify::selected",
                    G_CALLBACK (update_shadow), popover);
  update_shadow (G_OBJECT (dropdown2), NULL, popover);

  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (g_list_model_get_n_items (bobgui_window_get_toplevels ()) > 0)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
