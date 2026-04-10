/* BOBGUI - The Bobgui Framework
 *
 * Copyright (C) 2010 Christian Dywan
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
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "bobguicomboboxtext.h"
#include "bobguicombobox.h"
#include "bobguicellrenderertext.h"
#include "bobguicelllayout.h"
#include "bobguibuildable.h"
#include "bobguibuilderprivate.h"
#include "bobguiliststore.h"

#include <string.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiComboBoxText:
 *
 * A `BobguiComboBoxText` is a simple variant of `BobguiComboBox` for text-only
 * use cases.
 *
 * <picture>
 *   <source srcset="combo-box-text-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiComboBoxText" src="combo-box-text.png">
 * </picture>
 *
 * `BobguiComboBoxText` hides the model-view complexity of `BobguiComboBox`.
 *
 * To create a `BobguiComboBoxText`, use [ctor@Bobgui.ComboBoxText.new] or
 * [ctor@Bobgui.ComboBoxText.new_with_entry].
 *
 * You can add items to a `BobguiComboBoxText` with
 * [method@Bobgui.ComboBoxText.append_text],
 * [method@Bobgui.ComboBoxText.insert_text] or
 * [method@Bobgui.ComboBoxText.prepend_text] and remove options with
 * [method@Bobgui.ComboBoxText.remove].
 *
 * If the `BobguiComboBoxText` contains an entry (via the
 * [property@Bobgui.ComboBox:has-entry] property), its contents can be retrieved
 * using [method@Bobgui.ComboBoxText.get_active_text].
 *
 * You should not call [method@Bobgui.ComboBox.set_model] or attempt to pack more
 * cells into this combo box via its [iface@Bobgui.CellLayout] interface.
 *
 * ## BobguiComboBoxText as BobguiBuildable
 *
 * The `BobguiComboBoxText` implementation of the `BobguiBuildable` interface supports
 * adding items directly using the `<items>` element and specifying `<item>`
 * elements for each item. Each `<item>` element can specify the “id”
 * corresponding to the appended text and also supports the regular
 * translation attributes “translatable”, “context” and “comments”.
 *
 * Here is a UI definition fragment specifying `BobguiComboBoxText` items:
 * ```xml
 * <object class="BobguiComboBoxText">
 *   <items>
 *     <item translatable="yes" id="factory">Factory</item>
 *     <item translatable="yes" id="home">Home</item>
 *     <item translatable="yes" id="subway">Subway</item>
 *   </items>
 * </object>
 * ```
 *
 * ## CSS nodes
 *
 * ```
 * combobox
 * ╰── box.linked
 *     ├── entry.combo
 *     ├── button.combo
 *     ╰── window.popup
 * ```
 *
 * `BobguiComboBoxText` has a single CSS node with name combobox. It adds
 * the style class .combo to the main CSS nodes of its entry and button
 * children, and the .linked class to the node of its internal box.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown] with a [class@Bobgui.StringList]
 *   instead
 */

typedef struct _BobguiComboBoxTextClass BobguiComboBoxTextClass;

struct _BobguiComboBoxText
{
  BobguiComboBox parent_instance;
};

struct _BobguiComboBoxTextClass
{
  BobguiComboBoxClass parent_class;
};


static void     bobgui_combo_box_text_buildable_interface_init   (BobguiBuildableIface  *iface);
static gboolean bobgui_combo_box_text_buildable_custom_tag_start (BobguiBuildable       *buildable,
                                                               BobguiBuilder         *builder,
                                                               GObject            *child,
                                                               const char         *tagname,
                                                               BobguiBuildableParser *parser,
                                                               gpointer           *data);

static void     bobgui_combo_box_text_buildable_custom_finished  (BobguiBuildable       *buildable,
                                                               BobguiBuilder         *builder,
                                                               GObject            *child,
                                                               const char         *tagname,
                                                               gpointer            user_data);


static BobguiBuildableIface *buildable_parent_iface = NULL;

G_DEFINE_TYPE_WITH_CODE (BobguiComboBoxText, bobgui_combo_box_text, BOBGUI_TYPE_COMBO_BOX,
			 G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
						bobgui_combo_box_text_buildable_interface_init));

static void
bobgui_combo_box_text_constructed (GObject *object)
{
  const int text_column = 0;

  G_OBJECT_CLASS (bobgui_combo_box_text_parent_class)->constructed (object);

  bobgui_combo_box_set_entry_text_column (BOBGUI_COMBO_BOX (object), text_column);
  bobgui_combo_box_set_id_column (BOBGUI_COMBO_BOX (object), 1);

  if (!bobgui_combo_box_get_has_entry (BOBGUI_COMBO_BOX (object)))
    {
      BobguiCellRenderer *cell;

      cell = bobgui_cell_renderer_text_new ();
      bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (object), cell, TRUE);
      bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (object), cell,
                                      "text", text_column,
                                      NULL);
    }
}

static void
bobgui_combo_box_text_init (BobguiComboBoxText *combo_box)
{
  BobguiListStore *store;

  store = bobgui_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
  bobgui_combo_box_set_model (BOBGUI_COMBO_BOX (combo_box), BOBGUI_TREE_MODEL (store));
  g_object_unref (store);
}

static void
bobgui_combo_box_text_class_init (BobguiComboBoxTextClass *klass)
{
  GObjectClass *object_class;

  object_class = (GObjectClass *)klass;
  object_class->constructed = bobgui_combo_box_text_constructed;
}

static void
bobgui_combo_box_text_buildable_interface_init (BobguiBuildableIface *iface)
{
  buildable_parent_iface = g_type_interface_peek_parent (iface);

  iface->custom_tag_start = bobgui_combo_box_text_buildable_custom_tag_start;
  iface->custom_finished = bobgui_combo_box_text_buildable_custom_finished;
}

typedef struct {
  BobguiBuilder    *builder;
  GObject       *object;
  const char    *domain;
  char          *id;

  GString       *string;

  char          *context;
  guint          translatable : 1;

  guint          is_text : 1;
} ItemParserData;

static void
item_start_element (BobguiBuildableParseContext  *context,
                    const char                *element_name,
                    const char               **names,
                    const char               **values,
                    gpointer                   user_data,
                    GError                   **error)
{
  ItemParserData *data = (ItemParserData*)user_data;

  if (strcmp (element_name, "items") == 0)
    {
      if (!_bobgui_builder_check_parent (data->builder, context, "object", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_INVALID, NULL, NULL,
                                        G_MARKUP_COLLECT_INVALID))
        _bobgui_builder_prefix_error (data->builder, context, error);
    }
  else if (strcmp (element_name, "item") == 0)
    {
      const char *id = NULL;
      gboolean translatable = FALSE;
      const char *msg_context = NULL;

      if (!_bobgui_builder_check_parent (data->builder, context, "items", error))
        return;

      if (!g_markup_collect_attributes (element_name, names, values, error,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "id", &id,
                                        G_MARKUP_COLLECT_BOOLEAN|G_MARKUP_COLLECT_OPTIONAL, "translatable", &translatable,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "comments", NULL,
                                        G_MARKUP_COLLECT_STRING|G_MARKUP_COLLECT_OPTIONAL, "context", &msg_context,
                                        G_MARKUP_COLLECT_INVALID))
        {
          _bobgui_builder_prefix_error (data->builder, context, error);
          return;
        }

      data->is_text = TRUE;
      data->translatable = translatable;
      data->context = g_strdup (msg_context);
      data->id = g_strdup (id);
    }
  else
    {
      _bobgui_builder_error_unhandled_tag (data->builder, context,
                                        "BobguiComboBoxText", element_name,
                                        error);
    }
}

static void
item_text (BobguiBuildableParseContext  *context,
           const char                *text,
           gsize                      text_len,
           gpointer                   user_data,
           GError                   **error)
{
  ItemParserData *data = (ItemParserData*)user_data;

  if (data->is_text)
    g_string_append_len (data->string, text, text_len);
}

static void
item_end_element (BobguiBuildableParseContext  *context,
                  const char                *element_name,
                  gpointer                   user_data,
                  GError                   **error)
{
  ItemParserData *data = (ItemParserData*)user_data;

  /* Append the translated strings */
  if (data->string->len)
    {
      if (data->translatable)
	{
	  const char *translated;

	  translated = _bobgui_builder_parser_translate (data->domain,
						      data->context,
						      data->string->str);
	  g_string_assign (data->string, translated);
	}

      bobgui_combo_box_text_append (BOBGUI_COMBO_BOX_TEXT (data->object), data->id, data->string->str);
    }

  data->translatable = FALSE;
  g_string_set_size (data->string, 0);
  g_clear_pointer (&data->context, g_free);
  g_clear_pointer (&data->id, g_free);
  data->is_text = FALSE;
}

static const BobguiBuildableParser item_parser =
  {
    item_start_element,
    item_end_element,
    item_text
  };

static gboolean
bobgui_combo_box_text_buildable_custom_tag_start (BobguiBuildable       *buildable,
                                               BobguiBuilder         *builder,
                                               GObject            *child,
                                               const char         *tagname,
                                               BobguiBuildableParser *parser,
                                               gpointer           *parser_data)
{
  if (buildable_parent_iface->custom_tag_start (buildable, builder, child,
						tagname, parser, parser_data))
    return TRUE;

  if (strcmp (tagname, "items") == 0)
    {
      ItemParserData *data;

      data = g_slice_new0 (ItemParserData);
      data->builder = g_object_ref (builder);
      data->object = (GObject *) g_object_ref (buildable);
      data->domain = bobgui_builder_get_translation_domain (builder);
      data->string = g_string_new ("");

      *parser = item_parser;
      *parser_data = data;

      return TRUE;
    }

  return FALSE;
}

static void
bobgui_combo_box_text_buildable_custom_finished (BobguiBuildable *buildable,
                                              BobguiBuilder   *builder,
                                              GObject      *child,
                                              const char   *tagname,
                                              gpointer      user_data)
{
  ItemParserData *data;

  buildable_parent_iface->custom_finished (buildable, builder, child,
					   tagname, user_data);

  if (strcmp (tagname, "items") == 0)
    {
      data = (ItemParserData*)user_data;

      g_object_unref (data->object);
      g_object_unref (data->builder);
      g_string_free (data->string, TRUE);
      g_slice_free (ItemParserData, data);
    }
}

/**
 * bobgui_combo_box_text_new:
 *
 * Creates a new `BobguiComboBoxText`.
 *
 * Returns: A new `BobguiComboBoxText`
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
BobguiWidget *
bobgui_combo_box_text_new (void)
{
  return g_object_new (BOBGUI_TYPE_COMBO_BOX_TEXT,
                       NULL);
}

/**
 * bobgui_combo_box_text_new_with_entry:
 *
 * Creates a new `BobguiComboBoxText` with an entry.
 *
 * Returns: a new `BobguiComboBoxText`
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
BobguiWidget *
bobgui_combo_box_text_new_with_entry (void)
{
  return g_object_new (BOBGUI_TYPE_COMBO_BOX_TEXT,
                       "has-entry", TRUE,
                       NULL);
}

/**
 * bobgui_combo_box_text_append_text:
 * @combo_box: A `BobguiComboBoxText`
 * @text: A string
 *
 * Appends @text to the list of strings stored in @combo_box.
 *
 * This is the same as calling [method@Bobgui.ComboBoxText.insert_text]
 * with a position of -1.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
void
bobgui_combo_box_text_append_text (BobguiComboBoxText *combo_box,
                                const char      *text)
{
  bobgui_combo_box_text_insert (combo_box, -1, NULL, text);
}

/**
 * bobgui_combo_box_text_prepend_text:
 * @combo_box: A `BobguiComboBox`
 * @text: A string
 *
 * Prepends @text to the list of strings stored in @combo_box.
 *
 * This is the same as calling [method@Bobgui.ComboBoxText.insert_text]
 * with a position of 0.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
void
bobgui_combo_box_text_prepend_text (BobguiComboBoxText *combo_box,
                                 const char      *text)
{
  bobgui_combo_box_text_insert (combo_box, 0, NULL, text);
}

/**
 * bobgui_combo_box_text_insert_text:
 * @combo_box: A `BobguiComboBoxText`
 * @position: An index to insert @text
 * @text: A string
 *
 * Inserts @text at @position in the list of strings stored in @combo_box.
 *
 * If @position is negative then @text is appended.
 *
 * This is the same as calling [method@Bobgui.ComboBoxText.insert]
 * with a %NULL ID string.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
void
bobgui_combo_box_text_insert_text (BobguiComboBoxText *combo_box,
                                int              position,
                                const char      *text)
{
  bobgui_combo_box_text_insert (combo_box, position, NULL, text);
}

/**
 * bobgui_combo_box_text_append:
 * @combo_box: A `BobguiComboBoxText`
 * @id: (nullable): a string ID for this value
 * @text: A string
 *
 * Appends @text to the list of strings stored in @combo_box.
 *
 * If @id is non-%NULL then it is used as the ID of the row.
 *
 * This is the same as calling [method@Bobgui.ComboBoxText.insert]
 * with a position of -1.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
void
bobgui_combo_box_text_append (BobguiComboBoxText *combo_box,
                           const char      *id,
                           const char      *text)
{
  bobgui_combo_box_text_insert (combo_box, -1, id, text);
}

/**
 * bobgui_combo_box_text_prepend:
 * @combo_box: A `BobguiComboBox`
 * @id: (nullable): a string ID for this value
 * @text: a string
 *
 * Prepends @text to the list of strings stored in @combo_box.
 *
 * If @id is non-%NULL then it is used as the ID of the row.
 *
 * This is the same as calling [method@Bobgui.ComboBoxText.insert]
 * with a position of 0.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
void
bobgui_combo_box_text_prepend (BobguiComboBoxText *combo_box,
                            const char      *id,
                            const char      *text)
{
  bobgui_combo_box_text_insert (combo_box, 0, id, text);
}


/**
 * bobgui_combo_box_text_insert:
 * @combo_box: A `BobguiComboBoxText`
 * @position: An index to insert @text
 * @id: (nullable): a string ID for this value
 * @text: A string to display
 *
 * Inserts @text at @position in the list of strings stored in @combo_box.
 *
 * If @id is non-%NULL then it is used as the ID of the row.
 * See [property@Bobgui.ComboBox:id-column].
 *
 * If @position is negative then @text is appended.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
void
bobgui_combo_box_text_insert (BobguiComboBoxText *combo_box,
                           int              position,
                           const char      *id,
                           const char      *text)
{
  BobguiListStore *store;
  BobguiTreeIter iter;
  int text_column;

  g_return_if_fail (BOBGUI_IS_COMBO_BOX_TEXT (combo_box));
  g_return_if_fail (text != NULL);

  store = BOBGUI_LIST_STORE (bobgui_combo_box_get_model (BOBGUI_COMBO_BOX (combo_box)));
  g_return_if_fail (BOBGUI_IS_LIST_STORE (store));

  text_column = bobgui_combo_box_get_entry_text_column (BOBGUI_COMBO_BOX (combo_box));

  if (bobgui_combo_box_get_has_entry (BOBGUI_COMBO_BOX (combo_box)))
    g_return_if_fail (text_column >= 0);
  else if (text_column < 0)
    text_column = 0;

  g_return_if_fail (bobgui_tree_model_get_column_type (BOBGUI_TREE_MODEL (store), text_column) == G_TYPE_STRING);

  if (position < 0)
    bobgui_list_store_append (store, &iter);
  else
    bobgui_list_store_insert (store, &iter, position);

  bobgui_list_store_set (store, &iter, text_column, text, -1);

  if (id != NULL)
    {
      int id_column;

      id_column = bobgui_combo_box_get_id_column (BOBGUI_COMBO_BOX (combo_box));
      g_return_if_fail (id_column >= 0);
      g_return_if_fail (bobgui_tree_model_get_column_type (BOBGUI_TREE_MODEL (store), id_column) == G_TYPE_STRING);

      bobgui_list_store_set (store, &iter, id_column, id, -1);
    }
}

/**
 * bobgui_combo_box_text_remove:
 * @combo_box: A `BobguiComboBox`
 * @position: Index of the item to remove
 *
 * Removes the string at @position from @combo_box.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
void
bobgui_combo_box_text_remove (BobguiComboBoxText *combo_box,
                           int              position)
{
  BobguiTreeModel *model;
  BobguiListStore *store;
  BobguiTreeIter iter;

  g_return_if_fail (BOBGUI_IS_COMBO_BOX_TEXT (combo_box));
  g_return_if_fail (position >= 0);

  model = bobgui_combo_box_get_model (BOBGUI_COMBO_BOX (combo_box));
  store = BOBGUI_LIST_STORE (model);
  g_return_if_fail (BOBGUI_IS_LIST_STORE (store));

  if (bobgui_tree_model_iter_nth_child (model, &iter, NULL, position))
    bobgui_list_store_remove (store, &iter);
}

/**
 * bobgui_combo_box_text_remove_all:
 * @combo_box: A `BobguiComboBoxText`
 *
 * Removes all the text entries from the combo box.
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
void
bobgui_combo_box_text_remove_all (BobguiComboBoxText *combo_box)
{
  BobguiListStore *store;

  g_return_if_fail (BOBGUI_IS_COMBO_BOX_TEXT (combo_box));

  store = BOBGUI_LIST_STORE (bobgui_combo_box_get_model (BOBGUI_COMBO_BOX (combo_box)));
  bobgui_list_store_clear (store);
}

/**
 * bobgui_combo_box_text_get_active_text:
 * @combo_box: A `BobguiComboBoxText`
 *
 * Returns the currently active string in @combo_box.
 *
 * If no row is currently selected, %NULL is returned.
 * If @combo_box contains an entry, this function will
 * return its contents (which will not necessarily
 * be an item from the list).
 *
 * Returns: (nullable) (transfer full): a newly allocated
 *   string containing the currently active text.
 *   Must be freed with g_free().
 *
 * Deprecated: 4.10: Use [class@Bobgui.DropDown]
 */
char *
bobgui_combo_box_text_get_active_text (BobguiComboBoxText *combo_box)
{
  BobguiTreeIter iter;
  char *text = NULL;

  g_return_val_if_fail (BOBGUI_IS_COMBO_BOX_TEXT (combo_box), NULL);

 if (bobgui_combo_box_get_has_entry (BOBGUI_COMBO_BOX (combo_box)))
   {
     BobguiWidget *entry;

     entry = bobgui_combo_box_get_child (BOBGUI_COMBO_BOX (combo_box));
     text = g_strdup (bobgui_editable_get_text (BOBGUI_EDITABLE (entry)));
   }
  else if (bobgui_combo_box_get_active_iter (BOBGUI_COMBO_BOX (combo_box), &iter))
    {
      BobguiTreeModel *model;
      int text_column;

      model = bobgui_combo_box_get_model (BOBGUI_COMBO_BOX (combo_box));
      g_return_val_if_fail (BOBGUI_IS_LIST_STORE (model), NULL);
      text_column = bobgui_combo_box_get_entry_text_column (BOBGUI_COMBO_BOX (combo_box));
      g_return_val_if_fail (text_column >= 0, NULL);
      g_return_val_if_fail (bobgui_tree_model_get_column_type (model, text_column) == G_TYPE_STRING, NULL);
      bobgui_tree_model_get (model, &iter, text_column, &text, -1);
    }

  return text;
}
