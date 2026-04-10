#include <stdlib.h>
#include <bobgui/bobgui.h>

static gboolean
boolean_to_text (GBinding *binding,
                 const GValue *source,
                 GValue *target,
                 gpointer dummy G_GNUC_UNUSED)
{
  if (g_value_get_boolean (source))
    g_value_set_string (target, "Enabled");
  else
    g_value_set_string (target, "Disabled");

  return TRUE;
}

static BobguiWidget *
make_switch (gboolean is_on,
             gboolean is_sensitive)
{
  BobguiWidget *hbox;
  BobguiWidget *sw, *label;

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 6);

  sw = bobgui_switch_new ();
  bobgui_switch_set_active (BOBGUI_SWITCH (sw), is_on);
  bobgui_box_append (BOBGUI_BOX (hbox), sw);
  bobgui_widget_set_sensitive (sw, is_sensitive);

  label = bobgui_label_new (is_on ? "Enabled" : "Disabled");
  bobgui_widget_set_hexpand (label, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), label);

  g_object_bind_property_full (sw, "active",
                               label, "label",
                               G_BINDING_DEFAULT,
                               boolean_to_text,
                               NULL,
                               NULL, NULL);

  return hbox;
}

typedef struct {
  BobguiSwitch *sw;
  gboolean state;
} SetStateData;

static gboolean
set_state_delayed (gpointer data)
{
  SetStateData *d = data;

  bobgui_switch_set_state (d->sw, d->state);

  g_object_set_data (G_OBJECT (d->sw), "timeout", NULL);

  return G_SOURCE_REMOVE; 
}

static gboolean
set_state (BobguiSwitch *sw, gboolean state, gpointer data)
{
  SetStateData *d;
  guint id;

  id = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (sw), "timeout"));

  if (id != 0)
    g_source_remove (id);

  d = g_new (SetStateData, 1);
  d->sw = sw;
  d->state = state;

  id = g_timeout_add_full (G_PRIORITY_DEFAULT, 2000, set_state_delayed, d, g_free);
  g_object_set_data (G_OBJECT (sw), "timeout", GUINT_TO_POINTER (id));

  return TRUE;
}

static void
sw_delay_notify (GObject *obj, GParamSpec *pspec, gpointer data)
{
  BobguiWidget *spinner = data;
  gboolean active;
  gboolean state;

  g_object_get (obj,
                "active", &active,
                "state", &state,
                NULL);

  if (active != state)
    {
      bobgui_spinner_start (BOBGUI_SPINNER (spinner));
      bobgui_widget_set_opacity (spinner, 1.0);
    }
  else
    {
      bobgui_widget_set_opacity (spinner, 0.0);
      bobgui_spinner_stop (BOBGUI_SPINNER (spinner));
    }
}

static BobguiWidget *
make_delayed_switch (gboolean is_on,
                     gboolean is_sensitive)
{
  BobguiWidget *hbox;
  BobguiWidget *sw, *label, *spinner, *check;

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 6);

  sw = bobgui_switch_new ();
  bobgui_switch_set_active (BOBGUI_SWITCH (sw), is_on);
  bobgui_box_append (BOBGUI_BOX (hbox), sw);
  bobgui_widget_set_sensitive (sw, is_sensitive);

  g_signal_connect (sw, "state-set", G_CALLBACK (set_state), NULL);

  spinner = bobgui_spinner_new ();
  bobgui_box_append (BOBGUI_BOX (hbox), spinner);
  bobgui_widget_set_opacity (spinner, 0.0);

  label = bobgui_label_new (is_on ? "Enabled" : "Disabled");
  bobgui_widget_set_hexpand (label, TRUE);
  bobgui_box_append (BOBGUI_BOX (hbox), label);

  g_object_bind_property_full (sw, "active",
                               label, "label",
                               G_BINDING_DEFAULT,
                               boolean_to_text,
                               NULL,
                               NULL, NULL);

  check = bobgui_check_button_new ();
  bobgui_box_append (BOBGUI_BOX (hbox), check);
  g_object_bind_property (sw, "state",
                          check, "active",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  g_signal_connect (sw, "notify", G_CALLBACK (sw_delay_notify), spinner);

  return hbox;
}

static void
quit_cb (BobguiWidget *widget,
         gpointer   data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

int main (int argc, char *argv[])
{
  BobguiWidget *window;
  BobguiWidget *vbox, *hbox;
  gboolean done = FALSE;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "BobguiSwitch");
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 400, -1);
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);
  bobgui_window_present (BOBGUI_WINDOW (window));

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 12);
  bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

  hbox = make_switch (FALSE, TRUE);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);

  hbox = make_switch (TRUE, TRUE);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);

  hbox = make_switch (FALSE, FALSE);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);

  hbox = make_switch (TRUE, FALSE);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);

  hbox = make_delayed_switch (FALSE, TRUE);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return EXIT_SUCCESS;
}
