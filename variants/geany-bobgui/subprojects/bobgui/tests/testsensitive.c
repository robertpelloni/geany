#include <bobgui/bobgui.h>

static void
set_insensitive (BobguiButton *b, BobguiWidget *w)
{
  bobgui_widget_set_sensitive (w, FALSE);
}

static void
state_flags_changed (BobguiWidget *widget)
{
  BobguiStateFlags flags;
  const char *sep;

  g_print ("state changed: \n");

  flags = bobgui_widget_get_state_flags (widget);
  sep = "";
  if (flags & BOBGUI_STATE_FLAG_ACTIVE)
    {
      g_print ("%sactive", sep);
      sep = "|";
    }
  if (flags & BOBGUI_STATE_FLAG_PRELIGHT)
    {
      g_print ("%sprelight", sep);
      sep = "|";
    }
  if (flags & BOBGUI_STATE_FLAG_SELECTED)
    {
      g_print ("%sselected", sep);
      sep = "|";
    }
  if (flags & BOBGUI_STATE_FLAG_INSENSITIVE)
    {
      g_print ("%sinsensitive", sep);
      sep = "|";
    }
  if (flags & BOBGUI_STATE_FLAG_INCONSISTENT)
    {
      g_print ("%sinconsistent", sep);
      sep = "|";
    }
  if (flags & BOBGUI_STATE_FLAG_FOCUSED)
    {
      g_print ("%sfocused", sep);
      sep = "|";
    }
  if (sep[0] == 0)
    g_print ("normal");
  g_print ("\n");
}

int main (int argc, char *argv[])
{
  BobguiWidget *window;
  BobguiWidget *box;
  BobguiWidget *bu;
  BobguiWidget *w, *c;

  bobgui_init ();

  window = bobgui_window_new ();
  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  w = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 15);
  bobgui_box_append (BOBGUI_BOX (box), w);
  bobgui_box_append (BOBGUI_BOX (w), bobgui_entry_new ());
  bu = bobgui_button_new_with_label ("Bu");
  bobgui_box_append (BOBGUI_BOX (w), bu);
  c = bobgui_switch_new ();
  bobgui_switch_set_active (BOBGUI_SWITCH (c), TRUE);
  bobgui_widget_set_halign (c, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (c, BOBGUI_ALIGN_CENTER);
  bobgui_box_append (BOBGUI_BOX (box), c);
  g_signal_connect (bu, "clicked", G_CALLBACK (set_insensitive), w);
  g_signal_connect (bu, "state-flags-changed", G_CALLBACK (state_flags_changed), NULL);

  g_object_bind_property (c, "active", w, "sensitive", G_BINDING_BIDIRECTIONAL);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
