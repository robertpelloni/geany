/* Benchmark/Themes
 *
 * This demo continuously switches themes, like some of you.
 *
 * Warning: This demo involves rapidly flashing changes and may
 * be hazardous to photosensitive viewers.
 */

#include <bobgui/bobgui.h>

static guint tick_cb;

typedef struct {
  const char *name;
  gboolean dark;
} Theme;

static Theme themes[] = {
  { "Adwaita", FALSE },
  { "Adwaita", TRUE },
  { "HighContrast", FALSE },
  { "HighContrastInverse", FALSE }
};

static int theme;

static gboolean
change_theme (BobguiWidget     *widget,
              GdkFrameClock *frame_clock,
              gpointer       data)
{
  BobguiWidget *label = data;
  Theme next = themes[theme++ % G_N_ELEMENTS (themes)];
  char *name;

  g_object_set (bobgui_settings_get_default (),
                "bobgui-theme-name", next.name,
                "bobgui-application-prefer-dark-theme", next.dark,
                NULL);

  name = g_strconcat (next.name, next.dark ? " (dark)" : NULL, NULL);
  bobgui_window_set_title (BOBGUI_WINDOW (widget), name);
  g_free (name);

  if (frame_clock)
    {
      char *fps;

      fps = g_strdup_printf ("%.2f fps", gdk_frame_clock_get_fps (frame_clock));
      bobgui_label_set_label (BOBGUI_LABEL (label), fps);
      g_free (fps);
    }
  else
    bobgui_label_set_label (BOBGUI_LABEL (label), "");

  return G_SOURCE_CONTINUE;
}

static void
toggle_cycle (GObject    *button,
              GParamSpec *pspec,
              gpointer    data)
{
  BobguiWidget *warning = data;
  gboolean active;
  BobguiWidget *window;

  g_object_get (button, "active", &active, NULL);

  window = bobgui_widget_get_ancestor (BOBGUI_WIDGET (button), BOBGUI_TYPE_WINDOW);

  if (active && !tick_cb)
    {
      bobgui_window_present (BOBGUI_WINDOW (warning));
    }
  else if (!active && tick_cb)
    {
      bobgui_widget_remove_tick_callback (window, tick_cb);
      tick_cb = 0;
    }
}

static void
warning_closed (BobguiDialog *warning,
                int        response_id,
                gpointer   data)
{
  BobguiWidget *window;
  BobguiWidget *button;

  bobgui_widget_set_visible (BOBGUI_WIDGET (warning), FALSE);

  window = bobgui_widget_get_ancestor (BOBGUI_WIDGET (data), BOBGUI_TYPE_WINDOW);
  button = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (window), "button"));

  if (response_id == BOBGUI_RESPONSE_OK)
    tick_cb = bobgui_widget_add_tick_callback (window, change_theme, data, NULL);
  else
    bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (button), FALSE);
}

BobguiWidget *
do_themes (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiBuilder *builder;
      BobguiWidget *button;
      BobguiWidget *label;
      BobguiWidget *warning;

      builder = bobgui_builder_new_from_resource ("/themes/themes.ui");
      window = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "window"));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));

      label = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "fps"));
      warning = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "warning"));
      g_signal_connect (warning, "response", G_CALLBACK (warning_closed), label);

      button = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "toggle"));
      g_object_set_data (G_OBJECT (window), "button", button);
      g_signal_connect (button, "notify::active", G_CALLBACK (toggle_cycle), warning);
      bobgui_widget_realize (window);

      g_object_unref (builder);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
