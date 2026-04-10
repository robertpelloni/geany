/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2018, Red Hat, Inc
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

#include "bobguicolorpickerkwinprivate.h"
#include <gio/gio.h>

struct _BobguiColorPickerKwin
{
  GObject parent_instance;

  GDBusProxy *kwin_proxy;
  GTask *task;
};

struct _BobguiColorPickerKwinClass
{
  GObjectClass parent_class;
};

static GInitableIface *initable_parent_iface;
static void bobgui_color_picker_kwin_initable_iface_init (GInitableIface *iface);
static void bobgui_color_picker_kwin_iface_init (BobguiColorPickerInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiColorPickerKwin, bobgui_color_picker_kwin, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, bobgui_color_picker_kwin_initable_iface_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_COLOR_PICKER, bobgui_color_picker_kwin_iface_init))

static gboolean
bobgui_color_picker_kwin_initable_init (GInitable     *initable,
                                      GCancellable  *cancellable,
                                      GError       **error)
{
  BobguiColorPickerKwin *picker = BOBGUI_COLOR_PICKER_KWIN (initable);
  char *owner;

  picker->kwin_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                      G_DBUS_PROXY_FLAGS_NONE,
                                                      NULL,
                                                      "org.kde.KWin",
                                                      "/ColorPicker",
                                                      "org.kde.kwin.ColorPicker",
                                                      NULL,
                                                      error);

  if (picker->kwin_proxy == NULL)
    {
      g_debug ("Failed to create kwin colorpicker proxy");
      return FALSE;
    }

  owner = g_dbus_proxy_get_name_owner (picker->kwin_proxy);
  if (owner == NULL)
    {
      g_debug ("org.kde.kwin.ColorPicker not provided");
      g_clear_object (&picker->kwin_proxy);
      return FALSE;
    }
  g_free (owner);

  return TRUE;
}

static void
bobgui_color_picker_kwin_initable_iface_init (GInitableIface *iface)
{
  initable_parent_iface = g_type_interface_peek_parent (iface);
  iface->init = bobgui_color_picker_kwin_initable_init;
}

static void
bobgui_color_picker_kwin_init (BobguiColorPickerKwin *picker)
{
}

static void
bobgui_color_picker_kwin_finalize (GObject *object)
{
  BobguiColorPickerKwin *picker = BOBGUI_COLOR_PICKER_KWIN (object);

  g_clear_object (&picker->kwin_proxy);

  G_OBJECT_CLASS (bobgui_color_picker_kwin_parent_class)->finalize (object);
}

static void
bobgui_color_picker_kwin_class_init (BobguiColorPickerKwinClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = bobgui_color_picker_kwin_finalize;
}

BobguiColorPicker *
bobgui_color_picker_kwin_new (void)
{
  return BOBGUI_COLOR_PICKER (g_initable_new (BOBGUI_TYPE_COLOR_PICKER_KWIN, NULL, NULL, NULL));
}

static void
color_picked (GObject      *source,
              GAsyncResult *res,
              gpointer      data)
{
  BobguiColorPickerKwin *picker = BOBGUI_COLOR_PICKER_KWIN (data);
  GError *error = NULL;
  GVariant *ret;

  ret = g_dbus_proxy_call_finish (picker->kwin_proxy, res, &error);

  if (ret == NULL)
    {
      g_task_return_error (picker->task, error);
    }
  else
    {
      GdkRGBA c;
      guint32 color;

      g_variant_get (ret, "(u)", &color);

      c.blue  = ( color        & 0xff) / 255.0;
      c.green = ((color >>  8) & 0xff) / 255.0; 
      c.red   = ((color >> 16) & 0xff) / 255.0; 
      c.alpha = ((color >> 24) & 0xff) / 255.0; 

      g_task_return_pointer (picker->task, gdk_rgba_copy (&c), (GDestroyNotify)gdk_rgba_free);

      g_variant_unref (ret);
    }

  g_clear_object (&picker->task);
}

static void
bobgui_color_picker_kwin_pick (BobguiColorPicker      *cp,
                            GAsyncReadyCallback  callback,
                            gpointer             user_data)
{
  BobguiColorPickerKwin *picker = BOBGUI_COLOR_PICKER_KWIN (cp);

  if (picker->task)
    return;

  picker->task = g_task_new (picker, NULL, callback, user_data);

  g_dbus_proxy_call (picker->kwin_proxy,
                     "pick",
                     NULL,
                     0,
                     -1,
                     NULL,
                     color_picked,
                     picker);
}

static GdkRGBA *
bobgui_color_picker_kwin_pick_finish (BobguiColorPicker  *cp,
                                   GAsyncResult    *res,
                                   GError         **error)
{
  g_return_val_if_fail (g_task_is_valid (res, cp), NULL);

  return g_task_propagate_pointer (G_TASK (res), error);
}

static void
bobgui_color_picker_kwin_iface_init (BobguiColorPickerInterface *iface)
{
  iface->pick = bobgui_color_picker_kwin_pick;
  iface->pick_finish = bobgui_color_picker_kwin_pick_finish;
}
