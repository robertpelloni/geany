/* Drag-and-Drop
 * #Keywords: dnd, menu, popover, gesture
 *
 * This demo shows dragging colors and widgets.
 * The items in this demo can be moved, recolored
 * and rotated.
 *
 * The demo also has an example for creating a
 * menu-like popover without using a menu model.
 */

#include <bobgui/bobgui.h>


G_DECLARE_FINAL_TYPE (CanvasItem, canvas_item, CANVAS, ITEM, BobguiWidget)

struct _CanvasItem {
  BobguiWidget parent;

  BobguiWidget *fixed;
  BobguiWidget *label;

  double r;
  double angle;
  double delta;

  BobguiWidget *editor;

  BobguiStyleProvider *provider;
  char *css_class;
};

struct _CanvasItemClass {
  BobguiWidgetClass parent_class;
};

G_DEFINE_TYPE (CanvasItem, canvas_item, BOBGUI_TYPE_WIDGET)

static int n_items = 0;

static void
unstyle_item (CanvasItem *item)
{
  if (item->provider)
    {
      bobgui_style_context_remove_provider_for_display (bobgui_widget_get_display (item->label), item->provider);
      g_clear_object (&item->provider);
    }

  if (item->css_class)
    {
      bobgui_widget_remove_css_class (item->label, item->css_class);
      g_clear_pointer (&item->css_class, g_free);
    }
}

static void
set_color (CanvasItem *item,
           GdkRGBA    *color)
{
  char *css;
  char *str;
  BobguiCssProvider *provider;
  const char *name;

  unstyle_item (item);

  str = gdk_rgba_to_string (color);
  name = bobgui_widget_get_name (item->label);
  css = g_strdup_printf ("#%s { background: %s; }", name, str);

  provider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_string (provider, css);
  bobgui_style_context_add_provider_for_display (bobgui_widget_get_display (item->label), BOBGUI_STYLE_PROVIDER (provider), 700);
  item->provider = BOBGUI_STYLE_PROVIDER (provider);

  g_free (str);
  g_free (css);
}

static void
set_css (CanvasItem *item,
         const char *class)
{
  unstyle_item (item);

  bobgui_widget_add_css_class (item->label, class);
  item->css_class = g_strdup (class);
}

static gboolean
item_drag_drop (BobguiDropTarget *dest,
                const GValue  *value,
                double         x,
                double         y)
{
  BobguiWidget *label = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (dest));
  CanvasItem *item = CANVAS_ITEM (bobgui_widget_get_parent (bobgui_widget_get_parent (label)));

  if (G_VALUE_TYPE (value) == GDK_TYPE_RGBA)
    set_color (item, g_value_get_boxed (value));
  else if (G_VALUE_TYPE (value) == G_TYPE_STRING)
    set_css (item, g_value_get_string (value));

  return TRUE;
}

static void
apply_transform (CanvasItem *item)
{
  GskTransform *transform;
  graphene_rect_t bounds;
  double x, y;

  /* Add css padding and margin */
  if (!bobgui_widget_compute_bounds (item->label, item->label, &bounds))
    return;

  x = bounds.size.width / 2.;
  y = bounds.size.height / 2.;

  item->r = sqrt (x * x + y * y);

  transform = gsk_transform_translate (NULL, &(graphene_point_t) { item->r, item->r });
  transform = gsk_transform_rotate (transform, item->angle + item->delta);
  transform = gsk_transform_translate (transform, &GRAPHENE_POINT_INIT (-x, -y));

  bobgui_fixed_set_child_transform (BOBGUI_FIXED (item->fixed), item->label, transform);
  gsk_transform_unref (transform);
}

static void
angle_changed (BobguiGestureRotate *gesture,
               double            angle,
               double            delta)
{
  CanvasItem *item = CANVAS_ITEM (bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture)));

  item->delta = angle / M_PI * 180.0;

  apply_transform (item);
}

static void
rotate_done (BobguiGesture *gesture)
{
  CanvasItem *item = CANVAS_ITEM (bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture)));

  item->angle = item->angle + item->delta;
  item->delta = 0;
}

static void
click_done (BobguiGesture *gesture)
{
  BobguiWidget *item = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture));
  BobguiWidget *canvas = bobgui_widget_get_parent (item);
  BobguiWidget *last_child;

  last_child = bobgui_widget_get_last_child (canvas);
  if (item != last_child)
    bobgui_widget_insert_after (item, canvas, last_child);
}

/* BobguiSettings treats `BOBGUI_THEME=foo:dark` as theme name `foo`, variant `dark`,
 * and our embedded CSS files let `foo-dark` work as an alias for `foo:dark`. */
static gboolean
has_dark_suffix (const char *theme)
{
  return g_str_has_suffix (theme, ":dark") ||
         g_str_has_suffix (theme, "-dark");
}

/* So we can make a good guess whether the current theme is dark by checking for
 * either: it is suffixed `[:-]dark`, or Settings:…prefer-dark-theme is TRUE. */
static gboolean
theme_is_dark (void)
{
  const char *env_theme;
  BobguiSettings *settings;
  char *theme;
  gboolean prefer_dark;
  gboolean dark;

  /* Like BobguiSettings, 1st see if theme is overridden by environment variable */
  env_theme = g_getenv ("BOBGUI_THEME");
  if (env_theme != NULL)
    return has_dark_suffix (env_theme);

  /* If not, test Settings:…theme-name in the same way OR :…prefer-dark-theme */
  settings = bobgui_settings_get_default ();
  g_object_get (settings,
                "bobgui-theme-name", &theme,
                "bobgui-application-prefer-dark-theme", &prefer_dark,
                NULL);
  dark = prefer_dark || has_dark_suffix (theme);
  g_free (theme);
  return dark;
}

static void
canvas_item_init (CanvasItem *item)
{
  char *text;
  char *id;
  GdkRGBA rgba;
  BobguiDropTarget *dest;
  BobguiGesture *gesture;
  GType types[2] = { GDK_TYPE_RGBA, G_TYPE_STRING };

  n_items++;

  text = g_strdup_printf ("Item %d", n_items);
  item->label = bobgui_label_new (text);
  bobgui_widget_add_css_class (item->label, "canvasitem");
  g_free (text);

  item->fixed = bobgui_fixed_new ();
  bobgui_widget_set_parent (item->fixed, BOBGUI_WIDGET (item));
  bobgui_fixed_put (BOBGUI_FIXED (item->fixed), item->label, 0, 0);

  bobgui_widget_add_css_class (item->label, "frame");

  id = g_strdup_printf ("item%d", n_items);
  bobgui_widget_set_name (item->label, id);
  g_free (id);

  if (theme_is_dark ())
    gdk_rgba_parse (&rgba, "blue");
  else
    gdk_rgba_parse (&rgba, "yellow");

  set_color (item, &rgba);

  item->angle = 0;

  dest = bobgui_drop_target_new (G_TYPE_INVALID, GDK_ACTION_COPY);
  bobgui_drop_target_set_gtypes (dest, types, G_N_ELEMENTS (types));
  g_signal_connect (dest, "drop", G_CALLBACK (item_drag_drop), NULL);
  bobgui_widget_add_controller (BOBGUI_WIDGET (item->label), BOBGUI_EVENT_CONTROLLER (dest));

  gesture = bobgui_gesture_rotate_new ();
  g_signal_connect (gesture, "angle-changed", G_CALLBACK (angle_changed), NULL);
  g_signal_connect (gesture, "end", G_CALLBACK (rotate_done), NULL);
  bobgui_widget_add_controller (BOBGUI_WIDGET (item), BOBGUI_EVENT_CONTROLLER (gesture));

  gesture = bobgui_gesture_click_new ();
  g_signal_connect (gesture, "released", G_CALLBACK (click_done), NULL);
  bobgui_widget_add_controller (BOBGUI_WIDGET (item), BOBGUI_EVENT_CONTROLLER (gesture));
}

static void
canvas_item_dispose (GObject *object)
{
  CanvasItem *item = CANVAS_ITEM (object);

  g_clear_pointer (&item->fixed, bobgui_widget_unparent);
  g_clear_pointer (&item->editor, bobgui_widget_unparent);

  G_OBJECT_CLASS (canvas_item_parent_class)->dispose (object);
}

static void
canvas_item_map (BobguiWidget *widget)
{
  CanvasItem *item = CANVAS_ITEM (widget);

  BOBGUI_WIDGET_CLASS (canvas_item_parent_class)->map (widget);

  apply_transform (item);
}

static void
canvas_item_class_init (CanvasItemClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = canvas_item_dispose;

  widget_class->map = canvas_item_map;

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, "item");
}

static BobguiWidget *
canvas_item_new (void)
{
  CanvasItem *item = g_object_new (canvas_item_get_type (), NULL);

  return BOBGUI_WIDGET (item);
}

static GdkPaintable *
canvas_item_get_drag_icon (CanvasItem *item)
{
  return bobgui_widget_paintable_new (item->fixed);
}

static gboolean
canvas_item_is_editing (CanvasItem *item)
{
  return item->editor != NULL;
}

static void
scale_changed (BobguiRange   *range,
               CanvasItem *item)
{
  item->angle = bobgui_range_get_value (range);
  apply_transform (item);
}

static void
text_changed (BobguiEditable *editable,
              GParamSpec  *pspec,
              CanvasItem  *item)
{
  bobgui_label_set_text (BOBGUI_LABEL (item->label), bobgui_editable_get_text (editable));
  apply_transform (item);
}

static void
canvas_item_stop_editing (CanvasItem *item)
{
  BobguiWidget *scale;

  if (!item->editor)
    return;

  scale = bobgui_widget_get_last_child (item->editor);
  g_signal_handlers_disconnect_by_func (scale, scale_changed, item);

  bobgui_fixed_remove (BOBGUI_FIXED (bobgui_widget_get_parent (item->editor)), item->editor);
  item->editor = NULL;
}

static void
canvas_item_start_editing (CanvasItem *item)
{
  BobguiWidget *canvas = bobgui_widget_get_parent (BOBGUI_WIDGET (item));
  BobguiWidget *entry;
  BobguiWidget *scale;
  graphene_point_t p;

  if (item->editor)
    return;

  item->editor = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 12);

  entry = bobgui_entry_new ();

  bobgui_editable_set_text (BOBGUI_EDITABLE (entry),
                         bobgui_label_get_text (BOBGUI_LABEL (item->label)));

  bobgui_editable_set_width_chars (BOBGUI_EDITABLE (entry), 12);
  g_signal_connect (entry, "notify::text", G_CALLBACK (text_changed), item);
  g_signal_connect_swapped (entry, "activate", G_CALLBACK (canvas_item_stop_editing), item);

  bobgui_box_append (BOBGUI_BOX (item->editor), entry);

  scale = bobgui_scale_new_with_range (BOBGUI_ORIENTATION_HORIZONTAL, 0, 360, 1);
  bobgui_scale_set_draw_value (BOBGUI_SCALE (scale), FALSE);
  bobgui_range_set_value (BOBGUI_RANGE (scale), fmod (item->angle, 360));

  g_signal_connect (scale, "value-changed", G_CALLBACK (scale_changed), item);

  bobgui_box_append (BOBGUI_BOX (item->editor), scale);

  if (!bobgui_widget_compute_point (BOBGUI_WIDGET (item), canvas, &GRAPHENE_POINT_INIT (0, 0), &p))
    graphene_point_init (&p, 0, 0);
  bobgui_fixed_put (BOBGUI_FIXED (canvas), item->editor, p.x, p.y + 2 * item->r);
  bobgui_widget_grab_focus (entry);

}

typedef struct {
  double x, y;
} Hotspot;

static GdkContentProvider *
prepare (BobguiDragSource *source,
         double         x,
         double         y)
{
  BobguiWidget *canvas;
  BobguiWidget *item;
  Hotspot *hotspot;
  graphene_point_t p;

  canvas = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (source));
  item = bobgui_widget_pick (canvas, x, y, BOBGUI_PICK_DEFAULT);

  item = bobgui_widget_get_ancestor (item, canvas_item_get_type ());
  if (!item)
    return NULL;

  g_object_set_data (G_OBJECT (canvas), "dragged-item", item);

  hotspot = g_new (Hotspot, 1);
  if (!bobgui_widget_compute_point (canvas, item, &GRAPHENE_POINT_INIT (x, y), &p))
    graphene_point_init (&p, x, y);
  hotspot->x = p.x;
  hotspot->y = p.y;
  g_object_set_data_full (G_OBJECT (canvas), "hotspot", hotspot, g_free);

  return gdk_content_provider_new_typed (BOBGUI_TYPE_WIDGET, item);
}

static void
drag_begin (BobguiDragSource *source,
            GdkDrag       *drag)
{
  BobguiWidget *canvas;
  CanvasItem *item;
  GdkPaintable *paintable;
  Hotspot *hotspot;
  BobguiWidget *trash;
  BobguiSvg *trash_paintable;

  canvas = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (source));
  item = CANVAS_ITEM (g_object_get_data (G_OBJECT (canvas), "dragged-item"));
  hotspot = (Hotspot *) g_object_get_data (G_OBJECT (canvas), "hotspot");

  trash = g_object_get_data (G_OBJECT (canvas), "trash");
  trash_paintable = BOBGUI_SVG (bobgui_image_get_paintable (BOBGUI_IMAGE (trash)));
  bobgui_svg_set_state (trash_paintable, 0);
  bobgui_widget_set_visible (trash, TRUE);

  paintable = canvas_item_get_drag_icon (item);
  bobgui_drag_source_set_icon (source, paintable, hotspot->x, hotspot->y);
  g_object_unref (paintable);

  bobgui_widget_set_opacity (BOBGUI_WIDGET (item), 0.3);
}

static void
drag_end (BobguiDragSource *source,
          GdkDrag       *drag)
{
  BobguiWidget *canvas;
  BobguiWidget *item;
  BobguiWidget *trash;

  canvas = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (source));
  item = g_object_get_data (G_OBJECT (canvas), "dragged-item");
  g_object_set_data (G_OBJECT (canvas), "dragged-item", NULL);

  bobgui_widget_set_opacity (item, 1.0);

  trash = g_object_get_data (G_OBJECT (canvas), "trash");
  bobgui_widget_set_visible (trash, FALSE);
}

static gboolean
drag_cancel (BobguiDragSource       *source,
             GdkDrag             *drag,
             GdkDragCancelReason  reason)
{
  BobguiWidget *canvas;
  BobguiWidget *trash;

  canvas = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (source));
  trash = g_object_get_data (G_OBJECT (canvas), "trash");
  bobgui_widget_set_visible (trash, FALSE);

  return FALSE;
}

static gboolean
drag_drop (BobguiDropTarget *target,
           const GValue  *value,
           double         x,
           double         y)
{
  CanvasItem *item;
  BobguiWidget *canvas;
  BobguiWidget *last_child;

  item = g_value_get_object (value);

  canvas = bobgui_widget_get_parent (BOBGUI_WIDGET (item));
  last_child = bobgui_widget_get_last_child (canvas);
  if (BOBGUI_WIDGET (item) != last_child)
    bobgui_widget_insert_after (BOBGUI_WIDGET (item), canvas, last_child);

  bobgui_fixed_move (BOBGUI_FIXED (canvas), BOBGUI_WIDGET (item), x - item->r, y - item->r);

  return TRUE;
}

static gboolean
drop_in_trash (BobguiDropTarget *target,
               const GValue  *value,
               double         x,
               double         y,
               BobguiSvg        *trash_paintable)
{
  BobguiWidget *item;
  BobguiWidget *canvas;
  BobguiWidget *trash;

  trash = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (target));
  item = g_value_get_object (value);
  canvas = bobgui_widget_get_parent (item);

  bobgui_svg_set_state (trash_paintable, 0);

  bobgui_fixed_remove (BOBGUI_FIXED (canvas), item);

  bobgui_widget_set_visible (trash, FALSE);

  return TRUE;
}

static GdkDragAction
enter_trash (BobguiDropTarget *target,
             double         x,
             double         y,
             BobguiSvg        *trash_paintable)
{
  BobguiWidget *widget;

  widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (target));

  bobgui_svg_set_frame_clock (trash_paintable, bobgui_widget_get_frame_clock (widget));
  bobgui_svg_set_state (trash_paintable, 1);
  return GDK_ACTION_MOVE;
}

static void
leave_trash (BobguiDropTarget *target,
             BobguiSvg        *trash_paintable)
{
  bobgui_svg_set_state (trash_paintable, 0);
}

static void
new_item_cb (BobguiWidget *button, gpointer data)
{
  BobguiWidget *canvas = data;
  BobguiWidget *popover;
  BobguiWidget *item;
  GdkRectangle rect;

  popover = bobgui_widget_get_ancestor (button, BOBGUI_TYPE_POPOVER);
  bobgui_popover_get_pointing_to (BOBGUI_POPOVER (popover), &rect);

  item = canvas_item_new ();
  bobgui_fixed_put (BOBGUI_FIXED (canvas), item, rect.x, rect.y);
  apply_transform (CANVAS_ITEM (item));

  bobgui_popover_popdown (BOBGUI_POPOVER (bobgui_widget_get_ancestor (button, BOBGUI_TYPE_POPOVER)));
}

static void
edit_cb (BobguiWidget *button, BobguiWidget *child)
{
  CanvasItem *item = CANVAS_ITEM (child);

  if (button)
    bobgui_popover_popdown (BOBGUI_POPOVER (bobgui_widget_get_ancestor (button, BOBGUI_TYPE_POPOVER)));

  if (!canvas_item_is_editing (item))
    canvas_item_start_editing (item);
}

static void
delete_cb (BobguiWidget *button, BobguiWidget *child)
{
  BobguiWidget *canvas = bobgui_widget_get_parent (child);

  bobgui_fixed_remove (BOBGUI_FIXED (canvas), child);

  bobgui_popover_popdown (BOBGUI_POPOVER (bobgui_widget_get_ancestor (button, BOBGUI_TYPE_POPOVER)));
}

static void
show_context_menu (BobguiWidget *widget,
                   BobguiWidget *child,
		   int        x,
		   int        y)
{
  BobguiWidget *menu;
  BobguiWidget *box;
  BobguiWidget *item;

  menu = bobgui_popover_new ();
  bobgui_widget_set_parent (menu, widget);
  bobgui_popover_set_has_arrow (BOBGUI_POPOVER (menu), FALSE);
  bobgui_popover_set_pointing_to (BOBGUI_POPOVER (menu), &(GdkRectangle){ x, y, 1, 1});
  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_popover_set_child (BOBGUI_POPOVER (menu), box);

  item = bobgui_button_new_with_label ("New");
  bobgui_button_set_has_frame (BOBGUI_BUTTON (item), FALSE);
  g_signal_connect (item, "clicked", G_CALLBACK (new_item_cb), widget);
  bobgui_box_append (BOBGUI_BOX (box), item);

  item = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
  bobgui_box_append (BOBGUI_BOX (box), item);

  item = bobgui_button_new_with_label ("Edit");
  bobgui_button_set_has_frame (BOBGUI_BUTTON (item), FALSE);
  bobgui_widget_set_sensitive (item, child != NULL && child != widget);
  g_signal_connect (item, "clicked", G_CALLBACK (edit_cb), child);
  bobgui_box_append (BOBGUI_BOX (box), item);

  item = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
  bobgui_box_append (BOBGUI_BOX (box), item);

  item = bobgui_button_new_with_label ("Delete");
  bobgui_button_set_has_frame (BOBGUI_BUTTON (item), FALSE);
  bobgui_widget_set_sensitive (item, child != NULL && child != widget);
  g_signal_connect (item, "clicked", G_CALLBACK (delete_cb), child);
  bobgui_box_append (BOBGUI_BOX (box), item);

  bobgui_popover_popup (BOBGUI_POPOVER (menu));
}

static void
pressed_cb (BobguiGesture *gesture,
            int         n_press,
            double      x,
            double      y,
            gpointer    data)
{
  BobguiWidget *widget;
  BobguiWidget *child;

  widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture));
  child = bobgui_widget_pick (widget, x, y, BOBGUI_PICK_DEFAULT);
  child = bobgui_widget_get_ancestor (child, canvas_item_get_type ());

  if (gdk_event_triggers_context_menu (bobgui_event_controller_get_current_event (BOBGUI_EVENT_CONTROLLER (gesture))))
    {
      show_context_menu (widget, child, x, y);
    }
}

static void
released_cb (BobguiGesture *gesture,
             int         n_press,
             double      x,
             double      y,
             gpointer    data)
{
  BobguiWidget *widget;
  BobguiWidget *child;
  CanvasItem *item;

  widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture));
  child = bobgui_widget_pick (widget, x, y, 0);

  if (!child)
    return;

  item = (CanvasItem *)bobgui_widget_get_ancestor (child, canvas_item_get_type ());
  if (!item)
    {
      GdkEvent *event = bobgui_event_controller_get_current_event (BOBGUI_EVENT_CONTROLLER (gesture));

      if (gdk_event_get_event_type (event) == GDK_TOUCH_END)
        {
          show_context_menu (widget, NULL, x, y);
        }

      return;
    }

  if (bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (gesture)) == GDK_BUTTON_PRIMARY)
    {
      if (canvas_item_is_editing (item))
        canvas_item_stop_editing (item);
      else
        canvas_item_start_editing (item);
    }
}

static BobguiWidget *
canvas_new (void)
{
  BobguiWidget *canvas;
  BobguiDragSource *source;
  BobguiDropTarget *dest;
  BobguiGesture *gesture;
  BobguiSvg *trash_paintable;
  BobguiWidget *trash;

  canvas = bobgui_fixed_new ();
  bobgui_widget_set_hexpand (canvas, TRUE);
  bobgui_widget_set_vexpand (canvas, TRUE);

  source = bobgui_drag_source_new ();
  bobgui_drag_source_set_actions (source, GDK_ACTION_MOVE);
  g_signal_connect (source, "prepare", G_CALLBACK (prepare), NULL);
  g_signal_connect (source, "drag-begin", G_CALLBACK (drag_begin), NULL);
  g_signal_connect (source, "drag-end", G_CALLBACK (drag_end), NULL);
  g_signal_connect (source, "drag-cancel", G_CALLBACK (drag_cancel), NULL);
  bobgui_widget_add_controller (canvas, BOBGUI_EVENT_CONTROLLER (source));

  dest = bobgui_drop_target_new (BOBGUI_TYPE_WIDGET, GDK_ACTION_MOVE);
  g_signal_connect (dest, "drop", G_CALLBACK (drag_drop), NULL);
  bobgui_widget_add_controller (canvas, BOBGUI_EVENT_CONTROLLER (dest));

  gesture = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture), 0);
  g_signal_connect (gesture, "pressed", G_CALLBACK (pressed_cb), NULL);
  g_signal_connect (gesture, "released", G_CALLBACK (released_cb), NULL);
  bobgui_widget_add_controller (canvas, BOBGUI_EVENT_CONTROLLER (gesture));

  trash_paintable = bobgui_svg_new_from_resource ("/dnd/user-trash-opening.gpa");
  bobgui_svg_play (trash_paintable);
  trash = bobgui_image_new_from_paintable (GDK_PAINTABLE (trash_paintable));
  g_object_unref (trash_paintable);
  bobgui_image_set_pixel_size (BOBGUI_IMAGE (trash), 64);
  bobgui_widget_add_css_class (trash, "trash");

  bobgui_fixed_put (BOBGUI_FIXED (canvas), trash, 20, 20);
  bobgui_widget_set_visible (trash, FALSE);

  dest = bobgui_drop_target_new (BOBGUI_TYPE_WIDGET, GDK_ACTION_MOVE);

  g_signal_connect (dest, "enter", G_CALLBACK (enter_trash), trash_paintable);
  g_signal_connect (dest, "leave", G_CALLBACK (leave_trash), trash_paintable);

  g_signal_connect (dest, "drop", G_CALLBACK (drop_in_trash), trash_paintable);
  bobgui_widget_add_controller (trash, BOBGUI_EVENT_CONTROLLER (dest));
  g_object_set_data (G_OBJECT (canvas), "trash", trash);

  return canvas;
}

static GdkContentProvider *
css_drag_prepare (BobguiDragSource *source,
                  double         x,
                  double         y,
                  BobguiWidget     *button)
{
  const char *class;
  GdkPaintable *paintable;

  class = (const char *)g_object_get_data (G_OBJECT (button), "css-class");

  paintable = bobgui_widget_paintable_new (button);
  bobgui_drag_source_set_icon (source, paintable, 0, 0);
  g_object_unref (paintable);

  return gdk_content_provider_new_typed (G_TYPE_STRING, class);
}

static BobguiWidget *
css_button_new (const char *class)
{
  BobguiWidget *button;
  BobguiDragSource *source;

  button = bobgui_image_new ();
  bobgui_widget_set_size_request (button, 48, 32);
  bobgui_widget_add_css_class (button, class);
  g_object_set_data (G_OBJECT (button), "css-class", (gpointer)class);

  source = bobgui_drag_source_new ();
  g_signal_connect (source, "prepare", G_CALLBACK (css_drag_prepare), button);
  bobgui_widget_add_controller (button, BOBGUI_EVENT_CONTROLLER (source));

  return button;
}

typedef struct
{
  BobguiWidget parent_instance;
  GdkRGBA color;
} ColorSwatch;

typedef struct
{
  BobguiWidgetClass parent_class;
} ColorSwatchClass;

G_DEFINE_TYPE (ColorSwatch, color_swatch, BOBGUI_TYPE_WIDGET)

static GdkContentProvider *
color_swatch_drag_prepare (BobguiDragSource  *source,
                           double          x,
                           double          y,
                           ColorSwatch    *swatch)
{
  return gdk_content_provider_new_typed (GDK_TYPE_RGBA, &swatch->color);
}

static void
color_swatch_init (ColorSwatch *swatch)
{
  BobguiDragSource *source = bobgui_drag_source_new ();
  g_signal_connect (source, "prepare", G_CALLBACK (color_swatch_drag_prepare), swatch);
  bobgui_widget_add_controller (BOBGUI_WIDGET (swatch), BOBGUI_EVENT_CONTROLLER (source));
}

static void
color_swatch_snapshot (BobguiWidget   *widget,
                       BobguiSnapshot *snapshot)
{
  ColorSwatch *swatch = (ColorSwatch *)widget;
  float w = bobgui_widget_get_width (widget);
  float h = bobgui_widget_get_height (widget);

  bobgui_snapshot_append_color (snapshot, &swatch->color,
                             &GRAPHENE_RECT_INIT(0, 0, w, h));
}

void
color_swatch_measure (BobguiWidget      *widget,
                      BobguiOrientation  orientation,
                      int             for_size,
                      int            *minimum_size,
                      int            *natural_size,
                      int            *minimum_baseline,
                      int            *natural_baseline)
{
  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    *minimum_size = *natural_size = 48;
  else
    *minimum_size = *natural_size = 32;
}

static void
color_swatch_class_init (ColorSwatchClass *class)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  widget_class->snapshot = color_swatch_snapshot;
  widget_class->measure = color_swatch_measure;
  bobgui_widget_class_set_css_name (widget_class, "colorswatch");
}

static BobguiWidget *
color_swatch_new (const char *color)
{
  ColorSwatch *swatch = g_object_new (color_swatch_get_type (), NULL);

  gdk_rgba_parse (&swatch->color, color);

  return BOBGUI_WIDGET (swatch);
}

static BobguiWidget *window = NULL;

BobguiWidget *
do_dnd (BobguiWidget *do_widget)
{
  if (!window)
    {
      BobguiWidget *button;
      BobguiWidget *sw;
      BobguiWidget *canvas;
      BobguiWidget *box, *box2, *box3;
      const char *colors[] = {
        "red", "green", "blue", "magenta", "orange", "gray", "black", "yellow",
        "white", "gray", "brown", "pink",  "cyan", "bisque", "gold", "maroon",
        "navy", "orchid", "olive", "peru", "salmon", "silver", "wheat",
        NULL
      };
      int i;
      int x, y;
      BobguiCssProvider *provider;
      GString *css;

      button = bobgui_color_dialog_button_new (bobgui_color_dialog_new ());
      g_object_unref (g_object_ref_sink (button));

      provider = bobgui_css_provider_new ();
      bobgui_css_provider_load_from_resource (provider, "/dnd/dnd.css");
      bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                                  BOBGUI_STYLE_PROVIDER (provider),
                                                  800);
      g_object_unref (provider);

      css = g_string_new ("");
      for (i = 0; colors[i]; i++)
        g_string_append_printf (css, ".canvasitem.%s { background: %s; }\n", colors[i], colors[i]);

      provider = bobgui_css_provider_new ();
      bobgui_css_provider_load_from_string (provider, css->str);
      bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                                  BOBGUI_STYLE_PROVIDER (provider),
                                                  800);
      g_object_unref (provider);
      g_string_free (css, TRUE);

      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Drag-and-Drop");
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 640, 480);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_window_set_child (BOBGUI_WINDOW (window), box);

      box2 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_box_append (BOBGUI_BOX (box), box2);

      canvas = canvas_new ();
      bobgui_box_append (BOBGUI_BOX (box2), canvas);

      n_items = 0;

      x = 150;
      y = 100;
      for (i = 0; i < 3; i++)
        {
          BobguiWidget *item;

          item = canvas_item_new ();
          bobgui_fixed_put (BOBGUI_FIXED (canvas), item, x, y);
          apply_transform (CANVAS_ITEM (item));

          x += 150;
          y += 100;
        }

      bobgui_box_append (BOBGUI_BOX (box), bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL));

      sw = bobgui_scrolled_window_new ();
      bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                      BOBGUI_POLICY_AUTOMATIC,
                                      BOBGUI_POLICY_NEVER);
      bobgui_box_append (BOBGUI_BOX (box), sw);

      box3 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_widget_add_css_class (box3, "linked");
      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), box3);

      for (i = 0; colors[i]; i++)
        bobgui_box_append (BOBGUI_BOX (box3), color_swatch_new (colors[i]));

      bobgui_box_append (BOBGUI_BOX (box3), css_button_new ("rainbow1"));
      bobgui_box_append (BOBGUI_BOX (box3), css_button_new ("rainbow2"));
      bobgui_box_append (BOBGUI_BOX (box3), css_button_new ("rainbow3"));
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
