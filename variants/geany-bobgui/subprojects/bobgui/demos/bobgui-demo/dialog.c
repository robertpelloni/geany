/* Dialogs
 * #Keywords: BobguiMessageDialog
 *
 * Dialogs are used to pop up transient windows for information
 * and user feedback.
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static BobguiWidget *window = NULL;
static BobguiWidget *entry1 = NULL;
static BobguiWidget *entry2 = NULL;

static void
message_dialog_clicked (BobguiButton *button,
                        gpointer   user_data)
{
  BobguiWidget *dialog;
  static int i = 1;

  dialog = bobgui_message_dialog_new (BOBGUI_WINDOW (window),
                                   BOBGUI_DIALOG_MODAL | BOBGUI_DIALOG_DESTROY_WITH_PARENT,
                                   BOBGUI_MESSAGE_INFO,
                                   BOBGUI_BUTTONS_OK_CANCEL,
                                   "Test message");
  bobgui_message_dialog_format_secondary_text (BOBGUI_MESSAGE_DIALOG (dialog),
                                            ngettext ("Has been shown once", "Has been shown %d times", i), i);
  g_signal_connect (dialog, "response", G_CALLBACK (bobgui_window_destroy), NULL);
  bobgui_window_present (BOBGUI_WINDOW (dialog));
  i++;
}

typedef struct {
  BobguiWidget *local_entry1;
  BobguiWidget *local_entry2;
  BobguiWidget *global_entry1;
  BobguiWidget *global_entry2;
} ResponseData;

static void
on_dialog_response (BobguiDialog *dialog,
                    int        response,
                    gpointer   user_data)
{
  ResponseData *data = user_data;

  if (response == BOBGUI_RESPONSE_OK)
    {
      bobgui_editable_set_text (BOBGUI_EDITABLE (data->global_entry1),
                             bobgui_editable_get_text (BOBGUI_EDITABLE (data->local_entry1)));
      bobgui_editable_set_text (BOBGUI_EDITABLE (data->global_entry2),
                             bobgui_editable_get_text (BOBGUI_EDITABLE (data->local_entry2)));
    }

  bobgui_window_destroy (BOBGUI_WINDOW (dialog));
}

static void
interactive_dialog_clicked (BobguiButton *button,
                            gpointer   user_data)
{
  BobguiWidget *content_area;
  BobguiWidget *dialog;
  BobguiWidget *table;
  BobguiWidget *local_entry1;
  BobguiWidget *local_entry2;
  BobguiWidget *label;
  ResponseData *data;

  dialog = bobgui_dialog_new_with_buttons ("Interactive Dialog",
                                        BOBGUI_WINDOW (window),
                                        BOBGUI_DIALOG_MODAL| BOBGUI_DIALOG_DESTROY_WITH_PARENT|BOBGUI_DIALOG_USE_HEADER_BAR,
                                        _("_OK"), BOBGUI_RESPONSE_OK,
                                        _("_Cancel"), BOBGUI_RESPONSE_CANCEL,
                                        NULL);

  bobgui_dialog_set_default_response (BOBGUI_DIALOG (dialog), BOBGUI_RESPONSE_OK);

  content_area = bobgui_dialog_get_content_area (BOBGUI_DIALOG (dialog));

  table = bobgui_grid_new ();
  bobgui_widget_set_hexpand (table, TRUE);
  bobgui_widget_set_vexpand (table, TRUE);
  bobgui_widget_set_halign (table, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (table, BOBGUI_ALIGN_CENTER);
  bobgui_box_append (BOBGUI_BOX (content_area), table);
  bobgui_grid_set_row_spacing (BOBGUI_GRID (table), 6);
  bobgui_grid_set_column_spacing (BOBGUI_GRID (table), 6);

  label = bobgui_label_new_with_mnemonic ("_Entry 1");
  bobgui_grid_attach (BOBGUI_GRID (table), label, 0, 0, 1, 1);
  local_entry1 = bobgui_entry_new ();
  bobgui_editable_set_text (BOBGUI_EDITABLE (local_entry1), bobgui_editable_get_text (BOBGUI_EDITABLE (entry1)));
  bobgui_grid_attach (BOBGUI_GRID (table), local_entry1, 1, 0, 1, 1);
  bobgui_label_set_mnemonic_widget (BOBGUI_LABEL (label), local_entry1);

  label = bobgui_label_new_with_mnemonic ("E_ntry 2");
  bobgui_grid_attach (BOBGUI_GRID (table), label, 0, 1, 1, 1);

  local_entry2 = bobgui_entry_new ();
  bobgui_editable_set_text (BOBGUI_EDITABLE (local_entry2), bobgui_editable_get_text (BOBGUI_EDITABLE (entry2)));
  bobgui_grid_attach (BOBGUI_GRID (table), local_entry2, 1, 1, 1, 1);
  bobgui_label_set_mnemonic_widget (BOBGUI_LABEL (label), local_entry2);

  data = g_new (ResponseData, 1);
  data->local_entry1 = local_entry1;
  data->local_entry2 = local_entry2;
  data->global_entry1 = entry1;
  data->global_entry2 = entry2;

  g_signal_connect_data (dialog, "response",
                         G_CALLBACK (on_dialog_response),
                         data, (GClosureNotify) g_free,
                         0);

  bobgui_window_present (BOBGUI_WINDOW (dialog));
}

BobguiWidget *
do_dialog (BobguiWidget *do_widget)
{
  BobguiWidget *vbox;
  BobguiWidget *vbox2;
  BobguiWidget *hbox;
  BobguiWidget *button;
  BobguiWidget *table;
  BobguiWidget *label;

  if (!window)
    {
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Dialogs");
      bobgui_window_set_resizable (BOBGUI_WINDOW (window), FALSE);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 8);
      bobgui_widget_set_margin_start (vbox, 8);
      bobgui_widget_set_margin_end (vbox, 8);
      bobgui_widget_set_margin_top (vbox, 8);
      bobgui_widget_set_margin_bottom (vbox, 8);
      bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

      /* Standard message dialog */
      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 8);
      bobgui_box_append (BOBGUI_BOX (vbox), hbox);
      button = bobgui_button_new_with_mnemonic ("_Message Dialog");
      g_signal_connect (button, "clicked",
                        G_CALLBACK (message_dialog_clicked), NULL);
      bobgui_box_append (BOBGUI_BOX (hbox), button);

      bobgui_box_append (BOBGUI_BOX (vbox), bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL));

      /* Interactive dialog*/
      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 8);
      bobgui_box_append (BOBGUI_BOX (vbox), hbox);
      vbox2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);

      button = bobgui_button_new_with_mnemonic ("_Interactive Dialog");
      g_signal_connect (button, "clicked",
                        G_CALLBACK (interactive_dialog_clicked), NULL);
      bobgui_box_append (BOBGUI_BOX (hbox), vbox2);
      bobgui_box_append (BOBGUI_BOX (vbox2), button);

      table = bobgui_grid_new ();
      bobgui_grid_set_row_spacing (BOBGUI_GRID (table), 4);
      bobgui_grid_set_column_spacing (BOBGUI_GRID (table), 4);
      bobgui_box_append (BOBGUI_BOX (hbox), table);

      label = bobgui_label_new_with_mnemonic ("_Entry 1");
      bobgui_grid_attach (BOBGUI_GRID (table), label, 0, 0, 1, 1);

      entry1 = bobgui_entry_new ();
      bobgui_grid_attach (BOBGUI_GRID (table), entry1, 1, 0, 1, 1);
      bobgui_label_set_mnemonic_widget (BOBGUI_LABEL (label), entry1);

      label = bobgui_label_new_with_mnemonic ("E_ntry 2");
      bobgui_grid_attach (BOBGUI_GRID (table), label, 0, 1, 1, 1);

      entry2 = bobgui_entry_new ();
      bobgui_grid_attach (BOBGUI_GRID (table), entry2, 1, 1, 1, 1);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
