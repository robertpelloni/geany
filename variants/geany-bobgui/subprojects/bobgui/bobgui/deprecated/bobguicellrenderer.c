/* bobguicellrenderer.c
 * Copyright (C) 2000  Red Hat, Inc. Jonathan Blandford
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "bobguicellrenderer.h"

#include "bobguimarshalers.h"
#include "bobguiprivate.h"
#include "bobguisnapshot.h"
#include "bobguistylecontext.h"
#include "bobguitreeprivate.h"
#include "bobguitypebuiltins.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiCellRenderer:
 *
 * An object for rendering a single cell
 *
 * The `BobguiCellRenderer` is a base class of a set of objects used for
 * rendering a cell to a `cairo_t`.  These objects are used primarily by
 * the `BobguiTreeView` widget, though they aren’t tied to them in any
 * specific way.  It is worth noting that `BobguiCellRenderer` is not a
 * `BobguiWidget` and cannot be treated as such.
 *
 * The primary use of a `BobguiCellRenderer` is for drawing a certain graphical
 * elements on a `cairo_t`. Typically, one cell renderer is used to
 * draw many cells on the screen.  To this extent, it isn’t expected that a
 * CellRenderer keep any permanent state around.  Instead, any state is set
 * just prior to use using `GObject`s property system.  Then, the
 * cell is measured using bobgui_cell_renderer_get_preferred_size(). Finally, the cell
 * is rendered in the correct location using bobgui_cell_renderer_snapshot().
 *
 * There are a number of rules that must be followed when writing a new
 * `BobguiCellRenderer`.  First and foremost, it’s important that a certain set
 * of properties will always yield a cell renderer of the same size,
 * barring a style change. The `BobguiCellRenderer` also has a number of
 * generic properties that are expected to be honored by all children.
 *
 * Beyond merely rendering a cell, cell renderers can optionally
 * provide active user interface elements. A cell renderer can be
 * “activatable” like `BobguiCellRenderer`Toggle,
 * which toggles when it gets activated by a mouse click, or it can be
 * “editable” like `BobguiCellRenderer`Text, which
 * allows the user to edit the text using a widget implementing the
 * `BobguiCellEditable` interface, e.g. `BobguiEntry`.
 * To make a cell renderer activatable or editable, you have to
 * implement the `BobguiCellRenderer`Class.activate or
 * `BobguiCellRenderer`Class.start_editing virtual functions, respectively.
 *
 * Many properties of `BobguiCellRenderer` and its subclasses have a
 * corresponding “set” property, e.g. “cell-background-set” corresponds
 * to “cell-background”. These “set” properties reflect whether a property
 * has been set or not. You should not set them independently.
 *
 * Deprecated: 4.10: List views use widgets for displaying their
 *   contents
 */


#define DEBUG_CELL_SIZE_REQUEST 0

static void bobgui_cell_renderer_init          (BobguiCellRenderer      *cell);
static void bobgui_cell_renderer_class_init    (BobguiCellRendererClass *class);
static void bobgui_cell_renderer_get_property  (GObject              *object,
					     guint                 param_id,
					     GValue               *value,
					     GParamSpec           *pspec);
static void bobgui_cell_renderer_set_property  (GObject              *object,
					     guint                 param_id,
					     const GValue         *value,
					     GParamSpec           *pspec);
static void set_cell_bg_color               (BobguiCellRenderer      *cell,
					     GdkRGBA              *rgba);

static BobguiSizeRequestMode bobgui_cell_renderer_real_get_request_mode(BobguiCellRenderer         *cell);
static void bobgui_cell_renderer_real_get_preferred_width           (BobguiCellRenderer         *cell,
                                                                  BobguiWidget               *widget,
                                                                  int                     *minimum_size,
                                                                  int                     *natural_size);
static void bobgui_cell_renderer_real_get_preferred_height          (BobguiCellRenderer         *cell,
                                                                  BobguiWidget               *widget,
                                                                  int                     *minimum_size,
                                                                  int                     *natural_size);
static void bobgui_cell_renderer_real_get_preferred_height_for_width(BobguiCellRenderer         *cell,
                                                                  BobguiWidget               *widget,
                                                                  int                      width,
                                                                  int                     *minimum_height,
                                                                  int                     *natural_height);
static void bobgui_cell_renderer_real_get_preferred_width_for_height(BobguiCellRenderer         *cell,
                                                                  BobguiWidget               *widget,
                                                                  int                      height,
                                                                  int                     *minimum_width,
                                                                  int                     *natural_width);
static void bobgui_cell_renderer_real_get_aligned_area              (BobguiCellRenderer         *cell,
								  BobguiWidget               *widget,
								  BobguiCellRendererState     flags,
								  const GdkRectangle      *cell_area,
								  GdkRectangle            *aligned_area);


struct _BobguiCellRendererPrivate
{
  float xalign;
  float yalign;

  int width;
  int height;

  guint16 xpad;
  guint16 ypad;

  guint mode                : 2;
  guint visible             : 1;
  guint is_expander         : 1;
  guint is_expanded         : 1;
  guint cell_background_set : 1;
  guint sensitive           : 1;
  guint editing             : 1;

  GdkRGBA cell_background;
};

enum {
  PROP_0,
  PROP_MODE,
  PROP_VISIBLE,
  PROP_SENSITIVE,
  PROP_XALIGN,
  PROP_YALIGN,
  PROP_XPAD,
  PROP_YPAD,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_IS_EXPANDER,
  PROP_IS_EXPANDED,
  PROP_CELL_BACKGROUND,
  PROP_CELL_BACKGROUND_RGBA,
  PROP_CELL_BACKGROUND_SET,
  PROP_EDITING
};

/* Signal IDs */
enum {
  EDITING_CANCELED,
  EDITING_STARTED,
  LAST_SIGNAL
};

static int BobguiCellRenderer_private_offset;
static guint  cell_renderer_signals[LAST_SIGNAL] = { 0 };

static inline gpointer
bobgui_cell_renderer_get_instance_private (BobguiCellRenderer *self)
{
  return (G_STRUCT_MEMBER_P (self, BobguiCellRenderer_private_offset));
}

static void
bobgui_cell_renderer_init (BobguiCellRenderer *cell)
{
  BobguiCellRendererPrivate *priv;

  cell->priv = bobgui_cell_renderer_get_instance_private (cell);
  priv = cell->priv;

  priv->mode = BOBGUI_CELL_RENDERER_MODE_INERT;
  priv->visible = TRUE;
  priv->width = -1;
  priv->height = -1;
  priv->xalign = 0.5;
  priv->yalign = 0.5;
  priv->xpad = 0;
  priv->ypad = 0;
  priv->sensitive = TRUE;
  priv->is_expander = FALSE;
  priv->is_expanded = FALSE;
  priv->editing = FALSE;
}

static void
bobgui_cell_renderer_class_init (BobguiCellRendererClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->get_property = bobgui_cell_renderer_get_property;
  object_class->set_property = bobgui_cell_renderer_set_property;

  class->snapshot = NULL;
  class->get_request_mode               = bobgui_cell_renderer_real_get_request_mode;
  class->get_preferred_width            = bobgui_cell_renderer_real_get_preferred_width;
  class->get_preferred_height           = bobgui_cell_renderer_real_get_preferred_height;
  class->get_preferred_width_for_height = bobgui_cell_renderer_real_get_preferred_width_for_height;
  class->get_preferred_height_for_width = bobgui_cell_renderer_real_get_preferred_height_for_width;
  class->get_aligned_area               = bobgui_cell_renderer_real_get_aligned_area;

  /**
   * BobguiCellRenderer::editing-canceled:
   * @renderer: the object which received the signal
   *
   * This signal gets emitted when the user cancels the process of editing a
   * cell.  For example, an editable cell renderer could be written to cancel
   * editing when the user presses Escape.
   *
   * See also: bobgui_cell_renderer_stop_editing().
   */
  cell_renderer_signals[EDITING_CANCELED] =
    g_signal_new (I_("editing-canceled"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BobguiCellRendererClass, editing_canceled),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);

  /**
   * BobguiCellRenderer::editing-started:
   * @renderer: the object which received the signal
   * @editable: the `BobguiCellEditable`
   * @path: the path identifying the edited cell
   *
   * This signal gets emitted when a cell starts to be edited.
   * The intended use of this signal is to do special setup
   * on @editable, e.g. adding a `BobguiEntryCompletion` or setting
   * up additional columns in a `BobguiComboBox`.
   *
   * See bobgui_cell_editable_start_editing() for information on the lifecycle of
   * the @editable and a way to do setup that doesn’t depend on the @renderer.
   *
   * Note that BOBGUI doesn't guarantee that cell renderers will
   * continue to use the same kind of widget for editing in future
   * releases, therefore you should check the type of @editable
   * before doing any specific setup, as in the following example:
   *
   * ```c
   * static void
   * text_editing_started (BobguiCellRenderer *cell,
   *                       BobguiCellEditable *editable,
   *                       const char      *path,
   *                       gpointer         data)
   * {
   *   if (BOBGUI_IS_ENTRY (editable))
   *     {
   *       BobguiEntry *entry = BOBGUI_ENTRY (editable);
   *
   *       // ... create a BobguiEntryCompletion
   *
   *       bobgui_entry_set_completion (entry, completion);
   *     }
   * }
   * ```
   */
  cell_renderer_signals[EDITING_STARTED] =
    g_signal_new (I_("editing-started"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BobguiCellRendererClass, editing_started),
		  NULL, NULL,
		  _bobgui_marshal_VOID__OBJECT_STRING,
		  G_TYPE_NONE, 2,
		  BOBGUI_TYPE_CELL_EDITABLE,
		  G_TYPE_STRING);
  g_signal_set_va_marshaller (cell_renderer_signals[EDITING_STARTED],
                              G_TYPE_FROM_CLASS (object_class),
                              _bobgui_marshal_VOID__OBJECT_STRINGv);

  g_object_class_install_property (object_class,
				   PROP_MODE,
				   g_param_spec_enum ("mode", NULL, NULL,
						      BOBGUI_TYPE_CELL_RENDERER_MODE,
						      BOBGUI_CELL_RENDERER_MODE_INERT,
						      BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_VISIBLE,
				   g_param_spec_boolean ("visible", NULL, NULL,
							 TRUE,
							 BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));
  g_object_class_install_property (object_class,
				   PROP_SENSITIVE,
				   g_param_spec_boolean ("sensitive", NULL, NULL,
							 TRUE,
							 BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_XALIGN,
				   g_param_spec_float ("xalign", NULL, NULL,
						       0.0,
						       1.0,
						       0.5,
						       BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_YALIGN,
				   g_param_spec_float ("yalign", NULL, NULL,
						       0.0,
						       1.0,
						       0.5,
						       BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_XPAD,
				   g_param_spec_uint ("xpad", NULL, NULL,
						      0,
						      G_MAXUINT,
						      0,
						      BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_YPAD,
				   g_param_spec_uint ("ypad", NULL, NULL,
						      0,
						      G_MAXUINT,
						      0,
						      BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_WIDTH,
				   g_param_spec_int ("width", NULL, NULL,
						     -1,
						     G_MAXINT,
						     -1,
						     BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_HEIGHT,
				   g_param_spec_int ("height", NULL, NULL,
						     -1,
						     G_MAXINT,
						     -1,
						     BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_IS_EXPANDER,
				   g_param_spec_boolean ("is-expander", NULL, NULL,
							 FALSE,
							 BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));


  g_object_class_install_property (object_class,
				   PROP_IS_EXPANDED,
				   g_param_spec_boolean ("is-expanded", NULL, NULL,
							 FALSE,
							 BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  g_object_class_install_property (object_class,
				   PROP_CELL_BACKGROUND,
				   g_param_spec_string ("cell-background", NULL, NULL,
							NULL,
							BOBGUI_PARAM_WRITABLE));

  /**
   * BobguiCellRenderer:cell-background-rgba:
   *
   * Cell background as a `GdkRGBA`
   */
  g_object_class_install_property (object_class,
				   PROP_CELL_BACKGROUND_RGBA,
				   g_param_spec_boxed ("cell-background-rgba", NULL, NULL,
						       GDK_TYPE_RGBA,
						       BOBGUI_PARAM_READWRITE));

  g_object_class_install_property (object_class,
				   PROP_EDITING,
				   g_param_spec_boolean ("editing", NULL, NULL,
							 FALSE,
							 BOBGUI_PARAM_READABLE));


#define ADD_SET_PROP(propname, propval, nick, blurb) g_object_class_install_property (object_class, propval, g_param_spec_boolean (propname, nick, blurb, FALSE, BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY))

  ADD_SET_PROP ("cell-background-set", PROP_CELL_BACKGROUND_SET, NULL, NULL);

  if (BobguiCellRenderer_private_offset != 0)
    g_type_class_adjust_private_offset (class, &BobguiCellRenderer_private_offset);
}

GType
bobgui_cell_renderer_get_type (void)
{
  static GType cell_renderer_type = 0;

  if (G_UNLIKELY (cell_renderer_type == 0))
    {
      const GTypeInfo cell_renderer_info =
      {
	sizeof (BobguiCellRendererClass),
        NULL,
        NULL,
	(GClassInitFunc) bobgui_cell_renderer_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_init */
	sizeof (BobguiWidget),
	0,		/* n_preallocs */
	(GInstanceInitFunc) bobgui_cell_renderer_init,
	NULL,		/* value_table */
      };
      cell_renderer_type = g_type_register_static (G_TYPE_INITIALLY_UNOWNED, "BobguiCellRenderer",
                                                   &cell_renderer_info, G_TYPE_FLAG_ABSTRACT);

      BobguiCellRenderer_private_offset =
        g_type_add_instance_private (cell_renderer_type, sizeof (BobguiCellRendererPrivate));
    }

  return cell_renderer_type;
}

static void
bobgui_cell_renderer_get_property (GObject     *object,
				guint        param_id,
				GValue      *value,
				GParamSpec  *pspec)
{
  BobguiCellRenderer *cell = BOBGUI_CELL_RENDERER (object);
  BobguiCellRendererPrivate *priv = cell->priv;

  switch (param_id)
    {
    case PROP_MODE:
      g_value_set_enum (value, priv->mode);
      break;
    case PROP_VISIBLE:
      g_value_set_boolean (value, priv->visible);
      break;
    case PROP_SENSITIVE:
      g_value_set_boolean (value, priv->sensitive);
      break;
    case PROP_EDITING:
      g_value_set_boolean (value, priv->editing);
      break;
    case PROP_XALIGN:
      g_value_set_float (value, priv->xalign);
      break;
    case PROP_YALIGN:
      g_value_set_float (value, priv->yalign);
      break;
    case PROP_XPAD:
      g_value_set_uint (value, priv->xpad);
      break;
    case PROP_YPAD:
      g_value_set_uint (value, priv->ypad);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, priv->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, priv->height);
      break;
    case PROP_IS_EXPANDER:
      g_value_set_boolean (value, priv->is_expander);
      break;
    case PROP_IS_EXPANDED:
      g_value_set_boolean (value, priv->is_expanded);
      break;
    case PROP_CELL_BACKGROUND_RGBA:
      g_value_set_boxed (value, &priv->cell_background);
      break;
    case PROP_CELL_BACKGROUND_SET:
      g_value_set_boolean (value, priv->cell_background_set);
      break;
    case PROP_CELL_BACKGROUND:
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }

}

static void
bobgui_cell_renderer_set_property (GObject      *object,
				guint         param_id,
				const GValue *value,
				GParamSpec   *pspec)
{
  BobguiCellRenderer *cell = BOBGUI_CELL_RENDERER (object);
  BobguiCellRendererPrivate *priv = cell->priv;

  switch (param_id)
    {
    case PROP_MODE:
      if (priv->mode != g_value_get_enum (value))
        {
          priv->mode = g_value_get_enum (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_VISIBLE:
      if (priv->visible != g_value_get_boolean (value))
        {
          priv->visible = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_SENSITIVE:
      if (priv->sensitive != g_value_get_boolean (value))
        {
          priv->sensitive = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_XALIGN:
      if (priv->xalign != g_value_get_float (value))
        {
          priv->xalign = g_value_get_float (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_YALIGN:
      if (priv->yalign != g_value_get_float (value))
        {
          priv->yalign = g_value_get_float (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_XPAD:
      if (priv->xpad != g_value_get_uint (value))
        {
          priv->xpad = g_value_get_uint (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_YPAD:
      if (priv->ypad != g_value_get_uint (value))
        {
          priv->ypad = g_value_get_uint (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_WIDTH:
      if (priv->width != g_value_get_int (value))
        {
          priv->width = g_value_get_int (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_HEIGHT:
      if (priv->height != g_value_get_int (value))
        {
          priv->height = g_value_get_int (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_IS_EXPANDER:
      if (priv->is_expander != g_value_get_boolean (value))
        {
          priv->is_expander = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_IS_EXPANDED:
      if (priv->is_expanded != g_value_get_boolean (value))
        {
          priv->is_expanded = g_value_get_boolean (value);
          g_object_notify_by_pspec (object, pspec);
        }
      break;
    case PROP_CELL_BACKGROUND:
      {
        GdkRGBA rgba;

        if (!g_value_get_string (value))
          set_cell_bg_color (cell, NULL);
        else if (gdk_rgba_parse (&rgba, g_value_get_string (value)))
          set_cell_bg_color (cell, &rgba);
        else
          g_warning ("Don't know color '%s'", g_value_get_string (value));

        g_object_notify (object, "cell-background");
      }
      break;
    case PROP_CELL_BACKGROUND_RGBA:
      set_cell_bg_color (cell, g_value_get_boxed (value));
      break;
    case PROP_CELL_BACKGROUND_SET:
      if (priv->cell_background_set != g_value_get_boolean (value))
        {
          priv->cell_background_set = g_value_get_boolean (value);
          g_object_notify (object, "cell-background-set");
        }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
set_cell_bg_color (BobguiCellRenderer *cell,
                   GdkRGBA         *rgba)
{
  BobguiCellRendererPrivate *priv = cell->priv;

  if (rgba)
    {
      if (!priv->cell_background_set)
        {
          priv->cell_background_set = TRUE;
          g_object_notify (G_OBJECT (cell), "cell-background-set");
        }

      priv->cell_background = *rgba;
    }
  else
    {
      if (priv->cell_background_set)
        {
	  priv->cell_background_set = FALSE;
	  g_object_notify (G_OBJECT (cell), "cell-background-set");
	}
    }
  g_object_notify (G_OBJECT (cell), "cell-background-rgba");
}

/**
 * bobgui_cell_renderer_snapshot:
 * @cell: a `BobguiCellRenderer`
 * @snapshot: a `BobguiSnapshot` to draw to
 * @widget: the widget owning @window
 * @background_area: entire cell area (including tree expanders and maybe
 *    padding on the sides)
 * @cell_area: area normally rendered by a cell renderer
 * @flags: flags that affect rendering
 *
 * Invokes the virtual render function of the `BobguiCellRenderer`. The three
 * passed-in rectangles are areas in @cr. Most renderers will draw within
 * @cell_area; the xalign, yalign, xpad, and ypad fields of the `BobguiCellRenderer`
 * should be honored with respect to @cell_area. @background_area includes the
 * blank space around the cell, and also the area containing the tree expander;
 * so the @background_area rectangles for all cells tile to cover the entire
 * @window.
 *
 * Deprecated: 4.10
 **/
void
bobgui_cell_renderer_snapshot (BobguiCellRenderer      *cell,
                            BobguiSnapshot          *snapshot,
                            BobguiWidget            *widget,
                            const GdkRectangle   *background_area,
                            const GdkRectangle   *cell_area,
                            BobguiCellRendererState  flags)
{
  gboolean selected = FALSE;
  BobguiCellRendererPrivate *priv = cell->priv;
  BobguiStyleContext *context;
  BobguiStateFlags state;

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));
  g_return_if_fail (BOBGUI_CELL_RENDERER_GET_CLASS (cell)->snapshot != NULL);
  g_return_if_fail (snapshot != NULL);

  selected = (flags & BOBGUI_CELL_RENDERER_SELECTED) == BOBGUI_CELL_RENDERER_SELECTED;

  bobgui_snapshot_push_debug (snapshot, "%s", G_OBJECT_TYPE_NAME (cell));

  if (priv->cell_background_set && !selected)
    {
      bobgui_snapshot_append_color (snapshot,
                                 &priv->cell_background,
                                 &GRAPHENE_RECT_INIT (
                                     background_area->x, background_area->y,
                                     background_area->width, background_area->height
                                 ));
    }

  bobgui_snapshot_push_clip (snapshot,
                          &GRAPHENE_RECT_INIT (
                              background_area->x, background_area->y,
                              background_area->width, background_area->height
                          ));

  context = bobgui_widget_get_style_context (widget);

  bobgui_style_context_save (context);
  bobgui_style_context_add_class (context, "cell");

  state = bobgui_cell_renderer_get_state (cell, widget, flags);
  bobgui_style_context_set_state (context, state);

  BOBGUI_CELL_RENDERER_GET_CLASS (cell)->snapshot (cell,
                                                snapshot,
                                                widget,
                                                background_area,
                                                cell_area,
                                                flags);
  bobgui_style_context_restore (context);
  bobgui_snapshot_pop (snapshot);
  bobgui_snapshot_pop (snapshot);
}

/**
 * bobgui_cell_renderer_activate:
 * @cell: a `BobguiCellRenderer`
 * @event: a `GdkEvent`
 * @widget: widget that received the event
 * @path: widget-dependent string representation of the event location;
 *    e.g. for `BobguiTreeView`, a string representation of `BobguiTreePath`
 * @background_area: background area as passed to bobgui_cell_renderer_render()
 * @cell_area: cell area as passed to bobgui_cell_renderer_render()
 * @flags: render flags
 *
 * Passes an activate event to the cell renderer for possible processing.
 * Some cell renderers may use events; for example, `BobguiCellRendererToggle`
 * toggles when it gets a mouse click.
 *
 * Returns: %TRUE if the event was consumed/handled
 *
 * Deprecated: 4.10
 **/
gboolean
bobgui_cell_renderer_activate (BobguiCellRenderer      *cell,
			    GdkEvent             *event,
			    BobguiWidget            *widget,
			    const char           *path,
			    const GdkRectangle   *background_area,
			    const GdkRectangle   *cell_area,
			    BobguiCellRendererState  flags)
{
  BobguiCellRendererPrivate *priv;

  g_return_val_if_fail (BOBGUI_IS_CELL_RENDERER (cell), FALSE);

  priv = cell->priv;

  if (priv->mode != BOBGUI_CELL_RENDERER_MODE_ACTIVATABLE)
    return FALSE;

  if (BOBGUI_CELL_RENDERER_GET_CLASS (cell)->activate == NULL)
    return FALSE;

  return BOBGUI_CELL_RENDERER_GET_CLASS (cell)->activate (cell,
						       event,
						       widget,
						       path,
						       (GdkRectangle *) background_area,
						       (GdkRectangle *) cell_area,
						       flags);
}

/**
 * bobgui_cell_renderer_start_editing:
 * @cell: a `BobguiCellRenderer`
 * @event: (nullable): a `GdkEvent`
 * @widget: widget that received the event
 * @path: widget-dependent string representation of the event location;
 *    e.g. for `BobguiTreeView`, a string representation of `BobguiTreePath`
 * @background_area: background area as passed to bobgui_cell_renderer_render()
 * @cell_area: cell area as passed to bobgui_cell_renderer_render()
 * @flags: render flags
 *
 * Starts editing the contents of this @cell, through a new `BobguiCellEditable`
 * widget created by the `BobguiCellRenderer`Class.start_editing virtual function.
 *
 * Returns: (nullable) (transfer none): A new `BobguiCellEditable` for editing this
 *   @cell, or %NULL if editing is not possible
 *
 * Deprecated: 4.10
 **/
BobguiCellEditable *
bobgui_cell_renderer_start_editing (BobguiCellRenderer      *cell,
				 GdkEvent             *event,
				 BobguiWidget            *widget,
				 const char           *path,
				 const GdkRectangle   *background_area,
				 const GdkRectangle   *cell_area,
				 BobguiCellRendererState  flags)

{
  BobguiCellRendererPrivate *priv;
  BobguiCellEditable *editable;

  g_return_val_if_fail (BOBGUI_IS_CELL_RENDERER (cell), NULL);

  priv = cell->priv;

  if (priv->mode != BOBGUI_CELL_RENDERER_MODE_EDITABLE)
    return NULL;

  if (BOBGUI_CELL_RENDERER_GET_CLASS (cell)->start_editing == NULL)
    return NULL;

  editable = BOBGUI_CELL_RENDERER_GET_CLASS (cell)->start_editing (cell,
								event,
								widget,
								path,
								(GdkRectangle *) background_area,
								(GdkRectangle *) cell_area,
								flags);
  if (editable == NULL)
    return NULL;

  bobgui_widget_add_css_class (BOBGUI_WIDGET (editable), "cell");

  g_signal_emit (cell,
		 cell_renderer_signals[EDITING_STARTED], 0,
		 editable, path);

  priv->editing = TRUE;

  return editable;
}

/**
 * bobgui_cell_renderer_set_fixed_size:
 * @cell: A `BobguiCellRenderer`
 * @width: the width of the cell renderer, or -1
 * @height: the height of the cell renderer, or -1
 *
 * Sets the renderer size to be explicit, independent of the properties set.
 *
 * Deprecated: 4.10
 **/
void
bobgui_cell_renderer_set_fixed_size (BobguiCellRenderer *cell,
				  int              width,
				  int              height)
{
  BobguiCellRendererPrivate *priv;

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));
  g_return_if_fail (width >= -1 && height >= -1);

  priv = cell->priv;

  if ((width != priv->width) || (height != priv->height))
    {
      g_object_freeze_notify (G_OBJECT (cell));

      if (width != priv->width)
        {
          priv->width = width;
          g_object_notify (G_OBJECT (cell), "width");
        }

      if (height != priv->height)
        {
          priv->height = height;
          g_object_notify (G_OBJECT (cell), "height");
        }

      g_object_thaw_notify (G_OBJECT (cell));
    }
}

/**
 * bobgui_cell_renderer_get_fixed_size:
 * @cell: A `BobguiCellRenderer`
 * @width: (out) (optional): location to fill in with the fixed width of the cell
 * @height: (out) (optional): location to fill in with the fixed height of the cell
 *
 * Fills in @width and @height with the appropriate size of @cell.
 *
 * Deprecated: 4.10
 */
void
bobgui_cell_renderer_get_fixed_size (BobguiCellRenderer *cell,
				  int             *width,
				  int             *height)
{
  BobguiCellRendererPrivate *priv;

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));

  priv = cell->priv;

  if (width)
    *width = priv->width;
  if (height)
    *height = priv->height;
}

/**
 * bobgui_cell_renderer_set_alignment:
 * @cell: A `BobguiCellRenderer`
 * @xalign: the x alignment of the cell renderer
 * @yalign: the y alignment of the cell renderer
 *
 * Sets the renderer’s alignment within its available space.
 *
 * Deprecated: 4.10
 **/
void
bobgui_cell_renderer_set_alignment (BobguiCellRenderer *cell,
                                 float            xalign,
                                 float            yalign)
{
  BobguiCellRendererPrivate *priv;

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));
  g_return_if_fail (xalign >= 0.0 && xalign <= 1.0);
  g_return_if_fail (yalign >= 0.0 && yalign <= 1.0);

  priv = cell->priv;

  if ((xalign != priv->xalign) || (yalign != priv->yalign))
    {
      g_object_freeze_notify (G_OBJECT (cell));

      if (xalign != priv->xalign)
        {
          priv->xalign = xalign;
          g_object_notify (G_OBJECT (cell), "xalign");
        }

      if (yalign != priv->yalign)
        {
          priv->yalign = yalign;
          g_object_notify (G_OBJECT (cell), "yalign");
        }

      g_object_thaw_notify (G_OBJECT (cell));
    }
}

/**
 * bobgui_cell_renderer_get_alignment:
 * @cell: A `BobguiCellRenderer`
 * @xalign: (out) (optional): location to fill in with the x alignment of the cell
 * @yalign: (out) (optional): location to fill in with the y alignment of the cell
 *
 * Fills in @xalign and @yalign with the appropriate values of @cell.
 *
 * Deprecated: 4.10
 */
void
bobgui_cell_renderer_get_alignment (BobguiCellRenderer *cell,
                                 float           *xalign,
                                 float           *yalign)
{
  BobguiCellRendererPrivate *priv;

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));

  priv = cell->priv;

  if (xalign)
    *xalign = priv->xalign;
  if (yalign)
    *yalign = priv->yalign;
}

/**
 * bobgui_cell_renderer_set_padding:
 * @cell: A `BobguiCellRenderer`
 * @xpad: the x padding of the cell renderer
 * @ypad: the y padding of the cell renderer
 *
 * Sets the renderer’s padding.
 *
 * Deprecated: 4.10
 **/
void
bobgui_cell_renderer_set_padding (BobguiCellRenderer *cell,
                               int              xpad,
                               int              ypad)
{
  BobguiCellRendererPrivate *priv;

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));
  g_return_if_fail (xpad >= 0 && ypad >= 0);

  priv = cell->priv;

  if ((xpad != priv->xpad) || (ypad != priv->ypad))
    {
      g_object_freeze_notify (G_OBJECT (cell));

      if (xpad != priv->xpad)
        {
          priv->xpad = xpad;
          g_object_notify (G_OBJECT (cell), "xpad");
        }

      if (ypad != priv->ypad)
        {
          priv->ypad = ypad;
          g_object_notify (G_OBJECT (cell), "ypad");
        }

      g_object_thaw_notify (G_OBJECT (cell));
    }
}

/**
 * bobgui_cell_renderer_get_padding:
 * @cell: A `BobguiCellRenderer`
 * @xpad: (out) (optional): location to fill in with the x padding of the cell
 * @ypad: (out) (optional): location to fill in with the y padding of the cell
 *
 * Fills in @xpad and @ypad with the appropriate values of @cell.
 *
 * Deprecated: 4.10
 */
void
bobgui_cell_renderer_get_padding (BobguiCellRenderer *cell,
                               int             *xpad,
                               int             *ypad)
{
  BobguiCellRendererPrivate *priv;

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));

  priv = cell->priv;

  if (xpad)
    *xpad = priv->xpad;
  if (ypad)
    *ypad = priv->ypad;
}

/**
 * bobgui_cell_renderer_set_visible:
 * @cell: A `BobguiCellRenderer`
 * @visible: the visibility of the cell
 *
 * Sets the cell renderer’s visibility.
 *
 * Deprecated: 4.10
 **/
void
bobgui_cell_renderer_set_visible (BobguiCellRenderer *cell,
                               gboolean         visible)
{
  BobguiCellRendererPrivate *priv;

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));

  priv = cell->priv;

  if (priv->visible != visible)
    {
      priv->visible = visible ? TRUE : FALSE;
      g_object_notify (G_OBJECT (cell), "visible");
    }
}

/**
 * bobgui_cell_renderer_get_visible:
 * @cell: A `BobguiCellRenderer`
 *
 * Returns the cell renderer’s visibility.
 *
 * Returns: %TRUE if the cell renderer is visible
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_cell_renderer_get_visible (BobguiCellRenderer *cell)
{
  g_return_val_if_fail (BOBGUI_IS_CELL_RENDERER (cell), FALSE);

  return cell->priv->visible;
}

/**
 * bobgui_cell_renderer_set_sensitive:
 * @cell: A `BobguiCellRenderer`
 * @sensitive: the sensitivity of the cell
 *
 * Sets the cell renderer’s sensitivity.
 *
 * Deprecated: 4.10
 **/
void
bobgui_cell_renderer_set_sensitive (BobguiCellRenderer *cell,
                                 gboolean         sensitive)
{
  BobguiCellRendererPrivate *priv;

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));

  priv = cell->priv;

  if (priv->sensitive != sensitive)
    {
      priv->sensitive = sensitive ? TRUE : FALSE;
      g_object_notify (G_OBJECT (cell), "sensitive");
    }
}

/**
 * bobgui_cell_renderer_get_sensitive:
 * @cell: A `BobguiCellRenderer`
 *
 * Returns the cell renderer’s sensitivity.
 *
 * Returns: %TRUE if the cell renderer is sensitive
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_cell_renderer_get_sensitive (BobguiCellRenderer *cell)
{
  g_return_val_if_fail (BOBGUI_IS_CELL_RENDERER (cell), FALSE);

  return cell->priv->sensitive;
}


/**
 * bobgui_cell_renderer_is_activatable:
 * @cell: A `BobguiCellRenderer`
 *
 * Checks whether the cell renderer can do something when activated.
 *
 * Returns: %TRUE if the cell renderer can do anything when activated
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_cell_renderer_is_activatable (BobguiCellRenderer *cell)
{
  BobguiCellRendererPrivate *priv;

  g_return_val_if_fail (BOBGUI_IS_CELL_RENDERER (cell), FALSE);

  priv = cell->priv;

  return (priv->visible &&
          (priv->mode == BOBGUI_CELL_RENDERER_MODE_EDITABLE ||
           priv->mode == BOBGUI_CELL_RENDERER_MODE_ACTIVATABLE));
}


/**
 * bobgui_cell_renderer_stop_editing:
 * @cell: A `BobguiCellRenderer`
 * @canceled: %TRUE if the editing has been canceled
 *
 * Informs the cell renderer that the editing is stopped.
 * If @canceled is %TRUE, the cell renderer will emit the
 * `BobguiCellRenderer`::editing-canceled signal.
 *
 * This function should be called by cell renderer implementations
 * in response to the `BobguiCellEditable::editing-done` signal of
 * `BobguiCellEditable`.
 *
 * Deprecated: 4.10
 **/
void
bobgui_cell_renderer_stop_editing (BobguiCellRenderer *cell,
				gboolean         canceled)
{
  BobguiCellRendererPrivate *priv;

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));

  priv = cell->priv;

  if (priv->editing)
    {
      priv->editing = FALSE;
      if (canceled)
	g_signal_emit (cell, cell_renderer_signals[EDITING_CANCELED], 0);
    }
}

static void
bobgui_cell_renderer_real_get_preferred_size (BobguiCellRenderer   *cell,
                                           BobguiWidget         *widget,
                                           BobguiOrientation     orientation,
                                           int               *minimum_size,
                                           int               *natural_size)
{
  BobguiRequisition min_req;

  min_req.width = 0;
  min_req.height = 0;

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      if (minimum_size)
	*minimum_size = min_req.width;

      if (natural_size)
	*natural_size = min_req.width;
    }
  else
    {
      if (minimum_size)
	*minimum_size = min_req.height;

      if (natural_size)
	*natural_size = min_req.height;
    }
}

static BobguiSizeRequestMode
bobgui_cell_renderer_real_get_request_mode (BobguiCellRenderer *cell)
{
  /* By default cell renderers are height-for-width. */
  return BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
bobgui_cell_renderer_real_get_preferred_width (BobguiCellRenderer *cell,
                                            BobguiWidget       *widget,
                                            int             *minimum_size,
                                            int             *natural_size)
{
  bobgui_cell_renderer_real_get_preferred_size (cell, widget, BOBGUI_ORIENTATION_HORIZONTAL,
                                             minimum_size, natural_size);
}

static void
bobgui_cell_renderer_real_get_preferred_height (BobguiCellRenderer *cell,
                                             BobguiWidget       *widget,
                                             int             *minimum_size,
                                             int             *natural_size)
{
  bobgui_cell_renderer_real_get_preferred_size (cell, widget, BOBGUI_ORIENTATION_VERTICAL,
                                             minimum_size, natural_size);
}


static void
bobgui_cell_renderer_real_get_preferred_height_for_width (BobguiCellRenderer *cell,
                                                       BobguiWidget       *widget,
                                                       int              width,
                                                       int             *minimum_height,
                                                       int             *natural_height)
{
  bobgui_cell_renderer_get_preferred_height (cell, widget, minimum_height, natural_height);
}

static void
bobgui_cell_renderer_real_get_preferred_width_for_height (BobguiCellRenderer *cell,
                                                       BobguiWidget       *widget,
                                                       int              height,
                                                       int             *minimum_width,
                                                       int             *natural_width)
{
  bobgui_cell_renderer_get_preferred_width (cell, widget, minimum_width, natural_width);
}


/* Default implementation assumes that a cell renderer will never use more
 * space than its natural size (this is fine for toggles and pixbufs etc
 * but needs to be overridden from wrapping/ellipsizing text renderers) */
static void
bobgui_cell_renderer_real_get_aligned_area (BobguiCellRenderer         *cell,
					 BobguiWidget               *widget,
					 BobguiCellRendererState     flags,
					 const GdkRectangle      *cell_area,
					 GdkRectangle            *aligned_area)
{
  int opposite_size, x_offset, y_offset;
  int natural_size;

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));
  g_return_if_fail (cell_area != NULL);
  g_return_if_fail (aligned_area != NULL);

  *aligned_area = *cell_area;

  /* Trim up the aligned size */
  if (bobgui_cell_renderer_get_request_mode (cell) == BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH)
    {
      bobgui_cell_renderer_get_preferred_width (cell, widget,
					     NULL, &natural_size);

      aligned_area->width = MIN (aligned_area->width, natural_size);

      bobgui_cell_renderer_get_preferred_height_for_width (cell, widget,
							aligned_area->width,
							NULL, &opposite_size);

      aligned_area->height = MIN (opposite_size, aligned_area->height);
    }
  else
    {
      bobgui_cell_renderer_get_preferred_height (cell, widget,
					      NULL, &natural_size);

      aligned_area->height = MIN (aligned_area->width, natural_size);

      bobgui_cell_renderer_get_preferred_width_for_height (cell, widget,
							aligned_area->height,
							NULL, &opposite_size);

      aligned_area->width = MIN (opposite_size, aligned_area->width);
    }

  /* offset the cell position */
  _bobgui_cell_renderer_calc_offset (cell, cell_area,
				  bobgui_widget_get_direction (widget),
				  aligned_area->width,
				  aligned_area->height,
				  &x_offset, &y_offset);

  aligned_area->x += x_offset;
  aligned_area->y += y_offset;
}


/* An internal convenience function for some containers to peek at the
 * cell alignment in a target allocation (used to draw focus and align
 * cells in the icon view).
 *
 * Note this is only a trivial “align * (allocation - request)” operation.
 */
void
_bobgui_cell_renderer_calc_offset    (BobguiCellRenderer      *cell,
				   const GdkRectangle   *cell_area,
				   BobguiTextDirection      direction,
				   int                   width,
				   int                   height,
				   int                  *x_offset,
				   int                  *y_offset)
{
  BobguiCellRendererPrivate *priv;

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));
  g_return_if_fail (cell_area != NULL);
  g_return_if_fail (x_offset || y_offset);

  priv = cell->priv;

  if (x_offset)
    {
      *x_offset = (((direction == BOBGUI_TEXT_DIR_RTL) ?
		    (1.0 - priv->xalign) : priv->xalign) *
		   (cell_area->width - width));
      *x_offset = MAX (*x_offset, 0);
    }
  if (y_offset)
    {
      *y_offset = (priv->yalign *
		   (cell_area->height - height));
      *y_offset = MAX (*y_offset, 0);
    }
}

/**
 * bobgui_cell_renderer_get_request_mode:
 * @cell: a `BobguiCellRenderer` instance
 *
 * Gets whether the cell renderer prefers a height-for-width layout
 * or a width-for-height layout.
 *
 * Returns: The `BobguiSizeRequestMode` preferred by this renderer.
 *
 * Deprecated: 4.10
 */
BobguiSizeRequestMode
bobgui_cell_renderer_get_request_mode (BobguiCellRenderer *cell)
{
  g_return_val_if_fail (BOBGUI_IS_CELL_RENDERER (cell), FALSE);

  return BOBGUI_CELL_RENDERER_GET_CLASS (cell)->get_request_mode (cell);
}

/**
 * bobgui_cell_renderer_get_preferred_width:
 * @cell: a `BobguiCellRenderer` instance
 * @widget: the `BobguiWidget` this cell will be rendering to
 * @minimum_size: (out) (optional): location to store the minimum size
 * @natural_size: (out) (optional): location to store the natural size
 *
 * Retrieves a renderer’s natural size when rendered to @widget.
 *
 * Deprecated: 4.10
 */
void
bobgui_cell_renderer_get_preferred_width (BobguiCellRenderer *cell,
                                       BobguiWidget       *widget,
                                       int             *minimum_size,
                                       int             *natural_size)
{
  BobguiCellRendererClass *klass;
  int width;

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));
  g_return_if_fail (NULL != minimum_size || NULL != natural_size);

  bobgui_cell_renderer_get_fixed_size (BOBGUI_CELL_RENDERER (cell), &width, NULL);

  if (width < 0)
    {
      klass = BOBGUI_CELL_RENDERER_GET_CLASS (cell);
      klass->get_preferred_width (cell, widget, minimum_size, natural_size);
    }
  else
    {
      if (minimum_size)
	*minimum_size = width;
      if (natural_size)
	*natural_size = width;
    }

#if DEBUG_CELL_SIZE_REQUEST
  g_message ("%s returning minimum width: %d and natural width: %d",
	     G_OBJECT_TYPE_NAME (cell),
	     minimum_size ? *minimum_size : 20000,
	     natural_size ? *natural_size : 20000);
#endif
}


/**
 * bobgui_cell_renderer_get_preferred_height:
 * @cell: a `BobguiCellRenderer` instance
 * @widget: the `BobguiWidget` this cell will be rendering to
 * @minimum_size: (out) (optional): location to store the minimum size
 * @natural_size: (out) (optional): location to store the natural size
 *
 * Retrieves a renderer’s natural size when rendered to @widget.
 *
 * Deprecated: 4.10
 */
void
bobgui_cell_renderer_get_preferred_height (BobguiCellRenderer *cell,
                                        BobguiWidget       *widget,
                                        int             *minimum_size,
                                        int             *natural_size)
{
  BobguiCellRendererClass *klass;
  int height;

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));
  g_return_if_fail (NULL != minimum_size || NULL != natural_size);

  bobgui_cell_renderer_get_fixed_size (BOBGUI_CELL_RENDERER (cell), NULL, &height);

  if (height < 0)
    {
      klass = BOBGUI_CELL_RENDERER_GET_CLASS (cell);
      klass->get_preferred_height (cell, widget, minimum_size, natural_size);
    }
  else
    {
      if (minimum_size)
	*minimum_size = height;
      if (natural_size)
	*natural_size = height;
    }

#if DEBUG_CELL_SIZE_REQUEST
  g_message ("%s returning minimum height: %d and natural height: %d",
	     G_OBJECT_TYPE_NAME (cell),
	     minimum_size ? *minimum_size : 20000,
	     natural_size ? *natural_size : 20000);
#endif
}


/**
 * bobgui_cell_renderer_get_preferred_width_for_height:
 * @cell: a `BobguiCellRenderer` instance
 * @widget: the `BobguiWidget` this cell will be rendering to
 * @height: the size which is available for allocation
 * @minimum_width: (out) (optional): location for storing the minimum size
 * @natural_width: (out) (optional): location for storing the preferred size
 *
 * Retrieves a cell renderers’s minimum and natural width if it were rendered to
 * @widget with the specified @height.
 *
 * Deprecated: 4.10
 */
void
bobgui_cell_renderer_get_preferred_width_for_height (BobguiCellRenderer *cell,
                                                  BobguiWidget       *widget,
                                                  int              height,
                                                  int             *minimum_width,
                                                  int             *natural_width)
{
  BobguiCellRendererClass *klass;
  int width;

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));
  g_return_if_fail (NULL != minimum_width || NULL != natural_width);

  bobgui_cell_renderer_get_fixed_size (BOBGUI_CELL_RENDERER (cell), &width, NULL);

  if (width < 0)
    {
      klass = BOBGUI_CELL_RENDERER_GET_CLASS (cell);
      klass->get_preferred_width_for_height (cell, widget, height, minimum_width, natural_width);
    }
  else
    {
      if (minimum_width)
	*minimum_width = width;
      if (natural_width)
	*natural_width = width;
    }

#if DEBUG_CELL_SIZE_REQUEST
  g_message ("%s width for height: %d is minimum %d and natural: %d",
	     G_OBJECT_TYPE_NAME (cell), height,
	     minimum_width ? *minimum_width : 20000,
	     natural_width ? *natural_width : 20000);
#endif
}

/**
 * bobgui_cell_renderer_get_preferred_height_for_width:
 * @cell: a `BobguiCellRenderer` instance
 * @widget: the `BobguiWidget` this cell will be rendering to
 * @width: the size which is available for allocation
 * @minimum_height: (out) (optional): location for storing the minimum size
 * @natural_height: (out) (optional): location for storing the preferred size
 *
 * Retrieves a cell renderers’s minimum and natural height if it were rendered to
 * @widget with the specified @width.
 *
 * Deprecated: 4.10
 */
void
bobgui_cell_renderer_get_preferred_height_for_width (BobguiCellRenderer *cell,
                                                  BobguiWidget       *widget,
                                                  int              width,
                                                  int             *minimum_height,
                                                  int             *natural_height)
{
  BobguiCellRendererClass *klass;
  int height;

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));
  g_return_if_fail (NULL != minimum_height || NULL != natural_height);

  bobgui_cell_renderer_get_fixed_size (BOBGUI_CELL_RENDERER (cell), NULL, &height);

  if (height < 0)
    {
      klass = BOBGUI_CELL_RENDERER_GET_CLASS (cell);
      klass->get_preferred_height_for_width (cell, widget, width, minimum_height, natural_height);
    }
  else
    {
      if (minimum_height)
	*minimum_height = height;
      if (natural_height)
	*natural_height = height;
    }

#if DEBUG_CELL_SIZE_REQUEST
  g_message ("%s height for width: %d is minimum %d and natural: %d",
	     G_OBJECT_TYPE_NAME (cell), width,
	     minimum_height ? *minimum_height : 20000,
	     natural_height ? *natural_height : 20000);
#endif
}

/**
 * bobgui_cell_renderer_get_preferred_size:
 * @cell: a `BobguiCellRenderer` instance
 * @widget: the `BobguiWidget` this cell will be rendering to
 * @minimum_size: (out) (optional): location for storing the minimum size
 * @natural_size: (out) (optional): location for storing the natural size
 *
 * Retrieves the minimum and natural size of a cell taking
 * into account the widget’s preference for height-for-width management.
 *
 * Deprecated: 4.10
 */
void
bobgui_cell_renderer_get_preferred_size (BobguiCellRenderer *cell,
                                      BobguiWidget       *widget,
                                      BobguiRequisition  *minimum_size,
                                      BobguiRequisition  *natural_size)
{
  int min_width, nat_width;
  int min_height, nat_height;

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));

  if (bobgui_cell_renderer_get_request_mode (cell) == BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH)
    {
      bobgui_cell_renderer_get_preferred_width (cell, widget, &min_width, &nat_width);

      if (minimum_size)
	{
	  minimum_size->width = min_width;
	  bobgui_cell_renderer_get_preferred_height_for_width (cell, widget, min_width,
                                                            &minimum_size->height, NULL);
	}

      if (natural_size)
	{
	  natural_size->width = nat_width;
	  bobgui_cell_renderer_get_preferred_height_for_width (cell, widget, nat_width,
                                                            NULL, &natural_size->height);
	}
    }
  else /* BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT */
    {
      bobgui_cell_renderer_get_preferred_height (cell, widget, &min_height, &nat_height);

      if (minimum_size)
	{
	  minimum_size->height = min_height;
	  bobgui_cell_renderer_get_preferred_width_for_height (cell, widget, min_height,
                                                            &minimum_size->width, NULL);
	}

      if (natural_size)
	{
	  natural_size->height = nat_height;
	  bobgui_cell_renderer_get_preferred_width_for_height (cell, widget, nat_height,
                                                            NULL, &natural_size->width);
	}
    }
}

/**
 * bobgui_cell_renderer_get_aligned_area:
 * @cell: a `BobguiCellRenderer` instance
 * @widget: the `BobguiWidget` this cell will be rendering to
 * @flags: render flags
 * @cell_area: cell area which would be passed to bobgui_cell_renderer_render()
 * @aligned_area: (out): the return location for the space inside @cell_area
 *                that would actually be used to render.
 *
 * Gets the aligned area used by @cell inside @cell_area. Used for finding
 * the appropriate edit and focus rectangle.
 *
 * Deprecated: 4.10
 */
void
bobgui_cell_renderer_get_aligned_area (BobguiCellRenderer      *cell,
				    BobguiWidget            *widget,
				    BobguiCellRendererState  flags,
				    const GdkRectangle   *cell_area,
				    GdkRectangle         *aligned_area)
{
  BobguiCellRendererClass *klass;

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));
  g_return_if_fail (BOBGUI_IS_WIDGET (widget));
  g_return_if_fail (cell_area != NULL);
  g_return_if_fail (aligned_area != NULL);

  klass = BOBGUI_CELL_RENDERER_GET_CLASS (cell);
  klass->get_aligned_area (cell, widget, flags, cell_area, aligned_area);

  g_assert (aligned_area->x >= cell_area->x && aligned_area->x <= cell_area->x + cell_area->width);
  g_assert (aligned_area->y >= cell_area->y && aligned_area->y <= cell_area->y + cell_area->height);
  g_assert ((aligned_area->x - cell_area->x) + aligned_area->width <= cell_area->width);
  g_assert ((aligned_area->y - cell_area->y) + aligned_area->height <= cell_area->height);
}

/**
 * bobgui_cell_renderer_get_state:
 * @cell: (nullable): a `BobguiCellRenderer`
 * @widget: (nullable): a `BobguiWidget`
 * @cell_state: cell renderer state
 *
 * Translates the cell renderer state to `BobguiStateFlags`,
 * based on the cell renderer and widget sensitivity, and
 * the given `BobguiCellRenderer`State.
 *
 * Returns: the widget state flags applying to @cell
 *
 * Deprecated: 4.10
 **/
BobguiStateFlags
bobgui_cell_renderer_get_state (BobguiCellRenderer      *cell,
			     BobguiWidget            *widget,
			     BobguiCellRendererState  cell_state)
{
  BobguiStateFlags state = 0;

  g_return_val_if_fail (!cell || BOBGUI_IS_CELL_RENDERER (cell), 0);
  g_return_val_if_fail (!widget || BOBGUI_IS_WIDGET (widget), 0);

  if (widget)
    state |= bobgui_widget_get_state_flags (widget);

  state &= ~(BOBGUI_STATE_FLAG_FOCUSED | BOBGUI_STATE_FLAG_PRELIGHT | BOBGUI_STATE_FLAG_SELECTED | BOBGUI_STATE_FLAG_DROP_ACTIVE);

  if ((state & BOBGUI_STATE_FLAG_INSENSITIVE) != 0 ||
      (cell && !bobgui_cell_renderer_get_sensitive (cell)) ||
      (cell_state & BOBGUI_CELL_RENDERER_INSENSITIVE) != 0)
    {
      state |= BOBGUI_STATE_FLAG_INSENSITIVE;
    }
  else
    {
      if ((widget && bobgui_widget_has_focus (widget)) &&
          (cell_state & BOBGUI_CELL_RENDERER_FOCUSED) != 0)
        state |= BOBGUI_STATE_FLAG_FOCUSED;

      if ((cell_state & BOBGUI_CELL_RENDERER_PRELIT) != 0)
        state |= BOBGUI_STATE_FLAG_PRELIGHT;
    }

  if ((cell_state & BOBGUI_CELL_RENDERER_SELECTED) != 0)
    state |= BOBGUI_STATE_FLAG_SELECTED;

  return state;
}

/**
 * bobgui_cell_renderer_set_is_expander:
 * @cell: a `BobguiCellRenderer`
 * @is_expander: whether @cell is an expander
 *
 * Sets whether the given `BobguiCellRenderer` is an expander.
 *
 * Deprecated: 4.10
 */
void
bobgui_cell_renderer_set_is_expander (BobguiCellRenderer *cell,
                                   gboolean         is_expander)
{
  BobguiCellRendererPrivate *priv = bobgui_cell_renderer_get_instance_private (cell);

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));

  is_expander = !!is_expander;

  if (is_expander != priv->is_expander)
    {
      priv->is_expander = is_expander;

      g_object_notify (G_OBJECT (cell), "is-expander");
    }
}

/**
 * bobgui_cell_renderer_get_is_expander:
 * @cell: a `BobguiCellRenderer`
 *
 * Checks whether the given `BobguiCellRenderer` is an expander.
 *
 * Returns: %TRUE if @cell is an expander, and %FALSE otherwise
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_cell_renderer_get_is_expander (BobguiCellRenderer *cell)
{
  BobguiCellRendererPrivate *priv = bobgui_cell_renderer_get_instance_private (cell);

  g_return_val_if_fail (BOBGUI_IS_CELL_RENDERER (cell), FALSE);

  return priv->is_expander;
}

/**
 * bobgui_cell_renderer_set_is_expanded:
 * @cell: a `BobguiCellRenderer`
 * @is_expanded: whether @cell should be expanded
 *
 * Sets whether the given `BobguiCellRenderer` is expanded.
 *
 * Deprecated: 4.10
 */
void
bobgui_cell_renderer_set_is_expanded (BobguiCellRenderer *cell,
                                   gboolean         is_expanded)
{
  BobguiCellRendererPrivate *priv = bobgui_cell_renderer_get_instance_private (cell);

  g_return_if_fail (BOBGUI_IS_CELL_RENDERER (cell));

  is_expanded = !!is_expanded;

  if (is_expanded != priv->is_expanded)
    {
      priv->is_expanded = is_expanded;

      g_object_notify (G_OBJECT (cell), "is-expanded");
    }
}

/**
 * bobgui_cell_renderer_get_is_expanded:
 * @cell: a `BobguiCellRenderer`
 *
 * Checks whether the given `BobguiCellRenderer` is expanded.
 *
 * Returns: %TRUE if the cell renderer is expanded
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_cell_renderer_get_is_expanded (BobguiCellRenderer *cell)
{
  BobguiCellRendererPrivate *priv = bobgui_cell_renderer_get_instance_private (cell);

  g_return_val_if_fail (BOBGUI_IS_CELL_RENDERER (cell), FALSE);

  return priv->is_expanded;
}
