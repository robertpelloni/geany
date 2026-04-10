/*
 * Copyright (c) 2014 Red Hat, Inc.
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
#include <glib/gi18n-lib.h>

#include "prop-editor.h"

#include "strv-editor.h"
#include "prop-list.h"

#include "bobguiactionable.h"
#include "bobguiadjustment.h"
#include "bobguiapplicationwindow.h"
#include "deprecated/bobguicelllayout.h"
#include "deprecated/bobguicombobox.h"
#include "deprecated/bobguiiconview.h"
#include "deprecated/bobguitreeview.h"
#include "bobguicolordialogbutton.h"
#include "bobguifontdialogbutton.h"
#include "bobguilabel.h"
#include "bobguipopover.h"
#include "bobguiscrolledwindow.h"
#include "bobguispinbutton.h"
#include "bobguisettingsprivate.h"
#include "bobguitogglebutton.h"
#include "bobguiviewport.h"
#include "bobguiwidgetprivate.h"
#include "bobguicssnodeprivate.h"
#include "bobguilistbox.h"
#include "bobguimenubutton.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

struct _BobguiInspectorPropEditor
{
  BobguiBox parent_instance;

  GObject *object;
  char *name;
  BobguiWidget *self;
  BobguiSizeGroup *size_group;
};

enum
{
  PROP_0,
  PROP_OBJECT,
  PROP_NAME,
  PROP_SIZE_GROUP
};

enum
{
  SHOW_OBJECT,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE (BobguiInspectorPropEditor, bobgui_inspector_prop_editor, BOBGUI_TYPE_BOX);

static GParamSpec *
find_property (BobguiInspectorPropEditor *self)
{
  return g_object_class_find_property (G_OBJECT_GET_CLASS (self->object), self->name);
}

typedef struct
{
  gpointer instance;
  GObject *alive_object;
  gulong id;
} DisconnectData;

static void
disconnect_func (gpointer data)
{
  DisconnectData *dd = data;

  g_signal_handler_disconnect (dd->instance, dd->id);
}

static void
signal_removed (gpointer  data,
                GClosure *closure)
{
  DisconnectData *dd = data;

  g_object_steal_data (dd->alive_object, "alive-object-data");
  g_free (dd);
}

static void
g_object_connect_property (GObject    *object,
                           GParamSpec *spec,
                           GCallback   func,
                           gpointer    data,
                           GObject    *alive_object)
{
  GClosure *closure;
  char *with_detail;
  DisconnectData *dd;

  with_detail = g_strconcat ("notify::", spec->name, NULL);

  dd = g_new (DisconnectData, 1);

  closure = g_cclosure_new (func, data, NULL);
  g_closure_add_invalidate_notifier (closure, dd, signal_removed);
  dd->id = g_signal_connect_closure (object, with_detail, closure, FALSE);
  dd->instance = object;
  dd->alive_object = alive_object;

  g_object_set_data_full (G_OBJECT (alive_object), "alive-object-data",
                          dd, disconnect_func);

  g_free (with_detail);
}

static void
block_notify (GObject *self)
{
  DisconnectData *dd = (DisconnectData *)g_object_get_data (self, "alive-object-data");

  if (dd)
    g_signal_handler_block (dd->instance, dd->id);
}

static void
unblock_notify (GObject *self)
{
  DisconnectData *dd = (DisconnectData *)g_object_get_data (self, "alive-object-data");

  if (dd)
    g_signal_handler_unblock (dd->instance, dd->id);
}

typedef struct
{
  GObject *obj;
  GParamSpec *spec;
  gulong modified_id;
} ObjectProperty;

static void
free_object_property (ObjectProperty *p)
{
  g_free (p);
}

static void
connect_controller (GObject     *controller,
                    const char *signal,
                    GObject     *model,
                    GParamSpec  *spec,
                    GCallback    func)
{
  ObjectProperty *p;

  p = g_new (ObjectProperty, 1);
  p->obj = model;
  p->spec = spec;

  p->modified_id = g_signal_connect_data (controller, signal, func, p,
                                          (GClosureNotify)free_object_property, 0);
  g_object_set_data (controller, "object-property", p);
}

static void
block_controller (GObject *controller)
{
  ObjectProperty *p = g_object_get_data (controller, "object-property");

  if (p)
    g_signal_handler_block (controller, p->modified_id);
}

static void
unblock_controller (GObject *controller)
{
  ObjectProperty *p = g_object_get_data (controller, "object-property");

  if (p)
    g_signal_handler_unblock (controller, p->modified_id);
}

static void
get_property_value (GObject *object, GParamSpec *pspec, GValue *value)
{
  g_object_get_property (object, pspec->name, value);
}

static void
set_property_value (GObject *object, GParamSpec *pspec, GValue *value)
{
  g_object_set_property (object, pspec->name, value);
}

static void
notify_property (GObject *object, GParamSpec *pspec)
{
  g_object_notify (object, pspec->name);
}

static void
int_modified (BobguiAdjustment *adj, ObjectProperty *p)
{
  GValue val = G_VALUE_INIT;

  g_value_init (&val, G_TYPE_INT);
  g_value_set_int (&val, (int) bobgui_adjustment_get_value (adj));
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
int_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  BobguiAdjustment *adj = BOBGUI_ADJUSTMENT (data);
  GValue val = G_VALUE_INIT;

  g_value_init (&val, G_TYPE_INT);
  get_property_value (object, pspec, &val);

  if (g_value_get_int (&val) != (int)bobgui_adjustment_get_value (adj))
    {
      block_controller (G_OBJECT (adj));
      bobgui_adjustment_set_value (adj, g_value_get_int (&val));
      unblock_controller (G_OBJECT (adj));
    }

  g_value_unset (&val);
}
static void
uint_modified (BobguiAdjustment *adj, ObjectProperty *p)
{
  GValue val = G_VALUE_INIT;

  g_value_init (&val, G_TYPE_UINT);
  g_value_set_uint (&val, (guint) bobgui_adjustment_get_value (adj));
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
uint_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  BobguiAdjustment *adj = BOBGUI_ADJUSTMENT (data);
  GValue val = G_VALUE_INIT;

  g_value_init (&val, G_TYPE_UINT);
  get_property_value (object, pspec, &val);

  if (g_value_get_uint (&val) != (guint)bobgui_adjustment_get_value (adj))
    {
      block_controller (G_OBJECT (adj));
      bobgui_adjustment_set_value (adj, g_value_get_uint (&val));
      unblock_controller (G_OBJECT (adj));
    }

  g_value_unset (&val);
}

static void
float_modified (BobguiAdjustment *adj, ObjectProperty *p)
{
  GValue val = G_VALUE_INIT;

  g_value_init (&val, G_TYPE_FLOAT);
  g_value_set_float (&val, (float) bobgui_adjustment_get_value (adj));
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
float_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  BobguiAdjustment *adj = BOBGUI_ADJUSTMENT (data);
  GValue val = G_VALUE_INIT;

  g_value_init (&val, G_TYPE_FLOAT);
  get_property_value (object, pspec, &val);

  if (g_value_get_float (&val) != (float) bobgui_adjustment_get_value (adj))
    {
      block_controller (G_OBJECT (adj));
      bobgui_adjustment_set_value (adj, g_value_get_float (&val));
      unblock_controller (G_OBJECT (adj));
    }

  g_value_unset (&val);
}

static void
double_modified (BobguiAdjustment *adj, ObjectProperty *p)
{
  GValue val = G_VALUE_INIT;

  g_value_init (&val, G_TYPE_DOUBLE);
  g_value_set_double (&val, bobgui_adjustment_get_value (adj));
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
double_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  BobguiAdjustment *adj = BOBGUI_ADJUSTMENT (data);
  GValue val = G_VALUE_INIT;

  g_value_init (&val, G_TYPE_DOUBLE);
  get_property_value (object, pspec, &val);

  if (g_value_get_double (&val) != bobgui_adjustment_get_value (adj))
    {
      block_controller (G_OBJECT (adj));
      bobgui_adjustment_set_value (adj, g_value_get_double (&val));
      unblock_controller (G_OBJECT (adj));
    }

  g_value_unset (&val);
}

static void
string_modified (BobguiEntry *entry, ObjectProperty *p)
{
  GValue val = G_VALUE_INIT;

  g_value_init (&val, G_TYPE_STRING);
  g_value_set_static_string (&val, bobgui_editable_get_text (BOBGUI_EDITABLE (entry)));
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
intern_string_modified (BobguiEntry *entry, ObjectProperty *p)
{
  const char *s;

  s = bobgui_editable_get_text (BOBGUI_EDITABLE (entry));
  if (g_str_equal (p->spec->name, "id"))
    bobgui_css_node_set_id (BOBGUI_CSS_NODE (p->obj), g_quark_from_string (s));
  else if (g_str_equal (p->spec->name, "name"))
    bobgui_css_node_set_name (BOBGUI_CSS_NODE (p->obj), g_quark_from_string (s));
}

static void
attr_list_modified (BobguiEntry *entry, ObjectProperty *p)
{
  GValue val = G_VALUE_INIT;
  PangoAttrList *attrs;

  attrs = pango_attr_list_from_string (bobgui_editable_get_text (BOBGUI_EDITABLE (entry)));
  if (!attrs)
    return;

  g_value_init (&val, PANGO_TYPE_ATTR_LIST);
  g_value_take_boxed (&val, attrs);
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
tab_array_modified (BobguiEntry *entry, ObjectProperty *p)
{
  GValue val = G_VALUE_INIT;
  PangoTabArray *tabs;

  tabs = pango_tab_array_from_string (bobgui_editable_get_text (BOBGUI_EDITABLE (entry)));
  if (!tabs)
    return;

  g_value_init (&val, PANGO_TYPE_TAB_ARRAY);
  g_value_take_boxed (&val, tabs);
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
string_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  BobguiEntry *entry = BOBGUI_ENTRY (data);
  GValue val = G_VALUE_INIT;
  const char *str;
  const char *text;

  g_value_init (&val, G_TYPE_STRING);
  get_property_value (object, pspec, &val);

  str = g_value_get_string (&val);
  if (str == NULL)
    str = "";
  text = bobgui_editable_get_text (BOBGUI_EDITABLE (entry));
  if (g_strcmp0 (str, text) != 0)
    {
      block_controller (G_OBJECT (entry));
      bobgui_editable_set_text (BOBGUI_EDITABLE (entry), str);
      unblock_controller (G_OBJECT (entry));
    }

  g_value_unset (&val);
}

static void
attr_list_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  BobguiEntry *entry = BOBGUI_ENTRY (data);
  GValue val = G_VALUE_INIT;
  char *str = NULL;
  const char *text;
  PangoAttrList *attrs;

  g_value_init (&val, PANGO_TYPE_ATTR_LIST);
  get_property_value (object, pspec, &val);

  attrs = g_value_get_boxed (&val);
  if (attrs)
    str = pango_attr_list_to_string (attrs);
  if (str == NULL)
    str = g_strdup ("");
  text = bobgui_editable_get_text (BOBGUI_EDITABLE (entry));
  if (g_strcmp0 (str, text) != 0)
    {
      block_controller (G_OBJECT (entry));
      bobgui_editable_set_text (BOBGUI_EDITABLE (entry), str);
      unblock_controller (G_OBJECT (entry));
    }

  g_free (str);

  g_value_unset (&val);
}

static void
tab_array_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  BobguiEntry *entry = BOBGUI_ENTRY (data);
  GValue val = G_VALUE_INIT;
  char *str = NULL;
  const char *text;
  PangoTabArray *tabs;

  g_value_init (&val, PANGO_TYPE_TAB_ARRAY);
  get_property_value (object, pspec, &val);

  tabs = g_value_get_boxed (&val);
  if (tabs)
    str = pango_tab_array_to_string (tabs);
  if (str == NULL)
    str = g_strdup ("");
  text = bobgui_editable_get_text (BOBGUI_EDITABLE (entry));
  if (g_strcmp0 (str, text) != 0)
    {
      block_controller (G_OBJECT (entry));
      bobgui_editable_set_text (BOBGUI_EDITABLE (entry), str);
      unblock_controller (G_OBJECT (entry));
    }

  g_free (str);

  g_value_unset (&val);
}

static void
strv_modified (BobguiInspectorStrvEditor *self, ObjectProperty *p)
{
  GValue val = G_VALUE_INIT;
  char **strv;

  g_value_init (&val, G_TYPE_STRV);
  strv = bobgui_inspector_strv_editor_get_strv (self);
  g_value_take_boxed (&val, strv);
  block_notify (G_OBJECT (self));
  set_property_value (p->obj, p->spec, &val);
  unblock_notify (G_OBJECT (self));
  g_value_unset (&val);
}

static void
strv_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  BobguiInspectorStrvEditor *self = data;
  GValue val = G_VALUE_INIT;
  char **strv;

  g_value_init (&val, G_TYPE_STRV);
  get_property_value (object, pspec, &val);

  strv = g_value_get_boxed (&val);
  block_controller (G_OBJECT (self));
  bobgui_inspector_strv_editor_set_strv (self, strv);
  unblock_controller (G_OBJECT (self));

  g_value_unset (&val);
}

static void
bool_modified (BobguiCheckButton *cb,
               ObjectProperty *p)
{
  GValue val = G_VALUE_INIT;

  g_value_init (&val, G_TYPE_BOOLEAN);
  g_value_set_boolean (&val, bobgui_check_button_get_active (cb));
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
bool_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  BobguiCheckButton *cb = BOBGUI_CHECK_BUTTON (data);
  GValue val = G_VALUE_INIT;

  g_value_init (&val, G_TYPE_BOOLEAN);
  get_property_value (object, pspec, &val);

  if (g_value_get_boolean (&val) != bobgui_check_button_get_active (cb))
    {
      block_controller (G_OBJECT (cb));
      bobgui_check_button_set_active (cb, g_value_get_boolean (&val));
      unblock_controller (G_OBJECT (cb));
    }

  g_value_unset (&val);
}

static void
enum_modified (BobguiDropDown *dropdown, GParamSpec *pspec, ObjectProperty *p)
{
  int i = bobgui_drop_down_get_selected (dropdown);
  GEnumClass *eclass;
  GValue val = G_VALUE_INIT;

  eclass = G_ENUM_CLASS (g_type_class_peek (p->spec->value_type));

  g_value_init (&val, p->spec->value_type);
  g_value_set_enum (&val, eclass->values[i].value);
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
enum_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  GValue val = G_VALUE_INIT;
  GEnumClass *eclass;
  int i;

  eclass = G_ENUM_CLASS (g_type_class_peek (pspec->value_type));

  g_value_init (&val, pspec->value_type);
  get_property_value (object, pspec, &val);

  i = 0;
  while (i < eclass->n_values)
    {
      if (eclass->values[i].value == g_value_get_enum (&val))
        break;
      ++i;
    }
  g_value_unset (&val);

  block_controller (G_OBJECT (data));
  bobgui_drop_down_set_selected (BOBGUI_DROP_DOWN (data), i);
  unblock_controller (G_OBJECT (data));
}

static void
flags_modified (BobguiCheckButton *button, ObjectProperty *p)
{
  gboolean active;
  GFlagsClass *fclass;
  guint flags;
  int i;
  GValue val = G_VALUE_INIT;

  active = bobgui_check_button_get_active (button);
  i = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (button), "index"));
  fclass = G_FLAGS_CLASS (g_type_class_peek (p->spec->value_type));

  g_value_init (&val, p->spec->value_type);
  get_property_value (p->obj, p->spec, &val);
  flags = g_value_get_flags (&val);
  if (active)
    flags |= fclass->values[i].value;
  else
    flags &= ~fclass->values[i].value;
  g_value_set_flags (&val, flags);
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static char *
flags_to_string (GFlagsClass *flags_class,
                 guint        value)
{
  GString *str;
  GFlagsValue *flags_value;

  str = g_string_new (NULL);

  while ((str->len == 0 || value != 0) &&
         (flags_value = g_flags_get_first_value (flags_class, value)) != NULL)
    {
      if (str->len > 0)
        g_string_append (str, " | ");

      g_string_append (str, flags_value->value_nick);

      value &= ~flags_value->value;
    }

  /* Show the extra bits */
  if (value != 0 || str->len == 0)
    {
      if (str->len > 0)
        g_string_append (str, " | ");

      g_string_append_printf (str, "0x%x", value);
    }

  return g_string_free (str, FALSE);
}

static void
flags_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  GValue val = G_VALUE_INIT;
  GFlagsClass *fclass;
  guint flags;
  int i;
  BobguiPopover *popover;
  BobguiWidget *sw;
  BobguiWidget *viewport;
  BobguiWidget *box;
  char *str;
  BobguiWidget *child;

  fclass = G_FLAGS_CLASS (g_type_class_peek (pspec->value_type));

  g_value_init (&val, pspec->value_type);
  get_property_value (object, pspec, &val);
  flags = g_value_get_flags (&val);
  g_value_unset (&val);

  str = flags_to_string (fclass, flags);
  child = bobgui_menu_button_get_child (BOBGUI_MENU_BUTTON (data));
  bobgui_label_set_text (BOBGUI_LABEL (child), str);
  g_free (str);

  popover = bobgui_menu_button_get_popover (BOBGUI_MENU_BUTTON (data));
  sw =  bobgui_popover_get_child (BOBGUI_POPOVER (popover));
  viewport = bobgui_scrolled_window_get_child (BOBGUI_SCROLLED_WINDOW (sw));
  box = bobgui_viewport_get_child (BOBGUI_VIEWPORT (viewport));
  for (child = bobgui_widget_get_first_child (box);
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    block_controller (G_OBJECT (child));

  for (child = bobgui_widget_get_first_child (box), i = 0;
       child != NULL;
       child = bobgui_widget_get_next_sibling (child), i++)
    bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (child),
                                 (fclass->values[i].value & flags) != 0);

  for (child = bobgui_widget_get_first_child (box);
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    unblock_controller (G_OBJECT (child));
}

static gunichar
unichar_get_value (BobguiEntry *entry)
{
  const char *text = bobgui_editable_get_text (BOBGUI_EDITABLE (entry));

  if (text[0])
    return g_utf8_get_char (text);
  else
    return 0;
}

static void
unichar_modified (BobguiEntry *entry, ObjectProperty *p)
{
  gunichar u = unichar_get_value (entry);
  GValue val = G_VALUE_INIT;

  g_value_init (&val, p->spec->value_type);
  g_value_set_uint (&val, u);
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}
static void
unichar_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  BobguiEntry *entry = BOBGUI_ENTRY (data);
  gunichar new_val;
  gunichar old_val = unichar_get_value (entry);
  GValue val = G_VALUE_INIT;
  char buf[7];
  int len;

  g_value_init (&val, pspec->value_type);
  get_property_value (object, pspec, &val);
  new_val = (gunichar)g_value_get_uint (&val);
  g_value_unset (&val);

  if (new_val != old_val)
    {
      if (!new_val)
        len = 0;
      else
        len = g_unichar_to_utf8 (new_val, buf);

      buf[len] = '\0';

      block_controller (G_OBJECT (entry));
      bobgui_editable_set_text (BOBGUI_EDITABLE (entry), buf);
      unblock_controller (G_OBJECT (entry));
    }
}

static void
pointer_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  BobguiLabel *label = BOBGUI_LABEL (data);
  char *str;
  gpointer ptr;

  g_object_get (object, pspec->name, &ptr, NULL);

  str = g_strdup_printf (_("Pointer: %p"), ptr);
  bobgui_label_set_text (label, str);
  g_free (str);
}

static char *
object_label (GObject *obj, GParamSpec *pspec)
{
  return g_strdup_printf ("%p", obj);
}

static void
object_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  BobguiWidget *label, *button;
  char *str;
  GObject *obj;

  label = bobgui_widget_get_first_child (BOBGUI_WIDGET (data));
  button = bobgui_widget_get_next_sibling (label);
  g_object_get (object, pspec->name, &obj, NULL);

  str = object_label (obj, pspec);

  bobgui_label_set_text (BOBGUI_LABEL (label), str);
  bobgui_widget_set_sensitive (button, G_IS_OBJECT (obj));

  if (obj)
    g_object_unref (obj);

  g_free (str);
}

static void
object_properties (BobguiInspectorPropEditor *self)
{
  GObject *obj;

  g_object_get (self->object, self->name, &obj, NULL);
  if (G_IS_OBJECT (obj))
    g_signal_emit (self, signals[SHOW_OBJECT], 0, obj, self->name, "properties");
}

static void
rgba_modified (BobguiColorDialogButton *cb, GParamSpec *ignored, ObjectProperty *p)
{
  GValue val = G_VALUE_INIT;

  g_value_init (&val, p->spec->value_type);
  g_object_get_property (G_OBJECT (cb), "rgba", &val);
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
rgba_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  BobguiColorChooser *cb = BOBGUI_COLOR_CHOOSER (data);
  GValue val = G_VALUE_INIT;
  GdkRGBA *color;

  g_value_init (&val, GDK_TYPE_RGBA);
  get_property_value (object, pspec, &val);

  color = g_value_get_boxed (&val);

  if (color != NULL)
    {
      block_controller (G_OBJECT (cb));
      bobgui_color_dialog_button_set_rgba (BOBGUI_COLOR_DIALOG_BUTTON (cb), color);
      unblock_controller (G_OBJECT (cb));
    }
 g_value_unset (&val);
}

static void
font_modified (BobguiFontDialogButton *fb, GParamSpec *pspec, ObjectProperty *p)
{
  GValue val = G_VALUE_INIT;

  g_value_init (&val, PANGO_TYPE_FONT_DESCRIPTION);
  g_object_get_property (G_OBJECT (fb), "font-desc", &val);
  set_property_value (p->obj, p->spec, &val);
  g_value_unset (&val);
}

static void
font_changed (GObject *object, GParamSpec *pspec, gpointer data)
{
  BobguiFontDialogButton *fb = BOBGUI_FONT_DIALOG_BUTTON (data);
  GValue val = G_VALUE_INIT;
  const PangoFontDescription *font_desc;

  g_value_init (&val, PANGO_TYPE_FONT_DESCRIPTION);
  get_property_value (object, pspec, &val);

  font_desc = g_value_get_boxed (&val);

  if (font_desc != NULL)
    {
      block_controller (G_OBJECT (fb));
      bobgui_font_dialog_button_set_font_desc (fb, font_desc);
      unblock_controller (G_OBJECT (fb));
    }

  g_value_unset (&val);
}

static char *
describe_expression (BobguiExpression *expression)
{
  if (expression == NULL)
    return NULL;

  if (G_TYPE_CHECK_INSTANCE_TYPE (expression, BOBGUI_TYPE_CONSTANT_EXPRESSION))
    {
      const GValue *value = bobgui_constant_expression_get_value (expression);
      GValue dest = G_VALUE_INIT;

      g_value_init (&dest, G_TYPE_STRING);
      if (g_value_transform (value, &dest))
        {
          /* Translators: %s is a type name, for example
           * BobguiPropertyExpression with value \"2.5\"
           */
          char *res = g_strdup_printf (_("%s with value \"%s\""),
                                       g_type_name (G_TYPE_FROM_INSTANCE (expression)),
                                       g_value_get_string (&dest));
          g_value_unset (&dest);
          return res;
        }
      else
        {
          /* Translators: Both %s are type names, for example
           * BobguiPropertyExpression with type GObject
           */
          return g_strdup_printf (_("%s with type %s"),
                                  g_type_name (G_TYPE_FROM_INSTANCE (expression)),
                                  g_type_name (G_VALUE_TYPE (value)));
        }
    }
  else if (G_TYPE_CHECK_INSTANCE_TYPE (expression, BOBGUI_TYPE_OBJECT_EXPRESSION))
    {
      gpointer obj = bobgui_object_expression_get_object (expression);

      if (obj)
        /* Translators: Both %s are type names, for example
         * BobguiObjectExpression for BobguiStringObject 0x23456789
         */
        return g_strdup_printf (_("%s for %s %p"),
                                g_type_name (G_TYPE_FROM_INSTANCE (expression)),
                                G_OBJECT_TYPE_NAME (obj), obj);
      else
        return g_strdup (g_type_name (G_TYPE_FROM_INSTANCE (expression)));
    }
  else if (G_TYPE_CHECK_INSTANCE_TYPE (expression, BOBGUI_TYPE_PROPERTY_EXPRESSION))
    {
      GParamSpec *pspec = bobgui_property_expression_get_pspec (expression);
      BobguiExpression *expr = bobgui_property_expression_get_expression (expression);
      char *str;
      char *res;

      str = describe_expression (expr);
      /* Translators: The first %s is a type name, %s:%s is a qualified
       * property name, and is a value, for example
       * BobguiPropertyExpression for property BobguiLabellabel on: GObjectExpression ...
       */
      res = g_strdup_printf ("%s for property %s:%s on: %s",
                             g_type_name (G_TYPE_FROM_INSTANCE (expression)),
                             g_type_name (pspec->owner_type),
                             pspec->name,
                             str);
      g_free (str);
      return res;
    }
  else
    /* Translators: Both %s are type names, for example
     * BobguiPropertyExpression with value type: gchararray
     */
    return g_strdup_printf (_("%s with value type %s"),
                            g_type_name (G_TYPE_FROM_INSTANCE (expression)),
                            g_type_name (bobgui_expression_get_value_type (expression)));
}

static void
toggle_unicode (BobguiWidget *stack,
                gboolean   show_unicode)
{
  BobguiWidget *entry;
  BobguiWidget *unicode;

  entry = bobgui_stack_get_child_by_name (BOBGUI_STACK (stack), "entry");
  unicode = bobgui_stack_get_child_by_name (BOBGUI_STACK (stack), "unicode");
  if (show_unicode)
    {
      const char *orig;
      const char *p;
      GString *s;
      PangoAttrList *attrs = NULL;
      char *text = NULL;

      orig = bobgui_editable_get_text (BOBGUI_EDITABLE (entry));
      s = g_string_sized_new (10 * strlen (orig));
      for (p = orig; *p; p = g_utf8_next_char (p))
        {
          gunichar ch = g_utf8_get_char (p);
          if (s->len > 0)
            g_string_append_unichar (s, 0x2005);
          g_string_append_unichar (s, ch);
          g_string_append_unichar (s, 0x2005);
          g_string_append (s, "<span alpha=\"70%\" font_size=\"smaller\">");
          g_string_append_printf (s, "%04X", ch);
          g_string_append (s, "</span>");
        }
      pango_parse_markup (s->str, s->len, 0, &attrs, &text, NULL, NULL);
      bobgui_editable_set_text (BOBGUI_EDITABLE (unicode), text);
      bobgui_entry_set_attributes (BOBGUI_ENTRY (unicode), attrs);
      pango_attr_list_unref (attrs);
      g_free (text);
      g_string_free (s, TRUE);

      bobgui_stack_set_visible_child_name (BOBGUI_STACK (stack), "unicode");
    }
  else
    {
      bobgui_editable_set_text (BOBGUI_EDITABLE (unicode), "");
      bobgui_stack_set_visible_child_name (BOBGUI_STACK (stack), "entry");
    }
}

static void
show_unicode (BobguiEntry             *entry,
              BobguiEntryIconPosition  pos,
              BobguiWidget            *stack)
{
  toggle_unicode (stack, TRUE);
}

static void
show_text (BobguiEntry             *entry,
           BobguiEntryIconPosition  pos,
           BobguiWidget            *stack)
{
  toggle_unicode (stack, FALSE);
}

static BobguiWidget *
property_editor (GObject                *object,
                 GParamSpec             *spec,
                 BobguiInspectorPropEditor *self)
{
  BobguiWidget *prop_edit;
  BobguiAdjustment *adj;
  char *msg;
  GType type = G_PARAM_SPEC_TYPE (spec);

  if (type == G_TYPE_PARAM_INT)
    {
      adj = bobgui_adjustment_new (G_PARAM_SPEC_INT (spec)->default_value,
                                G_PARAM_SPEC_INT (spec)->minimum,
                                G_PARAM_SPEC_INT (spec)->maximum,
                                1,
                                MAX ((((double) G_PARAM_SPEC_INT (spec)->maximum - (double)G_PARAM_SPEC_INT (spec)->minimum)) / 10, 1),
                                0.0);

      prop_edit = bobgui_spin_button_new (adj, 1.0, 0);

      g_object_connect_property (object, spec, G_CALLBACK (int_changed), adj, G_OBJECT (adj));

      connect_controller (G_OBJECT (adj), "value_changed",
                          object, spec, G_CALLBACK (int_modified));
    }
  else if (type == G_TYPE_PARAM_UINT)
    {
      adj = bobgui_adjustment_new (G_PARAM_SPEC_UINT (spec)->default_value,
                                G_PARAM_SPEC_UINT (spec)->minimum,
                                G_PARAM_SPEC_UINT (spec)->maximum,
                                1,
                                MAX ((G_PARAM_SPEC_UINT (spec)->maximum - G_PARAM_SPEC_UINT (spec)->minimum) / 10, 1),
                                0.0);

      prop_edit = bobgui_spin_button_new (adj, 1.0, 0);

      g_object_connect_property (object, spec,
                                 G_CALLBACK (uint_changed),
                                 adj, G_OBJECT (adj));

      connect_controller (G_OBJECT (adj), "value_changed",
                          object, spec, G_CALLBACK (uint_modified));
    }
  else if (type == G_TYPE_PARAM_FLOAT)
    {
      adj = bobgui_adjustment_new (G_PARAM_SPEC_FLOAT (spec)->default_value,
                                G_PARAM_SPEC_FLOAT (spec)->minimum,
                                G_PARAM_SPEC_FLOAT (spec)->maximum,
                                0.1,
                                MAX (((double) G_PARAM_SPEC_FLOAT (spec)->maximum - (double) G_PARAM_SPEC_FLOAT (spec)->minimum) / 10, 0.1),
                                0.0);

      prop_edit = bobgui_spin_button_new (adj, 0.1, 2);

      g_object_connect_property (object, spec,
                                 G_CALLBACK (float_changed),
                                 adj, G_OBJECT (adj));

      connect_controller (G_OBJECT (adj), "value_changed",
                          object, spec, G_CALLBACK (float_modified));
    }
  else if (type == G_TYPE_PARAM_DOUBLE)
    {
      adj = bobgui_adjustment_new (G_PARAM_SPEC_DOUBLE (spec)->default_value,
                                G_PARAM_SPEC_DOUBLE (spec)->minimum,
                                G_PARAM_SPEC_DOUBLE (spec)->maximum,
                                0.1,
                                1.0,
                                0.0);

      prop_edit = bobgui_spin_button_new (adj, 0.1, 2);

      g_object_connect_property (object, spec,
                                 G_CALLBACK (double_changed),
                                 adj, G_OBJECT (adj));

      connect_controller (G_OBJECT (adj), "value_changed",
                          object, spec, G_CALLBACK (double_modified));
    }
  else if (type == G_TYPE_PARAM_STRING)
    {
      BobguiWidget *entry;
      BobguiWidget *stack;
      BobguiWidget *unicode;

      stack = bobgui_stack_new ();

      entry = bobgui_entry_new ();
      bobgui_entry_set_icon_from_icon_name (BOBGUI_ENTRY (entry),
                                         BOBGUI_ENTRY_ICON_SECONDARY,
                                         "info-outline-symbolic");
      bobgui_entry_set_icon_tooltip_text (BOBGUI_ENTRY (entry),
                                       BOBGUI_ENTRY_ICON_SECONDARY,
                                       "Show Unicode");
      g_signal_connect (entry, "icon-press",
                        G_CALLBACK (show_unicode), stack);

      g_object_connect_property (object, spec,
                                 G_CALLBACK (string_changed),
                                 entry, G_OBJECT (entry));

      if (BOBGUI_IS_CSS_NODE (object))
        connect_controller (G_OBJECT (entry), "changed",
                            object, spec, G_CALLBACK (intern_string_modified));
      else
        connect_controller (G_OBJECT (entry), "changed",
                            object, spec, G_CALLBACK (string_modified));

      unicode = bobgui_entry_new ();
      bobgui_editable_set_editable (BOBGUI_EDITABLE (unicode), FALSE);
      bobgui_entry_set_icon_from_icon_name (BOBGUI_ENTRY (unicode),
                                         BOBGUI_ENTRY_ICON_SECONDARY,
                                         "view-reveal-symbolic");
      bobgui_entry_set_icon_tooltip_text (BOBGUI_ENTRY (unicode),
                                       BOBGUI_ENTRY_ICON_SECONDARY,
                                       "Show Text");
      g_signal_connect (unicode, "icon-press",
                        G_CALLBACK (show_text), stack);

      bobgui_stack_add_named (BOBGUI_STACK (stack), entry, "entry");
      bobgui_stack_add_named (BOBGUI_STACK (stack), unicode, "unicode");

      prop_edit = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_box_append (BOBGUI_BOX (prop_edit), stack);
    }
  else if (type == G_TYPE_PARAM_BOOLEAN)
    {
      prop_edit = bobgui_check_button_new_with_label ("");

      g_object_connect_property (object, spec,
                                 G_CALLBACK (bool_changed),
                                 prop_edit, G_OBJECT (prop_edit));

      connect_controller (G_OBJECT (prop_edit), "toggled",
                          object, spec, G_CALLBACK (bool_modified));
    }
  else if (type == G_TYPE_PARAM_ENUM)
    {
      BobguiEnumList *list;
      BobguiExpression *expression;

      list = bobgui_enum_list_new (spec->value_type);
      expression = bobgui_property_expression_new (BOBGUI_TYPE_ENUM_LIST_ITEM, NULL, "nick");
      prop_edit = bobgui_drop_down_new (G_LIST_MODEL (list), expression);

      connect_controller (G_OBJECT (prop_edit), "notify::selected",
                          object, spec, G_CALLBACK (enum_modified));

      g_object_connect_property (object, spec,
                                 G_CALLBACK (enum_changed),
                                 prop_edit, G_OBJECT (prop_edit));
    }
  else if (type == G_TYPE_PARAM_FLAGS)
    {
      BobguiWidget *label;
      BobguiWidget *box;
      BobguiWidget *sw;
      BobguiWidget *popover;
      GFlagsClass *fclass;
      int j;

      popover = bobgui_popover_new ();
      prop_edit = bobgui_menu_button_new ();
      bobgui_menu_button_set_always_show_arrow (BOBGUI_MENU_BUTTON (prop_edit), TRUE);
      bobgui_menu_button_set_popover (BOBGUI_MENU_BUTTON (prop_edit), popover);
      label = bobgui_label_new ("");
      bobgui_label_set_ellipsize (BOBGUI_LABEL (label), PANGO_ELLIPSIZE_END);
      bobgui_menu_button_set_child (BOBGUI_MENU_BUTTON (prop_edit), label);

      sw = bobgui_scrolled_window_new ();
      bobgui_popover_set_child (BOBGUI_POPOVER (popover), sw);
      g_object_set (sw,
                    "hexpand", TRUE,
                    "vexpand", TRUE,
                    "hscrollbar-policy", BOBGUI_POLICY_NEVER,
                    "vscrollbar-policy", BOBGUI_POLICY_NEVER,
                    NULL);
      box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), box);

      fclass = G_FLAGS_CLASS (g_type_class_ref (spec->value_type));

      for (j = 0; j < fclass->n_values; j++)
        {
          BobguiWidget *b;

          b = bobgui_check_button_new_with_label (fclass->values[j].value_nick);
          g_object_set_data (G_OBJECT (b), "index", GINT_TO_POINTER (j));
          bobgui_box_append (BOBGUI_BOX (box), b);
          connect_controller (G_OBJECT (b), "toggled",
                              object, spec, G_CALLBACK (flags_modified));
        }

      if (j >= 10)
        {
          g_object_set (sw,
                        "vscrollbar-policy", BOBGUI_POLICY_AUTOMATIC,
                        "min-content-height", 250,
                        NULL);
        }

      g_type_class_unref (fclass);

      g_object_connect_property (object, spec,
                                 G_CALLBACK (flags_changed),
                                 prop_edit, G_OBJECT (prop_edit));
    }
  else if (type == G_TYPE_PARAM_UNICHAR)
    {
      prop_edit = bobgui_entry_new ();
      bobgui_entry_set_max_length (BOBGUI_ENTRY (prop_edit), 1);

      g_object_connect_property (object, spec,
                                 G_CALLBACK (unichar_changed),
                                 prop_edit, G_OBJECT (prop_edit));

      connect_controller (G_OBJECT (prop_edit), "changed",
                          object, spec, G_CALLBACK (unichar_modified));
    }
  else if (type == G_TYPE_PARAM_POINTER)
    {
      prop_edit = bobgui_label_new ("");

      g_object_connect_property (object, spec,
                                 G_CALLBACK (pointer_changed),
                                 prop_edit, G_OBJECT (prop_edit));
    }
  else if (type == G_TYPE_PARAM_OBJECT)
    {
      BobguiWidget *label, *button;

      prop_edit = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 5);

      label = bobgui_label_new ("");
      button = bobgui_button_new_from_icon_name ("info-outline-symbolic");
      bobgui_widget_set_css_classes (button, (const char *[]) { "flat", "circular", NULL });
      bobgui_widget_set_tooltip_text (button, "Show properties");
      g_signal_connect_swapped (button, "clicked",
                                G_CALLBACK (object_properties),
                                self);
      bobgui_box_append (BOBGUI_BOX (prop_edit), label);
      bobgui_box_append (BOBGUI_BOX (prop_edit), button);

      g_object_connect_property (object, spec,
                                 G_CALLBACK (object_changed),
                                 prop_edit, G_OBJECT (label));
    }
  else if (type == G_TYPE_PARAM_BOXED &&
           G_PARAM_SPEC_VALUE_TYPE (spec) == GDK_TYPE_RGBA)
    {
      prop_edit = bobgui_color_dialog_button_new (bobgui_color_dialog_new ());

      g_object_connect_property (object, spec,
                                 G_CALLBACK (rgba_changed),
                                 prop_edit, G_OBJECT (prop_edit));

      connect_controller (G_OBJECT (prop_edit), "notify::rgba",
                          object, spec, G_CALLBACK (rgba_modified));
    }
  else if (type == G_TYPE_PARAM_BOXED &&
           G_PARAM_SPEC_VALUE_TYPE (spec) == PANGO_TYPE_FONT_DESCRIPTION)
    {
      prop_edit = bobgui_font_dialog_button_new (bobgui_font_dialog_new ());

      g_object_connect_property (object, spec,
                                 G_CALLBACK (font_changed),
                                 prop_edit, G_OBJECT (prop_edit));

      connect_controller (G_OBJECT (prop_edit), "notify::font-desc",
                          object, spec, G_CALLBACK (font_modified));
    }
  else if (type == G_TYPE_PARAM_BOXED &&
           G_PARAM_SPEC_VALUE_TYPE (spec) == G_TYPE_STRV)
    {
      prop_edit = g_object_new (bobgui_inspector_strv_editor_get_type (),
                                "visible", TRUE,
                                NULL);

      g_object_connect_property (object, spec,
                                 G_CALLBACK (strv_changed),
                                 prop_edit, G_OBJECT (prop_edit));

      connect_controller (G_OBJECT (prop_edit), "changed",
                          object, spec, G_CALLBACK (strv_modified));

      bobgui_widget_set_halign (prop_edit, BOBGUI_ALIGN_START);
      bobgui_widget_set_valign (prop_edit, BOBGUI_ALIGN_CENTER);
    }
  else if (type == G_TYPE_PARAM_BOXED &&
           G_PARAM_SPEC_VALUE_TYPE (spec) == PANGO_TYPE_ATTR_LIST)
    {
      prop_edit = bobgui_entry_new ();

      g_object_connect_property (object, spec,
                                 G_CALLBACK (attr_list_changed),
                                 prop_edit, G_OBJECT (prop_edit));

      connect_controller (G_OBJECT (prop_edit), "activate",
                          object, spec, G_CALLBACK (attr_list_modified));
    }
  else if (type == G_TYPE_PARAM_BOXED &&
           G_PARAM_SPEC_VALUE_TYPE (spec) == PANGO_TYPE_TAB_ARRAY)
    {
      prop_edit = bobgui_entry_new ();

      g_object_connect_property (object, spec,
                                 G_CALLBACK (tab_array_changed),
                                 prop_edit, G_OBJECT (prop_edit));

      connect_controller (G_OBJECT (prop_edit), "activate",
                          object, spec, G_CALLBACK (tab_array_modified));
    }
  else if (type == BOBGUI_TYPE_PARAM_SPEC_EXPRESSION)
    {
      BobguiExpression *expression;
      g_object_get (object, spec->name, &expression, NULL);
      msg = describe_expression (expression);
      prop_edit = bobgui_label_new (msg);
      g_free (msg);
      g_clear_pointer (&expression, bobgui_expression_unref);
      bobgui_widget_set_halign (prop_edit, BOBGUI_ALIGN_START);
      bobgui_widget_set_valign (prop_edit, BOBGUI_ALIGN_CENTER);
    }
  else
    {
      msg = g_strdup_printf (_("Uneditable property type: %s"),
                             g_type_name (G_PARAM_SPEC_TYPE (spec)));
      prop_edit = bobgui_label_new (msg);
      g_free (msg);
      bobgui_widget_set_halign (prop_edit, BOBGUI_ALIGN_START);
      bobgui_widget_set_valign (prop_edit, BOBGUI_ALIGN_CENTER);
    }

  notify_property (object, spec);

  if (BOBGUI_IS_LABEL (prop_edit))
    {
      bobgui_widget_set_can_focus (prop_edit, TRUE);
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (prop_edit),
                                      BOBGUI_ACCESSIBLE_PROPERTY_LABEL,
                                      g_strdup_printf ("%s: %s",
                                                       self->name,
                                                       bobgui_label_get_text (BOBGUI_LABEL (prop_edit))),
                                      -1);
    }
  else
    {
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (prop_edit),
                                      BOBGUI_ACCESSIBLE_PROPERTY_LABEL, self->name,
                                      -1);
    }

  return prop_edit;
}

static void
bobgui_inspector_prop_editor_init (BobguiInspectorPropEditor *self)
{
  g_object_set (self,
                "orientation", BOBGUI_ORIENTATION_HORIZONTAL,
                "spacing", 10,
                NULL);
}

static BobguiTreeModel *
bobgui_cell_layout_get_model (BobguiCellLayout *layout)
{
  if (BOBGUI_IS_TREE_VIEW_COLUMN (layout))
    return bobgui_tree_view_get_model (BOBGUI_TREE_VIEW (bobgui_tree_view_column_get_tree_view (BOBGUI_TREE_VIEW_COLUMN (layout))));
  else if (BOBGUI_IS_ICON_VIEW (layout))
    return bobgui_icon_view_get_model (BOBGUI_ICON_VIEW (layout));
  else if (BOBGUI_IS_COMBO_BOX (layout))
    return bobgui_combo_box_get_model (BOBGUI_COMBO_BOX (layout));
  else
    return NULL;
}

static BobguiWidget *
bobgui_cell_layout_get_widget (BobguiCellLayout *layout)
{
  if (BOBGUI_IS_TREE_VIEW_COLUMN (layout))
    return bobgui_tree_view_column_get_tree_view (BOBGUI_TREE_VIEW_COLUMN (layout));
  else if (BOBGUI_IS_WIDGET (layout))
    return BOBGUI_WIDGET (layout);
  else
    return NULL;
}

static void
model_properties (BobguiButton              *button,
                  BobguiInspectorPropEditor *self)
{
  GObject *model;

  model = g_object_get_data (G_OBJECT (button), "model");
  g_signal_emit (self, signals[SHOW_OBJECT], 0, model, "model", "data");
}

static void
attribute_mapping_changed (BobguiDropDown            *dropdown,
                           GParamSpec             *pspec,
                           BobguiInspectorPropEditor *self)
{
  int col;
  gpointer layout;
  BobguiCellRenderer *cell;
  BobguiCellArea *area;

  col = bobgui_drop_down_get_selected (dropdown) - 1;
  layout = g_object_get_data (self->object, "bobgui-inspector-cell-layout");
  if (BOBGUI_IS_CELL_LAYOUT (layout))
    {
      cell = BOBGUI_CELL_RENDERER (self->object);
      area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (layout));
      bobgui_cell_area_attribute_disconnect (area, cell, self->name);
      if (col != -1)
        bobgui_cell_area_attribute_connect (area, cell, self->name, col);
      bobgui_widget_set_sensitive (self->self, col == -1);
      notify_property (self->object, find_property (self));
      bobgui_widget_queue_draw (bobgui_cell_layout_get_widget (BOBGUI_CELL_LAYOUT (layout)));
    }
}

#define ATTRIBUTE_TYPE_HOLDER (attribute_holder_get_type ())
G_DECLARE_FINAL_TYPE (AttributeHolder, attribute_holder, ATTRIBUTE, HOLDER, GObject)

struct _AttributeHolder {
  GObject parent_instance;
  int column;
  gboolean sensitive;
};

G_DEFINE_TYPE (AttributeHolder, attribute_holder, G_TYPE_OBJECT);

static void
attribute_holder_init (AttributeHolder *holder)
{
}

static void
attribute_holder_class_init (AttributeHolderClass *class)
{
}

static AttributeHolder *
attribute_holder_new (int      column,
                      gboolean sensitive)
{
  AttributeHolder *holder = g_object_new (ATTRIBUTE_TYPE_HOLDER, NULL);
  holder->column = column;
  holder->sensitive = sensitive;
  return holder;
}

static void
attribute_setup_item (BobguiSignalListItemFactory *factory,
                      BobguiListItem              *item)
{
  BobguiWidget *label;

  label = bobgui_label_new ("");
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.0);

  bobgui_list_item_set_child (item, label);
}

static void
attribute_bind_item (BobguiSignalListItemFactory *factory,
                     BobguiListItem              *item)
{
  BobguiWidget *label;
  AttributeHolder *holder;

  holder = bobgui_list_item_get_item (item);
  label = bobgui_list_item_get_child (item);

  if (holder->column >= 0)
    {
      char *text = g_strdup_printf ("%d", holder->column);
      bobgui_label_set_label (BOBGUI_LABEL (label), text);
      g_free (text);
    }
  else
    bobgui_label_set_label (BOBGUI_LABEL (label), C_("column number", "None"));

  bobgui_list_item_set_selectable (item, holder->sensitive);
  bobgui_widget_set_sensitive (label, holder->sensitive);
}

static BobguiWidget *
attribute_editor (GObject                *object,
                  GParamSpec             *spec,
                  BobguiInspectorPropEditor *self)
{
  gpointer layout;
  BobguiCellArea *area;
  BobguiTreeModel *model = NULL;
  int col = -1;
  BobguiWidget *label;
  BobguiWidget *button;
  BobguiWidget *box;
  BobguiWidget *dropdown;
  GListStore *store;
  BobguiListItemFactory *factory;
  int i;
  AttributeHolder *holder;
  gboolean sensitive;

  layout = g_object_get_data (object, "bobgui-inspector-cell-layout");
  if (BOBGUI_IS_CELL_LAYOUT (layout))
    {
      area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (layout));
      col = bobgui_cell_area_attribute_get_column (area,
                                                BOBGUI_CELL_RENDERER (object),
                                                self->name);
      model = bobgui_cell_layout_get_model (BOBGUI_CELL_LAYOUT (layout));
    }

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);

  label = bobgui_label_new (_("Attribute:"));
  bobgui_box_append (BOBGUI_BOX (box), label);

  button = bobgui_button_new_with_label (_("Model"));
  g_object_set_data (G_OBJECT (button), "model", model);
  g_signal_connect (button, "clicked", G_CALLBACK (model_properties), self);
  bobgui_box_append (BOBGUI_BOX (box), button);

  bobgui_box_append (BOBGUI_BOX (box), bobgui_label_new (_("Column:")));
  dropdown = bobgui_drop_down_new (NULL, NULL);

  store = g_list_store_new (ATTRIBUTE_TYPE_HOLDER);
  holder = attribute_holder_new (-1, TRUE);
  g_list_store_append (store, holder);
  g_object_unref (holder);

  for (i = 0; i < bobgui_tree_model_get_n_columns (model); i++)
    {
      sensitive = g_value_type_transformable (bobgui_tree_model_get_column_type (model, i),
                                              spec->value_type);
      holder = attribute_holder_new (i, sensitive);
      g_list_store_append (store, holder);
      g_object_unref (holder);
    }
  bobgui_drop_down_set_model (BOBGUI_DROP_DOWN (dropdown), G_LIST_MODEL (store));
  g_object_unref (store);

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (attribute_setup_item), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (attribute_bind_item), NULL);
  bobgui_drop_down_set_factory (BOBGUI_DROP_DOWN (dropdown), factory);
  g_object_unref (factory);

  bobgui_drop_down_set_selected (BOBGUI_DROP_DOWN (dropdown), col + 1);
  attribute_mapping_changed (BOBGUI_DROP_DOWN (dropdown), NULL, self);
  g_signal_connect (dropdown, "notify::selected",
                    G_CALLBACK (attribute_mapping_changed), self);

  bobgui_box_append (BOBGUI_BOX (box), dropdown);

  return box;
}

static GObject *
find_action_owner (BobguiActionable *actionable)
{
  BobguiWidget *widget = BOBGUI_WIDGET (actionable);
  const char *full_name;
  BobguiWidget *win;

  full_name = bobgui_actionable_get_action_name (actionable);
  if (!full_name)
    return NULL;

  win = bobgui_widget_get_ancestor (widget, BOBGUI_TYPE_APPLICATION_WINDOW);
  if (g_str_has_prefix (full_name, "win.") == 0)
    {
      if (G_IS_OBJECT (win))
        return (GObject *)win;
    }
  else if (g_str_has_prefix (full_name, "app.") == 0)
    {
      if (BOBGUI_IS_WINDOW (win))
        return (GObject *)bobgui_window_get_application (BOBGUI_WINDOW (win));
    }

  while (widget != NULL)
    {
      BobguiActionMuxer *muxer;

      muxer = _bobgui_widget_get_action_muxer (widget, FALSE);
      if (muxer && bobgui_action_muxer_find (muxer, full_name, NULL))
        return (GObject *)widget;

      widget = bobgui_widget_get_parent (widget);
    }

  return NULL;
}

static void
show_action_owner (BobguiButton              *button,
                   BobguiInspectorPropEditor *self)
{
  GObject *owner;

  owner = g_object_get_data (G_OBJECT (button), "owner");
  g_signal_emit (self, signals[SHOW_OBJECT], 0, owner, NULL, "actions");
}

static BobguiWidget *
action_editor (GObject                *object,
               BobguiInspectorPropEditor *self)
{
  BobguiWidget *box;
  BobguiWidget *button;
  GObject *owner;
  char *text;

  owner = find_action_owner (BOBGUI_ACTIONABLE (object));

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  if (owner)
    {
      /* Translators: %s is a type name, for example
       * Action from 0x2345678 (BobguiApplicationWindow)
       */
      text = g_strdup_printf (_("Action from: %p (%s)"),
                              owner, g_type_name_from_instance ((GTypeInstance *)owner));
      bobgui_box_append (BOBGUI_BOX (box), bobgui_label_new (text));
      g_free (text);
      button = bobgui_button_new_with_label (_("Properties"));
      g_object_set_data (G_OBJECT (button), "owner", owner);
      g_signal_connect (button, "clicked",
                        G_CALLBACK (show_action_owner), self);
      bobgui_box_append (BOBGUI_BOX (box), button);
    }

  return box;
}

static void
add_attribute_info (BobguiInspectorPropEditor *self,
                    GParamSpec             *spec)
{
  if (BOBGUI_IS_CELL_RENDERER (self->object))
    bobgui_box_append (BOBGUI_BOX (self),
                    attribute_editor (self->object, spec, self));
}

static void
add_actionable_info (BobguiInspectorPropEditor *self)
{
  if (BOBGUI_IS_ACTIONABLE (self->object) &&
      g_strcmp0 (self->name, "action-name") == 0)
    bobgui_box_append (BOBGUI_BOX (self),
                    action_editor (self->object, self));
}

static void
reset_setting (BobguiInspectorPropEditor *self)
{
  bobgui_settings_reset_property (BOBGUI_SETTINGS (self->object), self->name);
}

static void
add_bobgui_settings_info (BobguiInspectorPropEditor *self)
{
  GObject *object;
  const char *name;
  BobguiWidget *row;
  const char *source;
  BobguiWidget *button;

  object = self->object;
  name = self->name;

  if (!BOBGUI_IS_SETTINGS (object))
    return;

  row = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);

  button = bobgui_button_new_with_label (_("Reset"));
  bobgui_box_append (BOBGUI_BOX (row), button);
  bobgui_widget_set_sensitive (button, FALSE);
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (reset_setting), self);

  switch (_bobgui_settings_get_setting_source (BOBGUI_SETTINGS (object), name))
    {
    case BOBGUI_SETTINGS_SOURCE_DEFAULT:
      source = C_("BobguiSettings source", "Default");
      break;
    case BOBGUI_SETTINGS_SOURCE_THEME:
      source = C_("BobguiSettings source", "Theme");
      break;
    case BOBGUI_SETTINGS_SOURCE_XSETTING:
      source = C_("BobguiSettings source", "XSettings");
      break;
    case BOBGUI_SETTINGS_SOURCE_APPLICATION:
      bobgui_widget_set_sensitive (button, TRUE);
      source = C_("BobguiSettings source", "Application");
      break;
    default:
      source = C_("BobguiSettings source", "Unknown");
      break;
    }
  bobgui_box_append (BOBGUI_BOX (row), bobgui_label_new (_("Source:")));
  bobgui_box_append (BOBGUI_BOX (row), bobgui_label_new (source));

  bobgui_box_append (BOBGUI_BOX (self), row);
}

static void
readonly_changed (GObject    *object,
                  GParamSpec *spec,
                  gpointer    data)
{
  GValue gvalue = {0};
  char *value;
  char *type;

  g_value_init (&gvalue, spec->value_type);
  g_object_get_property (object, spec->name, &gvalue);
  strdup_value_contents (&gvalue, &value, &type);

  bobgui_label_set_label (BOBGUI_LABEL (data), value);

  g_value_unset (&gvalue);
  g_free (value);
  g_free (type);
}

static void
constructed (GObject *object)
{
  BobguiInspectorPropEditor *self = BOBGUI_INSPECTOR_PROP_EDITOR (object);
  GParamSpec *spec;
  BobguiWidget *label;
  gboolean can_modify;
  BobguiWidget *box;

  spec = find_property (self);

  can_modify = ((spec->flags & G_PARAM_WRITABLE) != 0 &&
                (spec->flags & G_PARAM_CONSTRUCT_ONLY) == 0);

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);

  if ((spec->flags & G_PARAM_CONSTRUCT_ONLY) != 0)
    label = bobgui_label_new ("(construct-only)");
  else if ((spec->flags & G_PARAM_WRITABLE) == 0)
    label = bobgui_label_new ("(not writable)");
  else
    label = NULL;

  if (label)
    {
      bobgui_widget_add_css_class (label, "dim-label");
      bobgui_box_append (BOBGUI_BOX (box), label);
    }

  /* By reaching this, we already know the property is readable.
   * Since all we can do for a GObject is dive down into it's properties
   * and inspect bindings and such, pretend to be mutable.
   */
  if (g_type_is_a (spec->value_type, G_TYPE_OBJECT))
    can_modify = TRUE;

  if (!can_modify)
    {
      label = bobgui_label_new ("");
      bobgui_label_set_ellipsize (BOBGUI_LABEL (label), PANGO_ELLIPSIZE_END);
      bobgui_label_set_max_width_chars (BOBGUI_LABEL (label), 20);
      bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.0);
      bobgui_widget_set_hexpand (label, TRUE);
      bobgui_widget_set_halign (label, BOBGUI_ALIGN_FILL);
      bobgui_box_append (BOBGUI_BOX (box), label);

      readonly_changed (self->object, spec, label);
      g_object_connect_property (self->object, spec,
                                 G_CALLBACK (readonly_changed),
                                 label, G_OBJECT (label));

      if (self->size_group)
        bobgui_size_group_add_widget (self->size_group, box);
      bobgui_box_append (BOBGUI_BOX (self), box);
      return;
    }

  self->self = property_editor (self->object, spec, self);
  bobgui_box_append (BOBGUI_BOX (box), self->self);
  if (self->size_group)
    bobgui_size_group_add_widget (self->size_group, box);
  bobgui_box_append (BOBGUI_BOX (self), box);

  add_attribute_info (self, spec);
  add_actionable_info (self);
  add_bobgui_settings_info (self);
}

static void
finalize (GObject *object)
{
  BobguiInspectorPropEditor *self = BOBGUI_INSPECTOR_PROP_EDITOR (object);

  g_free (self->name);

  G_OBJECT_CLASS (bobgui_inspector_prop_editor_parent_class)->finalize (object);
}

static void
get_property (GObject    *object,
              guint       param_id,
              GValue     *value,
              GParamSpec *pspec)
{
  BobguiInspectorPropEditor *self = BOBGUI_INSPECTOR_PROP_EDITOR (object);

  switch (param_id)
    {
    case PROP_OBJECT:
      g_value_set_object (value, self->object);
      break;

    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;

    case PROP_SIZE_GROUP:
      g_value_set_object (value, self->size_group);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
set_property (GObject      *object,
              guint         param_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  BobguiInspectorPropEditor *self = BOBGUI_INSPECTOR_PROP_EDITOR (object);

  switch (param_id)
    {
    case PROP_OBJECT:
      self->object = g_value_get_object (value);
      break;

    case PROP_NAME:
      g_free (self->name);
      self->name = g_value_dup_string (value);
      break;

    case PROP_SIZE_GROUP:
      self->size_group = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }
}

static void
bobgui_inspector_prop_editor_class_init (BobguiInspectorPropEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->constructed = constructed;
  object_class->finalize = finalize;
  object_class->get_property = get_property;
  object_class->set_property = set_property;

  widget_class->focus = bobgui_widget_focus_child;
  widget_class->grab_focus = bobgui_widget_grab_focus_child;

  signals[SHOW_OBJECT] =
    g_signal_new ("show-object",
                  G_TYPE_FROM_CLASS (object_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 3, G_TYPE_OBJECT, G_TYPE_STRING, G_TYPE_STRING);

  g_object_class_install_property (object_class, PROP_OBJECT,
      g_param_spec_object ("object", NULL, NULL,
                           G_TYPE_OBJECT, G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_NAME,
      g_param_spec_string ("name", NULL, NULL,
                           NULL, G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_SIZE_GROUP,
      g_param_spec_object ("size-group", NULL, NULL,
                           BOBGUI_TYPE_SIZE_GROUP, G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
}

BobguiWidget *
bobgui_inspector_prop_editor_new (GObject      *object,
                               const char   *name,
                               BobguiSizeGroup *values)
{
  return g_object_new (BOBGUI_TYPE_INSPECTOR_PROP_EDITOR,
                       "object", object,
                       "name", name,
                       "size-group", values,
                       NULL);
}

gboolean
bobgui_inspector_prop_editor_should_expand (BobguiInspectorPropEditor *self)
{
  if (BOBGUI_IS_SCROLLED_WINDOW (self->self))
    {
      BobguiPolicyType policy;

      g_object_get (self->self, "vscrollbar-policy", &policy, NULL);
      if (policy != BOBGUI_POLICY_NEVER)
        return TRUE;
    }

  return FALSE;
}


// vim: set et:
