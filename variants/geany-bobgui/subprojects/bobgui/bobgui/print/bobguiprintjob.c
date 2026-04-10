/* BobguiPrintJob
 * Copyright (C) 2006 John (J5) Palmieri  <johnp@redhat.com>
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

/**
 * BobguiPrintJob:
 *
 * Represents a job that is sent to a printer.
 *
 * You only need to deal directly with print jobs if you use the
 * non-portable [class@Bobgui.PrintUnixDialog] API.
 *
 * Use [method@Bobgui.PrintJob.get_surface] to obtain the cairo surface
 * onto which the pages must be drawn. Use [method@Bobgui.PrintJob.send]
 * to send the finished job to the printer. If you don’t use cairo
 * `BobguiPrintJob` also supports printing of manually generated PostScript,
 * via [method@Bobgui.PrintJob.set_source_file].
 */
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>

#include <glib/gstdio.h>

#include "bobguiprintjob.h"
#include "bobguiprinter.h"
#include "bobguiprinterprivate.h"
#include "bobguiprintbackendprivate.h"

#ifndef O_BINARY
#define O_BINARY 0
#endif

typedef struct _BobguiPrintJobClass     BobguiPrintJobClass;

struct _BobguiPrintJob
{
  GObject parent_instance;

  char *title;

  GIOChannel *spool_io;
  cairo_surface_t *surface;

  BobguiPrintStatus status;
  BobguiPrintBackend *backend;
  BobguiPrinter *printer;
  BobguiPrintSettings *settings;
  BobguiPageSetup *page_setup;

  BobguiPrintPages print_pages;
  BobguiPageRange *page_ranges;
  int num_page_ranges;
  BobguiPageSet page_set;
  int num_copies;
  double scale;
  guint number_up;
  BobguiNumberUpLayout number_up_layout;

  guint printer_set           : 1;
  guint page_setup_set        : 1;
  guint settings_set          : 1;
  guint track_print_status    : 1;
  guint rotate_to_orientation : 1;
  guint collate               : 1;
  guint reverse               : 1;
};

struct _BobguiPrintJobClass
{
  GObjectClass parent_class;

  void (*status_changed) (BobguiPrintJob *job);
};

static void     bobgui_print_job_finalize     (GObject               *object);
static void     bobgui_print_job_set_property (GObject               *object,
					    guint                  prop_id,
					    const GValue          *value,
					    GParamSpec            *pspec);
static void     bobgui_print_job_get_property (GObject               *object,
					    guint                  prop_id,
					    GValue                *value,
					    GParamSpec            *pspec);
static void     bobgui_print_job_constructed  (GObject               *object);

enum {
  STATUS_CHANGED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_TITLE,
  PROP_PRINTER,
  PROP_PAGE_SETUP,
  PROP_SETTINGS,
  PROP_TRACK_PRINT_STATUS
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (BobguiPrintJob, bobgui_print_job, G_TYPE_OBJECT)

static void
bobgui_print_job_class_init (BobguiPrintJobClass *class)
{
  GObjectClass *object_class;
  object_class = (GObjectClass *) class;

  object_class->finalize = bobgui_print_job_finalize;
  object_class->constructed = bobgui_print_job_constructed;
  object_class->set_property = bobgui_print_job_set_property;
  object_class->get_property = bobgui_print_job_get_property;

  /**
   * BobguiPrintJob:title:
   *
   * The title of the print job.
   */
  g_object_class_install_property (object_class,
                                   PROP_TITLE,
                                   g_param_spec_string ("title", NULL, NULL,
						        NULL,
							G_PARAM_READWRITE |
						        G_PARAM_CONSTRUCT_ONLY));

  /**
   * BobguiPrintJob:printer:
   *
   * The printer to send the job to.
   */
  g_object_class_install_property (object_class,
                                   PROP_PRINTER,
                                   g_param_spec_object ("printer", NULL, NULL,
						        BOBGUI_TYPE_PRINTER,
							G_PARAM_READWRITE |
						        G_PARAM_CONSTRUCT_ONLY));

  /**
   * BobguiPrintJob:settings:
   *
   * Printer settings.
   */
  g_object_class_install_property (object_class,
                                   PROP_SETTINGS,
                                   g_param_spec_object ("settings", NULL, NULL,
						        BOBGUI_TYPE_PRINT_SETTINGS,
							G_PARAM_READWRITE |
						        G_PARAM_CONSTRUCT_ONLY));

  /**
   * BobguiPrintJob:page-setup:
   *
   * Page setup.
   */
  g_object_class_install_property (object_class,
                                   PROP_PAGE_SETUP,
                                   g_param_spec_object ("page-setup", NULL, NULL,
						        BOBGUI_TYPE_PAGE_SETUP,
							G_PARAM_READWRITE |
						        G_PARAM_CONSTRUCT_ONLY));

  /**
   * BobguiPrintJob:track-print-status:
   *
   * %TRUE if the print job will continue to emit status-changed
   * signals after the print data has been setn to the printer.
   */
  g_object_class_install_property (object_class,
				   PROP_TRACK_PRINT_STATUS,
				   g_param_spec_boolean ("track-print-status", NULL, NULL,
							 FALSE,
							 G_PARAM_READWRITE));

  /**
   * BobguiPrintJob::status-changed:
   * @job: the `BobguiPrintJob` object on which the signal was emitted
   *
   * Emitted when the status of a job changes.
   *
   * The signal handler can use [method@Bobgui.PrintJob.get_status]
   * to obtain the new status.
   */
  signals[STATUS_CHANGED] =
    g_signal_new ("status-changed",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BobguiPrintJobClass, status_changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);
}

static void
bobgui_print_job_init (BobguiPrintJob *job)
{
  job->spool_io = NULL;

  job->title = g_strdup ("");
  job->surface = NULL;
  job->backend = NULL;
  job->printer = NULL;

  job->printer_set = FALSE;
  job->settings_set = FALSE;
  job->page_setup_set = FALSE;
  job->status = BOBGUI_PRINT_STATUS_INITIAL;
  job->track_print_status = FALSE;

  job->print_pages = BOBGUI_PRINT_PAGES_ALL;
  job->page_ranges = NULL;
  job->num_page_ranges = 0;
  job->collate = FALSE;
  job->reverse = FALSE;
  job->num_copies = 1;
  job->scale = 1.0;
  job->page_set = BOBGUI_PAGE_SET_ALL;
  job->rotate_to_orientation = FALSE;
  job->number_up = 1;
  job->number_up_layout = BOBGUI_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM;
}


static void
bobgui_print_job_constructed (GObject *object)
{
  BobguiPrintJob *job = BOBGUI_PRINT_JOB (object);

  G_OBJECT_CLASS (bobgui_print_job_parent_class)->constructed (object);

  g_assert (job->printer_set &&
	    job->settings_set &&
	    job->page_setup_set);

  _bobgui_printer_prepare_for_print (job->printer,
				  job,
				  job->settings,
				  job->page_setup);
}


static void
bobgui_print_job_finalize (GObject *object)
{
  BobguiPrintJob *job = BOBGUI_PRINT_JOB (object);

  if (job->surface)
    cairo_surface_destroy (job->surface);

  if (job->backend)
    g_object_unref (job->backend);

  if (job->spool_io != NULL)
    {
      g_io_channel_unref (job->spool_io);
      job->spool_io = NULL;
    }

  if (job->printer)
    g_object_unref (job->printer);

  if (job->settings)
    g_object_unref (job->settings);

  if (job->page_setup)
    g_object_unref (job->page_setup);

  g_free (job->page_ranges);
  job->page_ranges = NULL;

  g_free (job->title);
  job->title = NULL;

  G_OBJECT_CLASS (bobgui_print_job_parent_class)->finalize (object);
}

/**
 * bobgui_print_job_new:
 * @title: the job title
 * @printer: a `BobguiPrinter`
 * @settings: a `BobguiPrintSettings`
 * @page_setup: a `BobguiPageSetup`
 *
 * Creates a new `BobguiPrintJob`.
 *
 * Returns: a new `BobguiPrintJob`
 */
BobguiPrintJob *
bobgui_print_job_new (const char       *title,
		   BobguiPrinter       *printer,
		   BobguiPrintSettings *settings,
		   BobguiPageSetup     *page_setup)
{
  GObject *result;
  result = g_object_new (BOBGUI_TYPE_PRINT_JOB,
                         "title", title,
			 "printer", printer,
			 "settings", settings,
			 "page-setup", page_setup,
			 NULL);
  return (BobguiPrintJob *) result;
}

/**
 * bobgui_print_job_get_settings:
 * @job: a `BobguiPrintJob`
 *
 * Gets the `BobguiPrintSettings` of the print job.
 *
 * Returns: (transfer none): the settings of @job
 */
BobguiPrintSettings *
bobgui_print_job_get_settings (BobguiPrintJob *job)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_JOB (job), NULL);

  return job->settings;
}

/**
 * bobgui_print_job_get_printer:
 * @job: a `BobguiPrintJob`
 *
 * Gets the `BobguiPrinter` of the print job.
 *
 * Returns: (transfer none): the printer of @job
 */
BobguiPrinter *
bobgui_print_job_get_printer (BobguiPrintJob *job)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_JOB (job), NULL);

  return job->printer;
}

/**
 * bobgui_print_job_get_title:
 * @job: a `BobguiPrintJob`
 *
 * Gets the job title.
 *
 * Returns: the title of @job
 */
const char *
bobgui_print_job_get_title (BobguiPrintJob *job)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_JOB (job), NULL);

  return job->title;
}

/**
 * bobgui_print_job_get_status:
 * @job: a `BobguiPrintJob`
 *
 * Gets the status of the print job.
 *
 * Returns: the status of @job
 */
BobguiPrintStatus
bobgui_print_job_get_status (BobguiPrintJob *job)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_JOB (job), BOBGUI_PRINT_STATUS_FINISHED);

  return job->status;
}

void
bobgui_print_job_set_status (BobguiPrintJob   *job,
			  BobguiPrintStatus status)
{
  g_return_if_fail (BOBGUI_IS_PRINT_JOB (job));

  if (job->status == status)
    return;

  job->status = status;
  g_signal_emit (job, signals[STATUS_CHANGED], 0);
}

/**
 * bobgui_print_job_set_source_file:
 * @job: a `BobguiPrintJob`
 * @filename: (type filename): the file to be printed
 * @error: return location for errors
 *
 * Make the `BobguiPrintJob` send an existing document to the
 * printing system.
 *
 * The file can be in any format understood by the platforms
 * printing system (typically PostScript, but on many platforms
 * PDF may work too). See [method@Bobgui.Printer.accepts_pdf] and
 * [method@Bobgui.Printer.accepts_ps].
 *
 * Returns: %FALSE if an error occurred
 */
gboolean
bobgui_print_job_set_source_file (BobguiPrintJob *job,
			       const char *filename,
			       GError     **error)
{
  GError *tmp_error;

  tmp_error = NULL;

  g_return_val_if_fail (BOBGUI_IS_PRINT_JOB (job), FALSE);

  if (job->spool_io != NULL)
    g_io_channel_unref (job->spool_io);

  job->spool_io = g_io_channel_new_file (filename, "r", &tmp_error);

  if (tmp_error == NULL)
    g_io_channel_set_encoding (job->spool_io, NULL, &tmp_error);

  if (tmp_error != NULL)
    {
      g_propagate_error (error, tmp_error);
      return FALSE;
    }

  return TRUE;
}

/**
 * bobgui_print_job_set_source_fd:
 * @job: a `BobguiPrintJob`
 * @fd: a file descriptor
 * @error: return location for errors
 *
 * Make the `BobguiPrintJob` send an existing document to the
 * printing system.
 *
 * The file can be in any format understood by the platforms
 * printing system (typically PostScript, but on many platforms
 * PDF may work too). See [method@Bobgui.Printer.accepts_pdf] and
 * [method@Bobgui.Printer.accepts_ps].
 *
 * This is similar to [method@Bobgui.PrintJob.set_source_file],
 * but takes expects an open file descriptor for the file,
 * instead of a filename.
 *
 * Returns: %FALSE if an error occurred
 */
gboolean
bobgui_print_job_set_source_fd (BobguiPrintJob  *job,
                             int           fd,
                             GError      **error)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_JOB (job), FALSE);
  g_return_val_if_fail (fd >= 0, FALSE);

  if (job->spool_io != NULL)
    g_io_channel_unref (job->spool_io);

  job->spool_io = g_io_channel_unix_new (fd);
  if (g_io_channel_set_encoding (job->spool_io, NULL, error) != G_IO_STATUS_NORMAL)
    return FALSE;

  return TRUE;
}

/**
 * bobgui_print_job_get_surface:
 * @job: a `BobguiPrintJob`
 * @error: (nullable): return location for errors
 *
 * Gets a cairo surface onto which the pages of
 * the print job should be rendered.
 *
 * Returns: (transfer none): the cairo surface of @job
 */
cairo_surface_t *
bobgui_print_job_get_surface (BobguiPrintJob  *job,
			   GError      **error)
{
  char *filename = NULL;
  double width, height;
  BobguiPaperSize *paper_size;
  int fd;
  GError *tmp_error;

  tmp_error = NULL;

  g_return_val_if_fail (BOBGUI_IS_PRINT_JOB (job), NULL);

  if (job->surface)
    return job->surface;

  g_return_val_if_fail (job->spool_io == NULL, NULL);

  fd = g_file_open_tmp ("bobguiprint_XXXXXX",
			 &filename,
			 &tmp_error);
  if (fd == -1)
    {
      g_free (filename);
      g_propagate_error (error, tmp_error);
      return NULL;
    }

  fchmod (fd, S_IRUSR | S_IWUSR);

  /* If we are debugging printing don't delete the tmp files */
  if (!BOBGUI_DEBUG_CHECK (PRINTING))
    g_unlink (filename);
  g_free (filename);

  paper_size = bobgui_page_setup_get_paper_size (job->page_setup);
  width = bobgui_paper_size_get_width (paper_size, BOBGUI_UNIT_POINTS);
  height = bobgui_paper_size_get_height (paper_size, BOBGUI_UNIT_POINTS);

  job->spool_io = g_io_channel_unix_new (fd);
  g_io_channel_set_close_on_unref (job->spool_io, TRUE);
  g_io_channel_set_encoding (job->spool_io, NULL, &tmp_error);

  if (tmp_error != NULL)
    {
      g_io_channel_unref (job->spool_io);
      job->spool_io = NULL;
      g_propagate_error (error, tmp_error);
      return NULL;
    }

  job->surface = _bobgui_printer_create_cairo_surface (job->printer,
						     job->settings,
						     width, height,
						     job->spool_io);

  return job->surface;
}

/**
 * bobgui_print_job_set_track_print_status:
 * @job: a `BobguiPrintJob`
 * @track_status: %TRUE to track status after printing
 *
 * If track_status is %TRUE, the print job will try to continue report
 * on the status of the print job in the printer queues and printer.
 *
 * This can allow your application to show things like “out of paper”
 * issues, and when the print job actually reaches the printer.
 *
 * This function is often implemented using some form of polling,
 * so it should not be enabled unless needed.
 */
void
bobgui_print_job_set_track_print_status (BobguiPrintJob *job,
				      gboolean     track_status)
{
  g_return_if_fail (BOBGUI_IS_PRINT_JOB (job));

  track_status = track_status != FALSE;

  if (job->track_print_status != track_status)
    {
      job->track_print_status = track_status;

      g_object_notify (G_OBJECT (job), "track-print-status");
    }
}

/**
 * bobgui_print_job_get_track_print_status:
 * @job: a `BobguiPrintJob`
 *
 * Returns whether jobs will be tracked after printing.
 *
 * For details, see [method@Bobgui.PrintJob.set_track_print_status].
 *
 * Returns: %TRUE if print job status will be reported after printing
 */
gboolean
bobgui_print_job_get_track_print_status (BobguiPrintJob *job)
{
  g_return_val_if_fail (BOBGUI_IS_PRINT_JOB (job), FALSE);

  return job->track_print_status;
}

static void
bobgui_print_job_set_property (GObject      *object,
	                    guint         prop_id,
	                    const GValue *value,
                            GParamSpec   *pspec)

{
  BobguiPrintJob *job = BOBGUI_PRINT_JOB (object);
  BobguiPrintSettings *settings;

  switch (prop_id)
    {
    case PROP_TITLE:
      g_free (job->title);
      job->title = g_value_dup_string (value);
      break;

    case PROP_PRINTER:
      job->printer = BOBGUI_PRINTER (g_value_dup_object (value));
      job->printer_set = TRUE;
      job->backend = g_object_ref (bobgui_printer_get_backend (job->printer));
      break;

    case PROP_PAGE_SETUP:
      job->page_setup = BOBGUI_PAGE_SETUP (g_value_dup_object (value));
      job->page_setup_set = TRUE;
      break;

    case PROP_SETTINGS:
      /* We save a copy of the settings since we modify
       * if when preparing the printer job. */
      settings = BOBGUI_PRINT_SETTINGS (g_value_get_object (value));
      job->settings = bobgui_print_settings_copy (settings);
      job->settings_set = TRUE;
      break;

    case PROP_TRACK_PRINT_STATUS:
      bobgui_print_job_set_track_print_status (job, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_print_job_get_property (GObject    *object,
			    guint       prop_id,
			    GValue     *value,
			    GParamSpec *pspec)
{
  BobguiPrintJob *job = BOBGUI_PRINT_JOB (object);

  switch (prop_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, job->title);
      break;
    case PROP_PRINTER:
      g_value_set_object (value, job->printer);
      break;
    case PROP_SETTINGS:
      g_value_set_object (value, job->settings);
      break;
    case PROP_PAGE_SETUP:
      g_value_set_object (value, job->page_setup);
      break;
    case PROP_TRACK_PRINT_STATUS:
      g_value_set_boolean (value, job->track_print_status);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * bobgui_print_job_send:
 * @job: a `BobguiPrintJob`
 * @callback: (scope notified) (closure user_data) (destroy dnotify): function
 *   to call when the job completes or an error occurs
 * @user_data: user data that gets passed to @callback
 * @dnotify: destroy notify for @user_data
 *
 * Sends the print job off to the printer.
 */
void
bobgui_print_job_send (BobguiPrintJob             *job,
                    BobguiPrintJobCompleteFunc  callback,
                    gpointer                 user_data,
		    GDestroyNotify           dnotify)
{
  g_return_if_fail (BOBGUI_IS_PRINT_JOB (job));
  g_return_if_fail (job->spool_io != NULL);

  bobgui_print_job_set_status (job, BOBGUI_PRINT_STATUS_SENDING_DATA);

  if (g_io_channel_get_flags (job->spool_io) & G_IO_FLAG_IS_SEEKABLE)
    g_io_channel_seek_position (job->spool_io, 0, G_SEEK_SET, NULL);

  bobgui_print_backend_print_stream (job->backend, job,
				  job->spool_io,
                                  callback, user_data, dnotify);
}

/**
 * bobgui_print_job_get_pages:
 * @job: a `BobguiPrintJob`
 *
 * Gets the `BobguiPrintPages` setting for this job.
 *
 * Returns: the `BobguiPrintPages` setting
 */
BobguiPrintPages
bobgui_print_job_get_pages (BobguiPrintJob *job)
{
  return job->print_pages;
}

/**
 * bobgui_print_job_set_pages:
 * @job: a `BobguiPrintJob`
 * @pages: the `BobguiPrintPages` setting
 *
 * Sets the `BobguiPrintPages` setting for this job.
 */
void
bobgui_print_job_set_pages (BobguiPrintJob   *job,
                         BobguiPrintPages  pages)
{
  job->print_pages = pages;
}

/**
 * bobgui_print_job_get_page_ranges:
 * @job: a `BobguiPrintJob`
 * @n_ranges: (out): return location for the number of ranges
 *
 * Gets the page ranges for this job.
 *
 * Returns: (array length=n_ranges) (transfer none): a pointer to an
 *   array of `BobguiPageRange` structs
 */
BobguiPageRange *
bobgui_print_job_get_page_ranges (BobguiPrintJob *job,
                               int         *n_ranges)
{
  *n_ranges = job->num_page_ranges;
  return job->page_ranges;
}

/**
 * bobgui_print_job_set_page_ranges:
 * @job: a `BobguiPrintJob`
 * @ranges: (array length=n_ranges) (transfer full): pointer to an array of
 *    `BobguiPageRange` structs
 * @n_ranges: the length of the @ranges array
 *
 * Sets the page ranges for this job.
 */
void
bobgui_print_job_set_page_ranges (BobguiPrintJob  *job,
                               BobguiPageRange *ranges,
                               int           n_ranges)
{
  g_free (job->page_ranges);
  job->page_ranges = ranges;
  job->num_page_ranges = n_ranges;
}

/**
 * bobgui_print_job_get_page_set:
 * @job: a `BobguiPrintJob`
 *
 * Gets the `BobguiPageSet` setting for this job.
 *
 * Returns: the `BobguiPageSet` setting
 */
BobguiPageSet
bobgui_print_job_get_page_set (BobguiPrintJob *job)
{
  return job->page_set;
}

/**
 * bobgui_print_job_set_page_set:
 * @job: a `BobguiPrintJob`
 * @page_set: a `BobguiPageSet` setting
 *
 * Sets the `BobguiPageSet` setting for this job.
 */
void
bobgui_print_job_set_page_set (BobguiPrintJob *job,
                            BobguiPageSet   page_set)
{
  job->page_set = page_set;
}

/**
 * bobgui_print_job_get_num_copies:
 * @job: a `BobguiPrintJob`
 *
 * Gets the number of copies of this job.
 *
 * Returns: the number of copies
 */
int
bobgui_print_job_get_num_copies (BobguiPrintJob *job)
{
  return job->num_copies;
}

/**
 * bobgui_print_job_set_num_copies:
 * @job: a `BobguiPrintJob`
 * @num_copies: the number of copies
 *
 * Sets the number of copies for this job.
 */
void
bobgui_print_job_set_num_copies (BobguiPrintJob *job,
                              int          num_copies)
{
  job->num_copies = num_copies;
}

/**
 * bobgui_print_job_get_scale:
 * @job: a `BobguiPrintJob`
 *
 * Gets the scale for this job.
 *
 * Returns: the scale
 */
double
bobgui_print_job_get_scale (BobguiPrintJob *job)

{
  return job->scale;
}

/**
 * bobgui_print_job_set_scale:
 * @job: a `BobguiPrintJob`
 * @scale: the scale
 *
 * Sets the scale for this job.
 *
 * 1.0 means unscaled.
 */
void
bobgui_print_job_set_scale (BobguiPrintJob *job,
                         double       scale)
{
  job->scale = scale;
}

/**
 * bobgui_print_job_get_n_up:
 * @job: a `BobguiPrintJob`
 *
 * Gets the n-up setting for this job.
 *
 * Returns: the n-up setting
 */
guint
bobgui_print_job_get_n_up (BobguiPrintJob *job)
{
  return job->number_up;
}

/**
 * bobgui_print_job_set_n_up:
 * @job: a `BobguiPrintJob`
 * @n_up: the n-up value
 *
 * Sets the n-up setting for this job.
 */
void
bobgui_print_job_set_n_up (BobguiPrintJob *job,
                        guint        n_up)
{
  job->number_up = n_up;
}

/**
 * bobgui_print_job_get_n_up_layout:
 * @job: a `BobguiPrintJob`
 *
 * Gets the n-up layout setting for this job.
 *
 * Returns: the n-up layout
 */
BobguiNumberUpLayout
bobgui_print_job_get_n_up_layout (BobguiPrintJob *job)
{
  return job->number_up_layout;
}

/**
 * bobgui_print_job_set_n_up_layout:
 * @job: a `BobguiPrintJob`
 * @layout: the n-up layout setting
 *
 * Sets the n-up layout setting for this job.
 */
void
bobgui_print_job_set_n_up_layout (BobguiPrintJob       *job,
                               BobguiNumberUpLayout  layout)
{
  job->number_up_layout = layout;
}

/**
 * bobgui_print_job_get_rotate:
 * @job: a `BobguiPrintJob`
 *
 * Gets whether the job is printed rotated.
 *
 * Returns: whether the job is printed rotated
 */
gboolean
bobgui_print_job_get_rotate (BobguiPrintJob *job)
{
  return job->rotate_to_orientation;
}

/**
 * bobgui_print_job_set_rotate:
 * @job: a `BobguiPrintJob`
 * @rotate: whether to print rotated
 *
 * Sets whether this job is printed rotated.
 */
void
bobgui_print_job_set_rotate (BobguiPrintJob *job,
                          gboolean     rotate)
{
  job->rotate_to_orientation = rotate;
}

/**
 * bobgui_print_job_get_collate:
 * @job: a `BobguiPrintJob`
 *
 * Gets whether this job is printed collated.
 *
 * Returns: whether the job is printed collated
 */
gboolean
bobgui_print_job_get_collate (BobguiPrintJob *job)
{
  return job->collate;
}

/**
 * bobgui_print_job_set_collate:
 * @job: a `BobguiPrintJob`
 * @collate: whether the job is printed collated
 *
 * Sets whether this job is printed collated.
 */
void
bobgui_print_job_set_collate (BobguiPrintJob *job,
                           gboolean     collate)
{
  job->collate = collate;
}

/**
 * bobgui_print_job_get_reverse:
 * @job: a `BobguiPrintJob`
 *
 * Gets whether this job is printed reversed.
 *
 * Returns: whether the job is printed reversed.
 */
gboolean
bobgui_print_job_get_reverse (BobguiPrintJob *job)
{
  return job->reverse;
}

/**
 * bobgui_print_job_set_reverse:
 * @job: a `BobguiPrintJob`
 * @reverse: whether the job is printed reversed
 *
 * Sets whether this job is printed reversed.
 */
void
bobgui_print_job_set_reverse (BobguiPrintJob *job,
                           gboolean     reverse)
{
  job->reverse = reverse;
}
