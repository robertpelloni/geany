/* bobguiaccessibletext.c: Interface for accessible text objects
 *
 * SPDX-FileCopyrightText: 2023  Emmanuele Bassi
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "bobguiaccessibletextprivate.h"

#include "bobguiatcontextprivate.h"

G_DEFINE_INTERFACE (BobguiAccessibleText, bobgui_accessible_text, BOBGUI_TYPE_ACCESSIBLE)

static GBytes *
bobgui_accessible_text_default_get_contents (BobguiAccessibleText *self,
                                          unsigned int start,
                                          unsigned int end)
{
  g_warning ("BobguiAccessibleText::get_contents not implemented for %s",
             G_OBJECT_TYPE_NAME (self));

  return NULL;
}

static GBytes *
bobgui_accessible_text_default_get_contents_at (BobguiAccessibleText            *self,
                                             unsigned int                  offset,
                                             BobguiAccessibleTextGranularity  granularity,
                                             unsigned int                 *start,
                                             unsigned int                 *end)
{
  g_warning ("BobguiAccessibleText::get_contents_at not implemented for %s",
             G_OBJECT_TYPE_NAME (self));

  if (start != NULL)
    *start = 0;
  if (end != NULL)
    *end = 0;

  return NULL;
}

static unsigned int
bobgui_accessible_text_default_get_caret_position (BobguiAccessibleText *self)
{
  return 0;
}

static gboolean
bobgui_accessible_text_default_get_selection (BobguiAccessibleText       *self,
                                           gsize                   *n_ranges,
                                           BobguiAccessibleTextRange **ranges)
{
  return FALSE;
}

static gboolean
bobgui_accessible_text_default_get_attributes (BobguiAccessibleText        *self,
                                            unsigned int              offset,
                                            gsize                    *n_ranges,
                                            BobguiAccessibleTextRange  **ranges,
                                            char                   ***attribute_names,
                                            char                   ***attribute_values)
{
  *attribute_names = NULL;
  *attribute_values = NULL;
  *n_ranges = 0;
  return FALSE;
}

static void
bobgui_accessible_text_default_get_default_attributes (BobguiAccessibleText   *self,
                                                    char              ***attribute_names,
                                                    char              ***attribute_values)
{
  *attribute_names = g_new0 (char *, 1);
  *attribute_values = g_new0 (char *, 1);
}

static gboolean
bobgui_accessible_text_default_set_caret_position (BobguiAccessibleText *self,
                                                unsigned int       offset)
{
  return FALSE;
}

static gboolean
bobgui_accessible_text_default_set_selection (BobguiAccessibleText      *self,
                                           gsize                   i,
                                           BobguiAccessibleTextRange *range)
{
  return FALSE;
}

static void
bobgui_accessible_text_default_init (BobguiAccessibleTextInterface *iface)
{
  iface->get_contents = bobgui_accessible_text_default_get_contents;
  iface->get_contents_at = bobgui_accessible_text_default_get_contents_at;
  iface->get_caret_position = bobgui_accessible_text_default_get_caret_position;
  iface->get_selection = bobgui_accessible_text_default_get_selection;
  iface->get_attributes = bobgui_accessible_text_default_get_attributes;
  iface->get_default_attributes = bobgui_accessible_text_default_get_default_attributes;
  iface->set_caret_position = bobgui_accessible_text_default_set_caret_position;
  iface->set_selection = bobgui_accessible_text_default_set_selection;
}

static GBytes *
nul_terminate_contents (GBytes *bytes)
{
  const char *data;
  gsize size;

  data = g_bytes_get_data (bytes, &size);
  if (size == 0 || (size > 0 && data[size - 1] != '\0'))
    {
      guchar *copy;

      copy = g_new (guchar, size + 1);
      if (size > 0)
        memcpy (copy, data, size);
      copy[size] = '\0';

      g_bytes_unref (bytes);
      bytes = g_bytes_new_take (copy, size + 1);
    }

  return bytes;
}

/*< private >
 * bobgui_accessible_text_get_contents:
 * @self: the accessible object
 * @start: the beginning of the range, in characters
 * @end: the end of the range, in characters
 *
 * Retrieve the current contents of the accessible object within
 * the given range.
 *
 * If @end is `G_MAXUINT`, the end of the range is the full content of the
 * accessible object.
 *
 * Returns: (transfer full): the requested slice of the contents of the
 *   accessible object, as NUL-terminated UTF-8
 *
 * Since: 4.14
 */
GBytes *
bobgui_accessible_text_get_contents (BobguiAccessibleText *self,
                                  unsigned int       start,
                                  unsigned int       end)
{
  GBytes *bytes;

  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE_TEXT (self), NULL);
  g_return_val_if_fail (end >= start, NULL);

  bytes = BOBGUI_ACCESSIBLE_TEXT_GET_IFACE (self)->get_contents (self, start, end);

  return nul_terminate_contents (bytes);
}

/*< private >
 * bobgui_accessible_text_get_contents_at:
 * @self: the accessible object
 * @offset: the offset of the text to retrieve
 * @granularity: specify the boundaries of the text
 * @start: (out): the starting offset of the contents, in characters
 * @end: (out): the ending offset of the contents, in characters
 *
 * Retrieve the current contents of the accessible object at the given
 * offset.
 *
 * Using the @granularity enumeration allows to adjust the offset so that
 * this function can return the beginning of the word, line, or sentence;
 * the initial and final boundaries are stored in @start and @end.
 *
 * Returns: (transfer full): the requested slice of the contents of the
 *   accessible object, as NUL-terminated UTF-8 buffer
 *
 * Since: 4.14
 */
GBytes *
bobgui_accessible_text_get_contents_at (BobguiAccessibleText            *self,
                                     unsigned int                  offset,
                                     BobguiAccessibleTextGranularity  granularity,
                                     unsigned int                 *start,
                                     unsigned int                 *end)
{
  static const char empty[] = {0};
  GBytes *bytes;

  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE_TEXT (self), NULL);

  bytes = BOBGUI_ACCESSIBLE_TEXT_GET_IFACE (self)->get_contents_at (self, offset, granularity, start, end);

  if (bytes == NULL)
    return g_bytes_new_static (empty, sizeof empty);

  return nul_terminate_contents (bytes);
}

/*< private >
 * bobgui_accessible_text_get_character_count:
 * @self: the accessible object
 *
 * Returns the amount of characters that the text contains.
 *
 * Returns: the length of the text, in characters
 *
 * Since: 4.18
 */
unsigned int
bobgui_accessible_text_get_character_count (BobguiAccessibleText *self)
{
  GBytes *contents;
  const char *str;
  gsize len;

  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE_TEXT (self), 0);

  contents = bobgui_accessible_text_get_contents (self, 0, G_MAXUINT);
  str = g_bytes_get_data (contents, NULL);
  len = g_utf8_strlen (str, -1);
  g_bytes_unref (contents);

  return len;
}

/*< private >
 * bobgui_accessible_text_get_caret_position:
 * @self: the accessible object
 *
 * Retrieves the position of the caret inside the accessible object.
 *
 * If the accessible has no caret, 0 is returned.
 *
 * Returns: the position of the caret, in characters
 *
 * Since: 4.14
 */
unsigned int
bobgui_accessible_text_get_caret_position (BobguiAccessibleText *self)
{
  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE_TEXT (self), 0);

  return BOBGUI_ACCESSIBLE_TEXT_GET_IFACE (self)->get_caret_position (self);
}

/*< private >
 * bobgui_accessible_text_get_selection:
 * @self: the accessible object
 * @n_ranges: (out): the number of selection ranges
 * @ranges: (optional) (out) (array length=n_ranges): the selection ranges
 *
 * Retrieves the selection ranges in the accessible object.
 *
 * If this function returns true, `n_ranges` will be set to a value
 * greater than or equal to one, and @ranges will be set to a newly
 * allocated array of [struct#Bobgui.AccessibleTextRange].
 *
 * Returns: true if there's at least one selected range inside the
 *   accessible object, and false otherwise
 *
 * Since: 4.14
 */
gboolean
bobgui_accessible_text_get_selection (BobguiAccessibleText       *self,
                                   gsize                   *n_ranges,
                                   BobguiAccessibleTextRange **ranges)
{
  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE_TEXT (self), FALSE);

  return BOBGUI_ACCESSIBLE_TEXT_GET_IFACE (self)->get_selection (self, n_ranges, ranges);
}

/*< private >
 * bobgui_accessible_text_get_attributes:
 * @self: the accessible object
 * @offset: the offset, in characters
 * @n_ranges: (out): the number of attributes
 * @ranges: (out) (array length=n_attributes) (optional): the ranges of the attributes
 *   inside the accessible object
 * @attribute_names: (out) (array zero-terminated=1) (element-type utf8) (optional) (transfer full):
 *   the names of the attributes inside the accessible object
 * @attribute_values: (out) (array zero-terminated=1) (element-type utf8) (optional) (transfer full):
 *   the values of the attributes inside the accessible object
 *
 * Retrieves the text attributes inside the accessible object.
 *
 * Each attribute is composed by:
 *
 * - a range
 * - a name, typically in the form of a reverse DNS identifier
 * - a value
 *
 * If this function returns true, `n_attributes` will be set to a value
 * greater than or equal to one, @ranges will be set to a newly
 * allocated array of [struct#Bobgui.AccessibleTextRange] which should
 * be freed with g_free(), @attribute_names and @attribute_values
 * will be set to string arrays that should be freed with g_strfreev().
 *
 * Returns: true if the accessible object has at least an attribute,
 *   and false otherwise
 *
 * Since: 4.14
 */
gboolean
bobgui_accessible_text_get_attributes (BobguiAccessibleText        *self,
                                    unsigned int              offset,
                                    gsize                    *n_ranges,
                                    BobguiAccessibleTextRange  **ranges,
                                    char                   ***attribute_names,
                                    char                   ***attribute_values)
{
  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE_TEXT (self), FALSE);

  return BOBGUI_ACCESSIBLE_TEXT_GET_IFACE (self)->get_attributes (self,
                                                               offset,
                                                               n_ranges,
                                                               ranges,
                                                               attribute_names,
                                                               attribute_values);
}

/*< private >
 * bobgui_accessible_text_get_default_attributes:
 * @self: the accessible object
 * @attribute_names: (out) (array zero-terminated=1) (element-type utf8) (optional) (transfer full):
 *   the names of the attributes inside the accessible object
 * @attribute_values: (out) (array zero-terminated=1) (element-type utf8) (optional) (transfer full):
 *   the values of the attributes inside the accessible object
 *
 * Retrieves the default text attributes inside the accessible object.
 *
 * Each attribute is composed by:
 *
 * - a name, typically in the form of a reverse DNS identifier
 * - a value
 *
 * If this function returns true, @attribute_names and @attribute_values
 * will be set to string arrays that should be freed with g_strfreev().
 *
 * Since: 4.14
 */
void
bobgui_accessible_text_get_default_attributes (BobguiAccessibleText   *self,
                                            char              ***attribute_names,
                                            char              ***attribute_values)
{
  g_return_if_fail (BOBGUI_IS_ACCESSIBLE_TEXT (self));

  BOBGUI_ACCESSIBLE_TEXT_GET_IFACE (self)->get_default_attributes (self,
                                                                attribute_names,
                                                                attribute_values);
}

/*< private >
 * bobgui_accessible_text_get_attributes_run:
 * @self: the accessible object
 * @offset: the offset, in characters
 * @include_defaults: whether to include the default attributes in the
 *   returned array
 * @n_attributes: (out): the number of attributes
 * @attribute_names: (out) (array zero-terminated=1) (element-type utf8) (optional) (transfer full):
 *   the names of the attributes inside the accessible object
 * @attribute_values: (out) (array zero-terminated=1) (element-type utf8) (optional) (transfer full):
 *   the values of the attributes inside the accessible object
 * @start (out): the start index of the attribute run (portion of text where attributes are the same)
 * @end (out): the end index of the attribute run (portion of text where attributes are the same)
 *
 * Retrieves the text attributes inside the accessible object.
 *
 * Each attribute is composed by:
 *
 * - a range
 * - a name, typically in the form of a reverse DNS identifier
 * - a value
 *
 * If this function returns true, `n_attributes` will be set to a value
 * greater than or equal to one, @attribute_names and @attribute_values
 * will be set to string arrays that should be freed with g_strfreev()
 * and @start and @end will be set to the start and end (character) index
 * of the run.
 *
 * Returns: true if the accessible object has at least an attribute,
 *   and false otherwise
 *
 * Since: 4.18
 */
gboolean
bobgui_accessible_text_get_attributes_run (BobguiAccessibleText        *self,
                                        unsigned int              offset,
                                        gboolean                  include_defaults,
                                        gsize                    *n_attributes,
                                        char                   ***attribute_names,
                                        char                   ***attribute_values,
                                        int                      *start,
                                        int                      *end)
{
  GHashTable *attrs;
  GHashTableIter attr_iter;
  gpointer key, value;
  char **attr_names, **attr_values;
  gboolean res;
  GStrvBuilder *names_builder;
  GStrvBuilder *values_builder;
  BobguiAccessibleTextRange *ranges = NULL;

  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE_TEXT (self), FALSE);

  attrs = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  if (include_defaults)
    {
      bobgui_accessible_text_get_default_attributes (self,
                                                  &attr_names,
                                                  &attr_values);

      for (unsigned i = 0; attr_names[i] != NULL; i++)
        {
          g_hash_table_insert (attrs,
                               g_steal_pointer (&attr_names[i]),
                               g_steal_pointer (&attr_values[i]));
        }

      g_free (attr_names);
      g_free (attr_values);
      attr_names = NULL;
      attr_values = NULL;
    }

  res = bobgui_accessible_text_get_attributes (self,
                                            offset,
                                            n_attributes,
                                            &ranges,
                                            &attr_names,
                                            &attr_values);

  /* If there are no attributes, we can bail out early */
  if (!res && !include_defaults)
    {
      g_hash_table_unref (attrs);
      *attribute_names = NULL;
      *attribute_values = NULL;
      return FALSE;
    }

  *start = 0;
  *end = G_MAXINT;

  /* The text attributes override the default ones */
  for (unsigned i = 0; i < *n_attributes; i++)
    {
      g_hash_table_insert (attrs,
                           g_steal_pointer (&attr_names[i]),
                           g_steal_pointer (&attr_values[i]));
      *start = MAX (*start, ranges[i].start);
      *end = MIN (*end, *start + ranges[i].length);
    }

  if (*end == G_MAXINT)
    *end = bobgui_accessible_text_get_character_count (self);

  g_free (attr_names);
  g_free (attr_values);

  names_builder = g_strv_builder_new ();
  values_builder = g_strv_builder_new ();
  g_hash_table_iter_init (&attr_iter, attrs);
  *n_attributes = 0;
  while (g_hash_table_iter_next (&attr_iter, &key, &value))
    {
      g_strv_builder_add (names_builder, key);
      g_strv_builder_add (values_builder, value);
      (*n_attributes)++;
    }

  *attribute_names = g_strv_builder_end (names_builder);
  *attribute_values = g_strv_builder_end (values_builder);

  g_strv_builder_unref (names_builder);
  g_strv_builder_unref (values_builder);
  g_hash_table_unref (attrs);
  g_clear_pointer (&ranges, g_free);

  return *n_attributes > 0;
}

/*< private >
 * bobgui_accessible_text_get_extents:
 * @self: a `BobguiAccessibleText`
 * @start: start offset, in characters
 * @end: end offset, in characters
 * @extents: (out caller-allocates): return location for the extents
 *
 * Obtains the extents of a range of text, in widget coordinates.
 *
 * Returns: true if the extents were filled in, false otherwise
 *
 * Since: 4.16
 */
gboolean
bobgui_accessible_text_get_extents (BobguiAccessibleText *self,
                                 unsigned int       start,
                                 unsigned int       end,
                                 graphene_rect_t   *extents)
{
  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE_TEXT (self), FALSE);
  g_return_val_if_fail (start <= end, FALSE);
  g_return_val_if_fail (extents != NULL, FALSE);

  if (BOBGUI_ACCESSIBLE_TEXT_GET_IFACE (self)->get_extents != NULL)
    return BOBGUI_ACCESSIBLE_TEXT_GET_IFACE (self)->get_extents (self, start, end, extents);

  return FALSE;
}

/*< private >
 * bobgui_accessible_get_text_offset:
 * @self: a `BobguiAccessibleText`
 * @point: a point in widget coordinates
 * @offset: (out): return location for the text offset at @point
 *
 * Determines the text offset at the given position in the
 * widget.
 *
 * Returns: true if the offset was set, and false otherwise
 */
gboolean
bobgui_accessible_text_get_offset (BobguiAccessibleText      *self,
                                const graphene_point_t *point,
                                unsigned int           *offset)
{
  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE_TEXT (self), FALSE);

  if (BOBGUI_ACCESSIBLE_TEXT_GET_IFACE (self)->get_offset != NULL)
    return BOBGUI_ACCESSIBLE_TEXT_GET_IFACE (self)->get_offset (self, point, offset);

  return FALSE;
}

/*< private >
 * bobgui_accessible_text_set_caret_position:
 * @self: the accessible object
 * @offset: the text offset in characters
 *
 * Sets the caret position.
 *
 * Returns: true if the caret position was updated
 */
gboolean
bobgui_accessible_text_set_caret_position (BobguiAccessibleText *self,
                                        unsigned int       offset)
{
  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE_TEXT (self), FALSE);

  return BOBGUI_ACCESSIBLE_TEXT_GET_IFACE (self)->set_caret_position (self, offset);
}

/*< private >
 * bobgui_accessible_text_set_selection:
 * @self: the accessible object
 * @i: the selection to set
 * @range: the range to set the selection to
 *
 * Sets the caret position.
 *
 * Returns: true if the selection was updated
 */
gboolean
bobgui_accessible_text_set_selection (BobguiAccessibleText      *self,
                                   gsize                   i,
                                   BobguiAccessibleTextRange *range)
{
  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE_TEXT (self), FALSE);

  return BOBGUI_ACCESSIBLE_TEXT_GET_IFACE (self)->set_selection (self, i, range);
}

/**
 * bobgui_accessible_text_update_caret_position:
 * @self: the accessible object
 *
 * Updates the position of the caret.
 *
 * Implementations of the `BobguiAccessibleText` interface should call this
 * function every time the caret has moved, in order to notify assistive
 * technologies.
 *
 * Since: 4.14
 */
void
bobgui_accessible_text_update_caret_position (BobguiAccessibleText *self)
{
  BobguiATContext *context;

  g_return_if_fail (BOBGUI_IS_ACCESSIBLE_TEXT (self));

  context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (self));
  if (context == NULL)
    return;

  bobgui_at_context_update_caret_position (context);

  g_object_unref (context);
}

/**
 * bobgui_accessible_text_update_selection_bound:
 * @self: the accessible object
 *
 * Updates the boundary of the selection.
 *
 * Implementations of the `BobguiAccessibleText` interface should call this
 * function every time the selection has moved, in order to notify assistive
 * technologies.
 *
 * Since: 4.14
 */
void
bobgui_accessible_text_update_selection_bound (BobguiAccessibleText *self)
{
  BobguiATContext *context;

  g_return_if_fail (BOBGUI_IS_ACCESSIBLE_TEXT (self));

  context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (self));
  if (context == NULL)
    return;

  bobgui_at_context_update_selection_bound (context);

  g_object_unref (context);
}

/**
 * bobgui_accessible_text_update_contents:
 * @self: the accessible object
 * @change: the type of change in the contents
 * @start: the starting offset of the change, in characters
 * @end: the end offset of the change, in characters
 *
 * Notifies assistive technologies of a change in contents.
 *
 * Implementations of the `BobguiAccessibleText` interface should call this
 * function every time their contents change as the result of an operation,
 * like an insertion or a removal.
 *
 * Note: If the change is a deletion, this function must be called *before*
 * removing the contents, if it is an insertion, it must be called *after*
 * inserting the new contents.
 *
 * Since: 4.14
 */
void
bobgui_accessible_text_update_contents (BobguiAccessibleText              *self,
                                     BobguiAccessibleTextContentChange  change,
                                     unsigned int                    start,
                                     unsigned int                    end)
{
  BobguiATContext *context;

  g_return_if_fail (BOBGUI_IS_ACCESSIBLE_TEXT (self));

  context = bobgui_accessible_get_at_context (BOBGUI_ACCESSIBLE (self));
  if (context == NULL)
    return;

  bobgui_at_context_update_text_contents (context, change, start, end);

  g_object_unref (context);
}
