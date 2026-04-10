/* BOBGUI - The Bobgui Framework
 * bobguiprintsettings.c: Print Settings
 * Copyright (C) 2006, Red Hat, Inc.
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

#include <string.h>
#include <stdlib.h>

#include <glib/gprintf.h>

#include <bobgui/bobgui.h>

#include "bobguiprintsettings.h"
#include "bobguiprintutilsprivate.h"


/**
 * BobguiPrintSettings:
 *
 * Collects the settings of a print dialog in a system-independent way.
 *
 * The main use for this object is that once you’ve printed you can get a
 * settings object that represents the settings the user chose, and the next
 * time you print you can pass that object in so that the user doesn’t have
 * to re-set all his settings.
 *
 * Its also possible to enumerate the settings so that you can easily save
 * the settings for the next time your app runs, or even store them in a
 * document. The predefined keys try to use shared values as much as possible
 * so that moving such a document between systems still works.
 */

typedef struct _BobguiPrintSettingsClass BobguiPrintSettingsClass;

#define BOBGUI_IS_PRINT_SETTINGS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_PRINT_SETTINGS))
#define BOBGUI_PRINT_SETTINGS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_PRINT_SETTINGS, BobguiPrintSettingsClass))
#define BOBGUI_PRINT_SETTINGS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_PRINT_SETTINGS, BobguiPrintSettingsClass))

struct _BobguiPrintSettings
{
  GObject parent_instance;

  GHashTable *hash;
};

struct _BobguiPrintSettingsClass
{
  GObjectClass parent_class;
};

#define KEYFILE_GROUP_NAME "Print Settings"

G_DEFINE_TYPE (BobguiPrintSettings, bobgui_print_settings, G_TYPE_OBJECT)

static void
bobgui_print_settings_finalize (GObject *object)
{
  BobguiPrintSettings *settings = BOBGUI_PRINT_SETTINGS (object);

  g_hash_table_destroy (settings->hash);

  G_OBJECT_CLASS (bobgui_print_settings_parent_class)->finalize (object);
}

static void
bobgui_print_settings_init (BobguiPrintSettings *settings)
{
  settings->hash = g_hash_table_new_full (g_str_hash, g_str_equal,
					  g_free, g_free);
}

static void
bobgui_print_settings_class_init (BobguiPrintSettingsClass *class)
{
  GObjectClass *gobject_class = (GObjectClass *)class;

  gobject_class->finalize = bobgui_print_settings_finalize;
}

/**
 * bobgui_print_settings_new:
 *
 * Creates a new `BobguiPrintSettings` object.
 *
 * Returns: a new `BobguiPrintSettings` object
 */
BobguiPrintSettings *
bobgui_print_settings_new (void)
{
  return g_object_new (BOBGUI_TYPE_PRINT_SETTINGS, NULL);
}

static void
copy_hash_entry  (gpointer  key,
		  gpointer  value,
		  gpointer  user_data)
{
  BobguiPrintSettings *settings = user_data;

  g_hash_table_insert (settings->hash,
		       g_strdup (key),
		       g_strdup (value));
}



/**
 * bobgui_print_settings_copy:
 * @other: a `BobguiPrintSettings`
 *
 * Copies a `BobguiPrintSettings` object.
 *
 * Returns: (transfer full): a newly allocated copy of @other
 */
BobguiPrintSettings *
bobgui_print_settings_copy (BobguiPrintSettings *other)
{
  BobguiPrintSettings *settings;

  if (other == NULL)
    return NULL;

  g_return_val_if_fail (BOBGUI_IS_PRINT_SETTINGS (other), NULL);

  settings = bobgui_print_settings_new ();

  g_hash_table_foreach (other->hash,
			copy_hash_entry,
			settings);

  return settings;
}

/**
 * bobgui_print_settings_get:
 * @settings: a `BobguiPrintSettings`
 * @key: a key
 *
 * Looks up the string value associated with @key.
 *
 * Returns: (nullable): the string value for @key
 */
const char *
bobgui_print_settings_get (BobguiPrintSettings *settings,
			const char       *key)
{
  return g_hash_table_lookup (settings->hash, key);
}

/**
 * bobgui_print_settings_set:
 * @settings: a `BobguiPrintSettings`
 * @key: a key
 * @value: (nullable): a string value
 *
 * Associates @value with @key.
 */
void
bobgui_print_settings_set (BobguiPrintSettings *settings,
			const char       *key,
			const char       *value)
{
  if (value == NULL)
    bobgui_print_settings_unset (settings, key);
  else
    g_hash_table_insert (settings->hash,
			 g_strdup (key),
			 g_strdup (value));
}

/**
 * bobgui_print_settings_unset:
 * @settings: a `BobguiPrintSettings`
 * @key: a key
 *
 * Removes any value associated with @key.
 *
 * This has the same effect as setting the value to %NULL.
 */
void
bobgui_print_settings_unset (BobguiPrintSettings *settings,
			  const char       *key)
{
  g_hash_table_remove (settings->hash, key);
}

/**
 * bobgui_print_settings_has_key:
 * @settings: a `BobguiPrintSettings`
 * @key: a key
 *
 * Returns %TRUE, if a value is associated with @key.
 *
 * Returns: %TRUE, if @key has a value
 */
gboolean
bobgui_print_settings_has_key (BobguiPrintSettings *settings,
			    const char       *key)
{
  return bobgui_print_settings_get (settings, key) != NULL;
}


/**
 * bobgui_print_settings_get_bool:
 * @settings: a `BobguiPrintSettings`
 * @key: a key
 *
 * Returns the boolean represented by the value
 * that is associated with @key.
 *
 * The string “true” represents %TRUE, any other
 * string %FALSE.
 *
 * Returns: %TRUE, if @key maps to a true value.
 */
gboolean
bobgui_print_settings_get_bool (BobguiPrintSettings *settings,
			     const char       *key)
{
  const char *val;

  val = bobgui_print_settings_get (settings, key);
  if (g_strcmp0 (val, "true") == 0)
    return TRUE;

  return FALSE;
}

/**
 * bobgui_print_settings_get_bool_with_default:
 * @settings: a `BobguiPrintSettings`
 * @key: a key
 * @default_val: the default value
 *
 * Returns the boolean represented by the value
 * that is associated with @key, or @default_val
 * if the value does not represent a boolean.
 *
 * The string “true” represents %TRUE, the string
 * “false” represents %FALSE.
 *
 * Returns: the boolean value associated with @key
 */
static gboolean
bobgui_print_settings_get_bool_with_default (BobguiPrintSettings *settings,
					  const char       *key,
					  gboolean          default_val)
{
  const char *val;

  val = bobgui_print_settings_get (settings, key);
  if (g_strcmp0 (val, "true") == 0)
    return TRUE;

  if (g_strcmp0 (val, "false") == 0)
    return FALSE;

  return default_val;
}

/**
 * bobgui_print_settings_set_bool:
 * @settings: a `BobguiPrintSettings`
 * @key: a key
 * @value: a boolean
 *
 * Sets @key to a boolean value.
 */
void
bobgui_print_settings_set_bool (BobguiPrintSettings *settings,
			     const char       *key,
			     gboolean          value)
{
  if (value)
    bobgui_print_settings_set (settings, key, "true");
  else
    bobgui_print_settings_set (settings, key, "false");
}

/**
 * bobgui_print_settings_get_double_with_default:
 * @settings: a `BobguiPrintSettings`
 * @key: a key
 * @def: the default value
 *
 * Returns the floating point number represented by
 * the value that is associated with @key, or @default_val
 * if the value does not represent a floating point number.
 *
 * Floating point numbers are parsed with g_ascii_strtod().
 *
 * Returns: the floating point number associated with @key
 */
double
bobgui_print_settings_get_double_with_default (BobguiPrintSettings *settings,
					    const char       *key,
					    double            def)
{
  const char *val;

  val = bobgui_print_settings_get (settings, key);
  if (val == NULL)
    return def;

  return g_ascii_strtod (val, NULL);
}

/**
 * bobgui_print_settings_get_double:
 * @settings: a `BobguiPrintSettings`
 * @key: a key
 *
 * Returns the double value associated with @key, or 0.
 *
 * Returns: the double value of @key
 */
double
bobgui_print_settings_get_double (BobguiPrintSettings *settings,
			       const char       *key)
{
  return bobgui_print_settings_get_double_with_default (settings, key, 0.0);
}

/**
 * bobgui_print_settings_set_double:
 * @settings: a `BobguiPrintSettings`
 * @key: a key
 * @value: a double value
 *
 * Sets @key to a double value.
 */
void
bobgui_print_settings_set_double (BobguiPrintSettings *settings,
			       const char       *key,
			       double            value)
{
  char buf[G_ASCII_DTOSTR_BUF_SIZE];

  g_ascii_dtostr (buf, G_ASCII_DTOSTR_BUF_SIZE, value);
  bobgui_print_settings_set (settings, key, buf);
}

/**
 * bobgui_print_settings_get_length:
 * @settings: a `BobguiPrintSettings`
 * @key: a key
 * @unit: the unit of the return value
 *
 * Returns the value associated with @key, interpreted
 * as a length.
 *
 * The returned value is converted to @units.
 *
 * Returns: the length value of @key, converted to @unit
 */
double
bobgui_print_settings_get_length (BobguiPrintSettings *settings,
			       const char       *key,
			       BobguiUnit           unit)
{
  double length = bobgui_print_settings_get_double (settings, key);
  return _bobgui_print_convert_from_mm (length, unit);
}

/**
 * bobgui_print_settings_set_length:
 * @settings: a `BobguiPrintSettings`
 * @key: a key
 * @value: a length
 * @unit: the unit of @length
 *
 * Associates a length in units of @unit with @key.
 */
void
bobgui_print_settings_set_length (BobguiPrintSettings *settings,
			       const char       *key,
			       double            value,
			       BobguiUnit           unit)
{
  bobgui_print_settings_set_double (settings, key,
				 _bobgui_print_convert_to_mm (value, unit));
}

/**
 * bobgui_print_settings_get_int_with_default:
 * @settings: a `BobguiPrintSettings`
 * @key: a key
 * @def: the default value
 *
 * Returns the value of @key, interpreted as
 * an integer, or the default value.
 *
 * Returns: the integer value of @key
 */
int
bobgui_print_settings_get_int_with_default (BobguiPrintSettings *settings,
					 const char       *key,
					 int               def)
{
  const char *val;

  val = bobgui_print_settings_get (settings, key);
  if (val == NULL)
    return def;

  return atoi (val);
}

/**
 * bobgui_print_settings_get_int:
 * @settings: a `BobguiPrintSettings`
 * @key: a key
 *
 * Returns the integer value of @key, or 0.
 *
 * Returns: the integer value of @key
 */
int
bobgui_print_settings_get_int (BobguiPrintSettings *settings,
			    const char       *key)
{
  return bobgui_print_settings_get_int_with_default (settings, key, 0);
}

/**
 * bobgui_print_settings_set_int:
 * @settings: a `BobguiPrintSettings`
 * @key: a key
 * @value: an integer
 *
 * Sets @key to an integer value.
 */
void
bobgui_print_settings_set_int (BobguiPrintSettings *settings,
			    const char       *key,
			    int               value)
{
  char buf[128];
  g_sprintf (buf, "%d", value);
  bobgui_print_settings_set (settings, key, buf);
}

/**
 * bobgui_print_settings_foreach:
 * @settings: a `BobguiPrintSettings`
 * @func: (scope call) (closure user_data): the function to call
 * @user_data: user data for @func
 *
 * Calls @func for each key-value pair of @settings.
 */
void
bobgui_print_settings_foreach (BobguiPrintSettings    *settings,
			    BobguiPrintSettingsFunc func,
			    gpointer             user_data)
{
  g_hash_table_foreach (settings->hash, (GHFunc)func, user_data);
}

/**
 * bobgui_print_settings_get_printer:
 * @settings: a `BobguiPrintSettings`
 *
 * Convenience function to obtain the value of
 * %BOBGUI_PRINT_SETTINGS_PRINTER.
 *
 * Returns: (nullable): the printer name
 */
const char *
bobgui_print_settings_get_printer (BobguiPrintSettings *settings)
{
  return bobgui_print_settings_get (settings, BOBGUI_PRINT_SETTINGS_PRINTER);
}


/**
 * bobgui_print_settings_set_printer:
 * @settings: a `BobguiPrintSettings`
 * @printer: the printer name
 *
 * Convenience function to set %BOBGUI_PRINT_SETTINGS_PRINTER
 * to @printer.
 */
void
bobgui_print_settings_set_printer (BobguiPrintSettings *settings,
				const char       *printer)
{
  bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_PRINTER, printer);
}

/**
 * bobgui_print_settings_get_orientation:
 * @settings: a `BobguiPrintSettings`
 *
 * Get the value of %BOBGUI_PRINT_SETTINGS_ORIENTATION,
 * converted to a `BobguiPageOrientation`.
 *
 * Returns: the orientation
 */
BobguiPageOrientation
bobgui_print_settings_get_orientation (BobguiPrintSettings *settings)
{
  const char *val;

  val = bobgui_print_settings_get (settings, BOBGUI_PRINT_SETTINGS_ORIENTATION);

  if (val == NULL || strcmp (val, "portrait") == 0)
    return BOBGUI_PAGE_ORIENTATION_PORTRAIT;

  if (strcmp (val, "landscape") == 0)
    return BOBGUI_PAGE_ORIENTATION_LANDSCAPE;

  if (strcmp (val, "reverse_portrait") == 0)
    return BOBGUI_PAGE_ORIENTATION_REVERSE_PORTRAIT;

  if (strcmp (val, "reverse_landscape") == 0)
    return BOBGUI_PAGE_ORIENTATION_REVERSE_LANDSCAPE;

  return BOBGUI_PAGE_ORIENTATION_PORTRAIT;
}

/**
 * bobgui_print_settings_set_orientation:
 * @settings: a `BobguiPrintSettings`
 * @orientation: a page orientation
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_ORIENTATION.
 */
void
bobgui_print_settings_set_orientation (BobguiPrintSettings   *settings,
				    BobguiPageOrientation  orientation)
{
  const char *val;

  switch (orientation)
    {
    case BOBGUI_PAGE_ORIENTATION_LANDSCAPE:
      val = "landscape";
      break;
    default:
    case BOBGUI_PAGE_ORIENTATION_PORTRAIT:
      val = "portrait";
      break;
    case BOBGUI_PAGE_ORIENTATION_REVERSE_LANDSCAPE:
      val = "reverse_landscape";
      break;
    case BOBGUI_PAGE_ORIENTATION_REVERSE_PORTRAIT:
      val = "reverse_portrait";
      break;
    }
  bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_ORIENTATION, val);
}

/**
 * bobgui_print_settings_get_paper_size:
 * @settings: a `BobguiPrintSettings`
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_PAPER_FORMAT,
 * converted to a `BobguiPaperSize`.
 *
 * Returns: (nullable): the paper size
 */
BobguiPaperSize *
bobgui_print_settings_get_paper_size (BobguiPrintSettings *settings)
{
  const char *val;
  const char *name;
  double w, h;

  val = bobgui_print_settings_get (settings, BOBGUI_PRINT_SETTINGS_PAPER_FORMAT);
  if (val == NULL)
    return NULL;

  if (g_str_has_prefix (val, "custom-"))
    {
      name = val + strlen ("custom-");
      w = bobgui_print_settings_get_paper_width (settings, BOBGUI_UNIT_MM);
      h = bobgui_print_settings_get_paper_height (settings, BOBGUI_UNIT_MM);
      return bobgui_paper_size_new_custom (name, name, w, h, BOBGUI_UNIT_MM);
    }

  return bobgui_paper_size_new (val);
}

/**
 * bobgui_print_settings_set_paper_size:
 * @settings: a `BobguiPrintSettings`
 * @paper_size: a paper size
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_PAPER_FORMAT,
 * %BOBGUI_PRINT_SETTINGS_PAPER_WIDTH and
 * %BOBGUI_PRINT_SETTINGS_PAPER_HEIGHT.
 */
void
bobgui_print_settings_set_paper_size (BobguiPrintSettings *settings,
				   BobguiPaperSize     *paper_size)
{
  char *custom_name;

  if (paper_size == NULL)
    {
      bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_PAPER_FORMAT, NULL);
      bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_PAPER_WIDTH, NULL);
      bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_PAPER_HEIGHT, NULL);
    }
  else if (bobgui_paper_size_is_custom (paper_size))
    {
      custom_name = g_strdup_printf ("custom-%s",
				     bobgui_paper_size_get_name (paper_size));
      bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_PAPER_FORMAT, custom_name);
      g_free (custom_name);
      bobgui_print_settings_set_paper_width (settings,
					  bobgui_paper_size_get_width (paper_size,
								    BOBGUI_UNIT_MM),
					  BOBGUI_UNIT_MM);
      bobgui_print_settings_set_paper_height (settings,
					   bobgui_paper_size_get_height (paper_size,
								      BOBGUI_UNIT_MM),
					   BOBGUI_UNIT_MM);
    }
  else
    bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_PAPER_FORMAT,
			    bobgui_paper_size_get_name (paper_size));
}

/**
 * bobgui_print_settings_get_paper_width:
 * @settings: a `BobguiPrintSettings`
 * @unit: the unit for the return value
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_PAPER_WIDTH,
 * converted to @unit.
 *
 * Returns: the paper width, in units of @unit
 */
double
bobgui_print_settings_get_paper_width (BobguiPrintSettings *settings,
				    BobguiUnit           unit)
{
  return bobgui_print_settings_get_length (settings, BOBGUI_PRINT_SETTINGS_PAPER_WIDTH, unit);
}

/**
 * bobgui_print_settings_set_paper_width:
 * @settings: a `BobguiPrintSettings`
 * @width: the paper width
 * @unit: the units of @width
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_PAPER_WIDTH.
 */
void
bobgui_print_settings_set_paper_width (BobguiPrintSettings *settings,
				    double            width,
				    BobguiUnit           unit)
{
  bobgui_print_settings_set_length (settings, BOBGUI_PRINT_SETTINGS_PAPER_WIDTH, width, unit);
}

/**
 * bobgui_print_settings_get_paper_height:
 * @settings: a `BobguiPrintSettings`
 * @unit: the unit for the return value
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_PAPER_HEIGHT,
 * converted to @unit.
 *
 * Returns: the paper height, in units of @unit
 */
double
bobgui_print_settings_get_paper_height (BobguiPrintSettings *settings,
				     BobguiUnit           unit)
{
  return bobgui_print_settings_get_length (settings,
					BOBGUI_PRINT_SETTINGS_PAPER_HEIGHT,
					unit);
}

/**
 * bobgui_print_settings_set_paper_height:
 * @settings: a `BobguiPrintSettings`
 * @height: the paper height
 * @unit: the units of @height
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_PAPER_HEIGHT.
 */
void
bobgui_print_settings_set_paper_height (BobguiPrintSettings *settings,
				     double            height,
				     BobguiUnit           unit)
{
  bobgui_print_settings_set_length (settings,
				 BOBGUI_PRINT_SETTINGS_PAPER_HEIGHT,
				 height, unit);
}

/**
 * bobgui_print_settings_get_use_color:
 * @settings: a `BobguiPrintSettings`
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_USE_COLOR.
 *
 * Returns: whether to use color
 */
gboolean
bobgui_print_settings_get_use_color (BobguiPrintSettings *settings)
{
  return bobgui_print_settings_get_bool_with_default (settings,
						   BOBGUI_PRINT_SETTINGS_USE_COLOR,
						   TRUE);
}

/**
 * bobgui_print_settings_set_use_color:
 * @settings: a `BobguiPrintSettings`
 * @use_color: whether to use color
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_USE_COLOR.
 */
void
bobgui_print_settings_set_use_color (BobguiPrintSettings *settings,
				  gboolean          use_color)
{
  bobgui_print_settings_set_bool (settings,
			       BOBGUI_PRINT_SETTINGS_USE_COLOR,
			       use_color);
}

/**
 * bobgui_print_settings_get_collate:
 * @settings: a `BobguiPrintSettings`
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_COLLATE.
 *
 * Returns: whether to collate the printed pages
 */
gboolean
bobgui_print_settings_get_collate (BobguiPrintSettings *settings)
{
  return bobgui_print_settings_get_bool_with_default (settings,
                                                   BOBGUI_PRINT_SETTINGS_COLLATE,
                                                   TRUE);
}

/**
 * bobgui_print_settings_set_collate:
 * @settings: a `BobguiPrintSettings`
 * @collate: whether to collate the output
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_COLLATE.
 */
void
bobgui_print_settings_set_collate (BobguiPrintSettings *settings,
				gboolean          collate)
{
  bobgui_print_settings_set_bool (settings,
			       BOBGUI_PRINT_SETTINGS_COLLATE,
			       collate);
}

/**
 * bobgui_print_settings_get_reverse:
 * @settings: a `BobguiPrintSettings`
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_REVERSE.
 *
 * Returns: whether to reverse the order of the printed pages
 */
gboolean
bobgui_print_settings_get_reverse (BobguiPrintSettings *settings)
{
  return bobgui_print_settings_get_bool (settings,
				      BOBGUI_PRINT_SETTINGS_REVERSE);
}

/**
 * bobgui_print_settings_set_reverse:
 * @settings: a `BobguiPrintSettings`
 * @reverse: whether to reverse the output
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_REVERSE.
 */
void
bobgui_print_settings_set_reverse (BobguiPrintSettings *settings,
				  gboolean        reverse)
{
  bobgui_print_settings_set_bool (settings,
			       BOBGUI_PRINT_SETTINGS_REVERSE,
			       reverse);
}

/**
 * bobgui_print_settings_get_duplex:
 * @settings: a `BobguiPrintSettings`
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_DUPLEX.
 *
 * Returns: whether to print the output in duplex.
 */
BobguiPrintDuplex
bobgui_print_settings_get_duplex (BobguiPrintSettings *settings)
{
  const char *val;

  val = bobgui_print_settings_get (settings, BOBGUI_PRINT_SETTINGS_DUPLEX);

  if (val == NULL || (strcmp (val, "simplex") == 0))
    return BOBGUI_PRINT_DUPLEX_SIMPLEX;

  if (strcmp (val, "horizontal") == 0)
    return BOBGUI_PRINT_DUPLEX_HORIZONTAL;

  if (strcmp (val, "vertical") == 0)
    return BOBGUI_PRINT_DUPLEX_VERTICAL;

  return BOBGUI_PRINT_DUPLEX_SIMPLEX;
}

/**
 * bobgui_print_settings_set_duplex:
 * @settings: a `BobguiPrintSettings`
 * @duplex: a `BobguiPrintDuplex` value
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_DUPLEX.
 */
void
bobgui_print_settings_set_duplex (BobguiPrintSettings *settings,
			       BobguiPrintDuplex    duplex)
{
  const char *str;

  switch (duplex)
    {
    default:
    case BOBGUI_PRINT_DUPLEX_SIMPLEX:
      str = "simplex";
      break;
    case BOBGUI_PRINT_DUPLEX_HORIZONTAL:
      str = "horizontal";
      break;
    case BOBGUI_PRINT_DUPLEX_VERTICAL:
      str = "vertical";
      break;
    }

  bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_DUPLEX, str);
}

/**
 * bobgui_print_settings_get_quality:
 * @settings: a `BobguiPrintSettings`
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_QUALITY.
 *
 * Returns: the print quality
 */
BobguiPrintQuality
bobgui_print_settings_get_quality (BobguiPrintSettings *settings)
{
  const char *val;

  val = bobgui_print_settings_get (settings, BOBGUI_PRINT_SETTINGS_QUALITY);

  if (val == NULL || (strcmp (val, "normal") == 0))
    return BOBGUI_PRINT_QUALITY_NORMAL;

  if (strcmp (val, "high") == 0)
    return BOBGUI_PRINT_QUALITY_HIGH;

  if (strcmp (val, "low") == 0)
    return BOBGUI_PRINT_QUALITY_LOW;

  if (strcmp (val, "draft") == 0)
    return BOBGUI_PRINT_QUALITY_DRAFT;

  return BOBGUI_PRINT_QUALITY_NORMAL;
}

/**
 * bobgui_print_settings_set_quality:
 * @settings: a `BobguiPrintSettings`
 * @quality: a `BobguiPrintQuality` value
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_QUALITY.
 */
void
bobgui_print_settings_set_quality (BobguiPrintSettings *settings,
				BobguiPrintQuality   quality)
{
  const char *str;

  switch (quality)
    {
    default:
    case BOBGUI_PRINT_QUALITY_NORMAL:
      str = "normal";
      break;
    case BOBGUI_PRINT_QUALITY_HIGH:
      str = "high";
      break;
    case BOBGUI_PRINT_QUALITY_LOW:
      str = "low";
      break;
    case BOBGUI_PRINT_QUALITY_DRAFT:
      str = "draft";
      break;
    }

  bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_QUALITY, str);
}

/**
 * bobgui_print_settings_get_page_set:
 * @settings: a `BobguiPrintSettings`
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_PAGE_SET.
 *
 * Returns: the set of pages to print
 */
BobguiPageSet
bobgui_print_settings_get_page_set (BobguiPrintSettings *settings)
{
  const char *val;

  val = bobgui_print_settings_get (settings, BOBGUI_PRINT_SETTINGS_PAGE_SET);

  if (val == NULL || (strcmp (val, "all") == 0))
    return BOBGUI_PAGE_SET_ALL;

  if (strcmp (val, "even") == 0)
    return BOBGUI_PAGE_SET_EVEN;

  if (strcmp (val, "odd") == 0)
    return BOBGUI_PAGE_SET_ODD;

  return BOBGUI_PAGE_SET_ALL;
}

/**
 * bobgui_print_settings_set_page_set:
 * @settings: a `BobguiPrintSettings`
 * @page_set: a `BobguiPageSet` value
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_PAGE_SET.
 */
void
bobgui_print_settings_set_page_set (BobguiPrintSettings *settings,
				 BobguiPageSet        page_set)
{
  const char *str;

  switch (page_set)
    {
    default:
    case BOBGUI_PAGE_SET_ALL:
      str = "all";
      break;
    case BOBGUI_PAGE_SET_EVEN:
      str = "even";
      break;
    case BOBGUI_PAGE_SET_ODD:
      str = "odd";
      break;
    }

  bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_PAGE_SET, str);
}

/**
 * bobgui_print_settings_get_number_up_layout:
 * @settings: a `BobguiPrintSettings`
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_NUMBER_UP_LAYOUT.
 *
 * Returns: layout of page in number-up mode
 */
BobguiNumberUpLayout
bobgui_print_settings_get_number_up_layout (BobguiPrintSettings *settings)
{
  BobguiNumberUpLayout layout;
  BobguiTextDirection  text_direction;
  GEnumClass       *enum_class;
  GEnumValue       *enum_value;
  const char       *val;

  g_return_val_if_fail (BOBGUI_IS_PRINT_SETTINGS (settings), BOBGUI_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM);

  val = bobgui_print_settings_get (settings, BOBGUI_PRINT_SETTINGS_NUMBER_UP_LAYOUT);
  text_direction = bobgui_widget_get_default_direction ();

  if (text_direction == BOBGUI_TEXT_DIR_LTR)
    layout = BOBGUI_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM;
  else
    layout = BOBGUI_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_TOP_TO_BOTTOM;

  if (val == NULL)
    return layout;

  enum_class = g_type_class_ref (BOBGUI_TYPE_NUMBER_UP_LAYOUT);
  enum_value = g_enum_get_value_by_nick (enum_class, val);
  if (enum_value)
    layout = enum_value->value;
  g_type_class_unref (enum_class);

  return layout;
}

/**
 * bobgui_print_settings_set_number_up_layout:
 * @settings: a `BobguiPrintSettings`
 * @number_up_layout: a `BobguiNumberUpLayout` value
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_NUMBER_UP_LAYOUT.
 */
void
bobgui_print_settings_set_number_up_layout (BobguiPrintSettings  *settings,
					 BobguiNumberUpLayout  number_up_layout)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  g_return_if_fail (BOBGUI_IS_PRINT_SETTINGS (settings));

  enum_class = g_type_class_ref (BOBGUI_TYPE_NUMBER_UP_LAYOUT);
  enum_value = g_enum_get_value (enum_class, number_up_layout);
  g_return_if_fail (enum_value != NULL);

  bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_NUMBER_UP_LAYOUT, enum_value->value_nick);
  g_type_class_unref (enum_class);
}

/**
 * bobgui_print_settings_get_n_copies:
 * @settings: a `BobguiPrintSettings`
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_N_COPIES.
 *
 * Returns: the number of copies to print
 */
int
bobgui_print_settings_get_n_copies (BobguiPrintSettings *settings)
{
  return bobgui_print_settings_get_int_with_default (settings, BOBGUI_PRINT_SETTINGS_N_COPIES, 1);
}

/**
 * bobgui_print_settings_set_n_copies:
 * @settings: a `BobguiPrintSettings`
 * @num_copies: the number of copies
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_N_COPIES.
 */
void
bobgui_print_settings_set_n_copies (BobguiPrintSettings *settings,
				 int               num_copies)
{
  bobgui_print_settings_set_int (settings, BOBGUI_PRINT_SETTINGS_N_COPIES,
			      num_copies);
}

/**
 * bobgui_print_settings_get_number_up:
 * @settings: a `BobguiPrintSettings`
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_NUMBER_UP.
 *
 * Returns: the number of pages per sheet
 */
int
bobgui_print_settings_get_number_up (BobguiPrintSettings *settings)
{
  return bobgui_print_settings_get_int_with_default (settings, BOBGUI_PRINT_SETTINGS_NUMBER_UP, 1);
}

/**
 * bobgui_print_settings_set_number_up:
 * @settings: a `BobguiPrintSettings`
 * @number_up: the number of pages per sheet
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_NUMBER_UP.
 */
void
bobgui_print_settings_set_number_up (BobguiPrintSettings *settings,
				  int               number_up)
{
  bobgui_print_settings_set_int (settings, BOBGUI_PRINT_SETTINGS_NUMBER_UP,
				number_up);
}

/**
 * bobgui_print_settings_get_resolution:
 * @settings: a `BobguiPrintSettings`
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_RESOLUTION.
 *
 * Returns: the resolution in dpi
 */
int
bobgui_print_settings_get_resolution (BobguiPrintSettings *settings)
{
  return bobgui_print_settings_get_int_with_default (settings, BOBGUI_PRINT_SETTINGS_RESOLUTION, 300);
}

/**
 * bobgui_print_settings_set_resolution:
 * @settings: a `BobguiPrintSettings`
 * @resolution: the resolution in dpi
 *
 * Sets the values of %BOBGUI_PRINT_SETTINGS_RESOLUTION,
 * %BOBGUI_PRINT_SETTINGS_RESOLUTION_X and
 * %BOBGUI_PRINT_SETTINGS_RESOLUTION_Y.
 */
void
bobgui_print_settings_set_resolution (BobguiPrintSettings *settings,
				   int               resolution)
{
  bobgui_print_settings_set_int (settings, BOBGUI_PRINT_SETTINGS_RESOLUTION,
			      resolution);
  bobgui_print_settings_set_int (settings, BOBGUI_PRINT_SETTINGS_RESOLUTION_X,
			      resolution);
  bobgui_print_settings_set_int (settings, BOBGUI_PRINT_SETTINGS_RESOLUTION_Y,
			      resolution);
}

/**
 * bobgui_print_settings_get_resolution_x:
 * @settings: a `BobguiPrintSettings`
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_RESOLUTION_X.
 *
 * Returns: the horizontal resolution in dpi
 */
int
bobgui_print_settings_get_resolution_x (BobguiPrintSettings *settings)
{
  return bobgui_print_settings_get_int_with_default (settings, BOBGUI_PRINT_SETTINGS_RESOLUTION_X, 300);
}

/**
 * bobgui_print_settings_get_resolution_y:
 * @settings: a `BobguiPrintSettings`
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_RESOLUTION_Y.
 *
 * Returns: the vertical resolution in dpi
 */
int
bobgui_print_settings_get_resolution_y (BobguiPrintSettings *settings)
{
  return bobgui_print_settings_get_int_with_default (settings, BOBGUI_PRINT_SETTINGS_RESOLUTION_Y, 300);
}

/**
 * bobgui_print_settings_set_resolution_xy:
 * @settings: a `BobguiPrintSettings`
 * @resolution_x: the horizontal resolution in dpi
 * @resolution_y: the vertical resolution in dpi
 *
 * Sets the values of %BOBGUI_PRINT_SETTINGS_RESOLUTION,
 * %BOBGUI_PRINT_SETTINGS_RESOLUTION_X and
 * %BOBGUI_PRINT_SETTINGS_RESOLUTION_Y.
 */
void
bobgui_print_settings_set_resolution_xy (BobguiPrintSettings *settings,
				      int               resolution_x,
				      int               resolution_y)
{
  bobgui_print_settings_set_int (settings, BOBGUI_PRINT_SETTINGS_RESOLUTION_X,
			      resolution_x);
  bobgui_print_settings_set_int (settings, BOBGUI_PRINT_SETTINGS_RESOLUTION_Y,
			      resolution_y);
  bobgui_print_settings_set_int (settings, BOBGUI_PRINT_SETTINGS_RESOLUTION,
			      resolution_x);
}

/**
 * bobgui_print_settings_get_printer_lpi:
 * @settings: a `BobguiPrintSettings`
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_PRINTER_LPI.
 *
 * Returns: the resolution in lpi (lines per inch)
 */
double
bobgui_print_settings_get_printer_lpi (BobguiPrintSettings *settings)
{
  return bobgui_print_settings_get_double_with_default (settings, BOBGUI_PRINT_SETTINGS_PRINTER_LPI, 150.0);
}

/**
 * bobgui_print_settings_set_printer_lpi:
 * @settings: a `BobguiPrintSettings`
 * @lpi: the resolution in lpi (lines per inch)
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_PRINTER_LPI.
 */
void
bobgui_print_settings_set_printer_lpi (BobguiPrintSettings *settings,
				    double            lpi)
{
  bobgui_print_settings_set_double (settings, BOBGUI_PRINT_SETTINGS_PRINTER_LPI,
			         lpi);
}

/**
 * bobgui_print_settings_get_scale:
 * @settings: a `BobguiPrintSettings`
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_SCALE.
 *
 * Returns: the scale in percent
 */
double
bobgui_print_settings_get_scale (BobguiPrintSettings *settings)
{
  return bobgui_print_settings_get_double_with_default (settings,
						     BOBGUI_PRINT_SETTINGS_SCALE,
						     100.0);
}

/**
 * bobgui_print_settings_set_scale:
 * @settings: a `BobguiPrintSettings`
 * @scale: the scale in percent
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_SCALE.
 */
void
bobgui_print_settings_set_scale (BobguiPrintSettings *settings,
			      double            scale)
{
  bobgui_print_settings_set_double (settings, BOBGUI_PRINT_SETTINGS_SCALE,
				 scale);
}

/**
 * bobgui_print_settings_get_print_pages:
 * @settings: a `BobguiPrintSettings`
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_PRINT_PAGES.
 *
 * Returns: which pages to print
 */
BobguiPrintPages
bobgui_print_settings_get_print_pages (BobguiPrintSettings *settings)
{
  const char *val;

  val = bobgui_print_settings_get (settings, BOBGUI_PRINT_SETTINGS_PRINT_PAGES);

  if (val == NULL || (strcmp (val, "all") == 0))
    return BOBGUI_PRINT_PAGES_ALL;

  if (strcmp (val, "selection") == 0)
    return BOBGUI_PRINT_PAGES_SELECTION;

  if (strcmp (val, "current") == 0)
    return BOBGUI_PRINT_PAGES_CURRENT;

  if (strcmp (val, "ranges") == 0)
    return BOBGUI_PRINT_PAGES_RANGES;

  return BOBGUI_PRINT_PAGES_ALL;
}

/**
 * bobgui_print_settings_set_print_pages:
 * @settings: a `BobguiPrintSettings`
 * @pages: a `BobguiPrintPages` value
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_PRINT_PAGES.
 */
void
bobgui_print_settings_set_print_pages (BobguiPrintSettings *settings,
				    BobguiPrintPages     pages)
{
  const char *str;

  switch (pages)
    {
    default:
    case BOBGUI_PRINT_PAGES_ALL:
      str = "all";
      break;
    case BOBGUI_PRINT_PAGES_CURRENT:
      str = "current";
      break;
    case BOBGUI_PRINT_PAGES_SELECTION:
      str = "selection";
      break;
    case BOBGUI_PRINT_PAGES_RANGES:
      str = "ranges";
      break;
    }

  bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_PRINT_PAGES, str);
}

/**
 * bobgui_print_settings_get_page_ranges:
 * @settings: a `BobguiPrintSettings`
 * @num_ranges: (out): return location for the length of the returned array
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_PAGE_RANGES.
 *
 * Returns: (array length=num_ranges) (transfer full): an array
 *   of `BobguiPageRange`s. Use g_free() to free the array when
 *   it is no longer needed.
 */
BobguiPageRange *
bobgui_print_settings_get_page_ranges (BobguiPrintSettings *settings,
				    int              *num_ranges)
{
  const char *val;
  char **range_strs;
  BobguiPageRange *ranges;
  int i, n;

  val = bobgui_print_settings_get (settings, BOBGUI_PRINT_SETTINGS_PAGE_RANGES);

  if (val == NULL)
    {
      *num_ranges = 0;
      return NULL;
    }

  range_strs = g_strsplit (val, ",", 0);

  for (i = 0; range_strs[i] != NULL; i++)
    ;

  n = i;

  ranges = g_new0 (BobguiPageRange, n);

  for (i = 0; i < n; i++)
    {
      int start, end;
      char *str;

      start = (int)strtol (range_strs[i], &str, 10);
      end = start;

      if (*str == '-')
	{
	  str++;
	  end = (int)strtol (str, NULL, 10);
	}

      ranges[i].start = start;
      ranges[i].end = end;
    }

  g_strfreev (range_strs);

  *num_ranges = n;
  return ranges;
}

/**
 * bobgui_print_settings_set_page_ranges:
 * @settings: a `BobguiPrintSettings`
 * @page_ranges: (array length=num_ranges): an array of `BobguiPageRange`s
 * @num_ranges: the length of @page_ranges
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_PAGE_RANGES.
 */
void
bobgui_print_settings_set_page_ranges  (BobguiPrintSettings *settings,
				     BobguiPageRange     *page_ranges,
				     int               num_ranges)
{
  GString *s;
  int i;

  s = g_string_new ("");

  for (i = 0; i < num_ranges; i++)
    {
      if (page_ranges[i].start == page_ranges[i].end)
	g_string_append_printf (s, "%d", page_ranges[i].start);
      else
	g_string_append_printf (s, "%d-%d",
				page_ranges[i].start,
				page_ranges[i].end);
      if (i < num_ranges - 1)
	g_string_append (s, ",");
    }


  bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_PAGE_RANGES,
			  s->str);

  g_string_free (s, TRUE);
}

/**
 * bobgui_print_settings_get_default_source:
 * @settings: a `BobguiPrintSettings`
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_DEFAULT_SOURCE.
 *
 * Returns: (nullable): the default source
 */
const char *
bobgui_print_settings_get_default_source (BobguiPrintSettings *settings)
{
  return bobgui_print_settings_get (settings, BOBGUI_PRINT_SETTINGS_DEFAULT_SOURCE);
}

/**
 * bobgui_print_settings_set_default_source:
 * @settings: a `BobguiPrintSettings`
 * @default_source: the default source
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_DEFAULT_SOURCE.
 */
void
bobgui_print_settings_set_default_source (BobguiPrintSettings *settings,
				       const char       *default_source)
{
  bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_DEFAULT_SOURCE, default_source);
}

/**
 * bobgui_print_settings_get_media_type:
 * @settings: a `BobguiPrintSettings`
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_MEDIA_TYPE.
 *
 * The set of media types is defined in PWG 5101.1-2002 PWG.
 *
 * Returns: (nullable): the media type
 */
const char *
bobgui_print_settings_get_media_type (BobguiPrintSettings *settings)
{
  return bobgui_print_settings_get (settings, BOBGUI_PRINT_SETTINGS_MEDIA_TYPE);
}

/**
 * bobgui_print_settings_set_media_type:
 * @settings: a `BobguiPrintSettings`
 * @media_type: the media type
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_MEDIA_TYPE.
 *
 * The set of media types is defined in PWG 5101.1-2002 PWG.
 */
void
bobgui_print_settings_set_media_type (BobguiPrintSettings *settings,
				   const char       *media_type)
{
  bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_MEDIA_TYPE, media_type);
}

/**
 * bobgui_print_settings_get_dither:
 * @settings: a `BobguiPrintSettings`
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_DITHER.
 *
 * Returns: (nullable): the dithering that is used
 */
const char *
bobgui_print_settings_get_dither (BobguiPrintSettings *settings)
{
  return bobgui_print_settings_get (settings, BOBGUI_PRINT_SETTINGS_DITHER);
}

/**
 * bobgui_print_settings_set_dither:
 * @settings: a `BobguiPrintSettings`
 * @dither: the dithering that is used
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_DITHER.
 */
void
bobgui_print_settings_set_dither (BobguiPrintSettings *settings,
			       const char       *dither)
{
  bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_DITHER, dither);
}

/**
 * bobgui_print_settings_get_finishings:
 * @settings: a `BobguiPrintSettings`
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_FINISHINGS.
 *
 * Returns: (nullable): the finishings
 */
const char *
bobgui_print_settings_get_finishings (BobguiPrintSettings *settings)
{
  return bobgui_print_settings_get (settings, BOBGUI_PRINT_SETTINGS_FINISHINGS);
}

/**
 * bobgui_print_settings_set_finishings:
 * @settings: a `BobguiPrintSettings`
 * @finishings: the finishings
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_FINISHINGS.
 */
void
bobgui_print_settings_set_finishings (BobguiPrintSettings *settings,
				   const char       *finishings)
{
  bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_FINISHINGS, finishings);
}

/**
 * bobgui_print_settings_get_output_bin:
 * @settings: a `BobguiPrintSettings`
 *
 * Gets the value of %BOBGUI_PRINT_SETTINGS_OUTPUT_BIN.
 *
 * Returns: (nullable): the output bin
 */
const char *
bobgui_print_settings_get_output_bin (BobguiPrintSettings *settings)
{
  return bobgui_print_settings_get (settings, BOBGUI_PRINT_SETTINGS_OUTPUT_BIN);
}

/**
 * bobgui_print_settings_set_output_bin:
 * @settings: a `BobguiPrintSettings`
 * @output_bin: the output bin
 *
 * Sets the value of %BOBGUI_PRINT_SETTINGS_OUTPUT_BIN.
 */
void
bobgui_print_settings_set_output_bin (BobguiPrintSettings *settings,
				   const char       *output_bin)
{
  bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_OUTPUT_BIN, output_bin);
}

/**
 * bobgui_print_settings_load_file:
 * @settings: a `BobguiPrintSettings`
 * @file_name: (type filename): the filename to read the settings from
 * @error: (nullable): return location for errors
 *
 * Reads the print settings from @file_name.
 *
 * If the file could not be loaded then error is set to either
 * a `GFileError` or `GKeyFileError`.
 *
 * See [method@Bobgui.PrintSettings.to_file].
 *
 * Returns: %TRUE on success
 */
gboolean
bobgui_print_settings_load_file (BobguiPrintSettings *settings,
                              const char       *file_name,
                              GError          **error)
{
  gboolean retval = FALSE;
  GKeyFile *key_file;

  g_return_val_if_fail (BOBGUI_IS_PRINT_SETTINGS (settings), FALSE);
  g_return_val_if_fail (file_name != NULL, FALSE);

  key_file = g_key_file_new ();

  if (g_key_file_load_from_file (key_file, file_name, 0, error) &&
      bobgui_print_settings_load_key_file (settings, key_file, NULL, error))
    retval = TRUE;

  g_key_file_free (key_file);

  return retval;
}

/**
 * bobgui_print_settings_new_from_file:
 * @file_name: (type filename): the filename to read the settings from
 * @error: (nullable): return location for errors
 *
 * Reads the print settings from @file_name.
 *
 * Returns a new `BobguiPrintSettings` object with the restored settings,
 * or %NULL if an error occurred. If the file could not be loaded then
 * error is set to either a `GFileError` or `GKeyFileError`.
 *
 * See [method@Bobgui.PrintSettings.to_file].
 *
 * Returns: the restored `BobguiPrintSettings`
 */
BobguiPrintSettings *
bobgui_print_settings_new_from_file (const char   *file_name,
			          GError      **error)
{
  BobguiPrintSettings *settings = bobgui_print_settings_new ();

  if (!bobgui_print_settings_load_file (settings, file_name, error))
    {
      g_object_unref (settings);
      settings = NULL;
    }

  return settings;
}

/**
 * bobgui_print_settings_load_key_file:
 * @settings: a `BobguiPrintSettings`
 * @key_file: the `GKeyFile` to retrieve the settings from
 * @group_name: (nullable): the name of the group to use, or %NULL
 *   to use the default “Print Settings”
 * @error: (nullable): return location for errors
 *
 * Reads the print settings from the group @group_name in @key_file.
 *
 * If the file could not be loaded then error is set to either a
 * `GFileError` or `GKeyFileError`.
 *
 * Returns: %TRUE on success
 */
gboolean
bobgui_print_settings_load_key_file (BobguiPrintSettings *settings,
				  GKeyFile         *key_file,
				  const char       *group_name,
				  GError          **error)
{
  char **keys;
  gsize n_keys, i;
  GError *err = NULL;

  g_return_val_if_fail (BOBGUI_IS_PRINT_SETTINGS (settings), FALSE);
  g_return_val_if_fail (key_file != NULL, FALSE);

  if (!group_name)
    group_name = KEYFILE_GROUP_NAME;

  keys = g_key_file_get_keys (key_file,
			      group_name,
			      &n_keys,
			      &err);
  if (err != NULL)
    {
      g_propagate_error (error, err);
      return FALSE;
    }

  for (i = 0 ; i < n_keys; ++i)
    {
      char *value;

      value = g_key_file_get_string (key_file,
				     group_name,
				     keys[i],
				     NULL);
      if (!value)
        continue;

      bobgui_print_settings_set (settings, keys[i], value);
      g_free (value);
    }

  g_strfreev (keys);

  return TRUE;
}

/**
 * bobgui_print_settings_new_from_key_file:
 * @key_file: the `GKeyFile` to retrieve the settings from
 * @group_name: (nullable): the name of the group to use, or %NULL to use
 *   the default “Print Settings”
 * @error: (nullable): return location for errors
 *
 * Reads the print settings from the group @group_name in @key_file.
 *
 * Returns a new `BobguiPrintSettings` object with the restored settings,
 * or %NULL if an error occurred. If the file could not be loaded then
 * error is set to either `GFileError` or `GKeyFileError`.
 *
 * Returns: the restored `BobguiPrintSettings`
 */
BobguiPrintSettings *
bobgui_print_settings_new_from_key_file (GKeyFile     *key_file,
				      const char   *group_name,
				      GError      **error)
{
  BobguiPrintSettings *settings = bobgui_print_settings_new ();

  if (!bobgui_print_settings_load_key_file (settings, key_file,
                                         group_name, error))
    {
      g_object_unref (settings);
      settings = NULL;
    }

  return settings;
}

/**
 * bobgui_print_settings_to_file:
 * @settings: a `BobguiPrintSettings`
 * @file_name: (type filename): the file to save to
 * @error: (nullable): return location for errors
 *
 * This function saves the print settings from @settings to @file_name.
 *
 * If the file could not be written then error is set to either a
 * `GFileError` or `GKeyFileError`.
 *
 * Returns: %TRUE on success
 */
gboolean
bobgui_print_settings_to_file (BobguiPrintSettings  *settings,
			    const char        *file_name,
			    GError           **error)
{
  GKeyFile *key_file;
  gboolean retval = FALSE;
  char *data = NULL;
  gsize len;
  GError *err = NULL;

  g_return_val_if_fail (BOBGUI_IS_PRINT_SETTINGS (settings), FALSE);
  g_return_val_if_fail (file_name != NULL, FALSE);

  key_file = g_key_file_new ();
  bobgui_print_settings_to_key_file (settings, key_file, NULL);

  data = g_key_file_to_data (key_file, &len, &err);
  if (!data)
    goto out;

  retval = g_file_set_contents (file_name, data, len, &err);

out:
  if (err != NULL)
    g_propagate_error (error, err);

  g_key_file_free (key_file);
  g_free (data);

  return retval;
}

typedef struct {
  GKeyFile *key_file;
  const char *group_name;
} SettingsData;

static void
add_value_to_key_file (const char   *key,
		       const char   *value,
		       SettingsData *data)
{
  g_key_file_set_string (data->key_file, data->group_name, key, value);
}

/**
 * bobgui_print_settings_to_key_file:
 * @settings: a `BobguiPrintSettings`
 * @key_file: the `GKeyFile` to save the print settings to
 * @group_name: (nullable): the group to add the settings to in @key_file, or
 *   %NULL to use the default “Print Settings”
 *
 * This function adds the print settings from @settings to @key_file.
 */
void
bobgui_print_settings_to_key_file (BobguiPrintSettings  *settings,
			        GKeyFile          *key_file,
				const char        *group_name)
{
  SettingsData data;

  g_return_if_fail (BOBGUI_IS_PRINT_SETTINGS (settings));
  g_return_if_fail (key_file != NULL);

  if (!group_name)
    group_name = KEYFILE_GROUP_NAME;

  data.key_file = key_file;
  data.group_name = group_name;

  bobgui_print_settings_foreach (settings,
			      (BobguiPrintSettingsFunc) add_value_to_key_file,
			      &data);
}

static void
add_to_variant (const char *key,
                const char *value,
                gpointer     data)
{
  GVariantBuilder *builder = data;
  g_variant_builder_add (builder, "{sv}", key, g_variant_new_string (value));
}

/**
 * bobgui_print_settings_to_gvariant:
 * @settings: a `BobguiPrintSettings`
 *
 * Serialize print settings to an a{sv} variant.
 *
 * Returns: (transfer none): a new, floating, `GVariant`
 */
GVariant *
bobgui_print_settings_to_gvariant (BobguiPrintSettings *settings)
{
  GVariantBuilder builder;

  g_variant_builder_init (&builder, G_VARIANT_TYPE_VARDICT);
  bobgui_print_settings_foreach (settings, add_to_variant, &builder);

  return g_variant_builder_end (&builder);
}

/**
 * bobgui_print_settings_new_from_gvariant:
 * @variant: an a{sv} `GVariant`
 *
 * Deserialize print settings from an a{sv} variant.
 *
 * The variant must be in the format produced by
 * [method@Bobgui.PrintSettings.to_gvariant].
 *
 * Returns: (transfer full): a new `BobguiPrintSettings` object
 */
BobguiPrintSettings *
bobgui_print_settings_new_from_gvariant (GVariant *variant)
{
  BobguiPrintSettings *settings;
  int i;

  g_return_val_if_fail (g_variant_is_of_type (variant, G_VARIANT_TYPE_VARDICT), NULL);

  settings = bobgui_print_settings_new ();

  for (i = 0; i < g_variant_n_children (variant); i++)
    {
      const char *key;
      GVariant *v;

      g_variant_get_child (variant, i, "{&sv}", &key, &v);
      if (g_variant_is_of_type (v, G_VARIANT_TYPE_STRING))
        bobgui_print_settings_set (settings, key, g_variant_get_string (v, NULL));
      g_variant_unref (v);
    }

  return settings;
}
