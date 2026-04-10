/* bobguiaccesskitcontext.c: AccessKit BobguiATContext implementation
 *
 * Copyright 2024  GNOME Foundation
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "bobguiaccesskitcontextprivate.h"

#include "bobguiaccessibleprivate.h"
#include "bobguiaccesskitrootprivate.h"

#include "bobguidebug.h"
#include "bobguiprivate.h"
#include "bobguieditable.h"
#include "bobguientryprivate.h"
#include "bobguiinscriptionprivate.h"
#include "bobguilabelprivate.h"
#include "bobguimodelbuttonprivate.h"
#include "bobguipasswordentry.h"
#include "bobguiroot.h"
#include "bobguiscrolledwindow.h"
#include "bobguistack.h"
#include "bobguitextview.h"
#include "bobguitextviewprivate.h"
#include "bobguitextbufferprivate.h"
#include "bobguitextiterprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguiwindow.h"

#include "bobguimenubutton.h"
#include "bobguicolordialogbutton.h"
#include "bobguifontdialogbutton.h"
#include "bobguiscalebutton.h"
#include "print/bobguiprinteroptionwidgetprivate.h"

#include <locale.h>

typedef struct _BobguiAccessKitTextLayout
{
  guint32 id;
  double offset_x;
  double offset_y;
  GArray *children;
} BobguiAccessKitTextLayout;

struct _BobguiAccessKitContext
{
  BobguiATContext parent_instance;

  /* Root object for the surface */
  BobguiAccessKitRoot *root;

  /* The AccessKit node ID; meaningless if root is null. Note that this ID
     is not a full 64-bit AccessKit node ID, for two reasons:
     1. It isn't possible to convert 64-bit integers to pointers
        (as required to use these IDs as GHashTable keys) on all platforms.
     2. By using only 32 bits for the IDs of AT contexts, we can use the other
        32 bits to identify inline text boxes or other children within
        a given context. */
  guint32 id;

  BobguiAccessKitTextLayout single_text_layout;
  GHashTable *text_view_lines;
  GHashTable *text_view_lines_by_id;
};

G_DEFINE_TYPE (BobguiAccessKitContext, bobgui_accesskit_context, BOBGUI_TYPE_AT_CONTEXT)

static void
bobgui_accesskit_context_finalize (GObject *gobject)
{
  BobguiAccessKitContext *self = BOBGUI_ACCESSKIT_CONTEXT (gobject);

  g_clear_object (&self->root);
  g_clear_pointer (&self->single_text_layout.children, g_array_unref);
  g_clear_pointer (&self->text_view_lines, g_hash_table_destroy);
  g_clear_pointer (&self->text_view_lines_by_id, g_hash_table_destroy);

  G_OBJECT_CLASS (bobgui_accesskit_context_parent_class)->finalize (gobject);
}

static void
bobgui_accesskit_context_realize (BobguiATContext *context)
{
  BobguiAccessKitContext *self = BOBGUI_ACCESSKIT_CONTEXT (context);
  BobguiAccessible *accessible = bobgui_at_context_get_accessible (context);


  if (BOBGUI_IS_ROOT (accessible))
    self->root = bobgui_accesskit_root_new (BOBGUI_ROOT (accessible));
  else
    {
      BobguiAccessible *parent = bobgui_accessible_get_accessible_parent (accessible);
      BobguiATContext *parent_ctx = bobgui_accessible_get_at_context (parent);
      BobguiAccessKitContext *parent_accesskit_ctx = BOBGUI_ACCESSKIT_CONTEXT (parent_ctx);
      bobgui_at_context_realize (parent_ctx);
      self->root = g_object_ref (parent_accesskit_ctx->root);
      g_object_unref (parent_ctx);
      g_object_unref (parent);
    }

  self->id = bobgui_accesskit_root_add_context (self->root, self);
}

static void
bobgui_accesskit_context_unrealize (BobguiATContext *context)
{
  BobguiAccessKitContext *self = BOBGUI_ACCESSKIT_CONTEXT (context);
  BobguiAccessible *accessible = bobgui_at_context_get_accessible (context);

  BOBGUI_DEBUG (A11Y, "Unrealizing AccessKit context for accessible '%s'",
                   G_OBJECT_TYPE_NAME (accessible));

  bobgui_accesskit_root_remove_context (self->root, self->id);

  g_clear_object (&self->root);
  self->single_text_layout.id = 0;
  g_clear_pointer (&self->single_text_layout.children, g_array_unref);
  g_clear_pointer (&self->text_view_lines, g_hash_table_destroy);
  g_clear_pointer (&self->text_view_lines_by_id, g_hash_table_destroy);
}

static void
queue_update (BobguiAccessKitContext *self, gboolean force_to_end)
{
  if (!self->root)
    return;

  bobgui_accesskit_root_queue_update (self->root, self->id, force_to_end);
}

static void
bobgui_accesskit_context_state_change (BobguiATContext                *ctx,
                                    BobguiAccessibleStateChange     changed_states,
                                    BobguiAccessiblePropertyChange  changed_properties,
                                    BobguiAccessibleRelationChange  changed_relations,
                                    BobguiAccessibleAttributeSet   *states,
                                    BobguiAccessibleAttributeSet   *properties,
                                    BobguiAccessibleAttributeSet   *relations)
{
  BobguiAccessKitContext *self = BOBGUI_ACCESSKIT_CONTEXT (ctx);

  queue_update (self, FALSE);
}

static void
bobgui_accesskit_context_platform_change (BobguiATContext                *ctx,
                                       BobguiAccessiblePlatformChange  change)
{
  BobguiAccessKitContext *self = BOBGUI_ACCESSKIT_CONTEXT (ctx);
  BobguiAccessible *accessible = bobgui_at_context_get_accessible (ctx);

  queue_update (self, FALSE);

  if (BOBGUI_IS_ROOT (accessible) &&
      change == BOBGUI_ACCESSIBLE_PLATFORM_CHANGE_ACTIVE)
    {
      gboolean active =
        bobgui_accessible_get_platform_state (accessible,
                                           BOBGUI_ACCESSIBLE_PLATFORM_STATE_ACTIVE);
      bobgui_accesskit_root_update_window_focus_state (self->root, active);
    }
}

static void
invalidate_text_view_line_layout (gpointer key,
                                  gpointer value,
                                  gpointer user_data)
{
  BobguiAccessKitTextLayout *layout = value;
  g_clear_pointer (&layout->children, g_array_unref);
}

static BobguiAccessible *
editable_ancestor (BobguiAccessible *accessible)
{
  BobguiAccessible *ancestor = bobgui_accessible_get_accessible_parent (accessible);

  while (ancestor)
    {
      BobguiAccessible *next;

      if (BOBGUI_IS_EDITABLE (ancestor))
        return ancestor;

      next = bobgui_accessible_get_accessible_parent (ancestor);
      g_object_unref (ancestor);
      ancestor = next;
    }

  return NULL;
}

static void
queue_update_on_editable_ancestor (BobguiAccessKitContext *self)
{
  BobguiATContext *ctx = BOBGUI_AT_CONTEXT (self);
  BobguiAccessible *accessible = bobgui_at_context_get_accessible (ctx);
  BobguiAccessible *ancestor = editable_ancestor (accessible);
  BobguiATContext *ancestor_ctx;

  if (!ancestor)
    return;

  ancestor_ctx =bobgui_accessible_get_at_context (ancestor);
  /* The editable ancestor must come after the BobguiText instance in the
     update queue, to ensure the AccessKit representation of the layout
     is rebuilt before the selection is updated on the ancestor. */
  queue_update (BOBGUI_ACCESSKIT_CONTEXT (ancestor_ctx), TRUE /* force_to_end */);
  g_object_unref (ancestor_ctx);
  g_object_unref (ancestor);
}

static void
bobgui_accesskit_context_bounds_change (BobguiATContext *ctx)
{
  BobguiAccessKitContext *self = BOBGUI_ACCESSKIT_CONTEXT (ctx);

  g_clear_pointer (&self->single_text_layout.children, g_array_unref);
  if (self->text_view_lines)
    g_hash_table_foreach (self->text_view_lines,
                          invalidate_text_view_line_layout, NULL);

  queue_update (self, FALSE);
  queue_update_on_editable_ancestor (self);
}

static void
bobgui_accesskit_context_child_change (BobguiATContext             *ctx,
                                    BobguiAccessibleChildChange  change,
                                    BobguiAccessible            *child)
{
  BobguiAccessKitContext *self = BOBGUI_ACCESSKIT_CONTEXT (ctx);

  queue_update (self, FALSE);
}

static void
bobgui_accesskit_context_announce (BobguiATContext                      *context,
                                const char                        *message,
                                BobguiAccessibleAnnouncementPriority  priority)
{
  /* TODO */
}

static void
bobgui_accesskit_context_update_caret_position (BobguiATContext *ctx)
{
  BobguiAccessKitContext *self = BOBGUI_ACCESSKIT_CONTEXT (ctx);

  queue_update (self, FALSE);
}

static void
bobgui_accesskit_context_update_selection_bound (BobguiATContext *ctx)
{
  BobguiAccessKitContext *self = BOBGUI_ACCESSKIT_CONTEXT (ctx);

  queue_update (self, FALSE);
  queue_update_on_editable_ancestor (self);
}

static void
bobgui_accesskit_context_update_text_contents (BobguiATContext *ctx,
                                            BobguiAccessibleTextContentChange change,
                                            unsigned int start_offset,
                                            unsigned int end_offset)
{
  BobguiAccessKitContext *self = BOBGUI_ACCESSKIT_CONTEXT (ctx);
  BobguiAccessible *accessible = bobgui_at_context_get_accessible (ctx);

  g_clear_pointer (&self->single_text_layout.children, g_array_unref);

  if (BOBGUI_IS_TEXT_VIEW (accessible) && self->text_view_lines)
    {
      BobguiTextView *text_view = BOBGUI_TEXT_VIEW (accessible);
      BobguiTextBuffer *buffer = bobgui_text_view_get_buffer (text_view);
      BobguiTextIter current, end;

      bobgui_text_buffer_get_iter_at_offset (buffer, &current, start_offset);
      bobgui_text_buffer_get_iter_at_offset (buffer, &end, end_offset);

      do
        {
          BobguiTextLine *line = _bobgui_text_iter_get_text_line (&current);
          BobguiAccessKitTextLayout *layout =
            g_hash_table_lookup (self->text_view_lines, line);

          if (layout)
            {
              if (change == BOBGUI_ACCESSIBLE_TEXT_CONTENT_CHANGE_REMOVE &&
                  bobgui_text_iter_get_line_offset (&current) == 0 &&
                  (bobgui_text_iter_get_offset (&current) +
                   bobgui_text_iter_get_chars_in_line (&current)) <= end_offset)
                {
                  g_hash_table_remove (self->text_view_lines_by_id,
                                       GUINT_TO_POINTER (layout->id));
                  g_hash_table_remove (self->text_view_lines, line);
                }
              else
                g_clear_pointer (&layout->children, g_array_unref);
            }

          if (bobgui_text_iter_compare (&current, &end) == 0)
            break;
          bobgui_text_iter_forward_line (&current);
        }
      while (bobgui_text_iter_compare (&current, &end) <= 0);
    }

  /* TODO: other text widget types */

  queue_update (self, FALSE);
  queue_update_on_editable_ancestor (self);
}

static void
bobgui_accesskit_context_class_init (BobguiAccessKitContextClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BobguiATContextClass *context_class = BOBGUI_AT_CONTEXT_CLASS (klass);

  gobject_class->finalize = bobgui_accesskit_context_finalize;

  context_class->realize = bobgui_accesskit_context_realize;
  context_class->unrealize = bobgui_accesskit_context_unrealize;
  context_class->state_change = bobgui_accesskit_context_state_change;
  context_class->platform_change = bobgui_accesskit_context_platform_change;
  context_class->bounds_change = bobgui_accesskit_context_bounds_change;
  context_class->child_change = bobgui_accesskit_context_child_change;
  context_class->announce = bobgui_accesskit_context_announce;
  context_class->update_caret_position = bobgui_accesskit_context_update_caret_position;
  context_class->update_selection_bound = bobgui_accesskit_context_update_selection_bound;
  context_class->update_text_contents = bobgui_accesskit_context_update_text_contents;
}

static void
bobgui_accesskit_context_init (BobguiAccessKitContext *self)
{
}

BobguiATContext *
bobgui_accesskit_create_context (BobguiAccessibleRole  accessible_role,
                              BobguiAccessible     *accessible,
                              GdkDisplay        *display)
{
  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE (accessible), NULL);
  g_return_val_if_fail (GDK_IS_DISPLAY (display), NULL);

  return g_object_new (BOBGUI_TYPE_ACCESSKIT_CONTEXT,
                       "accessible-role", accessible_role,
                       "accessible", accessible,
                       "display", display,
                       NULL);
}

static accesskit_role
bobgui_accessible_role_to_accesskit_role (BobguiAccessibleRole role)
{
  switch (role)
    {
    case BOBGUI_ACCESSIBLE_ROLE_ALERT:
      return ACCESSKIT_ROLE_ALERT;

    case BOBGUI_ACCESSIBLE_ROLE_ALERT_DIALOG:
      return ACCESSKIT_ROLE_ALERT_DIALOG;

    case BOBGUI_ACCESSIBLE_ROLE_APPLICATION:
      return ACCESSKIT_ROLE_WINDOW;

    case BOBGUI_ACCESSIBLE_ROLE_ARTICLE:
      return ACCESSKIT_ROLE_ARTICLE;

    case BOBGUI_ACCESSIBLE_ROLE_BANNER:
      return ACCESSKIT_ROLE_BANNER;

    case BOBGUI_ACCESSIBLE_ROLE_BLOCK_QUOTE:
      return ACCESSKIT_ROLE_BLOCKQUOTE;

    case BOBGUI_ACCESSIBLE_ROLE_BUTTON:
      return ACCESSKIT_ROLE_BUTTON;

    case BOBGUI_ACCESSIBLE_ROLE_CAPTION:
      return ACCESSKIT_ROLE_CAPTION;

    case BOBGUI_ACCESSIBLE_ROLE_CELL:
      return ACCESSKIT_ROLE_CELL;

    case BOBGUI_ACCESSIBLE_ROLE_CHECKBOX:
      return ACCESSKIT_ROLE_CHECK_BOX;

    case BOBGUI_ACCESSIBLE_ROLE_COLUMN_HEADER:
      return ACCESSKIT_ROLE_COLUMN_HEADER;

    case BOBGUI_ACCESSIBLE_ROLE_COMBO_BOX:
      return ACCESSKIT_ROLE_COMBO_BOX;

    case BOBGUI_ACCESSIBLE_ROLE_COMMAND:
      return ACCESSKIT_ROLE_GENERIC_CONTAINER;

    case BOBGUI_ACCESSIBLE_ROLE_COMMENT:
      return ACCESSKIT_ROLE_COMMENT;

    case BOBGUI_ACCESSIBLE_ROLE_COMPOSITE:
      return ACCESSKIT_ROLE_GENERIC_CONTAINER;

    case BOBGUI_ACCESSIBLE_ROLE_DIALOG:
      return ACCESSKIT_ROLE_DIALOG;

    case BOBGUI_ACCESSIBLE_ROLE_DOCUMENT:
      return ACCESSKIT_ROLE_DOCUMENT;

    case BOBGUI_ACCESSIBLE_ROLE_FEED:
      return ACCESSKIT_ROLE_FEED;

    case BOBGUI_ACCESSIBLE_ROLE_FORM:
      return ACCESSKIT_ROLE_FORM;

    case BOBGUI_ACCESSIBLE_ROLE_GENERIC:
      return ACCESSKIT_ROLE_GENERIC_CONTAINER;

    case BOBGUI_ACCESSIBLE_ROLE_GRID:
      return ACCESSKIT_ROLE_GRID;

    case BOBGUI_ACCESSIBLE_ROLE_GRID_CELL:
      return ACCESSKIT_ROLE_CELL;

    case BOBGUI_ACCESSIBLE_ROLE_GROUP:
      return ACCESSKIT_ROLE_GROUP;

    case BOBGUI_ACCESSIBLE_ROLE_HEADING:
      return ACCESSKIT_ROLE_HEADING;

    case BOBGUI_ACCESSIBLE_ROLE_IMG:
      return ACCESSKIT_ROLE_IMAGE;

    case BOBGUI_ACCESSIBLE_ROLE_INPUT:
      return ACCESSKIT_ROLE_TEXT_INPUT;

    case BOBGUI_ACCESSIBLE_ROLE_LABEL:
      return ACCESSKIT_ROLE_LABEL;

    case BOBGUI_ACCESSIBLE_ROLE_LANDMARK:
      return ACCESSKIT_ROLE_GENERIC_CONTAINER;

    case BOBGUI_ACCESSIBLE_ROLE_LEGEND:
      return ACCESSKIT_ROLE_LEGEND;

    case BOBGUI_ACCESSIBLE_ROLE_LINK:
      return ACCESSKIT_ROLE_LINK;

    case BOBGUI_ACCESSIBLE_ROLE_LIST:
      return ACCESSKIT_ROLE_LIST;

    case BOBGUI_ACCESSIBLE_ROLE_LIST_BOX:
      return ACCESSKIT_ROLE_LIST_BOX;

    case BOBGUI_ACCESSIBLE_ROLE_LIST_ITEM:
      return ACCESSKIT_ROLE_LIST_ITEM;

    case BOBGUI_ACCESSIBLE_ROLE_LOG:
      return ACCESSKIT_ROLE_LOG;

    case BOBGUI_ACCESSIBLE_ROLE_MAIN:
      return ACCESSKIT_ROLE_MAIN;

    case BOBGUI_ACCESSIBLE_ROLE_MARQUEE:
      return ACCESSKIT_ROLE_MARQUEE;

    case BOBGUI_ACCESSIBLE_ROLE_MATH:
      return ACCESSKIT_ROLE_MATH;

    case BOBGUI_ACCESSIBLE_ROLE_METER:
      return ACCESSKIT_ROLE_METER;

    case BOBGUI_ACCESSIBLE_ROLE_MENU:
      return ACCESSKIT_ROLE_MENU;

    case BOBGUI_ACCESSIBLE_ROLE_MENU_BAR:
      return ACCESSKIT_ROLE_MENU_BAR;

    case BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM:
      return ACCESSKIT_ROLE_MENU_ITEM;

    case BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_CHECKBOX:
      return ACCESSKIT_ROLE_MENU_ITEM_CHECK_BOX;

    case BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_RADIO:
      return ACCESSKIT_ROLE_MENU_ITEM_RADIO;

    case BOBGUI_ACCESSIBLE_ROLE_NAVIGATION:
      return ACCESSKIT_ROLE_NAVIGATION;

    case BOBGUI_ACCESSIBLE_ROLE_NONE:
      return ACCESSKIT_ROLE_GENERIC_CONTAINER;

    case BOBGUI_ACCESSIBLE_ROLE_NOTE:
      return ACCESSKIT_ROLE_NOTE;

    case BOBGUI_ACCESSIBLE_ROLE_OPTION:
      return ACCESSKIT_ROLE_LIST_BOX_OPTION;

    case BOBGUI_ACCESSIBLE_ROLE_PARAGRAPH:
      return ACCESSKIT_ROLE_PARAGRAPH;

    case BOBGUI_ACCESSIBLE_ROLE_PRESENTATION:
      return ACCESSKIT_ROLE_GENERIC_CONTAINER;

    case BOBGUI_ACCESSIBLE_ROLE_PROGRESS_BAR:
      return ACCESSKIT_ROLE_PROGRESS_INDICATOR;

    case BOBGUI_ACCESSIBLE_ROLE_RADIO:
      return ACCESSKIT_ROLE_RADIO_BUTTON;

    case BOBGUI_ACCESSIBLE_ROLE_RADIO_GROUP:
      return ACCESSKIT_ROLE_RADIO_GROUP;

    case BOBGUI_ACCESSIBLE_ROLE_RANGE:
      return ACCESSKIT_ROLE_GENERIC_CONTAINER;

    case BOBGUI_ACCESSIBLE_ROLE_REGION:
      return ACCESSKIT_ROLE_REGION;

    case BOBGUI_ACCESSIBLE_ROLE_ROW:
      return ACCESSKIT_ROLE_ROW;

    case BOBGUI_ACCESSIBLE_ROLE_ROW_GROUP:
      return ACCESSKIT_ROLE_ROW_GROUP;

    case BOBGUI_ACCESSIBLE_ROLE_ROW_HEADER:
      return ACCESSKIT_ROLE_ROW_HEADER;

    case BOBGUI_ACCESSIBLE_ROLE_SCROLLBAR:
      return ACCESSKIT_ROLE_SCROLL_BAR;

    case BOBGUI_ACCESSIBLE_ROLE_SEARCH:
      return ACCESSKIT_ROLE_SEARCH;

    case BOBGUI_ACCESSIBLE_ROLE_SEARCH_BOX:
      return ACCESSKIT_ROLE_SEARCH_INPUT;

    case BOBGUI_ACCESSIBLE_ROLE_SECTION:
      return ACCESSKIT_ROLE_SECTION;

    case BOBGUI_ACCESSIBLE_ROLE_SECTION_HEAD:
      return ACCESSKIT_ROLE_GENERIC_CONTAINER;

    case BOBGUI_ACCESSIBLE_ROLE_SELECT:
      return ACCESSKIT_ROLE_GENERIC_CONTAINER;

    case BOBGUI_ACCESSIBLE_ROLE_SEPARATOR:
      return ACCESSKIT_ROLE_GENERIC_CONTAINER;

    case BOBGUI_ACCESSIBLE_ROLE_SLIDER:
      return ACCESSKIT_ROLE_SLIDER;

    case BOBGUI_ACCESSIBLE_ROLE_SPIN_BUTTON:
      return ACCESSKIT_ROLE_SPIN_BUTTON;

    case BOBGUI_ACCESSIBLE_ROLE_STATUS:
      return ACCESSKIT_ROLE_STATUS;

    case BOBGUI_ACCESSIBLE_ROLE_STRUCTURE:
      return ACCESSKIT_ROLE_GENERIC_CONTAINER;

    case BOBGUI_ACCESSIBLE_ROLE_SWITCH:
      return ACCESSKIT_ROLE_SWITCH;

    case BOBGUI_ACCESSIBLE_ROLE_TAB:
      return ACCESSKIT_ROLE_TAB;

    case BOBGUI_ACCESSIBLE_ROLE_TABLE:
      return ACCESSKIT_ROLE_TABLE;

    case BOBGUI_ACCESSIBLE_ROLE_TAB_LIST:
      return ACCESSKIT_ROLE_TAB_LIST;

    case BOBGUI_ACCESSIBLE_ROLE_TAB_PANEL:
      return ACCESSKIT_ROLE_TAB_PANEL;

    case BOBGUI_ACCESSIBLE_ROLE_TEXT_BOX:
      return ACCESSKIT_ROLE_TEXT_INPUT;

    case BOBGUI_ACCESSIBLE_ROLE_TIME:
      return ACCESSKIT_ROLE_TIME_INPUT;

    case BOBGUI_ACCESSIBLE_ROLE_TIMER:
      return ACCESSKIT_ROLE_TIMER;

    case BOBGUI_ACCESSIBLE_ROLE_TOOLBAR:
      return ACCESSKIT_ROLE_TOOLBAR;

    case BOBGUI_ACCESSIBLE_ROLE_TOOLTIP:
      return ACCESSKIT_ROLE_TOOLTIP;

    case BOBGUI_ACCESSIBLE_ROLE_TREE:
      return ACCESSKIT_ROLE_TREE;

    case BOBGUI_ACCESSIBLE_ROLE_TREE_GRID:
      return ACCESSKIT_ROLE_TREE_GRID;

    case BOBGUI_ACCESSIBLE_ROLE_TREE_ITEM:
      return ACCESSKIT_ROLE_TREE_ITEM;

    case BOBGUI_ACCESSIBLE_ROLE_WIDGET:
      return ACCESSKIT_ROLE_UNKNOWN;

    case BOBGUI_ACCESSIBLE_ROLE_WINDOW:
      return ACCESSKIT_ROLE_WINDOW;

    case BOBGUI_ACCESSIBLE_ROLE_TOGGLE_BUTTON:
      return ACCESSKIT_ROLE_BUTTON;

    case BOBGUI_ACCESSIBLE_ROLE_TERMINAL:
      return ACCESSKIT_ROLE_TERMINAL;

    default:
      break;
    }

  return ACCESSKIT_ROLE_UNKNOWN;
}

static accesskit_role
accesskit_role_for_context (BobguiATContext *context)
{
  BobguiAccessible *accessible = bobgui_at_context_get_accessible (context);
  BobguiAccessibleRole role = bobgui_at_context_get_accessible_role (context);
  accesskit_role result;

  if (BOBGUI_IS_MENU_BUTTON (accessible) ||
      BOBGUI_IS_COLOR_DIALOG_BUTTON (accessible) ||
      BOBGUI_IS_FONT_DIALOG_BUTTON (accessible) ||
      BOBGUI_IS_SCALE_BUTTON (accessible)
#ifdef G_OS_UNIX
      || BOBGUI_IS_PRINTER_OPTION_WIDGET (accessible)
#endif
    )
    return ACCESSKIT_ROLE_GENERIC_CONTAINER;

  /* ARIA does not have a "password entry" role, so we need to fudge it here */
  if (role == BOBGUI_ACCESSIBLE_ROLE_TEXT_BOX &&
      bobgui_accessible_is_password_text (accessible))
    return ACCESSKIT_ROLE_PASSWORD_INPUT;

  /* ARIA does not have a "scroll area" role */
  if (BOBGUI_IS_SCROLLED_WINDOW (accessible))
    return ACCESSKIT_ROLE_SCROLL_VIEW;

  result = bobgui_accessible_role_to_accesskit_role (role);

  if (result == ACCESSKIT_ROLE_TEXT_INPUT &&
      bobgui_at_context_has_accessible_property (context, BOBGUI_ACCESSIBLE_PROPERTY_MULTI_LINE))
    {
      BobguiAccessibleValue *value;

      value = bobgui_at_context_get_accessible_property (context,
                                                      BOBGUI_ACCESSIBLE_PROPERTY_MULTI_LINE);
      if (bobgui_boolean_accessible_value_get (value))
        result = ACCESSKIT_ROLE_MULTILINE_TEXT_INPUT;
    }

  return result;
}

guint32
bobgui_accesskit_context_get_id (BobguiAccessKitContext *self)
{
  g_assert (self->root);
  return self->id;
}

static void
set_bounds (BobguiAccessible *accessible, accesskit_node *node)
{
  int x, y, width, height;

  if (bobgui_accessible_get_bounds (accessible, &x, &y, &width, &height))
    {
      accesskit_vec2 p;
      accesskit_affine transform;
      accesskit_rect bounds = {0, 0, width, height};

      if (BOBGUI_IS_ROOT (accessible) && BOBGUI_IS_NATIVE (accessible))
        {
          GdkSurface *surface = bobgui_native_get_surface (BOBGUI_NATIVE (accessible));
          accesskit_affine scale = accesskit_affine_scale (gdk_surface_get_scale (surface));
          accesskit_affine translate;

          bobgui_native_get_surface_transform (BOBGUI_NATIVE (accessible), &p.x, &p.y);
          translate = accesskit_affine_translate (p);

          transform = accesskit_affine_mul (scale, translate);
        }
      else
        {
          p.x = x;
          p.y = y;
          transform = accesskit_affine_translate (p);
        }

      accesskit_node_set_transform (node, transform);
      accesskit_node_set_bounds (node, bounds);
    }
}

typedef void (*AccessKitFlagSetter) (accesskit_node *);

static gboolean
set_flag_from_state (BobguiATContext        *ctx,
                     BobguiAccessibleState   state,
                     AccessKitFlagSetter  setter,
                     accesskit_node      *node)
{
  if (bobgui_at_context_has_accessible_state (ctx, state))
    {
      BobguiAccessibleValue *value;

      value = bobgui_at_context_get_accessible_state (ctx, state);
      if (bobgui_boolean_accessible_value_get (value))
        {
          setter (node);
          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
set_flag_from_property (BobguiATContext          *ctx,
                        BobguiAccessibleProperty  property,
                        AccessKitFlagSetter    setter,
                        accesskit_node        *node)
{
  if (bobgui_at_context_has_accessible_property (ctx, property))
    {
      BobguiAccessibleValue *value;

      value = bobgui_at_context_get_accessible_property (ctx, property);
      if (bobgui_boolean_accessible_value_get (value))
        {
          setter (node);
          return TRUE;
        }
    }

  return FALSE;
}

typedef void (*AccessKitOptionalFlagSetter) (accesskit_node *, bool);

static gboolean
set_optional_flag_from_state (BobguiATContext                *ctx,
                              BobguiAccessibleState           state,
                              AccessKitOptionalFlagSetter  setter,
                              accesskit_node              *node)
{
  if (bobgui_at_context_has_accessible_state (ctx, state))
    {
      BobguiAccessibleValue *value;

      value = bobgui_at_context_get_accessible_state (ctx, state);
      setter (node, bobgui_boolean_accessible_value_get (value));
      return TRUE;
    }

  return FALSE;
}

static gboolean
set_toggled (BobguiATContext       *ctx,
             BobguiAccessibleState  state,
             accesskit_node     *node)
{
  if (bobgui_at_context_has_accessible_state (ctx, state))
    {
      BobguiAccessibleValue *value;
      accesskit_toggled toggled;

      value = bobgui_at_context_get_accessible_state (ctx, state);

      switch (bobgui_tristate_accessible_value_get (value))
        {
        case BOBGUI_ACCESSIBLE_TRISTATE_FALSE:
          toggled = ACCESSKIT_TOGGLED_FALSE;
          break;

        case BOBGUI_ACCESSIBLE_TRISTATE_TRUE:
          toggled = ACCESSKIT_TOGGLED_TRUE;
          break;

        case BOBGUI_ACCESSIBLE_TRISTATE_MIXED:
        default:
          toggled = ACCESSKIT_TOGGLED_MIXED;
          break;
        }

      accesskit_node_set_toggled (node, toggled);
      return TRUE;
    }

  return FALSE;
}

typedef void (*AccessKitStringSetter) (accesskit_node *, const char *);

static gboolean
set_string_property (BobguiATContext          *ctx,
                     BobguiAccessibleProperty  property,
                     AccessKitStringSetter  setter,
                     accesskit_node        *node)
{
  if (bobgui_at_context_has_accessible_property (ctx, property))
    {
      BobguiAccessibleValue *value;
      const char *str;

      value = bobgui_at_context_get_accessible_property (ctx, property);
      str = bobgui_string_accessible_value_get (value);
      if (str)
        {
          setter (node, str);
          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
set_string_from_relation (BobguiATContext          *ctx,
                          BobguiAccessibleRelation  relation,
                          AccessKitStringSetter  setter,
                          accesskit_node        *node)
{
  if (bobgui_at_context_has_accessible_relation (ctx, relation))
    {
      BobguiAccessibleValue *value;
      const char *str;

      value = bobgui_at_context_get_accessible_relation (ctx, relation);
      str = bobgui_string_accessible_value_get (value);
      if (str)
        {
          setter (node, str);
          return TRUE;
        }
    }

  return FALSE;
}

typedef void (*AccessKitSizeSetter) (accesskit_node *, size_t);

static gboolean
set_size_from_property (BobguiATContext          *ctx,
                        BobguiAccessibleProperty  property,
                        AccessKitSizeSetter    setter,
                        accesskit_node        *node)
{
  if (bobgui_at_context_has_accessible_property (ctx, property))
    {
      BobguiAccessibleValue *value;

      value = bobgui_at_context_get_accessible_property (ctx, property);
      setter (node, bobgui_int_accessible_value_get (value));
      return TRUE;
    }

  return FALSE;
}

static gboolean
set_size_from_relation (BobguiATContext          *ctx,
                        BobguiAccessibleRelation  relation,
                        AccessKitSizeSetter    setter,
                        accesskit_node        *node)
{
  if (bobgui_at_context_has_accessible_relation (ctx, relation))
    {
      BobguiAccessibleValue *value;

      value = bobgui_at_context_get_accessible_relation (ctx, relation);
      setter (node, bobgui_int_accessible_value_get (value));
      return TRUE;
    }

  return FALSE;
}

typedef void (*AccessKitDoubleSetter) (accesskit_node *, double);

static gboolean
set_double_property (BobguiATContext          *ctx,
                     BobguiAccessibleProperty  property,
                     AccessKitDoubleSetter  setter,
                     accesskit_node        *node)
{
  if (bobgui_at_context_has_accessible_property (ctx, property))
    {
      BobguiAccessibleValue *value;

      value = bobgui_at_context_get_accessible_property (ctx, property);
      setter (node, bobgui_number_accessible_value_get (value));
      return TRUE;
    }

  return FALSE;
}

typedef void (*AccessKitNodeIdSetter) (accesskit_node *, accesskit_node_id);

static gboolean
set_single_relation (BobguiATContext          *ctx,
                    BobguiAccessibleRelation  relation,
                    AccessKitNodeIdSetter  setter,
                    accesskit_node        *node)
{
  if (bobgui_at_context_has_accessible_relation (ctx, relation))
    {
      BobguiAccessibleValue *value;
      BobguiAccessible *target;

      value = bobgui_at_context_get_accessible_relation (ctx, relation);
      target = bobgui_reference_accessible_value_get (value);

      if (target)
        {
          BobguiATContext *target_ctx = bobgui_accessible_get_at_context (target);

          bobgui_at_context_realize (target_ctx);
          setter (node, BOBGUI_ACCESSKIT_CONTEXT (target_ctx)->id);
          g_object_unref (target_ctx);

          return TRUE;
        }
    }

  return FALSE;
}

typedef void (*AccessKitNodeIdPusher) (accesskit_node *, accesskit_node_id);

static gboolean
set_multi_relation (BobguiATContext          *ctx,
                    BobguiAccessibleRelation  relation,
                    AccessKitNodeIdPusher  pusher,
                    accesskit_node        *node)
{
  if (bobgui_at_context_has_accessible_relation (ctx, relation))
    {
      BobguiAccessibleValue *value;
      GList *l;
      gboolean has_target;

      value = bobgui_at_context_get_accessible_relation (ctx, relation);
      l = bobgui_reference_list_accessible_value_get (value);
      has_target = (l != NULL);

      for (; l; l = l->next)
        {
          BobguiATContext *target_ctx =
            bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (l->data));

          bobgui_at_context_realize (target_ctx);
          pusher (node, BOBGUI_ACCESSKIT_CONTEXT (target_ctx)->id);
          g_object_unref (target_ctx);
        }

      return has_target;
    }

  return FALSE;
}

static void
set_bounds_from_pango (accesskit_node *node,
                       PangoRectangle *pango_rect,
                       double          offset_x,
                       double          offset_y)
{
  accesskit_rect rect;

  rect.x0 = offset_x + (double)(pango_rect->x) / PANGO_SCALE;
  rect.y0 = offset_y + (double)(pango_rect->y) / PANGO_SCALE;
  rect.x1 = offset_x + (double)(pango_rect->x + pango_rect->width) / PANGO_SCALE;
  rect.y1 = offset_y + (double)(pango_rect->y + pango_rect->height) / PANGO_SCALE;
  accesskit_node_set_bounds (node, rect);
}

static accesskit_node_id
run_node_id (BobguiAccessKitTextLayout *layout,
             gint                    start_index)
{
  return ((accesskit_node_id)layout->id << 32) | start_index;
}

static void
add_run_node (BobguiAccessKitTextLayout *layout,
              accesskit_tree_update  *update,
              gint                    start_index,
              accesskit_node         *node)
{
  accesskit_node_id id = run_node_id (layout, start_index);
  accesskit_tree_update_push_node (update, id, node);
  g_array_append_val (layout->children, id);
}

typedef struct _BobguiAccessKitRunInfo
{
  PangoLayoutRun *run;
  PangoRectangle extents;
} BobguiAccessKitRunInfo;

static int
compare_run_info (gconstpointer a, gconstpointer b)
{
  const BobguiAccessKitRunInfo *run_info_a = a;
  const BobguiAccessKitRunInfo *run_info_b = b;
  int a_offset = run_info_a->run->item->offset;
  int b_offset = run_info_b->run->item->offset;

  if (a_offset < b_offset)
    return -1;
  if (a_offset > b_offset)
    return 1;
  return 0;
}

static void
add_text_layout_inner (BobguiAccessKitTextLayout *layout,
                       accesskit_tree_update  *update,
                       PangoLayout            *pango_layout,
                       const char             *end_delimiter,
                       double                  offset_x,
                       double                  offset_y)
{
  const char *text = pango_layout_get_text (pango_layout);
  const PangoLogAttr *log_attrs =
    pango_layout_get_log_attrs_readonly (pango_layout, NULL);
  PangoLayoutIter *iter = pango_layout_get_iter (pango_layout);
  GArray *line_runs = NULL;
  guint usv_offset = 0, byte_offset = 0;

  while (true)
    {
      if (pango_layout_iter_get_run_readonly (iter))
        {
          PangoLayoutRun *run = pango_layout_iter_get_run_readonly (iter);
          BobguiAccessKitRunInfo run_info;

          run_info.run = run;
          pango_layout_iter_get_run_extents (iter, NULL, &run_info.extents);

          if (!line_runs)
            line_runs = g_array_new (FALSE, FALSE, sizeof (BobguiAccessKitRunInfo));
          g_array_append_val (line_runs, run_info);

          /* We can always assume there's at least one more null run. */
          pango_layout_iter_next_run (iter);
        }
      else
        {
          PangoLayoutLine *line = pango_layout_iter_get_line_readonly (iter);
          PangoRectangle extents;
          PangoLayoutLine *next_line;
          guint line_end_byte_offset;

          pango_layout_iter_get_run_extents (iter, NULL, &extents);

          if (pango_layout_iter_next_line (iter))
            {
              next_line = pango_layout_iter_get_line_readonly (iter);
              line_end_byte_offset = next_line->start_index;
            }
          else
            {
              next_line = NULL;
              line_end_byte_offset = line->start_index + line->length;
            }

          if (line_runs)
            {
              guint prev_run_usv_offset = 0;
              guint i;

              g_array_sort (line_runs, compare_run_info);

              for (i = 0; i < line_runs->len; i++)
                {
                  BobguiAccessKitRunInfo *run_info =
                    &(g_array_index (line_runs, BobguiAccessKitRunInfo, i));
                  PangoLayoutRun *run = run_info->run;
                  PangoItem *item = run->item;
                  accesskit_node *node =
                    accesskit_node_new (ACCESSKIT_ROLE_TEXT_RUN);
                  guint node_text_byte_count;
                  gchar *node_text;
                  accesskit_text_direction dir;
                  int *log_widths = g_new0 (int, item->num_chars);
                  GArray *char_lengths =
                    g_array_new (FALSE, FALSE, sizeof (uint8_t));
                  GArray *word_lengths =
                    g_array_new (FALSE, FALSE, sizeof (uint8_t));
                  GArray *char_positions =
                    g_array_new (FALSE, FALSE, sizeof (float));
                  GArray *char_widths =
                    g_array_new (FALSE, FALSE, sizeof (float));
                  guint run_start_usv_offset = usv_offset;
                  guint last_word_start_char_offset = 0;
                  guint char_count = 0;
                  float char_pos = 0.0f;

                  g_assert (byte_offset == item->offset);

                  if (i > 0)
                    {
                      accesskit_node_id id =
                        run_node_id (layout, prev_run_usv_offset);
                      accesskit_node_set_previous_on_line (node, id);
                    }

                  set_bounds_from_pango (node, &run_info->extents, offset_x, offset_y);

                  if (i == (line_runs->len - 1))
                    node_text_byte_count =
                      line_end_byte_offset - byte_offset;
                  else
                    node_text_byte_count = item->length;
                  node_text = g_strndup (text + item->offset,
                                         node_text_byte_count);

                  if (i == (line_runs->len - 1) && !next_line && end_delimiter)
                    {
                      gchar *new_text = g_strconcat (node_text, end_delimiter,
                                                     NULL);
                      node_text_byte_count += strlen (end_delimiter);
                      g_free (node_text);
                      node_text = new_text;
                    }

                  accesskit_node_set_value (node, node_text);

                  /* The following logic for determining the run's direction
                     is copied from update_run in pango-layout.c. */
                  if ((item->analysis.level % 2) == 0)
                    dir = ACCESSKIT_TEXT_DIRECTION_LEFT_TO_RIGHT;
                  else
                    dir = ACCESSKIT_TEXT_DIRECTION_RIGHT_TO_LEFT;
                  accesskit_node_set_text_direction (node, dir);

                  /* TODO: attributes, once the AccessKit backends support them */

                  pango_glyph_item_get_logical_widths (run, text, log_widths);

                  while (byte_offset < (item->offset + node_text_byte_count))
                    {
                      guint char_start_byte_offset = byte_offset;
                      uint8_t char_len;

                      if (byte_offset >= (item->offset + item->length))
                        {
                          float width = 0.0f;

                          byte_offset =
                            item->offset + node_text_byte_count;
                          usv_offset +=
                            g_utf8_strlen (node_text + item->length,
                                           node_text_byte_count - item->length);
                          g_array_append_val (char_positions, char_pos);
                          g_array_append_val (char_widths, width);
                        }
                      else
                        {
                          float width = 0.0f;

                          if (log_attrs[usv_offset].is_word_start &&
                              (char_count > last_word_start_char_offset))
                            {
                              uint8_t word_len =
                                char_count - last_word_start_char_offset;

                              g_array_append_val (word_lengths, word_len);
                              last_word_start_char_offset = char_count;
                            }

                          do
                            {
                              width +=
                                (float) (log_widths[usv_offset - run_start_usv_offset]) / PANGO_SCALE;
                              byte_offset =
                                g_utf8_next_char (text + byte_offset) - text;
                              usv_offset++;
                            }
                          while (byte_offset < (item->offset + item->length) &&
                                 !log_attrs[usv_offset].is_cursor_position);

                          g_array_append_val (char_positions, char_pos);
                          g_array_append_val (char_widths, width);
                          char_pos += width;
                        }

                      char_len = byte_offset - char_start_byte_offset;
                      g_array_append_val (char_lengths, char_len);
                      char_count++;
                    }

                  if (char_count > last_word_start_char_offset)
                    {
                      uint8_t word_len =
                        char_count - last_word_start_char_offset;

                      g_array_append_val (word_lengths, word_len);
                    }

                  accesskit_node_set_character_lengths (node,
                                                        char_lengths->len,
                                                        (uint8_t *) char_lengths->data);
                  g_array_unref (char_lengths);
                  accesskit_node_set_word_lengths (node,
                                                   word_lengths->len,
                                                   (uint8_t *) word_lengths->data);
                  g_array_unref (word_lengths);
                  accesskit_node_set_character_positions (node,
                                                          char_positions->len,
                                                          (float *) char_positions->data);
                  g_array_unref (char_positions);
                  accesskit_node_set_character_widths (node,
                                                       char_widths->len,
                                                       (float *) char_widths->data);
                  g_array_unref (char_widths);

                  if (i < (line_runs->len - 1))
                    {
                      accesskit_node_id id = run_node_id (layout, usv_offset);
                      accesskit_node_set_next_on_line (node, id);
                    }

                  add_run_node (layout, update, run_start_usv_offset, node);
                  prev_run_usv_offset = run_start_usv_offset;
                  g_free (node_text);
                  g_free (log_widths);
                }

              g_clear_pointer (&line_runs, g_array_unref);
            }
          else
            {
              accesskit_node *node =
                accesskit_node_new (ACCESSKIT_ROLE_TEXT_RUN);
              uint8_t char_len = line_end_byte_offset - line->start_index;
              gchar *line_text = g_strndup (text + line->start_index, char_len);
              accesskit_text_direction dir;
              uint8_t char_count;
              float coord = 0.0f;

              g_assert (byte_offset == line->start_index);

              set_bounds_from_pango (node, &extents, offset_x, offset_y);

              if (!next_line && end_delimiter)
                {
                  gchar *new_text = g_strconcat (line_text, end_delimiter, NULL);
                  char_len += strlen (end_delimiter);
                  g_free (line_text);
                  line_text = new_text;
                }
              char_count = char_len ? 1 : 0;
              accesskit_node_set_value (node, line_text);

              switch (pango_layout_line_get_resolved_direction (line))
                {
                case PANGO_DIRECTION_RTL:
                case PANGO_DIRECTION_TTB_RTL:
                case PANGO_DIRECTION_WEAK_RTL:
                  dir = ACCESSKIT_TEXT_DIRECTION_RIGHT_TO_LEFT;
                  break;

                case PANGO_DIRECTION_LTR:
                case PANGO_DIRECTION_TTB_LTR:
                case PANGO_DIRECTION_WEAK_LTR:
                case PANGO_DIRECTION_NEUTRAL:
                default:
                  dir = ACCESSKIT_TEXT_DIRECTION_LEFT_TO_RIGHT;
                  break;
                }
              accesskit_node_set_text_direction (node, dir);

              accesskit_node_set_character_lengths (node, char_count, &char_len);
              accesskit_node_set_word_lengths (node, 1, &char_count);
              accesskit_node_set_character_positions (node, char_count, &coord);
              accesskit_node_set_character_widths (node, char_count, &coord);

              add_run_node (layout, update, usv_offset, node);
              byte_offset += char_len;
              usv_offset += g_utf8_strlen (line_text, char_len);
              g_free (line_text);
            }

          if (!next_line)
            break;
        }
    }

  /* Iteration should always end with a null run, and processing that null run
     should dispose of line_runs (see above). */
  g_assert (!line_runs);

  pango_layout_iter_free (iter);
}

static void
add_text_layout (BobguiAccessKitTextLayout *layout,
                 accesskit_tree_update  *update,
                 accesskit_node         *parent_node,
                 PangoLayout            *pango_layout,
                 const char             *end_delimiter,
                 double                  offset_x,
                 double                  offset_y,
                 double                  inner_offset_x,
                 double                  inner_offset_y)
{
  g_assert (layout->id);

  if (!layout->children || offset_x != layout->offset_x ||
      offset_y != layout->offset_y)
    {
      accesskit_node *container_node =
        accesskit_node_new (ACCESSKIT_ROLE_GENERIC_CONTAINER);
      accesskit_vec2 p = {offset_x, offset_y};

      layout->offset_x = offset_x;
      layout->offset_y = offset_y;

      if (offset_x || offset_y)
        accesskit_node_set_transform (container_node,
                                      accesskit_affine_translate (p));

      if (!layout->children)
        {
          layout->children =
            g_array_new (FALSE, FALSE, sizeof (accesskit_node_id));
          add_text_layout_inner (layout, update, pango_layout, end_delimiter,
                                 inner_offset_x, inner_offset_y);
        }

      accesskit_node_set_children (container_node,
                                   layout->children->len,
                                   (accesskit_node_id *)
                                   layout->children->data);

      accesskit_tree_update_push_node (update, layout->id, container_node);
    }

  accesskit_node_push_child (parent_node, layout->id);
}

static void
add_single_text_layout (BobguiAccessKitContext   *self,
                        accesskit_tree_update *update,
                        accesskit_node        *parent_node,
                        PangoLayout           *pango_layout,
                        double                 offset_x,
                        double                 offset_y)
{
  if (!self->single_text_layout.id)
    self->single_text_layout.id = bobgui_accesskit_root_new_id (self->root);

  add_text_layout (&self->single_text_layout, update, parent_node,
                   pango_layout, NULL, offset_x, offset_y, 0.0, 0.0);
}

/* Adapted from bobguiatspitext.c */
static BobguiText *
bobgui_editable_get_text_widget (BobguiEditable *editable)
{
  guint redirects = 0;

  do {
    if (BOBGUI_IS_TEXT (editable))
      return BOBGUI_TEXT (editable);

    if (++redirects >= 6)
      g_assert_not_reached ();

    editable = bobgui_editable_get_delegate (editable);
  } while (editable != NULL);

  return NULL;
}

static void
usv_offset_to_text_position (BobguiAccessKitTextLayout  *layout,
                             PangoLayout             *pango_layout,
                             guint                    usv_offset,
                             accesskit_text_position *pos)
{
  gint i;
  accesskit_node_id id = 0;
  guint run_start_usv_offset = 0;
  const PangoLogAttr *attrs =
    pango_layout_get_log_attrs_readonly (pango_layout, NULL);

  for (i = layout->children->len - 1; i >= 0; i--)
    {
      id = g_array_index (layout->children, accesskit_node_id, i);
      run_start_usv_offset = id & 0xffffffff;

      if (run_start_usv_offset <= usv_offset)
        break;
    }

  g_assert (id != 0);

  pos->node = id;
  pos->character_index = 0;

  for (i = run_start_usv_offset; i < usv_offset; i++)
    {
      if (attrs[i].is_cursor_position)
        pos->character_index++;
    }
}

static void
text_view_mark_to_text_position (BobguiAccessKitContext     *self,
                                 BobguiTextView             *text_view,
                                 BobguiTextMark             *mark,
                                 accesskit_text_position *pos)
{
  BobguiTextBuffer *buffer = bobgui_text_view_get_buffer (text_view);
  BobguiTextLayout *layout = bobgui_text_view_get_layout (text_view);
  BobguiTextIter iter;
  BobguiTextLine *line;
  BobguiAccessKitTextLayout *line_layout;
  BobguiTextLineDisplay *display;

  bobgui_text_buffer_get_iter_at_mark (buffer, &iter, mark);
  line = _bobgui_text_iter_get_text_line (&iter);

  g_assert (self->text_view_lines);
  line_layout = g_hash_table_lookup (self->text_view_lines, line);
  g_assert (line_layout);
  g_assert (line_layout->children);

  display = bobgui_text_layout_get_line_display (layout, line, FALSE);

  usv_offset_to_text_position (line_layout, display->layout,
                               bobgui_text_iter_get_line_offset (&iter), pos);

  bobgui_text_line_display_unref (display);
}

static void
destroy_text_view_lines_value (gpointer data)
{
  BobguiAccessKitTextLayout *layout = data;

  g_clear_pointer (&layout->children, g_array_unref);
  g_free (layout);
}

void
bobgui_accesskit_context_add_to_update (BobguiAccessKitContext   *self,
                                     accesskit_tree_update *update)
{
  BobguiATContext *ctx = BOBGUI_AT_CONTEXT (self);
  accesskit_role role = accesskit_role_for_context (ctx);
  accesskit_node *node = accesskit_node_new (role);
  BobguiAccessible *accessible = bobgui_at_context_get_accessible (ctx);
  BobguiAccessible *child = bobgui_accessible_get_first_accessible_child (accessible);

  if (bobgui_accessible_get_platform_state (accessible,
                                         BOBGUI_ACCESSIBLE_PLATFORM_STATE_FOCUSABLE))
    accesskit_node_add_action (node, ACCESSKIT_ACTION_FOCUS);

  if (BOBGUI_IS_BUTTON (accessible) ||
      BOBGUI_IS_MODEL_BUTTON (accessible) ||
      BOBGUI_IS_SWITCH (accessible) ||
      BOBGUI_IS_EXPANDER (accessible))
    accesskit_node_add_action (node, ACCESSKIT_ACTION_CLICK);
  /* TODO: other actions */

  set_bounds (accessible, node);

  while (child)
    {
      BobguiATContext *child_ctx = bobgui_accessible_get_at_context (child);
      BobguiAccessKitContext *child_accesskit_ctx = BOBGUI_ACCESSKIT_CONTEXT (child_ctx);
      BobguiAccessible *next = bobgui_accessible_get_next_accessible_sibling (child);

      g_assert (bobgui_at_context_is_realized (child_ctx));
      accesskit_node_push_child (node, child_accesskit_ctx->id);
      g_object_unref (child_ctx);
      g_object_unref (child);
      child = next;
    }

  set_flag_from_state (ctx, BOBGUI_ACCESSIBLE_STATE_BUSY,
                       accesskit_node_set_busy, node);
  set_flag_from_state (ctx, BOBGUI_ACCESSIBLE_STATE_DISABLED,
                       accesskit_node_set_disabled, node);
  set_flag_from_state (ctx, BOBGUI_ACCESSIBLE_STATE_HIDDEN,
                       accesskit_node_set_hidden, node);
  set_flag_from_state (ctx, BOBGUI_ACCESSIBLE_STATE_VISITED,
                       accesskit_node_set_visited, node);

  set_optional_flag_from_state (ctx, BOBGUI_ACCESSIBLE_STATE_EXPANDED,
                                accesskit_node_set_expanded, node);
  set_optional_flag_from_state (ctx, BOBGUI_ACCESSIBLE_STATE_SELECTED,
                                accesskit_node_set_selected, node);

  if (!set_toggled (ctx, BOBGUI_ACCESSIBLE_STATE_CHECKED, node))
    set_toggled (ctx, BOBGUI_ACCESSIBLE_STATE_PRESSED, node);


  if (bobgui_at_context_has_accessible_state (ctx, BOBGUI_ACCESSIBLE_STATE_INVALID))
    {
      BobguiAccessibleValue *value;

      value = bobgui_at_context_get_accessible_state (ctx, BOBGUI_ACCESSIBLE_STATE_INVALID);

      switch (bobgui_invalid_accessible_value_get (value))
        {
        case BOBGUI_ACCESSIBLE_INVALID_TRUE:
          accesskit_node_set_invalid (node, ACCESSKIT_INVALID_TRUE);
          break;

        case BOBGUI_ACCESSIBLE_INVALID_GRAMMAR:
          accesskit_node_set_invalid (node, ACCESSKIT_INVALID_GRAMMAR);
          break;

        case BOBGUI_ACCESSIBLE_INVALID_SPELLING:
          accesskit_node_set_invalid (node, ACCESSKIT_INVALID_SPELLING);
          break;

        case BOBGUI_ACCESSIBLE_INVALID_FALSE:
        default:
          break;
        }
    }

  set_flag_from_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_MODAL,
                          accesskit_node_set_modal, node);
  set_flag_from_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_MULTI_SELECTABLE,
                          accesskit_node_set_multiselectable, node);
  set_flag_from_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_READ_ONLY,
                          accesskit_node_set_read_only, node);
  set_flag_from_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_REQUIRED,
                          accesskit_node_set_required, node);

  set_string_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION,
                       accesskit_node_set_description, node);
  set_string_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_KEY_SHORTCUTS,
                       accesskit_node_set_keyboard_shortcut, node);
  set_string_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_LABEL,
                       accesskit_node_set_label, node);
  set_string_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_PLACEHOLDER,
                       accesskit_node_set_placeholder, node);
  set_string_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_ROLE_DESCRIPTION,
                       accesskit_node_set_role_description, node);
  set_string_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT,
                       accesskit_node_set_value, node);

  set_size_from_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_LEVEL,
                          accesskit_node_set_level, node);

  set_double_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX,
                       accesskit_node_set_max_numeric_value, node);
  set_double_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN,
                       accesskit_node_set_min_numeric_value, node);
  set_double_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW,
                       accesskit_node_set_numeric_value, node);

  if (bobgui_at_context_has_accessible_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_AUTOCOMPLETE))
    {
      BobguiAccessibleValue *value;

      value = bobgui_at_context_get_accessible_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_AUTOCOMPLETE);

      switch (bobgui_autocomplete_accessible_value_get (value))
        {
        case BOBGUI_ACCESSIBLE_AUTOCOMPLETE_INLINE:
          accesskit_node_set_auto_complete (node, ACCESSKIT_AUTO_COMPLETE_INLINE);
          break;

        case BOBGUI_ACCESSIBLE_AUTOCOMPLETE_LIST:
          accesskit_node_set_auto_complete (node, ACCESSKIT_AUTO_COMPLETE_LIST);
          break;

        case BOBGUI_ACCESSIBLE_AUTOCOMPLETE_BOTH:
          accesskit_node_set_auto_complete (node, ACCESSKIT_AUTO_COMPLETE_BOTH);
          break;

        case BOBGUI_ACCESSIBLE_AUTOCOMPLETE_NONE:
        default:
          break;
        }
    }

  if (bobgui_at_context_has_accessible_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP))
    {
      BobguiAccessibleValue *value;

      value = bobgui_at_context_get_accessible_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP);
      if (bobgui_boolean_accessible_value_get (value))
        accesskit_node_set_has_popup(node, ACCESSKIT_HAS_POPUP_MENU);
    }

  if (bobgui_at_context_has_accessible_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION))
    {
      BobguiAccessibleValue *value;

      value = bobgui_at_context_get_accessible_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION);

      switch (bobgui_orientation_accessible_value_get (value))
        {
        case BOBGUI_ORIENTATION_HORIZONTAL:
          accesskit_node_set_orientation (node, ACCESSKIT_ORIENTATION_HORIZONTAL);
          break;

        case BOBGUI_ORIENTATION_VERTICAL:
          accesskit_node_set_orientation (node, ACCESSKIT_ORIENTATION_VERTICAL);
          break;

        default:
          break;
        }
    }

  if (bobgui_at_context_has_accessible_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_SORT))
    {
      BobguiAccessibleValue *value;

      value = bobgui_at_context_get_accessible_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_SORT);

      switch (bobgui_sort_accessible_value_get (value))
        {
        case BOBGUI_ACCESSIBLE_SORT_ASCENDING:
          accesskit_node_set_sort_direction (node, ACCESSKIT_SORT_DIRECTION_ASCENDING);
          break;

        case BOBGUI_ACCESSIBLE_SORT_DESCENDING:
          accesskit_node_set_sort_direction (node, ACCESSKIT_SORT_DIRECTION_DESCENDING);
          break;

        case BOBGUI_ACCESSIBLE_SORT_OTHER:
          accesskit_node_set_sort_direction (node, ACCESSKIT_SORT_DIRECTION_OTHER);
          break;

        case BOBGUI_ACCESSIBLE_SORT_NONE:
        default:
          break;
        }
    }

  set_single_relation (ctx, BOBGUI_ACCESSIBLE_RELATION_ACTIVE_DESCENDANT,
                       accesskit_node_set_active_descendant, node);
  set_single_relation (ctx, BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE,
                       accesskit_node_set_error_message, node);

  set_multi_relation (ctx, BOBGUI_ACCESSIBLE_RELATION_CONTROLS,
                      accesskit_node_push_controlled, node);
  set_multi_relation (ctx, BOBGUI_ACCESSIBLE_RELATION_DESCRIBED_BY,
                      accesskit_node_push_described_by, node);
  set_multi_relation (ctx, BOBGUI_ACCESSIBLE_RELATION_DETAILS,
                      accesskit_node_push_detail, node);
  set_multi_relation (ctx, BOBGUI_ACCESSIBLE_RELATION_FLOW_TO,
                      accesskit_node_push_flow_to, node);
  set_multi_relation (ctx, BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY,
                      accesskit_node_push_labelled_by, node);
  set_multi_relation (ctx, BOBGUI_ACCESSIBLE_RELATION_OWNS,
                      accesskit_node_push_owned, node);

  set_size_from_relation (ctx, BOBGUI_ACCESSIBLE_RELATION_COL_COUNT,
                          accesskit_node_set_column_count, node);
  set_size_from_relation (ctx, BOBGUI_ACCESSIBLE_RELATION_COL_INDEX,
                          accesskit_node_set_column_index, node);
  set_size_from_relation (ctx, BOBGUI_ACCESSIBLE_RELATION_COL_SPAN,
                          accesskit_node_set_column_span, node);
  set_size_from_relation (ctx, BOBGUI_ACCESSIBLE_RELATION_POS_IN_SET,
                          accesskit_node_set_position_in_set, node);
  set_size_from_relation (ctx, BOBGUI_ACCESSIBLE_RELATION_ROW_COUNT,
                          accesskit_node_set_row_count, node);
  set_size_from_relation (ctx, BOBGUI_ACCESSIBLE_RELATION_ROW_INDEX,
                          accesskit_node_set_row_index, node);
  set_size_from_relation (ctx, BOBGUI_ACCESSIBLE_RELATION_ROW_SPAN,
                          accesskit_node_set_row_span, node);
  set_size_from_relation (ctx, BOBGUI_ACCESSIBLE_RELATION_SET_SIZE,
                          accesskit_node_set_size_of_set, node);

  set_string_from_relation (ctx, BOBGUI_ACCESSIBLE_RELATION_COL_INDEX_TEXT,
                            accesskit_node_set_column_index_text, node);
  set_string_from_relation (ctx, BOBGUI_ACCESSIBLE_RELATION_ROW_INDEX_TEXT,
                            accesskit_node_set_row_index_text, node);

  accesskit_node_set_class_name (node, g_type_name (G_TYPE_FROM_INSTANCE (accessible)));

  if (!(bobgui_at_context_has_accessible_property (ctx, BOBGUI_ACCESSIBLE_PROPERTY_LABEL) ||
        bobgui_at_context_has_accessible_relation (ctx, BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY)))
    {
      const char *tooltip_text;

      if (BOBGUI_IS_WIDGET (accessible))
        {
          tooltip_text = bobgui_widget_get_tooltip_text (BOBGUI_WIDGET (accessible));
          if (tooltip_text)
            accesskit_node_set_label (node, tooltip_text);
        }
      else
        tooltip_text = NULL;

      if (!tooltip_text && bobgui_at_context_is_nested_button (ctx))
        {
          BobguiAccessible *parent = bobgui_accessible_get_accessible_parent (accessible);
          BobguiATContext *parent_ctx = bobgui_accessible_get_at_context (parent);

          bobgui_at_context_realize (parent_ctx);
          accesskit_node_push_labelled_by (node, BOBGUI_ACCESSKIT_CONTEXT (parent_ctx)->id);
          g_object_unref (parent_ctx);
          g_object_unref (parent);
        }
    }

  if (BOBGUI_IS_LABEL (accessible))
    {
      BobguiLabel *label = BOBGUI_LABEL (accessible);
      PangoLayout *layout = bobgui_label_get_layout (label);
      float x, y;

      bobgui_label_get_layout_location (label, &x, &y);
      add_single_text_layout (self, update, node, layout, x, y);
    }
  else if (BOBGUI_IS_INSCRIPTION (accessible))
    {
      BobguiInscription *inscription = BOBGUI_INSCRIPTION (accessible);
      PangoLayout *layout = bobgui_inscription_get_layout (inscription);
      float x, y;

      bobgui_inscription_get_layout_location (inscription, &x, &y);
      add_single_text_layout (self, update, node, layout, x, y);
    }
  else if (BOBGUI_IS_TEXT (accessible))
    {
      BobguiText *text = BOBGUI_TEXT (accessible);
      PangoLayout *layout = bobgui_text_get_layout (text);
      int x, y;

      bobgui_text_get_layout_offsets (text, &x, &y);
      add_single_text_layout (self, update, node, layout, x, y);
    }
  else if (BOBGUI_IS_TEXT_VIEW (accessible))
    {
      BobguiTextView *text_view = BOBGUI_TEXT_VIEW (accessible);
      BobguiTextBuffer *buffer = bobgui_text_view_get_buffer (text_view);
      BobguiTextLayout *layout = bobgui_text_view_get_layout (text_view);
      BobguiTextBTree *btree = _bobgui_text_buffer_get_btree (buffer);
      BobguiTextIter current;

      if (!self->text_view_lines)
        {
          self->text_view_lines =
            g_hash_table_new_full (NULL, NULL, NULL, destroy_text_view_lines_value);
          self->text_view_lines_by_id = g_hash_table_new (NULL, NULL);
        }

      bobgui_text_buffer_get_start_iter (buffer, &current);

      while (true)
        {
          BobguiTextLine *line = _bobgui_text_iter_get_text_line (&current);
          BobguiAccessKitTextLayout *line_layout =
            g_hash_table_lookup (self->text_view_lines, line);
          BobguiTextIter line_end = current;
          BobguiTextLineDisplay *display;
          PangoLayout *pango_layout;
          char *end_delimiter;
          int inner_offset_x, inner_offset_y;
          int buffer_offset_y;
          int widget_offset_x, widget_offset_y;

          if (!line_layout)
            {
              line_layout = g_new0 (BobguiAccessKitTextLayout, 1);
              line_layout->id = bobgui_accesskit_root_new_id (self->root);
              g_hash_table_insert (self->text_view_lines, line, line_layout);
              g_hash_table_insert (self->text_view_lines_by_id,
                                   GUINT_TO_POINTER (line_layout->id), line);
            }

          if (!bobgui_text_iter_ends_line (&line_end))
            bobgui_text_iter_forward_to_line_end (&line_end);

          if (line_layout->children)
            {
              display = NULL;
              pango_layout = NULL;
              end_delimiter = NULL;
              inner_offset_x = inner_offset_y = 0;
            }
          else
            {
              display = bobgui_text_layout_create_display (layout, line, FALSE);
              pango_layout = display->layout;

              if (bobgui_text_iter_is_end (&line_end))
                end_delimiter = NULL;
              else
                {
                  BobguiTextIter next_line = line_end;
                  bobgui_text_iter_forward_line (&next_line);
                  end_delimiter =
                    bobgui_text_buffer_get_text (buffer, &line_end, &next_line,
                                              TRUE);
                }

              inner_offset_x = display->x_offset;
              inner_offset_y = display->top_margin;
            }

          buffer_offset_y = _bobgui_text_btree_find_line_top (btree, line, layout);
          bobgui_text_view_buffer_to_window_coords (text_view,
                                                 BOBGUI_TEXT_WINDOW_WIDGET,
                                                 0, buffer_offset_y,
                                                 &widget_offset_x,
                                                 &widget_offset_y);

          add_text_layout (line_layout, update, node, pango_layout,
                           end_delimiter, widget_offset_x, widget_offset_y,
                           inner_offset_x, inner_offset_y);

          g_clear_pointer (&display, bobgui_text_line_display_unref);
          g_clear_pointer (&end_delimiter, g_free);

          if (bobgui_text_iter_is_end (&line_end))
            break;
          bobgui_text_iter_forward_line (&current);
        }
    }

  if (BOBGUI_IS_TEXT_VIEW (accessible))
    {
      BobguiTextView *text_view = BOBGUI_TEXT_VIEW (accessible);
      BobguiTextBuffer *buffer = bobgui_text_view_get_buffer (text_view);
      BobguiTextMark *anchor_mark = bobgui_text_buffer_get_selection_bound (buffer);
      BobguiTextMark *focus_mark = bobgui_text_buffer_get_insert (buffer);
      accesskit_text_selection selection;

      text_view_mark_to_text_position (self, text_view, anchor_mark,
                                       &selection.anchor);
      text_view_mark_to_text_position (self, text_view, focus_mark,
                                       &selection.focus);
      accesskit_node_set_text_selection (node, selection);
      accesskit_node_add_action (node, ACCESSKIT_ACTION_SET_TEXT_SELECTION);
    }
  else if (BOBGUI_IS_LABEL (accessible))
    {
      BobguiLabel *label = BOBGUI_LABEL (accessible);

      if (bobgui_label_get_selectable (label))
        {
          PangoLayout *layout = bobgui_label_get_layout (label);
          int anchor = _bobgui_label_get_selection_bound (label);
          int focus = _bobgui_label_get_cursor_position (label);
          accesskit_text_selection selection;

          usv_offset_to_text_position (&self->single_text_layout, layout,
                                       anchor, &selection.anchor);
          usv_offset_to_text_position (&self->single_text_layout, layout,
                                       focus, &selection.focus);
          accesskit_node_set_text_selection (node, selection);
          accesskit_node_add_action (node, ACCESSKIT_ACTION_SET_TEXT_SELECTION);
        }
    }
  else if (BOBGUI_IS_EDITABLE (accessible) &&
           role != ACCESSKIT_ROLE_GENERIC_CONTAINER)
    {
      BobguiText *text = bobgui_editable_get_text_widget (BOBGUI_EDITABLE (accessible));

      if (text)
        {
          BobguiATContext *text_ctx = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (text));
          BobguiAccessKitContext *text_accesskit_ctx = BOBGUI_ACCESSKIT_CONTEXT (text_ctx);
          PangoLayout *layout = bobgui_text_get_layout (text);
          int start, end;
          accesskit_text_selection selection;

          g_assert (bobgui_at_context_is_realized (text_ctx));
          g_assert (text_accesskit_ctx->single_text_layout.children);

          bobgui_editable_get_selection_bounds (BOBGUI_EDITABLE (text), &start, &end);
          usv_offset_to_text_position (&text_accesskit_ctx->single_text_layout,
                                       layout, start, &selection.anchor);
          usv_offset_to_text_position (&text_accesskit_ctx->single_text_layout,
                                       layout, end, &selection.focus);
          accesskit_node_set_text_selection (node, selection);
          accesskit_node_add_action (node, ACCESSKIT_ACTION_SET_TEXT_SELECTION);

          g_object_unref (text_ctx);
        }
    }

  /* TODO: other text widget types */

  accesskit_tree_update_push_node (update, self->id, node);
}

void
bobgui_accesskit_context_update_tree (BobguiAccessKitContext *self)
{
  if (!bobgui_at_context_is_realized (BOBGUI_AT_CONTEXT (self)))
    return;

  bobgui_accesskit_root_update_tree (self->root);
}

static gint
text_position_to_usv_offset (BobguiAccessKitTextLayout        *layout,
                             PangoLayout                   *pango_layout,
                             const accesskit_text_position *pos)
{
  gint n_attrs;
  const PangoLogAttr *attrs =
    pango_layout_get_log_attrs_readonly (pango_layout, &n_attrs);
  guint offset;
  size_t char_index = 0;

  if ((pos->node >> 32) != layout->id)
    return -1;
  offset = pos->node & 0xffffffff;
  if (offset > n_attrs)
    return -1;

  while (char_index < pos->character_index)
    {
      if (offset == n_attrs)
        return -1;
      ++offset;
      if (offset == n_attrs || attrs[offset].is_cursor_position)
        ++char_index;
    }

  return offset;
}

static gboolean
text_position_to_text_view_iter (BobguiAccessKitContext           *self,
                                 BobguiTextView                   *text_view,
                                 const accesskit_text_position *pos,
                                 BobguiTextIter                   *iter)
{
  BobguiTextBuffer *buffer = bobgui_text_view_get_buffer (text_view);
  BobguiTextBTree *btree = _bobgui_text_buffer_get_btree (buffer);
  BobguiTextLayout *layout = bobgui_text_view_get_layout (text_view);
  guint line_id;
  BobguiTextLine *line;
  BobguiAccessKitTextLayout *line_layout;
  BobguiTextLineDisplay *display;
  gint usv_offset;

  if (pos->node == (pos->node & 0xffffffff))
    return FALSE;
  line_id = pos->node >> 32;

  line = g_hash_table_lookup (self->text_view_lines_by_id,
                              GUINT_TO_POINTER (line_id));
  if (!line)
    return FALSE;
  line_layout = g_hash_table_lookup (self->text_view_lines, line);
  g_assert (line_layout);
  if (!line_layout->children)
    return FALSE;
  display = bobgui_text_layout_get_line_display (layout, line, FALSE);

  usv_offset = text_position_to_usv_offset (line_layout, display->layout, pos);
  if (usv_offset == -1)
    return FALSE;

  _bobgui_text_btree_get_iter_at_line_ptr_char (btree, iter, line, usv_offset);
  return TRUE;
}

void
bobgui_accesskit_context_do_action (BobguiAccessKitContext            *self,
                                 const accesskit_action_request *request)
{
  BobguiATContext *ctx = BOBGUI_AT_CONTEXT (self);
  BobguiAccessible *accessible = bobgui_at_context_get_accessible (ctx);
  BobguiWidget *widget;
  const accesskit_text_selection *selection;

  switch (request->action)
  {
  case ACCESSKIT_ACTION_CLICK:
    if (!BOBGUI_IS_WIDGET (accessible))
      return;
    widget = BOBGUI_WIDGET (accessible);

    if (!bobgui_widget_is_sensitive (widget) || !bobgui_widget_is_visible (widget))
      return;

    bobgui_widget_activate (widget);
    break;

  case ACCESSKIT_ACTION_FOCUS:
    if (!BOBGUI_IS_WIDGET (accessible))
      return;
    widget = BOBGUI_WIDGET (accessible);

    if (!bobgui_widget_is_sensitive (widget) || !bobgui_widget_is_visible (widget))
      return;

    bobgui_widget_grab_focus (widget);
    break;

  case ACCESSKIT_ACTION_SET_TEXT_SELECTION:
    if (!request->data.has_value ||
        request->data.value.tag != ACCESSKIT_ACTION_DATA_SET_TEXT_SELECTION)
      return;
    selection = &request->data.value.set_text_selection;

    if (BOBGUI_IS_TEXT_VIEW (accessible))
      {
        BobguiTextView *text_view = BOBGUI_TEXT_VIEW (accessible);
        BobguiTextBuffer *buffer = bobgui_text_view_get_buffer (text_view);
        BobguiTextIter anchor, focus;

        if (!text_position_to_text_view_iter (self, text_view,
                                              &selection->anchor, &anchor))
          return;
        if (!text_position_to_text_view_iter (self, text_view,
                                              &selection->focus, &focus))
          return;

        bobgui_text_buffer_select_range (buffer, &focus, &anchor);
        bobgui_text_view_scroll_to_iter (text_view, &focus, 0, FALSE, 0, 0);
      }
    else if (BOBGUI_IS_LABEL (accessible))
      {
        BobguiLabel *label = BOBGUI_LABEL (accessible);
        PangoLayout *layout;
        gint anchor, focus;

        if (!bobgui_label_get_selectable (label))
          return;

        layout = bobgui_label_get_layout (label);

        anchor =
          text_position_to_usv_offset (&self->single_text_layout, layout,
                                       &selection->anchor);
        if (anchor == -1)
          return;
        focus =
          text_position_to_usv_offset (&self->single_text_layout, layout,
                                       &selection->focus);
        if (focus == -1)
          return;

        bobgui_label_select_region (label, anchor, focus);
      }
    else if (BOBGUI_IS_EDITABLE (accessible))
      {
        BobguiEditable *editable = BOBGUI_EDITABLE (accessible);
        BobguiText *text = bobgui_editable_get_text_widget (editable);
        BobguiATContext *text_ctx;
        BobguiAccessKitContext *text_accesskit_ctx;
        PangoLayout *layout;
        gint anchor, focus;

        if (!text)
          return;
        text_ctx = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (text));
        text_accesskit_ctx = BOBGUI_ACCESSKIT_CONTEXT (text_ctx);
        if (!bobgui_at_context_is_realized (text_ctx) ||
            !text_accesskit_ctx->single_text_layout.children)
          {
            g_object_unref (text_ctx);
            return;
          }
        layout = bobgui_text_get_layout (text);

        anchor =
          text_position_to_usv_offset (&text_accesskit_ctx->single_text_layout,
                                       layout, &selection->anchor);
        if (anchor == -1)
          return;
        focus =
          text_position_to_usv_offset (&text_accesskit_ctx->single_text_layout,
                                       layout, &selection->focus);
        if (focus == -1)
          return;

        if (anchor == focus)
          bobgui_editable_set_position (editable, focus);
        else if (anchor > focus)
          bobgui_editable_select_region (editable, focus, anchor);
        else
          bobgui_editable_select_region (editable, anchor, focus);

        g_object_unref (text_ctx);
      }
    /* TODO: other text widgets */

    break;

  /* TODO: other actions */

  default:
    break;
  }
}
