/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2012 Red Hat, Inc.
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

#include "bobguicoloreditorprivate.h"

#include <glib/gi18n-lib.h>

#include "deprecated/bobguicolorchooserprivate.h"
#include "bobguicolorplaneprivate.h"
#include "bobguicolorscaleprivate.h"
#include "bobguicolorswatchprivate.h"
#include "bobguicolorchooserwidgetprivate.h"
#include "bobguicolorutils.h"
#include "bobguicolorpickerprivate.h"
#include "bobguigrid.h"
#include "bobguibutton.h"
#include "bobguiorientable.h"
#include "bobguientry.h"
#include "bobguioverlay.h"
#include "bobguiadjustment.h"
#include "bobguilabel.h"
#include "bobguispinbutton.h"
#include "bobguieventcontrollerkey.h"
#include "bobguiroot.h"

#include <math.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef struct _BobguiColorEditorClass BobguiColorEditorClass;

struct _BobguiColorEditor
{
  BobguiBox parent_instance;

  BobguiWidget *overlay;
  BobguiWidget *grid;
  BobguiWidget *swatch;
  BobguiWidget *entry;
  BobguiWidget *h_slider;
  BobguiWidget *h_popup;
  BobguiWidget *h_entry;
  BobguiWidget *a_slider;
  BobguiWidget *a_popup;
  BobguiWidget *a_entry;
  BobguiWidget *sv_plane;
  BobguiWidget *sv_popup;
  BobguiWidget *s_entry;
  BobguiWidget *v_entry;
  BobguiWidget *current_popup;
  BobguiWidget *popdown_focus;

  BobguiAdjustment *h_adj;
  BobguiAdjustment *s_adj;
  BobguiAdjustment *v_adj;
  BobguiAdjustment *a_adj;

  BobguiWidget *picker_button;
  BobguiColorPicker *picker;

  int popup_position;

  guint text_changed : 1;
  guint use_alpha    : 1;
};

struct _BobguiColorEditorClass
{
  BobguiBoxClass parent_class;
};

enum
{
  PROP_ZERO,
  PROP_RGBA,
  PROP_USE_ALPHA
};

static void bobgui_color_editor_iface_init (BobguiColorChooserInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiColorEditor, bobgui_color_editor, BOBGUI_TYPE_BOX,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_COLOR_CHOOSER,
                                                bobgui_color_editor_iface_init))

static guint
scale_round (double value, double scale)
{
  value = floor (value * scale + 0.5);
  value = MAX (value, 0);
  value = MIN (value, scale);
  return (guint)value;
}

static void
entry_set_rgba (BobguiColorEditor *editor,
                const GdkRGBA  *color)
{
  char *text;

  text = g_strdup_printf ("#%02X%02X%02X",
                          scale_round (color->red, 255),
                          scale_round (color->green, 255),
                          scale_round (color->blue, 255));
  bobgui_editable_set_text (BOBGUI_EDITABLE (editor->entry), text);
  editor->text_changed = FALSE;
  g_free (text);
}

static void
entry_apply (BobguiWidget      *entry,
             BobguiColorEditor *editor)
{
  GdkRGBA color;
  char *text;

  if (!editor->text_changed)
    return;

  text = bobgui_editable_get_chars (BOBGUI_EDITABLE (editor->entry), 0, -1);
  if (gdk_rgba_parse (&color, text))
    {
      color.alpha = bobgui_adjustment_get_value (editor->a_adj);
      bobgui_color_chooser_set_rgba (BOBGUI_COLOR_CHOOSER (editor), &color);
    }

  editor->text_changed = FALSE;

  g_free (text);
}

static void
entry_focus_changed (BobguiWidget      *entry,
                     GParamSpec     *pspec,
                     BobguiColorEditor *editor)
{
  if (!bobgui_widget_has_focus (entry))
    entry_apply (entry, editor);
}

static void
entry_text_changed (BobguiWidget      *entry,
                    GParamSpec     *pspec,
                    BobguiColorEditor *editor)
{
  editor->text_changed = TRUE;
}

static void
update_color (BobguiColorEditor *editor,
              const GdkRGBA  *color)
{
  char *name;
  char *text;
  name = accessible_color_name (color);
  text = g_strdup_printf (_("Color: %s"), name);
  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (editor->swatch),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL, text,
                                  -1);
  g_free (name);
  g_free (text);
  bobgui_color_swatch_set_rgba (BOBGUI_COLOR_SWATCH (editor->swatch), color);
  bobgui_color_scale_set_rgba (BOBGUI_COLOR_SCALE (editor->a_slider), color);
  entry_set_rgba (editor, color);
}

static void
hsv_changed (BobguiColorEditor *editor)
{
  GdkRGBA color;
  double h, s, v, a;

  h = bobgui_adjustment_get_value (editor->h_adj);
  s = bobgui_adjustment_get_value (editor->s_adj);
  v = bobgui_adjustment_get_value (editor->v_adj);
  a = bobgui_adjustment_get_value (editor->a_adj);

  bobgui_hsv_to_rgb (h, s, v, &color.red, &color.green, &color.blue);
  color.alpha = a;

  update_color (editor, &color);

  g_object_notify (G_OBJECT (editor), "rgba");
}

static void
dismiss_current_popup (BobguiColorEditor *editor)
{
  if (editor->current_popup)
    {
      bobgui_widget_set_visible (editor->current_popup, FALSE);
      editor->current_popup = NULL;
      editor->popup_position = 0;
      if (editor->popdown_focus)
        {
          if (bobgui_widget_is_visible (editor->popdown_focus))
            bobgui_widget_grab_focus (editor->popdown_focus);
          g_clear_object (&editor->popdown_focus);
        }
    }
}

static void
popup_edit (BobguiWidget  *widget,
            const char *action_name,
            GVariant   *parameters)
{
  BobguiColorEditor *editor = BOBGUI_COLOR_EDITOR (widget);
  BobguiWidget *popup;
  BobguiRoot *root;
  BobguiWidget *focus;
  int position;
  int s, e;
  const char *param;

  param = g_variant_get_string (parameters, NULL);

  if (strcmp (param, "sv") == 0)
    {
      popup = editor->sv_popup;
      focus = editor->s_entry;
      position = 0;
    }
  else if (strcmp (param, "h") == 0)
    {
      popup = editor->h_popup;
      focus = editor->h_entry;
      bobgui_range_get_slider_range (BOBGUI_RANGE (editor->h_slider), &s, &e);
      position = (s + e) / 2;
    }
  else if (strcmp (param, "a") == 0)
    {
      popup = editor->a_popup;
      focus = editor->a_entry;
      bobgui_range_get_slider_range (BOBGUI_RANGE (editor->a_slider), &s, &e);
      position = (s + e) / 2;
    }
  else
    {
      g_warning ("unsupported popup_edit parameter %s", param);
      popup = NULL;
      focus = NULL;
      position = 0;
    }

  if (popup == editor->current_popup)
    dismiss_current_popup (editor);
  else if (popup)
    {
      dismiss_current_popup (editor);
      root = bobgui_widget_get_root (BOBGUI_WIDGET (editor));
      g_set_object (&editor->popdown_focus, bobgui_root_get_focus (root));
      editor->current_popup = popup;
      editor->popup_position = position;
      bobgui_widget_set_visible (popup, TRUE);
      bobgui_widget_grab_focus (focus);
    }
}

static gboolean
popup_key_pressed (BobguiEventController *controller,
                   guint               keyval,
                   guint               keycode,
                   GdkModifierType     state,
                   BobguiColorEditor     *editor)
{
  if (keyval == GDK_KEY_Escape)
    {
      dismiss_current_popup (editor);
      return TRUE;
    }

  return FALSE;
}

static gboolean
get_child_position (BobguiOverlay     *overlay,
                    BobguiWidget      *widget,
                    BobguiAllocation  *allocation,
                    BobguiColorEditor *editor)
{
  BobguiRequisition req;
  graphene_point_t p;

  bobgui_widget_get_preferred_size (widget, &req, NULL);

  allocation->x = 0;
  allocation->y = 0;
  allocation->width = req.width;
  allocation->height = req.height;

  if (widget == editor->sv_popup)
    {
      if (!bobgui_widget_compute_point (editor->sv_plane,
                                     bobgui_widget_get_parent (editor->grid),
                                     &GRAPHENE_POINT_INIT (0, -6),
                                     &p))
        return FALSE;
      if (bobgui_widget_get_direction (BOBGUI_WIDGET (overlay)) == BOBGUI_TEXT_DIR_RTL)
        p.x = 0;
      else
        p.x = bobgui_widget_get_width (BOBGUI_WIDGET (overlay)) - req.width;
    }
  else if (widget == editor->h_popup)
    {
      int slider_width;

      slider_width = bobgui_widget_get_width (editor->h_slider);

      if (!bobgui_widget_compute_point (editor->h_slider,
                                     bobgui_widget_get_parent (editor->grid),
                                     bobgui_widget_get_direction (BOBGUI_WIDGET (overlay)) == BOBGUI_TEXT_DIR_RTL
                                       ? &GRAPHENE_POINT_INIT (- req.width - 6, editor->popup_position - req.height / 2)
                                       : &GRAPHENE_POINT_INIT (slider_width + 6, editor->popup_position - req.height / 2),
                                     &p))
        return FALSE;
    }
  else if (widget == editor->a_popup)
    {
      if (!bobgui_widget_compute_point (editor->a_slider,
                                     bobgui_widget_get_parent (editor->grid),
                                     &GRAPHENE_POINT_INIT (editor->popup_position - req.width / 2, - req.height - 6),
                                     &p))
        return FALSE;
    }
  else
    return FALSE;

  allocation->x = CLAMP (p.x, 0, bobgui_widget_get_width (BOBGUI_WIDGET (overlay)) - req.width);
  allocation->y = CLAMP (p.y, 0, bobgui_widget_get_height (BOBGUI_WIDGET (overlay)) - req.height);

  return TRUE;
}

static void
value_changed (BobguiAdjustment *a,
               BobguiAdjustment *as)
{
  double scale;

  scale = bobgui_adjustment_get_upper (as) / bobgui_adjustment_get_upper (a);
  g_signal_handlers_block_by_func (as, value_changed, a);
  bobgui_adjustment_set_value (as, bobgui_adjustment_get_value (a) * scale);
  g_signal_handlers_unblock_by_func (as, value_changed, a);
}

static BobguiAdjustment *
scaled_adjustment (BobguiAdjustment *a,
                   double         scale)
{
  BobguiAdjustment *as;

  as = bobgui_adjustment_new (bobgui_adjustment_get_value (a) * scale,
                           bobgui_adjustment_get_lower (a) * scale,
                           bobgui_adjustment_get_upper (a) * scale,
                           bobgui_adjustment_get_step_increment (a) * scale,
                           bobgui_adjustment_get_page_increment (a) * scale,
                           bobgui_adjustment_get_page_size (a) * scale);

  g_signal_connect (a, "value-changed", G_CALLBACK (value_changed), as);
  g_signal_connect (as, "value-changed", G_CALLBACK (value_changed), a);

  return as;
}

static void
color_picked (GObject      *source,
              GAsyncResult *res,
              gpointer      data)
{
  BobguiColorPicker *picker = BOBGUI_COLOR_PICKER (source);
  BobguiColorEditor *editor = data;
  GError *error = NULL;
  GdkRGBA *color;

  color = bobgui_color_picker_pick_finish (picker, res, &error);
  if (color == NULL)
    {
      g_error_free (error);
    }
  else
    {
      bobgui_color_chooser_set_rgba (BOBGUI_COLOR_CHOOSER (editor), color);
      gdk_rgba_free (color);
    }
}

static void
pick_color (BobguiButton      *button,
            BobguiColorEditor *editor)
{
  bobgui_color_picker_pick (editor->picker, color_picked, editor);
}

static void
bobgui_color_editor_init (BobguiColorEditor *editor)
{
  BobguiEventController *controller;

  editor->use_alpha = TRUE;

  g_type_ensure (BOBGUI_TYPE_COLOR_SCALE);
  g_type_ensure (BOBGUI_TYPE_COLOR_PLANE);
  g_type_ensure (BOBGUI_TYPE_COLOR_SWATCH);
  bobgui_widget_init_template (BOBGUI_WIDGET (editor));

  /* Create the scaled popup adjustments manually here because connecting user data is not
   * supported by template BobguiBuilder xml (it would be possible to set this up in the xml
   * but require 4 separate callbacks and would be rather ugly).
   */
  bobgui_spin_button_set_adjustment (BOBGUI_SPIN_BUTTON (editor->h_entry), scaled_adjustment (editor->h_adj, 360));
  bobgui_spin_button_set_adjustment (BOBGUI_SPIN_BUTTON (editor->s_entry), scaled_adjustment (editor->s_adj, 100));
  bobgui_spin_button_set_adjustment (BOBGUI_SPIN_BUTTON (editor->v_entry), scaled_adjustment (editor->v_adj, 100));
  bobgui_spin_button_set_adjustment (BOBGUI_SPIN_BUTTON (editor->a_entry), scaled_adjustment (editor->a_adj, 100));

  /* This can be setup in the .ui file, but requires work in Glade otherwise it cannot be edited there */
  bobgui_overlay_add_overlay (BOBGUI_OVERLAY (editor->overlay), editor->sv_popup);
  bobgui_overlay_add_overlay (BOBGUI_OVERLAY (editor->overlay), editor->h_popup);
  bobgui_overlay_add_overlay (BOBGUI_OVERLAY (editor->overlay), editor->a_popup);

  controller = bobgui_event_controller_key_new ();
  g_signal_connect (controller, "key-pressed", G_CALLBACK (popup_key_pressed), editor);
  bobgui_widget_add_controller (editor->h_entry, controller);
  controller = bobgui_event_controller_key_new ();
  g_signal_connect (controller, "key-pressed", G_CALLBACK (popup_key_pressed), editor);
  bobgui_widget_add_controller (editor->s_entry, controller);
  controller = bobgui_event_controller_key_new ();
  g_signal_connect (controller, "key-pressed", G_CALLBACK (popup_key_pressed), editor);
  bobgui_widget_add_controller (editor->v_entry, controller);
  controller = bobgui_event_controller_key_new ();
  g_signal_connect (controller, "key-pressed", G_CALLBACK (popup_key_pressed), editor);
  bobgui_widget_add_controller (editor->a_entry, controller);

  bobgui_widget_remove_css_class (editor->swatch, "activatable");

  editor->picker = bobgui_color_picker_new ();
  bobgui_widget_set_visible (editor->picker_button, editor->picker != NULL);
}

static void
bobgui_color_editor_dispose (GObject *object)
{
  BobguiColorEditor *editor = BOBGUI_COLOR_EDITOR (object);

  dismiss_current_popup (editor);
  g_clear_object (&editor->picker);

  G_OBJECT_CLASS (bobgui_color_editor_parent_class)->dispose (object);
}

static void
bobgui_color_editor_get_property (GObject    *object,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  BobguiColorEditor *ce = BOBGUI_COLOR_EDITOR (object);
  BobguiColorChooser *cc = BOBGUI_COLOR_CHOOSER (object);

  switch (prop_id)
    {
    case PROP_RGBA:
      {
        GdkRGBA color;
        bobgui_color_chooser_get_rgba (cc, &color);
        g_value_set_boxed (value, &color);
      }
      break;
    case PROP_USE_ALPHA:
      g_value_set_boolean (value, bobgui_widget_get_visible (ce->a_slider));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_color_editor_set_use_alpha (BobguiColorEditor *editor,
                                gboolean        use_alpha)
{
  if (editor->use_alpha != use_alpha)
    {
      editor->use_alpha = use_alpha;
      bobgui_widget_set_visible (editor->a_slider, use_alpha);
      bobgui_color_swatch_set_use_alpha (BOBGUI_COLOR_SWATCH (editor->swatch), use_alpha);
    }
}

static void
bobgui_color_editor_set_property (GObject      *object,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  BobguiColorEditor *ce = BOBGUI_COLOR_EDITOR (object);
  BobguiColorChooser *cc = BOBGUI_COLOR_CHOOSER (object);

  switch (prop_id)
    {
    case PROP_RGBA:
      bobgui_color_chooser_set_rgba (cc, g_value_get_boxed (value));
      break;
    case PROP_USE_ALPHA:
      bobgui_color_editor_set_use_alpha (ce, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static gboolean
bobgui_color_editor_grab_focus (BobguiWidget *widget)
{
  BobguiColorEditor *ce = BOBGUI_COLOR_EDITOR (widget);

  return bobgui_widget_grab_focus (ce->entry);
}

static void
bobgui_color_editor_class_init (BobguiColorEditorClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = bobgui_color_editor_dispose;
  object_class->get_property = bobgui_color_editor_get_property;
  object_class->set_property = bobgui_color_editor_set_property;

  widget_class->grab_focus = bobgui_color_editor_grab_focus;

  g_object_class_override_property (object_class, PROP_RGBA, "rgba");
  g_object_class_override_property (object_class, PROP_USE_ALPHA, "use-alpha");

  /* Bind class to template
   */
  bobgui_widget_class_set_template_from_resource (widget_class,
					       "/org/bobgui/libbobgui/ui/bobguicoloreditor.ui");

  bobgui_widget_class_bind_template_child (widget_class, BobguiColorEditor, overlay);
  bobgui_widget_class_bind_template_child (widget_class, BobguiColorEditor, grid);
  bobgui_widget_class_bind_template_child (widget_class, BobguiColorEditor, swatch);
  bobgui_widget_class_bind_template_child (widget_class, BobguiColorEditor, entry);
  bobgui_widget_class_bind_template_child (widget_class, BobguiColorEditor, h_slider);
  bobgui_widget_class_bind_template_child (widget_class, BobguiColorEditor, h_popup);
  bobgui_widget_class_bind_template_child (widget_class, BobguiColorEditor, h_entry);
  bobgui_widget_class_bind_template_child (widget_class, BobguiColorEditor, a_slider);
  bobgui_widget_class_bind_template_child (widget_class, BobguiColorEditor, a_popup);
  bobgui_widget_class_bind_template_child (widget_class, BobguiColorEditor, a_entry);
  bobgui_widget_class_bind_template_child (widget_class, BobguiColorEditor, sv_plane);
  bobgui_widget_class_bind_template_child (widget_class, BobguiColorEditor, sv_popup);
  bobgui_widget_class_bind_template_child (widget_class, BobguiColorEditor, s_entry);
  bobgui_widget_class_bind_template_child (widget_class, BobguiColorEditor, v_entry);
  bobgui_widget_class_bind_template_child (widget_class, BobguiColorEditor, h_adj);
  bobgui_widget_class_bind_template_child (widget_class, BobguiColorEditor, s_adj);
  bobgui_widget_class_bind_template_child (widget_class, BobguiColorEditor, v_adj);
  bobgui_widget_class_bind_template_child (widget_class, BobguiColorEditor, a_adj);
  bobgui_widget_class_bind_template_child (widget_class, BobguiColorEditor, picker_button);

  bobgui_widget_class_bind_template_callback (widget_class, hsv_changed);
  bobgui_widget_class_bind_template_callback (widget_class, dismiss_current_popup);
  bobgui_widget_class_bind_template_callback (widget_class, get_child_position);
  bobgui_widget_class_bind_template_callback (widget_class, entry_text_changed);
  bobgui_widget_class_bind_template_callback (widget_class, entry_apply);
  bobgui_widget_class_bind_template_callback (widget_class, entry_focus_changed);
  bobgui_widget_class_bind_template_callback (widget_class, pick_color);

  /**
   * BobguiColorEditor|color.edit:
   * @component: the component to edit, "h", "sv" or "a"
   *
   * Opens the edit popup for one of the color components.
   */ 
  bobgui_widget_class_install_action (widget_class, "color.edit", "s", popup_edit);
}

static void
bobgui_color_editor_get_rgba (BobguiColorChooser *chooser,
                           GdkRGBA         *color)
{
  BobguiColorEditor *editor = BOBGUI_COLOR_EDITOR (chooser);
  float h, s, v;

  h = bobgui_adjustment_get_value (editor->h_adj);
  s = bobgui_adjustment_get_value (editor->s_adj);
  v = bobgui_adjustment_get_value (editor->v_adj);
  bobgui_hsv_to_rgb (h, s, v, &color->red, &color->green, &color->blue);
  color->alpha = bobgui_adjustment_get_value (editor->a_adj);
}

static void
bobgui_color_editor_set_rgba (BobguiColorChooser *chooser,
                           const GdkRGBA   *color)
{
  BobguiColorEditor *editor = BOBGUI_COLOR_EDITOR (chooser);
  float h, s, v;

  bobgui_rgb_to_hsv (color->red, color->green, color->blue, &h, &s, &v);

  bobgui_adjustment_set_value (editor->h_adj, h);
  bobgui_adjustment_set_value (editor->s_adj, s);
  bobgui_adjustment_set_value (editor->v_adj, v);
  bobgui_adjustment_set_value (editor->a_adj, color->alpha);

  update_color (editor, color);

  g_object_notify (G_OBJECT (editor), "rgba");
}

static void
bobgui_color_editor_iface_init (BobguiColorChooserInterface *iface)
{
  iface->get_rgba = bobgui_color_editor_get_rgba;
  iface->set_rgba = bobgui_color_editor_set_rgba;
}

BobguiWidget *
bobgui_color_editor_new (void)
{
  return (BobguiWidget *) g_object_new (BOBGUI_TYPE_COLOR_EDITOR, NULL);
}
