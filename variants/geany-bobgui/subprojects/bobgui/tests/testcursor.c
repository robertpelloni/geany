
#include <bobgui/bobgui.h>

static const char *names[] = {
  "none",
  "default",
  "help",
  "pointer",
  "context-menu",
  "progress",
  "wait",
  "cell",
  "crosshair",
  "text",
  "vertical-text",
  "alias",
  "copy",
  "move",
  "dnd-ask",
  "no-drop",
  "not-allowed",
  "grab",
  "grabbing",
  "n-resize",
  "e-resize",
  "s-resize",
  "w-resize",
  "ne-resize",
  "nw-resize",
  "sw-resize",
  "se-resize",
  "col-resize",
  "row-resize",
  "ew-resize",
  "ns-resize",
  "nesw-resize",
  "nwse-resize",
  "all-resize",
  "all-scroll",
  "zoom-in",
  "zoom-out"
};

static int count = 0;

static gboolean
change_cursor (gpointer data)
{
  BobguiWidget *window = data;
  BobguiWidget *label;
  char buffer[128];

  bobgui_widget_set_cursor_from_name (window, names[count % G_N_ELEMENTS (names)]);
  count++;

  label = bobgui_window_get_child (BOBGUI_WINDOW (window));
  g_snprintf (buffer, sizeof (buffer), "%d", count);
  bobgui_label_set_label (BOBGUI_LABEL (label), buffer);

  return G_SOURCE_CONTINUE;
}

int
main (int argc, char *argv[])
{
  BobguiWidget *window, *label;
  gboolean done = FALSE;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_window_set_resizable (BOBGUI_WINDOW (window), TRUE);
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 600, 400);

  label = bobgui_label_new ("");
  bobgui_window_set_child (BOBGUI_WINDOW (window), label);

  g_timeout_add (4, change_cursor, window);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
