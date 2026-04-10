/* Info Bars
 * #Keywords: BobguiInfoBar
 *
 * Info bar widgets are used to report important messages to the user.
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
on_bar_response (BobguiInfoBar *info_bar,
                 int         response_id,
                 gpointer    user_data)
{
  BobguiAlertDialog *dialog;
  char *detail;

  if (response_id == BOBGUI_RESPONSE_CLOSE)
    {
      bobgui_info_bar_set_revealed (info_bar, FALSE);
      return;
    }

  dialog = bobgui_alert_dialog_new ("You clicked a button on an info bar");
  detail = g_strdup_printf ("Your response has been %d", response_id);
  bobgui_alert_dialog_set_detail (dialog, detail);
  g_free (detail);
  bobgui_alert_dialog_show (dialog, BOBGUI_WINDOW (bobgui_widget_get_root (BOBGUI_WIDGET (info_bar))));
  g_object_unref (dialog);
}

BobguiWidget *
do_infobar (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;
  BobguiWidget *frame;
  BobguiWidget *bar;
  BobguiWidget *vbox;
  BobguiWidget *label;
  BobguiWidget *actions;
  BobguiWidget *button;

  if (!window)
    {
      actions = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_widget_add_css_class (actions, "linked");

      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Info Bars");
      bobgui_window_set_resizable (BOBGUI_WINDOW (window), FALSE);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_widget_set_margin_start (vbox, 8);
      bobgui_widget_set_margin_end (vbox, 8);
      bobgui_widget_set_margin_top (vbox, 8);
      bobgui_widget_set_margin_bottom (vbox, 8);
      bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

      bar = bobgui_info_bar_new ();
      bobgui_box_append (BOBGUI_BOX (vbox), bar);
      bobgui_info_bar_set_message_type (BOBGUI_INFO_BAR (bar), BOBGUI_MESSAGE_INFO);
      label = bobgui_label_new ("This is an info bar with message type BOBGUI_MESSAGE_INFO");
      bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
      bobgui_label_set_xalign (BOBGUI_LABEL (label), 0);
      bobgui_info_bar_add_child (BOBGUI_INFO_BAR (bar), label);

      button = bobgui_toggle_button_new_with_label ("Message");
      g_object_bind_property (bar, "revealed", button, "active", G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      bobgui_box_append (BOBGUI_BOX (actions), button);

      bar = bobgui_info_bar_new ();
      bobgui_box_append (BOBGUI_BOX (vbox), bar);
      bobgui_info_bar_set_message_type (BOBGUI_INFO_BAR (bar), BOBGUI_MESSAGE_WARNING);
      label = bobgui_label_new ("This is an info bar with message type BOBGUI_MESSAGE_WARNING");
      bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
      bobgui_label_set_xalign (BOBGUI_LABEL (label), 0);
      bobgui_info_bar_add_child (BOBGUI_INFO_BAR (bar), label);

      button = bobgui_toggle_button_new_with_label ("Warning");
      g_object_bind_property (bar, "revealed", button, "active", G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      bobgui_box_append (BOBGUI_BOX (actions), button);

      bar = bobgui_info_bar_new_with_buttons (_("_OK"), BOBGUI_RESPONSE_OK, NULL);
      bobgui_info_bar_set_show_close_button (BOBGUI_INFO_BAR (bar), TRUE);
      g_signal_connect (bar, "response", G_CALLBACK (on_bar_response), window);
      bobgui_box_append (BOBGUI_BOX (vbox), bar);
      bobgui_info_bar_set_message_type (BOBGUI_INFO_BAR (bar), BOBGUI_MESSAGE_QUESTION);
      label = bobgui_label_new ("This is an info bar with message type BOBGUI_MESSAGE_QUESTION");
      bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
      bobgui_label_set_xalign (BOBGUI_LABEL (label), 0);
      bobgui_info_bar_add_child (BOBGUI_INFO_BAR (bar), label);
      bobgui_info_bar_set_default_response (BOBGUI_INFO_BAR (bar), BOBGUI_RESPONSE_OK);

      button = bobgui_toggle_button_new_with_label ("Question");
      g_object_bind_property (bar, "revealed", button, "active", G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      bobgui_box_append (BOBGUI_BOX (actions), button);

      bar = bobgui_info_bar_new ();
      bobgui_box_append (BOBGUI_BOX (vbox), bar);
      bobgui_info_bar_set_message_type (BOBGUI_INFO_BAR (bar), BOBGUI_MESSAGE_ERROR);
      label = bobgui_label_new ("This is an info bar with message type BOBGUI_MESSAGE_ERROR");
      bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
      bobgui_label_set_xalign (BOBGUI_LABEL (label), 0);
      bobgui_info_bar_add_child (BOBGUI_INFO_BAR (bar), label);

      button = bobgui_toggle_button_new_with_label ("Error");
      g_object_bind_property (bar, "revealed", button, "active", G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

      bobgui_box_append (BOBGUI_BOX (actions), button);

      bar = bobgui_info_bar_new ();
      bobgui_box_append (BOBGUI_BOX (vbox), bar);
      bobgui_info_bar_set_message_type (BOBGUI_INFO_BAR (bar), BOBGUI_MESSAGE_OTHER);
      label = bobgui_label_new ("This is an info bar with message type BOBGUI_MESSAGE_OTHER");
      bobgui_label_set_wrap (BOBGUI_LABEL (label), TRUE);
      bobgui_label_set_xalign (BOBGUI_LABEL (label), 0);
      bobgui_info_bar_add_child (BOBGUI_INFO_BAR (bar), label);

      button = bobgui_toggle_button_new_with_label ("Other");
      g_object_bind_property (bar, "revealed", button, "active", G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
      bobgui_box_append (BOBGUI_BOX (actions), button);

      frame = bobgui_frame_new ("An example of different info bars");
      bobgui_widget_set_margin_top (frame, 8);
      bobgui_widget_set_margin_bottom (frame, 8);
      bobgui_box_append (BOBGUI_BOX (vbox), frame);

      bobgui_widget_set_halign (actions, BOBGUI_ALIGN_CENTER);

      bobgui_widget_set_margin_start (actions, 8);
      bobgui_widget_set_margin_end (actions, 8);
      bobgui_widget_set_margin_top (actions, 8);
      bobgui_widget_set_margin_bottom (actions, 8);
      bobgui_frame_set_child (BOBGUI_FRAME (frame), actions);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
