/*
 * Copyright © 2023 Red Hat, Inc.
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Matthias Clasen <mclasen@redhat.com>
 */

#include "config.h"

#include "a11yoverlay.h"

#include "bobguiwidget.h"
#include "bobguiroot.h"
#include "bobguinative.h"
#include "bobguiwidgetprivate.h"
#include "bobguiatcontextprivate.h"
#include "bobguiaccessibleprivate.h"
#include "bobguitypebuiltins.h"

struct _BobguiA11yOverlay
{
  BobguiInspectorOverlay parent_instance;

  GdkRGBA recommend_color;
  GdkRGBA error_color;

  GArray *context;
};

struct _BobguiA11yOverlayClass
{
  BobguiInspectorOverlayClass parent_class;
};

G_DEFINE_TYPE (BobguiA11yOverlay, bobgui_a11y_overlay, BOBGUI_TYPE_INSPECTOR_OVERLAY)

typedef enum
{
  FIX_SEVERITY_GOOD,
  FIX_SEVERITY_RECOMMENDATION,
  FIX_SEVERITY_ERROR
} FixSeverity;

typedef enum
{
  ATTRIBUTE_STATE,
  ATTRIBUTE_PROPERTY,
  ATTRIBUTE_RELATION
} AttributeType;

static struct {
  BobguiAccessibleRole role;
  AttributeType type;
  int id;
} required_attributes[] = {
  { BOBGUI_ACCESSIBLE_ROLE_CHECKBOX, ATTRIBUTE_STATE, BOBGUI_ACCESSIBLE_STATE_CHECKED },
  { BOBGUI_ACCESSIBLE_ROLE_COMBO_BOX, ATTRIBUTE_STATE, BOBGUI_ACCESSIBLE_STATE_EXPANDED },
  { BOBGUI_ACCESSIBLE_ROLE_COMBO_BOX, ATTRIBUTE_RELATION, BOBGUI_ACCESSIBLE_RELATION_CONTROLS },
  { BOBGUI_ACCESSIBLE_ROLE_HEADING, ATTRIBUTE_PROPERTY, BOBGUI_ACCESSIBLE_PROPERTY_LEVEL },
  { BOBGUI_ACCESSIBLE_ROLE_SCROLLBAR, ATTRIBUTE_RELATION, BOBGUI_ACCESSIBLE_RELATION_CONTROLS },
  { BOBGUI_ACCESSIBLE_ROLE_SCROLLBAR, ATTRIBUTE_PROPERTY, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW },
  { BOBGUI_ACCESSIBLE_ROLE_SWITCH, ATTRIBUTE_STATE, BOBGUI_ACCESSIBLE_STATE_CHECKED },
};

static struct {
  BobguiAccessibleRole role;
  BobguiAccessibleRole context;
} required_context[] = {
  { BOBGUI_ACCESSIBLE_ROLE_CAPTION, BOBGUI_ACCESSIBLE_ROLE_GRID },
  { BOBGUI_ACCESSIBLE_ROLE_CAPTION, BOBGUI_ACCESSIBLE_ROLE_TABLE },
  { BOBGUI_ACCESSIBLE_ROLE_CAPTION, BOBGUI_ACCESSIBLE_ROLE_TREE_GRID },
  { BOBGUI_ACCESSIBLE_ROLE_CELL, BOBGUI_ACCESSIBLE_ROLE_ROW },
  { BOBGUI_ACCESSIBLE_ROLE_COLUMN_HEADER, BOBGUI_ACCESSIBLE_ROLE_ROW },
  { BOBGUI_ACCESSIBLE_ROLE_GRID_CELL, BOBGUI_ACCESSIBLE_ROLE_ROW },
  { BOBGUI_ACCESSIBLE_ROLE_LIST_ITEM, BOBGUI_ACCESSIBLE_ROLE_LIST },
  { BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM, BOBGUI_ACCESSIBLE_ROLE_GROUP },
  { BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM, BOBGUI_ACCESSIBLE_ROLE_MENU },
  { BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM, BOBGUI_ACCESSIBLE_ROLE_MENU_BAR },
  { BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_CHECKBOX, BOBGUI_ACCESSIBLE_ROLE_GROUP },
  { BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_CHECKBOX, BOBGUI_ACCESSIBLE_ROLE_MENU },
  { BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_CHECKBOX, BOBGUI_ACCESSIBLE_ROLE_MENU_BAR },
  { BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_RADIO, BOBGUI_ACCESSIBLE_ROLE_GROUP },
  { BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_RADIO, BOBGUI_ACCESSIBLE_ROLE_MENU },
  { BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_RADIO, BOBGUI_ACCESSIBLE_ROLE_MENU_BAR },
  { BOBGUI_ACCESSIBLE_ROLE_OPTION, BOBGUI_ACCESSIBLE_ROLE_GROUP },
  { BOBGUI_ACCESSIBLE_ROLE_OPTION, BOBGUI_ACCESSIBLE_ROLE_LIST_BOX },
  { BOBGUI_ACCESSIBLE_ROLE_ROW, BOBGUI_ACCESSIBLE_ROLE_GRID },
  { BOBGUI_ACCESSIBLE_ROLE_ROW, BOBGUI_ACCESSIBLE_ROLE_ROW_GROUP },
  { BOBGUI_ACCESSIBLE_ROLE_ROW, BOBGUI_ACCESSIBLE_ROLE_TABLE },
  { BOBGUI_ACCESSIBLE_ROLE_ROW, BOBGUI_ACCESSIBLE_ROLE_TREE_GRID },
  { BOBGUI_ACCESSIBLE_ROLE_ROW_GROUP, BOBGUI_ACCESSIBLE_ROLE_GRID },
  { BOBGUI_ACCESSIBLE_ROLE_ROW_GROUP, BOBGUI_ACCESSIBLE_ROLE_TABLE },
  { BOBGUI_ACCESSIBLE_ROLE_ROW_GROUP, BOBGUI_ACCESSIBLE_ROLE_TREE_GRID },
  { BOBGUI_ACCESSIBLE_ROLE_ROW_HEADER, BOBGUI_ACCESSIBLE_ROLE_ROW },
  { BOBGUI_ACCESSIBLE_ROLE_TAB, BOBGUI_ACCESSIBLE_ROLE_TAB_LIST },
  { BOBGUI_ACCESSIBLE_ROLE_TREE_ITEM, BOBGUI_ACCESSIBLE_ROLE_GROUP },
  { BOBGUI_ACCESSIBLE_ROLE_TREE_ITEM, BOBGUI_ACCESSIBLE_ROLE_TREE },
};

static FixSeverity
check_accessibility_errors (BobguiATContext       *context,
                            BobguiAccessibleRole   role,
                            GArray             *context_elements,
                            char              **hint)
{
  gboolean label_set;
  const char *role_name;
  GEnumClass *states;
  GEnumClass *properties;
  GEnumClass *relations;
  gboolean has_context;

  *hint = NULL;
  role_name = bobgui_accessible_role_to_name (role, NULL);

  if (!bobgui_at_context_is_realized (context))
    bobgui_at_context_realize (context);

  /* Check for abstract roles */
  if (bobgui_accessible_role_is_abstract (role))
    {
      *hint = g_strdup_printf ("%s is an abstract role", role_name);
      return FIX_SEVERITY_ERROR;
    }

  /* Check for name and description */
  label_set = bobgui_at_context_has_accessible_property (context, BOBGUI_ACCESSIBLE_PROPERTY_LABEL) ||
              bobgui_at_context_has_accessible_relation (context, BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY);

  switch (bobgui_accessible_role_get_naming (role))
    {
    case BOBGUI_ACCESSIBLE_NAME_ALLOWED:
      break;

    case BOBGUI_ACCESSIBLE_NAME_REQUIRED:
      if (!label_set)
        {
          if (bobgui_accessible_role_supports_name_from_author (role))
            {
              char *name = bobgui_at_context_get_name (context);

              if (strcmp (name, "") == 0)
                {
                  g_free (name);
                  *hint = g_strdup_printf ("%s must have text content or label", role_name);

                  return FIX_SEVERITY_ERROR;
                }
            }
          else
            {
              *hint = g_strdup_printf ("%s must have label", role_name);

              return FIX_SEVERITY_ERROR;
            }
        }
      break;

    case BOBGUI_ACCESSIBLE_NAME_PROHIBITED:
      if (label_set)
        {
          *hint = g_strdup_printf ("%s can't have label", role_name);

          return FIX_SEVERITY_ERROR;
        }
      break;

    case BOBGUI_ACCESSIBLE_NAME_RECOMMENDED:
      if (!label_set)
        {
          *hint = g_strdup_printf ("label recommended for %s", role_name);

          return FIX_SEVERITY_RECOMMENDATION;
        }
      break;

    case BOBGUI_ACCESSIBLE_NAME_NOT_RECOMMENDED:
      if (label_set)
        {
          *hint = g_strdup_printf ("label not recommended for %s", role_name);

          return FIX_SEVERITY_RECOMMENDATION;
        }
      break;

    default:
      g_assert_not_reached ();
    }

  /* Check for required attributes */
  states = g_type_class_peek (BOBGUI_TYPE_ACCESSIBLE_STATE);
  properties = g_type_class_peek (BOBGUI_TYPE_ACCESSIBLE_PROPERTY);
  relations = g_type_class_peek (BOBGUI_TYPE_ACCESSIBLE_RELATION);

  for (unsigned int i = 0; i < G_N_ELEMENTS (required_attributes); i++)
    {
      if (role == required_attributes[i].role)
        {
          switch (required_attributes[i].type)
            {
            case ATTRIBUTE_STATE:
              if (!bobgui_at_context_has_accessible_state (context, required_attributes[i].id))
                {
                  *hint = g_strdup_printf ("%s must have state %s", role_name, g_enum_get_value (states, required_attributes[i].id)->value_nick);
                  return FIX_SEVERITY_ERROR;
                }
              break;

            case ATTRIBUTE_PROPERTY:
              if (!bobgui_at_context_has_accessible_property (context, required_attributes[i].id))
                {
                  *hint = g_strdup_printf ("%s must have property %s", role_name, g_enum_get_value (properties, required_attributes[i].id)->value_nick);
                  return FIX_SEVERITY_ERROR;
                }
              break;

            case ATTRIBUTE_RELATION:
              if (!bobgui_at_context_has_accessible_relation (context, required_attributes[i].id))
                {
                  *hint = g_strdup_printf ("%s must have relation %s", role_name, g_enum_get_value (relations, required_attributes[i].id)->value_nick);
                  return FIX_SEVERITY_ERROR;
                }
              break;

            default:
              g_assert_not_reached ();
            }
        }
    }

  /* Check for required context */
  has_context = TRUE;

  for (unsigned int i = 0; i < G_N_ELEMENTS (required_context); i++)
    {
      if (required_context[i].role != role)
        continue;

      has_context = FALSE;

      for (unsigned int j = 0; j < context_elements->len; j++)
        {
          BobguiAccessibleRole elt = g_array_index (context_elements, BobguiAccessibleRole, j);

          if (required_context[i].context == elt)
            {
              has_context = TRUE;
              break;
            }
        }

      if (has_context)
        break;
    }

  if (!has_context)
    {
      GString *s = g_string_new ("");

      for (unsigned int i = 0; i < G_N_ELEMENTS (required_context); i++)
        {
          if (required_context[i].role != role)
            continue;

          if (s->len > 0)
            g_string_append (s, ", ");

          g_string_append (s, bobgui_accessible_role_to_name (required_context[i].context, NULL));
        }

      *hint = g_strdup_printf ("%s requires context: %s", role_name, s->str);

      g_string_free (s, TRUE);

      return FIX_SEVERITY_ERROR;
    }

  if (role == BOBGUI_ACCESSIBLE_ROLE_BUTTON)
    {
      BobguiAccessible *accessible = bobgui_at_context_get_accessible (context);

      if (BOBGUI_IS_WIDGET (accessible))
        {
          int width = bobgui_widget_get_width (BOBGUI_WIDGET (accessible));
          int height = bobgui_widget_get_height (BOBGUI_WIDGET (accessible));

          if (width < 24 || height < 24)
            {
              *hint = g_strdup_printf ("Button is too small: %dx%d", width, height);
              return FIX_SEVERITY_ERROR;
            }
        }
    }

  return FIX_SEVERITY_GOOD;
}

static FixSeverity
check_widget_accessibility_errors (BobguiWidget  *widget,
                                   GArray     *context_elements,
                                   char      **hint)
{
  BobguiAccessibleRole role;
  BobguiATContext *context;
  FixSeverity ret;

  role = bobgui_accessible_get_accessible_role (BOBGUI_ACCESSIBLE (widget));
  context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (widget));
  ret = check_accessibility_errors (context, role, context_elements, hint);
  g_object_unref (context);

  return ret;
}

static void
center_over_within (graphene_rect_t       *rect,
                    const graphene_rect_t *over,
                    const graphene_rect_t *within)
{
  graphene_point_t center;

  graphene_rect_get_center (over, &center);

  rect->origin.x = CLAMP (center.x - 0.5 * rect->size.width, within->origin.x, within->origin.x + within->size.width - rect->size.width);
  rect->origin.y = CLAMP (center.y - 0.5 * rect->size.height, within->origin.y, within->origin.y + within->size.height - rect->size.height);
}

static void
recurse_child_widgets (BobguiA11yOverlay *self,
                       BobguiWidget      *widget,
                       BobguiSnapshot    *snapshot)
{
  BobguiWidget *child;
  char *hint;
  FixSeverity severity;
  BobguiAccessibleRole role;

  if (!bobgui_widget_get_mapped (widget))
    return;

  severity = check_widget_accessibility_errors (widget, self->context, &hint);

  if (severity != FIX_SEVERITY_GOOD)
    {
      int width, height;
      GdkRGBA color;

      width = bobgui_widget_get_width (widget);
      height = bobgui_widget_get_height (widget);

      if (severity == FIX_SEVERITY_ERROR)
        color = self->error_color;
      else
        color = self->recommend_color;

      bobgui_snapshot_save (snapshot);
      bobgui_snapshot_push_debug (snapshot, "Widget a11y debugging");

      bobgui_snapshot_append_color (snapshot, &color,
                                 &GRAPHENE_RECT_INIT (0, 0, width, height));

      if (hint)
        {
          int toplevel_width, toplevel_height;
          PangoLayout *layout;
          PangoRectangle extents;
          GdkRGBA black = { 0, 0, 0, 1 };
          float widths[4] = { 1, 1, 1, 1 };
          GdkRGBA colors[4] = {
            { 0, 0, 0, 1 },
            { 0, 0, 0, 1 },
            { 0, 0, 0, 1 },
            { 0, 0, 0, 1 },
          };
          BobguiNative *native;
          graphene_rect_t over, within, bounds;
          gboolean ret G_GNUC_UNUSED;

          native = bobgui_widget_get_native (widget);
          toplevel_width = bobgui_widget_get_width (BOBGUI_WIDGET (native));
          toplevel_height = bobgui_widget_get_height (BOBGUI_WIDGET (native));

          bobgui_snapshot_save (snapshot);

          layout = bobgui_widget_create_pango_layout (widget, hint);
          pango_layout_set_width (layout, toplevel_width * PANGO_SCALE);

          pango_layout_get_pixel_extents (layout, NULL, &extents);

          extents.x -= 5;
          extents.y -= 5;
          extents.width += 10;
          extents.height += 10;

          ret = bobgui_widget_compute_point (widget, BOBGUI_WIDGET (native), &GRAPHENE_POINT_INIT (0, 0), &over.origin);
          over.size.width = width;
          over.size.height = height;

          graphene_rect_init (&within, 0, 0, toplevel_width, toplevel_height);

          graphene_rect_init (&bounds, 0, 0, extents.width, extents.height);
          center_over_within (&bounds, &over, &within);

          color.alpha = 0.8f;

          bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (bounds.origin.x - over.origin.x,
                                                                  bounds.origin.y - over.origin.y));

          bobgui_snapshot_append_border (snapshot,
                                       &GSK_ROUNDED_RECT_INIT (0, 0,
                                                               extents.width, extents.height),
                                      widths, colors);
          bobgui_snapshot_append_color (snapshot, &color,
                                     &GRAPHENE_RECT_INIT (0, 0,
                                                          extents.width, extents.height));

          bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (5, 5));
          bobgui_snapshot_append_layout (snapshot, layout, &black);
          g_object_unref (layout);

          bobgui_snapshot_restore (snapshot);
        }

      bobgui_snapshot_pop (snapshot);
      bobgui_snapshot_restore (snapshot);
   }

  g_free (hint);

  /* Recurse into child widgets */

  role = bobgui_accessible_get_accessible_role (BOBGUI_ACCESSIBLE (widget));
  g_array_append_val (self->context, role);

  for (child = bobgui_widget_get_first_child (widget);
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      bobgui_snapshot_save (snapshot);
      bobgui_snapshot_transform (snapshot, child->priv->transform);

      recurse_child_widgets (self, child, snapshot);

      bobgui_snapshot_restore (snapshot);
    }

  g_array_remove_index (self->context, self->context->len - 1);
}

static void
bobgui_a11y_overlay_snapshot (BobguiInspectorOverlay *overlay,
                           BobguiSnapshot         *snapshot,
                           GskRenderNode       *node,
                           BobguiWidget           *widget)
{
  BobguiA11yOverlay *self = BOBGUI_A11Y_OVERLAY (overlay);

  g_assert (self->context->len == 0);

  recurse_child_widgets (self, widget, snapshot);

  g_assert (self->context->len == 0);
}

static void
bobgui_a11y_overlay_finalize (GObject *object)
{
  BobguiA11yOverlay *self = BOBGUI_A11Y_OVERLAY (object);

  g_array_free (self->context, TRUE);

  G_OBJECT_CLASS (bobgui_a11y_overlay_parent_class)->finalize (object);
}

static void
bobgui_a11y_overlay_class_init (BobguiA11yOverlayClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiInspectorOverlayClass *overlay_class = BOBGUI_INSPECTOR_OVERLAY_CLASS (klass);

  object_class->finalize = bobgui_a11y_overlay_finalize;

  overlay_class->snapshot = bobgui_a11y_overlay_snapshot;
}

static void
bobgui_a11y_overlay_init (BobguiA11yOverlay *self)
{
  self->recommend_color = (GdkRGBA) { 0.0, 0.5, 1.0, 0.2 };
  self->error_color = (GdkRGBA) { 1.0, 0.0, 0.0, 0.2 };

  self->context = g_array_new (FALSE, FALSE, sizeof (BobguiAccessibleRole));
}

BobguiInspectorOverlay *
bobgui_a11y_overlay_new (void)
{
  BobguiA11yOverlay *self;

  self = g_object_new (BOBGUI_TYPE_A11Y_OVERLAY, NULL);

  return BOBGUI_INSPECTOR_OVERLAY (self);
}
