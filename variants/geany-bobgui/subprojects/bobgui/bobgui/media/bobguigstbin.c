/*
 * Copyright (C) 2021 Red Hat, Inc.
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

#include "config.h"

#include "bobguigstbinprivate.h"

static GstStaticPadTemplate src_templ = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

struct _BobguiGstBin {
  GstBin parent;

  GstElement *src;
  char *uri;
};

struct _BobguiGstBinClass {
  GstBinClass parent_class;
};

static GstURIType
bobgui_gst_uri_handler_get_type (GType type)
{
  return GST_URI_SRC;
}

static const char * const *
bobgui_gst_uri_handler_get_protocols (GType type)
{
  static const char *protocols[] = { "bobgui-media-stream", NULL };

  return protocols;
}

static char *
bobgui_gst_uri_handler_get_uri (GstURIHandler *handler)
{
  BobguiGstBin *self = BOBGUI_GST_BIN (handler);

  return g_strdup (self->uri);
}

static gboolean
bobgui_gst_uri_handler_set_uri (GstURIHandler  *handler,
                             const char     *uri,
                             GError        **error)
{
  BobguiGstBin *self = BOBGUI_GST_BIN (handler);

  g_set_str (&self->uri, uri);

  return TRUE;
}

static void
bobgui_gst_uri_handler_iface_init (GstURIHandlerInterface *iface)
{
  iface->get_type = bobgui_gst_uri_handler_get_type;
  iface->get_protocols = bobgui_gst_uri_handler_get_protocols;
  iface->get_uri = bobgui_gst_uri_handler_get_uri;
  iface->set_uri = bobgui_gst_uri_handler_set_uri;
}

G_DEFINE_TYPE_WITH_CODE (BobguiGstBin, bobgui_gst_bin, GST_TYPE_BIN,
                         G_IMPLEMENT_INTERFACE (GST_TYPE_URI_HANDLER, bobgui_gst_uri_handler_iface_init))

static void
bobgui_gst_bin_finalize (GObject *object)
{
  BobguiGstBin *self = BOBGUI_GST_BIN (object);

  g_free (self->uri);

  G_OBJECT_CLASS (bobgui_gst_bin_parent_class)->finalize (object);
}

static void
bobgui_gst_bin_init (BobguiGstBin *self)
{
  GstPadTemplate *templ;
  GstPad *pad;

  self->src = gst_element_factory_make ("giostreamsrc", "src");
  gst_bin_add (GST_BIN (self), self->src);

  templ = gst_element_get_pad_template (GST_ELEMENT (self), "src");
  pad = gst_element_get_static_pad (self->src, "src");
  gst_element_add_pad (GST_ELEMENT (self),
                       gst_ghost_pad_new_from_template ("src", pad, templ));
  gst_object_unref (pad);
}

static void
bobgui_gst_bin_class_init (BobguiGstBinClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = bobgui_gst_bin_finalize;

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (class),
                                         "BobguiGstBin",
                                         "Source", "Handles BobguiMediaFile sources",
                                         "Matthias Clasen");

  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (class), &src_templ);
}

void
bobgui_gst_bin_set_stream (BobguiGstBin    *bin,
                        GInputStream *stream)
{
  g_object_set (bin->src, "stream", stream, NULL);
}
