#include <bobgui/bobgui.h>
#include <glib/gstdio.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
show_message_dialog1 (BobguiWindow *parent)
{
  BobguiWidget *dialog;

  dialog = BOBGUI_WIDGET (bobgui_message_dialog_new (parent,
                                               BOBGUI_DIALOG_MODAL|
                                               BOBGUI_DIALOG_DESTROY_WITH_PARENT|
                                               BOBGUI_DIALOG_USE_HEADER_BAR,
                                               BOBGUI_MESSAGE_INFO,
                                               BOBGUI_BUTTONS_OK,
                                               "Oops! Something went wrong."));
  bobgui_message_dialog_format_secondary_text (BOBGUI_MESSAGE_DIALOG (dialog),
                                            "Unhandled error message: SSH program unexpectedly exited");

  bobgui_window_present (BOBGUI_WINDOW (dialog));
  g_signal_connect (dialog, "response", G_CALLBACK (bobgui_window_destroy), NULL);
}

static void
show_message_dialog1a (BobguiWindow *parent)
{
  BobguiWidget *dialog;

  dialog = BOBGUI_WIDGET (bobgui_message_dialog_new (parent,
                                               BOBGUI_DIALOG_MODAL|
                                               BOBGUI_DIALOG_DESTROY_WITH_PARENT|
                                               BOBGUI_DIALOG_USE_HEADER_BAR,
                                               BOBGUI_MESSAGE_INFO,
                                               BOBGUI_BUTTONS_OK,
                                               "The system network services are not compatible with this version."));

  bobgui_window_present (BOBGUI_WINDOW (dialog));
  g_signal_connect (dialog, "response", G_CALLBACK (bobgui_window_destroy), NULL);
}

static void
show_message_dialog2 (BobguiWindow *parent)
{
  BobguiWidget *dialog;

  dialog = BOBGUI_WIDGET (bobgui_message_dialog_new (parent,
                                               BOBGUI_DIALOG_MODAL|
                                               BOBGUI_DIALOG_DESTROY_WITH_PARENT|
                                               BOBGUI_DIALOG_USE_HEADER_BAR,
                                               BOBGUI_MESSAGE_INFO,
                                               BOBGUI_BUTTONS_NONE,
                                               "Empty all items from Wastebasket?"));
  bobgui_message_dialog_format_secondary_text (BOBGUI_MESSAGE_DIALOG (dialog),
                                            "All items in the Wastebasket will be permanently deleted");
  bobgui_dialog_add_buttons (BOBGUI_DIALOG (dialog), 
                          "Cancel", BOBGUI_RESPONSE_CANCEL,
                          "Empty Wastebasket", BOBGUI_RESPONSE_OK,
                          NULL);  

  bobgui_window_present (BOBGUI_WINDOW (dialog));
  g_signal_connect (dialog, "response", G_CALLBACK (bobgui_window_destroy), NULL);
}

static void
show_color_chooser (BobguiWindow *parent)
{
  BobguiWidget *dialog;

  dialog = bobgui_color_chooser_dialog_new ("Builtin", parent);
  bobgui_window_present (BOBGUI_WINDOW (dialog));
  g_signal_connect (dialog, "response", G_CALLBACK (bobgui_window_destroy), NULL);
}

static void
show_color_chooser_generic (BobguiWindow *parent)
{
  BobguiWidget *dialog;

  dialog = g_object_new (BOBGUI_TYPE_COLOR_CHOOSER_DIALOG,
                         "title", "Generic Builtin",
                         "transient-for", parent,
                         NULL);

  bobgui_window_present (BOBGUI_WINDOW (dialog));
  g_signal_connect (dialog, "response", G_CALLBACK (bobgui_window_destroy), NULL);
}

static void
add_content (BobguiWidget *dialog)
{
  BobguiWidget *label;

  label = bobgui_label_new ("content");
  bobgui_widget_set_margin_start (label, 50);
  bobgui_widget_set_margin_end (label, 50);
  bobgui_widget_set_margin_top (label, 50);
  bobgui_widget_set_margin_bottom (label, 50);
  bobgui_widget_set_hexpand (label, TRUE);
  bobgui_widget_set_vexpand (label, TRUE);

  bobgui_box_append (BOBGUI_BOX (bobgui_dialog_get_content_area (BOBGUI_DIALOG (dialog))), label);
}

static void
add_buttons (BobguiWidget *dialog)
{
  bobgui_dialog_add_button (BOBGUI_DIALOG (dialog), "Done", BOBGUI_RESPONSE_OK);
  bobgui_dialog_set_default_response (BOBGUI_DIALOG (dialog), BOBGUI_RESPONSE_OK);
}

static void
show_dialog (BobguiWindow *parent)
{
  BobguiWidget *dialog;

  dialog = bobgui_dialog_new_with_buttons ("Simple", parent, 
					BOBGUI_DIALOG_MODAL|BOBGUI_DIALOG_DESTROY_WITH_PARENT,
				        "Close", BOBGUI_RESPONSE_CLOSE,
                                        NULL);

  add_content (dialog);
  bobgui_window_present (BOBGUI_WINDOW (dialog));
  g_signal_connect (dialog, "response", G_CALLBACK (bobgui_window_destroy), NULL);
}

static void
show_dialog_with_header (BobguiWindow *parent)
{
  BobguiWidget *dialog;

  dialog = bobgui_dialog_new_with_buttons ("With Header", parent, 
					BOBGUI_DIALOG_MODAL|BOBGUI_DIALOG_DESTROY_WITH_PARENT|BOBGUI_DIALOG_USE_HEADER_BAR,
				        "Close", BOBGUI_RESPONSE_CLOSE,
                                        NULL);

  add_content (dialog);
  bobgui_window_present (BOBGUI_WINDOW (dialog));
  g_signal_connect (dialog, "response", G_CALLBACK (bobgui_window_destroy), NULL);
}

static void
show_dialog_with_buttons (BobguiWindow *parent)
{
  BobguiWidget *dialog;

  dialog = bobgui_dialog_new_with_buttons ("With Buttons", parent, 
					BOBGUI_DIALOG_MODAL|BOBGUI_DIALOG_DESTROY_WITH_PARENT,
				        "Close", BOBGUI_RESPONSE_CLOSE,
				        "Frob", 25,
                                        NULL);

  add_content (dialog);
  bobgui_window_present (BOBGUI_WINDOW (dialog));
  g_signal_connect (dialog, "response", G_CALLBACK (bobgui_window_destroy), NULL);
}

static void
show_dialog_with_header_buttons (BobguiWindow *parent)
{
  BobguiWidget *dialog;

  dialog = bobgui_dialog_new_with_buttons ("Header & Buttons", parent, 
					BOBGUI_DIALOG_MODAL|BOBGUI_DIALOG_DESTROY_WITH_PARENT|BOBGUI_DIALOG_USE_HEADER_BAR,
				        "Close", BOBGUI_RESPONSE_CLOSE,
				        "Frob", 25,
                                        NULL);

  add_content (dialog);
  bobgui_window_present (BOBGUI_WINDOW (dialog));
  g_signal_connect (dialog, "response", G_CALLBACK (bobgui_window_destroy), NULL);
}

static void
show_dialog_with_header_buttons2 (BobguiWindow *parent)
{
  BobguiBuilder *builder;
  BobguiWidget *dialog;

  builder = bobgui_builder_new_from_file ("dialog.ui");
  dialog = (BobguiWidget *)bobgui_builder_get_object (builder, "dialog");
  g_object_unref (builder);
  bobgui_window_present (BOBGUI_WINDOW (dialog));
  g_signal_connect (dialog, "response", G_CALLBACK (bobgui_window_destroy), NULL);
}

typedef struct {
  BobguiDialog parent;
} MyDialog;

typedef struct {
  BobguiDialogClass parent_class;
} MyDialogClass;

static GType my_dialog_get_type (void);
G_DEFINE_TYPE (MyDialog, my_dialog, BOBGUI_TYPE_DIALOG);

static void
my_dialog_init (MyDialog *dialog)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (dialog));
}

static void
my_dialog_class_init (MyDialogClass *class)
{
  char *buffer;
  gsize size;
  GBytes *bytes;

  if (!g_file_get_contents ("mydialog.ui", &buffer, &size, NULL))
    g_error ("Template file mydialog.ui not found");

  bytes = g_bytes_new_static (buffer, size);
  bobgui_widget_class_set_template (BOBGUI_WIDGET_CLASS (class), bytes);
  g_bytes_unref (bytes);
}

static void
show_dialog_from_template (BobguiWindow *parent)
{
  BobguiWidget *dialog;

  dialog = g_object_new (my_dialog_get_type (),
                         "title", "Template",
                         "transient-for", parent,
                         NULL);

  add_content (dialog);
  bobgui_window_present (BOBGUI_WINDOW (dialog));
  g_signal_connect (dialog, "response", G_CALLBACK (bobgui_window_destroy), NULL);
}

static void
show_dialog_flex_template (BobguiWindow *parent)
{
  BobguiWidget *dialog;
  gboolean use_header;

  g_object_get (bobgui_settings_get_default (),
                "bobgui-dialogs-use-header", &use_header,
                NULL);
  dialog = g_object_new (my_dialog_get_type (),
                         "title", "Flexible Template",
                         "transient-for", parent,
                         "use-header-bar", use_header,
                         NULL);

  add_content (dialog);
  bobgui_window_present (BOBGUI_WINDOW (dialog));
  g_signal_connect (dialog, "response", G_CALLBACK (bobgui_window_destroy), NULL);
}

typedef struct {
  BobguiDialog parent;

  BobguiWidget *content;
} MyDialog2;

typedef struct {
  BobguiDialogClass parent_class;
} MyDialog2Class;

static GType my_dialog2_get_type (void);
G_DEFINE_TYPE (MyDialog2, my_dialog2, BOBGUI_TYPE_DIALOG);

static void
my_dialog2_init (MyDialog2 *dialog)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (dialog));
}

static void
my_dialog2_class_init (MyDialog2Class *class)
{
  char *buffer;
  gsize size;
  GBytes *bytes;

  if (!g_file_get_contents ("mydialog2.ui", &buffer, &size, NULL))
    g_error ("Template file mydialog2.ui not found");

  bytes = g_bytes_new_static (buffer, size);
  bobgui_widget_class_set_template (BOBGUI_WIDGET_CLASS (class), bytes);
  g_bytes_unref (bytes);

  bobgui_widget_class_bind_template_child (BOBGUI_WIDGET_CLASS (class), MyDialog2, content);
}

static void
show_dialog_from_template_with_header (BobguiWindow *parent)
{
  BobguiWidget *dialog;

  dialog = g_object_new (my_dialog2_get_type (),
                         "transient-for", parent,
                         "use-header-bar", TRUE,
                         NULL);

  add_buttons (dialog);
  add_content (dialog);
  bobgui_window_present (BOBGUI_WINDOW (dialog));
  g_signal_connect (dialog, "response", G_CALLBACK (bobgui_window_destroy), NULL);
}

int
main (int argc, char *argv[])
{
  BobguiWidget *window;
  BobguiWidget *vbox;
  BobguiWidget *box;
  BobguiWidget *button;

#ifdef BOBGUI_SRCDIR
  g_chdir (BOBGUI_SRCDIR);
#endif

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 600, 400);

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);
  bobgui_widget_set_halign (vbox, BOBGUI_ALIGN_FILL);
  bobgui_widget_set_valign (vbox, BOBGUI_ALIGN_CENTER);
  bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);
  
  box = bobgui_flow_box_new ();
  bobgui_flow_box_set_selection_mode (BOBGUI_FLOW_BOX (box), BOBGUI_SELECTION_NONE);
  bobgui_widget_set_hexpand (box, TRUE);
  bobgui_box_append (BOBGUI_BOX (vbox), box);

  button = bobgui_button_new_with_label ("Message dialog");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_message_dialog1), window);
  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (box), button, -1);

  button = bobgui_button_new_with_label ("Message with icon");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_message_dialog1a), window);
  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (box), button, -1);

  button = bobgui_button_new_with_label ("Confirmation dialog");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_message_dialog2), window);
  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (box), button, -1);

  button = bobgui_button_new_with_label ("Builtin");
  button = bobgui_button_new_with_label ("Builtin");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_color_chooser), window);
  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (box), button, -1);

  button = bobgui_button_new_with_label ("Generic Builtin");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_color_chooser_generic), window);
  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (box), button, -1);

  button = bobgui_button_new_with_label ("Simple");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_dialog), window);
  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (box), button, -1);

  button = bobgui_button_new_with_label ("With Header");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_dialog_with_header), window);
  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (box), button, -1);

  button = bobgui_button_new_with_label ("With Buttons");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_dialog_with_buttons), window);
  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (box), button, -1);

  button = bobgui_button_new_with_label ("Header & Buttons");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_dialog_with_header_buttons), window);
  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (box), button, -1);

  button = bobgui_button_new_with_label ("Header & Buttons & Builder");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_dialog_with_header_buttons2), window);
  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (box), button, -1);

  button = bobgui_button_new_with_label ("Template");
  button = bobgui_button_new_with_label ("Template");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_dialog_from_template), window);
  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (box), button, -1);

  button = bobgui_button_new_with_label ("Template With Header");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_dialog_from_template_with_header), window);
  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (box), button, -1);

  button = bobgui_button_new_with_label ("Flexible Template");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (show_dialog_flex_template), window);
  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (box), button, -1);

  button = bobgui_check_button_new_with_label ("Dialogs have headers");
  g_object_bind_property (bobgui_settings_get_default (), "bobgui-dialogs-use-header",
                          button, "active",
                          G_BINDING_BIDIRECTIONAL|G_BINDING_SYNC_CREATE);
  bobgui_widget_set_halign (button, BOBGUI_ALIGN_CENTER);
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  button = bobgui_spinner_new ();
  bobgui_spinner_start (BOBGUI_SPINNER (button));
  bobgui_widget_set_halign (button, BOBGUI_ALIGN_CENTER);
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  while (TRUE)
    g_main_context_iteration (NULL, TRUE);
  
  return 0;
}

