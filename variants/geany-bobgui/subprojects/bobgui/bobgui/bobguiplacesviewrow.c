/* bobguiplacesviewrow.c
 *
 * Copyright (C) 2015 Georges Basile Stavracas Neto <georges.stavracas@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gio/gio.h>

#include "bobguiplacesviewrowprivate.h"

/* As this widget is shared with Nautilus, we use this guard to
 * ensure that internally we only include the files that we need
 * instead of including bobgui.h
 */
#ifdef BOBGUI_COMPILATION
#include "bobguibutton.h"
#include "bobguigesture.h"
#include "bobguiimage.h"
#include <glib/gi18n-lib.h>
#include "bobguilabel.h"
#include "bobguispinner.h"
#include "bobguistack.h"
#include "bobguitypebuiltins.h"
#include "bobguinative.h"
#include "bobguipopover.h"
#else
#include <bobgui/bobgui.h>
#endif

struct _BobguiPlacesViewRow
{
  BobguiListBoxRow  parent_instance;

  BobguiLabel      *available_space_label;
  BobguiStack      *mount_stack;
  BobguiSpinner    *busy_spinner;
  BobguiButton     *eject_button;
  BobguiImage      *eject_icon;
  BobguiImage      *icon_image;
  BobguiLabel      *name_label;
  BobguiLabel      *path_label;

  GVolume       *volume;
  GMount        *mount;
  GFile         *file;

  GCancellable  *cancellable;

  int            is_network : 1;
};

G_DEFINE_TYPE (BobguiPlacesViewRow, bobgui_places_view_row, BOBGUI_TYPE_LIST_BOX_ROW)

enum {
  PROP_0,
  PROP_ICON,
  PROP_NAME,
  PROP_PATH,
  PROP_VOLUME,
  PROP_MOUNT,
  PROP_FILE,
  PROP_IS_NETWORK,
  LAST_PROP
};

static GParamSpec *properties [LAST_PROP];

static void
measure_available_space_finished (GObject      *object,
                                  GAsyncResult *res,
                                  gpointer      user_data)
{
  BobguiPlacesViewRow *row = user_data;
  GFileInfo *info;
  GError *error;
  guint64 free_space;
  guint64 total_space;
  char *formatted_free_size;
  char *formatted_total_size;
  char *label;
  guint plural_form;

  error = NULL;

  info = g_file_query_filesystem_info_finish (G_FILE (object),
                                              res,
                                              &error);

  if (error)
    {
      if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED) &&
          !g_error_matches (error, G_IO_ERROR, G_IO_ERROR_NOT_MOUNTED))
        {
          g_warning ("Failed to measure available space: %s", error->message);
        }

      g_clear_error (&error);
      goto out;
    }

  if (!g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_FILESYSTEM_FREE) ||
      !g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_FILESYSTEM_SIZE))
    {
      g_object_unref (info);
      goto out;
    }

  free_space = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
  total_space = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_FILESYSTEM_SIZE);

  formatted_free_size = g_format_size (free_space);
  formatted_total_size = g_format_size (total_space);

  /* read g_format_size code in glib for further understanding */
  plural_form = free_space < 1000 ? free_space : free_space % 1000 + 1000;

  /* Translators: respectively, free and total space of the drive. The plural form
   * should be based on the free space available.
   * i.e. 1 GB / 24 GB available.
   */
  label = g_strdup_printf (dngettext (GETTEXT_PACKAGE, "%s / %s available", "%s / %s available", plural_form),
                           formatted_free_size, formatted_total_size);

  bobgui_label_set_label (row->available_space_label, label);

  g_object_unref (info);
  g_free (formatted_total_size);
  g_free (formatted_free_size);
  g_free (label);
out:
  g_object_unref (object);
}

static void
measure_available_space (BobguiPlacesViewRow *row)
{
  gboolean should_measure;

  should_measure = (!row->is_network && (row->volume || row->mount || row->file));

  bobgui_label_set_label (row->available_space_label, "");
  bobgui_widget_set_visible (BOBGUI_WIDGET (row->available_space_label), should_measure);

  if (should_measure)
    {
      GFile *file = NULL;

      if (row->file)
        {
          file = g_object_ref (row->file);
        }
      else if (row->mount)
        {
          file = g_mount_get_root (row->mount);
        }
      else if (row->volume)
        {
          GMount *mount;

          mount = g_volume_get_mount (row->volume);

          if (mount)
            file = g_mount_get_root (row->mount);

          g_clear_object (&mount);
        }

      if (file)
        {
          g_cancellable_cancel (row->cancellable);
          g_clear_object (&row->cancellable);
          row->cancellable = g_cancellable_new ();

          g_file_query_filesystem_info_async (file,
                                              G_FILE_ATTRIBUTE_FILESYSTEM_FREE "," G_FILE_ATTRIBUTE_FILESYSTEM_SIZE,
                                              G_PRIORITY_DEFAULT,
                                              row->cancellable,
                                              measure_available_space_finished,
                                              row);
        }
    }
}

static void
bobgui_places_view_row_finalize (GObject *object)
{
  BobguiPlacesViewRow *self = BOBGUI_PLACES_VIEW_ROW (object);

  g_cancellable_cancel (self->cancellable);

  g_clear_object (&self->volume);
  g_clear_object (&self->mount);
  g_clear_object (&self->file);
  g_clear_object (&self->cancellable);

  G_OBJECT_CLASS (bobgui_places_view_row_parent_class)->finalize (object);
}

static void
bobgui_places_view_row_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  BobguiPlacesViewRow *self;

  self = BOBGUI_PLACES_VIEW_ROW (object);

  switch (prop_id)
    {
    case PROP_ICON:
      g_value_set_object (value, bobgui_image_get_gicon (self->icon_image));
      break;

    case PROP_NAME:
      g_value_set_string (value, bobgui_label_get_label (self->name_label));
      break;

    case PROP_PATH:
      g_value_set_string (value, bobgui_label_get_label (self->path_label));
      break;

    case PROP_VOLUME:
      g_value_set_object (value, self->volume);
      break;

    case PROP_MOUNT:
      g_value_set_object (value, self->mount);
      break;

    case PROP_FILE:
      g_value_set_object (value, self->file);
      break;

    case PROP_IS_NETWORK:
      g_value_set_boolean (value, self->is_network);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_places_view_row_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  BobguiPlacesViewRow *self = BOBGUI_PLACES_VIEW_ROW (object);

  switch (prop_id)
    {
    case PROP_ICON:
      bobgui_image_set_from_gicon (self->icon_image, g_value_get_object (value));
      break;

    case PROP_NAME:
      bobgui_label_set_label (self->name_label, g_value_get_string (value));
      break;

    case PROP_PATH:
      bobgui_label_set_label (self->path_label, g_value_get_string (value));
      break;

    case PROP_VOLUME:
      g_set_object (&self->volume, g_value_get_object (value));
      break;

    case PROP_MOUNT:
      g_set_object (&self->mount, g_value_get_object (value));
      if (self->mount != NULL)
        {
          bobgui_stack_set_visible_child (self->mount_stack, BOBGUI_WIDGET (self->eject_button));
          bobgui_widget_set_child_visible (BOBGUI_WIDGET (self->mount_stack), TRUE);
        }
      else
        {
          bobgui_widget_set_child_visible (BOBGUI_WIDGET (self->mount_stack), FALSE);
        }
      measure_available_space (self);
      break;

    case PROP_FILE:
      g_set_object (&self->file, g_value_get_object (value));
      measure_available_space (self);
      break;

    case PROP_IS_NETWORK:
      bobgui_places_view_row_set_is_network (self, g_value_get_boolean (value));
      measure_available_space (self);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_places_view_row_size_allocate (BobguiWidget *widget,
                                   int        width,
                                   int        height,
                                   int        baseline)
{
  BobguiWidget *menu = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (widget), "menu"));

  BOBGUI_WIDGET_CLASS (bobgui_places_view_row_parent_class)->size_allocate (widget, width, height, baseline);
  if (menu)
    bobgui_popover_present (BOBGUI_POPOVER (menu));
}

static void
bobgui_places_view_row_class_init (BobguiPlacesViewRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->finalize = bobgui_places_view_row_finalize;
  object_class->get_property = bobgui_places_view_row_get_property;
  object_class->set_property = bobgui_places_view_row_set_property;

  widget_class->size_allocate = bobgui_places_view_row_size_allocate;

  properties[PROP_ICON] =
          g_param_spec_object ("icon", NULL, NULL,
                               G_TYPE_ICON,
                               G_PARAM_READWRITE);

  properties[PROP_NAME] =
          g_param_spec_string ("name", NULL, NULL,
                               "",
                               G_PARAM_READWRITE);

  properties[PROP_PATH] =
          g_param_spec_string ("path", NULL, NULL,
                               "",
                               G_PARAM_READWRITE);

  properties[PROP_VOLUME] =
          g_param_spec_object ("volume", NULL, NULL,
                               G_TYPE_VOLUME,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  properties[PROP_MOUNT] =
          g_param_spec_object ("mount", NULL, NULL,
                               G_TYPE_MOUNT,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  properties[PROP_FILE] =
          g_param_spec_object ("file", NULL, NULL,
                               G_TYPE_FILE,
                               G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  properties[PROP_IS_NETWORK] =
          g_param_spec_boolean ("is-network", NULL, NULL,
                                FALSE,
                                G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, LAST_PROP, properties);

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/ui/bobguiplacesviewrow.ui");

  bobgui_widget_class_bind_template_child (widget_class, BobguiPlacesViewRow, available_space_label);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPlacesViewRow, mount_stack);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPlacesViewRow, busy_spinner);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPlacesViewRow, eject_button);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPlacesViewRow, eject_icon);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPlacesViewRow, icon_image);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPlacesViewRow, name_label);
  bobgui_widget_class_bind_template_child (widget_class, BobguiPlacesViewRow, path_label);
}

static void
bobgui_places_view_row_init (BobguiPlacesViewRow *self)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (self));
}

BobguiWidget*
bobgui_places_view_row_new (GVolume *volume,
                         GMount  *mount)
{
  return g_object_new (BOBGUI_TYPE_PLACES_VIEW_ROW,
                       "volume", volume,
                       "mount", mount,
                       NULL);
}

GMount*
bobgui_places_view_row_get_mount (BobguiPlacesViewRow *row)
{
  g_return_val_if_fail (BOBGUI_IS_PLACES_VIEW_ROW (row), NULL);

  return row->mount;
}

GVolume*
bobgui_places_view_row_get_volume (BobguiPlacesViewRow *row)
{
  g_return_val_if_fail (BOBGUI_IS_PLACES_VIEW_ROW (row), NULL);

  return row->volume;
}

GFile*
bobgui_places_view_row_get_file (BobguiPlacesViewRow *row)
{
  g_return_val_if_fail (BOBGUI_IS_PLACES_VIEW_ROW (row), NULL);

  return row->file;
}

BobguiWidget*
bobgui_places_view_row_get_eject_button (BobguiPlacesViewRow *row)
{
  g_return_val_if_fail (BOBGUI_IS_PLACES_VIEW_ROW (row), NULL);

  return BOBGUI_WIDGET (row->eject_button);
}

void
bobgui_places_view_row_set_busy (BobguiPlacesViewRow *row,
                              gboolean          is_busy)
{
  g_return_if_fail (BOBGUI_IS_PLACES_VIEW_ROW (row));

  if (is_busy)
    {
      bobgui_stack_set_visible_child (row->mount_stack, BOBGUI_WIDGET (row->busy_spinner));
      bobgui_widget_set_child_visible (BOBGUI_WIDGET (row->mount_stack), TRUE);
      bobgui_spinner_start (row->busy_spinner);
    }
  else
    {
      bobgui_widget_set_child_visible (BOBGUI_WIDGET (row->mount_stack), FALSE);
      bobgui_spinner_stop (row->busy_spinner);
    }
}

gboolean
bobgui_places_view_row_get_is_network (BobguiPlacesViewRow *row)
{
  g_return_val_if_fail (BOBGUI_IS_PLACES_VIEW_ROW (row), FALSE);

  return row->is_network;
}

void
bobgui_places_view_row_set_is_network (BobguiPlacesViewRow *row,
                                    gboolean          is_network)
{
  if (row->is_network != is_network)
    {
      row->is_network = is_network;

      bobgui_image_set_from_icon_name (row->eject_icon, "media-eject-symbolic");
      bobgui_widget_set_tooltip_text (BOBGUI_WIDGET (row->eject_button), is_network ? _("Disconnect") : _("Unmount"));
    }
}

void
bobgui_places_view_row_set_path_size_group (BobguiPlacesViewRow *row,
                                         BobguiSizeGroup     *group)
{
  if (group)
    bobgui_size_group_add_widget (group, BOBGUI_WIDGET (row->path_label));
}

void
bobgui_places_view_row_set_space_size_group (BobguiPlacesViewRow *row,
                                          BobguiSizeGroup     *group)
{
  if (group)
    bobgui_size_group_add_widget (group, BOBGUI_WIDGET (row->available_space_label));
}
