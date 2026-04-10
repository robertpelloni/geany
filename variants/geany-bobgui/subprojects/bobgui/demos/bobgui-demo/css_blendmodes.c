/* Theming/CSS Blend Modes
 *
 * You can blend multiple backgrounds using the CSS blend modes available.
 */

#include <bobgui/bobgui.h>

#define WID(x) ((BobguiWidget*) bobgui_builder_get_object (builder, x))

/*
 * These are the available blend modes.
 */
struct {
  const char *name;
  const char *id;
} blend_modes[] =
{
  { "Color", "color" },
  { "Color (burn)", "color-burn" },
  { "Color (dodge)", "color-dodge" },
  { "Darken", "darken" },
  { "Difference", "difference" },
  { "Exclusion", "exclusion" },
  { "Hard Light", "hard-light" },
  { "Hue", "hue" },
  { "Lighten", "lighten" },
  { "Luminosity", "luminosity" },
  { "Multiply", "multiply" },
  { "Normal", "normal" },
  { "Overlay", "overlay" },
  { "Saturate", "saturation" },
  { "Screen", "screen" },
  { "Soft Light", "soft-light" },
  { NULL }
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
static void
update_css_for_blend_mode (BobguiCssProvider *provider,
                           const char     *blend_mode)
{
  GBytes *bytes;
  char *css;

  bytes = g_resources_lookup_data ("/css_blendmodes/css_blendmodes.css", 0, NULL);

  css = g_strdup_printf ((char *) g_bytes_get_data (bytes, NULL),
                         blend_mode,
                         blend_mode,
                         blend_mode);

  bobgui_css_provider_load_from_string (provider, css);

  g_bytes_unref (bytes);
  g_free (css);
}
#pragma GCC diagnostic pop

static void
row_activated (BobguiListBox     *listbox,
               BobguiListBoxRow  *row,
               BobguiCssProvider *provider)
{
  const char *blend_mode;

  blend_mode = blend_modes[bobgui_list_box_row_get_index (row)].id;

  update_css_for_blend_mode (provider, blend_mode);
}

static void
setup_listbox (BobguiBuilder       *builder,
               BobguiStyleProvider *provider)
{
  BobguiWidget *normal_row;
  BobguiWidget *listbox;
  int i;

  normal_row = NULL;
  listbox = bobgui_list_box_new ();
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (WID ("scrolledwindow")), listbox);

  g_signal_connect (listbox, "row-activated", G_CALLBACK (row_activated), provider);

  /* Add a row for each blend mode available */
  for (i = 0; blend_modes[i].name != NULL; i++)
    {
      BobguiWidget *label;
      BobguiWidget *row;

      row = bobgui_list_box_row_new ();
      label = g_object_new (BOBGUI_TYPE_LABEL,
                            "label", blend_modes[i].name,
                            "xalign", 0.0,
                            NULL);

      bobgui_list_box_row_set_child (BOBGUI_LIST_BOX_ROW (row), label);
      bobgui_list_box_insert (BOBGUI_LIST_BOX (listbox), row, -1);

      /* The first selected row is "normal" */
      if (g_strcmp0 (blend_modes[i].id, "normal") == 0)
        normal_row = row;
    }

  /* Select the "normal" row */
  bobgui_list_box_select_row (BOBGUI_LIST_BOX (listbox), BOBGUI_LIST_BOX_ROW (normal_row));
  g_signal_emit_by_name (G_OBJECT (normal_row), "activate");

  bobgui_widget_grab_focus (normal_row);
}

BobguiWidget *
do_css_blendmodes (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiStyleProvider *provider;
      BobguiBuilder *builder;

      builder = bobgui_builder_new_from_resource ("/css_blendmodes/blendmodes.ui");

      window = WID ("window");
      bobgui_window_set_transient_for (BOBGUI_WINDOW (window), BOBGUI_WINDOW (do_widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      /* Setup the CSS provider for window */
      provider = BOBGUI_STYLE_PROVIDER (bobgui_css_provider_new ());

      bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                                  provider,
                                                  BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);

      setup_listbox (builder, provider);

      g_object_unref (builder);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
