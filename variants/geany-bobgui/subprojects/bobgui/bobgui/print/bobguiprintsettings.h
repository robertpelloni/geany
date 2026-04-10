/* BOBGUI - The Bobgui Framework
 * bobguiprintsettings.h: Print Settings
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

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/print/bobguipapersize.h>

G_BEGIN_DECLS

typedef struct _BobguiPrintSettings BobguiPrintSettings;

#define BOBGUI_TYPE_PRINT_SETTINGS    (bobgui_print_settings_get_type ())
#define BOBGUI_PRINT_SETTINGS(obj)    (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PRINT_SETTINGS, BobguiPrintSettings))
#define BOBGUI_IS_PRINT_SETTINGS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PRINT_SETTINGS))

/**
 * BobguiPrintSettingsFunc:
 * @key: the setting key
 * @value: the setting value
 * @user_data: (closure): The user data provided with the function
 *
 * Function called by [method@Bobgui.PrintSettings.foreach] on every key/value pair
 * inside a [class@Bobgui.PrintSettings].
 */
typedef void  (*BobguiPrintSettingsFunc)  (const char *key,
					const char *value,
					gpointer     user_data);

typedef struct _BobguiPageRange BobguiPageRange;

/**
 * BobguiPageRange:
 * @start: start of page range.
 * @end: end of page range.
 *
 * A range of pages to print.
 *
 * See also [method@Bobgui.PrintSettings.set_page_ranges].
 */
struct _BobguiPageRange
{
  int start;
  int end;
};

GDK_AVAILABLE_IN_ALL
GType             bobgui_print_settings_get_type                (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiPrintSettings *bobgui_print_settings_new                     (void);

GDK_AVAILABLE_IN_ALL
BobguiPrintSettings *bobgui_print_settings_copy                    (BobguiPrintSettings     *other);

GDK_AVAILABLE_IN_ALL
BobguiPrintSettings *bobgui_print_settings_new_from_file           (const char           *file_name,
							      GError              **error);
GDK_AVAILABLE_IN_ALL
gboolean          bobgui_print_settings_load_file               (BobguiPrintSettings     *settings,
							      const char           *file_name,
							      GError              **error);
GDK_AVAILABLE_IN_ALL
gboolean          bobgui_print_settings_to_file                 (BobguiPrintSettings     *settings,
							      const char           *file_name,
							      GError              **error);
GDK_AVAILABLE_IN_ALL
BobguiPrintSettings *bobgui_print_settings_new_from_key_file       (GKeyFile             *key_file,
							      const char           *group_name,
							      GError              **error);
GDK_AVAILABLE_IN_ALL
gboolean          bobgui_print_settings_load_key_file           (BobguiPrintSettings     *settings,
							      GKeyFile             *key_file,
							      const char           *group_name,
							      GError              **error);
GDK_AVAILABLE_IN_ALL
void              bobgui_print_settings_to_key_file             (BobguiPrintSettings     *settings,
							      GKeyFile             *key_file,
							      const char           *group_name);
GDK_AVAILABLE_IN_ALL
gboolean          bobgui_print_settings_has_key                 (BobguiPrintSettings     *settings,
							      const char           *key);
GDK_AVAILABLE_IN_ALL
const char *     bobgui_print_settings_get                     (BobguiPrintSettings     *settings,
							      const char           *key);
GDK_AVAILABLE_IN_ALL
void              bobgui_print_settings_set                     (BobguiPrintSettings     *settings,
							      const char           *key,
							      const char           *value);
GDK_AVAILABLE_IN_ALL
void              bobgui_print_settings_unset                   (BobguiPrintSettings     *settings,
							      const char           *key);
GDK_AVAILABLE_IN_ALL
void              bobgui_print_settings_foreach                 (BobguiPrintSettings     *settings,
							      BobguiPrintSettingsFunc  func,
							      gpointer              user_data);
GDK_AVAILABLE_IN_ALL
gboolean          bobgui_print_settings_get_bool                (BobguiPrintSettings     *settings,
							      const char           *key);
GDK_AVAILABLE_IN_ALL
void              bobgui_print_settings_set_bool                (BobguiPrintSettings     *settings,
							      const char           *key,
							      gboolean              value);
GDK_AVAILABLE_IN_ALL
double            bobgui_print_settings_get_double              (BobguiPrintSettings     *settings,
							      const char           *key);
GDK_AVAILABLE_IN_ALL
double            bobgui_print_settings_get_double_with_default (BobguiPrintSettings     *settings,
							      const char           *key,
							      double                def);
GDK_AVAILABLE_IN_ALL
void              bobgui_print_settings_set_double              (BobguiPrintSettings     *settings,
							      const char           *key,
							      double                value);
GDK_AVAILABLE_IN_ALL
double            bobgui_print_settings_get_length              (BobguiPrintSettings     *settings,
							      const char           *key,
							      BobguiUnit               unit);
GDK_AVAILABLE_IN_ALL
void              bobgui_print_settings_set_length              (BobguiPrintSettings     *settings,
							      const char           *key,
							      double                value,
							      BobguiUnit               unit);
GDK_AVAILABLE_IN_ALL
int               bobgui_print_settings_get_int                 (BobguiPrintSettings     *settings,
							      const char           *key);
GDK_AVAILABLE_IN_ALL
int               bobgui_print_settings_get_int_with_default    (BobguiPrintSettings     *settings,
							      const char           *key,
							      int                   def);
GDK_AVAILABLE_IN_ALL
void              bobgui_print_settings_set_int                 (BobguiPrintSettings     *settings,
							      const char           *key,
							      int                   value);

/**
 * BOBGUI_PRINT_SETTINGS_PRINTER:
 *
 * The key used by the “Print to file” printer to store the printer name.
 */
#define BOBGUI_PRINT_SETTINGS_PRINTER          "printer"

/**
 * BOBGUI_PRINT_SETTINGS_ORIENTATION:
 *
 * The key used by the “Print to file” printer to store the orientation.
 */
#define BOBGUI_PRINT_SETTINGS_ORIENTATION      "orientation"

/**
 * BOBGUI_PRINT_SETTINGS_PAPER_FORMAT:
 *
 * The key used by the “Print to file” printer to store the page format.
 */
#define BOBGUI_PRINT_SETTINGS_PAPER_FORMAT     "paper-format"

/**
 * BOBGUI_PRINT_SETTINGS_PAPER_WIDTH:
 *
 * The key used by the “Print to file” printer to store the paper width.
 */
#define BOBGUI_PRINT_SETTINGS_PAPER_WIDTH      "paper-width"

/**
 * BOBGUI_PRINT_SETTINGS_PAPER_HEIGHT:
 *
 * The key used by the “Print to file” printer to store the page height.
 */
#define BOBGUI_PRINT_SETTINGS_PAPER_HEIGHT     "paper-height"

/**
 * BOBGUI_PRINT_SETTINGS_N_COPIES:
 *
 * The key used by the “Print to file” printer to store the number of copies.
 */
#define BOBGUI_PRINT_SETTINGS_N_COPIES         "n-copies"

/**
 * BOBGUI_PRINT_SETTINGS_DEFAULT_SOURCE:
 *
 * The key used by the “Print to file” printer to store the default source.
 */
#define BOBGUI_PRINT_SETTINGS_DEFAULT_SOURCE   "default-source"

/**
 * BOBGUI_PRINT_SETTINGS_QUALITY:
 *
 * The key used by the “Print to file” printer to store the printing quality.
 */
#define BOBGUI_PRINT_SETTINGS_QUALITY          "quality"

/**
 * BOBGUI_PRINT_SETTINGS_RESOLUTION:
 *
 * The key used by the “Print to file” printer to store the resolution in DPI.
 */
#define BOBGUI_PRINT_SETTINGS_RESOLUTION       "resolution"

/**
 * BOBGUI_PRINT_SETTINGS_USE_COLOR:
 *
 * The key used by the “Print to file” printer to store whether to print with
 * colors.
 */
#define BOBGUI_PRINT_SETTINGS_USE_COLOR        "use-color"

/**
 * BOBGUI_PRINT_SETTINGS_DUPLEX:
 *
 * The key used by the “Print to file” printer to store whether to print the
 * output in duplex.
 */
#define BOBGUI_PRINT_SETTINGS_DUPLEX           "duplex"

/**
 * BOBGUI_PRINT_SETTINGS_COLLATE:
 *
 * The key used by the “Print to file” printer to store whether to collate the
 * printed pages.
 */
#define BOBGUI_PRINT_SETTINGS_COLLATE          "collate"

/**
 * BOBGUI_PRINT_SETTINGS_REVERSE:
 *
 * The key used by the “Print to file” printer to store whether to reverse the
 * order of the printed pages.
 */
#define BOBGUI_PRINT_SETTINGS_REVERSE          "reverse"

/**
 * BOBGUI_PRINT_SETTINGS_MEDIA_TYPE:
 *
 * The key used by the “Print to file” printer to store the media type.
 *
 * The set of media types is defined in PWG 5101.1-2002 PWG.
 */
#define BOBGUI_PRINT_SETTINGS_MEDIA_TYPE       "media-type"

/**
 * BOBGUI_PRINT_SETTINGS_DITHER:
 *
 * The key used by the “Print to file” printer to store the dither used.
 */
#define BOBGUI_PRINT_SETTINGS_DITHER           "dither"

/**
 * BOBGUI_PRINT_SETTINGS_SCALE:
 *
 * The key used by the “Print to file” printer to store the scale.
 */
#define BOBGUI_PRINT_SETTINGS_SCALE            "scale"

/**
 * BOBGUI_PRINT_SETTINGS_PRINT_PAGES:
 *
 * The key used by the “Print to file” printer to store which pages to print.
 */
#define BOBGUI_PRINT_SETTINGS_PRINT_PAGES      "print-pages"

/**
 * BOBGUI_PRINT_SETTINGS_PAGE_RANGES:
 *
 * The key used by the “Print to file” printer to store the array of page ranges
 * to print.
 */
#define BOBGUI_PRINT_SETTINGS_PAGE_RANGES      "page-ranges"

/**
 * BOBGUI_PRINT_SETTINGS_PAGE_SET:
 *
 * The key used by the “Print to file” printer to store the set of pages to print.
 */
#define BOBGUI_PRINT_SETTINGS_PAGE_SET         "page-set"

/**
 * BOBGUI_PRINT_SETTINGS_FINISHINGS:
 *
 * The key used by the “Print to file” printer to store the finishings.
 */
#define BOBGUI_PRINT_SETTINGS_FINISHINGS       "finishings"

/**
 * BOBGUI_PRINT_SETTINGS_NUMBER_UP:
 *
 * The key used by the “Print to file” printer to store the number of pages per
 * sheet.
 */
#define BOBGUI_PRINT_SETTINGS_NUMBER_UP        "number-up"

/**
 * BOBGUI_PRINT_SETTINGS_NUMBER_UP_LAYOUT:
 *
 * The key used by the “Print to file” printer to store the number of pages per
 * sheet in number-up mode.
 */
#define BOBGUI_PRINT_SETTINGS_NUMBER_UP_LAYOUT "number-up-layout"

/**
 * BOBGUI_PRINT_SETTINGS_OUTPUT_BIN:
 *
 * The key used by the “Print to file” printer to store the output bin.
 */
#define BOBGUI_PRINT_SETTINGS_OUTPUT_BIN       "output-bin"

/**
 * BOBGUI_PRINT_SETTINGS_RESOLUTION_X:
 *
 * The key used by the “Print to file” printer to store the horizontal
 * resolution in DPI.
 */
#define BOBGUI_PRINT_SETTINGS_RESOLUTION_X     "resolution-x"

/**
 * BOBGUI_PRINT_SETTINGS_RESOLUTION_Y:
 *
 * The key used by the “Print to file” printer to store the vertical resolution
 * in DPI.
 */
#define BOBGUI_PRINT_SETTINGS_RESOLUTION_Y     "resolution-y"

/**
 * BOBGUI_PRINT_SETTINGS_PRINTER_LPI:
 *
 * The key used by the “Print to file” printer to store the resolution in lines
 * per inch.
 */
#define BOBGUI_PRINT_SETTINGS_PRINTER_LPI      "printer-lpi"

/**
 * BOBGUI_PRINT_SETTINGS_OUTPUT_DIR:
 *
 * The key used by the “Print to file” printer to store the
 * directory to which the output should be written.
 */
#define BOBGUI_PRINT_SETTINGS_OUTPUT_DIR       "output-dir"

/**
 * BOBGUI_PRINT_SETTINGS_OUTPUT_BASENAME:
 *
 * The key used by the “Print to file” printer to store the file
 * name of the output without the path to the directory and the
 * file extension.
 */
#define BOBGUI_PRINT_SETTINGS_OUTPUT_BASENAME  "output-basename"

/**
 * BOBGUI_PRINT_SETTINGS_OUTPUT_FILE_FORMAT:
 *
 * The key used by the “Print to file” printer to store the format
 * of the output. The supported values are “PS” and “PDF”.
 */
#define BOBGUI_PRINT_SETTINGS_OUTPUT_FILE_FORMAT  "output-file-format"

/**
 * BOBGUI_PRINT_SETTINGS_OUTPUT_URI:
 *
 * The key used by the “Print to file” printer to store the URI
 * to which the output should be written. BOBGUI itself supports
 * only “file://” URIs.
 */
#define BOBGUI_PRINT_SETTINGS_OUTPUT_URI          "output-uri"

/**
 * BOBGUI_PRINT_SETTINGS_WIN32_DRIVER_VERSION:
 *
 * The key used by the “Print to file” printer to store the 32-bit Windows
 * driver version.
 */
#define BOBGUI_PRINT_SETTINGS_WIN32_DRIVER_VERSION "win32-driver-version"

/**
 * BOBGUI_PRINT_SETTINGS_WIN32_DRIVER_EXTRA:
 *
 * The key used by the “Print to file” printer to store 32-bit Windows extra
 * driver.
 */
#define BOBGUI_PRINT_SETTINGS_WIN32_DRIVER_EXTRA   "win32-driver-extra"

/* Helpers: */

GDK_AVAILABLE_IN_ALL
const char *         bobgui_print_settings_get_printer           (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_printer           (BobguiPrintSettings   *settings,
								const char         *printer);
GDK_AVAILABLE_IN_ALL
BobguiPageOrientation    bobgui_print_settings_get_orientation       (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_orientation       (BobguiPrintSettings   *settings,
								BobguiPageOrientation  orientation);
GDK_AVAILABLE_IN_ALL
BobguiPaperSize *        bobgui_print_settings_get_paper_size        (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_paper_size        (BobguiPrintSettings   *settings,
								BobguiPaperSize       *paper_size);
GDK_AVAILABLE_IN_ALL
double                bobgui_print_settings_get_paper_width       (BobguiPrintSettings   *settings,
								BobguiUnit             unit);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_paper_width       (BobguiPrintSettings   *settings,
								double              width,
								BobguiUnit             unit);
GDK_AVAILABLE_IN_ALL
double                bobgui_print_settings_get_paper_height      (BobguiPrintSettings   *settings,
								BobguiUnit             unit);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_paper_height      (BobguiPrintSettings   *settings,
								double              height,
								BobguiUnit             unit);
GDK_AVAILABLE_IN_ALL
gboolean              bobgui_print_settings_get_use_color         (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_use_color         (BobguiPrintSettings   *settings,
								gboolean            use_color);
GDK_AVAILABLE_IN_ALL
gboolean              bobgui_print_settings_get_collate           (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_collate           (BobguiPrintSettings   *settings,
								gboolean            collate);
GDK_AVAILABLE_IN_ALL
gboolean              bobgui_print_settings_get_reverse           (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_reverse           (BobguiPrintSettings   *settings,
								gboolean            reverse);
GDK_AVAILABLE_IN_ALL
BobguiPrintDuplex        bobgui_print_settings_get_duplex            (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_duplex            (BobguiPrintSettings   *settings,
								BobguiPrintDuplex      duplex);
GDK_AVAILABLE_IN_ALL
BobguiPrintQuality       bobgui_print_settings_get_quality           (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_quality           (BobguiPrintSettings   *settings,
								BobguiPrintQuality     quality);
GDK_AVAILABLE_IN_ALL
int                   bobgui_print_settings_get_n_copies          (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_n_copies          (BobguiPrintSettings   *settings,
								int                 num_copies);
GDK_AVAILABLE_IN_ALL
int                   bobgui_print_settings_get_number_up         (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_number_up         (BobguiPrintSettings   *settings,
								int                 number_up);
GDK_AVAILABLE_IN_ALL
BobguiNumberUpLayout     bobgui_print_settings_get_number_up_layout  (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_number_up_layout  (BobguiPrintSettings   *settings,
								BobguiNumberUpLayout   number_up_layout);
GDK_AVAILABLE_IN_ALL
int                   bobgui_print_settings_get_resolution        (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_resolution        (BobguiPrintSettings   *settings,
								int                 resolution);
GDK_AVAILABLE_IN_ALL
int                   bobgui_print_settings_get_resolution_x      (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
int                   bobgui_print_settings_get_resolution_y      (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_resolution_xy     (BobguiPrintSettings   *settings,
								int                 resolution_x,
								int                 resolution_y);
GDK_AVAILABLE_IN_ALL
double                bobgui_print_settings_get_printer_lpi       (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_printer_lpi       (BobguiPrintSettings   *settings,
								double              lpi);
GDK_AVAILABLE_IN_ALL
double                bobgui_print_settings_get_scale             (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_scale             (BobguiPrintSettings   *settings,
								double              scale);
GDK_AVAILABLE_IN_ALL
BobguiPrintPages         bobgui_print_settings_get_print_pages       (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_print_pages       (BobguiPrintSettings   *settings,
								BobguiPrintPages       pages);
GDK_AVAILABLE_IN_ALL
BobguiPageRange *        bobgui_print_settings_get_page_ranges       (BobguiPrintSettings   *settings,
								int                *num_ranges);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_page_ranges       (BobguiPrintSettings   *settings,
								BobguiPageRange       *page_ranges,
								int                 num_ranges);
GDK_AVAILABLE_IN_ALL
BobguiPageSet            bobgui_print_settings_get_page_set          (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_page_set          (BobguiPrintSettings   *settings,
								BobguiPageSet          page_set);
GDK_AVAILABLE_IN_ALL
const char *         bobgui_print_settings_get_default_source    (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_default_source    (BobguiPrintSettings   *settings,
								const char         *default_source);
GDK_AVAILABLE_IN_ALL
const char *         bobgui_print_settings_get_media_type        (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_media_type        (BobguiPrintSettings   *settings,
								const char         *media_type);
GDK_AVAILABLE_IN_ALL
const char *         bobgui_print_settings_get_dither            (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_dither            (BobguiPrintSettings   *settings,
								const char         *dither);
GDK_AVAILABLE_IN_ALL
const char *         bobgui_print_settings_get_finishings        (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_finishings        (BobguiPrintSettings   *settings,
								const char         *finishings);
GDK_AVAILABLE_IN_ALL
const char *         bobgui_print_settings_get_output_bin        (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
void                  bobgui_print_settings_set_output_bin        (BobguiPrintSettings   *settings,
								const char         *output_bin);

GDK_AVAILABLE_IN_ALL
GVariant             *bobgui_print_settings_to_gvariant           (BobguiPrintSettings   *settings);
GDK_AVAILABLE_IN_ALL
BobguiPrintSettings     *bobgui_print_settings_new_from_gvariant     (GVariant           *variant);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiPrintSettings, g_object_unref)

G_END_DECLS

