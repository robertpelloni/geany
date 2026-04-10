/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2011 Red Hat, Inc.
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

#include "bobguicsssection.h"

#include "bobguicssparserprivate.h"

struct _BobguiCssSection
{
  int                 ref_count;
  BobguiCssSection      *parent;
  GFile              *file;
  GBytes             *bytes;
  BobguiCssLocation      start_location;
  BobguiCssLocation      end_location;   /* end location if parser is %NULL */
};

G_DEFINE_BOXED_TYPE (BobguiCssSection, bobgui_css_section, bobgui_css_section_ref, bobgui_css_section_unref)

/**
 * bobgui_css_section_new: (constructor)
 * @file: (nullable) (transfer none): The file this section refers to
 * @start: The start location
 * @end: The end location
 *
 * Creates a new `BobguiCssSection` referring to the section
 * in the given `file` from the `start` location to the
 * `end` location.
 *
 * Returns: (transfer full): a new `BobguiCssSection`
 **/
BobguiCssSection *
bobgui_css_section_new (GFile                *file,
                     const BobguiCssLocation *start,
                     const BobguiCssLocation *end)
{
  return bobgui_css_section_new_with_bytes (file, NULL,start, end);
}

/**
 * bobgui_css_section_new_with_bytes: (constructor)
 * @file: (nullable) (transfer none): The file this section refers to
 * @bytes: (nullable) (transfer none): The bytes this sections refers to
 * @start: The start location
 * @end: The end location
 *
 * Creates a new `BobguiCssSection` referring to the section
 * in the given `file` or the given `bytes` from the `start` location to the
 * `end` location.
 *
 * Returns: (transfer full): a new `BobguiCssSection`
 *
 * Since: 4.16
 **/
BobguiCssSection *
bobgui_css_section_new_with_bytes (GFile  *file,
                                GBytes *bytes,
                                const BobguiCssLocation *start,
                                const BobguiCssLocation *end)
{
  BobguiCssSection *result;

  g_return_val_if_fail (file == NULL || G_IS_FILE (file), NULL);
  g_return_val_if_fail (start != NULL, NULL);
  g_return_val_if_fail (end != NULL, NULL);

  result = g_new0 (BobguiCssSection, 1);

  result->ref_count = 1;
  if (file)
    result->file = g_object_ref (file);
  if (bytes)
    result->bytes = g_bytes_ref (bytes);
  result->start_location = *start;
  result->end_location = *end;

  return result;
}

/**
 * bobgui_css_section_ref:
 * @section: a `BobguiCssSection`
 *
 * Increments the reference count on `section`.
 *
 * Returns: (transfer full): the CSS section itself.
 **/
BobguiCssSection *
bobgui_css_section_ref (BobguiCssSection *section)
{
  g_return_val_if_fail (section != NULL, NULL);

  section->ref_count += 1;

  return section;
}

/**
 * bobgui_css_section_unref:
 * @section: (transfer full): a `BobguiCssSection`
 *
 * Decrements the reference count on `section`, freeing the
 * structure if the reference count reaches 0.
 **/
void
bobgui_css_section_unref (BobguiCssSection *section)
{
  g_return_if_fail (section != NULL);

  section->ref_count -= 1;
  if (section->ref_count > 0)
    return;

  if (section->parent)
    bobgui_css_section_unref (section->parent);
  if (section->file)
    g_object_unref (section->file);
  if (section->bytes)
    g_bytes_unref (section->bytes);

  g_free (section);
}

/**
 * bobgui_css_section_get_parent:
 * @section: the section
 *
 * Gets the parent section for the given `section`.
 *
 * The parent section is the section that contains this `section`. A special
 * case are sections of  type `BOBGUI_CSS_SECTION_DOCUMENT`. Their parent will
 * either be `NULL` if they are the original CSS document that was loaded by
 * [method@Bobgui.CssProvider.load_from_file] or a section of type
 * `BOBGUI_CSS_SECTION_IMPORT` if it was loaded with an `@import` rule from
 * a different file.
 *
 * Returns: (nullable) (transfer none): the parent section
 **/
BobguiCssSection *
bobgui_css_section_get_parent (const BobguiCssSection *section)
{
  g_return_val_if_fail (section != NULL, NULL);

  return section->parent;
}

/**
 * bobgui_css_section_get_file:
 * @section: the section
 *
 * Gets the file that @section was parsed from.
 *
 * If no such file exists, for example because the CSS was loaded via
 * [method@Bobgui.CssProvider.load_from_data], then `NULL` is returned.
 *
 * Returns: (transfer none) (nullable): the `GFile` from which the `section`
 *   was parsed
 **/
GFile *
bobgui_css_section_get_file (const BobguiCssSection *section)
{
  g_return_val_if_fail (section != NULL, NULL);

  return section->file;
}

/**
 * bobgui_css_section_get_bytes:
 * @section: the section
 *
 * Gets the bytes that @section was parsed from.
 *
 * Returns: (transfer none) (nullable): the `GBytes` from which the `section`
 *   was parsed
 *
 * Since: 4.16
 **/
GBytes *
bobgui_css_section_get_bytes (const BobguiCssSection *section)
{
  g_return_val_if_fail (section != NULL, NULL);

  return section->bytes;
}

/**
 * bobgui_css_section_get_start_location:
 * @section: the section
 *
 * Returns the location in the CSS document where this section starts.
 *
 * Returns: (transfer none) (not nullable): The start location of
 *   this section
 */
const BobguiCssLocation *
bobgui_css_section_get_start_location (const BobguiCssSection *section)
{
  g_return_val_if_fail (section != NULL, NULL);

  return &section->start_location;
}

/**
 * bobgui_css_section_get_end_location:
 * @section: the section
 *
 * Returns the location in the CSS document where this section ends.
 *
 * Returns: (transfer none) (not nullable): The end location of
 *   this section
 */
const BobguiCssLocation *
bobgui_css_section_get_end_location (const BobguiCssSection *section)
{
  g_return_val_if_fail (section != NULL, NULL);

  return &section->end_location;
}

/**
 * bobgui_css_section_print:
 * @section: a section
 * @string: a `GString` to print to
 *
 * Prints the `section` into `string` in a human-readable form.
 *
 * This is a form like `bobgui.css:32:1-23` to denote line 32, characters
 * 1 to 23 in the file `bobgui.css`.
 **/
void
bobgui_css_section_print (const BobguiCssSection  *section,
                       GString              *string)
{
  if (section->file)
    {
      GFileInfo *info;

      info = g_file_query_info (section->file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME, 0, NULL, NULL);

      if (info)
        {
          g_string_append (string, g_file_info_get_display_name (info));
          g_object_unref (info);
        }
      else
        {
          g_string_append (string, "<broken file>");
        }
    }
  else
    {
      g_string_append (string, "<data>");
    }

  g_string_append_printf (string, ":%zu:%zu",
                          section->start_location.lines + 1,
                          section->start_location.line_chars + 1);
  if (section->start_location.lines != section->end_location.lines ||
      section->start_location.line_chars != section->end_location.line_chars)
    {
      g_string_append (string, "-");
      if (section->start_location.lines != section->end_location.lines)
        g_string_append_printf (string, "%zu:", section->end_location.lines + 1);
      g_string_append_printf (string, "%zu", section->end_location.line_chars + 1);
    }
}

/**
 * bobgui_css_section_to_string:
 * @section: a `BobguiCssSection`
 *
 * Prints the section into a human-readable text form using
 * [method@Bobgui.CssSection.print].
 *
 * Returns: (transfer full): A new string.
 **/
char *
bobgui_css_section_to_string (const BobguiCssSection *section)
{
  GString *string;

  g_return_val_if_fail (section != NULL, NULL);

  string = g_string_new (NULL);
  bobgui_css_section_print (section, string);

  return g_string_free (string, FALSE);
}
