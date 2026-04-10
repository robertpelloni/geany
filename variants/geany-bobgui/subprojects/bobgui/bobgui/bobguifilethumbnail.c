/* bobguifilethumbnail.c
 *
 * Copyright 2022 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "bobguifilethumbnail.h"

#include "bobguibinlayout.h"
#include "bobguifilechooserutils.h"
#include "bobguiimage.h"
#include "bobguiprivate.h"
#include "bobguiwidget.h"

#define ICON_SIZE 16

struct _BobguiFileThumbnail
{
  BobguiWidget parent;

  BobguiWidget *image;
  int icon_size;

  GCancellable *cancellable;
  GFileInfo *info;
};

typedef struct
{
  BobguiWidgetClass parent;
} BobguiFileThumbnailClass;

G_DEFINE_FINAL_TYPE (BobguiFileThumbnail, _bobgui_file_thumbnail, BOBGUI_TYPE_WIDGET)

enum {
  PROP_0,
  PROP_ICON_SIZE,
  PROP_INFO,
  N_PROPS,
};

static GParamSpec *properties [N_PROPS];

static void
copy_attribute (GFileInfo  *to,
                GFileInfo  *from,
                const char *attribute)
{
  GFileAttributeType type;
  gpointer value;

  if (g_file_info_get_attribute_data (from, attribute, &type, &value, NULL))
    g_file_info_set_attribute (to, attribute, type, value);
}

static gboolean
update_image (BobguiFileThumbnail *self)
{
  BobguiIconTheme *icon_theme;
  GIcon *icon;
  int icon_size;
  int scale;

  if (!g_file_info_has_attribute (self->info, G_FILE_ATTRIBUTE_STANDARD_ICON))
    {
      bobgui_image_clear (BOBGUI_IMAGE (self->image));
      return FALSE;
    }

  scale = bobgui_widget_get_scale_factor (BOBGUI_WIDGET (self));
  icon_theme = bobgui_icon_theme_get_for_display (bobgui_widget_get_display (BOBGUI_WIDGET (self)));

  icon_size = self->icon_size != -1 ? self->icon_size : ICON_SIZE;
  icon = _bobgui_file_info_get_icon (self->info, icon_size, scale, icon_theme);

  bobgui_image_set_from_gicon (BOBGUI_IMAGE (self->image), icon);

  g_object_unref (icon);

  return TRUE;
}

static void
thumbnail_queried_cb (GObject      *object,
                      GAsyncResult *result,
                      gpointer      user_data)
{
  BobguiFileThumbnail *self = user_data; /* might be unreffed if operation was cancelled */
  GFile *file = G_FILE (object);
  GFileInfo *queried;
  GError *error = NULL;

  queried = g_file_query_info_finish (file, result, &error);

  if (error)
    {
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_file_info_set_attribute_boolean (self->info, "filechooser::queried", TRUE);
      g_clear_error (&error);
      return;
    }

  g_file_info_set_attribute_boolean (self->info, "filechooser::queried", TRUE);

  copy_attribute (self->info, queried, G_FILE_ATTRIBUTE_THUMBNAIL_PATH);
  copy_attribute (self->info, queried, G_FILE_ATTRIBUTE_THUMBNAILING_FAILED);
  copy_attribute (self->info, queried, G_FILE_ATTRIBUTE_STANDARD_ICON);

  update_image (self);

  g_clear_object (&queried);

  g_clear_object (&self->cancellable);
}

static void
cancel_thumbnail (BobguiFileThumbnail *self)
{
  g_cancellable_cancel (self->cancellable);
  g_clear_object (&self->cancellable);
}

static void
get_thumbnail (BobguiFileThumbnail *self)
{
  if (!self->info)
    {
      bobgui_image_clear (BOBGUI_IMAGE (self->image));
      return;
    }

  if (!update_image (self))
    {
      GFile *file;

      if (g_file_info_has_attribute (self->info, "filechooser::queried"))
        return;

      g_assert (self->cancellable == NULL);
      self->cancellable = g_cancellable_new ();

      file = _bobgui_file_info_get_file (self->info);
      g_file_query_info_async (file,
                               G_FILE_ATTRIBUTE_THUMBNAIL_PATH ","
                               G_FILE_ATTRIBUTE_THUMBNAILING_FAILED ","
                               G_FILE_ATTRIBUTE_STANDARD_ICON,
                               G_FILE_QUERY_INFO_NONE,
                               G_PRIORITY_DEFAULT,
                               self->cancellable,
                               thumbnail_queried_cb,
                               self);
    }
}

static void
_bobgui_file_thumbnail_dispose (GObject *object)
{
  BobguiFileThumbnail *self = (BobguiFileThumbnail *)object;

  _bobgui_file_thumbnail_set_info (self, NULL);

  g_clear_pointer (&self->image, bobgui_widget_unparent);

  G_OBJECT_CLASS (_bobgui_file_thumbnail_parent_class)->dispose (object);
}

static void
_bobgui_file_thumbnail_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  BobguiFileThumbnail *self = BOBGUI_FILE_THUMBNAIL (object);

  switch (prop_id)
    {
    case PROP_ICON_SIZE:
      g_value_set_int (value, self->icon_size);
      break;

    case PROP_INFO:
      g_value_set_object (value, self->info);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
_bobgui_file_thumbnail_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  BobguiFileThumbnail *self = BOBGUI_FILE_THUMBNAIL (object);

  switch (prop_id)
    {
    case PROP_ICON_SIZE:
      _bobgui_file_thumbnail_set_icon_size (self, g_value_get_int (value));
      break;

    case PROP_INFO:
      _bobgui_file_thumbnail_set_info (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
_bobgui_file_thumbnail_class_init (BobguiFileThumbnailClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = _bobgui_file_thumbnail_dispose;
  object_class->get_property = _bobgui_file_thumbnail_get_property;
  object_class->set_property = _bobgui_file_thumbnail_set_property;

  properties[PROP_ICON_SIZE] =
    g_param_spec_int ("icon-size", NULL, NULL,
                      -1, G_MAXINT, -1,
                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_INFO] =
    g_param_spec_object ("file-info", NULL, NULL,
                         G_TYPE_FILE_INFO,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  bobgui_widget_class_set_css_name (widget_class, I_("filethumbnail"));

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
}

static void
_bobgui_file_thumbnail_init (BobguiFileThumbnail *self)
{
  self->icon_size = -1;
  self->image = bobgui_image_new ();
  bobgui_widget_set_parent (self->image, BOBGUI_WIDGET (self));
}

GFileInfo *
_bobgui_file_thumbnail_get_info (BobguiFileThumbnail *self)
{
  g_assert (BOBGUI_IS_FILE_THUMBNAIL (self));

  return self->info;
}

void
_bobgui_file_thumbnail_set_info (BobguiFileThumbnail *self,
                              GFileInfo        *info)
{
  g_assert (BOBGUI_IS_FILE_THUMBNAIL (self));
  g_assert (info == NULL || G_IS_FILE_INFO (info));

  if (g_set_object (&self->info, info))
    {
      cancel_thumbnail (self);
      get_thumbnail (self);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INFO]);
    }
}

int
_bobgui_file_thumbnail_get_icon_size (BobguiFileThumbnail *self)
{
  g_assert (BOBGUI_IS_FILE_THUMBNAIL (self));

  return self->icon_size;
}

void
_bobgui_file_thumbnail_set_icon_size (BobguiFileThumbnail *self,
                                   int               icon_size)
{
  g_assert (BOBGUI_IS_FILE_THUMBNAIL (self));
  g_assert (icon_size == -1 || icon_size > 0);

  if (self->icon_size == icon_size)
    return;

  self->icon_size = icon_size;
  if (self->icon_size == -1)
    bobgui_image_set_pixel_size (BOBGUI_IMAGE (self->image), ICON_SIZE);
  else
    bobgui_image_set_pixel_size (BOBGUI_IMAGE (self->image), icon_size);

  cancel_thumbnail (self);
  get_thumbnail (self);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ICON_SIZE]);
}

