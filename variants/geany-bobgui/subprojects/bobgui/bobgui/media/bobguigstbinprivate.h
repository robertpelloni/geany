/*
 * Copyright (C) 2021 Red Hat, Inc
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __BOBGUI_GST_BIN_PRIVATE_H__
#define __BOBGUI_GST_BIN_PRIVATE_H__

#include <gio/gio.h>
#include <gst/gst.h>

#define BOBGUI_TYPE_GST_BIN (bobgui_gst_bin_get_type ())
G_DECLARE_FINAL_TYPE (BobguiGstBin, bobgui_gst_bin, BOBGUI, GST_BIN, GstBin);

void
bobgui_gst_bin_set_stream (BobguiGstBin    *bin,
                        GInputStream *stream);

#endif /* __BOBGUI_GST_BIN_PRIVATE_H__ */
