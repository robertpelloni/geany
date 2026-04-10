#include <bobgui/bobgui.h>
#include "bobgui/bobguimodelbuttonprivate.h"

static void
on_action_beep (GSimpleAction *action,
                GVariant      *parameter,
                void          *user_data)
{
  GdkDisplay *display = gdk_display_get_default ();
  gdk_display_beep (display);
}

static void
on_application_activate (GApplication *gapplication,
                         void         *user_data)
{
  BobguiApplication *application = BOBGUI_APPLICATION (gapplication);
  BobguiCssProvider *css_provider = bobgui_css_provider_new ();
  GdkDisplay *display = gdk_display_get_default ();

  GSimpleAction *action;
  BobguiWidget *box;
  GIcon *gicon;
  BobguiWidget *model_button;
  BobguiWidget *widget;

  bobgui_css_provider_load_from_data (css_provider,
    "window > box { padding: 0.5em; }"
    "window > box > * { margin: 0.5em; }"
    /* :iconic == FALSE */
    "modelbutton > check { background: red; }"
    "modelbutton > radio { background: green; }"
    "modelbutton > arrow { background: blue; }"
    /* :iconic == TRUE */
    "button.model { background: yellow; }"
    , -1);
  g_assert (GDK_IS_DISPLAY (display));
  bobgui_style_context_add_provider_for_display (display,
                                              BOBGUI_STYLE_PROVIDER (css_provider),
                                              BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);

  action = g_simple_action_new ("beep", NULL);
  g_signal_connect (action, "activate", G_CALLBACK (on_action_beep), NULL);
  g_action_map_add_action (G_ACTION_MAP (application), G_ACTION (action));

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);

  gicon = g_themed_icon_new ("face-smile");

  model_button = g_object_new (g_type_from_name ("BobguiModelButton"),
                               "action-name", "app.beep",
                               "text", "It’s-a-me! ModelButton",
                               "icon", gicon,
                               NULL);
  bobgui_box_append (BOBGUI_BOX (box), model_button);

  g_object_unref (gicon);

  widget = bobgui_combo_box_text_new ();
  bobgui_combo_box_text_append (BOBGUI_COMBO_BOX_TEXT (widget),
                             NULL, "BOBGUI_BUTTON_ROLE_NORMAL");
  bobgui_combo_box_text_append (BOBGUI_COMBO_BOX_TEXT (widget),
                             NULL, "BOBGUI_BUTTON_ROLE_CHECK");
  bobgui_combo_box_text_append (BOBGUI_COMBO_BOX_TEXT (widget),
                             NULL, "BOBGUI_BUTTON_ROLE_RADIO");
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (widget), 0);
  g_object_bind_property (widget, "active",
                          model_button, "role",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  bobgui_box_append (BOBGUI_BOX (box), widget);

  widget = bobgui_toggle_button_new_with_label (":iconic");
  g_object_bind_property (widget, "active",
                          model_button, "iconic",
                          G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  bobgui_box_append (BOBGUI_BOX (box), widget);

  widget = bobgui_window_new ();
  bobgui_box_append (BOBGUI_BOX (widget), box);
  bobgui_window_present (BOBGUI_WINDOW (widget))
  bobgui_application_add_window (BOBGUI_APPLICATION (application), BOBGUI_WINDOW (widget));
}

int
main (int   argc,
      char *argv[])
{
  BobguiApplication *application = bobgui_application_new ("org.bobgui.test.modelbutton",
                                                     G_APPLICATION_DEFAULT_FLAGS);
  int result;

  g_signal_connect (application, "activate",
                    G_CALLBACK (on_application_activate), NULL);

  result = g_application_run (G_APPLICATION (application), argc, argv);
  g_object_unref (application);
  return result;
}
