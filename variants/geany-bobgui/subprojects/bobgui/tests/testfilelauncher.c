#include <bobgui/bobgui.h>

static void
launched_cb (GObject *source,
             GAsyncResult *result,
             gpointer data)
{
  BobguiFileLauncher *launcher = BOBGUI_FILE_LAUNCHER (source);
  GError *error = NULL;

  if (!bobgui_file_launcher_launch_finish (launcher, result, &error))
    {
      g_print ("Launching failed: %s\n", error->message);
      g_error_free (error);
    }
}

int
main (int argc, char *argv[])
{
  BobguiWidget *window;
  BobguiFileLauncher *launcher;

  bobgui_init ();

  window = bobgui_window_new ();

  launcher = bobgui_file_launcher_new (NULL);

  bobgui_window_present (BOBGUI_WINDOW (window));

  for (int i = 1; i < argc; i++)
    {
      GFile *file = g_file_new_for_commandline_arg (argv[i]);

      g_print ("launching %s\n", argv[i]);

      bobgui_file_launcher_set_file (launcher, file);
      bobgui_file_launcher_launch (launcher, BOBGUI_WINDOW (window), NULL, launched_cb, NULL);
      g_object_unref (file);
    }

  while (g_list_model_get_n_items (bobgui_window_get_toplevels ()) > 0)
    g_main_context_iteration (NULL, FALSE);

  return 0;
}
