/*
 * Copyright © 2025 Red Hat, Inc
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
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * Authors: Matthias Clasen <mclasen@redhat.com>
 */

#include "state-editor.h"
#include "shape-editor.h"
#include "path-paintable.h"
#include "bobgui/svg/bobguisvgelementprivate.h"

struct _StateEditor
{
  BobguiWindow parent_instance;

  BobguiGrid *grid;
  BobguiSpinButton *initial_state;
  PathPaintable *paintable;
  unsigned int max_state;

  gboolean updating;
};

struct _StateEditorClass
{
  BobguiWidgetClass parent_class;
};

enum
{
  PROP_PAINTABLE = 1,
  NUM_PROPERTIES,
};

static GParamSpec *properties[NUM_PROPERTIES];

G_DEFINE_TYPE (StateEditor, state_editor, BOBGUI_TYPE_WINDOW)

/* {{{ Utilities, callbacks */ 

static void repopulate (StateEditor *self);

static GdkPaintable *
get_paintable_for_shape (StateEditor *self,
                         SvgElement       *shape)
{
  return shape_get_path_image (shape, path_paintable_get_svg (self->paintable));
}

static gboolean
valid_state_name (const char *name)
{
  if (strcmp (name, "all") == 0 ||
      strcmp (name, "none") == 0 ||
      g_ascii_isdigit (name[0]))
    return FALSE;

  return TRUE;
}

static unsigned int
find_max_state (SvgElement *shape)
{
  uint64_t states = svg_element_get_states (shape);

  if (svg_element_get_type (shape) == SVG_ELEMENT_SVG ||
      svg_element_get_type (shape) == SVG_ELEMENT_GROUP)
    {
      unsigned int state = 0;
      for (unsigned int i = 0; i < svg_element_get_n_children (shape); i++)
        {
          SvgElement *sh = svg_element_get_child (shape, i);
          state = MAX (state, find_max_state (sh));
        }
      return state;
    }
  else if (states == 0 || states == G_MAXUINT64)
    return 0;
  else
    return g_bit_nth_msf (states, -1);
}

static void
update_state_names (StateEditor *self)
{
  const char *names[65] = { NULL, };
  unsigned int i;

  for (i = 0; i <= self->max_state; i++)
    {
      BobguiEditable *e;
      const char *text;

      e = BOBGUI_EDITABLE (bobgui_grid_get_child_at (self->grid, i, -1));
      text = bobgui_editable_get_text (e);
      if (text && valid_state_name (text))
        {
          names[i] = text;
        }
      else
        {
          char num[64];
          g_snprintf (num, sizeof (num), "%u", i);
          if (strcmp (num, text) == 0)
            {
              names[i] = NULL;
              break;
            }
          else
            {
              bobgui_editable_set_text (e, num);
              return;
            }
        }
    }

  names[i + 1] = NULL;
  path_paintable_set_state_names (self->paintable, names);
}

static void
update_states (StateEditor *self)
{
  BobguiLayoutManager *mgr = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (self->grid));
  uint64_t *states;
  unsigned int n;

  n = path_paintable_get_shape_count (self->paintable);

  states = g_newa0 (uint64_t, n);

  for (unsigned int i = 0; i < n; i++)
    {
      BobguiLayoutChild *layout_child;
      int row;

      BobguiWidget *child = bobgui_grid_get_child_at (self->grid, -2, i);
      BobguiWidget *toggle = bobgui_grid_get_child_at (self->grid, -1, i);

      if (!BOBGUI_IS_LABEL (child))
        break;

      layout_child = bobgui_layout_manager_get_layout_child (mgr, child);
      row = bobgui_grid_layout_child_get_row (BOBGUI_GRID_LAYOUT_CHILD (layout_child));

      if (bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (toggle)))
        states[row] = G_MAXUINT64;
    }

  for (BobguiWidget *child = bobgui_widget_get_first_child (BOBGUI_WIDGET (self->grid));
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      BobguiLayoutChild *layout_child;
      int row, col;

      layout_child = bobgui_layout_manager_get_layout_child (mgr, child);
      row = bobgui_grid_layout_child_get_row (BOBGUI_GRID_LAYOUT_CHILD (layout_child));
      col = bobgui_grid_layout_child_get_column (BOBGUI_GRID_LAYOUT_CHILD (layout_child));

      if (BOBGUI_IS_CHECK_BUTTON (child))
        {
          if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (child)))
            states[row] |= G_GUINT64_CONSTANT (1) << (unsigned int) col;
          else
            states[row] &= ~(G_GUINT64_CONSTANT (1) << (unsigned int) col);
        }
    }

  self->updating = TRUE;

  for (unsigned int i = 0; i < n; i++)
    {
      BobguiWidget *child = bobgui_grid_get_child_at (self->grid, -2, i);
      const char *id;

      if (!BOBGUI_IS_LABEL (child))
        break;

      id = bobgui_label_get_label (BOBGUI_LABEL (child));
      path_paintable_set_path_states_by_id (self->paintable, id, states[i]);
    }

  self->updating = FALSE;

  repopulate (self);
}

static void
update_one (BobguiWidget   *check,
            GParamSpec  *pspec,
            StateEditor *self)
{
  BobguiLayoutManager *mgr;
  BobguiLayoutChild *layout_child;
  int row;
  BobguiWidget *label;
  const char *id;
  uint64_t states;

  mgr = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (self->grid));
  layout_child = bobgui_layout_manager_get_layout_child (mgr, check);
  row = bobgui_grid_layout_child_get_row (BOBGUI_GRID_LAYOUT_CHILD (layout_child));

  label = bobgui_grid_get_child_at (self->grid, -2, row);
  id = bobgui_label_get_label (BOBGUI_LABEL (label));

  states = 0;
  for (unsigned int i = 0; i < self->max_state; i++)
    {
      BobguiWidget *child;

      child = bobgui_grid_get_child_at (self->grid, i, row);
      if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (child)))
        states |= (G_GUINT64_CONSTANT (1) << (unsigned int) i);
    }

  self->updating = TRUE;
  path_paintable_set_path_states_by_id (self->paintable, id, states);
  self->updating = FALSE;

  if (!bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (check)))
    {
      BobguiWidget *toggle = bobgui_grid_get_child_at (self->grid, -1, row);
      bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (toggle), FALSE);
    }
}

static void
update_all (BobguiWidget   *toggle,
            GParamSpec  *pspec,
            StateEditor *self)
{
  BobguiLayoutManager *mgr;
  BobguiLayoutChild *layout_child;
  int row;
  BobguiWidget *label;
  const char *id;

  if (!bobgui_toggle_button_get_active (BOBGUI_TOGGLE_BUTTON (toggle)))
    return;

  mgr = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (self->grid));
  layout_child = bobgui_layout_manager_get_layout_child (mgr, toggle);
  row = bobgui_grid_layout_child_get_row (BOBGUI_GRID_LAYOUT_CHILD (layout_child));

  label = bobgui_grid_get_child_at (self->grid, -2, row);
  id = bobgui_label_get_label (BOBGUI_LABEL (label));

  self->updating = TRUE;
  path_paintable_set_path_states_by_id (self->paintable, id, G_MAXUINT64);
  self->updating = FALSE;

  repopulate (self);
}

static void
drop_state (StateEditor *self)
{
  if (self->max_state == 0)
    return;

  self->max_state--;
  self->max_state = CLAMP (self->max_state, 0, 63);

  update_state_names (self);
  update_states (self);
}

static void
add_state (StateEditor *self)
{
  if (self->max_state == 63)
    return;

  self->max_state++;
  self->max_state = CLAMP (self->max_state, 0, 63);

  update_states (self);
}

static void
clear_paths (StateEditor *self)
{
  BobguiWidget *child;

  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (self->grid))) != NULL)
    bobgui_widget_unparent (child);
}

static void
create_paths_for_shape (StateEditor  *self,
                        SvgElement   *shape,
                        unsigned int *row)
{
  for (unsigned int i = 0; i < svg_element_get_n_children (shape); i++)
    {
      SvgElement *sh = svg_element_get_child (shape, i);

      if (svg_element_get_type (sh) == SVG_ELEMENT_GROUP)
        {
          create_paths_for_shape (self, sh, row);
          continue;
        }
      else if (shape_is_graphical (sh))
        {
          uint64_t states = svg_element_get_states (sh);
          const char *id = svg_element_get_id (sh);
          GdkPaintable *paintable = get_paintable_for_shape (self, sh);
          BobguiWidget *child;

          child = bobgui_image_new_from_paintable (paintable);
          bobgui_image_set_pixel_size (BOBGUI_IMAGE (child), 20);
          g_object_unref (paintable);
          bobgui_grid_attach (self->grid, child, -3, *row, 1, 1);

          child = bobgui_label_new (id);
          bobgui_grid_attach (self->grid, child, -2, *row, 1, 1);

          child = bobgui_toggle_button_new_with_label ("All");
          bobgui_grid_attach (self->grid, child, -1, *row, 1, 1);

          bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (child), states == G_MAXUINT64);

          g_signal_connect (child, "notify::active", G_CALLBACK (update_all), self);

          for (unsigned int j = 0; j <= self->max_state; j++)
            {
              child = bobgui_check_button_new ();
              bobgui_widget_set_halign (child, BOBGUI_ALIGN_CENTER);
              bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (child),
                                           (states & ((G_GUINT64_CONSTANT (1) << j))) != 0);
              g_signal_connect (child, "notify::active", G_CALLBACK (update_one), self);
              bobgui_grid_attach (self->grid, child, j, *row, 1, 1);
            }

          (*row)++;
        }
    }
}

static void
state_name_changed (BobguiEditable *editable,
                    GParamSpec  *pspec,
                    gpointer     data)
{
  StateEditor *self = (StateEditor *) data;

  if (self->updating)
    return;

  if (bobgui_editable_label_get_editing (BOBGUI_EDITABLE_LABEL (editable)))
    return;

  update_state_names (self);
}

static void
create_paths (StateEditor *self)
{
  BobguiWidget *child;
  const char **names;
  unsigned int n_names;
  unsigned int row;

  names = path_paintable_get_state_names (self->paintable, &n_names);

  for (unsigned int i = 0; i <= self->max_state; i++)
    {
      if (i < n_names)
        child = bobgui_editable_label_new (names[i]);
      else
        {
          char *s = g_strdup_printf ("%u", i);
          child = bobgui_editable_label_new (s);
          g_free (s);
        }
      bobgui_editable_set_width_chars (BOBGUI_EDITABLE (child), 6);
      bobgui_grid_attach (self->grid, child, i, -1, 1, 1);
      g_signal_connect (child, "notify::editing", G_CALLBACK (state_name_changed), self);
    }

  row = 0;
  create_paths_for_shape (self, path_paintable_get_content (self->paintable), &row);
}

static void
repopulate (StateEditor *self)
{
  if (self->updating)
    return;

  clear_paths (self);
  create_paths (self);
}

static void
paths_changed (StateEditor *self)
{
  self->max_state = MAX (self->max_state, find_max_state (path_paintable_get_content (self->paintable)));
  self->max_state = CLAMP (self->max_state, 0, 63);

  repopulate (self);
}

static void
initial_state_changed (StateEditor *self)
{
  BobguiSvg *svg = path_paintable_get_svg (self->paintable);
  svg->initial_state = (unsigned int) bobgui_spin_button_get_value_as_int (self->initial_state);
  path_paintable_changed (self->paintable);
}

/* }}} */
/* {{{ GObject boilerplate */

static void
state_editor_init (StateEditor *self)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (self));
}

static void
state_editor_set_property (GObject      *object,
                           unsigned int  prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  StateEditor *self = STATE_EDITOR (object);

  switch (prop_id)
    {
    case PROP_PAINTABLE:
      state_editor_set_paintable (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
state_editor_get_property (GObject      *object,
                           unsigned int  prop_id,
                           GValue       *value,
                           GParamSpec   *pspec)
{
  StateEditor *self = STATE_EDITOR (object);

  switch (prop_id)
    {
    case PROP_PAINTABLE:
      g_value_set_object (value, self->paintable);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
state_editor_dispose (GObject *object)
{
  StateEditor *self = STATE_EDITOR (object);

  if (self->paintable)
    g_signal_handlers_disconnect_by_func (self->paintable, paths_changed, self);

  g_clear_object (&self->paintable);

  bobgui_widget_dispose_template (BOBGUI_WIDGET (object), STATE_EDITOR_TYPE);

  G_OBJECT_CLASS (state_editor_parent_class)->dispose (object);
}

static void
state_editor_finalize (GObject *object)
{
  G_OBJECT_CLASS (state_editor_parent_class)->finalize (object);
}

static void
state_editor_class_init (StateEditorClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  g_type_ensure (SHAPE_EDITOR_TYPE);

  object_class->dispose = state_editor_dispose;
  object_class->finalize = state_editor_finalize;
  object_class->set_property = state_editor_set_property;
  object_class->get_property = state_editor_get_property;

  properties[PROP_PAINTABLE] =
    g_param_spec_object ("paintable", NULL, NULL,
                        PATH_PAINTABLE_TYPE,
                        G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

  bobgui_widget_class_set_template_from_resource (widget_class,
                                               "/org/bobgui/Shaper/state-editor.ui");

  bobgui_widget_class_bind_template_child (widget_class, StateEditor, grid);
  bobgui_widget_class_bind_template_child (widget_class, StateEditor, initial_state);
  bobgui_widget_class_bind_template_callback (widget_class, drop_state);
  bobgui_widget_class_bind_template_callback (widget_class, add_state);
  bobgui_widget_class_bind_template_callback (widget_class, initial_state_changed);
}

/* }}} */
 /* {{{ Public API */

StateEditor *
state_editor_new (void)
{
  return g_object_new (STATE_EDITOR_TYPE, NULL);
}

PathPaintable *
state_editor_get_paintable (StateEditor *self)
{
  g_return_val_if_fail (STATE_IS_EDITOR (self), NULL);

  return self->paintable;
}

void
state_editor_set_paintable (StateEditor *self,
                            PathPaintable   *paintable)
{
  g_return_if_fail (STATE_IS_EDITOR (self));

  if (self->paintable == paintable)
    return;

  if (self->paintable)
    {
      g_signal_handlers_disconnect_by_func (self->paintable, paths_changed, self);
    }

  g_set_object (&self->paintable, paintable);

  if (paintable)
    {
      g_signal_connect_swapped (paintable, "paths-changed",
                                G_CALLBACK (paths_changed), self);
      paths_changed (self);
      bobgui_spin_button_set_value (self->initial_state, path_paintable_get_svg (paintable)->initial_state);
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PAINTABLE]);
}

/* }}} */

/* vim:set foldmethod=marker: */
