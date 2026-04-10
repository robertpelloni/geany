/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/. 
 */

#include "config.h"

#include "bobguitexttagtable.h"
#include "bobguitexttagtableprivate.h"

#include "bobguibuildable.h"
#include "bobguitexttagprivate.h"
#include "bobguimarshalers.h"
#include "bobguitextbufferprivate.h" /* just for the lame notify_will_remove_tag hack */
#include "bobguiprivate.h"

#include <stdlib.h>


/**
 * BobguiTextTagTable:
 *
 * Collects the tags in a `BobguiTextBuffer`.
 *
 * You may wish to begin by reading the
 * [text widget conceptual overview](section-text-widget.html),
 * which gives an overview of all the objects and data types
 * related to the text widget and how they work together.
 *
 * # BobguiTextTagTables as BobguiBuildable
 *
 * The `BobguiTextTagTable` implementation of the `BobguiBuildable` interface
 * supports adding tags by specifying “tag” as the “type” attribute
 * of a `<child>` element.
 *
 * An example of a UI definition fragment specifying tags:
 * ```xml
 * <object class="BobguiTextTagTable">
 *  <child type="tag">
 *    <object class="BobguiTextTag"/>
 *  </child>
 * </object>
 * ```
 */

typedef struct _BobguiTextTagTablePrivate       BobguiTextTagTablePrivate;
typedef struct _BobguiTextTagTableClass         BobguiTextTagTableClass;

struct _BobguiTextTagTable
{
  GObject parent_instance;

  BobguiTextTagTablePrivate *priv;
};

struct _BobguiTextTagTableClass
{
  GObjectClass parent_class;

  void (* tag_changed) (BobguiTextTagTable *table, BobguiTextTag *tag, gboolean size_changed);
  void (* tag_added) (BobguiTextTagTable *table, BobguiTextTag *tag);
  void (* tag_removed) (BobguiTextTagTable *table, BobguiTextTag *tag);
};

struct _BobguiTextTagTablePrivate
{
  GHashTable *hash;
  GSList     *anonymous;
  GSList     *buffers;

  int anon_count;

  guint seen_invisible : 1;
};

enum {
  TAG_CHANGED,
  TAG_ADDED,
  TAG_REMOVED,
  LAST_SIGNAL
};

static void bobgui_text_tag_table_finalize                 (GObject             *object);

static void bobgui_text_tag_table_buildable_interface_init (BobguiBuildableIface   *iface);
static void bobgui_text_tag_table_buildable_add_child      (BobguiBuildable        *buildable,
							 BobguiBuilder          *builder,
							 GObject             *child,
							 const char          *type);

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (BobguiTextTagTable, bobgui_text_tag_table, G_TYPE_OBJECT,
                         G_ADD_PRIVATE (BobguiTextTagTable)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_text_tag_table_buildable_interface_init))

static void
bobgui_text_tag_table_class_init (BobguiTextTagTableClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bobgui_text_tag_table_finalize;

  /**
   * BobguiTextTagTable::tag-changed:
   * @texttagtable: the object which received the signal.
   * @tag: the changed tag.
   * @size_changed: whether the change affects the `BobguiTextView` layout.
   *
   * Emitted every time a tag in the `BobguiTextTagTable` changes.
   */
  signals[TAG_CHANGED] =
    g_signal_new (I_("tag-changed"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiTextTagTableClass, tag_changed),
                  NULL, NULL,
                  _bobgui_marshal_VOID__OBJECT_BOOLEAN,
                  G_TYPE_NONE,
                  2,
                  BOBGUI_TYPE_TEXT_TAG,
                  G_TYPE_BOOLEAN);  
  g_signal_set_va_marshaller (signals[TAG_CHANGED],
                              G_OBJECT_CLASS_TYPE (object_class),
                              _bobgui_marshal_VOID__OBJECT_BOOLEANv);

  /**
   * BobguiTextTagTable::tag-added:
   * @texttagtable: the object which received the signal.
   * @tag: the added tag.
   *
   * Emitted every time a new tag is added in the `BobguiTextTagTable`.
   */
  signals[TAG_ADDED] =
    g_signal_new (I_("tag-added"),
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiTextTagTableClass, tag_added),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  BOBGUI_TYPE_TEXT_TAG);

  /**
   * BobguiTextTagTable::tag-removed:
   * @texttagtable: the object which received the signal.
   * @tag: the removed tag.
   *
   * Emitted every time a tag is removed from the `BobguiTextTagTable`.
   *
   * The @tag is still valid by the time the signal is emitted, but
   * it is not associated with a tag table any more.
   */
  signals[TAG_REMOVED] =
    g_signal_new (I_("tag-removed"),  
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiTextTagTableClass, tag_removed),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE,
                  1,
                  BOBGUI_TYPE_TEXT_TAG);
}

static void
bobgui_text_tag_table_init (BobguiTextTagTable *table)
{
  table->priv = bobgui_text_tag_table_get_instance_private (table);
  table->priv->hash = g_hash_table_new (g_str_hash, g_str_equal);
}

static void
check_visible (BobguiTextTagTable *table,
               BobguiTextTag      *tag)
{
  if (table->priv->seen_invisible)
    return;

  if (tag->priv->invisible_set)
    {
      gboolean invisible;

      g_object_get (tag, "invisible", &invisible, NULL);
      table->priv->seen_invisible = invisible;
    }
}

/**
 * bobgui_text_tag_table_new:
 *
 * Creates a new `BobguiTextTagTable`.
 *
 * The table contains no tags by default.
 *
 * Returns: a new `BobguiTextTagTable`
 */
BobguiTextTagTable*
bobgui_text_tag_table_new (void)
{
  BobguiTextTagTable *table;

  table = g_object_new (BOBGUI_TYPE_TEXT_TAG_TABLE, NULL);

  return table;
}

static void
foreach_unref (BobguiTextTag *tag, gpointer data)
{
  BobguiTextTagTable *table = BOBGUI_TEXT_TAG_TABLE (tag->priv->table);
  BobguiTextTagTablePrivate *priv = table->priv;
  GSList *l;

  /* We don't want to emit the remove signal here; so we just unparent
   * and unref the tag.
   */

  for (l = priv->buffers; l != NULL; l = l->next)
    _bobgui_text_buffer_notify_will_remove_tag (BOBGUI_TEXT_BUFFER (l->data),
                                             tag);

  tag->priv->table = NULL;
  g_object_unref (tag);
}

static void
bobgui_text_tag_table_finalize (GObject *object)
{
  BobguiTextTagTable *table = BOBGUI_TEXT_TAG_TABLE (object);
  BobguiTextTagTablePrivate *priv = table->priv;

  bobgui_text_tag_table_foreach (table, foreach_unref, NULL);

  g_hash_table_destroy (priv->hash);
  g_slist_free (priv->anonymous);
  g_slist_free (priv->buffers);

  G_OBJECT_CLASS (bobgui_text_tag_table_parent_class)->finalize (object);
}

static void
bobgui_text_tag_table_buildable_interface_init (BobguiBuildableIface   *iface)
{
  iface->add_child = bobgui_text_tag_table_buildable_add_child;
}

static void
bobgui_text_tag_table_buildable_add_child (BobguiBuildable        *buildable,
					BobguiBuilder          *builder,
					GObject             *child,
					const char          *type)
{
  if (type && strcmp (type, "tag") == 0)
    bobgui_text_tag_table_add (BOBGUI_TEXT_TAG_TABLE (buildable),
			    BOBGUI_TEXT_TAG (child));
}

/**
 * bobgui_text_tag_table_add:
 * @table: a `BobguiTextTagTable`
 * @tag: a `BobguiTextTag`
 *
 * Add a tag to the table.
 *
 * The tag is assigned the highest priority in the table.
 *
 * @tag must not be in a tag table already, and may not have
 * the same name as an already-added tag.
 *
 * Returns: %TRUE on success.
 */
gboolean
bobgui_text_tag_table_add (BobguiTextTagTable *table,
                        BobguiTextTag      *tag)
{
  BobguiTextTagTablePrivate *priv;
  guint size;

  g_return_val_if_fail (BOBGUI_IS_TEXT_TAG_TABLE (table), FALSE);
  g_return_val_if_fail (BOBGUI_IS_TEXT_TAG (tag), FALSE);
  g_return_val_if_fail (tag->priv->table == NULL, FALSE);

  priv = table->priv;

  if (tag->priv->name && g_hash_table_lookup (priv->hash, tag->priv->name))
    {
      g_warning ("A tag named '%s' is already in the tag table.",
                 tag->priv->name);
      return FALSE;
    }
  
  g_object_ref (tag);

  if (tag->priv->name)
    g_hash_table_insert (priv->hash, tag->priv->name, tag);
  else
    {
      priv->anonymous = g_slist_prepend (priv->anonymous, tag);
      priv->anon_count++;
    }

  tag->priv->table = table;

  /* We get the highest tag priority, as the most-recently-added
     tag. Note that we do NOT use bobgui_text_tag_set_priority,
     as it assumes the tag is already in the table. */
  size = bobgui_text_tag_table_get_size (table);
  g_assert (size > 0);
  tag->priv->priority = size - 1;

  check_visible (table, tag);

  g_signal_emit (table, signals[TAG_ADDED], 0, tag);
  return TRUE;
}

/**
 * bobgui_text_tag_table_lookup:
 * @table: a `BobguiTextTagTable`
 * @name: name of a tag
 *
 * Look up a named tag.
 *
 * Returns: (nullable) (transfer none): The tag
 */
BobguiTextTag*
bobgui_text_tag_table_lookup (BobguiTextTagTable *table,
                           const char      *name)
{
  BobguiTextTagTablePrivate *priv;

  g_return_val_if_fail (BOBGUI_IS_TEXT_TAG_TABLE (table), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  priv = table->priv;

  return g_hash_table_lookup (priv->hash, name);
}

/**
 * bobgui_text_tag_table_remove:
 * @table: a `BobguiTextTagTable`
 * @tag: a `BobguiTextTag`
 *
 * Remove a tag from the table.
 *
 * If a `BobguiTextBuffer` has @table as its tag table, the tag is
 * removed from the buffer. The table’s reference to the tag is
 * removed, so the tag will end up destroyed if you don’t have
 * a reference to it.
 */
void
bobgui_text_tag_table_remove (BobguiTextTagTable *table,
                           BobguiTextTag      *tag)
{
  BobguiTextTagTablePrivate *priv;
  GSList *l;

  g_return_if_fail (BOBGUI_IS_TEXT_TAG_TABLE (table));
  g_return_if_fail (BOBGUI_IS_TEXT_TAG (tag));
  g_return_if_fail (tag->priv->table == table);

  priv = table->priv;

  /* Our little bad hack to be sure buffers don't still have the tag
   * applied to text in the buffer
   */
  for (l = priv->buffers; l != NULL; l = l->next)
    _bobgui_text_buffer_notify_will_remove_tag (BOBGUI_TEXT_BUFFER (l->data),
                                             tag);

  /* Set ourselves to the highest priority; this means
     when we're removed, there won't be any gaps in the
     priorities of the tags in the table. */
  bobgui_text_tag_set_priority (tag, bobgui_text_tag_table_get_size (table) - 1);

  tag->priv->table = NULL;

  if (tag->priv->name)
    g_hash_table_remove (priv->hash, tag->priv->name);
  else
    {
      priv->anonymous = g_slist_remove (priv->anonymous, tag);
      priv->anon_count--;
    }

  g_signal_emit (table, signals[TAG_REMOVED], 0, tag);

  g_object_unref (tag);
}

struct ForeachData
{
  BobguiTextTagTableForeach func;
  gpointer data;
};

static void
hash_foreach (gpointer key, gpointer value, gpointer data)
{
  struct ForeachData *fd = data;

  g_return_if_fail (BOBGUI_IS_TEXT_TAG (value));

  (* fd->func) (value, fd->data);
}

static void
list_foreach (gpointer data, gpointer user_data)
{
  struct ForeachData *fd = user_data;

  g_return_if_fail (BOBGUI_IS_TEXT_TAG (data));

  (* fd->func) (data, fd->data);
}

/**
 * bobgui_text_tag_table_foreach:
 * @table: a `BobguiTextTagTable`
 * @func: (scope call): a function to call on each tag
 * @data: user data
 *
 * Calls @func on each tag in @table, with user data @data.
 *
 * Note that the table may not be modified while iterating
 * over it (you can’t add/remove tags).
 */
void
bobgui_text_tag_table_foreach (BobguiTextTagTable       *table,
                            BobguiTextTagTableForeach func,
                            gpointer               data)
{
  BobguiTextTagTablePrivate *priv;
  struct ForeachData d;

  g_return_if_fail (BOBGUI_IS_TEXT_TAG_TABLE (table));
  g_return_if_fail (func != NULL);

  priv = table->priv;

  d.func = func;
  d.data = data;

  g_hash_table_foreach (priv->hash, hash_foreach, &d);
  g_slist_foreach (priv->anonymous, list_foreach, &d);
}

/**
 * bobgui_text_tag_table_get_size:
 * @table: a `BobguiTextTagTable`
 *
 * Returns the size of the table (number of tags)
 *
 * Returns: number of tags in @table
 */
int
bobgui_text_tag_table_get_size (BobguiTextTagTable *table)
{
  BobguiTextTagTablePrivate *priv;

  g_return_val_if_fail (BOBGUI_IS_TEXT_TAG_TABLE (table), 0);

  priv = table->priv;

  return g_hash_table_size (priv->hash) + priv->anon_count;
}

void
_bobgui_text_tag_table_add_buffer (BobguiTextTagTable *table,
                                gpointer         buffer)
{
  BobguiTextTagTablePrivate *priv = table->priv;

  priv->buffers = g_slist_prepend (priv->buffers, buffer);
}

static void
foreach_remove_tag (BobguiTextTag *tag, gpointer data)
{
  BobguiTextBuffer *buffer;

  buffer = BOBGUI_TEXT_BUFFER (data);

  _bobgui_text_buffer_notify_will_remove_tag (buffer, tag);
}

void
_bobgui_text_tag_table_remove_buffer (BobguiTextTagTable *table,
                                   gpointer         buffer)
{
  BobguiTextTagTablePrivate *priv = table->priv;

  bobgui_text_tag_table_foreach (table, foreach_remove_tag, buffer);

  priv->buffers = g_slist_remove (priv->buffers, buffer);
}

void
_bobgui_text_tag_table_tag_changed (BobguiTextTagTable *table,
                                 BobguiTextTag      *tag,
                                 gboolean         size_changed)
{
  check_visible (table, tag);
  g_signal_emit (table, signals[TAG_CHANGED], 0, tag, size_changed);
}

gboolean
_bobgui_text_tag_table_affects_visibility (BobguiTextTagTable *table)
{
  return table->priv->seen_invisible;
}
