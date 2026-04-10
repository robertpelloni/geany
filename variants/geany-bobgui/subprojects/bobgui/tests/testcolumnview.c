#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

GSList *pending = NULL;
guint active = 0;

static void
loading_cb (BobguiDirectoryList *dir,
            GParamSpec       *pspec,
            gpointer          unused)
{
  if (bobgui_directory_list_is_loading (dir))
    {
      active++;
      /* HACK: ensure loading finishes and the dir doesn't get destroyed */
      g_object_ref (dir);
    }
  else
    {
      active--;
      g_object_unref (dir);

      while (active < 20 && pending)
        {
          BobguiDirectoryList *dir2 = pending->data;
          pending = g_slist_remove (pending, dir2);
          bobgui_directory_list_set_file (dir2, g_object_get_data (G_OBJECT (dir2), "file"));
          g_object_unref (dir2);
        }
    }
}

static BobguiDirectoryList *
create_directory_list (GFile *file)
{
  BobguiDirectoryList *dir;

  dir = bobgui_directory_list_new ("*",
                                NULL);
  bobgui_directory_list_set_io_priority (dir, G_PRIORITY_DEFAULT_IDLE);
  g_signal_connect (dir, "notify::loading", G_CALLBACK (loading_cb), NULL);
  g_assert (!bobgui_directory_list_is_loading (dir));

  if (active > 20)
    {
      g_object_set_data_full (G_OBJECT (dir), "file", g_object_ref (file), g_object_unref);
      pending = g_slist_prepend (pending, g_object_ref (dir));
    }
  else
    {
      bobgui_directory_list_set_file (dir, file);
    }

  return dir;
}

static GListModel *
create_list_model_for_directory (gpointer file)
{
  if (g_file_query_file_type (file, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL) != G_FILE_TYPE_DIRECTORY)
    return NULL;

  return G_LIST_MODEL (create_directory_list (file));
}

static GListModel *
create_recent_files_list (void)
{
  BobguiBookmarkList *dir;

  dir = bobgui_bookmark_list_new (NULL, "*");

  return G_LIST_MODEL (dir);
}

#if 0
typedef struct _RowData RowData;
struct _RowData
{
  BobguiWidget *expander;
  BobguiWidget *icon;
  BobguiWidget *name;
  GCancellable *cancellable;

  BobguiTreeListRow *current_item;
};

static void row_data_notify_item (BobguiListItem *item,
                                  GParamSpec  *pspec,
                                  RowData     *data);
static void
row_data_unbind (RowData *data)
{
  if (data->current_item == NULL)
    return;

  if (data->cancellable)
    {
      g_cancellable_cancel (data->cancellable);
      g_clear_object (&data->cancellable);
    }

  g_clear_object (&data->current_item);
}

static void
row_data_update_info (RowData   *data,
                      GFileInfo *info)
{
  GIcon *icon;
  const char *thumbnail_path;

  thumbnail_path = g_file_info_get_attribute_byte_string (info, G_FILE_ATTRIBUTE_THUMBNAIL_PATH);
  if (thumbnail_path)
    {
      /* XXX: not async */
      GFile *thumbnail_file = g_file_new_for_path (thumbnail_path);
      icon = g_file_icon_new (thumbnail_file);
      g_object_unref (thumbnail_file);
    }
  else if (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_STANDARD_ICON))
    {
      icon = g_file_info_get_icon (info);
    }
  else
    {
      icon = NULL;
    }


  bobgui_widget_set_visible (data->icon, icon != NULL);
  bobgui_image_set_from_gicon (BOBGUI_IMAGE (data->icon), icon);
}

static void
copy_attribute (GFileInfo   *to,
                GFileInfo   *from,
                const char *attribute)
{
  GFileAttributeType type;
  gpointer value;

  if (g_file_info_get_attribute_data (from, attribute, &type, &value, NULL))
    g_file_info_set_attribute (to, attribute, type, value);
}

static void
row_data_got_thumbnail_info_cb (GObject      *source,
                                GAsyncResult *res,
                                gpointer      _data)
{
  RowData *data = _data; /* invalid if operation was cancelled */
  GFile *file = G_FILE (source);
  GFileInfo *queried, *info;

  queried = g_file_query_info_finish (file, res, NULL);
  if (queried == NULL)
    return;

  /* now we know row is valid */

  info = bobgui_tree_list_row_get_item (data->current_item);

  copy_attribute (info, queried, G_FILE_ATTRIBUTE_THUMBNAIL_PATH);
  copy_attribute (info, queried, G_FILE_ATTRIBUTE_THUMBNAILING_FAILED);
  copy_attribute (info, queried, G_FILE_ATTRIBUTE_STANDARD_ICON);

  g_object_unref (queried);

  row_data_update_info (data, info);
  
  g_clear_object (&data->cancellable);
}

static void
row_data_bind (RowData        *data,
               BobguiTreeListRow *item)
{
  GFileInfo *info;

  row_data_unbind (data);

  if (item == NULL)
    return;

  data->current_item = g_object_ref (item);

  bobgui_tree_expander_set_list_row (BOBGUI_TREE_EXPANDER (data->expander), item);

  info = bobgui_tree_list_row_get_item (item);

  if (!g_file_info_has_attribute (info, "filechooser::queried"))
    {
      data->cancellable = g_cancellable_new ();
      g_file_info_set_attribute_boolean (info, "filechooser::queried", TRUE);
      g_file_query_info_async (G_FILE (g_file_info_get_attribute_object (info, "standard::file")),
                               G_FILE_ATTRIBUTE_THUMBNAIL_PATH ","
                               G_FILE_ATTRIBUTE_THUMBNAILING_FAILED ","
                               G_FILE_ATTRIBUTE_STANDARD_ICON,
                               G_FILE_QUERY_INFO_NONE,
                               G_PRIORITY_DEFAULT,
                               data->cancellable,
                               row_data_got_thumbnail_info_cb,
                               data);
    }

  row_data_update_info (data, info);

  bobgui_inscription_set_text (BOBGUI_LABEL (data->name), g_file_info_get_display_name (info));

  g_object_unref (info);
}

static void
row_data_notify_item (BobguiListItem *item,
                      GParamSpec  *pspec,
                      RowData     *data)
{
  row_data_bind (data, bobgui_list_item_get_item (item));
}

static void
row_data_free (gpointer _data)
{
  RowData *data = _data;

  row_data_unbind (data);

  g_free (data);
}

static void
setup_widget (BobguiListItem *list_item,
              gpointer     unused)
{
  BobguiWidget *box, *child;
  RowData *data;

  data = g_new0 (RowData, 1);
  g_signal_connect (list_item, "notify::item", G_CALLBACK (row_data_notify_item), data);
  g_object_set_data_full (G_OBJECT (list_item), "row-data", data, row_data_free);

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 4);
  bobgui_container_add (BOBGUI_CONTAINER (list_item), box);

  child = bobgui_inscription_new (NULL);
  bobgui_inscription_set_min_chars (BOBGUI_LABEL (child), 5);
  bobgui_inscription_set_xalign (BOBGUI_LABEL (child), 1.0);
  g_object_bind_property (list_item, "position", child, "text", G_BINDING_SYNC_CREATE);
  bobgui_container_add (BOBGUI_CONTAINER (box), child);

  data->expander = bobgui_tree_expander_new ();
  bobgui_container_add (BOBGUI_CONTAINER (box), data->expander);

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 4);
  bobgui_tree_expander_set_child (BOBGUI_TREE_EXPANDER (data->expander), box);

  data->icon = bobgui_image_new ();
  bobgui_container_add (BOBGUI_CONTAINER (box), data->icon);

  data->name = bobgui_inscription_new (NULL);
  bobgui_inscription_set_nat_chars (BOBGUI_LABEL (data->name), 25);
  bobgui_container_add (BOBGUI_CONTAINER (box), data->name);
}
#endif

static GListModel *
create_list_model_for_file_info (gpointer file_info,
                                 gpointer unused)
{
  GFile *file = G_FILE (g_file_info_get_attribute_object (file_info, "standard::file"));

  if (file == NULL)
    return NULL;

  return create_list_model_for_directory (file);
}

static gboolean
update_statusbar (BobguiStatusbar *statusbar)
{
  GListModel *model = g_object_get_data (G_OBJECT (statusbar), "model");
  GString *string = g_string_new (NULL);
  guint n;
  gboolean result = G_SOURCE_REMOVE;

  bobgui_statusbar_remove_all (statusbar, 0);

  n = g_list_model_get_n_items (model);
  g_string_append_printf (string, "%u", n);
  if (BOBGUI_IS_FILTER_LIST_MODEL (model))
    {
      guint n_unfiltered = g_list_model_get_n_items (bobgui_filter_list_model_get_model (BOBGUI_FILTER_LIST_MODEL (model)));
      if (n != n_unfiltered)
        g_string_append_printf (string, "/%u", n_unfiltered);
    }
  g_string_append (string, " items");

  if (pending || active)
    {
      g_string_append_printf (string, " (%u directories remaining)", active + g_slist_length (pending));
      result = G_SOURCE_CONTINUE;
    }
      result = G_SOURCE_CONTINUE;

  bobgui_statusbar_push (statusbar, 0, string->str);
  g_free (string->str);

  return result;
}

static gboolean
match_file (gpointer item, gpointer data)
{
  BobguiWidget *search_entry = data;
  GFileInfo *info = bobgui_tree_list_row_get_item (item);
  GFile *file = G_FILE (g_file_info_get_attribute_object (info, "standard::file"));
  char *path;
  gboolean result;
  
  path = g_file_get_path (file);

  result = strstr (path, bobgui_editable_get_text (BOBGUI_EDITABLE (search_entry))) != NULL;

  g_object_unref (info);
  g_free (path);

  return result;
}

static int
compare_file_attribute (gconstpointer info1_,
                        gconstpointer info2_,
                        gpointer      data)
{
  GFileInfo *info1 = (gpointer) info1_;
  GFileInfo *info2 = (gpointer) info2_;
  const char *attribute = data;
  GFileAttributeType type1, type2;

  type1 = g_file_info_get_attribute_type (info1, attribute);
  type2 = g_file_info_get_attribute_type (info2, attribute);
  if (type1 != type2)
    return (int) type2 - (int) type1;

  switch (type1)
    {
    case G_FILE_ATTRIBUTE_TYPE_INVALID:
    case G_FILE_ATTRIBUTE_TYPE_OBJECT:
    case G_FILE_ATTRIBUTE_TYPE_STRINGV:
      return 0;
    case G_FILE_ATTRIBUTE_TYPE_STRING:
      return g_utf8_collate (g_file_info_get_attribute_string (info1, attribute),
                             g_file_info_get_attribute_string (info2, attribute));
    case G_FILE_ATTRIBUTE_TYPE_BYTE_STRING:
      return strcmp (g_file_info_get_attribute_byte_string (info1, attribute),
                     g_file_info_get_attribute_byte_string (info2, attribute));
    case G_FILE_ATTRIBUTE_TYPE_BOOLEAN:
      return g_file_info_get_attribute_boolean (info1, attribute)
          -  g_file_info_get_attribute_boolean (info2, attribute);
    case G_FILE_ATTRIBUTE_TYPE_UINT32:
      return g_file_info_get_attribute_uint32 (info1, attribute)
          -  g_file_info_get_attribute_uint32 (info2, attribute);
    case G_FILE_ATTRIBUTE_TYPE_INT32:
      return g_file_info_get_attribute_int32 (info1, attribute)
          -  g_file_info_get_attribute_int32 (info2, attribute);
    case G_FILE_ATTRIBUTE_TYPE_UINT64:
      return g_file_info_get_attribute_uint64 (info1, attribute)
          -  g_file_info_get_attribute_uint64 (info2, attribute);
    case G_FILE_ATTRIBUTE_TYPE_INT64:
      return g_file_info_get_attribute_int64 (info1, attribute)
          -  g_file_info_get_attribute_int64 (info2, attribute);
    default:
      g_assert_not_reached ();
      return 0;
    }
}

static GObject *
get_object (GObject    *unused,
            GFileInfo  *info,
            const char *attribute)
{
  GObject *o;

  if (info == NULL)
    return NULL;

  o = g_file_info_get_attribute_object (info, attribute);
  if (o)
    g_object_ref (o);

  return o;
}

static char *
get_string (GObject    *unused,
            GFileInfo  *info,
            const char *attribute)
{
  if (info == NULL)
    return NULL;

  return g_file_info_get_attribute_as_string (info, attribute);
}

static gboolean
get_boolean (GObject    *unused,
             GFileInfo  *info,
             const char *attribute)
{
  if (info == NULL)
    return FALSE;

  return g_file_info_get_attribute_boolean (info, attribute);
}

const char *ui_file =
"<?xml version='1.0' encoding='UTF-8'?>\n"
"<interface>\n"
"  <object class='BobguiColumnView' id='view'>\n"
"    <child>\n"
"      <object class='BobguiColumnViewColumn'>\n"
"        <property name='title'>Name</property>\n"
"        <property name='factory'>\n"
"          <object class='BobguiBuilderListItemFactory'>\n"
"            <property name='bytes'><![CDATA[\n"
"<?xml version='1.0' encoding='UTF-8'?>\n"
"<interface>\n"
"  <template class='BobguiColumnViewCell'>\n"
"    <property name='child'>\n"
"      <object class='BobguiTreeExpander' id='expander'>\n"
"        <binding name='list-row'>\n"
"          <lookup name='item'>BobguiColumnViewCell</lookup>\n"
"        </binding>\n"
"        <property name='child'>\n"
"          <object class='BobguiBox'>\n"
"            <child>\n"
"              <object class='BobguiImage'>\n"
"                <binding name='gicon'>\n"
"                  <closure type='GIcon' function='get_object'>\n"
"                    <lookup name='item'>expander</lookup>\n"
"                    <constant type='gchararray'>standard::icon</constant>"
"                  </closure>\n"
"                </binding>\n"
"              </object>\n"
"            </child>\n"
"            <child>\n"
"              <object class='BobguiInscription'>\n"
"                <binding name='text'>\n"
"                  <closure type='gchararray' function='get_string'>\n"
"                    <lookup name='item'>expander</lookup>\n"
"                    <constant type='gchararray'>standard::display-name</constant>"
"                  </closure>\n"
"                </binding>\n"
"              </object>\n"
"            </child>\n"
"          </object>\n"
"        </property>\n"
"      </object>\n"
"    </property>\n"
"  </template>\n"
"</interface>\n"
"            ]]></property>\n"
"          </object>\n"
"        </property>\n"
"        <property name='sorter'>\n"
"          <object class='BobguiStringSorter'>\n"
"            <property name='expression'>\n"
"              <closure type='gchararray' function='g_file_info_get_attribute_as_string'>\n"
"                <constant type='gchararray'>standard::display-name</constant>"
"              </closure>\n"
"            </property>\n"
"          </object>\n"
"        </property>\n"
"      </object>\n"
"    </child>\n"
"  </object>\n"
"</interface>\n";

#define SIMPLE_STRING_FACTORY(attr, type) \
"<?xml version='1.0' encoding='UTF-8'?>\n" \
"<interface>\n" \
"  <template class='BobguiColumnViewCell'>\n" \
"    <property name='child'>\n" \
"      <object class='BobguiInscription'>\n" \
"        <binding name='text'>\n" \
"          <closure type='gchararray' function='get_string'>\n" \
"            <lookup name='item' type='BobguiTreeListRow'><lookup name='item'>BobguiColumnViewCell</lookup></lookup>\n" \
"            <constant type='gchararray'>" attr "</constant>" \
"          </closure>\n" \
"        </binding>\n" \
"      </object>\n" \
"    </property>\n" \
"  </template>\n" \
"</interface>\n" \

#define BOOLEAN_FACTORY(attr) \
"<?xml version='1.0' encoding='UTF-8'?>\n" \
"<interface>\n" \
"  <template class='BobguiColumnViewCell'>\n" \
"    <property name='child'>\n" \
"      <object class='BobguiCheckButton'>\n" \
"        <binding name='active'>\n" \
"          <closure type='gboolean' function='get_boolean'>\n" \
"            <lookup name='item' type='BobguiTreeListRow'><lookup name='item'>BobguiColumnViewCell</lookup></lookup>\n" \
"            <constant type='gchararray'>" attr "</constant>" \
"          </closure>\n" \
"        </binding>\n" \
"      </object>\n" \
"    </property>\n" \
"  </template>\n" \
"</interface>\n" \

#define ICON_FACTORY(attr) \
"<?xml version='1.0' encoding='UTF-8'?>\n" \
"<interface>\n" \
"  <template class='BobguiColumnViewCell'>\n" \
"    <property name='child'>\n" \
"      <object class='BobguiImage'>\n" \
"        <binding name='gicon'>\n" \
"          <closure type='GIcon' function='get_object'>\n" \
"            <lookup name='item' type='BobguiTreeListRow'><lookup name='item'>BobguiColumnViewCell</lookup></lookup>\n" \
"            <constant type='gchararray'>" attr "</constant>" \
"          </closure>\n" \
"        </binding>\n" \
"      </object>\n" \
"    </property>\n" \
"  </template>\n" \
"</interface>\n" \

struct {
  const char *title;
  const char *attribute;
  const char *factory_xml;
} extra_columns[] = {
  { "Type",              G_FILE_ATTRIBUTE_STANDARD_TYPE, SIMPLE_STRING_FACTORY (G_FILE_ATTRIBUTE_STANDARD_TYPE, "uint32") },
  { "Hidden",            G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN, BOOLEAN_FACTORY (G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN) },
  { "Backup",            G_FILE_ATTRIBUTE_STANDARD_IS_BACKUP, BOOLEAN_FACTORY (G_FILE_ATTRIBUTE_STANDARD_IS_BACKUP) },
  { "Symlink",           G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK, BOOLEAN_FACTORY (G_FILE_ATTRIBUTE_STANDARD_IS_SYMLINK) },
  { "Virtual",           G_FILE_ATTRIBUTE_STANDARD_IS_VIRTUAL, BOOLEAN_FACTORY (G_FILE_ATTRIBUTE_STANDARD_IS_VIRTUAL) },
  { "Volatile",          G_FILE_ATTRIBUTE_STANDARD_IS_VOLATILE, BOOLEAN_FACTORY (G_FILE_ATTRIBUTE_STANDARD_IS_VOLATILE) },
  { "Edit name",         G_FILE_ATTRIBUTE_STANDARD_EDIT_NAME, SIMPLE_STRING_FACTORY (G_FILE_ATTRIBUTE_STANDARD_EDIT_NAME, "string") },
  { "Copy name",         G_FILE_ATTRIBUTE_STANDARD_COPY_NAME, SIMPLE_STRING_FACTORY (G_FILE_ATTRIBUTE_STANDARD_COPY_NAME, "string") },
  { "Description",       G_FILE_ATTRIBUTE_STANDARD_DESCRIPTION, SIMPLE_STRING_FACTORY (G_FILE_ATTRIBUTE_STANDARD_DESCRIPTION, "string") },
  { "Icon",              G_FILE_ATTRIBUTE_STANDARD_ICON, ICON_FACTORY (G_FILE_ATTRIBUTE_STANDARD_ICON) },
  { "Symbolic icon",     G_FILE_ATTRIBUTE_STANDARD_SYMBOLIC_ICON, ICON_FACTORY (G_FILE_ATTRIBUTE_STANDARD_SYMBOLIC_ICON) },
  { "Content type",      G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, SIMPLE_STRING_FACTORY (G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE, "string") },
  { "Fast content type", G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE, SIMPLE_STRING_FACTORY (G_FILE_ATTRIBUTE_STANDARD_FAST_CONTENT_TYPE, "string") },
  { "Size",              G_FILE_ATTRIBUTE_STANDARD_SIZE, SIMPLE_STRING_FACTORY (G_FILE_ATTRIBUTE_STANDARD_SIZE, "uint64") },
  { "Allocated size",    G_FILE_ATTRIBUTE_STANDARD_ALLOCATED_SIZE, SIMPLE_STRING_FACTORY (G_FILE_ATTRIBUTE_STANDARD_ALLOCATED_SIZE, "uint64") },
  { "Target URI",        G_FILE_ATTRIBUTE_STANDARD_TARGET_URI, SIMPLE_STRING_FACTORY (G_FILE_ATTRIBUTE_STANDARD_TARGET_URI, "string") },
  { "Sort order",        G_FILE_ATTRIBUTE_STANDARD_SORT_ORDER, SIMPLE_STRING_FACTORY (G_FILE_ATTRIBUTE_STANDARD_SORT_ORDER, "int32") },
  { "ETAG value",        G_FILE_ATTRIBUTE_ETAG_VALUE, SIMPLE_STRING_FACTORY (G_FILE_ATTRIBUTE_ETAG_VALUE, "string") },
  { "File ID",           G_FILE_ATTRIBUTE_ID_FILE, SIMPLE_STRING_FACTORY (G_FILE_ATTRIBUTE_ID_FILE, "string") },
  { "Filesystem ID",     G_FILE_ATTRIBUTE_ID_FILESYSTEM, SIMPLE_STRING_FACTORY (G_FILE_ATTRIBUTE_ID_FILESYSTEM, "string") },
  { "Read",              G_FILE_ATTRIBUTE_ACCESS_CAN_READ, BOOLEAN_FACTORY (G_FILE_ATTRIBUTE_ACCESS_CAN_READ) },
  { "Write",             G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE, BOOLEAN_FACTORY (G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE) },
  { "Execute",           G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE, BOOLEAN_FACTORY (G_FILE_ATTRIBUTE_ACCESS_CAN_EXECUTE) },
  { "Delete",            G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE, BOOLEAN_FACTORY (G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE) },
  { "Trash",             G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH, BOOLEAN_FACTORY (G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH) },
  { "Rename",            G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME, BOOLEAN_FACTORY (G_FILE_ATTRIBUTE_ACCESS_CAN_RENAME) },
  { "Can mount",         G_FILE_ATTRIBUTE_MOUNTABLE_CAN_MOUNT, BOOLEAN_FACTORY (G_FILE_ATTRIBUTE_MOUNTABLE_CAN_MOUNT) },
  { "Can unmount",       G_FILE_ATTRIBUTE_MOUNTABLE_CAN_UNMOUNT, BOOLEAN_FACTORY (G_FILE_ATTRIBUTE_MOUNTABLE_CAN_UNMOUNT) },
  { "Can eject",         G_FILE_ATTRIBUTE_MOUNTABLE_CAN_EJECT, BOOLEAN_FACTORY (G_FILE_ATTRIBUTE_MOUNTABLE_CAN_EJECT) },
  { "UNIX device",       G_FILE_ATTRIBUTE_MOUNTABLE_UNIX_DEVICE, SIMPLE_STRING_FACTORY (G_FILE_ATTRIBUTE_MOUNTABLE_UNIX_DEVICE, "uint32") },
  { "UNIX device file",  G_FILE_ATTRIBUTE_MOUNTABLE_UNIX_DEVICE_FILE, SIMPLE_STRING_FACTORY (G_FILE_ATTRIBUTE_MOUNTABLE_UNIX_DEVICE_FILE, "string") },
  { "owner",             G_FILE_ATTRIBUTE_OWNER_USER, SIMPLE_STRING_FACTORY (G_FILE_ATTRIBUTE_OWNER_USER, "string") },
  { "owner (real)",      G_FILE_ATTRIBUTE_OWNER_USER_REAL, SIMPLE_STRING_FACTORY (G_FILE_ATTRIBUTE_OWNER_USER_REAL, "string") },
  { "group",             G_FILE_ATTRIBUTE_OWNER_GROUP, SIMPLE_STRING_FACTORY (G_FILE_ATTRIBUTE_OWNER_GROUP, "string") },
  { "Preview icon",      G_FILE_ATTRIBUTE_PREVIEW_ICON, ICON_FACTORY (G_FILE_ATTRIBUTE_PREVIEW_ICON) },
  { "Private",           "recent::private", BOOLEAN_FACTORY ("recent::private") },
};

#if 0
#define G_FILE_ATTRIBUTE_MOUNTABLE_CAN_START "mountable::can-start"     /* boolean */
#define G_FILE_ATTRIBUTE_MOUNTABLE_CAN_START_DEGRADED "mountable::can-start-degraded"     /* boolean */
#define G_FILE_ATTRIBUTE_MOUNTABLE_CAN_STOP "mountable::can-stop"      /* boolean */
#define G_FILE_ATTRIBUTE_MOUNTABLE_START_STOP_TYPE "mountable::start-stop-type" /* uint32 (GDriveStartStopType) */
#define G_FILE_ATTRIBUTE_MOUNTABLE_CAN_POLL "mountable::can-poll"      /* boolean */
#define G_FILE_ATTRIBUTE_MOUNTABLE_IS_MEDIA_CHECK_AUTOMATIC "mountable::is-media-check-automatic"      /* boolean */
#define G_FILE_ATTRIBUTE_TIME_MODIFIED "time::modified"           /* uint64 */
#define G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC "time::modified-usec" /* uint32 */
#define G_FILE_ATTRIBUTE_TIME_ACCESS "time::access"               /* uint64 */
#define G_FILE_ATTRIBUTE_TIME_ACCESS_USEC "time::access-usec"     /* uint32 */
#define G_FILE_ATTRIBUTE_TIME_CHANGED "time::changed"             /* uint64 */
#define G_FILE_ATTRIBUTE_TIME_CHANGED_USEC "time::changed-usec"   /* uint32 */
#define G_FILE_ATTRIBUTE_TIME_CREATED "time::created"             /* uint64 */
#define G_FILE_ATTRIBUTE_TIME_CREATED_USEC "time::created-usec"   /* uint32 */
#define G_FILE_ATTRIBUTE_UNIX_DEVICE "unix::device"               /* uint32 */
#define G_FILE_ATTRIBUTE_UNIX_INODE "unix::inode"                 /* uint64 */
#define G_FILE_ATTRIBUTE_UNIX_MODE "unix::mode"                   /* uint32 */
#define G_FILE_ATTRIBUTE_UNIX_NLINK "unix::nlink"                 /* uint32 */
#define G_FILE_ATTRIBUTE_UNIX_UID "unix::uid"                     /* uint32 */
#define G_FILE_ATTRIBUTE_UNIX_GID "unix::gid"                     /* uint32 */
#define G_FILE_ATTRIBUTE_UNIX_RDEV "unix::rdev"                   /* uint32 */
#define G_FILE_ATTRIBUTE_UNIX_BLOCK_SIZE "unix::block-size"       /* uint32 */
#define G_FILE_ATTRIBUTE_UNIX_BLOCKS "unix::blocks"               /* uint64 */
#define G_FILE_ATTRIBUTE_UNIX_IS_MOUNTPOINT "unix::is-mountpoint" /* boolean */
#define G_FILE_ATTRIBUTE_DOS_IS_ARCHIVE "dos::is-archive"         /* boolean */
#define G_FILE_ATTRIBUTE_DOS_IS_SYSTEM "dos::is-system"           /* boolean */
#define G_FILE_ATTRIBUTE_DOS_IS_MOUNTPOINT "dos::is-mountpoint"   /* boolean */
#define G_FILE_ATTRIBUTE_DOS_REPARSE_POINT_TAG "dos::reparse-point-tag"   /* uint32 */
#define G_FILE_ATTRIBUTE_THUMBNAIL_PATH "thumbnail::path"         /* bytestring */
#define G_FILE_ATTRIBUTE_THUMBNAILING_FAILED "thumbnail::failed"         /* boolean */
#define G_FILE_ATTRIBUTE_THUMBNAIL_IS_VALID "thumbnail::is-valid"        /* boolean */
#define G_FILE_ATTRIBUTE_FILESYSTEM_SIZE "filesystem::size"                       /* uint64 */
#define G_FILE_ATTRIBUTE_FILESYSTEM_FREE "filesystem::free"                       /* uint64 */
#define G_FILE_ATTRIBUTE_FILESYSTEM_USED "filesystem::used"                       /* uint64 */
#define G_FILE_ATTRIBUTE_FILESYSTEM_TYPE "filesystem::type"                       /* string */
#define G_FILE_ATTRIBUTE_FILESYSTEM_READONLY "filesystem::readonly"               /* boolean */
#define G_FILE_ATTRIBUTE_FILESYSTEM_USE_PREVIEW "filesystem::use-preview"        /* uint32 (GFilesystemPreviewType) */
#define G_FILE_ATTRIBUTE_FILESYSTEM_REMOTE "filesystem::remote"                   /* boolean */
#define G_FILE_ATTRIBUTE_GVFS_BACKEND "gvfs::backend"             /* string */
#define G_FILE_ATTRIBUTE_SELINUX_CONTEXT "selinux::context"       /* string */
#define G_FILE_ATTRIBUTE_TRASH_ITEM_COUNT "trash::item-count"     /* uint32 */
#define G_FILE_ATTRIBUTE_TRASH_ORIG_PATH "trash::orig-path"     /* byte string */
#define G_FILE_ATTRIBUTE_TRASH_DELETION_DATE "trash::deletion-date"  /* string */
#define G_FILE_ATTRIBUTE_RECENT_MODIFIED "recent::modified"          /* int64 (time_t) */
#endif

const char *factory_ui =
"<?xml version='1.0' encoding='UTF-8'?>\n"
"<interface>\n"
"  <template class='BobguiListItem'>\n"
"    <property name='child'>\n"
"      <object class='BobguiLabel'>\n"
"        <property name='xalign'>0</property>\n"
"        <binding name='label'>\n"
"          <lookup name='title' type='BobguiColumnViewColumn'>\n"
"            <lookup name='item'>BobguiListItem</lookup>\n"
"          </lookup>\n"
"        </binding>\n"
"      </object>\n"
"    </property>\n"
"  </template>\n"
"</interface>\n";

static BobguiBuilderScope *
create_scope (void)
{
#define ADD_SYMBOL(name) \
  bobgui_builder_cscope_add_callback_symbol (BOBGUI_BUILDER_CSCOPE (scope), G_STRINGIFY (name), G_CALLBACK (name))
  BobguiBuilderScope *scope;

  scope = bobgui_builder_cscope_new ();

  ADD_SYMBOL (get_object);
  ADD_SYMBOL (get_string);
  ADD_SYMBOL (get_boolean);

  return scope;
#undef ADD_SYMBOL
}

static void
add_extra_columns (BobguiColumnView   *view,
                   BobguiBuilderScope *scope)
{
  BobguiColumnViewColumn *column;
  BobguiSorter *sorter;
  GBytes *bytes;
  guint i;

  for (i = 0; i < G_N_ELEMENTS(extra_columns); i++)
    {
      bytes = g_bytes_new_static (extra_columns[i].factory_xml, strlen (extra_columns[i].factory_xml));
      column = bobgui_column_view_column_new (extra_columns[i].title,
          bobgui_builder_list_item_factory_new_from_bytes (scope, bytes));
      g_bytes_unref (bytes);
      sorter = BOBGUI_SORTER (bobgui_custom_sorter_new (compare_file_attribute, (gpointer) extra_columns[i].attribute, NULL));
      bobgui_column_view_column_set_sorter (column, sorter);
      g_object_unref (sorter);
      bobgui_column_view_append_column (view, column);
    }
}

static void
search_changed_cb (BobguiSearchEntry *entry,
                   BobguiFilter      *custom_filter)
{
  bobgui_filter_changed (custom_filter, BOBGUI_FILTER_CHANGE_DIFFERENT);
}

int
main (int argc, char *argv[])
{
  GListModel *toplevels;
  BobguiWidget *win, *hbox, *vbox, *sw, *view, *list, *search_entry, *statusbar;
  GListModel *dirmodel;
  BobguiTreeListModel *tree;
  BobguiFilterListModel *filter;
  BobguiFilter *custom_filter;
  BobguiSortListModel *sort;
  BobguiSorter *sorter;
  GFile *root;
  BobguiBuilderScope *scope;
  BobguiBuilder *builder;
  GError *error = NULL;
  BobguiSelectionModel *selection;

  bobgui_init ();

  win = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (win), 800, 600);

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 6);
  bobgui_window_set_child (BOBGUI_WINDOW (win), hbox);

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_box_append (BOBGUI_BOX (hbox), vbox);

  search_entry = bobgui_search_entry_new ();
  bobgui_box_append (BOBGUI_BOX (vbox), search_entry);

  sw = bobgui_scrolled_window_new ();
  bobgui_widget_set_hexpand (sw, TRUE);
  bobgui_widget_set_vexpand (sw, TRUE);
  bobgui_search_entry_set_key_capture_widget (BOBGUI_SEARCH_ENTRY (search_entry), sw);
  bobgui_box_append (BOBGUI_BOX (vbox), sw);

  scope = create_scope ();
  builder = bobgui_builder_new ();
  bobgui_builder_set_scope (builder, scope);
  if (!bobgui_builder_add_from_string (builder, ui_file, -1, &error))
    {
      g_assert_no_error (error);
    }
  view = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "view"));
  add_extra_columns (BOBGUI_COLUMN_VIEW (view), scope);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), view);
  g_object_unref (builder);

  if (argc > 1)
    {
      if (g_strcmp0 (argv[1], "--recent") == 0)
        {
          dirmodel = create_recent_files_list ();
        }
      else
        {
          root = g_file_new_for_commandline_arg (argv[1]);
          dirmodel = create_list_model_for_directory (root);
          g_object_unref (root);
        }
    }
  else
    {
      root = g_file_new_for_path (g_get_current_dir ());
      dirmodel = create_list_model_for_directory (root);
      g_object_unref (root);
    }
  tree = bobgui_tree_list_model_new (dirmodel,
                                  FALSE,
                                  TRUE,
                                  create_list_model_for_file_info,
                                  NULL, NULL);

  sorter = BOBGUI_SORTER (bobgui_tree_list_row_sorter_new (g_object_ref (bobgui_column_view_get_sorter (BOBGUI_COLUMN_VIEW (view)))));
  sort = bobgui_sort_list_model_new (G_LIST_MODEL (tree), sorter);

  custom_filter = BOBGUI_FILTER (bobgui_custom_filter_new (match_file, g_object_ref (search_entry), g_object_unref));
  filter = bobgui_filter_list_model_new (G_LIST_MODEL (sort), custom_filter);
  g_signal_connect (search_entry, "search-changed", G_CALLBACK (search_changed_cb), custom_filter);

  selection = BOBGUI_SELECTION_MODEL (bobgui_single_selection_new (G_LIST_MODEL (filter)));
  bobgui_column_view_set_model (BOBGUI_COLUMN_VIEW (view), selection);
  g_object_unref (selection);

  statusbar = bobgui_statusbar_new ();
  bobgui_widget_add_tick_callback (statusbar, (BobguiTickCallback) update_statusbar, NULL, NULL);
  g_object_set_data (G_OBJECT (statusbar), "model", filter);
  g_signal_connect_swapped (filter, "items-changed", G_CALLBACK (update_statusbar), statusbar);
  update_statusbar (BOBGUI_STATUSBAR (statusbar));
  bobgui_box_append (BOBGUI_BOX (vbox), statusbar);

  list = bobgui_list_view_new (
             BOBGUI_SELECTION_MODEL (bobgui_single_selection_new (g_object_ref (bobgui_column_view_get_columns (BOBGUI_COLUMN_VIEW (view))))),
             bobgui_builder_list_item_factory_new_from_bytes (scope, g_bytes_new_static (factory_ui, strlen (factory_ui))));
  bobgui_box_append (BOBGUI_BOX (hbox), list);

  g_object_unref (scope);

  bobgui_window_present (BOBGUI_WINDOW (win));

  toplevels = bobgui_window_get_toplevels ();
  while (g_list_model_get_n_items (toplevels))
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
