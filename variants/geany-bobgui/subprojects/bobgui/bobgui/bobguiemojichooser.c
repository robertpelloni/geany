/* bobguiemojichooser.c: An Emoji chooser widget
 * Copyright 2017, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "bobguiemojichooser.h"

#include "bobguiadjustmentprivate.h"
#include "bobguibox.h"
#include "bobguibutton.h"
#include "bobguientry.h"
#include "bobguiflowboxprivate.h"
#include "bobguistack.h"
#include "bobguilabel.h"
#include "bobguigesturelongpress.h"
#include "bobguipopover.h"
#include "bobguiscrolledwindow.h"
#include "bobguisearchentryprivate.h"
#include "bobguitext.h"
#include "bobguinative.h"
#include "bobguiwidgetprivate.h"
#include "gdk/gdkprofilerprivate.h"
#include "bobguimain.h"
#include "bobguiprivate.h"

/**
 * BobguiEmojiChooser:
 *
 * Used by text widgets to let users insert Emoji characters.
 *
 * <picture>
 *   <source srcset="emojichooser-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiEmojiChooser" src="emojichooser.png">
 * </picture>
 *
 * `BobguiEmojiChooser` emits the [signal@Bobgui.EmojiChooser::emoji-picked]
 * signal when an Emoji is selected.
 *
 * # Shortcuts and Gestures
 *
 * `BobguiEmojiChooser` supports the following keyboard shortcuts:
 *
 * - <kbd>Ctrl</kbd>+<kbd>N</kbd> scrolls to the next section.
 * - <kbd>Ctrl</kbd>+<kbd>P</kbd> scrolls to the previous section.
 * - <kbd>Enter</kbd> to select the first emoji result.
 *
 * # Actions
 *
 * `BobguiEmojiChooser` defines a set of built-in actions:
 *
 * - `scroll.section` scrolls to the next or previous section.
 *
 * # CSS nodes
 *
 * ```
 * popover
 * ├── box.emoji-searchbar
 * │   ╰── entry.search
 * ╰── box.emoji-toolbar
 *     ├── button.image-button.emoji-section
 *     ├── ...
 *     ╰── button.image-button.emoji-section
 * ```
 *
 * Every `BobguiEmojiChooser` consists of a main node called popover.
 * The contents of the popover are largely implementation defined
 * and supposed to inherit general styles.
 * The top searchbar used to search emoji and gets the .emoji-searchbar
 * style class itself.
 * The bottom toolbar used to switch between different emoji categories
 * consists of buttons with the .emoji-section style class and gets the
 * .emoji-toolbar style class itself.
 */

#define BOX_SPACE 6

GType bobgui_emoji_chooser_child_get_type (void);

#define BOBGUI_TYPE_EMOJI_CHOOSER_CHILD (bobgui_emoji_chooser_child_get_type ())

typedef struct
{
  BobguiFlowBoxChild parent;
  BobguiWidget *variations;
} BobguiEmojiChooserChild;

typedef struct
{
  BobguiFlowBoxChildClass parent_class;
} BobguiEmojiChooserChildClass;

G_DEFINE_TYPE (BobguiEmojiChooserChild, bobgui_emoji_chooser_child, BOBGUI_TYPE_FLOW_BOX_CHILD)

static void
bobgui_emoji_chooser_child_init (BobguiEmojiChooserChild *child)
{
}

static void
bobgui_emoji_chooser_child_dispose (GObject *object)
{
  BobguiEmojiChooserChild *child = (BobguiEmojiChooserChild *)object;

  g_clear_pointer (&child->variations, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_emoji_chooser_child_parent_class)->dispose (object);
}

static void
bobgui_emoji_chooser_child_size_allocate (BobguiWidget *widget,
                                       int        width,
                                       int        height,
                                       int        baseline)
{
  BobguiEmojiChooserChild *child = (BobguiEmojiChooserChild *)widget;

  BOBGUI_WIDGET_CLASS (bobgui_emoji_chooser_child_parent_class)->size_allocate (widget, width, height, baseline);
  if (child->variations)
    bobgui_popover_present (BOBGUI_POPOVER (child->variations));
}

static gboolean
bobgui_emoji_chooser_child_focus (BobguiWidget        *widget,
                               BobguiDirectionType  direction)
{
  BobguiEmojiChooserChild *child = (BobguiEmojiChooserChild *)widget;

  if (child->variations && bobgui_widget_is_visible (child->variations))
    {
      if (bobgui_widget_child_focus (child->variations, direction))
        return TRUE;
    }

  return BOBGUI_WIDGET_CLASS (bobgui_emoji_chooser_child_parent_class)->focus (widget, direction);
}

static void scroll_to_child (BobguiWidget *child);

static gboolean
bobgui_emoji_chooser_child_grab_focus (BobguiWidget *widget)
{
  bobgui_widget_grab_focus_self (widget);
  scroll_to_child (widget);
  return TRUE;
}

static void show_variations (BobguiEmojiChooser *chooser,
                             BobguiWidget       *child);

static void
bobgui_emoji_chooser_child_popup_menu (BobguiWidget  *widget,
                                    const char *action_name,
                                    GVariant   *parameters)
{
  BobguiWidget *chooser;

  chooser = bobgui_widget_get_ancestor (widget, BOBGUI_TYPE_EMOJI_CHOOSER);

  show_variations (BOBGUI_EMOJI_CHOOSER (chooser), widget);
}

static void
bobgui_emoji_chooser_child_class_init (BobguiEmojiChooserChildClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = bobgui_emoji_chooser_child_dispose;
  widget_class->size_allocate = bobgui_emoji_chooser_child_size_allocate;
  widget_class->focus = bobgui_emoji_chooser_child_focus;
  widget_class->grab_focus = bobgui_emoji_chooser_child_grab_focus;

  bobgui_widget_class_install_action (widget_class, "menu.popup", NULL, bobgui_emoji_chooser_child_popup_menu);

  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_F10, GDK_SHIFT_MASK,
                                       "menu.popup",
                                       NULL);
  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_Menu, 0,
                                       "menu.popup",
                                       NULL);

  bobgui_widget_class_set_css_name (widget_class, "emoji");
}

typedef struct {
  BobguiWidget *box;
  BobguiWidget *heading;
  BobguiWidget *button;
  int group;
  gunichar label;
  gboolean empty;
} EmojiSection;

struct _BobguiEmojiChooser
{
  BobguiPopover parent_instance;

  BobguiWidget *search_entry;
  BobguiWidget *stack;
  BobguiWidget *scrolled_window;

  int emoji_max_width;

  EmojiSection recent;
  EmojiSection people;
  EmojiSection body;
  EmojiSection nature;
  EmojiSection food;
  EmojiSection travel;
  EmojiSection activities;
  EmojiSection objects;
  EmojiSection symbols;
  EmojiSection flags;

  GVariant *data;
  BobguiWidget *box;
  GVariantIter *iter;
  guint populate_idle;

  GSettings *settings;
};

struct _BobguiEmojiChooserClass {
  BobguiPopoverClass parent_class;
};

enum {
  EMOJI_PICKED,
  LAST_SIGNAL
};

static int signals[LAST_SIGNAL];

G_DEFINE_TYPE (BobguiEmojiChooser, bobgui_emoji_chooser, BOBGUI_TYPE_POPOVER)

static void
bobgui_emoji_chooser_finalize (GObject *object)
{
  BobguiEmojiChooser *chooser = BOBGUI_EMOJI_CHOOSER (object);

  if (chooser->populate_idle)
    g_source_remove (chooser->populate_idle);

  g_clear_pointer (&chooser->data, g_variant_unref);
  g_clear_pointer (&chooser->iter, g_variant_iter_free);
  g_clear_object (&chooser->settings);

  G_OBJECT_CLASS (bobgui_emoji_chooser_parent_class)->finalize (object);
}

static void
bobgui_emoji_chooser_dispose (GObject *object)
{
  bobgui_widget_dispose_template (BOBGUI_WIDGET (object), BOBGUI_TYPE_EMOJI_CHOOSER);

  G_OBJECT_CLASS (bobgui_emoji_chooser_parent_class)->dispose (object);
}

static void
activate_first_result (BobguiEmojiChooser *chooser,
                       BobguiFlowBox      *flow_box)
{
  BobguiFlowBoxChild *emoji_child;
  guint i;

  i = 0;
  while ((emoji_child = bobgui_flow_box_get_child_at_index (flow_box, i)) != NULL)
    {
      if (bobgui_widget_get_mapped (BOBGUI_WIDGET (emoji_child)))
        {
          bobgui_widget_grab_focus (BOBGUI_WIDGET (emoji_child));
          bobgui_widget_activate_action (BOBGUI_WIDGET (flow_box), "default.activate", NULL);
          return;
        }
      i++;
    }
}

static void
scroll_to_section (EmojiSection *section)
{
  BobguiEmojiChooser *chooser;
  BobguiAdjustment *adj;
  graphene_rect_t bounds = GRAPHENE_RECT_INIT (0, 0, 0, 0);

  chooser = BOBGUI_EMOJI_CHOOSER (bobgui_widget_get_ancestor (section->box, BOBGUI_TYPE_EMOJI_CHOOSER));

  adj = bobgui_scrolled_window_get_vadjustment (BOBGUI_SCROLLED_WINDOW (chooser->scrolled_window));
  if (section->heading)
    {
      if (!bobgui_widget_compute_bounds (section->heading, bobgui_widget_get_parent (section->heading), &bounds))
        graphene_rect_init (&bounds, 0, 0, 0, 0);
    }

  bobgui_adjustment_animate_to_value (adj, bounds.origin.y - BOX_SPACE);
}

static void
scroll_to_child (BobguiWidget *child)
{
  BobguiEmojiChooser *chooser;
  BobguiAdjustment *adj;
  graphene_point_t p;
  double value;
  double page_size;
  graphene_rect_t bounds = GRAPHENE_RECT_INIT (0, 0, 0, 0);

  chooser = BOBGUI_EMOJI_CHOOSER (bobgui_widget_get_ancestor (child, BOBGUI_TYPE_EMOJI_CHOOSER));

  adj = bobgui_scrolled_window_get_vadjustment (BOBGUI_SCROLLED_WINDOW (chooser->scrolled_window));

  if (!bobgui_widget_compute_bounds (child, bobgui_widget_get_parent (child), &bounds))
    graphene_rect_init (&bounds, 0, 0, 0, 0);

  value = bobgui_adjustment_get_value (adj);
  page_size = bobgui_adjustment_get_page_size (adj);

  if (!bobgui_widget_compute_point (child, bobgui_widget_get_parent (chooser->recent.box),
                                 &GRAPHENE_POINT_INIT (0, 0), &p))
    return;

  if (p.y < value)
    bobgui_adjustment_animate_to_value (adj, p.y);
  else if (p.y + bounds.size.height >= value + page_size)
    bobgui_adjustment_animate_to_value (adj, value + ((p.y + bounds.size.height) - (value + page_size)));
}

static void
add_emoji (BobguiWidget    *box,
           gboolean      prepend,
           GVariant     *item,
           gunichar      modifier,
           BobguiEmojiChooser *chooser);

#define MAX_RECENT (7*3)

static void
populate_recent_section (BobguiEmojiChooser *chooser)
{
  GVariant *variant;
  GVariant *item;
  GVariantIter iter;
  gboolean empty = TRUE;

  variant = g_settings_get_value (chooser->settings, "recently-used-emoji");
  g_variant_iter_init (&iter, variant);
  while ((item = g_variant_iter_next_value (&iter)))
    {
      GVariant *emoji_data;
      gunichar modifier;

      emoji_data = g_variant_get_child_value (item, 0);
      g_variant_get_child (item, 1, "u", &modifier);
      add_emoji (chooser->recent.box, FALSE, emoji_data, modifier, chooser);
      g_variant_unref (emoji_data);
      g_variant_unref (item);
      empty = FALSE;
    }

  bobgui_widget_set_visible (chooser->recent.box, !empty);
  bobgui_widget_set_sensitive (chooser->recent.button, !empty);

  g_variant_unref (variant);
}

static void
add_recent_item (BobguiEmojiChooser *chooser,
                 GVariant        *item,
                 gunichar         modifier)
{
  GList *children, *l;
  int i;
  GVariantBuilder builder;
  BobguiWidget *child;

  g_variant_ref (item);

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a((aussasasu)u)"));
  g_variant_builder_add (&builder, "(@(aussasasu)u)", item, modifier);

  children = NULL;
  for (child = bobgui_widget_get_last_child (chooser->recent.box);
       child != NULL;
       child = bobgui_widget_get_prev_sibling (child))
    children = g_list_prepend (children, child);

  for (l = children, i = 1; l; l = l->next, i++)
    {
      GVariant *item2 = g_object_get_data (G_OBJECT (l->data), "emoji-data");
      gunichar modifier2 = GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (l->data), "modifier"));

      if (modifier == modifier2 && g_variant_equal (item, item2))
        {
          bobgui_flow_box_remove (BOBGUI_FLOW_BOX (chooser->recent.box), l->data);
          i--;
          continue;
        }
      if (i >= MAX_RECENT)
        {
          bobgui_flow_box_remove (BOBGUI_FLOW_BOX (chooser->recent.box), l->data);
          continue;
        }

      g_variant_builder_add (&builder, "(@(aussasasu)u)", item2, modifier2);
    }
  g_list_free (children);

  add_emoji (chooser->recent.box, TRUE, item, modifier, chooser);

  /* Enable recent */
  bobgui_widget_set_visible (chooser->recent.box, TRUE);
  bobgui_widget_set_sensitive (chooser->recent.button, TRUE);

  g_settings_set_value (chooser->settings, "recently-used-emoji", g_variant_builder_end (&builder));

  g_variant_unref (item);
}

static gboolean
should_close (BobguiEmojiChooser *chooser)
{
  GdkDisplay *display;
  GdkSeat *seat;
  GdkDevice *device;
  GdkModifierType state;

  display = bobgui_widget_get_display (BOBGUI_WIDGET (chooser));
  seat = gdk_display_get_default_seat (display);
  device = gdk_seat_get_keyboard (seat);
  state = gdk_device_get_modifier_state (device);

  return (state & GDK_CONTROL_MASK) == 0;
}

static void
emoji_activated (BobguiFlowBox      *box,
                 BobguiFlowBoxChild *child,
                 gpointer         data)
{
  BobguiEmojiChooser *chooser = data;
  char *text;
  BobguiWidget *label;
  GVariant *item;
  gunichar modifier;

  label = bobgui_flow_box_child_get_child (child);
  text = g_strdup (bobgui_label_get_label (BOBGUI_LABEL (label)));

  item = (GVariant*) g_object_get_data (G_OBJECT (child), "emoji-data");
  modifier = (gunichar) GPOINTER_TO_UINT (g_object_get_data (G_OBJECT (child), "modifier"));
  if ((BobguiWidget *) box != chooser->recent.box)
    add_recent_item (chooser, item, modifier);

  g_signal_emit (data, signals[EMOJI_PICKED], 0, text);
  g_free (text);

  if (should_close (chooser))
    bobgui_popover_popdown (BOBGUI_POPOVER (chooser));
  else
    {
      BobguiWidget *popover;

      popover = bobgui_widget_get_ancestor (BOBGUI_WIDGET (box), BOBGUI_TYPE_POPOVER);
      if (popover != BOBGUI_WIDGET (chooser))
        bobgui_popover_popdown (BOBGUI_POPOVER (popover));
    }
}

static gboolean
has_variations (GVariant *emoji_data)
{
  GVariant *codes;
  gsize i;
  gboolean has_variations;

  has_variations = FALSE;
  codes = g_variant_get_child_value (emoji_data, 0);
  for (i = 0; i < g_variant_n_children (codes); i++)
    {
      gunichar code;
      g_variant_get_child (codes, i, "u", &code);
      if (code == 0 || code == 0x1f3fb)
        {
          has_variations = TRUE;
          break;
        }
    }
  g_variant_unref (codes);

  return has_variations;
}

static void
show_variations (BobguiEmojiChooser *chooser,
                 BobguiWidget       *child)
{
  BobguiWidget *popover;
  BobguiWidget *view;
  BobguiWidget *box;
  GVariant *emoji_data;
  BobguiWidget *parent_popover;
  gunichar modifier;
  BobguiEmojiChooserChild *ch = (BobguiEmojiChooserChild *)child;

  if (!child)
    return;

  emoji_data = (GVariant*) g_object_get_data (G_OBJECT (child), "emoji-data");
  if (!emoji_data)
    return;

  if (!has_variations (emoji_data))
    return;

  parent_popover = bobgui_widget_get_ancestor (child, BOBGUI_TYPE_POPOVER);
  g_clear_pointer (&ch->variations, bobgui_widget_unparent);
  popover = ch->variations = bobgui_popover_new ();
  bobgui_popover_set_autohide (BOBGUI_POPOVER (popover), TRUE);
  bobgui_widget_set_parent (popover, child);
  view = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_add_css_class (view, "view");
  box = bobgui_flow_box_new ();
  bobgui_flow_box_set_homogeneous (BOBGUI_FLOW_BOX (box), TRUE);
  bobgui_flow_box_set_min_children_per_line (BOBGUI_FLOW_BOX (box), 6);
  bobgui_flow_box_set_max_children_per_line (BOBGUI_FLOW_BOX (box), 6);
  bobgui_flow_box_set_activate_on_single_click (BOBGUI_FLOW_BOX (box), TRUE);
  bobgui_flow_box_set_selection_mode (BOBGUI_FLOW_BOX (box), BOBGUI_SELECTION_NONE);
  g_object_set (box, "accept-unpaired-release", TRUE, NULL);
  bobgui_popover_set_child (BOBGUI_POPOVER (popover), view);
  bobgui_box_append (BOBGUI_BOX (view), box);

  g_signal_connect (box, "child-activated", G_CALLBACK (emoji_activated), parent_popover);

  add_emoji (box, FALSE, emoji_data, 0, chooser);
  for (modifier = 0x1f3fb; modifier <= 0x1f3ff; modifier++)
    add_emoji (box, FALSE, emoji_data, modifier, chooser);

  bobgui_popover_popup (BOBGUI_POPOVER (popover));
}

static void
long_pressed_cb (BobguiGesture *gesture,
                 double      x,
                 double      y,
                 gpointer    data)
{
  BobguiEmojiChooser *chooser = data;
  BobguiWidget *box;
  BobguiWidget *child;

  box = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture));
  child = BOBGUI_WIDGET (bobgui_flow_box_get_child_at_pos (BOBGUI_FLOW_BOX (box), x, y));
  show_variations (chooser, child);
}

static void
pressed_cb (BobguiGesture *gesture,
            int         n_press,
            double      x,
            double      y,
            gpointer    data)
{
  BobguiEmojiChooser *chooser = data;
  BobguiWidget *box;
  BobguiWidget *child;

  box = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture));
  child = BOBGUI_WIDGET (bobgui_flow_box_get_child_at_pos (BOBGUI_FLOW_BOX (box), x, y));
  show_variations (chooser, child);
}

static void
add_emoji (BobguiWidget    *box,
           gboolean      prepend,
           GVariant     *item,
           gunichar      modifier,
           BobguiEmojiChooser *chooser)
{
  BobguiWidget *child;
  BobguiWidget *label;
  PangoAttrList *attrs;
  GVariant *codes;
  char text[64];
  char *p = text;
  int i;
  PangoLayout *layout;
  PangoRectangle rect;
  gunichar code = 0;

  codes = g_variant_get_child_value (item, 0);
  for (i = 0; i < g_variant_n_children (codes); i++)
    {
      g_variant_get_child (codes, i, "u", &code);
      if (code == 0)
        code = modifier != 0 ? modifier : 0xfe0f;
      if (code == 0x1f3fb)
        code = modifier;
      if (code != 0)
        p += g_unichar_to_utf8 (code, p);
    }
  g_variant_unref (codes);

  p[0] = 0;

  label = bobgui_label_new (text);
  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_scale_new (PANGO_SCALE_X_LARGE));
  bobgui_label_set_attributes (BOBGUI_LABEL (label), attrs);
  pango_attr_list_unref (attrs);

  layout = bobgui_label_get_layout (BOBGUI_LABEL (label));
  pango_layout_get_extents (layout, &rect, NULL);

  /* Check for fallback rendering that generates too wide items */
  if (pango_layout_get_unknown_glyphs_count (layout) > 0 ||
      rect.width >= 1.5 * chooser->emoji_max_width)
    {
      g_object_ref_sink (label);
      g_object_unref (label);
      return;
    }

  child = g_object_new (BOBGUI_TYPE_EMOJI_CHOOSER_CHILD, NULL);
  g_object_set_data_full (G_OBJECT (child), "emoji-data",
                          g_variant_ref (item),
                          (GDestroyNotify)g_variant_unref);
  if (modifier != 0)
    g_object_set_data (G_OBJECT (child), "modifier", GUINT_TO_POINTER (modifier));

  bobgui_flow_box_child_set_child (BOBGUI_FLOW_BOX_CHILD (child), label);
  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (box), child, prepend ? 0 : -1);
}

static GBytes *
get_emoji_data_by_language (const char *lang)
{
  GBytes *bytes;
  char *path;
  GError *error = NULL;

  path = g_strconcat ("/org/bobgui/libbobgui/emoji/", lang, ".data", NULL);
  bytes = g_resources_lookup_data (path, 0, &error);
  if (bytes)
    {
      g_debug ("Found emoji data for %s in resource %s", lang, path);
      g_free (path);
      return bytes;
    }

  if (g_error_matches (error, G_RESOURCE_ERROR, G_RESOURCE_ERROR_NOT_FOUND))
    {
      char *filename;
      char *gresource_name;
      GMappedFile *file;

      g_clear_error (&error);

      gresource_name = g_strconcat (lang, ".gresource", NULL);
      filename = g_build_filename (_bobgui_get_data_prefix (), "share", "bobgui-4.0",
                                   "emoji", gresource_name, NULL);
      g_clear_pointer (&gresource_name, g_free);
      file = g_mapped_file_new (filename, FALSE, NULL);

      if (file)
        {
          GBytes *data;
          GResource *resource;

          data = g_mapped_file_get_bytes (file);
          g_mapped_file_unref (file);

          resource = g_resource_new_from_data (data, NULL);
          g_bytes_unref (data);

          g_debug ("Registering resource for Emoji data for %s from file %s", lang, filename);
          g_resources_register (resource);
          g_resource_unref (resource);

          bytes = g_resources_lookup_data (path, 0, NULL);
          if (bytes)
            {
              g_debug ("Found emoji data for %s in resource %s", lang, path);
              g_free (path);
              g_free (filename);
              return bytes;
            }
        }

      g_free (filename);
    }

  g_clear_error (&error);
  g_free (path);

  return NULL;
}

GBytes *
get_emoji_data (void)
{
  GBytes *bytes;
  const char *lang;

  lang = pango_language_to_string (bobgui_get_default_language ());
  bytes = get_emoji_data_by_language (lang);
  if (bytes)
    return bytes;

  if (strchr (lang, '-'))
    {
      char q[5];
      int i;

      for (i = 0; lang[i] != '-' && i < 4; i++)
        q[i] = lang[i];
      q[i] = '\0';

      bytes = get_emoji_data_by_language (q);
      if (bytes)
        return bytes;
    }

  bytes = get_emoji_data_by_language ("en");
  g_assert (bytes);

  return bytes;
}

static gboolean
populate_emoji_chooser (gpointer data)
{
  BobguiEmojiChooser *chooser = data;
  GVariant *item;
  gint64 start, now;

  start = g_get_monotonic_time ();

  if (!chooser->data)
    {
      GBytes *bytes;

      bytes = get_emoji_data ();

      chooser->data = g_variant_ref_sink (g_variant_new_from_bytes (G_VARIANT_TYPE ("a(aussasasu)"), bytes, TRUE));
      g_bytes_unref (bytes);
    }

  if (!chooser->iter)
    {
      chooser->iter = g_variant_iter_new (chooser->data);
      chooser->box = chooser->people.box;
    }

  while ((item = g_variant_iter_next_value (chooser->iter)))
    {
      guint group;

      g_variant_get_child (item, 5, "u", &group);

      if (group == chooser->people.group)
        chooser->box = chooser->people.box;
      else if (group == chooser->body.group)
        chooser->box = chooser->body.box;
      else if (group == chooser->nature.group)
        chooser->box = chooser->nature.box;
      else if (group == chooser->food.group)
        chooser->box = chooser->food.box;
      else if (group == chooser->travel.group)
        chooser->box = chooser->travel.box;
      else if (group == chooser->activities.group)
        chooser->box = chooser->activities.box;
      else if (group == chooser->objects.group)
        chooser->box = chooser->objects.box;
      else if (group == chooser->symbols.group)
        chooser->box = chooser->symbols.box;
      else if (group == chooser->flags.group)
        chooser->box = chooser->flags.box;

      add_emoji (chooser->box, FALSE, item, 0, chooser);
      g_variant_unref (item);

      now = g_get_monotonic_time ();
      if (now > start + 200) /* 2 ms */
        {
          gdk_profiler_add_mark (start * 1000, (now - start) * 1000, "Emojichooser populate", NULL);
          return G_SOURCE_CONTINUE;
        }
    }

  g_variant_iter_free (chooser->iter);
  chooser->iter = NULL;
  chooser->box = NULL;
  chooser->populate_idle = 0;

  gdk_profiler_end_mark (start, "Emojichooser populate (finish)", NULL);

  return G_SOURCE_REMOVE;
}

static void
adj_value_changed (BobguiAdjustment *adj,
                   gpointer       data)
{
  BobguiEmojiChooser *chooser = data;
  double value = bobgui_adjustment_get_value (adj);
  EmojiSection const *sections[] = {
    &chooser->recent,
    &chooser->people,
    &chooser->body,
    &chooser->nature,
    &chooser->food,
    &chooser->travel,
    &chooser->activities,
    &chooser->objects,
    &chooser->symbols,
    &chooser->flags,
  };
  EmojiSection const *select_section = sections[0];
  gsize i;

  /* Figure out which section the current scroll position is within */
  for (i = 0; i < G_N_ELEMENTS (sections); ++i)
    {
      EmojiSection const *section = sections[i];
      BobguiWidget *child;
      graphene_rect_t bounds = GRAPHENE_RECT_INIT (0, 0, 0, 0);

      if (!bobgui_widget_get_visible (section->box))
        continue;

      if (section->heading)
        child = section->heading;
      else
        child = section->box;

      if (!bobgui_widget_compute_bounds (child, bobgui_widget_get_parent (child), &bounds))
        graphene_rect_init (&bounds, 0, 0, 0, 0);

      if (value < bounds.origin.y - BOX_SPACE)
        break;

      select_section = section;
    }

  /* Un/Check the section buttons accordingly */
  for (i = 0; i < G_N_ELEMENTS (sections); ++i)
    {
      EmojiSection const *section = sections[i];

      if (section == select_section)
        bobgui_widget_set_state_flags (section->button, BOBGUI_STATE_FLAG_CHECKED, FALSE);
      else
        bobgui_widget_unset_state_flags (section->button, BOBGUI_STATE_FLAG_CHECKED);
    }
}

static gboolean
match_tokens (const char **term_tokens,
              const char **hit_tokens)
{
  int i, j;
  gboolean matched;

  matched = TRUE;

  for (i = 0; term_tokens[i]; i++)
    {
      for (j = 0; hit_tokens[j]; j++)
        if (g_str_has_prefix (hit_tokens[j], term_tokens[i]))
          goto one_matched;

      matched = FALSE;
      break;

one_matched:
      continue;
    }

  return matched;
}

static gboolean
filter_func (BobguiFlowBoxChild *child,
             gpointer         data)
{
  EmojiSection *section = data;
  BobguiEmojiChooser *chooser;
  GVariant *emoji_data;
  const char *text;
  const char *name_en;
  const char *name;
  const char **keywords_en;
  const char **keywords;
  char **term_tokens;
  char **name_tokens_en;
  char **name_tokens;
  gboolean res;

  res = TRUE;

  chooser = BOBGUI_EMOJI_CHOOSER (bobgui_widget_get_ancestor (BOBGUI_WIDGET (child), BOBGUI_TYPE_EMOJI_CHOOSER));
  text = bobgui_editable_get_text (BOBGUI_EDITABLE (chooser->search_entry));
  emoji_data = (GVariant *) g_object_get_data (G_OBJECT (child), "emoji-data");

  if (text[0] == 0)
    goto out;

  if (!emoji_data)
    goto out;

  term_tokens = g_str_tokenize_and_fold (text, "en", NULL);
  g_variant_get_child (emoji_data, 1, "&s", &name_en);
  name_tokens = g_str_tokenize_and_fold (name_en, "en", NULL);
  g_variant_get_child (emoji_data, 2, "&s", &name);
  name_tokens_en = g_str_tokenize_and_fold (name, "en", NULL);
  g_variant_get_child (emoji_data, 3, "^a&s", &keywords_en);
  g_variant_get_child (emoji_data, 4, "^a&s", &keywords);

  res = match_tokens ((const char **)term_tokens, (const char **)name_tokens) ||
        match_tokens ((const char **)term_tokens, (const char **)name_tokens_en) ||
        match_tokens ((const char **)term_tokens, keywords) ||
        match_tokens ((const char **)term_tokens, keywords_en);

  g_strfreev (term_tokens);
  g_strfreev (name_tokens);
  g_strfreev (name_tokens_en);
  g_free (keywords_en);
  g_free (keywords);

out:
  if (res)
    section->empty = FALSE;

  return res;
}

static void
invalidate_section (EmojiSection *section)
{
  section->empty = TRUE;
  bobgui_flow_box_invalidate_filter (BOBGUI_FLOW_BOX (section->box));
}

static void
update_headings (BobguiEmojiChooser *chooser)
{
  bobgui_widget_set_visible (chooser->people.heading, !chooser->people.empty);
  bobgui_widget_set_visible (chooser->people.box, !chooser->people.empty);
  bobgui_widget_set_visible (chooser->body.heading, !chooser->body.empty);
  bobgui_widget_set_visible (chooser->body.box, !chooser->body.empty);
  bobgui_widget_set_visible (chooser->nature.heading, !chooser->nature.empty);
  bobgui_widget_set_visible (chooser->nature.box, !chooser->nature.empty);
  bobgui_widget_set_visible (chooser->food.heading, !chooser->food.empty);
  bobgui_widget_set_visible (chooser->food.box, !chooser->food.empty);
  bobgui_widget_set_visible (chooser->travel.heading, !chooser->travel.empty);
  bobgui_widget_set_visible (chooser->travel.box, !chooser->travel.empty);
  bobgui_widget_set_visible (chooser->activities.heading, !chooser->activities.empty);
  bobgui_widget_set_visible (chooser->activities.box, !chooser->activities.empty);
  bobgui_widget_set_visible (chooser->objects.heading, !chooser->objects.empty);
  bobgui_widget_set_visible (chooser->objects.box, !chooser->objects.empty);
  bobgui_widget_set_visible (chooser->symbols.heading, !chooser->symbols.empty);
  bobgui_widget_set_visible (chooser->symbols.box, !chooser->symbols.empty);
  bobgui_widget_set_visible (chooser->flags.heading, !chooser->flags.empty);
  bobgui_widget_set_visible (chooser->flags.box, !chooser->flags.empty);

  if (chooser->recent.empty && chooser->people.empty &&
      chooser->body.empty && chooser->nature.empty &&
      chooser->food.empty && chooser->travel.empty &&
      chooser->activities.empty && chooser->objects.empty &&
      chooser->symbols.empty && chooser->flags.empty)
    bobgui_stack_set_visible_child_name (BOBGUI_STACK (chooser->stack), "empty");
  else
    bobgui_stack_set_visible_child_name (BOBGUI_STACK (chooser->stack), "list");
}

static void
search_changed (BobguiEntry *entry,
                gpointer  data)
{
  BobguiEmojiChooser *chooser = data;

  invalidate_section (&chooser->recent);
  invalidate_section (&chooser->people);
  invalidate_section (&chooser->body);
  invalidate_section (&chooser->nature);
  invalidate_section (&chooser->food);
  invalidate_section (&chooser->travel);
  invalidate_section (&chooser->activities);
  invalidate_section (&chooser->objects);
  invalidate_section (&chooser->symbols);
  invalidate_section (&chooser->flags);

  update_headings (chooser);
}

static void
stop_search (BobguiEntry *entry,
             gpointer  data)
{
  bobgui_popover_popdown (BOBGUI_POPOVER (data));
}



static void
activate_search (BobguiEmojiChooser *chooser,
                 BobguiEntry        *entry,
                 gpointer         data)
{
  if (chooser->recent.empty && chooser->people.empty &&
      chooser->body.empty && chooser->nature.empty &&
      chooser->food.empty && chooser->travel.empty &&
      chooser->activities.empty && chooser->objects.empty &&
      chooser->symbols.empty && chooser->flags.empty)
    return;

  if (!chooser->recent.empty)
    activate_first_result (chooser, BOBGUI_FLOW_BOX (chooser->recent.box));
  else if (!chooser->people.empty)
    activate_first_result (chooser, BOBGUI_FLOW_BOX (chooser->people.box));
  else if (!chooser->body.empty)
    activate_first_result (chooser, BOBGUI_FLOW_BOX (chooser->body.box));
  else if (!chooser->nature.empty)
    activate_first_result (chooser, BOBGUI_FLOW_BOX (chooser->nature.box));
  else if (!chooser->food.empty)
    activate_first_result (chooser, BOBGUI_FLOW_BOX (chooser->food.box));
  else if (!chooser->travel.empty)
    activate_first_result (chooser, BOBGUI_FLOW_BOX (chooser->travel.box));
  else if (!chooser->activities.empty)
    activate_first_result (chooser, BOBGUI_FLOW_BOX (chooser->activities.box));
  else if (!chooser->objects.empty)
    activate_first_result (chooser, BOBGUI_FLOW_BOX (chooser->objects.box));
  else if (!chooser->symbols.empty)
    activate_first_result (chooser, BOBGUI_FLOW_BOX (chooser->symbols.box));
  else if (!chooser->flags.empty)
    activate_first_result (chooser, BOBGUI_FLOW_BOX (chooser->flags.box));
}

static void
setup_section (BobguiEmojiChooser *chooser,
               EmojiSection    *section,
               int              group,
               const char      *icon)
{
  section->group = group;

  bobgui_button_set_icon_name (BOBGUI_BUTTON (section->button), icon);
  
  bobgui_flow_box_disable_move_cursor (BOBGUI_FLOW_BOX (section->box));
  bobgui_flow_box_set_filter_func (BOBGUI_FLOW_BOX (section->box), filter_func, section, NULL);
  g_signal_connect_swapped (section->button, "clicked", G_CALLBACK (scroll_to_section), section);
}

static void
bobgui_emoji_chooser_init (BobguiEmojiChooser *chooser)
{
  BobguiAdjustment *adj;
  BobguiText *text;

  chooser->settings = g_settings_new ("org.bobgui.bobgui4.Settings.EmojiChooser");

  bobgui_widget_init_template (BOBGUI_WIDGET (chooser));

  text = bobgui_search_entry_get_text_widget (BOBGUI_SEARCH_ENTRY (chooser->search_entry));
  bobgui_text_set_input_hints (text, BOBGUI_INPUT_HINT_NO_EMOJI);

  /* Get a reasonable maximum width for an emoji. We do this to
   * skip overly wide fallback rendering for certain emojis the
   * font does not contain and therefore end up being rendered
   * as multiply glyphs.
   */
  {
    PangoLayout *layout = bobgui_widget_create_pango_layout (BOBGUI_WIDGET (chooser), "🙂");
    PangoAttrList *attrs;
    PangoRectangle rect;

    attrs = pango_attr_list_new ();
    pango_attr_list_insert (attrs, pango_attr_scale_new (PANGO_SCALE_X_LARGE));
    pango_layout_set_attributes (layout, attrs);
    pango_attr_list_unref (attrs);

    pango_layout_get_extents (layout, &rect, NULL);
    chooser->emoji_max_width = rect.width;

    g_object_unref (layout);
  }

  adj = bobgui_scrolled_window_get_vadjustment (BOBGUI_SCROLLED_WINDOW (chooser->scrolled_window));
  g_signal_connect (adj, "value-changed", G_CALLBACK (adj_value_changed), chooser);

  setup_section (chooser, &chooser->recent, -1, "emoji-recent-symbolic");
  setup_section (chooser, &chooser->people, 0, "emoji-people-symbolic");
  setup_section (chooser, &chooser->body, 1, "emoji-body-symbolic");
  setup_section (chooser, &chooser->nature, 3, "emoji-nature-symbolic");
  setup_section (chooser, &chooser->food, 4, "emoji-food-symbolic");
  setup_section (chooser, &chooser->travel, 5, "emoji-travel-symbolic");
  setup_section (chooser, &chooser->activities, 6, "emoji-activities-symbolic");
  setup_section (chooser, &chooser->objects, 7, "emoji-objects-symbolic");
  setup_section (chooser, &chooser->symbols, 8, "emoji-symbols-symbolic");
  setup_section (chooser, &chooser->flags, 9, "emoji-flags-symbolic");

  populate_recent_section (chooser);

  chooser->populate_idle = g_idle_add (populate_emoji_chooser, chooser);
  gdk_source_set_static_name_by_id (chooser->populate_idle, "[bobgui] populate_emoji_chooser");
}

static void
bobgui_emoji_chooser_show (BobguiWidget *widget)
{
  BobguiEmojiChooser *chooser = BOBGUI_EMOJI_CHOOSER (widget);
  BobguiAdjustment *adj;

  BOBGUI_WIDGET_CLASS (bobgui_emoji_chooser_parent_class)->show (widget);

  adj = bobgui_scrolled_window_get_vadjustment (BOBGUI_SCROLLED_WINDOW (chooser->scrolled_window));
  bobgui_adjustment_set_value (adj, 0);
  adj_value_changed (adj, chooser);

  bobgui_editable_set_text (BOBGUI_EDITABLE (chooser->search_entry), "");
}

static EmojiSection *
find_section (BobguiEmojiChooser *chooser,
              BobguiWidget       *box)
{
  if (box == chooser->recent.box)
    return &chooser->recent;
  else if (box == chooser->people.box)
    return &chooser->people;
  else if (box == chooser->body.box)
    return &chooser->body;
  else if (box == chooser->nature.box)
    return &chooser->nature;
  else if (box == chooser->food.box)
    return &chooser->food;
  else if (box == chooser->travel.box)
    return &chooser->travel;
  else if (box == chooser->activities.box)
    return &chooser->activities;
  else if (box == chooser->objects.box)
    return &chooser->objects;
  else if (box == chooser->symbols.box)
    return &chooser->symbols;
  else if (box == chooser->flags.box)
    return &chooser->flags;
  else
    return NULL;
}

static EmojiSection *
find_next_section (BobguiEmojiChooser *chooser,
                   BobguiWidget       *box,
                   gboolean         down)
{
  EmojiSection *next;

  if (box == chooser->recent.box)
    next = down ? &chooser->people : NULL;
  else if (box == chooser->people.box)
    next = down ? &chooser->body : &chooser->recent;
  else if (box == chooser->body.box)
    next = down ? &chooser->nature : &chooser->people;
  else if (box == chooser->nature.box)
    next = down ? &chooser->food : &chooser->body;
  else if (box == chooser->food.box)
    next = down ? &chooser->travel : &chooser->nature;
  else if (box == chooser->travel.box)
    next = down ? &chooser->activities : &chooser->food;
  else if (box == chooser->activities.box)
    next = down ? &chooser->objects : &chooser->travel;
  else if (box == chooser->objects.box)
    next = down ? &chooser->symbols : &chooser->activities;
  else if (box == chooser->symbols.box)
    next = down ? &chooser->flags : &chooser->objects;
  else if (box == chooser->flags.box)
    next = down ? NULL : &chooser->symbols;
  else
    next = NULL;

  return next;
}

static void
bobgui_emoji_chooser_scroll_section (BobguiWidget  *widget,
                                  const char *action_name,
                                  GVariant   *parameter)
{
  BobguiEmojiChooser *chooser = BOBGUI_EMOJI_CHOOSER (widget);
  int direction = g_variant_get_int32 (parameter);
  BobguiWidget *focus;
  BobguiWidget *box;
  EmojiSection *next;

  focus = bobgui_root_get_focus (bobgui_widget_get_root (widget));
  if (focus == NULL)
    return;

  if (bobgui_widget_is_ancestor (focus, chooser->search_entry))
    box = chooser->recent.box;
  else
    box = bobgui_widget_get_ancestor (focus, BOBGUI_TYPE_FLOW_BOX);

  next = find_next_section (chooser, box, direction > 0);

  if (next)
    {
      bobgui_widget_child_focus (next->box, BOBGUI_DIR_TAB_FORWARD);
      scroll_to_section (next);
    }
}

static gboolean
keynav_failed (BobguiWidget        *box,
               BobguiDirectionType  direction,
               BobguiEmojiChooser  *chooser)
{
  EmojiSection *next;
  BobguiWidget *focus;
  BobguiWidget *child;
  BobguiWidget *sibling;
  int i;
  int column;
  int child_x;
  graphene_rect_t bounds = GRAPHENE_RECT_INIT (0, 0, 0, 0);

  focus = bobgui_root_get_focus (bobgui_widget_get_root (box));
  if (focus == NULL)
    return FALSE;

  child = bobgui_widget_get_ancestor (focus, BOBGUI_TYPE_EMOJI_CHOOSER_CHILD);

  column = 0;
  child_x = G_MAXINT;
  for (sibling = bobgui_widget_get_first_child (box);
       sibling;
       sibling = bobgui_widget_get_next_sibling (sibling))
    {
      if (!bobgui_widget_get_child_visible (sibling))
        continue;

      if (!bobgui_widget_compute_bounds (sibling, box, &bounds))
        graphene_rect_init (&bounds, 0, 0, 0, 0);

      if (bounds.origin.x < child_x)
        column = 0;
      else
        column++;

      child_x = (int) bounds.origin.x;

      if (sibling == child)
        break;
    }

  if (direction == BOBGUI_DIR_DOWN)
   {
      next = find_section (chooser, box);
      while (TRUE)
        {
          next = find_next_section (chooser, next->box, TRUE);
          if (next == NULL)
            return FALSE;

          i = 0;
          child_x = G_MAXINT;
          for (sibling = bobgui_widget_get_first_child (next->box);
               sibling;
               sibling = bobgui_widget_get_next_sibling (sibling))
            {
              if (!bobgui_widget_get_child_visible (sibling))
                continue;

              if (!bobgui_widget_compute_bounds (sibling, next->box, &bounds))
                graphene_rect_init (&bounds, 0, 0, 0, 0);

              if (bounds.origin.x < child_x)
                i = 0;
              else
                i++;

              child_x = (int) bounds.origin.x;

              if (i == column)
                {
                  bobgui_widget_grab_focus (sibling);
                  return TRUE;
                }
            }
        }
    }
  else if (direction == BOBGUI_DIR_UP)
    {
      next = find_section (chooser, box);
      while (TRUE)
        {
          next = find_next_section (chooser, next->box, FALSE);
          if (next == NULL)
            return FALSE;

          i = 0;
          child_x = G_MAXINT;
          child = NULL;
          for (sibling = bobgui_widget_get_first_child (next->box);
               sibling;
               sibling = bobgui_widget_get_next_sibling (sibling))
            {
              if (!bobgui_widget_get_child_visible (sibling))
                continue;

              if (!bobgui_widget_compute_bounds (sibling, next->box, &bounds))
                graphene_rect_init (&bounds, 0, 0, 0, 0);

              if (bounds.origin.x < child_x)
                i = 0;
              else
                i++;

              child_x = (int) bounds.origin.x;

              if (i == column)
                child = sibling;
            }

          if (child)
            {
              bobgui_widget_grab_focus (child);
              return TRUE;
            }
        }
    }

  return FALSE;
}

static void
bobgui_emoji_chooser_map (BobguiWidget *widget)
{
  BobguiEmojiChooser *chooser = BOBGUI_EMOJI_CHOOSER (widget);

  BOBGUI_WIDGET_CLASS (bobgui_emoji_chooser_parent_class)->map (widget);

  bobgui_widget_grab_focus (chooser->search_entry);
}

static void
bobgui_emoji_chooser_class_init (BobguiEmojiChooserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->finalize = bobgui_emoji_chooser_finalize;
  object_class->dispose = bobgui_emoji_chooser_dispose;
  widget_class->show = bobgui_emoji_chooser_show;
  widget_class->map = bobgui_emoji_chooser_map;

  /**
   * BobguiEmojiChooser::emoji-picked:
   * @chooser: the `BobguiEmojiChooser`
   * @text: the Unicode sequence for the picked Emoji, in UTF-8
   *
   * Emitted when the user selects an Emoji.
   */
  signals[EMOJI_PICKED] = g_signal_new ("emoji-picked",
                                        G_OBJECT_CLASS_TYPE (object_class),
                                        G_SIGNAL_RUN_LAST,
                                        0,
                                        NULL, NULL,
                                        NULL,
                                        G_TYPE_NONE, 1, G_TYPE_STRING|G_SIGNAL_TYPE_STATIC_SCOPE);

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/ui/bobguiemojichooser.ui");

  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, search_entry);
  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, stack);
  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, scrolled_window);

  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, recent.box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, recent.button);

  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, people.box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, people.heading);
  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, people.button);

  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, body.box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, body.heading);
  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, body.button);

  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, nature.box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, nature.heading);
  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, nature.button);

  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, food.box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, food.heading);
  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, food.button);

  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, travel.box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, travel.heading);
  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, travel.button);

  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, activities.box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, activities.heading);
  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, activities.button);

  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, objects.box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, objects.heading);
  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, objects.button);

  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, symbols.box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, symbols.heading);
  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, symbols.button);

  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, flags.box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, flags.heading);
  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiChooser, flags.button);

  bobgui_widget_class_bind_template_callback (widget_class, emoji_activated);
  bobgui_widget_class_bind_template_callback (widget_class, search_changed);
  bobgui_widget_class_bind_template_callback (widget_class, stop_search);
  bobgui_widget_class_bind_template_callback (widget_class, activate_search);
  bobgui_widget_class_bind_template_callback (widget_class, pressed_cb);
  bobgui_widget_class_bind_template_callback (widget_class, long_pressed_cb);
  bobgui_widget_class_bind_template_callback (widget_class, keynav_failed);

  /**
   * BobguiEmojiChooser|scroll.section:
   * @direction: 1 to scroll forward, -1 to scroll back
   *
   * Scrolls to the next or previous section.
   */
  bobgui_widget_class_install_action (widget_class, "scroll.section", "i",
                                   bobgui_emoji_chooser_scroll_section);

  bobgui_widget_class_add_binding_action (widget_class, GDK_KEY_n, GDK_CONTROL_MASK,
                                       "scroll.section", "i", 1);
  bobgui_widget_class_add_binding_action (widget_class, GDK_KEY_p, GDK_CONTROL_MASK,
                                       "scroll.section", "i", -1);
}

/**
 * bobgui_emoji_chooser_new:
 *
 * Creates a new `BobguiEmojiChooser`.
 *
 * Returns: a new `BobguiEmojiChooser`
 */
BobguiWidget *
bobgui_emoji_chooser_new (void)
{
  return BOBGUI_WIDGET (g_object_new (BOBGUI_TYPE_EMOJI_CHOOSER, NULL));
}
