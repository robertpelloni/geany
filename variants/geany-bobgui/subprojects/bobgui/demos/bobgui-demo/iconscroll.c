/* Benchmark/Scrolling
 * #Keywords: BobguiScrolledWindow
 *
 * This demo scrolls a view with various content.
 */

#include <bobgui/bobgui.h>

static guint tick_cb;
static BobguiAdjustment *hadjustment;
static BobguiAdjustment *vadjustment;
static BobguiWidget *window = NULL;
static BobguiWidget *scrolledwindow;
static int selected;

#define N_WIDGET_TYPES 12


static int hincrement = 5;
static int vincrement = 5;

static gboolean
scroll_cb (BobguiWidget *widget,
           GdkFrameClock *frame_clock,
           gpointer data)
{
  double value;

  value = bobgui_adjustment_get_value (vadjustment);
  if (value + vincrement <= bobgui_adjustment_get_lower (vadjustment) ||
     (value + vincrement >= bobgui_adjustment_get_upper (vadjustment) - bobgui_adjustment_get_page_size (vadjustment)))
    vincrement = - vincrement;

  bobgui_adjustment_set_value (vadjustment, value + vincrement);

  value = bobgui_adjustment_get_value (hadjustment);
  if (value + hincrement <= bobgui_adjustment_get_lower (hadjustment) ||
     (value + hincrement >= bobgui_adjustment_get_upper (hadjustment) - bobgui_adjustment_get_page_size (hadjustment)))
    hincrement = - hincrement;

  bobgui_adjustment_set_value (hadjustment, value + hincrement);

  return G_SOURCE_CONTINUE;
}

extern BobguiWidget *create_icon_by_id (gsize id);

static void
populate_icons (void)
{
  BobguiWidget *grid;
  int top, left;

  grid = bobgui_grid_new ();
  bobgui_widget_set_halign (grid, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_margin_start (grid, 10);
  bobgui_widget_set_margin_end (grid, 10);
  bobgui_widget_set_margin_top (grid, 10);
  bobgui_widget_set_margin_bottom (grid, 10);
  bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 10);
  bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 10);

  for (top = 0; top < 100; top++)
    for (left = 0; left < 15; left++)
      bobgui_grid_attach (BOBGUI_GRID (grid), create_icon_by_id (top * 15 + left), left, top, 1, 1);

  hincrement = 0;
  vincrement = 5;

  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolledwindow),
                                  BOBGUI_POLICY_NEVER,
                                  BOBGUI_POLICY_AUTOMATIC);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolledwindow), grid);
}

extern void fontify (const char *format, BobguiTextBuffer *buffer);

enum {
  PLAIN_TEXT,
  HIGHLIGHTED_TEXT,
  UNDERLINED_TEXT,
};

static void
underlinify (BobguiTextBuffer *buffer)
{
  BobguiTextTagTable *tags;
  BobguiTextTag *tag[3];
  BobguiTextIter start, end;

  tags = bobgui_text_buffer_get_tag_table (buffer);
  tag[0] = bobgui_text_tag_new ("error");
  tag[1] = bobgui_text_tag_new ("strikeout");
  tag[2] = bobgui_text_tag_new ("double");
  g_object_set (tag[0], "underline", PANGO_UNDERLINE_ERROR, NULL);
  g_object_set (tag[1], "strikethrough", TRUE, NULL);
  g_object_set (tag[2],
                "underline", PANGO_UNDERLINE_DOUBLE,
                "underline-rgba", &(GdkRGBA){0., 1., 1., 1. },
                NULL);
  bobgui_text_tag_table_add (tags, tag[0]);
  bobgui_text_tag_table_add (tags, tag[1]);
  bobgui_text_tag_table_add (tags, tag[2]);

  bobgui_text_buffer_get_start_iter (buffer, &end);

  while (TRUE)
    {
      bobgui_text_iter_forward_word_end (&end);
      start = end;
      bobgui_text_iter_backward_word_start (&start);
      bobgui_text_buffer_apply_tag (buffer, tag[g_random_int_range (0, 3)], &start, &end);
      if (!bobgui_text_iter_forward_word_ends (&end, 3))
        break;
    }
}

static void
populate_text (const char *resource, int kind)
{
  BobguiWidget *textview;
  BobguiTextBuffer *buffer;
  char *content;
  gsize content_len;
  GBytes *bytes;

  bytes = g_resources_lookup_data (resource, 0, NULL);
  content = g_bytes_unref_to_data (bytes, &content_len);

  buffer = bobgui_text_buffer_new (NULL);
  bobgui_text_buffer_set_text (buffer, content, (int)content_len);

  switch (kind)
    {
    case HIGHLIGHTED_TEXT:
      fontify ("c", buffer);
      break;

    case UNDERLINED_TEXT:
      underlinify (buffer);
      break;

    case PLAIN_TEXT:
    default:
      break;
    }

  textview = bobgui_text_view_new ();
  bobgui_text_view_set_buffer (BOBGUI_TEXT_VIEW (textview), buffer);

  hincrement = 0;
  vincrement = 5;

  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolledwindow),
                                  BOBGUI_POLICY_NEVER,
                                  BOBGUI_POLICY_AUTOMATIC);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolledwindow), textview);
}

static void
populate_emoji_text (void)
{
  BobguiWidget *textview;
  BobguiTextBuffer *buffer;
  GString *s;
  BobguiTextIter iter;

  s = g_string_sized_new (500 * 30 * 4);

  for (int i = 0; i < 500; i++)
    {
      if (i % 2)
        g_string_append (s, "<span underline=\"single\" underline_color=\"red\">x</span>");
      for (int j = 0; j < 30; j++)
        {
          g_string_append (s, "💓");
          g_string_append (s, "<span underline=\"single\" underline_color=\"red\">x</span>");
        }
      g_string_append (s, "\n");
    }

  buffer = bobgui_text_buffer_new (NULL);
  bobgui_text_buffer_get_start_iter (buffer, &iter);
  bobgui_text_buffer_insert_markup (buffer, &iter, s->str, s->len);

  g_string_free (s, TRUE);

  textview = bobgui_text_view_new ();
  bobgui_text_view_set_buffer (BOBGUI_TEXT_VIEW (textview), buffer);

  hincrement = 0;
  vincrement = 5;

  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolledwindow),
                                  BOBGUI_POLICY_NEVER,
                                  BOBGUI_POLICY_AUTOMATIC);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolledwindow), textview);
}

static void
populate_image (void)
{
  BobguiWidget *image;

  image = bobgui_picture_new_for_resource ("/sliding_puzzle/portland-rose.jpg");
  bobgui_picture_set_can_shrink (BOBGUI_PICTURE (image), FALSE);

  hincrement = 5;
  vincrement = 5;

  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolledwindow),
                                  BOBGUI_POLICY_AUTOMATIC,
                                  BOBGUI_POLICY_AUTOMATIC);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolledwindow), image);
}

extern BobguiWidget *create_weather_view (void);

static void
populate_list (void)
{
  BobguiWidget *list;

  list = create_weather_view ();

  hincrement = 5;
  vincrement = 0;

  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolledwindow),
                                  BOBGUI_POLICY_AUTOMATIC,
                                  BOBGUI_POLICY_AUTOMATIC);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolledwindow), list);
}

extern BobguiWidget *create_color_grid (void);
extern GListModel *bobgui_color_list_new (guint size);

static void
populate_grid (void)
{
  BobguiWidget *list;
  BobguiNoSelection *selection;

  list = create_color_grid ();

  selection = bobgui_no_selection_new (bobgui_color_list_new (2097152));
  bobgui_grid_view_set_model (BOBGUI_GRID_VIEW (list), BOBGUI_SELECTION_MODEL (selection));
  g_object_unref (selection);

  hincrement = 0;
  vincrement = 5;

  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolledwindow),
                                  BOBGUI_POLICY_AUTOMATIC,
                                  BOBGUI_POLICY_AUTOMATIC);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolledwindow), list);
}

extern BobguiWidget *create_ucd_view (BobguiWidget *label);

static void
populate_list2 (void)
{
  BobguiWidget *list;

  list = create_ucd_view (NULL);

  hincrement = 0;
  vincrement = 5;

  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolledwindow),
                                  BOBGUI_POLICY_AUTOMATIC,
                                  BOBGUI_POLICY_AUTOMATIC);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolledwindow), list);
}

struct {
  const char *path;
  GdkPaintable *paintable;
} symbolics[] = {
  { "bookmark-new-symbolic.svg", NULL },
  { "color-select-symbolic.svg", NULL },
  { "document-open-recent-symbolic.svg", NULL },
  { "document-open-symbolic.svg", NULL },
  { "document-save-as-symbolic.svg", NULL },
  { "document-save-symbolic.svg", NULL },
  { "edit-clear-all-symbolic.svg", NULL },
  { "edit-clear-symbolic-rtl.svg", NULL },
  { "edit-clear-symbolic.svg", NULL },
  { "edit-copy-symbolic.svg", NULL },
  { "edit-cut-symbolic.svg", NULL },
  { "edit-delete-symbolic.svg", NULL },
  { "edit-find-symbolic.svg", NULL },
  { "edit-paste-symbolic.svg", NULL },
  { "edit-select-all-symbolic.svg", NULL },
  { "find-location-symbolic.svg", NULL },
  { "folder-new-symbolic.svg", NULL },
  { "function-linear-symbolic.svg", NULL },
  { "gesture-pinch-symbolic.svg", NULL },
  { "gesture-rotate-anticlockwise-symbolic.svg", NULL },
  { "gesture-rotate-clockwise-symbolic.svg", NULL },
  { "gesture-stretch-symbolic.svg", NULL },
  { "gesture-swipe-left-symbolic.svg", NULL },
  { "gesture-swipe-right-symbolic.svg", NULL },
  { "gesture-two-finger-swipe-left-symbolic.svg", NULL },
  { "gesture-two-finger-swipe-right-symbolic.svg", NULL },
  { "go-next-symbolic-rtl.svg", NULL },
  { "go-next-symbolic.svg", NULL },
  { "go-previous-symbolic-rtl.svg", NULL },
  { "go-previous-symbolic.svg", NULL },
  { "insert-image-symbolic.svg", NULL },
  { "insert-object-symbolic.svg", NULL },
  { "list-add-symbolic.svg", NULL },
  { "list-remove-all-symbolic.svg", NULL },
  { "list-remove-symbolic.svg", NULL },
  { "media-eject-symbolic.svg", NULL },
  { "media-playback-pause-symbolic.svg", NULL },
  { "media-playback-start-symbolic.svg", NULL },
  { "media-playback-stop-symbolic.svg", NULL },
  { "media-record-symbolic.svg", NULL },
  { "object-select-symbolic.svg", NULL },
  { "open-menu-symbolic.svg", NULL },
  { "pan-down-symbolic.svg", NULL },
  { "pan-end-symbolic-rtl.svg", NULL },
  { "pan-end-symbolic.svg", NULL },
  { "pan-start-symbolic-rtl.svg", NULL },
  { "pan-start-symbolic.svg", NULL },
  { "pan-up-symbolic.svg", NULL },
  { "system-run-symbolic.svg", NULL },
  { "system-search-symbolic.svg", NULL },
  { "value-decrease-symbolic.svg", NULL },
  { "value-increase-symbolic.svg", NULL },
  { "view-conceal-symbolic.svg", NULL },
  { "view-grid-symbolic.svg", NULL },
  { "view-list-symbolic.svg", NULL },
  { "view-more-symbolic.svg", NULL },
  { "view-refresh-symbolic.svg", NULL },
  { "view-reveal-symbolic.svg", NULL },
  { "window-close-symbolic.svg", NULL },
  { "window-maximize-symbolic.svg", NULL },
  { "window-minimize-symbolic.svg", NULL },
  { "window-restore-symbolic.svg", NULL },
  { "emoji-activities-symbolic.svg", NULL },
  { "emoji-body-symbolic.svg", NULL },
  { "emoji-flags-symbolic.svg", NULL },
  { "emoji-food-symbolic.svg", NULL },
  { "emoji-nature-symbolic.svg", NULL },
  { "emoji-objects-symbolic.svg", NULL },
  { "emoji-people-symbolic.svg", NULL },
  { "emoji-recent-symbolic.svg", NULL },
  { "emoji-symbols-symbolic.svg", NULL },
  { "emoji-travel-symbolic.svg", NULL },
  { "drive-harddisk-symbolic.svg", NULL },
  { "printer-symbolic.svg", NULL },
  { "emblem-important-symbolic.svg", NULL },
  { "emblem-system-symbolic.svg", NULL },
  { "face-smile-big-symbolic.svg", NULL },
  { "face-smile-symbolic.svg", NULL },
  { "application-x-executable-symbolic.svg", NULL },
  { "text-x-generic-symbolic.svg", NULL },
  { "folder-documents-symbolic.svg", NULL },
  { "folder-download-symbolic.svg", NULL },
  { "folder-music-symbolic.svg", NULL },
  { "folder-pictures-symbolic.svg", NULL },
  { "folder-publicshare-symbolic.svg", NULL },
  { "folder-remote-symbolic.svg", NULL },
  { "folder-saved-search-symbolic.svg", NULL },
  { "folder-symbolic.svg", NULL },
  { "folder-templates-symbolic.svg", NULL },
  { "folder-videos-symbolic.svg", NULL },
  { "network-server-symbolic.svg", NULL },
  { "network-workgroup-symbolic.svg", NULL },
  { "user-desktop-symbolic.svg", NULL },
  { "user-home-symbolic.svg", NULL },
  { "user-trash-symbolic.svg", NULL },
  { "audio-volume-high-symbolic.svg", NULL },
  { "audio-volume-low-symbolic.svg", NULL },
  { "audio-volume-medium-symbolic.svg", NULL },
  { "audio-volume-muted-symbolic.svg", NULL },
  { "caps-lock-symbolic.svg", NULL },
  { "changes-allow-symbolic.svg", NULL },
  { "changes-prevent-symbolic.svg", NULL },
  { "dialog-error-symbolic.svg", NULL },
  { "dialog-information-symbolic.svg", NULL },
  { "dialog-password-symbolic.svg", NULL },
  { "dialog-question-symbolic.svg", NULL },
  { "dialog-warning-symbolic.svg", NULL },
  { "display-brightness-symbolic.svg", NULL },
  { "media-playlist-repeat-symbolic.svg", NULL },
  { "orientation-landscape-inverse-symbolic.svg", NULL },
  { "orientation-landscape-symbolic.svg", NULL },
  { "orientation-portrait-inverse-symbolic.svg", NULL },
  { "orientation-portrait-symbolic.svg", NULL },
  { "process-working-symbolic.svg", NULL },
  { "switch-off-symbolic.svg", NULL },
  { "switch-on-symbolic.svg", NULL },
};

BobguiWidget *
create_symbolic (void)
{
  BobguiWidget *image;
  static int idx = 0;

  idx = (idx + 1) % G_N_ELEMENTS (symbolics);
  if (symbolics[idx].paintable == NULL)
    {
      char *uri;
      GFile *file;

      uri = g_strconcat ("resource:///org/bobgui/libbobgui/icons/", symbolics[idx].path, NULL);
      file = g_file_new_for_uri (uri);
      symbolics[idx].paintable = GDK_PAINTABLE (bobgui_icon_paintable_new_for_file (file, 16, 1));
      g_object_unref (file);
      g_free (uri);
    }

  image = bobgui_image_new ();
  bobgui_image_set_icon_size (BOBGUI_IMAGE (image), BOBGUI_ICON_SIZE_LARGE);
  bobgui_image_set_from_paintable (BOBGUI_IMAGE (image), symbolics[idx].paintable);

  return image;
}

static void
populate_symbolics (void)
{
  BobguiWidget *grid;
  int top, left;

  grid = bobgui_grid_new ();
  bobgui_widget_set_halign (grid, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_margin_start (grid, 10);
  bobgui_widget_set_margin_end (grid, 10);
  bobgui_widget_set_margin_top (grid, 10);
  bobgui_widget_set_margin_bottom (grid, 10);
  bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 10);
  bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 10);

  for (top = 0; top < 100; top++)
    for (left = 0; left < 15; left++)
       {
         bobgui_grid_attach (BOBGUI_GRID (grid), create_symbolic (), left, top, 1, 1);
       }

  hincrement = 0;
  vincrement = 5;

  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolledwindow),
                                  BOBGUI_POLICY_NEVER,
                                  BOBGUI_POLICY_AUTOMATIC);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolledwindow), grid);
}

BobguiWidget *
create_svg (void)
{
  BobguiWidget *image;
  static int idx = 0;

  idx = (idx + 1) % G_N_ELEMENTS (symbolics);
  if (symbolics[idx].paintable == NULL)
    {
      char *path;

      path = g_strconcat ("/org/bobgui/libbobgui/icons/", symbolics[idx].path, NULL);
      symbolics[idx].paintable = GDK_PAINTABLE (bobgui_svg_new_from_resource (path));
      g_free (path);
    }

  image = bobgui_image_new ();
  bobgui_image_set_icon_size (BOBGUI_IMAGE (image), BOBGUI_ICON_SIZE_LARGE);
  bobgui_image_set_from_paintable (BOBGUI_IMAGE (image), symbolics[idx].paintable);

  return image;
}

static void
populate_svg (void)
{
  BobguiWidget *grid;
  int top, left;

  grid = bobgui_grid_new ();
  bobgui_widget_set_halign (grid, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_margin_start (grid, 10);
  bobgui_widget_set_margin_end (grid, 10);
  bobgui_widget_set_margin_top (grid, 10);
  bobgui_widget_set_margin_bottom (grid, 10);
  bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 10);
  bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 10);

  for (top = 0; top < 100; top++)
    for (left = 0; left < 15; left++)
       {
         bobgui_grid_attach (BOBGUI_GRID (grid), create_svg (), left, top, 1, 1);
       }

  hincrement = 0;
  vincrement = 5;

  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolledwindow),
                                  BOBGUI_POLICY_NEVER,
                                  BOBGUI_POLICY_AUTOMATIC);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolledwindow), grid);
}

static BobguiWidget *
create_squiggle (void)
{
  BobguiWidget *image;
  BobguiSnapshot *snapshot;
  GdkPaintable *paintable;
  GskStroke *stroke;
  GskPathBuilder *builder;
  GskPath *path;
  float x, y;

  builder = gsk_path_builder_new ();

  x = g_random_double_range (1, 16);
  y = g_random_double_range (1, 16);
  gsk_path_builder_move_to (builder, x, y);
  for (int i = 0; i < 5; i++)
    {
      x = g_random_double_range (1, 16);
      y = g_random_double_range (1, 16);
      gsk_path_builder_line_to (builder, x, y);
    }
  gsk_path_builder_close (builder);

  path = gsk_path_builder_free_to_path (builder);
  stroke = gsk_stroke_new (1);

  snapshot = bobgui_snapshot_new ();
  bobgui_snapshot_append_stroke (snapshot, path, stroke, &(GdkRGBA) { 0, 0, 0, 1});
  paintable = bobgui_snapshot_free_to_paintable (snapshot, &GRAPHENE_SIZE_INIT (18, 18));

  image = bobgui_image_new ();
  bobgui_image_set_icon_size (BOBGUI_IMAGE (image), BOBGUI_ICON_SIZE_LARGE);
  bobgui_image_set_from_paintable (BOBGUI_IMAGE (image), paintable);

  g_object_unref (paintable);
  gsk_stroke_free (stroke);
  gsk_path_unref (path);

  return image;
}

static void
populate_squiggles (void)
{
  BobguiWidget *grid;
  int top, left;

  grid = bobgui_grid_new ();
  bobgui_widget_set_halign (grid, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_margin_start (grid, 10);
  bobgui_widget_set_margin_end (grid, 10);
  bobgui_widget_set_margin_top (grid, 10);
  bobgui_widget_set_margin_bottom (grid, 10);
  bobgui_grid_set_row_spacing (BOBGUI_GRID (grid), 10);
  bobgui_grid_set_column_spacing (BOBGUI_GRID (grid), 10);

  for (top = 0; top < 100; top++)
    for (left = 0; left < 15; left++)
       {
         bobgui_grid_attach (BOBGUI_GRID (grid), create_squiggle (), left, top, 1, 1);
       }

  hincrement = 0;
  vincrement = 5;

  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolledwindow),
                                  BOBGUI_POLICY_NEVER,
                                  BOBGUI_POLICY_AUTOMATIC);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolledwindow), grid);
}

static void
set_widget_type (int type)
{
  if (tick_cb)
    bobgui_widget_remove_tick_callback (window, tick_cb);

  if (bobgui_scrolled_window_get_child (BOBGUI_SCROLLED_WINDOW (scrolledwindow)))
    bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolledwindow), NULL);

  selected = type;

  switch (selected)
    {
    case 0:
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Scrolling icons");
      populate_icons ();
      break;

    case 1:
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Scrolling plain text");
      populate_text ("/sources/font_features.c", PLAIN_TEXT);
      break;

    case 2:
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Scrolling colored text");
      populate_text ("/sources/font_features.c", HIGHLIGHTED_TEXT);
      break;

    case 3:
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Scrolling text with underlines");
      populate_text ("/org/bobgui/Demo4/Moby-Dick.txt", UNDERLINED_TEXT);
      break;

    case 4:
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Scrolling text with Emoji");
      populate_emoji_text ();
      break;

    case 5:
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Scrolling a big image");
      populate_image ();
      break;

    case 6:
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Scrolling a list");
      populate_list ();
      break;

    case 7:
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Scrolling a columned list");
      populate_list2 ();
      break;

    case 8:
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Scrolling a grid");
      populate_grid ();
      break;

    case 9:
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Scrolling symbolics");
      populate_symbolics ();
      break;

    case 10:
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Scrolling SVG");
      populate_svg ();
      break;

    case 11:
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Scrolling squiggles");
      populate_squiggles ();
      break;

    default:
      g_assert_not_reached ();
    }

  tick_cb = bobgui_widget_add_tick_callback (window, scroll_cb, NULL, NULL);
}

G_MODULE_EXPORT void
iconscroll_next_clicked_cb (BobguiButton *source,
                            gpointer   user_data)
{
  int new_index;

  if (selected + 1 >= N_WIDGET_TYPES)
    new_index = 0;
  else
    new_index = selected + 1;
 

  set_widget_type (new_index);
}

G_MODULE_EXPORT void
iconscroll_prev_clicked_cb (BobguiButton *source,
                            gpointer   user_data)
{
  int new_index;

  if (selected - 1 < 0)
    new_index = N_WIDGET_TYPES - 1;
  else
    new_index = selected - 1;

  set_widget_type (new_index);
}

static gboolean
update_fps (gpointer data)
{
  BobguiWidget *label = data;
  GdkFrameClock *frame_clock;
  double fps;
  char *str;

  frame_clock = bobgui_widget_get_frame_clock (label);

  fps = gdk_frame_clock_get_fps (frame_clock);
  str = g_strdup_printf ("%.2f fps", fps);
  bobgui_label_set_label (BOBGUI_LABEL (label), str);
  g_free (str);

  return G_SOURCE_CONTINUE;
}

static void
remove_timeout (gpointer data)
{
  g_source_remove (GPOINTER_TO_UINT (data));
}

G_MODULE_EXPORT BobguiWidget *
do_iconscroll (BobguiWidget *do_widget)
{
  if (!window)
    {
      BobguiBuilder *builder;
      BobguiBuilderScope *scope;
      BobguiWidget *label;
      guint id;

      scope = bobgui_builder_cscope_new ();
      bobgui_builder_cscope_add_callback (BOBGUI_BUILDER_CSCOPE (scope), iconscroll_prev_clicked_cb);
      bobgui_builder_cscope_add_callback (BOBGUI_BUILDER_CSCOPE (scope), iconscroll_next_clicked_cb);

      builder = bobgui_builder_new ();
      bobgui_builder_set_scope (builder, scope);

      bobgui_builder_add_from_resource (builder, "/iconscroll/iconscroll.ui", NULL);
      window = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "window"));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));

      scrolledwindow = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "scrolledwindow"));
      bobgui_widget_realize (window);
      hadjustment = BOBGUI_ADJUSTMENT (bobgui_builder_get_object (builder, "hadjustment"));
      vadjustment = BOBGUI_ADJUSTMENT (bobgui_builder_get_object (builder, "vadjustment"));
      if (g_getenv ("ICONSCROLL"))
        set_widget_type (CLAMP (atoi (g_getenv ("ICONSCROLL")), 0, N_WIDGET_TYPES));
      else
        set_widget_type (0);

      label = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "fps_label"));
      id = g_timeout_add_full (G_PRIORITY_HIGH, 500, update_fps, label, NULL);
      g_object_set_data_full (G_OBJECT (label), "timeout",
                              GUINT_TO_POINTER (id), remove_timeout);

      g_object_unref (builder);
      g_object_unref (scope);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
