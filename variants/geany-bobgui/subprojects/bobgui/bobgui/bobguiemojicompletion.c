/* bobguiemojicompletion.c: An Emoji picker widget
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

#include "bobguiemojicompletion.h"

#include "bobguitextprivate.h"
#include "bobguieditable.h"
#include "bobguibox.h"
#include "bobguicssprovider.h"
#include "bobguilistbox.h"
#include "bobguilabel.h"
#include "bobguipopover.h"
#include "bobguiprivate.h"
#include "bobguigesturelongpress.h"
#include "bobguieventcontrollerkey.h"
#include "bobguiflowbox.h"
#include "bobguistack.h"

struct _BobguiEmojiCompletion
{
  BobguiPopover parent_instance;

  BobguiText *entry;
  char *text;
  guint length;
  guint offset;
  gulong changed_id;
  guint n_matches;

  BobguiWidget *list;
  BobguiWidget *active;
  BobguiWidget *active_variation;

  GVariant *data;
};

struct _BobguiEmojiCompletionClass {
  BobguiPopoverClass parent_class;
};

static void connect_signals    (BobguiEmojiCompletion *completion,
                                BobguiText            *text);
static void disconnect_signals (BobguiEmojiCompletion *completion);
static int populate_completion (BobguiEmojiCompletion *completion,
                                const char          *text,
                                guint                offset);

#define MAX_ROWS 5

G_DEFINE_TYPE (BobguiEmojiCompletion, bobgui_emoji_completion, BOBGUI_TYPE_POPOVER)

static void
bobgui_emoji_completion_finalize (GObject *object)
{
  BobguiEmojiCompletion *completion = BOBGUI_EMOJI_COMPLETION (object);

  disconnect_signals (completion);

  g_free (completion->text);
  g_variant_unref (completion->data);

  G_OBJECT_CLASS (bobgui_emoji_completion_parent_class)->finalize (object);
}

static void
update_completion (BobguiEmojiCompletion *completion)
{
  const char *text;
  guint length;
  guint n_matches;

  n_matches = 0;

  text = bobgui_editable_get_text (BOBGUI_EDITABLE (completion->entry));
  length = strlen (text);

  if (length > 0)
    {
      gboolean found_candidate = FALSE;
      const char *p;

      p = text + length;
      do
        {
next:
          p = g_utf8_prev_char (p);
          if (*p == ':')
            {
              if (p + 1 == text + length)
                goto next;

              if (p == text || !g_unichar_isalnum (g_utf8_get_char (p - 1)))
                {
                  found_candidate = TRUE;
                }

              break;
            }
        }
      while (p > text &&
             (g_unichar_isalnum (g_utf8_get_char (p)) || *p == '_' || *p == ' '));

      if (found_candidate)
        n_matches = populate_completion (completion, p, 0);
    }

  if (n_matches > 0)
    bobgui_popover_popup (BOBGUI_POPOVER (completion));
  else
    bobgui_popover_popdown (BOBGUI_POPOVER (completion));
}

static void
changed_cb (BobguiText            *text,
            BobguiEmojiCompletion *completion)
{
  update_completion (completion);
}

static void
emoji_activated (BobguiWidget          *row,
                 BobguiEmojiCompletion *completion)
{
  const char *emoji;
  guint length;

  bobgui_popover_popdown (BOBGUI_POPOVER (completion));

  emoji = (const char *)g_object_get_data (G_OBJECT (row), "text");

  g_signal_handler_block (completion->entry, completion->changed_id);

  length = g_utf8_strlen (bobgui_editable_get_text (BOBGUI_EDITABLE (completion->entry)), -1);
  bobgui_editable_select_region (BOBGUI_EDITABLE (completion->entry), length - completion->length, length);
  bobgui_text_enter_text (completion->entry, emoji);

  g_signal_handler_unblock (completion->entry, completion->changed_id);
}

static void
row_activated (BobguiListBox    *list,
               BobguiListBoxRow *row,
               gpointer       data)
{
  BobguiEmojiCompletion *completion = data;

  emoji_activated (BOBGUI_WIDGET (row), completion);
}

static void
child_activated (BobguiFlowBox      *box,
                 BobguiFlowBoxChild *child,
                 gpointer         data)
{
  BobguiEmojiCompletion *completion = data;

  emoji_activated (BOBGUI_WIDGET (child), completion);
}

static void
move_active_row (BobguiEmojiCompletion *completion,
                 int                 direction)
{
  BobguiWidget *child;

  for (child = bobgui_widget_get_first_child (completion->list);
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    bobgui_widget_unset_state_flags (child, BOBGUI_STATE_FLAG_FOCUSED);

  if (completion->active != NULL)
    {
      if (direction == 1)
        completion->active = bobgui_widget_get_next_sibling (completion->active);
      else
        completion->active = bobgui_widget_get_prev_sibling (completion->active);
    }

  if (completion->active == NULL)
    {
      if (direction == 1)
        completion->active = bobgui_widget_get_first_child (completion->list);
      else
        completion->active = bobgui_widget_get_last_child (completion->list);
    }

  if (completion->active != NULL)
    bobgui_widget_set_state_flags (completion->active, BOBGUI_STATE_FLAG_FOCUSED, FALSE);

  if (completion->active_variation)
    {
      bobgui_widget_unset_state_flags (completion->active_variation, BOBGUI_STATE_FLAG_FOCUSED);
      completion->active_variation = NULL;
    }
}

static void
activate_active_row (BobguiEmojiCompletion *completion)
{
  if (BOBGUI_IS_FLOW_BOX_CHILD (completion->active_variation))
    emoji_activated (completion->active_variation, completion);
  else if (completion->active != NULL)
    emoji_activated (completion->active, completion);
}

static void
show_variations (BobguiEmojiCompletion *completion,
                 BobguiWidget          *row,
                 gboolean            visible)
{
  BobguiWidget *stack;
  BobguiWidget *box;
  BobguiWidget *child;
  gboolean is_visible;

  if (!row)
    return;

  stack = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (row), "stack"));
  box = bobgui_stack_get_child_by_name (BOBGUI_STACK (stack), "variations");
  if (!box)
    return;

  is_visible = bobgui_stack_get_visible_child (BOBGUI_STACK (stack)) == box;
  if (is_visible == visible)
    return;

  bobgui_stack_set_visible_child_name (BOBGUI_STACK (stack), visible ? "variations" : "text");
  for (child = bobgui_widget_get_first_child (box); child; child = bobgui_widget_get_next_sibling (child))
    bobgui_widget_unset_state_flags (child, BOBGUI_STATE_FLAG_FOCUSED);
  completion->active_variation = NULL;
}

static gboolean
move_active_variation (BobguiEmojiCompletion *completion,
                       int                 direction)
{
  BobguiWidget *base;
  BobguiWidget *stack;
  BobguiWidget *box;
  BobguiWidget *next;

  if (!completion->active)
    return FALSE;

  base = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (completion->active), "base"));
  stack = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (completion->active), "stack"));
  box = bobgui_stack_get_child_by_name (BOBGUI_STACK (stack), "variations");

  if (bobgui_stack_get_visible_child (BOBGUI_STACK (stack)) != box)
    return FALSE;

  next = NULL;

  if (!completion->active_variation)
    next = base;
  else if (completion->active_variation == base && direction == 1)
    next = bobgui_widget_get_first_child (box);
  else if (completion->active_variation == bobgui_widget_get_first_child (box) && direction == -1)
    next = base;
  else if (direction == 1)
    next = bobgui_widget_get_next_sibling (completion->active_variation);
  else if (direction == -1)
    next = bobgui_widget_get_prev_sibling (completion->active_variation);

  if (next)
    {
      if (completion->active_variation)
        bobgui_widget_unset_state_flags (completion->active_variation, BOBGUI_STATE_FLAG_FOCUSED);
      completion->active_variation = next;
      bobgui_widget_set_state_flags (completion->active_variation, BOBGUI_STATE_FLAG_FOCUSED, FALSE);
    }

  return next != NULL;
}

static gboolean
key_press_cb (BobguiEventControllerKey *key,
              guint                  keyval,
              guint                  keycode,
              GdkModifierType        modifiers,
              BobguiEmojiCompletion    *completion)
{
  if (!bobgui_widget_get_visible (BOBGUI_WIDGET (completion)))
    return FALSE;

  if (keyval == GDK_KEY_Escape)
    {
      bobgui_popover_popdown (BOBGUI_POPOVER (completion));
      return TRUE;
    }

  if (keyval == GDK_KEY_Tab)
    {
      show_variations (completion, completion->active, FALSE);

      guint offset = completion->offset + MAX_ROWS;
      if (offset >= completion->n_matches)
        offset = 0;
      populate_completion (completion, completion->text, offset);
      return TRUE;
    }

  if (keyval == GDK_KEY_Up)
    {
      show_variations (completion, completion->active, FALSE);

      move_active_row (completion, -1);
      return TRUE;
    }

  if (keyval == GDK_KEY_Down)
    {
      show_variations (completion, completion->active, FALSE);

      move_active_row (completion, 1);
      return TRUE;
    }

  if (keyval == GDK_KEY_Return ||
      keyval == GDK_KEY_KP_Enter ||
      keyval == GDK_KEY_ISO_Enter)
    {
      activate_active_row (completion);
      return TRUE;
    }

  if (keyval == GDK_KEY_Right)
    {
      show_variations (completion, completion->active, TRUE);
      move_active_variation (completion, 1);
      return TRUE;
    }

  if (keyval == GDK_KEY_Left)
    {
      if (!move_active_variation (completion, -1))
        show_variations (completion, completion->active, FALSE);
      return TRUE;
    }

  return FALSE;
}

static gboolean
focus_out_cb (BobguiWidget          *text,
              GParamSpec         *pspec,
              BobguiEmojiCompletion *completion)
{
  if (!bobgui_widget_has_focus (text))
    bobgui_popover_popdown (BOBGUI_POPOVER (completion));
  return FALSE;
}

static void
connect_signals (BobguiEmojiCompletion *completion,
                 BobguiText            *entry)
{
  BobguiEventController *key_controller;

  completion->entry = g_object_ref (entry);
  key_controller = bobgui_text_get_key_controller (entry);

  g_signal_connect (key_controller, "key-pressed", G_CALLBACK (key_press_cb), completion);
  completion->changed_id = g_signal_connect (entry, "changed", G_CALLBACK (changed_cb), completion);
  g_signal_connect (entry, "notify::has-focus", G_CALLBACK (focus_out_cb), completion);
}

static void
disconnect_signals (BobguiEmojiCompletion *completion)
{
  BobguiEventController *key_controller;

  key_controller = bobgui_text_get_key_controller (completion->entry);

  g_signal_handlers_disconnect_by_func (completion->entry, changed_cb, completion);
  g_signal_handlers_disconnect_by_func (key_controller, key_press_cb, completion);
  g_signal_handlers_disconnect_by_func (completion->entry, focus_out_cb, completion);

  g_clear_object (&completion->entry);
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
get_text (GVariant *emoji_data,
          gunichar  modifier,
          char     *text,
          gsize     length)
{
  GVariant *codes;
  gsize i;
  char *p;

  p = text;
  codes = g_variant_get_child_value (emoji_data, 0);
  for (i = 0; i < g_variant_n_children (codes); i++)
    {
      gunichar code;

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
}

static void
add_emoji_variation (BobguiWidget *box,
                     GVariant  *emoji_data,
                     gunichar   modifier)
{
  BobguiWidget *child;
  BobguiWidget *label;
  PangoAttrList *attrs;
  char text[64];

  get_text (emoji_data, modifier, text, 64);

  label = bobgui_label_new (text);
  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_scale_new (PANGO_SCALE_X_LARGE));
  bobgui_label_set_attributes (BOBGUI_LABEL (label), attrs);
  pango_attr_list_unref (attrs);

  child = g_object_new (BOBGUI_TYPE_FLOW_BOX_CHILD, "css-name", "emoji", NULL);
  g_object_set_data_full (G_OBJECT (child), "text", g_strdup (text), g_free);
  g_object_set_data_full (G_OBJECT (child), "emoji-data",
                          g_variant_ref (emoji_data),
                          (GDestroyNotify)g_variant_unref);
  if (modifier != 0)
    g_object_set_data (G_OBJECT (child), "modifier", GUINT_TO_POINTER (modifier));

  bobgui_flow_box_child_set_child (BOBGUI_FLOW_BOX_CHILD (child), label);
  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (box), child, -1);
}

static void
add_emoji (BobguiWidget          *list,
           GVariant           *emoji_data,
           BobguiEmojiCompletion *completion)
{
  BobguiWidget *child;
  BobguiWidget *label;
  BobguiWidget *box;
  PangoAttrList *attrs;
  char text[64];
  const char *name;
  BobguiWidget *stack;
  gunichar modifier;

  get_text (emoji_data, 0, text, 64);

  label = bobgui_label_new (text);
  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_scale_new (PANGO_SCALE_X_LARGE));
  bobgui_label_set_attributes (BOBGUI_LABEL (label), attrs);
  pango_attr_list_unref (attrs);
  bobgui_widget_add_css_class (label, "emoji");

  child = g_object_new (BOBGUI_TYPE_LIST_BOX_ROW, "css-name", "emoji-completion-row", NULL);
  bobgui_widget_set_focus_on_click (child, FALSE);
  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_list_box_row_set_child (BOBGUI_LIST_BOX_ROW (child), box);
  bobgui_box_append (BOBGUI_BOX (box), label);
  g_object_set_data (G_OBJECT (child), "base", label);

  stack = bobgui_stack_new ();
  bobgui_stack_set_hhomogeneous (BOBGUI_STACK (stack), TRUE);
  bobgui_stack_set_vhomogeneous (BOBGUI_STACK (stack), TRUE);
  bobgui_stack_set_transition_type (BOBGUI_STACK (stack), BOBGUI_STACK_TRANSITION_TYPE_OVER_RIGHT_LEFT);
  bobgui_box_append (BOBGUI_BOX (box), stack);
  g_object_set_data (G_OBJECT (child), "stack", stack);

  g_variant_get_child (emoji_data, 1, "&s", &name);
  label = bobgui_label_new (name);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0);

  bobgui_stack_add_named (BOBGUI_STACK (stack), label, "text");

  if (has_variations (emoji_data))
    {
      box = bobgui_flow_box_new ();
      bobgui_flow_box_set_homogeneous (BOBGUI_FLOW_BOX (box), TRUE);
      bobgui_flow_box_set_min_children_per_line (BOBGUI_FLOW_BOX (box), 5);
      bobgui_flow_box_set_max_children_per_line (BOBGUI_FLOW_BOX (box), 5);
      bobgui_flow_box_set_activate_on_single_click (BOBGUI_FLOW_BOX (box), TRUE);
      bobgui_flow_box_set_selection_mode (BOBGUI_FLOW_BOX (box), BOBGUI_SELECTION_NONE);
      g_signal_connect (box, "child-activated", G_CALLBACK (child_activated), completion);
      for (modifier = 0x1f3fb; modifier <= 0x1f3ff; modifier++)
        add_emoji_variation (box, emoji_data, modifier);

      bobgui_stack_add_named (BOBGUI_STACK (stack), box, "variations");
    }

  g_object_set_data_full (G_OBJECT (child), "text", g_strdup (text), g_free);
  g_object_set_data_full (G_OBJECT (child), "emoji-data",
                          g_variant_ref (emoji_data), (GDestroyNotify)g_variant_unref);

  bobgui_list_box_insert (BOBGUI_LIST_BOX (list), child, -1);
}

static int
populate_completion (BobguiEmojiCompletion *completion,
                     const char         *text,
                     guint               offset)
{
  guint n_matches;
  guint n_added;
  GVariantIter iter;
  GVariant *item;
  BobguiWidget *child;

  if (completion->text != text)
    {
      g_free (completion->text);
      completion->text = g_strdup (text);
      completion->length = g_utf8_strlen (text, -1);
    }
  completion->offset = offset;

  while ((child = bobgui_widget_get_first_child (completion->list)))
    bobgui_list_box_remove (BOBGUI_LIST_BOX (completion->list), child);

  completion->active = NULL;

  n_matches = 0;
  n_added = 0;
  g_variant_iter_init (&iter, completion->data);
  while ((item = g_variant_iter_next_value (&iter)))
    {
      const char *name;

      g_variant_get_child (item, 1, "&s", &name);

      if (g_str_has_prefix (name, text + 1))
        {
          n_matches++;

          if (n_matches > offset && n_added < MAX_ROWS)
            {
              add_emoji (completion->list, item, completion);
              n_added++;
            }
        }
    }

  completion->n_matches = n_matches;

  if (n_added > 0)
    {
      completion->active = bobgui_widget_get_first_child (completion->list);
      bobgui_widget_set_state_flags (completion->active, BOBGUI_STATE_FLAG_FOCUSED, FALSE);
    }

  return n_added;
}

static void
long_pressed_cb (BobguiGesture *gesture,
                 double      x,
                 double      y,
                 gpointer    data)
{
  BobguiEmojiCompletion *completion = data;
  BobguiWidget *row;

  row = BOBGUI_WIDGET (bobgui_list_box_get_row_at_y (BOBGUI_LIST_BOX (completion->list), y));
  if (!row)
    return;

  show_variations (completion, row, TRUE);
}

static void
bobgui_emoji_completion_init (BobguiEmojiCompletion *completion)
{
  GBytes *bytes = NULL;
  BobguiGesture *long_press;

  bobgui_widget_init_template (BOBGUI_WIDGET (completion));

  bytes = get_emoji_data ();
  completion->data = g_variant_ref_sink (g_variant_new_from_bytes (G_VARIANT_TYPE ("a(ausasu)"), bytes, TRUE));

  g_bytes_unref (bytes);

  long_press = bobgui_gesture_long_press_new ();
  g_signal_connect (long_press, "pressed", G_CALLBACK (long_pressed_cb), completion);
  bobgui_widget_add_controller (completion->list, BOBGUI_EVENT_CONTROLLER (long_press));
}

static void
bobgui_emoji_completion_class_init (BobguiEmojiCompletionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->finalize = bobgui_emoji_completion_finalize;

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/ui/bobguiemojicompletion.ui");

  bobgui_widget_class_bind_template_child (widget_class, BobguiEmojiCompletion, list);

  bobgui_widget_class_bind_template_callback (widget_class, row_activated);
}

BobguiWidget *
bobgui_emoji_completion_new (BobguiText *text)
{
  BobguiEmojiCompletion *completion;

  completion = BOBGUI_EMOJI_COMPLETION (g_object_new (BOBGUI_TYPE_EMOJI_COMPLETION, NULL));
  bobgui_widget_set_parent (BOBGUI_WIDGET (completion), BOBGUI_WIDGET (text));

  connect_signals (completion, text);

  return BOBGUI_WIDGET (completion);
}
