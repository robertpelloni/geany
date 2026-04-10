#include <bobgui/bobgui.h>

static void
ended (GObject *object)
{
  g_object_unref (object);
}

static void
play (const char *name)
{
  char *path;
  BobguiMediaStream *stream;

  path = g_build_filename ("tests", name, NULL);

  stream = bobgui_media_file_new_for_filename (path);
  bobgui_media_stream_set_volume (stream, 1.0);

  bobgui_media_stream_play (stream);

  g_signal_connect (stream, "notify::ended", G_CALLBACK (ended), NULL);

  g_free (path);
}

static void
enter (BobguiButton *button)
{
  play ("service-login.oga");
}

static void
leave (BobguiButton *button)
{
  play ("service-logout.oga");
}

int main (int argc, char *argv[])
{
  BobguiWidget *window;
  BobguiWidget *box;
  BobguiWidget *button;

  bobgui_init ();

  window = bobgui_window_new ();

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  bobgui_widget_set_halign (box, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (box, BOBGUI_ALIGN_CENTER);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  button = bobgui_button_new_with_label ("Α");
  g_signal_connect (button, "clicked", G_CALLBACK (enter), NULL);
  bobgui_box_append (BOBGUI_BOX (box), button);

  button = bobgui_button_new_with_label ("Ω");
  g_signal_connect (button, "clicked", G_CALLBACK (leave), NULL);
  bobgui_box_append (BOBGUI_BOX (box), button);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (1)
    g_main_context_iteration (NULL, FALSE);

  return 0;
}
