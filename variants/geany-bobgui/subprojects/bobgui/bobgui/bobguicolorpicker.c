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

#include "bobguicolorpickerprivate.h"
#include "bobguicolorpickerportalprivate.h"
#include "bobguicolorpickershellprivate.h"
#include "bobguicolorpickerkwinprivate.h"

#ifdef __APPLE__
#include "bobguicolorpickerquartzprivate.h"
#endif

#ifdef G_OS_WIN32
#include "bobguicolorpickerwin32private.h"
#endif

#include <gio/gio.h>


G_DEFINE_INTERFACE_WITH_CODE (BobguiColorPicker, bobgui_color_picker, G_TYPE_OBJECT,
                              g_type_interface_add_prerequisite (g_define_type_id, G_TYPE_INITABLE);)

static void
bobgui_color_picker_default_init (BobguiColorPickerInterface *iface)
{
}

void
bobgui_color_picker_pick (BobguiColorPicker      *picker,
                       GAsyncReadyCallback  callback,
                       gpointer             user_data)
{
  BOBGUI_COLOR_PICKER_GET_INTERFACE (picker)->pick (picker, callback, user_data);
}

GdkRGBA *
bobgui_color_picker_pick_finish (BobguiColorPicker  *picker,
                              GAsyncResult    *res,
                              GError         **error)
{
  return BOBGUI_COLOR_PICKER_GET_INTERFACE (picker)->pick_finish (picker, res, error);
}

BobguiColorPicker *
bobgui_color_picker_new (void)
{
  BobguiColorPicker *picker = NULL;

#if defined (G_OS_UNIX) && !defined(__APPLE__)
  if (!picker)
    picker = bobgui_color_picker_portal_new ();
  if (!picker)
    picker = bobgui_color_picker_shell_new ();
  if (!picker)
    picker = bobgui_color_picker_kwin_new ();
#elif defined (__APPLE__)
  if (!picker)
    picker = bobgui_color_picker_quartz_new ();
#elif defined (G_OS_WIN32)
  if (!picker)
    picker = bobgui_color_picker_win32_new ();
#endif

  if (!picker)
    g_debug ("No suitable BobguiColorPicker implementation");
  else
    g_debug ("Using %s for picking colors", G_OBJECT_TYPE_NAME (picker));

  return picker;
}

