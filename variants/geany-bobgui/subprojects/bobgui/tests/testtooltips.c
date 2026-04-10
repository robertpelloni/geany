/* testtooltips.c: Test application for BOBGUI >= 2.12 tooltips code
 *
 * Copyright (C) 2006-2007  Imendio AB
 * Contact: Kristian Rietveld <kris@imendio.com>
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef struct _MyTooltip MyTooltip;
typedef struct _MyTooltipClass MyTooltipClass;


struct _MyTooltip
{
  BobguiWidget parent_instance;
};

struct _MyTooltipClass
{
  BobguiWidgetClass parent_class;
};

static GType my_tooltip_get_type (void);
G_DEFINE_TYPE (MyTooltip, my_tooltip, BOBGUI_TYPE_WIDGET)

static void
my_tooltip_init (MyTooltip *tt)
{
  BobguiWidget *label = bobgui_label_new ("Some text in a tooltip");

  bobgui_widget_set_parent (label, BOBGUI_WIDGET (tt));

  bobgui_widget_add_css_class (BOBGUI_WIDGET (tt), "background");
}

static void
my_tooltip_class_init (MyTooltipClass *tt_class)
{
  bobgui_widget_class_set_layout_manager_type (BOBGUI_WIDGET_CLASS (tt_class), BOBGUI_TYPE_BIN_LAYOUT);
  bobgui_widget_class_set_css_name (BOBGUI_WIDGET_CLASS (tt_class), "tooltip");
}

static gboolean
query_tooltip_cb (BobguiWidget  *widget,
		  int         x,
		  int         y,
		  gboolean    keyboard_tip,
		  BobguiTooltip *tooltip,
		  gpointer    data)
{
  bobgui_tooltip_set_markup (tooltip, bobgui_button_get_label (BOBGUI_BUTTON (widget)));
  bobgui_tooltip_set_icon_from_icon_name (tooltip, "edit-delete");

  return TRUE;
}

static gboolean
query_tooltip_text_view_cb (BobguiWidget  *widget,
			    int         x,
			    int         y,
			    gboolean    keyboard_tip,
			    BobguiTooltip *tooltip,
			    gpointer    data)
{
  BobguiTextTag *tag = data;
  BobguiTextIter iter;
  BobguiTextView *text_view = BOBGUI_TEXT_VIEW (widget);
  BobguiTextBuffer *buffer = bobgui_text_view_get_buffer (text_view);

  if (keyboard_tip)
    {
      int offset;

      g_object_get (buffer, "cursor-position", &offset, NULL);
      bobgui_text_buffer_get_iter_at_offset (buffer, &iter, offset);
    }
  else
    {
      int bx, by, trailing;

      bobgui_text_view_window_to_buffer_coords (text_view, BOBGUI_TEXT_WINDOW_TEXT,
					     x, y, &bx, &by);
      bobgui_text_view_get_iter_at_position (text_view, &iter, &trailing, bx, by);
    }

  if (bobgui_text_iter_has_tag (&iter, tag))
    bobgui_tooltip_set_text (tooltip, "Tooltip on text tag");
  else
   return FALSE;

  return TRUE;
}

static gboolean
query_tooltip_tree_view_cb (BobguiWidget  *widget,
			    int         x,
			    int         y,
			    gboolean    keyboard_tip,
			    BobguiTooltip *tooltip,
			    gpointer    data)
{
  BobguiTreeIter iter;
  BobguiTreeView *tree_view = BOBGUI_TREE_VIEW (widget);
  BobguiTreeModel *model = bobgui_tree_view_get_model (tree_view);
  BobguiTreePath *path = NULL;
  char *tmp;
  char *pathstring;

  char buffer[512];

  if (!bobgui_tree_view_get_tooltip_context (tree_view, x, y,
					  keyboard_tip,
					  &model, &path, &iter))
    return FALSE;

  bobgui_tree_model_get (model, &iter, 0, &tmp, -1);
  pathstring = bobgui_tree_path_to_string (path);

  g_snprintf (buffer, 511, "<b>Path %s:</b> %s", pathstring, tmp);
  bobgui_tooltip_set_markup (tooltip, buffer);

  bobgui_tree_view_set_tooltip_row (tree_view, tooltip, path);

  bobgui_tree_path_free (path);
  g_free (pathstring);
  g_free (tmp);

  return TRUE;
}

static BobguiTreeModel *
create_model (void)
{
  BobguiTreeStore *store;
  BobguiTreeIter iter;

  store = bobgui_tree_store_new (1, G_TYPE_STRING);

  /* A tree store with some random words ... */
  bobgui_tree_store_insert_with_values (store, &iter, NULL, 0,
				     0, "File Manager", -1);
  bobgui_tree_store_insert_with_values (store, &iter, NULL, 0,
				     0, "Gossip", -1);
  bobgui_tree_store_insert_with_values (store, &iter, NULL, 0,
				     0, "System Settings", -1);
  bobgui_tree_store_insert_with_values (store, &iter, NULL, 0,
				     0, "The GIMP", -1);
  bobgui_tree_store_insert_with_values (store, &iter, NULL, 0,
				     0, "Terminal", -1);
  bobgui_tree_store_insert_with_values (store, &iter, NULL, 0,
				     0, "Word Processor", -1);

  return BOBGUI_TREE_MODEL (store);
}

static void
selection_changed_cb (BobguiTreeSelection *selection,
		      BobguiWidget        *tree_view)
{
  bobgui_widget_trigger_tooltip_query (tree_view);
}

static struct Rectangle
{
  int x;
  int y;
  float r;
  float g;
  float b;
  const char *tooltip;
}
rectangles[] =
{
  { 10, 10, 0.0, 0.0, 0.9, "Blue box!" },
  { 200, 170, 1.0, 0.0, 0.0, "Red thing" },
  { 100, 50, 0.8, 0.8, 0.0, "Yellow thing" }
};

static gboolean
query_tooltip_drawing_area_cb (BobguiWidget  *widget,
			       int         x,
			       int         y,
			       gboolean    keyboard_tip,
			       BobguiTooltip *tooltip,
			       gpointer    data)
{
  int i;

  if (keyboard_tip)
    return FALSE;

  for (i = 0; i < G_N_ELEMENTS (rectangles); i++)
    {
      struct Rectangle *r = &rectangles[i];

      if (r->x < x && x < r->x + 50
	  && r->y < y && y < r->y + 50)
        {
	  bobgui_tooltip_set_markup (tooltip, r->tooltip);
	  return TRUE;
	}
    }

  return FALSE;
}

static void
drawing_area_draw (BobguiDrawingArea *drawing_area,
		   cairo_t        *cr,
                   int             width,
                   int             height,
		   gpointer        data)
{
  int i;

  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_paint (cr);

  for (i = 0; i < G_N_ELEMENTS (rectangles); i++)
    {
      struct Rectangle *r = &rectangles[i];

      cairo_rectangle (cr, r->x, r->y, 50, 50);
      cairo_set_source_rgb (cr, r->r, r->g, r->b);
      cairo_stroke (cr);

      cairo_rectangle (cr, r->x, r->y, 50, 50);
      cairo_set_source_rgba (cr, r->r, r->g, r->b, 0.5);
      cairo_fill (cr);
    }
}

static gboolean
query_tooltip_label_cb (BobguiWidget  *widget,
			int         x,
			int         y,
			gboolean    keyboard_tip,
			BobguiTooltip *tooltip,
			gpointer    data)
{
  BobguiWidget *custom = data;

  bobgui_tooltip_set_custom (tooltip, custom);

  return TRUE;
}

static void
quit_cb (BobguiWidget *widget,
         gpointer   data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

int
main (int argc, char *argv[])
{
  BobguiWidget *window;
  BobguiWidget *box;
  BobguiWidget *drawing_area;
  BobguiWidget *button;
  BobguiWidget *tooltip;
  BobguiWidget *popover;
  BobguiWidget *box2;
  BobguiWidget *custom;

  BobguiWidget *tree_view;
  BobguiTreeViewColumn *column;

  BobguiWidget *text_view;
  BobguiTextBuffer *buffer;
  BobguiTextIter iter;
  BobguiTextTag *tag;

  const char *text, *markup;
  gboolean done = FALSE;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Tooltips test");
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 3);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  tooltip = g_object_new (my_tooltip_get_type (), NULL);
  bobgui_widget_set_margin_top (tooltip, 20);
  bobgui_widget_set_margin_bottom (tooltip, 20);
  bobgui_widget_set_halign (tooltip, BOBGUI_ALIGN_CENTER);
  bobgui_box_append (BOBGUI_BOX (box), tooltip);


  /* A check button using the tooltip-markup property */
  button = bobgui_check_button_new_with_label ("This one uses the tooltip-markup property");
  bobgui_widget_set_tooltip_text (button, "Hello, I am a static tooltip.");
  bobgui_box_append (BOBGUI_BOX (box), button);

  text = bobgui_widget_get_tooltip_text (button);
  markup = bobgui_widget_get_tooltip_markup (button);
  g_assert_true (g_str_equal ("Hello, I am a static tooltip.", text));
  g_assert_true (g_str_equal ("Hello, I am a static tooltip.", markup));

  /* A check button using the query-tooltip signal */
  button = bobgui_check_button_new_with_label ("I use the query-tooltip signal");
  g_object_set (button, "has-tooltip", TRUE, NULL);
  g_signal_connect (button, "query-tooltip",
		    G_CALLBACK (query_tooltip_cb), NULL);
  bobgui_box_append (BOBGUI_BOX (box), button);

  /* A label */
  button = bobgui_label_new ("I am just a label");
  bobgui_label_set_selectable (BOBGUI_LABEL (button), FALSE);
  bobgui_widget_set_tooltip_text (button, "Label & and tooltip");
  bobgui_box_append (BOBGUI_BOX (box), button);

  text = bobgui_widget_get_tooltip_text (button);
  markup = bobgui_widget_get_tooltip_markup (button);
  g_assert_true (g_str_equal ("Label & and tooltip", text));
  g_assert_true (g_str_equal ("Label &amp; and tooltip", markup));

  /* A selectable label */
  button = bobgui_label_new ("I am a selectable label");
  bobgui_label_set_selectable (BOBGUI_LABEL (button), TRUE);
  bobgui_widget_set_tooltip_markup (button, "<b>Another</b> Label tooltip");
  bobgui_box_append (BOBGUI_BOX (box), button);

  text = bobgui_widget_get_tooltip_text (button);
  markup = bobgui_widget_get_tooltip_markup (button);
  g_assert_true (g_str_equal ("Another Label tooltip", text));
  g_assert_true (g_str_equal ("<b>Another</b> Label tooltip", markup));

  /* An insensitive button */
  button = bobgui_button_new_with_label ("This one is insensitive");
  bobgui_widget_set_sensitive (button, FALSE);
  g_object_set (button, "tooltip-text", "Insensitive!", NULL);
  bobgui_box_append (BOBGUI_BOX (box), button);

  /* Testcases from Kris without a tree view don't exist. */
  tree_view = bobgui_tree_view_new_with_model (create_model ());
  bobgui_widget_set_size_request (tree_view, 200, 240);

  bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (tree_view),
					       0, "Test",
					       bobgui_cell_renderer_text_new (),
					       "text", 0,
					       NULL);

  g_object_set (tree_view, "has-tooltip", TRUE, NULL);
  g_signal_connect (tree_view, "query-tooltip",
		    G_CALLBACK (query_tooltip_tree_view_cb), NULL);
  g_signal_connect (bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (tree_view)),
		    "changed", G_CALLBACK (selection_changed_cb), tree_view);

  /* Set a tooltip on the column */
  column = bobgui_tree_view_get_column (BOBGUI_TREE_VIEW (tree_view), 0);
  bobgui_tree_view_column_set_clickable (column, TRUE);
  g_object_set (bobgui_tree_view_column_get_button (column), "tooltip-text", "Header", NULL);

  bobgui_box_append (BOBGUI_BOX (box), tree_view);

  /* And a text view for Matthias */
  buffer = bobgui_text_buffer_new (NULL);

  bobgui_text_buffer_get_end_iter (buffer, &iter);
  bobgui_text_buffer_insert (buffer, &iter, "Hello, the text ", -1);

  tag = bobgui_text_buffer_create_tag (buffer, "bold", NULL);
  g_object_set (tag, "weight", PANGO_WEIGHT_BOLD, NULL);

  bobgui_text_buffer_get_end_iter (buffer, &iter);
  bobgui_text_buffer_insert_with_tags (buffer, &iter, "in bold", -1, tag, NULL);

  bobgui_text_buffer_get_end_iter (buffer, &iter);
  bobgui_text_buffer_insert (buffer, &iter, " has a tooltip!", -1);

  text_view = bobgui_text_view_new_with_buffer (buffer);
  bobgui_widget_set_size_request (text_view, 200, 50);

  g_object_set (text_view, "has-tooltip", TRUE, NULL);
  g_signal_connect (text_view, "query-tooltip",
		    G_CALLBACK (query_tooltip_text_view_cb), tag);

  bobgui_box_append (BOBGUI_BOX (box), text_view);

  /* Drawing area */
  drawing_area = bobgui_drawing_area_new ();
  bobgui_drawing_area_set_content_width (BOBGUI_DRAWING_AREA (drawing_area), 320);
  bobgui_drawing_area_set_content_height (BOBGUI_DRAWING_AREA (drawing_area), 240);
  bobgui_drawing_area_set_draw_func (BOBGUI_DRAWING_AREA (drawing_area),
                                  drawing_area_draw, NULL, NULL);
  g_object_set (drawing_area, "has-tooltip", TRUE, NULL);
  g_signal_connect (drawing_area, "query-tooltip",
		    G_CALLBACK (query_tooltip_drawing_area_cb), NULL);
  bobgui_box_append (BOBGUI_BOX (box), drawing_area);

  button = bobgui_menu_button_new ();
  bobgui_widget_set_halign (button, BOBGUI_ALIGN_CENTER);
  bobgui_menu_button_set_label (BOBGUI_MENU_BUTTON (button), "Custom tooltip I");
  bobgui_box_append (BOBGUI_BOX (box), button);
  popover = bobgui_popover_new ();
  bobgui_menu_button_set_popover (BOBGUI_MENU_BUTTON (button), popover);
  box2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_popover_set_child (BOBGUI_POPOVER (popover), box2);

  button = bobgui_label_new ("Hidden here");
  custom = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 5);
  bobgui_box_append (BOBGUI_BOX (custom), bobgui_label_new ("See, custom"));
  bobgui_box_append (BOBGUI_BOX (custom), g_object_new (BOBGUI_TYPE_SPINNER, "spinning", TRUE, NULL));
  g_object_ref_sink (custom);
  g_object_set (button, "has-tooltip", TRUE, NULL);
  bobgui_box_append (BOBGUI_BOX (box2), button);
  g_signal_connect (button, "query-tooltip",
		    G_CALLBACK (query_tooltip_label_cb), custom);

  button = bobgui_label_new ("Custom tooltip II");
  custom = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 5);
  bobgui_box_append (BOBGUI_BOX (custom), bobgui_label_new ("See, custom too"));
  bobgui_box_append (BOBGUI_BOX (custom), g_object_new (BOBGUI_TYPE_SPINNER, "spinning", TRUE, NULL));
  g_object_ref_sink (custom);
  g_object_set (button, "has-tooltip", TRUE, NULL);
  g_signal_connect (button, "query-tooltip",
		    G_CALLBACK (query_tooltip_label_cb), custom);
  bobgui_box_append (BOBGUI_BOX (box), button);

  /* Done! */
  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
