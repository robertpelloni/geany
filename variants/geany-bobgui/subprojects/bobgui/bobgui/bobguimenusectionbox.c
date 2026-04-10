/*
 * Copyright © 2014 Canonical Limited
 * Copyright © 2013 Carlos Garnacho
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "config.h"

#include "bobguimenusectionboxprivate.h"

#include "bobguiwidgetprivate.h"
#include "bobguilabel.h"
#include "bobguimenutrackerprivate.h"
#include "bobguimodelbuttonprivate.h"
#include "bobguiseparator.h"
#include "bobguisizegroup.h"
#include "bobguistack.h"
#include "bobguipopovermenuprivate.h"
#include "bobguiorientable.h"
#include "bobguibuiltiniconprivate.h"
#include "bobguigizmoprivate.h"
#include "bobguibinlayout.h"
#include "bobguiprivate.h"

typedef BobguiBoxClass BobguiMenuSectionBoxClass;

struct _BobguiMenuSectionBox
{
  BobguiBox               parent_instance;

  BobguiMenuSectionBox   *toplevel;
  BobguiMenuTracker      *tracker;
  BobguiBox              *item_box;
  BobguiWidget           *separator;
  guint                separator_sync_idle;
  gboolean             iconic;
  gboolean             inline_buttons;
  gboolean             circular;
  int                  depth;
  BobguiPopoverMenuFlags  flags;
  BobguiSizeGroup        *indicators;
  GHashTable          *custom_slots;
};

typedef struct
{
  int      n_items;
  gboolean previous_is_iconic;
} MenuData;

G_DEFINE_TYPE (BobguiMenuSectionBox, bobgui_menu_section_box, BOBGUI_TYPE_BOX)

static void        bobgui_menu_section_box_sync_separators (BobguiMenuSectionBox  *box,
                                                         MenuData           *data);
static void        bobgui_menu_section_box_new_submenu     (BobguiMenuTrackerItem *item,
                                                         BobguiMenuSectionBox  *toplevel,
                                                         BobguiWidget          *focus,
                                                         const char         *name);
static BobguiWidget * bobgui_menu_section_box_new_section     (BobguiMenuTrackerItem *item,
                                                         BobguiMenuSectionBox  *parent);

/* We are trying to implement the following rules here:
 *
 * rule 1: never ever show separators for empty sections
 * rule 2: always show a separator if there is a label
 * rule 3: don't show a separator for the first section
 * rule 4: don't show a separator for the following sections if there are
 *         no items before it
 * rule 5: never show separators directly above or below an iconic box
 * (rule 6: these rules don't apply exactly the same way for subsections)
 */
static void
bobgui_menu_section_box_sync_separators (BobguiMenuSectionBox *box,
                                      MenuData          *data)
{
  gboolean previous_section_is_iconic;
  gboolean should_have_separator;
  gboolean should_have_top_margin = FALSE;
  gboolean is_not_empty_item;
  gboolean has_separator;
  gboolean has_label;
  gboolean separator_condition;
  int n_items_before;
  BobguiWidget *child;

  n_items_before =  data->n_items;
  previous_section_is_iconic = data->previous_is_iconic;

  for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (box->item_box));
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      if (BOBGUI_IS_MENU_SECTION_BOX (child))
        bobgui_menu_section_box_sync_separators (BOBGUI_MENU_SECTION_BOX (child), data);
      else
        data->n_items++;
    }

  is_not_empty_item = (data->n_items > n_items_before);

  if (is_not_empty_item)
    data->previous_is_iconic = box->iconic;

  if (box->separator == NULL)
    return;

  has_separator = bobgui_widget_get_parent (box->separator) != NULL;
  has_label = !BOBGUI_IS_SEPARATOR (box->separator);

  separator_condition = has_label ? TRUE : n_items_before > 0 &&
                                           box->depth <= 1 &&
                                           !previous_section_is_iconic &&
                                           !box->iconic;

  should_have_separator = separator_condition && is_not_empty_item;

  should_have_top_margin = !should_have_separator &&
                           (box->depth <= 1 || box->iconic || box->circular) &&
                           n_items_before > 0 &&
                           is_not_empty_item;

  bobgui_widget_set_margin_top (BOBGUI_WIDGET (box->item_box), should_have_top_margin ? 10 : 0);

  if (has_label)
    {
      BobguiWidget *separator = bobgui_widget_get_first_child (box->separator);

      bobgui_widget_set_visible (separator, n_items_before > 0);
    }

  if (should_have_separator == has_separator)
    return;

  if (should_have_separator)
    bobgui_box_insert_child_after (BOBGUI_BOX (box), box->separator, NULL);
  else
    bobgui_box_remove (BOBGUI_BOX (box), box->separator);
}

static gboolean
bobgui_menu_section_box_handle_sync_separators (gpointer user_data)
{
  BobguiMenuSectionBox *box = user_data;
  MenuData data;

  data.n_items = 0;
  data.previous_is_iconic = FALSE;
  bobgui_menu_section_box_sync_separators (box, &data);

  box->separator_sync_idle = 0;

  return G_SOURCE_REMOVE;
}

static void
bobgui_menu_section_box_schedule_separator_sync (BobguiMenuSectionBox *box)
{
  box = box->toplevel;

  if (!box->separator_sync_idle)
    {
      box->separator_sync_idle = g_idle_add_full (G_PRIORITY_DEFAULT, /* before menu is drawn... */
                                                  bobgui_menu_section_box_handle_sync_separators,
                                                  box, NULL);
      gdk_source_set_static_name_by_id (box->separator_sync_idle, "[bobgui] menu section box handle sync separators");
    }
}

static void
bobgui_popover_item_activate (BobguiWidget *button,
                           gpointer   user_data)
{
  BobguiMenuTrackerItem *item = user_data;
  BobguiWidget *popover = NULL;

  if (bobgui_menu_tracker_item_get_role (item) == BOBGUI_MENU_TRACKER_ITEM_ROLE_NORMAL)
    {
      /* Activating the item could cause the popover
       * to be free'd, for example if it is a Quit item
       */
      popover = bobgui_widget_get_ancestor (button, BOBGUI_TYPE_POPOVER);
      if (popover)
        g_object_ref (popover);
    }

  bobgui_menu_tracker_item_activated (item);

  if (popover != NULL)
    {
      bobgui_widget_set_visible (popover, FALSE);
      g_object_unref (popover);
    }
}

static void
bobgui_menu_section_box_remove_func (int      position,
                                  gpointer user_data)
{
  BobguiMenuSectionBox *box = user_data;
  BobguiMenuTrackerItem *item;
  BobguiWidget *widget;
  int pos;

  for (widget = bobgui_widget_get_first_child (BOBGUI_WIDGET (box->item_box)), pos = 0;
       widget != NULL;
       widget = bobgui_widget_get_next_sibling (widget), pos++)
    {
      if (pos == position)
        break;
    }

  item = g_object_get_data (G_OBJECT (widget), "BobguiMenuTrackerItem");
  if (bobgui_menu_tracker_item_get_has_link (item, G_MENU_LINK_SUBMENU))
    {
      BobguiWidget *stack, *subbox;

      stack = bobgui_widget_get_ancestor (BOBGUI_WIDGET (box->toplevel), BOBGUI_TYPE_STACK);
      subbox = bobgui_stack_get_child_by_name (BOBGUI_STACK (stack), bobgui_menu_tracker_item_get_label (item));
      if (subbox != NULL)
        bobgui_stack_remove (BOBGUI_STACK (stack), subbox);
    }

  bobgui_box_remove (BOBGUI_BOX (box->item_box), widget);

  bobgui_menu_section_box_schedule_separator_sync (box);
}

static gboolean
get_ancestors (BobguiWidget  *widget,
               GType       widget_type,
               BobguiWidget **ancestor,
               BobguiWidget **below)
{
  BobguiWidget *a, *b;

  a = NULL;
  b = widget;
  while (b != NULL)
    {
      a = bobgui_widget_get_parent (b);
      if (!a)
        return FALSE;
      if (g_type_is_a (G_OBJECT_TYPE (a), widget_type))
        break;
      b = a;
    }

  *below = b;
  *ancestor = a;

  return TRUE;
}

static void
close_submenu (BobguiWidget *button,
               gpointer   data)
{
  BobguiMenuTrackerItem *item = data;
  BobguiWidget *focus;

  if (bobgui_menu_tracker_item_get_should_request_show (item))
    bobgui_menu_tracker_item_request_submenu_shown (item, FALSE);

  focus = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (button), "focus"));
  bobgui_widget_grab_focus (focus);
}

static void
open_submenu (BobguiWidget *button,
              gpointer   data)
{
  BobguiMenuTrackerItem *item = data;
  BobguiWidget *focus;

  if (bobgui_menu_tracker_item_get_should_request_show (item))
    bobgui_menu_tracker_item_request_submenu_shown (item, TRUE);

  focus = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (button), "focus"));
  bobgui_widget_grab_focus (focus);
}

static void
submenu_shown (BobguiPopoverMenu     *popover,
               BobguiMenuTrackerItem *item)
{
  if (bobgui_menu_tracker_item_get_should_request_show (item))
    bobgui_menu_tracker_item_request_submenu_shown (item, TRUE);
}

static void
submenu_hidden (BobguiPopoverMenu     *popover,
                BobguiMenuTrackerItem *item)
{
  if (bobgui_menu_tracker_item_get_should_request_show (item))
    bobgui_menu_tracker_item_request_submenu_shown (item, FALSE);
}

/* We're using the gizmo as an easy single child container, not as
 * a custom widget to draw some visual indicators (like markers).
 * As such its default focus functions blocks focus through the children,
 * so we need to handle it correctly here so that custom widgets inside
 * menus can be focused.
 */
static gboolean
custom_widget_focus (BobguiGizmo        *gizmo,
                     BobguiDirectionType direction)
{
  return bobgui_widget_focus_child (BOBGUI_WIDGET (gizmo), direction);
}

static gboolean
custom_widget_grab_focus (BobguiGizmo *gizmo)
{
  return bobgui_widget_grab_focus_child (BOBGUI_WIDGET (gizmo));
}

static void
bobgui_menu_section_box_insert_func (BobguiMenuTrackerItem *item,
                                  int                 position,
                                  gpointer            user_data)
{
  BobguiMenuSectionBox *box = user_data;
  BobguiWidget *widget;

  if (bobgui_menu_tracker_item_get_is_separator (item))
    {
      widget = bobgui_menu_section_box_new_section (item, box);
    }
  else if (bobgui_menu_tracker_item_get_has_link (item, G_MENU_LINK_SUBMENU))
    {
      if (box->flags & BOBGUI_POPOVER_MENU_NESTED)
        {
          GMenuModel *model;
          BobguiWidget *submenu;

          model = _bobgui_menu_tracker_item_get_link (item, G_MENU_LINK_SUBMENU);

          submenu = bobgui_popover_menu_new_from_model_full (model, box->flags);
          bobgui_popover_set_has_arrow (BOBGUI_POPOVER (submenu), FALSE);
          bobgui_widget_set_valign (submenu, BOBGUI_ALIGN_START);

          widget = g_object_new (BOBGUI_TYPE_MODEL_BUTTON,
                                 "popover", submenu,
                                 "indicator-size-group", box->indicators,
                                 NULL);
          g_object_bind_property (item, "label", widget, "text", G_BINDING_SYNC_CREATE);
          g_object_bind_property (item, "icon", widget, "icon", G_BINDING_SYNC_CREATE);
          g_object_bind_property (item, "use-markup", widget, "use-markup", G_BINDING_SYNC_CREATE);
          g_object_bind_property (item, "sensitive", widget, "sensitive", G_BINDING_SYNC_CREATE);

          g_signal_connect (submenu, "show", G_CALLBACK (submenu_shown), item);
          g_signal_connect (submenu, "hide", G_CALLBACK (submenu_hidden), item);
        }
      else
        {
          BobguiWidget *stack = NULL;
          BobguiWidget *parent = NULL;
          char *name;

          widget = g_object_new (BOBGUI_TYPE_MODEL_BUTTON,
                                 "menu-name", bobgui_menu_tracker_item_get_label (item),
                                 "indicator-size-group", box->indicators,
                                 NULL);
          g_object_bind_property (item, "label", widget, "text", G_BINDING_SYNC_CREATE);
          g_object_bind_property (item, "icon", widget, "icon", G_BINDING_SYNC_CREATE);
          g_object_bind_property (item, "use-markup", widget, "use-markup", G_BINDING_SYNC_CREATE);
          g_object_bind_property (item, "sensitive", widget, "sensitive", G_BINDING_SYNC_CREATE);

          get_ancestors (BOBGUI_WIDGET (box->toplevel), BOBGUI_TYPE_STACK, &stack, &parent);
          g_object_get (bobgui_stack_get_page (BOBGUI_STACK (stack), parent), "name", &name, NULL);
          bobgui_menu_section_box_new_submenu (item, box->toplevel, widget, name);
          g_free (name);
        }
    }
  else if (bobgui_menu_tracker_item_get_custom (item))
    {
      const char *id = bobgui_menu_tracker_item_get_custom (item);

      widget = bobgui_gizmo_new ("widget", NULL, NULL, NULL, NULL, custom_widget_focus, custom_widget_grab_focus);
      bobgui_widget_set_layout_manager (widget, bobgui_bin_layout_new ());

      if (g_hash_table_lookup (box->custom_slots, id))
        g_warning ("Duplicate custom ID: %s", id);
      else
        {
          char *slot_id = g_strdup (id);
          g_object_set_data_full (G_OBJECT (widget), "slot-id", slot_id, g_free);
          g_hash_table_insert (box->custom_slots, slot_id, widget);
        }
    }
  else
    {
      widget = g_object_new (BOBGUI_TYPE_MODEL_BUTTON,
                             "indicator-size-group", box->indicators,
                             NULL);
      g_object_bind_property (item, "label", widget, "text", G_BINDING_SYNC_CREATE);

      if (box->iconic)
        {
          g_object_bind_property (item, "verb-icon", widget, "icon", G_BINDING_SYNC_CREATE);
          g_object_set (widget, "iconic", TRUE, NULL);
        }
      else if (box->inline_buttons)
        {
          g_object_bind_property (item, "verb-icon", widget, "icon", G_BINDING_SYNC_CREATE);
          g_object_set (widget, "iconic", TRUE, NULL);
          bobgui_widget_add_css_class (widget, "flat");
        }
      else if (box->circular)
        {
          g_object_bind_property (item, "verb-icon", widget, "icon", G_BINDING_SYNC_CREATE);
          g_object_set (widget, "iconic", TRUE, NULL);
          bobgui_widget_add_css_class (widget, "circular");
        }
      else
        g_object_bind_property (item, "icon", widget, "icon", G_BINDING_SYNC_CREATE);

      g_object_bind_property (item, "use-markup", widget, "use-markup", G_BINDING_SYNC_CREATE);
      g_object_bind_property (item, "sensitive", widget, "sensitive", G_BINDING_SYNC_CREATE);
      g_object_bind_property (item, "role", widget, "role", G_BINDING_SYNC_CREATE);
      g_object_bind_property (item, "toggled", widget, "active", G_BINDING_SYNC_CREATE);
      g_object_bind_property (item, "accel", widget, "accel", G_BINDING_SYNC_CREATE);
      g_signal_connect (widget, "clicked", G_CALLBACK (bobgui_popover_item_activate), item);
    }

  g_object_set_data_full (G_OBJECT (widget), "BobguiMenuTrackerItem", g_object_ref (item), g_object_unref);

  if (box->circular)
    {
      bobgui_widget_set_hexpand (widget, TRUE);
      bobgui_widget_set_halign (widget, BOBGUI_ALIGN_CENTER);
    }
  else
    {
      bobgui_widget_set_halign (widget, BOBGUI_ALIGN_FILL);
    }
  bobgui_box_append (BOBGUI_BOX (box->item_box), widget);

  if (position == 0)
    bobgui_box_reorder_child_after (BOBGUI_BOX (box->item_box), widget, NULL);
  else
    {
      BobguiWidget *sibling = bobgui_widget_get_first_child (BOBGUI_WIDGET (box->item_box));
      int i;
      for (i = 1; i < position; i++)
        sibling = bobgui_widget_get_next_sibling (sibling);
      bobgui_box_reorder_child_after (BOBGUI_BOX (box->item_box), widget, sibling);
    }

  if (box->circular)
    {
      BobguiWidget *c1, *c2, *c3;

      /* special-case the n > 2 case */
      c1 = bobgui_widget_get_first_child (BOBGUI_WIDGET (box->item_box));
      if ((c2 = bobgui_widget_get_next_sibling (c1)) != NULL &&
          (c3 = bobgui_widget_get_next_sibling (c2)) != NULL)
        {
          bobgui_widget_set_halign (c1, BOBGUI_ALIGN_START);
          while (c3 != NULL)
            {
              bobgui_widget_set_halign (c2, BOBGUI_ALIGN_CENTER);
              c2 = c3;
              c3 = bobgui_widget_get_next_sibling (c3);
            }
          bobgui_widget_set_halign (c2, BOBGUI_ALIGN_END);
        }
    }

  bobgui_menu_section_box_schedule_separator_sync (box);
}

static void
bobgui_menu_section_box_init (BobguiMenuSectionBox *box)
{
  BobguiWidget *item_box;

  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (box), BOBGUI_ORIENTATION_VERTICAL);

  box->toplevel = box;

  item_box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  box->item_box = BOBGUI_BOX (item_box);
  bobgui_box_append (BOBGUI_BOX (box), item_box);
  bobgui_widget_set_halign (BOBGUI_WIDGET (item_box), BOBGUI_ALIGN_FILL);
  bobgui_widget_set_halign (BOBGUI_WIDGET (box), BOBGUI_ALIGN_FILL);
}

static void
bobgui_menu_section_box_dispose (GObject *object)
{
  BobguiMenuSectionBox *box = BOBGUI_MENU_SECTION_BOX (object);

  if (box->separator_sync_idle)
    {
      g_source_remove (box->separator_sync_idle);
      box->separator_sync_idle = 0;
    }

  g_clear_object (&box->separator);

  if (box->tracker)
    {
      bobgui_menu_tracker_free (box->tracker);
      box->tracker = NULL;
    }

  g_clear_object (&box->indicators);
  g_clear_pointer (&box->custom_slots, g_hash_table_unref);

  G_OBJECT_CLASS (bobgui_menu_section_box_parent_class)->dispose (object);
}

static void
bobgui_menu_section_box_class_init (BobguiMenuSectionBoxClass *class)
{
  G_OBJECT_CLASS (class)->dispose = bobgui_menu_section_box_dispose;
}

static void
update_popover_position_cb (GObject    *source,
                            GParamSpec *spec,
                            gpointer   *user_data)
{
  BobguiPopover *popover = BOBGUI_POPOVER (source);
  BobguiMenuSectionBox *box = BOBGUI_MENU_SECTION_BOX (user_data);
  BobguiWidget *w;

  BobguiPositionType new_pos = bobgui_popover_get_position (popover);

  for (w = bobgui_widget_get_first_child (bobgui_widget_get_parent (BOBGUI_WIDGET (box)));
       w != NULL;
       w = bobgui_widget_get_next_sibling (w))
    {
      if (new_pos == BOBGUI_POS_BOTTOM)
        bobgui_widget_set_valign (w, BOBGUI_ALIGN_START);
      else if (new_pos == BOBGUI_POS_TOP)
        bobgui_widget_set_valign (w, BOBGUI_ALIGN_END);
      else
        bobgui_widget_set_valign (w, BOBGUI_ALIGN_CENTER);
    }
}

void
bobgui_menu_section_box_new_toplevel (BobguiPopoverMenu      *popover,
                                   GMenuModel          *model,
                                   BobguiPopoverMenuFlags  flags)
{
  BobguiMenuSectionBox *box;

  box = g_object_new (BOBGUI_TYPE_MENU_SECTION_BOX, NULL);
  box->indicators = bobgui_size_group_new (BOBGUI_SIZE_GROUP_HORIZONTAL);
  box->custom_slots = g_hash_table_new (g_str_hash, g_str_equal);
  box->flags = flags;

  bobgui_popover_menu_add_submenu (popover, BOBGUI_WIDGET (box), "main");

  box->tracker = bobgui_menu_tracker_new (BOBGUI_ACTION_OBSERVABLE (_bobgui_widget_get_action_muxer (BOBGUI_WIDGET (box), TRUE)),
                                       model, TRUE, FALSE, FALSE, NULL,
                                       bobgui_menu_section_box_insert_func,
                                       bobgui_menu_section_box_remove_func, box);

  g_signal_connect_object (G_OBJECT (popover), "notify::position", G_CALLBACK (update_popover_position_cb), box, G_CONNECT_DEFAULT);
}

static void
bobgui_menu_section_box_new_submenu (BobguiMenuTrackerItem *item,
                                  BobguiMenuSectionBox  *toplevel,
                                  BobguiWidget          *focus,
                                  const char         *name)
{
  BobguiMenuSectionBox *box;
  BobguiWidget *button;

  box = g_object_new (BOBGUI_TYPE_MENU_SECTION_BOX, NULL);
  box->indicators = bobgui_size_group_new (BOBGUI_SIZE_GROUP_HORIZONTAL);
  box->custom_slots = g_hash_table_ref (toplevel->custom_slots);
  box->flags = toplevel->flags;

  button = g_object_new (BOBGUI_TYPE_MODEL_BUTTON,
                         "menu-name", name,
                         "role", BOBGUI_BUTTON_ROLE_TITLE,
                         NULL);

  g_object_bind_property (item, "label", button, "text", G_BINDING_SYNC_CREATE);
  g_object_bind_property (item, "icon", button, "icon", G_BINDING_SYNC_CREATE);

  g_object_set_data (G_OBJECT (button), "focus", focus);
  g_object_set_data (G_OBJECT (focus), "focus", button);

  bobgui_box_insert_child_after (BOBGUI_BOX (box), button, NULL);

  g_signal_connect (focus, "clicked", G_CALLBACK (open_submenu), item);
  g_signal_connect (button, "clicked", G_CALLBACK (close_submenu), item);

  bobgui_stack_add_named (BOBGUI_STACK (bobgui_widget_get_ancestor (BOBGUI_WIDGET (toplevel), BOBGUI_TYPE_STACK)),
                       BOBGUI_WIDGET (box), bobgui_menu_tracker_item_get_label (item));

  box->tracker = bobgui_menu_tracker_new_for_item_link (item, G_MENU_LINK_SUBMENU, FALSE, FALSE,
                                                     bobgui_menu_section_box_insert_func,
                                                     bobgui_menu_section_box_remove_func,
                                                     box);
}

static BobguiWidget *
bobgui_menu_section_box_new_section (BobguiMenuTrackerItem *item,
                                  BobguiMenuSectionBox  *parent)
{
  BobguiMenuSectionBox *box;
  const char *label;
  const char *hint;
  const char *text_direction;

  box = g_object_new (BOBGUI_TYPE_MENU_SECTION_BOX, NULL);
  box->indicators = g_object_ref (parent->indicators);
  box->custom_slots = g_hash_table_ref (parent->toplevel->custom_slots);
  box->toplevel = parent->toplevel;
  box->depth = parent->depth + 1;
  box->flags = parent->flags;

  label = bobgui_menu_tracker_item_get_label (item);
  hint = bobgui_menu_tracker_item_get_display_hint (item);
  text_direction = bobgui_menu_tracker_item_get_text_direction (item);

  if (hint && g_str_equal (hint, "horizontal-buttons"))
    {
      bobgui_box_set_homogeneous (box->item_box, TRUE);
      bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (box->item_box), BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_widget_add_css_class (BOBGUI_WIDGET (box->item_box), "linked");
      bobgui_widget_add_css_class (BOBGUI_WIDGET (box->item_box), "horizontal-buttons");
      box->iconic = TRUE;

      if (text_direction)
        {
          BobguiTextDirection dir = BOBGUI_TEXT_DIR_NONE;

          if (g_str_equal (text_direction, "rtl"))
            dir = BOBGUI_TEXT_DIR_RTL;
          else if (g_str_equal (text_direction, "ltr"))
            dir = BOBGUI_TEXT_DIR_LTR;

          bobgui_widget_set_direction (BOBGUI_WIDGET (box->item_box), dir);
        }
    }
  else if (hint && g_str_equal (hint, "inline-buttons"))
    {
      BobguiWidget *item_box;
      BobguiWidget *spacer;

      box->inline_buttons = TRUE;

      bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (box->item_box), BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_widget_add_css_class (BOBGUI_WIDGET (box->item_box), "inline-buttons");

      spacer = bobgui_gizmo_new ("none", NULL, NULL, NULL,NULL, NULL, NULL);
      bobgui_box_append (BOBGUI_BOX (box->item_box), spacer);
      bobgui_size_group_add_widget (box->indicators, spacer);

      if (label != NULL)
        {
          BobguiWidget *title;

          title = bobgui_label_new (label);
          bobgui_widget_set_hexpand (title, TRUE);
          bobgui_widget_set_halign (title, BOBGUI_ALIGN_START);
          g_object_bind_property (item, "label", title, "label", G_BINDING_SYNC_CREATE);
          bobgui_box_append (BOBGUI_BOX (box->item_box), title);
          bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (box),
                                          BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY,
                                          title,
                                          NULL, -1);
        }

      item_box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_box_append (BOBGUI_BOX (box->item_box), item_box);
      box->item_box = BOBGUI_BOX (item_box);
    }
  else if (hint && g_str_equal (hint, "circular-buttons"))
    {
      bobgui_box_set_homogeneous (box->item_box, TRUE);
      bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (box->item_box), BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_widget_add_css_class (BOBGUI_WIDGET (box->item_box), "circular-buttons");
      box->circular = TRUE;
    }

  if (label != NULL && !box->inline_buttons)
    {
      BobguiWidget *separator;
      BobguiWidget *title;

      box->separator = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      g_object_ref_sink (box->separator);

      separator = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_widget_set_valign (separator, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_hexpand (separator, TRUE);
      bobgui_box_append (BOBGUI_BOX (box->separator), separator);

      title = bobgui_label_new (label);
      g_object_bind_property (item, "label", title, "label", G_BINDING_SYNC_CREATE);
      bobgui_widget_add_css_class (title, "separator");
      bobgui_widget_set_halign (title, BOBGUI_ALIGN_START);
      bobgui_label_set_xalign (BOBGUI_LABEL (title), 0.0);
      bobgui_widget_add_css_class (title, "title");
      bobgui_box_append (BOBGUI_BOX (box->separator), title);
      bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (box),
                                      BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY,
                                      title,
                                      NULL, -1);
    }
  else
    {
      box->separator = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
      g_object_ref_sink (box->separator);
    }

  box->tracker = bobgui_menu_tracker_new_for_item_link (item, G_MENU_LINK_SECTION, FALSE, FALSE,
                                                     bobgui_menu_section_box_insert_func,
                                                     bobgui_menu_section_box_remove_func,
                                                     box);

  return BOBGUI_WIDGET (box);
}

gboolean
bobgui_menu_section_box_add_custom (BobguiPopoverMenu *popover,
                                 BobguiWidget      *child,
                                 const char     *id)
{
  BobguiWidget *stack;
  BobguiMenuSectionBox *box;
  BobguiWidget *slot;

  stack = bobgui_popover_menu_get_stack (popover);
  box = BOBGUI_MENU_SECTION_BOX (bobgui_stack_get_child_by_name (BOBGUI_STACK (stack), "main"));
  if (box == NULL)
    return FALSE;

  slot = (BobguiWidget *)g_hash_table_lookup (box->custom_slots, id);

  if (slot == NULL)
    return FALSE;

  if (bobgui_widget_get_first_child (slot))
    return FALSE;

  bobgui_widget_insert_before (child, slot, NULL);
  return TRUE;
}

gboolean
bobgui_menu_section_box_remove_custom (BobguiPopoverMenu *popover,
                                    BobguiWidget      *child)
{
  BobguiWidget *stack;
  BobguiMenuSectionBox *box;
  BobguiWidget *parent;
  const char *id;
  BobguiWidget *slot;

  stack = bobgui_popover_menu_get_stack (popover);
  box = BOBGUI_MENU_SECTION_BOX (bobgui_stack_get_child_by_name (BOBGUI_STACK (stack), "main"));
  if (box == NULL)
    return FALSE;

  parent = bobgui_widget_get_parent (child);

  id = (const char *) g_object_get_data (G_OBJECT (parent), "slot-id");
  g_return_val_if_fail (id != NULL, FALSE);

  slot = (BobguiWidget *)g_hash_table_lookup (box->custom_slots, id);

  if (slot != parent)
    return FALSE;

  bobgui_widget_unparent (child);

  return TRUE;
}
