

#include "config.h"
#include <bobgui/bobgui.h>

typedef struct {
  BobguiTextView parent;
} MyTextView;

typedef struct {
  BobguiTextViewClass parent_class;
} MyTextViewClass;

static GType my_text_view_get_type (void);
G_DEFINE_TYPE (MyTextView, my_text_view, BOBGUI_TYPE_TEXT_VIEW);

static void snapshot_background (BobguiWidget *widget, BobguiSnapshot *snapshot);

static void
my_text_view_init (MyTextView *text_view)
{
}

static void my_text_view_snapshot_layer (BobguiTextView       *textview,
                                         BobguiTextViewLayer   layer,
                                         BobguiSnapshot       *snapshot)
{
  if (layer == BOBGUI_TEXT_VIEW_LAYER_BELOW_TEXT)
    snapshot_background (BOBGUI_WIDGET (textview), snapshot);
}

static void
my_text_view_class_init (MyTextViewClass *klass)
{
  BOBGUI_TEXT_VIEW_CLASS (klass)->snapshot_layer = my_text_view_snapshot_layer;
}

static void
create_tags (BobguiTextBuffer *buffer)
{

  bobgui_text_buffer_create_tag (buffer, "italic",
                              "style", PANGO_STYLE_ITALIC, NULL);

  bobgui_text_buffer_create_tag (buffer, "bold",
                              "weight", PANGO_WEIGHT_BOLD, NULL);

  bobgui_text_buffer_create_tag (buffer, "x-large",
                              "scale", PANGO_SCALE_X_LARGE, NULL);

  bobgui_text_buffer_create_tag (buffer, "semi_blue_foreground",
                              "foreground", "rgba(0,0,255,0.7)", NULL);

  bobgui_text_buffer_create_tag (buffer, "semi_red_background",
                              "background", "rgba(255,0,0,0.5)", NULL);

  bobgui_text_buffer_create_tag (buffer, "semi_orange_paragraph_background",
                              "paragraph-background", "rgba(255,165,0,0.5)", NULL);

  bobgui_text_buffer_create_tag (buffer, "word_wrap",
                              "wrap_mode", BOBGUI_WRAP_WORD, NULL);
}


static BobguiTextChildAnchor *
insert_text (BobguiTextBuffer *buffer)
{
  BobguiTextIter  iter;
  BobguiTextIter  start, end;
  BobguiTextMark *para_start;
  BobguiTextChildAnchor *anchor;

  /* get start of buffer; each insertion will revalidate the
   * iterator to point to just after the inserted text.
   */
  bobgui_text_buffer_get_iter_at_offset (buffer, &iter, 0);

  bobgui_text_buffer_insert (buffer, &iter,
      "This test shows text view rendering some text with rgba colors.\n\n", -1);

  bobgui_text_buffer_insert (buffer, &iter, "For example, you can have ", -1);
  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "italic translucent blue text", -1,
                                            "italic", 
					    "semi_blue_foreground",
					    "x-large",
					    NULL);

  bobgui_text_buffer_insert (buffer, &iter, ", or ", -1);

  bobgui_text_buffer_insert (buffer, &iter, ", ", -1);
  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "bold text with translucent red background", -1,
                                            "bold", 
					    "semi_red_background",
					    "x-large",
					    NULL);
  bobgui_text_buffer_insert (buffer, &iter, ".\n\n", -1);

  anchor = bobgui_text_buffer_create_child_anchor (buffer, &iter);

  /* Store the beginning of the other paragraph */
  para_start = bobgui_text_buffer_create_mark (buffer, "para_start", &iter, TRUE);

  bobgui_text_buffer_insert (buffer, &iter,
      "Paragraph background colors can also be set with rgba color values .\n", -1);

  bobgui_text_buffer_insert (buffer, &iter, "For instance, you can have ", -1);
  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "bold translucent blue text", -1,
                                            "bold", 
					    "semi_blue_foreground",
					    "x-large",
					    NULL);

  bobgui_text_buffer_insert (buffer, &iter, ", or ", -1);

  bobgui_text_buffer_insert (buffer, &iter, ", ", -1);
  bobgui_text_buffer_insert_with_tags_by_name (buffer, &iter,
                                            "italic text with translucent red background", -1,
                                            "italic", 
					    "semi_red_background",
					    "x-large",
					    NULL);

  bobgui_text_buffer_insert (buffer, &iter, " all rendered onto a translucent orange paragraph background.\n", -1);

  bobgui_text_buffer_get_bounds (buffer, &start, &end);

  bobgui_text_buffer_get_iter_at_mark (buffer, &iter, para_start);
  bobgui_text_buffer_apply_tag_by_name (buffer, "semi_orange_paragraph_background", &iter, &end);

  /* Apply word_wrap tag to whole buffer */
  bobgui_text_buffer_get_bounds (buffer, &start, &end);
  bobgui_text_buffer_apply_tag_by_name (buffer, "word_wrap", &start, &end);

  return anchor;
}


/* Size of checks and gray levels for alpha compositing checkerboard */
#define CHECK_SIZE  10
#define CHECK_DARK  (1.0 / 3.0)
#define CHECK_LIGHT (2.0 / 3.0)

static void
snapshot_background (BobguiWidget   *widget,
                     BobguiSnapshot *snapshot)
{
  GdkRectangle visible_rect;

  bobgui_text_view_get_visible_rect (BOBGUI_TEXT_VIEW (widget), &visible_rect);

  bobgui_snapshot_append_color (snapshot,
                             &(GdkRGBA) { CHECK_DARK, CHECK_DARK, CHECK_DARK, 1.0 },
                             &GRAPHENE_RECT_INIT(visible_rect.x, visible_rect.y, visible_rect.width, visible_rect.height));

  bobgui_snapshot_push_repeat (snapshot,
                            &GRAPHENE_RECT_INIT(visible_rect.x, visible_rect.y, visible_rect.width, visible_rect.height),
                            &GRAPHENE_RECT_INIT(visible_rect.x, visible_rect.y, CHECK_SIZE * 2, CHECK_SIZE * 2));
  bobgui_snapshot_append_color (snapshot,
                             &(GdkRGBA) { CHECK_LIGHT, CHECK_LIGHT, CHECK_LIGHT, 1.0 },
                             &GRAPHENE_RECT_INIT(visible_rect.x, visible_rect.y, CHECK_SIZE, CHECK_SIZE));
  bobgui_snapshot_append_color (snapshot,
                             &(GdkRGBA) { CHECK_LIGHT, CHECK_LIGHT, CHECK_LIGHT, 1.0 },
                             &GRAPHENE_RECT_INIT(visible_rect.x + CHECK_SIZE, visible_rect.y + CHECK_SIZE, CHECK_SIZE, CHECK_SIZE));
  bobgui_snapshot_pop (snapshot);
}

int
main (int argc, char **argv)
{
  BobguiWidget *window, *textview, *sw, *button, *button2;
  BobguiTextBuffer *buffer;
  BobguiTextChildAnchor *anchor;

  bobgui_init ();

  window   = bobgui_window_new ();
  sw       = bobgui_scrolled_window_new ();
  textview = g_object_new (my_text_view_get_type (), NULL);
  buffer   = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (textview));
  button   = bobgui_button_new_with_label ("Fixed Child");
  button2   = bobgui_button_new_with_label ("Flowed Child");

  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                  BOBGUI_POLICY_AUTOMATIC,
                                  BOBGUI_POLICY_AUTOMATIC);

  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 400, 400);

  create_tags (buffer);
  anchor = insert_text (buffer);

  bobgui_window_set_child (BOBGUI_WINDOW (window), sw);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), textview);
  bobgui_text_view_add_overlay (BOBGUI_TEXT_VIEW (textview),
                             button,
                             50, 150);

  bobgui_text_view_add_child_at_anchor (BOBGUI_TEXT_VIEW (textview),
                                     button2, anchor);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
