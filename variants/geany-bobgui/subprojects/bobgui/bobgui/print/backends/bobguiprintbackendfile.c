/* BOBGUI - The Bobgui Framework
 * bobguiprintbackendfile.c: Default implementation of BobguiPrintBackend 
 * for printing to a file
 * Copyright (C) 2003, Red Hat, Inc.
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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#include <cairo.h>
#ifdef CAIRO_HAS_PDF_SURFACE
#include <cairo-pdf.h>
#endif
#ifdef CAIRO_HAS_PS_SURFACE
#include <cairo-ps.h>
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
#include <cairo-svg.h>
#endif

#include <glib/gi18n-lib.h>

#include "bobgui/bobgui.h"
#include "bobgui/print/bobguiprinterprivate.h"
#include "bobgui/bobguiprivate.h"
#include "bobgui/bobguimodulesprivate.h"

#include "bobguiprintbackendfile.h"

typedef struct _BobguiPrintBackendFileClass BobguiPrintBackendFileClass;

#define BOBGUI_PRINT_BACKEND_FILE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_PRINT_BACKEND_FILE, BobguiPrintBackendFileClass))
#define BOBGUI_IS_PRINT_BACKEND_FILE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_PRINT_BACKEND_FILE))
#define BOBGUI_PRINT_BACKEND_FILE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_PRINT_BACKEND_FILE, BobguiPrintBackendFileClass))

#define _STREAM_MAX_CHUNK_SIZE 8192

struct _BobguiPrintBackendFileClass
{
  BobguiPrintBackendClass parent_class;
};

struct _BobguiPrintBackendFile
{
  BobguiPrintBackend parent_instance;
};

typedef enum
{
#ifdef CAIRO_HAS_PDF_SURFACE
  FORMAT_PDF,
#endif
#ifdef CAIRO_HAS_PS_SURFACE
  FORMAT_PS,
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
  FORMAT_SVG,
#endif
  N_FORMATS
} OutputFormat;

static const char * formats[N_FORMATS] =
{
#ifdef CAIRO_HAS_PDF_SURFACE
  "pdf",
#endif
#ifdef CAIRO_HAS_PS_SURFACE
  "ps",
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
  "svg"
#endif
};

static GObjectClass *backend_parent_class;

static void                 file_printer_get_settings_from_options (BobguiPrinter              *printer,
								    BobguiPrinterOptionSet     *options,
								    BobguiPrintSettings        *settings);
static BobguiPrinterOptionSet *file_printer_get_options               (BobguiPrinter              *printer,
								    BobguiPrintSettings        *settings,
								    BobguiPageSetup            *page_setup,
								    BobguiPrintCapabilities     capabilities);
static void                 file_printer_prepare_for_print         (BobguiPrinter              *printer,
								    BobguiPrintJob             *print_job,
								    BobguiPrintSettings        *settings,
								    BobguiPageSetup            *page_setup);
static void                 bobgui_print_backend_file_print_stream    (BobguiPrintBackend         *print_backend,
								    BobguiPrintJob             *job,
								    GIOChannel              *data_io,
								    BobguiPrintJobCompleteFunc  callback,
								    gpointer                 user_data,
								    GDestroyNotify           dnotify);
static cairo_surface_t *    file_printer_create_cairo_surface      (BobguiPrinter              *printer,
								    BobguiPrintSettings        *settings,
								    double                   width,
								    double                   height,
								    GIOChannel              *cache_io);

static GList *              file_printer_list_papers               (BobguiPrinter              *printer);
static BobguiPageSetup *       file_printer_get_default_page_size     (BobguiPrinter              *printer);

BOBGUI_DEFINE_BUILTIN_MODULE_TYPE_WITH_CODE (BobguiPrintBackendFile, bobgui_print_backend_file, BOBGUI_TYPE_PRINT_BACKEND,
                         g_io_extension_point_implement (BOBGUI_PRINT_BACKEND_EXTENSION_POINT_NAME,
                                                         g_define_type_id,
                                                         "file",
                                                         20))

/**
 * bobgui_print_backend_file_new:
 *
 * Creates a new #BobguiPrintBackendFile object. #BobguiPrintBackendFile
 * implements the #BobguiPrintBackend interface with direct access to
 * the filesystem using Unix/Linux API calls
 *
 * Returns: the new #BobguiPrintBackendFile object
 **/
BobguiPrintBackend *
bobgui_print_backend_file_new (void)
{
  return g_object_new (BOBGUI_TYPE_PRINT_BACKEND_FILE, NULL);
}

static void
bobgui_print_backend_file_class_init (BobguiPrintBackendFileClass *class)
{
  BobguiPrintBackendClass *backend_class = BOBGUI_PRINT_BACKEND_CLASS (class);

  backend_parent_class = g_type_class_peek_parent (class);

  backend_class->print_stream = bobgui_print_backend_file_print_stream;
  backend_class->printer_create_cairo_surface = file_printer_create_cairo_surface;
  backend_class->printer_get_options = file_printer_get_options;
  backend_class->printer_get_settings_from_options = file_printer_get_settings_from_options;
  backend_class->printer_prepare_for_print = file_printer_prepare_for_print;
  backend_class->printer_list_papers = file_printer_list_papers;
  backend_class->printer_get_default_page_size = file_printer_get_default_page_size;
}

/* return N_FORMATS if no explicit format in the settings */
static OutputFormat
format_from_settings (BobguiPrintSettings *settings)
{
  const char *value;
  int i;

  if (settings == NULL)
    return N_FORMATS;

  value = bobgui_print_settings_get (settings,
                                  BOBGUI_PRINT_SETTINGS_OUTPUT_FILE_FORMAT);
  if (value == NULL)
    return N_FORMATS;

  for (i = 0; i < N_FORMATS; ++i)
    if (strcmp (value, formats[i]) == 0)
      break;

  g_assert (i < N_FORMATS);

  return (OutputFormat) i;
}

static char *
output_file_from_settings (BobguiPrintSettings *settings,
			   const char       *default_format)
{
  char *uri = NULL;

  if (settings)
    uri = g_strdup (bobgui_print_settings_get (settings, BOBGUI_PRINT_SETTINGS_OUTPUT_URI));

  if (uri == NULL)
    { 
      const char *extension, *basename = NULL, *output_dir = NULL;
      char *name, *locale_name, *path;

      if (default_format)
        extension = default_format;
      else
        {
          OutputFormat format;

          format = format_from_settings (settings);
          switch (format)
            {
              default:
              case N_FORMATS:
#ifdef CAIRO_HAS_PDF_SURFACE
              case FORMAT_PDF:
                extension = "pdf";
                break;
#endif
#ifdef CAIRO_HAS_PS_SURFACE
              case FORMAT_PS:
                extension = "ps";
                break;
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
              case FORMAT_SVG:
                extension = "svg";
                break;
#endif
            }
        }

      if (settings)
        basename = bobgui_print_settings_get (settings, BOBGUI_PRINT_SETTINGS_OUTPUT_BASENAME);
      if (basename == NULL)
        basename = _("output");

      name = g_strconcat (basename, ".", extension, NULL);

      locale_name = g_filename_from_utf8 (name, -1, NULL, NULL, NULL);
      g_free (name);

      if (locale_name != NULL)
        {
          if (settings)
            output_dir = bobgui_print_settings_get (settings, BOBGUI_PRINT_SETTINGS_OUTPUT_DIR);
          if (output_dir == NULL)
            {
              const char *document_dir = g_get_user_special_dir (G_USER_DIRECTORY_DOCUMENTS);

              if (document_dir == NULL)
                {
                  char *current_dir = g_get_current_dir ();
                  path = g_build_filename (current_dir, locale_name, NULL);
                  g_free (current_dir);
                }
              else
                path = g_build_filename (document_dir, locale_name, NULL);

              uri = g_filename_to_uri (path, NULL, NULL); 
	    }
          else
            {
              path = g_build_filename (output_dir, locale_name, NULL);
              uri = g_filename_to_uri (path, NULL, NULL);
            }

          g_free (path); 
          g_free (locale_name);
        }
    }

  return uri;
}

static cairo_status_t
_cairo_write (void                *closure,
              const unsigned char *data,
              unsigned int         length)
{
  GIOChannel *io = (GIOChannel *)closure;
  gsize written = 0;
  GError *error;

  error = NULL;

  BOBGUI_DEBUG (PRINTING, "FILE Backend: Writing %u byte chunk to temp file", length);

  while (length > 0) 
    {
      GIOStatus status;

      status = g_io_channel_write_chars (io, (const char *) data, length, &written, &error);

      if (status == G_IO_STATUS_ERROR)
        {
          if (error != NULL)
            {
              BOBGUI_DEBUG (PRINTING, "FILE Backend: Error writing to temp file, %s", error->message);

              g_error_free (error);
            }

	  return CAIRO_STATUS_WRITE_ERROR;
	}    

      BOBGUI_DEBUG (PRINTING, "FILE Backend: Wrote %zd bytes to temp file", written);
      
      data += written;
      length -= written;
    }

  return CAIRO_STATUS_SUCCESS;
}


static cairo_surface_t *
file_printer_create_cairo_surface (BobguiPrinter       *printer,
				   BobguiPrintSettings *settings,
				   double            width, 
				   double            height,
				   GIOChannel       *cache_io)
{
  cairo_surface_t *surface;
  OutputFormat format;
#ifdef CAIRO_HAS_SVG_SURFACE
  const cairo_svg_version_t *versions;
  int num_versions = 0;
#endif

  format = format_from_settings (settings);

  switch (format)
    {
      default:
      case N_FORMATS:
#ifdef CAIRO_HAS_PDF_SURFACE
      case FORMAT_PDF:
        surface = cairo_pdf_surface_create_for_stream (_cairo_write, cache_io, width, height);
        break;
#endif
#ifdef CAIRO_HAS_PS_SURFACE
      case FORMAT_PS:
        surface = cairo_ps_surface_create_for_stream (_cairo_write, cache_io, width, height);
        break;
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
      case FORMAT_SVG:
        surface = cairo_svg_surface_create_for_stream (_cairo_write, cache_io, width, height);
        cairo_svg_get_versions (&versions, &num_versions);
        if (num_versions > 0)
          cairo_svg_surface_restrict_to_version (surface, versions[num_versions - 1]);
        break;
#endif
    }

  cairo_surface_set_fallback_resolution (surface,
                                         2.0 * bobgui_print_settings_get_printer_lpi (settings),
                                         2.0 * bobgui_print_settings_get_printer_lpi (settings));

  return surface;
}

typedef struct {
  BobguiPrintBackend *backend;
  BobguiPrintJobCompleteFunc callback;
  BobguiPrintJob *job;
  GFileOutputStream *target_io_stream;
  gpointer user_data;
  GDestroyNotify dnotify;
} _PrintStreamData;

static void
file_print_cb (BobguiPrintBackendFile *print_backend,
               GError              *error,
               gpointer            user_data)
{
  char *uri;

  _PrintStreamData *ps = (_PrintStreamData *) user_data;
  BobguiRecentManager *recent_manager;

  if (ps->target_io_stream != NULL)
    (void)g_output_stream_close (G_OUTPUT_STREAM (ps->target_io_stream), NULL, NULL);

  if (ps->callback)
    ps->callback (ps->job, ps->user_data, error);

  if (ps->dnotify)
    ps->dnotify (ps->user_data);

  bobgui_print_job_set_status (ps->job,
			    (error != NULL)
                              ? BOBGUI_PRINT_STATUS_FINISHED_ABORTED
                              : BOBGUI_PRINT_STATUS_FINISHED);

  recent_manager = bobgui_recent_manager_get_default ();
  uri = output_file_from_settings (bobgui_print_job_get_settings (ps->job), NULL);
  bobgui_recent_manager_add_item (recent_manager, uri);
  g_free (uri);

  if (ps->job)
    g_object_unref (ps->job);

  g_free (ps);
}

static gboolean
file_write (GIOChannel   *source,
            GIOCondition  con,
            gpointer      user_data)
{
  char buf[_STREAM_MAX_CHUNK_SIZE];
  gsize bytes_read;
  GError *error;
  GIOStatus read_status;
  _PrintStreamData *ps = (_PrintStreamData *) user_data;

  error = NULL;

  read_status =
    g_io_channel_read_chars (source,
                             buf,
                             _STREAM_MAX_CHUNK_SIZE,
                             &bytes_read,
                             &error);

  if (read_status != G_IO_STATUS_ERROR)
    {
      gsize bytes_written;

      g_output_stream_write_all (G_OUTPUT_STREAM (ps->target_io_stream),
                                 buf,
                                 bytes_read,
                                 &bytes_written,
                                 NULL,
                                 &error);
    }

  if (error != NULL || read_status == G_IO_STATUS_EOF)
    {
      file_print_cb (BOBGUI_PRINT_BACKEND_FILE (ps->backend), error, user_data);

      if (error != NULL)
        {
          BOBGUI_DEBUG (PRINTING, "FILE Backend: %s", error->message);

          g_error_free (error);
        }

      return FALSE;
    }

  BOBGUI_DEBUG (PRINTING, "FILE Backend: Writing %"G_GSIZE_FORMAT" byte chunk to target file", bytes_read);

  return TRUE;
}

static void
bobgui_print_backend_file_print_stream (BobguiPrintBackend        *print_backend,
				     BobguiPrintJob            *job,
				     GIOChannel             *data_io,
				     BobguiPrintJobCompleteFunc callback,
				     gpointer                user_data,
				     GDestroyNotify          dnotify)
{
  GError *internal_error = NULL;
  _PrintStreamData *ps;
  BobguiPrintSettings *settings;
  char *uri;
  GFile *file = NULL;

  settings = bobgui_print_job_get_settings (job);

  ps = g_new0 (_PrintStreamData, 1);
  ps->callback = callback;
  ps->user_data = user_data;
  ps->dnotify = dnotify;
  ps->job = g_object_ref (job);
  ps->backend = print_backend;

  internal_error = NULL;
  uri = output_file_from_settings (settings, NULL);

  if (uri == NULL)
    goto error;

  file = g_file_new_for_uri (uri);
  ps->target_io_stream = g_file_replace (file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &internal_error);

  g_object_unref (file);
  g_free (uri);

error:
  if (internal_error != NULL)
    {
      file_print_cb (BOBGUI_PRINT_BACKEND_FILE (print_backend),
                     internal_error, ps);

      g_error_free (internal_error);
      return;
    }

  g_io_add_watch (data_io, 
                  G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP,
                  (GIOFunc) file_write,
                  ps);
}

static void
bobgui_print_backend_file_init (BobguiPrintBackendFile *backend)
{
  BobguiPrinter *printer;
  
  printer = g_object_new (BOBGUI_TYPE_PRINTER,
			  "name", _("Print to File"),
			  "backend", backend,
			  "is-virtual", TRUE,
			  "accepts-pdf", TRUE,
			  NULL); 

  bobgui_printer_set_has_details (printer, TRUE);
  bobgui_printer_set_icon_name (printer, "document-save");
  bobgui_printer_set_is_active (printer, TRUE);

  bobgui_print_backend_add_printer (BOBGUI_PRINT_BACKEND (backend), printer);
  g_object_unref (printer);

  bobgui_print_backend_set_list_done (BOBGUI_PRINT_BACKEND (backend));
}

typedef struct {
  BobguiPrinter          *printer;
  BobguiPrinterOptionSet *set;
} _OutputFormatChangedData;

static void
set_printer_format_from_option_set (BobguiPrinter          *printer,
				    BobguiPrinterOptionSet *set)
{
  BobguiPrinterOption *format_option;
  const char *value;
  int i;

  format_option = bobgui_printer_option_set_lookup (set, "output-file-format");
  if (format_option && format_option->value)
    {
      value = format_option->value;
      if (value)
        {
	  for (i = 0; i < N_FORMATS; ++i)
	    if (strcmp (value, formats[i]) == 0)
	      break;

	  g_assert (i < N_FORMATS);

	  switch (i)
	    {
#ifdef CAIRO_HAS_PDF_SURFACE
	      case FORMAT_PDF:
		bobgui_printer_set_accepts_pdf (printer, TRUE);
		bobgui_printer_set_accepts_ps (printer, FALSE);
		break;
#endif
#ifdef CAIRO_HAS_PS_SURFACE
	      case FORMAT_PS:
		bobgui_printer_set_accepts_pdf (printer, FALSE);
		bobgui_printer_set_accepts_ps (printer, TRUE);
		break;
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
	      case FORMAT_SVG:
#endif
	      default:
		bobgui_printer_set_accepts_pdf (printer, FALSE);
		bobgui_printer_set_accepts_ps (printer, FALSE);
		break;
	    }
	}
    }
}

static void
file_printer_output_file_format_changed (BobguiPrinterOption    *format_option,
					 gpointer             user_data)
{
  BobguiPrinterOption *uri_option;
  char             *base = NULL;
  _OutputFormatChangedData *data = (_OutputFormatChangedData *) user_data;

  if (! format_option->value)
    return;

  uri_option = bobgui_printer_option_set_lookup (data->set,
                                              "bobgui-main-page-custom-input");

  if (uri_option && uri_option->value)
    {
      const char *uri = uri_option->value;
      const char *dot = strrchr (uri, '.');

      if (dot)
        {
          int i;

          /*  check if the file extension matches one of the known ones  */
          for (i = 0; i < N_FORMATS; i++)
            if (strcmp (dot + 1, formats[i]) == 0)
              break;

          if (i < N_FORMATS && strcmp (formats[i], format_option->value))
            {
              /*  the file extension is known but doesn't match the
               *  selected one, strip it away
               */
              base = g_strndup (uri, dot - uri);
            }
        }
      else
        {
          /*  there's no file extension  */
          base = g_strdup (uri);
        }
    }

  if (base)
    {
      char *tmp = g_strdup_printf ("%s.%s", base, format_option->value);

      bobgui_printer_option_set (uri_option, tmp);
      g_free (tmp);
      g_free (base);
    }

  set_printer_format_from_option_set (data->printer, data->set);
}

static BobguiPrinterOptionSet *
file_printer_get_options (BobguiPrinter           *printer,
			  BobguiPrintSettings     *settings,
			  BobguiPageSetup         *page_setup,
			  BobguiPrintCapabilities  capabilities)
{
  BobguiPrinterOptionSet *set;
  BobguiPrinterOption *option;
  const char *n_up[] = {"1", "2", "4", "6", "9", "16" };
  const char *pages_per_sheet = NULL;
  const char *format_names[N_FORMATS] = { N_("PDF"), N_("PostScript"), N_("SVG") };
  const char *supported_formats[N_FORMATS];
  const char *display_format_names[N_FORMATS];
  int n_formats = 0;
  OutputFormat format;
  char *uri;
  int current_format = 0;
  _OutputFormatChangedData *format_changed_data;

  format = format_from_settings (settings);

  set = bobgui_printer_option_set_new ();

  option = bobgui_printer_option_new ("bobgui-n-up", _("Pages per _sheet:"), BOBGUI_PRINTER_OPTION_TYPE_PICKONE);
  bobgui_printer_option_choices_from_array (option, G_N_ELEMENTS (n_up),
                                         n_up, n_up /* FIXME i18n (localised digits)! */);
  if (settings)
    pages_per_sheet = bobgui_print_settings_get (settings, BOBGUI_PRINT_SETTINGS_NUMBER_UP);
  if (pages_per_sheet)
    bobgui_printer_option_set (option, pages_per_sheet);
  else
    bobgui_printer_option_set (option, "1");
  bobgui_printer_option_set_add (set, option);
  g_object_unref (option);

  if (capabilities & (BOBGUI_PRINT_CAPABILITY_GENERATE_PDF | BOBGUI_PRINT_CAPABILITY_GENERATE_PS))
    {
#ifdef CAIRO_HAS_PDF_SURFACE
      if (capabilities & BOBGUI_PRINT_CAPABILITY_GENERATE_PDF)
        {
	  if (format == FORMAT_PDF || format == N_FORMATS)
            {
              format = FORMAT_PDF;
	      current_format = n_formats;
            }
          supported_formats[n_formats] = formats[FORMAT_PDF];
	  display_format_names[n_formats] = _(format_names[FORMAT_PDF]);
	  n_formats++;
	}
#endif
#ifdef CAIRO_HAS_PS_SURFACE
      if (capabilities & BOBGUI_PRINT_CAPABILITY_GENERATE_PS)
        {
	  if (format == FORMAT_PS || format == N_FORMATS)
	    current_format = n_formats;
          supported_formats[n_formats] = formats[FORMAT_PS];
          display_format_names[n_formats] = _(format_names[FORMAT_PS]);
	  n_formats++;
	}
#endif
    }
  else
    {
      switch (format)
        {
          default:
          case N_FORMATS:
#ifdef CAIRO_HAS_PDF_SURFACE
          case FORMAT_PDF:
            current_format = FORMAT_PDF;
            break;
#endif
#ifdef CAIRO_HAS_PS_SURFACE
          case FORMAT_PS:
            current_format = FORMAT_PS;
            break;
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
          case FORMAT_SVG:
            current_format = FORMAT_SVG;            
            break;
#endif
        }

      for (n_formats = 0; n_formats < N_FORMATS; ++n_formats)
        {
	  supported_formats[n_formats] = formats[n_formats];
          display_format_names[n_formats] = _(format_names[n_formats]);
	}
    }

  uri = output_file_from_settings (settings, supported_formats[current_format]);

  option = bobgui_printer_option_new ("bobgui-main-page-custom-input", _("File"), 
				   BOBGUI_PRINTER_OPTION_TYPE_FILESAVE);
  bobgui_printer_option_set_activates_default (option, TRUE);
  bobgui_printer_option_set (option, uri);
  g_free (uri);
  option->group = g_strdup ("BobguiPrintDialogExtension");
  bobgui_printer_option_set_add (set, option);

  if (n_formats > 1)
    {
      option = bobgui_printer_option_new ("output-file-format", _("_Output format"), 
				       BOBGUI_PRINTER_OPTION_TYPE_ALTERNATIVE);
      option->group = g_strdup ("BobguiPrintDialogExtension");

      bobgui_printer_option_choices_from_array (option, n_formats,
                                             supported_formats,
                                             display_format_names);
      bobgui_printer_option_set (option, supported_formats[current_format]);
      bobgui_printer_option_set_add (set, option);

      set_printer_format_from_option_set (printer, set);
      format_changed_data = g_new (_OutputFormatChangedData, 1);
      format_changed_data->printer = printer;
      format_changed_data->set = set;
      g_signal_connect_data (option, "changed",
			     G_CALLBACK (file_printer_output_file_format_changed),
			     format_changed_data, (GClosureNotify)g_free, 0);

      g_object_unref (option);
    }

  return set;
}

static void
file_printer_get_settings_from_options (BobguiPrinter          *printer,
					BobguiPrinterOptionSet *options,
					BobguiPrintSettings    *settings)
{
  BobguiPrinterOption *option;

  option = bobgui_printer_option_set_lookup (options, "bobgui-main-page-custom-input");
  bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_OUTPUT_URI, option->value);

  option = bobgui_printer_option_set_lookup (options, "output-file-format");
  if (option)
    bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_OUTPUT_FILE_FORMAT, option->value);

  option = bobgui_printer_option_set_lookup (options, "bobgui-n-up");
  if (option)
    bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_NUMBER_UP, option->value);

  option = bobgui_printer_option_set_lookup (options, "bobgui-n-up-layout");
  if (option)
    bobgui_print_settings_set (settings, BOBGUI_PRINT_SETTINGS_NUMBER_UP_LAYOUT, option->value);
}

static void
file_printer_prepare_for_print (BobguiPrinter       *printer,
				BobguiPrintJob      *print_job,
				BobguiPrintSettings *settings,
				BobguiPageSetup     *page_setup)
{
  double scale;
  BobguiPrintPages pages;
  BobguiPageRange *ranges;
  int n_ranges;
  OutputFormat format;

  pages = bobgui_print_settings_get_print_pages (settings);
  bobgui_print_job_set_pages (print_job, pages);

  if (pages == BOBGUI_PRINT_PAGES_RANGES)
    ranges = bobgui_print_settings_get_page_ranges (settings, &n_ranges);
  else
    {
      ranges = NULL;
      n_ranges = 0;
    }

  bobgui_print_job_set_page_ranges (print_job, ranges, n_ranges);
  bobgui_print_job_set_collate (print_job, bobgui_print_settings_get_collate (settings));
  bobgui_print_job_set_reverse (print_job, bobgui_print_settings_get_reverse (settings));
  bobgui_print_job_set_num_copies (print_job, bobgui_print_settings_get_n_copies (settings));
  bobgui_print_job_set_n_up (print_job, bobgui_print_settings_get_number_up (settings));
  bobgui_print_job_set_n_up_layout (print_job, bobgui_print_settings_get_number_up_layout (settings));

  scale = bobgui_print_settings_get_scale (settings);
  if (scale != 100.0)
    bobgui_print_job_set_scale (print_job, scale / 100.0);

  bobgui_print_job_set_page_set (print_job, bobgui_print_settings_get_page_set (settings));

  format = format_from_settings (settings);
  switch (format)
    {
#ifdef CAIRO_HAS_PDF_SURFACE
      case FORMAT_PDF:
#endif
      case N_FORMATS:
	bobgui_print_job_set_rotate (print_job, FALSE);
        break;
      default:
#ifdef CAIRO_HAS_PS_SURFACE
      case FORMAT_PS:
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
      case FORMAT_SVG:
#endif
	bobgui_print_job_set_rotate (print_job, TRUE);
        break;
    }
}

static GList *
file_printer_list_papers (BobguiPrinter *printer)
{
  GList *result = NULL;
  GList *papers, *p;
  BobguiPageSetup *page_setup;

  papers = bobgui_paper_size_get_paper_sizes (FALSE);

  for (p = papers; p; p = p->next)
    {
      BobguiPaperSize *paper_size = p->data;

      page_setup = bobgui_page_setup_new ();
      bobgui_page_setup_set_paper_size (page_setup, paper_size);
      bobgui_paper_size_free (paper_size);
      result = g_list_prepend (result, page_setup);
    }

  g_list_free (papers);

  return g_list_reverse (result);
}

static BobguiPageSetup *
file_printer_get_default_page_size (BobguiPrinter *printer)
{
  BobguiPageSetup *result = NULL;

  return result;
}
