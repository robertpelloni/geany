/* Lists/Words
 * #Keywords: BobguiListView, BobguiFilterListModel, BobguiInscription
 *
 * This demo shows filtering a long list - of words.
 *
 * You should have the file `/usr/share/dict/words` installed for
 * this demo to work.
 */

#include <bobgui/bobgui.h>

static BobguiWidget *window = NULL;
static BobguiWidget *progress;

const char *factory_text =
"<?xml version='1.0' encoding='UTF-8'?>\n"
"<interface>\n"
"  <template class='BobguiListItem'>\n"
"    <property name='child'>\n"
"      <object class='BobguiInscription'>\n"
"        <property name='xalign'>0</property>\n"
"        <binding name='text'>\n"
"          <lookup name='string' type='BobguiStringObject'>\n"
"            <lookup name='item'>BobguiListItem</lookup>\n"
"          </lookup>\n"
"        </binding>\n"
"      </object>\n"
"    </property>\n"
"  </template>\n"
"</interface>\n";

static void
update_title_cb (BobguiFilterListModel *model)
{
  guint total;
  char *title;
  guint pending;

  total = g_list_model_get_n_items (bobgui_filter_list_model_get_model (model));
  pending = bobgui_filter_list_model_get_pending (model);

  title = g_strdup_printf ("%u lines", g_list_model_get_n_items (G_LIST_MODEL (model)));

  bobgui_widget_set_visible (progress, pending != 0);
  bobgui_progress_bar_set_fraction (BOBGUI_PROGRESS_BAR (progress), total > 0 ? (total - pending) / (double) total : 0.);
  bobgui_window_set_title (BOBGUI_WINDOW (window), title);
  g_free (title);
}

static void
read_lines_cb (GObject      *object,
               GAsyncResult *result,
               gpointer      data)
{
  GBufferedInputStream *stream = G_BUFFERED_INPUT_STREAM (object);
  BobguiStringList *stringlist = data;
  GError *error = NULL;
  gsize size;
  GPtrArray *lines;
  gssize n_filled;
  const char *buffer, *newline;

  n_filled = g_buffered_input_stream_fill_finish (stream, result, &error);
  if (n_filled < 0)
    {
      g_print ("Could not read data: %s\n", error->message);
      g_clear_error (&error);
      g_object_unref (stringlist);
      return;
    }

  buffer = g_buffered_input_stream_peek_buffer (stream, &size);

  if (n_filled == 0)
    {
      if (size)
        bobgui_string_list_take (stringlist, g_utf8_make_valid (buffer, size));
      g_object_unref (stringlist);
      return;
    }

  lines = NULL;
  while ((newline = memchr (buffer, '\n', size)))
    {
      if (newline > buffer)
        {
          if (lines == NULL)
            lines = g_ptr_array_new_with_free_func (g_free);
          g_ptr_array_add (lines, g_utf8_make_valid (buffer, newline - buffer));
        }
      if (g_input_stream_skip (G_INPUT_STREAM (stream), newline - buffer + 1, NULL, &error) < 0)
        {
          g_clear_error (&error);
          break;
        }
      buffer = g_buffered_input_stream_peek_buffer (stream, &size);
    }
  if (lines == NULL)
    {
      g_buffered_input_stream_set_buffer_size (stream, g_buffered_input_stream_get_buffer_size (stream) + 4096);
    }
  else
    {
      g_ptr_array_add (lines, NULL);
      bobgui_string_list_splice (stringlist, g_list_model_get_n_items (G_LIST_MODEL (stringlist)), 0, (const char **) lines->pdata);
      g_ptr_array_free (lines, TRUE);
    }

  g_buffered_input_stream_fill_async (stream, -1, G_PRIORITY_HIGH_IDLE, NULL, read_lines_cb, data);
}
               
static void
file_is_open_cb (GObject      *file,
                 GAsyncResult *result,
                 gpointer      data)
{
  GError *error = NULL;
  GFileInputStream *file_stream;
  GBufferedInputStream *stream;

  file_stream = g_file_read_finish (G_FILE (file), result, &error);
  if (file_stream == NULL)
    {
      g_print ("Could not open file: %s\n", error->message);
      g_error_free (error);
      g_object_unref (data);
      return;
    }

  stream = G_BUFFERED_INPUT_STREAM (g_buffered_input_stream_new (G_INPUT_STREAM (file_stream)));
  g_buffered_input_stream_fill_async (stream, -1, G_PRIORITY_HIGH_IDLE, NULL, read_lines_cb, data);
  g_object_unref (stream);
}

static void
load_file (BobguiStringList *list,
           GFile         *file)
{
  bobgui_string_list_splice (list, 0, g_list_model_get_n_items (G_LIST_MODEL (list)), NULL);
  g_file_read_async (file, G_PRIORITY_HIGH_IDLE, NULL, file_is_open_cb, g_object_ref (list));
}

static void
open_response_cb (GObject *source,
                  GAsyncResult *result,
                  void *user_data)
{
  BobguiFileDialog *dialog = BOBGUI_FILE_DIALOG (source);
  BobguiStringList *stringlist = BOBGUI_STRING_LIST (user_data);
  GFile *file;

  file = bobgui_file_dialog_open_finish (dialog, result, NULL);
  if (file)
    {
      load_file (stringlist, file);
      g_object_unref (file);
    }
}

static void
file_open_cb (BobguiWidget     *button,
              BobguiStringList *stringlist)
{
  BobguiFileDialog *dialog;

  dialog = bobgui_file_dialog_new ();
  bobgui_file_dialog_open (dialog,
                        BOBGUI_WINDOW (bobgui_widget_get_root (button)),
                        NULL,
                        open_response_cb, stringlist);
  g_object_unref (dialog);
}

BobguiWidget *
do_listview_words (BobguiWidget *do_widget)
{
  if (window == NULL)
    {
      BobguiWidget *header, *listview, *sw, *vbox, *search_entry, *open_button, *overlay;
      BobguiFilterListModel *filter_model;
      BobguiStringList *stringlist;
      BobguiFilter *filter;
      GFile *file;

      file = g_file_new_for_path ("/usr/share/dict/words");
      if (g_file_query_exists (file, NULL))
        {
          stringlist = bobgui_string_list_new (NULL);
          load_file (stringlist, file);
        }
      else
        {
          char **words;
          words = g_strsplit ("lorem ipsum dolor sit amet consectetur adipisci elit sed eiusmod tempor incidunt labore et dolore magna aliqua ut enim ad minim veniam quis nostrud exercitation ullamco laboris nisi ut aliquid ex ea commodi consequat", " ", -1);
          stringlist = bobgui_string_list_new ((const char **) words);
          g_strfreev (words);
        }
      g_object_unref (file);

      filter = BOBGUI_FILTER (bobgui_string_filter_new (bobgui_property_expression_new (BOBGUI_TYPE_STRING_OBJECT, NULL, "string")));
      filter_model = bobgui_filter_list_model_new (G_LIST_MODEL (stringlist), filter);
      bobgui_filter_list_model_set_incremental (filter_model, TRUE);

      window = bobgui_window_new ();
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 400, 600);

      header = bobgui_header_bar_new ();
      bobgui_header_bar_set_show_title_buttons (BOBGUI_HEADER_BAR (header), TRUE);
      open_button = bobgui_button_new_with_mnemonic ("_Open");
      g_signal_connect (open_button, "clicked", G_CALLBACK (file_open_cb), stringlist);
      bobgui_header_bar_pack_start (BOBGUI_HEADER_BAR (header), open_button);
      bobgui_window_set_titlebar (BOBGUI_WINDOW (window), header);

      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer*)&window);

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

      search_entry = bobgui_search_entry_new ();
      g_object_bind_property (search_entry, "text", filter, "search", 0);
      bobgui_box_append (BOBGUI_BOX (vbox), search_entry);

      overlay = bobgui_overlay_new ();
      bobgui_box_append (BOBGUI_BOX (vbox), overlay);

      progress = bobgui_progress_bar_new ();
      bobgui_widget_set_halign (progress, BOBGUI_ALIGN_FILL);
      bobgui_widget_set_valign (progress, BOBGUI_ALIGN_START);
      bobgui_widget_set_hexpand (progress, TRUE);
      bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), progress);

      sw = bobgui_scrolled_window_new ();
      bobgui_widget_set_vexpand (sw, TRUE);
      bobgui_overlay_set_child (BOBGUI_OVERLAY (overlay), sw);

      listview = bobgui_list_view_new (
          BOBGUI_SELECTION_MODEL (bobgui_no_selection_new (G_LIST_MODEL (filter_model))),
          bobgui_builder_list_item_factory_new_from_bytes (NULL,
              g_bytes_new_static (factory_text, strlen (factory_text))));
      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), listview);

      g_signal_connect (filter_model, "items-changed", G_CALLBACK (update_title_cb), progress);
      g_signal_connect (filter_model, "notify::pending", G_CALLBACK (update_title_cb), progress);
      update_title_cb (filter_model);

    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
