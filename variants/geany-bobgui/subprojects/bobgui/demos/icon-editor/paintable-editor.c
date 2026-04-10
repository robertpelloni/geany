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

#include "paintable-editor.h"
#include "shape-editor.h"
#include "path-paintable.h"
#include "bobgui/svg/bobguisvgnumberprivate.h"
#include "bobgui/svg/bobguisvgviewboxprivate.h"
#include "bobgui/svg/bobguisvgelementprivate.h"


static void size_changed (PaintableEditor *self);
static void paintable_editor_set_compat_classes (PaintableEditor *self,
                                                 gboolean         compat_classes);

struct _PaintableEditor
{
  BobguiWidget parent_instance;

  PathPaintable *paintable;
  unsigned int state;
  gboolean compat_classes;

  BobguiScrolledWindow *swin;
  BobguiEntry *author;
  BobguiEntry *license;
  BobguiEntry *description;
  BobguiEntry *keywords;
  BobguiEntry *width;
  BobguiEntry *height;
  BobguiEntry *viewbox_x;
  BobguiEntry *viewbox_y;
  BobguiEntry *viewbox_w;
  BobguiEntry *viewbox_h;
  BobguiLabel *compat;
  BobguiLabel *summary1;
  BobguiLabel *summary2;
  BobguiImage *icon_image;
  BobguiCheckButton *compat_check;
  BobguiStack *stack;
  BobguiToggleButton *xml_toggle;
  BobguiTextView *xml_view;
  BobguiTextBuffer *xml_buffer;

  guint timeout;
  GList *errors;

  BobguiBox *path_elts;
};

struct _PaintableEditorClass
{
  BobguiWidgetClass parent_class;
};

enum
{
  PROP_PAINTABLE = 1,
  PROP_INITIAL_STATE,
  PROP_COMPAT_CLASSES,
  NUM_PROPERTIES,
};

static GParamSpec *properties[NUM_PROPERTIES];

G_DEFINE_TYPE (PaintableEditor, paintable_editor, BOBGUI_TYPE_WIDGET)

/* {{{ Utilities, callbacks */

static void
clear_shape_editors (PaintableEditor *self)
{
  BobguiWidget *child;

  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (self->path_elts))) != NULL)
    bobgui_box_remove (self->path_elts, child);
}

static void
append_shape_editor (PaintableEditor *self,
                     SvgElement      *shape)
{
  ShapeEditor *pe;

  pe = shape_editor_new (self->paintable, shape);
  if (pe)
    {
      bobgui_box_append (self->path_elts, BOBGUI_WIDGET (pe));
      bobgui_box_append (self->path_elts, bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL));
    }
}

static void
create_shape_editors (PaintableEditor *self)
{
  SvgElement *content;

  bobgui_box_append (self->path_elts, bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL));

  content = path_paintable_get_content (self->paintable);
  for (unsigned int i = 0; i < svg_element_get_n_children (content); i++)
    {
      SvgElement *shape = svg_element_get_child (content, i);
      append_shape_editor (self, shape);
    }
}

static void
update_size (PaintableEditor *self)
{
  BobguiSvg *svg = path_paintable_get_svg (self->paintable);
  g_autofree char *text = NULL;

  text = g_strdup_printf ("%g", svg->width);
  bobgui_editable_set_text (BOBGUI_EDITABLE (self->width), text);
  g_set_str (&text, g_strdup_printf ("%g", svg->height));
  bobgui_editable_set_text (BOBGUI_EDITABLE (self->height), text);
}

static void
update_viewbox (PaintableEditor *self)
{
  BobguiSvg *svg = path_paintable_get_svg (self->paintable);
  SvgValue *value;
  graphene_rect_t rect;

  value = svg_element_get_base_value (svg->content, SVG_PROPERTY_VIEW_BOX);
  if (svg_view_box_get (value, &rect))
    {
      g_autofree char *text = g_strdup_printf ("%g", rect.origin.x);
      bobgui_editable_set_text (BOBGUI_EDITABLE (self->viewbox_x), text);
      g_set_str (&text, g_strdup_printf ("%g", rect.origin.y));
      bobgui_editable_set_text (BOBGUI_EDITABLE (self->viewbox_y), text);
      g_set_str (&text, g_strdup_printf ("%g", rect.size.width));
      bobgui_editable_set_text (BOBGUI_EDITABLE (self->viewbox_w), text);
      g_set_str (&text, g_strdup_printf ("%g", rect.size.height));
      bobgui_editable_set_text (BOBGUI_EDITABLE (self->viewbox_h), text);
    }
  else
    {
      bobgui_editable_set_text (BOBGUI_EDITABLE (self->viewbox_x), "");
      bobgui_editable_set_text (BOBGUI_EDITABLE (self->viewbox_y), "");
      bobgui_editable_set_text (BOBGUI_EDITABLE (self->viewbox_w), "");
      bobgui_editable_set_text (BOBGUI_EDITABLE (self->viewbox_h), "");
    }
}

typedef struct
{
  unsigned int all;
  unsigned int graphical;
  unsigned int current;

  unsigned int state;
} ShapeCountData;

static void
count_shapes (SvgElement    *shape,
              gpointer  data)
{
  ShapeCountData *d = data;

  d->all++;

  if (!shape_is_graphical (shape))
    return;

  d->graphical++;

  if ((svg_element_get_states (shape) & (G_GUINT64_CONSTANT (1) << d->state)) == 0)
    return;

  d->current++;
}

static void
update_metadata (PaintableEditor *self)
{
  if (self->paintable)
    {
      BobguiSvg *svg = path_paintable_get_svg (self->paintable);

      bobgui_editable_set_text (BOBGUI_EDITABLE (self->author), svg->author ? svg->author : "");
      bobgui_editable_set_text (BOBGUI_EDITABLE (self->license), svg->license ? svg->license : "");
      bobgui_editable_set_text (BOBGUI_EDITABLE (self->description), svg->description ? svg->description : "");
      bobgui_editable_set_text (BOBGUI_EDITABLE (self->keywords), svg->keywords ? svg->keywords : "");
    }
}

static void
update_summary (PaintableEditor *self)
{
  if (self->paintable)
    {
      BobguiSvg *svg = path_paintable_get_svg (self->paintable);
      unsigned int state, n_names;
      const char **names;
      g_autofree char *summary1 = NULL;
      g_autofree char *summary2 = NULL;
      ShapeCountData counts;

      state = bobgui_svg_get_state (svg);
      names = bobgui_svg_get_state_names (svg, &n_names);

      counts.state = state;
      counts.all = counts.graphical = counts.current = 0;
      svg_element_foreach (svg->content, count_shapes, &counts);

      if (state < n_names)
        summary1 = g_strdup_printf ("Current state: %u (%s)", state, names[state]);
      else
        summary1 = g_strdup_printf ("Current state: %u", state);

      summary2 = g_strdup_printf ("%u graphical shapes, %u in current state", counts.graphical, counts.current);

      bobgui_label_set_label (self->summary1, summary1);
      bobgui_label_set_label (self->summary2, summary2);
    }
  else
    {
      bobgui_label_set_label (self->summary1, "");
      bobgui_label_set_label (self->summary2, "");
    }
}

static void
update_compat (PaintableEditor *self)
{
  switch (path_paintable_get_compatibility (self->paintable))
    {
    case BOBGUI_4_0:
      bobgui_label_set_label (self->compat, "BOBGUI 4.0");
      break;
    case BOBGUI_4_20:
      bobgui_label_set_label (self->compat, "BOBGUI 4.20");
      break;
    case BOBGUI_4_22:
      bobgui_label_set_label (self->compat, "BOBGUI 4.22");
      break;
    default:
      g_assert_not_reached ();
    }
}

static void
update_icon_paintable (PaintableEditor *self)
{
  GdkPaintable *paintable = GDK_PAINTABLE (path_paintable_get_icon_paintable (self->paintable));

  bobgui_image_set_from_paintable (self->icon_image, paintable);

  g_object_unref (paintable);
}

typedef struct
{
  GError *error;
  BobguiTextIter start;
  BobguiTextIter end;
} SvgError;

static void
svg_error_free (gpointer data)
{
  SvgError *error = data;

  g_error_free (error->error);
  g_free (error);
}

typedef struct
{
  PaintableEditor *self;
  const char *text;
} ErrorData;

static void
error_cb (BobguiSvg   *svg,
          GError   *error,
          gpointer  data)
{
/* Without GLib 2.88, we don't get usable location
 * information from GMarkup, so don't try to highlight
 * errors
 */
#if GLIB_CHECK_VERSION (2, 88, 0)
  ErrorData *d = data;
  PaintableEditor *self = d->self;
  SvgError *svg_error;
  size_t offset;
  const BobguiSvgLocation *start, *end;
  const char *tag;

  if (error->domain != BOBGUI_SVG_ERROR)
    return;

  start = bobgui_svg_error_get_start (error);
  end = bobgui_svg_error_get_end (error);

  svg_error = g_new (SvgError, 1);
  svg_error->error = g_error_copy (error);

  offset = g_utf8_pointer_to_offset (d->text, d->text + start->bytes);
  bobgui_text_buffer_get_iter_at_offset (self->xml_buffer, &svg_error->start, offset);
  offset = g_utf8_pointer_to_offset (d->text, d->text + end->bytes);
  bobgui_text_buffer_get_iter_at_offset (self->xml_buffer, &svg_error->end, offset);

  self->errors = g_list_append (self->errors, svg_error);

  if (bobgui_text_iter_equal (&svg_error->start, &svg_error->end))
    bobgui_text_iter_forward_chars (&svg_error->end, 1);

  if (error->code == BOBGUI_SVG_ERROR_IGNORED_ELEMENT)
    tag = "ignored";
  else if (error->code == BOBGUI_SVG_ERROR_NOT_IMPLEMENTED)
    tag = "unimplemented";
  else
    tag = "error";

  bobgui_text_buffer_apply_tag_by_name (self->xml_buffer,
                                     tag,
                                     &svg_error->start,
                                     &svg_error->end);
#endif
}

static void
update_timeout (gpointer data)
{
  PaintableEditor *self = data;
  BobguiTextIter start, end;
  char *text;
  g_autoptr (GBytes) bytes = NULL;
  g_autoptr (BobguiSvg) svg = NULL;
  gulong handler;
  ErrorData d;

  bobgui_text_buffer_get_bounds (self->xml_buffer, &start, &end);
  bobgui_text_buffer_remove_all_tags (self->xml_buffer, &start, &end);

  text = bobgui_text_buffer_get_text (self->xml_buffer, &start, &end, FALSE);
  bytes = g_bytes_new_take (text, strlen (text));

  svg = bobgui_svg_new ();

  g_list_free_full (self->errors, svg_error_free);
  self->errors = NULL;

  d.self = self;
  d.text = text;

  handler = g_signal_connect (svg, "error", G_CALLBACK (error_cb), &d);
  bobgui_svg_load_from_bytes (svg, bytes);
  g_signal_handler_disconnect (svg, handler);

  path_paintable_set_svg (self->paintable, svg);

  self->timeout = 0;
}

static void
xml_changed (PaintableEditor *self)
{
  if (self->timeout != 0)
    g_source_remove (self->timeout);
  self->timeout = g_timeout_add_once (100, update_timeout, self);
}

/* }}} */

static void
update_xml (PaintableEditor *self)
{
  BobguiSvg *svg = path_paintable_get_svg (self->paintable);
  g_autoptr (GBytes) xml = bobgui_svg_serialize (svg);

  g_signal_handlers_block_by_func (self->xml_buffer, xml_changed, self);
  bobgui_text_buffer_set_text (self->xml_buffer, g_bytes_get_data (xml, NULL), g_bytes_get_size (xml));
  update_timeout (self);
  g_signal_handlers_unblock_by_func (self->xml_buffer, xml_changed, self);
}

static void
paths_changed (PaintableEditor *self)
{
  clear_shape_editors (self);
  create_shape_editors (self);
  update_summary (self);
  update_icon_paintable (self);
}

static void
changed (PaintableEditor *self)
{
  update_compat (self);
  update_size (self);
  update_viewbox (self);
  update_metadata (self);
  update_summary (self);
  update_icon_paintable (self);
}

static void
set_size (PaintableEditor *self,
          double           width,
          double           height)
{
  BobguiSvg *svg = path_paintable_get_svg (self->paintable);
  SvgValue *value;
  graphene_rect_t rect;

  svg->width = width;
  svg->height = height;

  svg_element_take_base_value (svg->content, SVG_PROPERTY_WIDTH, svg_number_new (width));
  svg_element_take_base_value (svg->content, SVG_PROPERTY_HEIGHT, svg_number_new (height));

  value = svg_element_get_base_value (svg->content, SVG_PROPERTY_VIEW_BOX);

  if (!svg_view_box_get (value, &rect))
    svg_element_take_base_value (svg->content, SVG_PROPERTY_VIEW_BOX,
                        svg_view_box_new (&GRAPHENE_RECT_INIT (0, 0, width, height)));

  path_paintable_changed (self->paintable);
  gdk_paintable_invalidate_size (GDK_PAINTABLE (self->paintable));
}

static void
size_changed (PaintableEditor *self)
{
  const char *text;
  double width, height;
  int res;

  text = bobgui_editable_get_text (BOBGUI_EDITABLE (self->width));
  res = sscanf (text, "%lf", &width);
  text = bobgui_editable_get_text (BOBGUI_EDITABLE (self->height));
  res += sscanf (text, "%lf", &height);
  if (res == 2 && width > 0 && height > 0)
    set_size (self, width, height);
}

static void
viewbox_changed (PaintableEditor *self)
{
  BobguiSvg *svg = path_paintable_get_svg (self->paintable);
  const char *text;
  double x, y, w, h;
  int res;

  text = bobgui_editable_get_text (BOBGUI_EDITABLE (self->viewbox_x));
  res = sscanf (text, "%lf", &x);
  text = bobgui_editable_get_text (BOBGUI_EDITABLE (self->viewbox_y));
  res += sscanf (text, "%lf", &y);
  text = bobgui_editable_get_text (BOBGUI_EDITABLE (self->viewbox_w));
  res += sscanf (text, "%lf", &w);
  text = bobgui_editable_get_text (BOBGUI_EDITABLE (self->viewbox_h));
  res += sscanf (text, "%lf", &h);
  if (res == 4 && w > 0 && h > 0)
    {
      svg_element_take_base_value (svg->content, SVG_PROPERTY_VIEW_BOX,
                                   svg_view_box_new (&GRAPHENE_RECT_INIT (x, y, w, h)));

      path_paintable_changed (self->paintable);
      gdk_paintable_invalidate_size (GDK_PAINTABLE (self->paintable));
    }
}

static void
author_changed (PaintableEditor *self)
{
  const char *text = bobgui_editable_get_text (BOBGUI_EDITABLE (self->author));
  path_paintable_set_author (self->paintable, text);
}

static void
license_changed (PaintableEditor *self)
{
  const char *text = bobgui_editable_get_text (BOBGUI_EDITABLE (self->license));
  path_paintable_set_license (self->paintable, text);
}

static void
description_changed (PaintableEditor *self)
{
  const char *text = bobgui_editable_get_text (BOBGUI_EDITABLE (self->description));
  path_paintable_set_description (self->paintable, text);
}

static void
keywords_changed (PaintableEditor *self)
{
  const char *text = bobgui_editable_get_text (BOBGUI_EDITABLE (self->keywords));
  path_paintable_set_keywords (self->paintable, text);
}

static void
xml_toggled (PaintableEditor *self)
{
  if (bobgui_toggle_button_get_active (self->xml_toggle))
    {
      update_xml (self);
      bobgui_stack_set_visible_child_name (self->stack, "xml");
      bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (self->xml_toggle), "View Controls");
    }
  else
    {
      bobgui_stack_set_visible_child_name (self->stack, "controls");
      bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (self->xml_toggle), "View XML");
    }
}

static gboolean
query_tooltip_cb (BobguiWidget       *widget,
                  int              x,
                  int              y,
                  gboolean         keyboard_tip,
                  BobguiTooltip      *tooltip,
                  PaintableEditor *self)
{
  BobguiTextIter iter;

  if (!bobgui_toggle_button_get_active (self->xml_toggle))
    return FALSE;

  if (keyboard_tip)
    {
      int offset;

      g_object_get (self->xml_view, "cursor-position", &offset, NULL);
      bobgui_text_buffer_get_iter_at_offset (self->xml_buffer, &iter, offset);
    }
  else
    {
      int bx, by, trailing;

      bobgui_text_view_window_to_buffer_coords (self->xml_view,
                                             BOBGUI_TEXT_WINDOW_TEXT,
                                             x, y, &bx, &by);
      bobgui_text_view_get_iter_at_position (self->xml_view, &iter, &trailing, bx, by);
    }

  for (GList *l = self->errors; l; l = l->next)
    {
      SvgError *error = l->data;

      if (bobgui_text_iter_in_range (&iter, &error->start, &error->end))
        {
          bobgui_tooltip_set_text (tooltip, error->error->message);
          return TRUE;
        }
    }

  return FALSE;
}

/* {{{ GObject boilerplate */

static void
paintable_editor_init (PaintableEditor *self)
{
  self->compat_classes = TRUE;
  bobgui_widget_init_template (BOBGUI_WIDGET (self));
}

static void
paintable_editor_set_property (GObject      *object,
                               unsigned int  prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  PaintableEditor *self = PAINTABLE_EDITOR (object);

  switch (prop_id)
    {
    case PROP_PAINTABLE:
      paintable_editor_set_paintable (self, g_value_get_object (value));
      break;

    case PROP_COMPAT_CLASSES:
      paintable_editor_set_compat_classes (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
paintable_editor_get_property (GObject      *object,
                               unsigned int  prop_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  PaintableEditor *self = PAINTABLE_EDITOR (object);

  switch (prop_id)
    {
    case PROP_PAINTABLE:
      g_value_set_object (value, self->paintable);
      break;

    case PROP_INITIAL_STATE:
      g_value_set_uint (value, self->state);
      break;

    case PROP_COMPAT_CLASSES:
      g_value_set_boolean (value, self->compat_classes);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
paintable_editor_dispose (GObject *object)
{
  PaintableEditor *self = PAINTABLE_EDITOR (object);

  g_list_free_full (self->errors, svg_error_free);
  self->errors = NULL;

  if (self->timeout)
    {
      g_source_remove (self->timeout);
      self->timeout = 0;
    }

  if (self->paintable)
    g_signal_handlers_disconnect_by_func (self->paintable, paths_changed, self);

  g_clear_object (&self->paintable);

  clear_shape_editors (self);

  bobgui_widget_dispose_template (BOBGUI_WIDGET (object), PAINTABLE_EDITOR_TYPE);

  G_OBJECT_CLASS (paintable_editor_parent_class)->dispose (object);
}

static void
paintable_editor_finalize (GObject *object)
{
  G_OBJECT_CLASS (paintable_editor_parent_class)->finalize (object);
}

static void
paintable_editor_class_init (PaintableEditorClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  g_type_ensure (SHAPE_EDITOR_TYPE);

  object_class->dispose = paintable_editor_dispose;
  object_class->finalize = paintable_editor_finalize;
  object_class->set_property = paintable_editor_set_property;
  object_class->get_property = paintable_editor_get_property;

  properties[PROP_PAINTABLE] =
    g_param_spec_object ("paintable", NULL, NULL,
                        PATH_PAINTABLE_TYPE,
                        G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  properties[PROP_INITIAL_STATE] =
    g_param_spec_uint ("initial-state", NULL, NULL,
                       0, 64, 0,
                       G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  properties[PROP_COMPAT_CLASSES] =
    g_param_spec_boolean ("compat-classes", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_NAME);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

  bobgui_widget_class_set_template_from_resource (widget_class,
                                               "/org/bobgui/Shaper/paintable-editor.ui");

  bobgui_widget_class_bind_template_child (widget_class, PaintableEditor, swin);
  bobgui_widget_class_bind_template_child (widget_class, PaintableEditor, author);
  bobgui_widget_class_bind_template_child (widget_class, PaintableEditor, license);
  bobgui_widget_class_bind_template_child (widget_class, PaintableEditor, description);
  bobgui_widget_class_bind_template_child (widget_class, PaintableEditor, keywords);
  bobgui_widget_class_bind_template_child (widget_class, PaintableEditor, width);
  bobgui_widget_class_bind_template_child (widget_class, PaintableEditor, height);
  bobgui_widget_class_bind_template_child (widget_class, PaintableEditor, viewbox_x);
  bobgui_widget_class_bind_template_child (widget_class, PaintableEditor, viewbox_y);
  bobgui_widget_class_bind_template_child (widget_class, PaintableEditor, viewbox_w);
  bobgui_widget_class_bind_template_child (widget_class, PaintableEditor, viewbox_h);
  bobgui_widget_class_bind_template_child (widget_class, PaintableEditor, compat);
  bobgui_widget_class_bind_template_child (widget_class, PaintableEditor, summary1);
  bobgui_widget_class_bind_template_child (widget_class, PaintableEditor, summary2);
  bobgui_widget_class_bind_template_child (widget_class, PaintableEditor, icon_image);
  bobgui_widget_class_bind_template_child (widget_class, PaintableEditor, path_elts);
  bobgui_widget_class_bind_template_child (widget_class, PaintableEditor, compat_check);
  bobgui_widget_class_bind_template_child (widget_class, PaintableEditor, stack);
  bobgui_widget_class_bind_template_child (widget_class, PaintableEditor, xml_toggle);
  bobgui_widget_class_bind_template_child (widget_class, PaintableEditor, xml_view);
  bobgui_widget_class_bind_template_child (widget_class, PaintableEditor, xml_buffer);

  bobgui_widget_class_bind_template_callback (widget_class, size_changed);
  bobgui_widget_class_bind_template_callback (widget_class, viewbox_changed);
  bobgui_widget_class_bind_template_callback (widget_class, author_changed);
  bobgui_widget_class_bind_template_callback (widget_class, license_changed);
  bobgui_widget_class_bind_template_callback (widget_class, description_changed);
  bobgui_widget_class_bind_template_callback (widget_class, keywords_changed);
  bobgui_widget_class_bind_template_callback (widget_class, xml_toggled);
  bobgui_widget_class_bind_template_callback (widget_class, xml_changed);
  bobgui_widget_class_bind_template_callback (widget_class, query_tooltip_cb);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, "PaintableEditor");
}

/* }}} */
/* {{{ Public API */

PaintableEditor *
paintable_editor_new (void)
{
  return g_object_new (PAINTABLE_EDITOR_TYPE, NULL);
}

PathPaintable *
paintable_editor_get_paintable (PaintableEditor *self)
{
  g_return_val_if_fail (PAINTABLE_IS_EDITOR (self), NULL);

  return self->paintable;
}

void
paintable_editor_set_paintable (PaintableEditor *self,
                                PathPaintable   *paintable)
{
  g_return_if_fail (PAINTABLE_IS_EDITOR (self));

  if (self->paintable == paintable)
    return;

  if (self->paintable)
    {
      g_signal_handlers_disconnect_by_func (self->paintable, paths_changed, self);
      g_signal_handlers_disconnect_by_func (self->paintable, changed, self);
      g_signal_handlers_disconnect_by_func (self->paintable, update_summary, self);
    }

  clear_shape_editors (self);

  g_set_object (&self->paintable, paintable);

  if (paintable)
    {
      g_signal_connect_swapped (paintable, "paths-changed",
                                G_CALLBACK (paths_changed), self);
      g_signal_connect_swapped (paintable, "changed",
                                G_CALLBACK (changed), self);
      g_signal_connect_swapped (paintable, "notify::state",
                                G_CALLBACK (update_summary), self);

      create_shape_editors (self);
      update_summary (self);
      update_metadata (self);
      update_size (self);
      update_viewbox (self);
      update_compat (self);
      update_icon_paintable (self);
      update_xml (self);
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PAINTABLE]);
}

static void
paintable_editor_set_compat_classes (PaintableEditor *self,
                                     gboolean         compat_classes)
{
  g_return_if_fail (PAINTABLE_IS_EDITOR (self));

  if (self->compat_classes == compat_classes)
    return;

  self->compat_classes = compat_classes;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_COMPAT_CLASSES]);
}

void
paintable_editor_add_path (PaintableEditor *self)
{
  BobguiSvg *svg = path_paintable_get_svg (self->paintable);
  GskPathBuilder *builder;
  g_autoptr (GskPath) path = NULL;
  SvgElement *content;
  size_t idx;
  SvgElement *shape;
  SvgValue *value;
  graphene_rect_t rect;
  char *id;

  if (svg_element_get_n_children (svg->content) == 0 && svg->width == 0 && svg->height == 0)
    set_size (self, 100, 100);

  value = svg_element_get_base_value (svg->content, SVG_PROPERTY_VIEW_BOX);
  svg_view_box_get (value, &rect);

  builder = gsk_path_builder_new ();
  gsk_path_builder_move_to (builder, rect.origin.x, rect.origin.y);
  gsk_path_builder_rel_line_to (builder, rect.size.width, rect.size.height);
  path = gsk_path_builder_free_to_path (builder);
  g_signal_handlers_block_by_func (self->paintable, paths_changed, self);
  idx = path_paintable_add_path (self->paintable, path);

  shape = path_paintable_get_shape (self->paintable, idx);
  id = path_paintable_find_unused_id (self->paintable, "path");
  svg_element_set_id (shape, id);
  g_free (id);

  content = path_paintable_get_content (self->paintable);
  append_shape_editor (self, svg_element_get_child (content, svg_element_get_n_children (content) - 1));
  g_signal_handlers_unblock_by_func (self->paintable, paths_changed, self);
}

/* }}} */

/* vim:set foldmethod=marker: */
