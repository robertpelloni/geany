/*
 * Copyright © 2020 Red Hat, Inc.
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
 * Authors: Matthias Clasen <mclasen@redhat.com>
 */

#include "config.h"

#include "bobguibookmarklist.h"

#include "bobguisettings.h"
#include "bobguiprivate.h"

/**
 * BobguiBookmarkList:
 *
 * A list model that wraps `GBookmarkFile`.
 *
 * It presents a `GListModel` and fills it asynchronously with the
 * `GFileInfo`s returned from that function.
 *
 * The `GFileInfo`s in the list have some attributes in the recent namespace
 * added: `recent::private` (boolean) and `recent:applications` (stringv). They
 * also have the `GFile` referred by the URI in `standard::file` attribute.
 */

enum {
  PROP_0,
  PROP_FILENAME,
  PROP_ATTRIBUTES,
  PROP_IO_PRIORITY,
  PROP_ITEM_TYPE,
  PROP_LOADING,
  PROP_N_ITEMS,

  NUM_PROPERTIES
};

struct _BobguiBookmarkList
{
  GObject parent_instance;

  char *attributes;
  char *filename;
  int io_priority;
  int loading;

  GCancellable *cancellable;
  GFileMonitor *monitor;
  GBookmarkFile *file;

  GSequence *items;
};

struct _BobguiBookmarkListClass
{
  GObjectClass parent_class;
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

static GType
bobgui_bookmark_list_get_item_type (GListModel *list)
{
  return G_TYPE_FILE_INFO;
}

static guint
bobgui_bookmark_list_get_n_items (GListModel *list)
{
  BobguiBookmarkList *self = BOBGUI_BOOKMARK_LIST (list);

  return g_sequence_get_length (self->items);
}

static gpointer
bobgui_bookmark_list_get_item (GListModel *list,
                                guint       position)
{
  BobguiBookmarkList *self = BOBGUI_BOOKMARK_LIST (list);
  GSequenceIter *iter;

  iter = g_sequence_get_iter_at_pos (self->items, position);

  if (g_sequence_iter_is_end (iter))
    return NULL;
  else
    return g_object_ref (g_sequence_get (iter));
}

static void
bobgui_bookmark_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = bobgui_bookmark_list_get_item_type;
  iface->get_n_items = bobgui_bookmark_list_get_n_items;
  iface->get_item = bobgui_bookmark_list_get_item;
}

static void     bobgui_bookmark_list_start_loading (BobguiBookmarkList *self);
static gboolean bobgui_bookmark_list_stop_loading  (BobguiBookmarkList *self);
static void     bookmark_file_changed (GFileMonitor       *monitor,
                                       GFile              *file,
                                       GFile              *other_file,
                                       GFileMonitorEvent   event,
                                       gpointer            data);
static void bobgui_bookmark_list_set_filename (BobguiBookmarkList *self,
                                                const char         *filename);

G_DEFINE_TYPE_WITH_CODE (BobguiBookmarkList, bobgui_bookmark_list, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, bobgui_bookmark_list_model_init))

static void
bobgui_bookmark_list_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  BobguiBookmarkList *self = BOBGUI_BOOKMARK_LIST (object);

  switch (prop_id)
    {
    case PROP_ATTRIBUTES:
      bobgui_bookmark_list_set_attributes (self, g_value_get_string (value));
      break;

    case PROP_IO_PRIORITY:
      bobgui_bookmark_list_set_io_priority (self, g_value_get_int (value));
      break;

    case PROP_FILENAME:
      bobgui_bookmark_list_set_filename (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_bookmark_list_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  BobguiBookmarkList *self = BOBGUI_BOOKMARK_LIST (object);

  switch (prop_id)
    {
    case PROP_ATTRIBUTES:
      g_value_set_string (value, self->attributes);
      break;

    case PROP_FILENAME:
      g_value_set_string (value, self->filename);
      break;

    case PROP_IO_PRIORITY:
      g_value_set_int (value, self->io_priority);
      break;

    case PROP_ITEM_TYPE:
      g_value_set_gtype (value, G_TYPE_FILE_INFO);
      break;

    case PROP_LOADING:
      g_value_set_boolean (value, bobgui_bookmark_list_is_loading (self));
      break;

    case PROP_N_ITEMS:
      g_value_set_uint (value, g_sequence_get_length (self->items));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_bookmark_list_dispose (GObject *object)
{
  BobguiBookmarkList *self = BOBGUI_BOOKMARK_LIST (object);

  bobgui_bookmark_list_stop_loading (self);

  g_clear_pointer (&self->attributes, g_free);
  g_clear_pointer (&self->filename, g_free);
  g_clear_pointer (&self->items, g_sequence_free);
  g_clear_pointer (&self->file, g_bookmark_file_free);

  g_signal_handlers_disconnect_by_func (self->monitor, G_CALLBACK (bookmark_file_changed), self);
  g_clear_object (&self->monitor);

  G_OBJECT_CLASS (bobgui_bookmark_list_parent_class)->dispose (object);
}

static void
bobgui_bookmark_list_class_init (BobguiBookmarkListClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->set_property = bobgui_bookmark_list_set_property;
  gobject_class->get_property = bobgui_bookmark_list_get_property;
  gobject_class->dispose = bobgui_bookmark_list_dispose;

  /**
   * BobguiBookmarkList:filename:
   *
   * The bookmark file to load.
   */
  properties[PROP_FILENAME] =
      g_param_spec_string ("filename", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_EXPLICIT_NOTIFY);
  /**
   * BobguiBookmarkList:attributes:
   *
   * The attributes to query.
   */
  properties[PROP_ATTRIBUTES] =
      g_param_spec_string ("attributes", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiBookmarkList:io-priority:
   *
   * Priority used when loading.
   */
  properties[PROP_IO_PRIORITY] =
      g_param_spec_int ("io-priority", NULL, NULL,
                        -G_MAXINT, G_MAXINT, G_PRIORITY_DEFAULT,
                        BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiBookmarkList:item-type:
   *
   * The type of items. See [method@Gio.ListModel.get_item_type].
   *
   * Since: 4.8
   **/
  properties[PROP_ITEM_TYPE] =
    g_param_spec_gtype ("item-type", NULL, NULL,
                        G_TYPE_FILE_INFO,
                        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiBookmarkList:loading: (getter is_loading)
   *
   * %TRUE if files are being loaded.
   */
  properties[PROP_LOADING] =
      g_param_spec_boolean ("loading", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiBookmarkList:n-items:
   *
   * The number of items. See [method@Gio.ListModel.get_n_items].
   *
   * Since: 4.8
   **/
  properties[PROP_N_ITEMS] =
    g_param_spec_uint ("n-items", NULL, NULL,
                       0, G_MAXUINT, 0,
                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, properties);
}

static void
bobgui_bookmark_list_init (BobguiBookmarkList *self)
{
  self->items = g_sequence_new (g_object_unref);
  self->io_priority = G_PRIORITY_DEFAULT;
  self->file = g_bookmark_file_new ();
}

static gboolean
bobgui_bookmark_list_stop_loading (BobguiBookmarkList *self)
{
  if (self->cancellable == NULL)
    return FALSE;

  g_cancellable_cancel (self->cancellable);
  g_clear_object (&self->cancellable);

  self->loading = 0;

  return TRUE;
}

static void
got_file_info (GObject      *source,
               GAsyncResult *res,
               gpointer      user_data)
{
  BobguiBookmarkList *self = user_data;
  GFile *file = G_FILE (source);
  GFileInfo *info;
  GError *error = NULL;

  info = g_file_query_info_finish (file, res, &error);
  if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    {
      g_error_free (error);
      return;
    }

  if (info)
    {
      char *uri;
      gboolean is_private;
      char **apps;

      uri = g_file_get_uri (file);
      is_private = g_bookmark_file_get_is_private (self->file, uri, NULL);
      apps = g_bookmark_file_get_applications (self->file, uri, NULL, NULL);

      g_file_info_set_attribute_object (info, "standard::file", G_OBJECT (file));
      g_file_info_set_attribute_boolean (info, "recent::private", is_private);
      g_file_info_set_attribute_stringv (info, "recent::applications", apps);

      g_strfreev (apps);

      g_sequence_append (self->items, info);
      g_list_model_items_changed (G_LIST_MODEL (self), g_sequence_get_length (self->items) - 1, 0, 1);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);

      g_free (uri);
    }

  self->loading--;

  if (self->loading == 0)
    {
      g_clear_object (&self->cancellable);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_LOADING]);
    }
}

static void
bobgui_bookmark_list_clear_items (BobguiBookmarkList *self)
{
  guint n_items;

  n_items = g_sequence_get_length (self->items);
  if (n_items > 0)
    {
      g_sequence_remove_range (g_sequence_get_begin_iter (self->items),
                               g_sequence_get_end_iter (self->items));

      g_list_model_items_changed (G_LIST_MODEL (self), 0, n_items, 0);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_N_ITEMS]);
    }
}

static void
bobgui_bookmark_list_start_loading (BobguiBookmarkList *self)
{
  gboolean was_loading;
  GError *error = NULL;

  was_loading = bobgui_bookmark_list_stop_loading (self);
  bobgui_bookmark_list_clear_items (self);

  if (g_bookmark_file_load_from_file (self->file, self->filename, &error))
    {
      char **uris;
      gsize len;
      int i;

      uris = g_bookmark_file_get_uris (self->file, &len);
      if (len > 0)
        {
          self->cancellable = g_cancellable_new ();
          self->loading = len;
        }

      for (i = 0; i < len; i++)
        {
          const char *uri = uris[i];
          GFile *file;

          /* add this item */
          file = g_file_new_for_uri (uri);
          g_file_query_info_async (file,
                                   self->attributes,
                                   0,
                                   self->io_priority,
                                   self->cancellable,
                                   got_file_info,
                                   self);
          g_object_unref (file);
        }

      g_strfreev (uris);
    }
  else
    {
      if (!g_error_matches (error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
        g_warning ("Failed to load %s: %s", self->filename, error->message);
      g_clear_error (&error);
    }

  if (was_loading != (self->cancellable != NULL))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_LOADING]);
}

static void
bookmark_file_changed (GFileMonitor      *monitor,
                       GFile             *file,
                       GFile             *other_file,
                       GFileMonitorEvent  event_type,
                       gpointer           data)
{
  BobguiBookmarkList *self = data;

  switch (event_type)
    {
    case G_FILE_MONITOR_EVENT_CHANGED:
    case G_FILE_MONITOR_EVENT_CREATED:
    case G_FILE_MONITOR_EVENT_DELETED:
      bobgui_bookmark_list_start_loading (self);
      break;

    case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
    case G_FILE_MONITOR_EVENT_ATTRIBUTE_CHANGED:
    case G_FILE_MONITOR_EVENT_PRE_UNMOUNT:
    case G_FILE_MONITOR_EVENT_UNMOUNTED:
    case G_FILE_MONITOR_EVENT_MOVED:
    case G_FILE_MONITOR_EVENT_RENAMED:
    case G_FILE_MONITOR_EVENT_MOVED_IN:
    case G_FILE_MONITOR_EVENT_MOVED_OUT:

    default:
      break;
    }
}

static void
bobgui_bookmark_list_set_filename (BobguiBookmarkList *self,
                                    const char         *filename)
{
  GFile *file;

  if (filename)
    self->filename = g_strdup (filename);
  else
    self->filename = g_build_filename (g_get_user_data_dir (), "recently-used.xbel", NULL);

  file = g_file_new_for_path (self->filename);
  self->monitor = g_file_monitor_file (file, G_FILE_MONITOR_NONE, NULL, NULL);
  g_signal_connect (self->monitor, "changed",
                    G_CALLBACK (bookmark_file_changed), self);
  g_object_unref (file);

  bobgui_bookmark_list_start_loading (self);
}

/**
 * bobgui_bookmark_list_get_filename:
 * @self: a `BobguiBookmarkList`
 *
 * Returns the filename of the bookmark file that
 * this list is loading.
 *
 * Returns: (type filename): the filename of the .xbel file
 */
const char *
bobgui_bookmark_list_get_filename (BobguiBookmarkList *self)
{
  g_return_val_if_fail (BOBGUI_IS_BOOKMARK_LIST (self), NULL);

  return self->filename;
}

/**
 * bobgui_bookmark_list_new:
 * @filename: (type filename) (nullable): The bookmark file to load
 * @attributes: (nullable): The attributes to query
 *
 * Creates a new `BobguiBookmarkList` with the given @attributes.
 *
 * Returns: a new `BobguiBookmarkList`
 */
BobguiBookmarkList *
bobgui_bookmark_list_new (const char *filename,
                       const char *attributes)
{
  return g_object_new (BOBGUI_TYPE_BOOKMARK_LIST,
                       "filename", filename,
                       "attributes", attributes,
                       NULL);
}

/**
 * bobgui_bookmark_list_set_attributes:
 * @self: a `BobguiBookmarkList`
 * @attributes: (nullable): the attributes to enumerate
 *
 * Sets the @attributes to be enumerated and starts the enumeration.
 *
 * If @attributes is %NULL, no attributes will be queried, but a list
 * of `GFileInfo`s will still be created.
 */
void
bobgui_bookmark_list_set_attributes (BobguiBookmarkList *self,
                                      const char         *attributes)
{
  g_return_if_fail (BOBGUI_IS_BOOKMARK_LIST (self));

  if (g_strcmp0 (self->attributes, attributes) == 0)
    return;

  g_object_freeze_notify (G_OBJECT (self));

  g_free (self->attributes);
  self->attributes = g_strdup (attributes);

  bobgui_bookmark_list_start_loading (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ATTRIBUTES]);

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_bookmark_list_get_attributes:
 * @self: a `BobguiBookmarkList`
 *
 * Gets the attributes queried on the children.
 *
 * Returns: (nullable) (transfer none): The queried attributes
 */
const char *
bobgui_bookmark_list_get_attributes (BobguiBookmarkList *self)
{
  g_return_val_if_fail (BOBGUI_IS_BOOKMARK_LIST (self), NULL);

  return self->attributes;
}

/**
 * bobgui_bookmark_list_set_io_priority:
 * @self: a `BobguiBookmarkList`
 * @io_priority: IO priority to use
 *
 * Sets the IO priority to use while loading files.
 *
 * The default IO priority is %G_PRIORITY_DEFAULT.
 */
void
bobgui_bookmark_list_set_io_priority (BobguiBookmarkList *self,
                                       int                 io_priority)
{
  g_return_if_fail (BOBGUI_IS_BOOKMARK_LIST (self));

  if (self->io_priority == io_priority)
    return;

  self->io_priority = io_priority;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_IO_PRIORITY]);
}

/**
 * bobgui_bookmark_list_get_io_priority:
 * @self: a `BobguiBookmarkList`
 *
 * Gets the IO priority to use while loading file.
 *
 * Returns: The IO priority.
 */
int
bobgui_bookmark_list_get_io_priority (BobguiBookmarkList *self)
{
  g_return_val_if_fail (BOBGUI_IS_BOOKMARK_LIST (self), G_PRIORITY_DEFAULT);

  return self->io_priority;
}

/**
 * bobgui_bookmark_list_is_loading: (get-property loading)
 * @self: a `BobguiBookmarkList`
 *
 * Returns %TRUE if the files are currently being loaded.
 *
 * Files will be added to @self from time to time while loading is
 * going on. The order in which are added is undefined and may change
 * in between runs.
 *
 * Returns: %TRUE if @self is loading
 */
gboolean
bobgui_bookmark_list_is_loading (BobguiBookmarkList *self)
{
  g_return_val_if_fail (BOBGUI_IS_BOOKMARK_LIST (self), FALSE);

  return self->cancellable != NULL;
}
