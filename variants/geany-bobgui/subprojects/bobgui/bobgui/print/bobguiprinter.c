/* BobguiPrinter
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

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "bobguiprinter.h"
#include "bobguiprinterprivate.h"
#include "bobguiprintbackendprivate.h"
#include "bobguiprintjob.h"

/**
 * BobguiPrintBackend:
 *
 * A print backend.
 */

/**
 * BobguiPrinter:
 *
 * Represents a printer.
 *
 * You only need to deal directly with printers if you use the
 * non-portable [class@Bobgui.PrintUnixDialog] API.
 *
 * A `BobguiPrinter` allows to get status information about the printer,
 * such as its description, its location, the number of queued jobs,
 * etc. Most importantly, a `BobguiPrinter` object can be used to create
 * a [class@Bobgui.PrintJob] object, which lets you print to the printer.
 */


static void bobgui_printer_finalize     (GObject *object);

struct _BobguiPrinterPrivate
{
  char *name;
  char *location;
  char *description;
  char *icon_name;

  guint is_active         : 1;
  guint is_paused         : 1;
  guint is_accepting_jobs : 1;
  guint is_new            : 1;
  guint is_virtual        : 1;
  guint is_default        : 1;
  guint has_details       : 1;
  guint accepts_pdf       : 1;
  guint accepts_ps        : 1;

  char *state_message;
  int job_count;

  BobguiPrintBackend *backend;
};

enum {
  DETAILS_ACQUIRED,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_NAME,
  PROP_BACKEND,
  PROP_IS_VIRTUAL,
  PROP_STATE_MESSAGE,
  PROP_LOCATION,
  PROP_ICON_NAME,
  PROP_JOB_COUNT,
  PROP_ACCEPTS_PDF,
  PROP_ACCEPTS_PS,
  PROP_PAUSED,
  PROP_ACCEPTING_JOBS
};

static guint signals[LAST_SIGNAL] = { 0 };

static void bobgui_printer_set_property (GObject      *object,
				      guint         prop_id,
				      const GValue *value,
				      GParamSpec   *pspec);
static void bobgui_printer_get_property (GObject      *object,
				      guint         prop_id,
				      GValue       *value,
				      GParamSpec   *pspec);

G_DEFINE_TYPE_WITH_PRIVATE (BobguiPrinter, bobgui_printer, G_TYPE_OBJECT)

static void
bobgui_printer_class_init (BobguiPrinterClass *class)
{
  GObjectClass *object_class;
  object_class = (GObjectClass *) class;

  object_class->finalize = bobgui_printer_finalize;

  object_class->set_property = bobgui_printer_set_property;
  object_class->get_property = bobgui_printer_get_property;

  /**
   * BobguiPrinter:name:
   *
   * The name of the printer.
   */
  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_NAME,
                                   g_param_spec_string ("name", NULL, NULL,
						        "",
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  /**
   * BobguiPrinter:backend:
   *
   * The backend for the printer.
   */
  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_BACKEND,
                                   g_param_spec_object ("backend", NULL, NULL,
						        BOBGUI_TYPE_PRINT_BACKEND,
							G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  /**
   * BobguiPrinter:is-virtual: (getter is_virtual)
   *
   * %FALSE if this represents a real hardware device.
   */
  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_IS_VIRTUAL,
                                   g_param_spec_boolean ("is-virtual", NULL, NULL,
							 FALSE,
							 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  /**
   * BobguiPrinter:accepts-pdf: (getter accepts_pdf)
   *
   * %TRUE if this printer can accept PDF.
   */
  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_ACCEPTS_PDF,
                                   g_param_spec_boolean ("accepts-pdf", NULL, NULL,
							 FALSE,
							 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  /**
   * BobguiPrinter:accepts-ps: (getter accepts_ps)
   *
   * %TRUE if this printer can accept PostScript.
   */
  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_ACCEPTS_PS,
                                   g_param_spec_boolean ("accepts-ps", NULL, NULL,
							 TRUE,
							 G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  /**
   * BobguiPrinter:state-message:
   *
   * String giving the current status of the printer.
   */
  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_STATE_MESSAGE,
                                   g_param_spec_string ("state-message", NULL, NULL,
						        "",
							G_PARAM_READABLE));

  /**
   * BobguiPrinter:location:
   *
   * Information about the location of the printer.
   */
  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_LOCATION,
                                   g_param_spec_string ("location", NULL, NULL,
						        "",
							G_PARAM_READABLE));

  /**
   * BobguiPrinter:icon-name:
   *
   * Icon name to use for the printer.
   */
  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_ICON_NAME,
                                   g_param_spec_string ("icon-name", NULL, NULL,
						        "printer",
							G_PARAM_READABLE));

  /**
   * BobguiPrinter:job-count:
   *
   * Number of jobs queued in the printer.
   */
  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_JOB_COUNT,
				   g_param_spec_int ("job-count", NULL, NULL,
 						     0,
 						     G_MAXINT,
 						     0,
 						     G_PARAM_READABLE));

  /**
   * BobguiPrinter:paused: (getter is_paused)
   *
   * %TRUE if this printer is paused.
   *
   * A paused printer still accepts jobs, but it does
   * not print them.
   */
  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_PAUSED,
                                   g_param_spec_boolean ("paused", NULL, NULL,
							 FALSE,
							 G_PARAM_READABLE));

  /**
   * BobguiPrinter:accepting-jobs: (getter is_accepting_jobs)
   *
   * %TRUE if the printer is accepting jobs.
   */
  g_object_class_install_property (G_OBJECT_CLASS (class),
                                   PROP_ACCEPTING_JOBS,
                                   g_param_spec_boolean ("accepting-jobs", NULL, NULL,
							 TRUE,
							 G_PARAM_READABLE));

  /**
   * BobguiPrinter::details-acquired:
   * @printer: the `BobguiPrinter` on which the signal is emitted
   * @success: %TRUE if the details were successfully acquired
   *
   * Emitted in response to a request for detailed information
   * about a printer from the print backend.
   *
   * The @success parameter indicates if the information was
   * actually obtained.
   */
  signals[DETAILS_ACQUIRED] =
    g_signal_new ("details-acquired",
		  G_TYPE_FROM_CLASS (class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (BobguiPrinterClass, details_acquired),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 1, G_TYPE_BOOLEAN);
}

static void
bobgui_printer_init (BobguiPrinter *printer)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  priv->name = NULL;
  priv->location = NULL;
  priv->description = NULL;
  priv->icon_name = g_strdup ("printer");

  priv->is_active = TRUE;
  priv->is_paused = FALSE;
  priv->is_accepting_jobs = TRUE;
  priv->is_new = TRUE;
  priv->has_details = FALSE;
  priv->accepts_pdf = FALSE;
  priv->accepts_ps = TRUE;

  priv->state_message = NULL;  
  priv->job_count = 0;
}

static void
bobgui_printer_finalize (GObject *object)
{
  BobguiPrinter *printer = BOBGUI_PRINTER (object);
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_free (priv->name);
  g_free (priv->location);
  g_free (priv->description);
  g_free (priv->state_message);
  g_free (priv->icon_name);

  if (priv->backend)
    g_object_unref (priv->backend);

  G_OBJECT_CLASS (bobgui_printer_parent_class)->finalize (object);
}

static void
bobgui_printer_set_property (GObject         *object,
			  guint            prop_id,
			  const GValue    *value,
			  GParamSpec      *pspec)
{
  BobguiPrinter *printer = BOBGUI_PRINTER (object);
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  switch (prop_id)
    {
    case PROP_NAME:
      priv->name = g_value_dup_string (value);
      break;
    
    case PROP_BACKEND:
      priv->backend = BOBGUI_PRINT_BACKEND (g_value_dup_object (value));
      break;

    case PROP_IS_VIRTUAL:
      priv->is_virtual = g_value_get_boolean (value);
      break;

    case PROP_ACCEPTS_PDF:
      priv->accepts_pdf = g_value_get_boolean (value);
      break;

    case PROP_ACCEPTS_PS:
      priv->accepts_ps = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_printer_get_property (GObject    *object,
			  guint       prop_id,
			  GValue     *value,
			  GParamSpec *pspec)
{
  BobguiPrinter *printer = BOBGUI_PRINTER (object);
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  switch (prop_id)
    {
    case PROP_NAME:
      if (priv->name)
	g_value_set_string (value, priv->name);
      else
	g_value_set_static_string (value, "");
      break;
    case PROP_BACKEND:
      g_value_set_object (value, priv->backend);
      break;
    case PROP_STATE_MESSAGE:
      if (priv->state_message)
	g_value_set_string (value, priv->state_message);
      else
	g_value_set_static_string (value, "");
      break;
    case PROP_LOCATION:
      if (priv->location)
	g_value_set_string (value, priv->location);
      else
	g_value_set_static_string (value, "");
      break;
    case PROP_ICON_NAME:
      if (priv->icon_name)
	g_value_set_string (value, priv->icon_name);
      else
	g_value_set_static_string (value, "printer");
      break;
    case PROP_JOB_COUNT:
      g_value_set_int (value, priv->job_count);
      break;
    case PROP_IS_VIRTUAL:
      g_value_set_boolean (value, priv->is_virtual);
      break;
    case PROP_ACCEPTS_PDF:
      g_value_set_boolean (value, priv->accepts_pdf);
      break;
    case PROP_ACCEPTS_PS:
      g_value_set_boolean (value, priv->accepts_ps);
      break;
    case PROP_PAUSED:
      g_value_set_boolean (value, priv->is_paused);
      break;
    case PROP_ACCEPTING_JOBS:
      g_value_set_boolean (value, priv->is_accepting_jobs);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/**
 * bobgui_printer_new:
 * @name: the name of the printer
 * @backend: a `BobguiPrintBackend`
 * @virtual_: whether the printer is virtual
 *
 * Creates a new `BobguiPrinter`.
 *
 * Returns: a new `BobguiPrinter`
 */
BobguiPrinter *
bobgui_printer_new (const char      *name,
		 BobguiPrintBackend *backend,
		 gboolean         virtual_)
{
  GObject *result;
  
  result = g_object_new (BOBGUI_TYPE_PRINTER,
			 "name", name,
			 "backend", backend,
			 "is-virtual", virtual_,
                         NULL);

  return (BobguiPrinter *) result;
}

/**
 * bobgui_printer_get_backend:
 * @printer: a `BobguiPrinter`
 *
 * Returns the backend of the printer.
 *
 * Returns: (transfer none): the backend of @printer
 */
BobguiPrintBackend *
bobgui_printer_get_backend (BobguiPrinter *printer)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), NULL);
  
  return priv->backend;
}

/**
 * bobgui_printer_get_name:
 * @printer: a `BobguiPrinter`
 *
 * Returns the name of the printer.
 *
 * Returns: the name of @printer
 */
const char *
bobgui_printer_get_name (BobguiPrinter *printer)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), NULL);

  return priv->name;
}

/**
 * bobgui_printer_get_description:
 * @printer: a `BobguiPrinter`
 *
 * Gets the description of the printer.
 *
 * Returns: the description of @printer
 */
const char *
bobgui_printer_get_description (BobguiPrinter *printer)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), NULL);
  
  return priv->description;
}

gboolean
bobgui_printer_set_description (BobguiPrinter  *printer,
			     const char *description)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), FALSE);

  if (g_strcmp0 (priv->description, description) == 0)
    return FALSE;

  g_free (priv->description);
  priv->description = g_strdup (description);
  
  return TRUE;
}

/**
 * bobgui_printer_get_state_message:
 * @printer: a `BobguiPrinter`
 *
 * Returns the state message describing the current state
 * of the printer.
 *
 * Returns: the state message of @printer
 */
const char *
bobgui_printer_get_state_message (BobguiPrinter *printer)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), NULL);

  return priv->state_message;
}

gboolean
bobgui_printer_set_state_message (BobguiPrinter  *printer,
			       const char *message)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), FALSE);

  if (g_strcmp0 (priv->state_message, message) == 0)
    return FALSE;

  g_free (priv->state_message);
  priv->state_message = g_strdup (message);
  g_object_notify (G_OBJECT (printer), "state-message");

  return TRUE;
}

/**
 * bobgui_printer_get_location:
 * @printer: a `BobguiPrinter`
 *
 * Returns a description of the location of the printer.
 *
 * Returns: the location of @printer
 */
const char *
bobgui_printer_get_location (BobguiPrinter *printer)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), NULL);

  return priv->location;
}

gboolean
bobgui_printer_set_location (BobguiPrinter  *printer,
			  const char *location)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), FALSE);

  if (g_strcmp0 (priv->location, location) == 0)
    return FALSE;

  g_free (priv->location);
  priv->location = g_strdup (location);
  g_object_notify (G_OBJECT (printer), "location");
  
  return TRUE;
}

/**
 * bobgui_printer_get_icon_name:
 * @printer: a `BobguiPrinter`
 *
 * Gets the name of the icon to use for the printer.
 *
 * Returns: the icon name for @printer
 */
const char *
bobgui_printer_get_icon_name (BobguiPrinter *printer)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), NULL);

  return priv->icon_name;
}

void
bobgui_printer_set_icon_name (BobguiPrinter  *printer,
			   const char *icon)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_if_fail (BOBGUI_IS_PRINTER (printer));

  g_free (priv->icon_name);
  priv->icon_name = g_strdup (icon);
  g_object_notify (G_OBJECT (printer), "icon-name");
}

/**
 * bobgui_printer_get_job_count:
 * @printer: a `BobguiPrinter`
 *
 * Gets the number of jobs currently queued on the printer.
 *
 * Returns: the number of jobs on @printer
 */
int
bobgui_printer_get_job_count (BobguiPrinter *printer)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), 0);

  return priv->job_count;
}

gboolean
bobgui_printer_set_job_count (BobguiPrinter *printer,
			   int         count)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), FALSE);

  if (priv->job_count == count)
    return FALSE;

  priv->job_count = count;
  
  g_object_notify (G_OBJECT (printer), "job-count");
  
  return TRUE;
}

/**
 * bobgui_printer_has_details:
 * @printer: a `BobguiPrinter`
 *
 * Returns whether the printer details are available.
 *
 * Returns: %TRUE if @printer details are available
 */
gboolean
bobgui_printer_has_details (BobguiPrinter *printer)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), FALSE);

  return priv->has_details;
}

void
bobgui_printer_set_has_details (BobguiPrinter *printer,
			     gboolean    val)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  priv->has_details = val;
}

/**
 * bobgui_printer_is_active:
 * @printer: a `BobguiPrinter`
 *
 * Returns whether the printer is currently active (i.e.
 * accepts new jobs).
 *
 * Returns: %TRUE if @printer is active
 */
gboolean
bobgui_printer_is_active (BobguiPrinter *printer)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), TRUE);
  
  return priv->is_active;
}

void
bobgui_printer_set_is_active (BobguiPrinter *printer,
			   gboolean val)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_if_fail (BOBGUI_IS_PRINTER (printer));

  priv->is_active = val;
}

/**
 * bobgui_printer_is_paused: (get-property paused)
 * @printer: a `BobguiPrinter`
 *
 * Returns whether the printer is currently paused.
 *
 * A paused printer still accepts jobs, but it is not
 * printing them.
 *
 * Returns: %TRUE if @printer is paused
 */
gboolean
bobgui_printer_is_paused (BobguiPrinter *printer)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), TRUE);
  
  return priv->is_paused;
}

gboolean
bobgui_printer_set_is_paused (BobguiPrinter *printer,
			   gboolean    val)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), FALSE);

  if (val == priv->is_paused)
    return FALSE;

  priv->is_paused = val;

  return TRUE;
}

/**
 * bobgui_printer_is_accepting_jobs: (get-property accepting-jobs)
 * @printer: a `BobguiPrinter`
 *
 * Returns whether the printer is accepting jobs
 *
 * Returns: %TRUE if @printer is accepting jobs
 */
gboolean
bobgui_printer_is_accepting_jobs (BobguiPrinter *printer)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), TRUE);
  
  return priv->is_accepting_jobs;
}

gboolean
bobgui_printer_set_is_accepting_jobs (BobguiPrinter *printer,
				   gboolean val)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), FALSE);

  if (val == priv->is_accepting_jobs)
    return FALSE;

  priv->is_accepting_jobs = val;

  return TRUE;
}

/**
 * bobgui_printer_is_virtual: (get-property is-virtual)
 * @printer: a `BobguiPrinter`
 *
 * Returns whether the printer is virtual (i.e. does not
 * represent actual printer hardware, but something like
 * a CUPS class).
 *
 * Returns: %TRUE if @printer is virtual
 */
gboolean
bobgui_printer_is_virtual (BobguiPrinter *printer)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), TRUE);
  
  return priv->is_virtual;
}

/**
 * bobgui_printer_accepts_pdf: (get-property accepts-pdf)
 * @printer: a `BobguiPrinter`
 *
 * Returns whether the printer accepts input in
 * PDF format.
 *
 * Returns: %TRUE if @printer accepts PDF
 */
gboolean
bobgui_printer_accepts_pdf (BobguiPrinter *printer)
{ 
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), TRUE);
  
  return priv->accepts_pdf;
}

void
bobgui_printer_set_accepts_pdf (BobguiPrinter *printer,
			     gboolean val)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_if_fail (BOBGUI_IS_PRINTER (printer));

  priv->accepts_pdf = val;
}

/**
 * bobgui_printer_accepts_ps: (get-property accepts-ps)
 * @printer: a `BobguiPrinter`
 *
 * Returns whether the printer accepts input in
 * PostScript format.
 *
 * Returns: %TRUE if @printer accepts PostScript
 */
gboolean
bobgui_printer_accepts_ps (BobguiPrinter *printer)
{ 
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), TRUE);
  
  return priv->accepts_ps;
}

void
bobgui_printer_set_accepts_ps (BobguiPrinter *printer,
			    gboolean val)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_if_fail (BOBGUI_IS_PRINTER (printer));

  priv->accepts_ps = val;
}

gboolean
bobgui_printer_is_new (BobguiPrinter *printer)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), FALSE);
  
  return priv->is_new;
}

void
bobgui_printer_set_is_new (BobguiPrinter *printer,
			gboolean val)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_if_fail (BOBGUI_IS_PRINTER (printer));

  priv->is_new = val;
}


/**
 * bobgui_printer_is_default:
 * @printer: a `BobguiPrinter`
 *
 * Returns whether the printer is the default printer.
 *
 * Returns: %TRUE if @printer is the default
 */
gboolean
bobgui_printer_is_default (BobguiPrinter *printer)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), FALSE);
  
  return priv->is_default;
}

void
bobgui_printer_set_is_default (BobguiPrinter *printer,
			    gboolean    val)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);

  g_return_if_fail (BOBGUI_IS_PRINTER (printer));

  priv->is_default = val;
}

/**
 * bobgui_printer_request_details:
 * @printer: a `BobguiPrinter`
 *
 * Requests the printer details.
 *
 * When the details are available, the
 * [signal@Bobgui.Printer::details-acquired] signal
 * will be emitted on @printer.
 */
void
bobgui_printer_request_details (BobguiPrinter *printer)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);
  BobguiPrintBackendClass *backend_class;

  g_return_if_fail (BOBGUI_IS_PRINTER (printer));

  backend_class = BOBGUI_PRINT_BACKEND_GET_CLASS (priv->backend);
  backend_class->printer_request_details (printer);
}

BobguiPrinterOptionSet *
_bobgui_printer_get_options (BobguiPrinter           *printer,
			  BobguiPrintSettings     *settings,
			  BobguiPageSetup         *page_setup,
			  BobguiPrintCapabilities  capabilities)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);
  BobguiPrintBackendClass *backend_class = BOBGUI_PRINT_BACKEND_GET_CLASS (priv->backend);

  return backend_class->printer_get_options (printer, settings, page_setup, capabilities);
}

gboolean
_bobgui_printer_mark_conflicts (BobguiPrinter          *printer,
			     BobguiPrinterOptionSet *options)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);
  BobguiPrintBackendClass *backend_class = BOBGUI_PRINT_BACKEND_GET_CLASS (priv->backend);

  return backend_class->printer_mark_conflicts (printer, options);
}
  
void
_bobgui_printer_get_settings_from_options (BobguiPrinter          *printer,
					BobguiPrinterOptionSet *options,
					BobguiPrintSettings    *settings)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);
  BobguiPrintBackendClass *backend_class = BOBGUI_PRINT_BACKEND_GET_CLASS (priv->backend);

  backend_class->printer_get_settings_from_options (printer, options, settings);
}

void
_bobgui_printer_prepare_for_print (BobguiPrinter       *printer,
				BobguiPrintJob      *print_job,
				BobguiPrintSettings *settings,
				BobguiPageSetup     *page_setup)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);
  BobguiPrintBackendClass *backend_class = BOBGUI_PRINT_BACKEND_GET_CLASS (priv->backend);

  backend_class->printer_prepare_for_print (printer, print_job, settings, page_setup);
}

cairo_surface_t *
_bobgui_printer_create_cairo_surface (BobguiPrinter       *printer,
				   BobguiPrintSettings *settings,
				   double            width, 
				   double            height,
				   GIOChannel       *cache_io)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);
  BobguiPrintBackendClass *backend_class = BOBGUI_PRINT_BACKEND_GET_CLASS (priv->backend);

  return backend_class->printer_create_cairo_surface (printer, settings,
						      width, height, cache_io);
}

/**
 * bobgui_printer_list_papers:
 * @printer: a `BobguiPrinter`
 *
 * Lists all the paper sizes @printer supports.
 *
 * This will return and empty list unless the printer’s details
 * are available, see [method@Bobgui.Printer.has_details] and
 * [method@Bobgui.Printer.request_details].
 *
 * Returns: (element-type BobguiPageSetup) (transfer full): a newly
 *   allocated list of newly allocated `BobguiPageSetup`s.
 */
GList  *
bobgui_printer_list_papers (BobguiPrinter *printer)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);
  BobguiPrintBackendClass *backend_class;

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), NULL);

  backend_class = BOBGUI_PRINT_BACKEND_GET_CLASS (priv->backend);
  return backend_class->printer_list_papers (printer);
}

/**
 * bobgui_printer_get_default_page_size:
 * @printer: a `BobguiPrinter`
 *
 * Returns default page size of @printer.
 *
 * Returns: (transfer full): a newly allocated `BobguiPageSetup` with default page size
 *   of the printer.
 */
BobguiPageSetup *
bobgui_printer_get_default_page_size (BobguiPrinter *printer)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);
  BobguiPrintBackendClass *backend_class;

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), NULL);

  backend_class = BOBGUI_PRINT_BACKEND_GET_CLASS (priv->backend);

  return backend_class->printer_get_default_page_size (printer);
}

/**
 * bobgui_printer_get_hard_margins:
 * @printer: a `BobguiPrinter`
 * @top: (out): a location to store the top margin in
 * @bottom: (out): a location to store the bottom margin in
 * @left: (out): a location to store the left margin in
 * @right: (out): a location to store the right margin in
 *
 * Retrieve the hard margins of @printer.
 *
 * These are the margins that define the area at the borders
 * of the paper that the printer cannot print to.
 *
 * Note: This will not succeed unless the printer’s details are
 * available, see [method@Bobgui.Printer.has_details] and
 * [method@Bobgui.Printer.request_details].
 *
 * Returns: %TRUE iff the hard margins were retrieved
 */
gboolean
bobgui_printer_get_hard_margins (BobguiPrinter *printer,
			      double     *top,
			      double     *bottom,
			      double     *left,
			      double     *right)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);
  BobguiPrintBackendClass *backend_class = BOBGUI_PRINT_BACKEND_GET_CLASS (priv->backend);

  return backend_class->printer_get_hard_margins (printer, top, bottom, left, right);
}

/**
 * bobgui_printer_get_hard_margins_for_paper_size:
 * @printer: a `BobguiPrinter`
 * @paper_size: a `BobguiPaperSize`
 * @top: (out): a location to store the top margin in
 * @bottom: (out): a location to store the bottom margin in
 * @left: (out): a location to store the left margin in
 * @right: (out): a location to store the right margin in
 *
 * Retrieve the hard margins of @printer for @paper_size.
 *
 * These are the margins that define the area at the borders
 * of the paper that the printer cannot print to.
 *
 * Note: This will not succeed unless the printer’s details are
 * available, see [method@Bobgui.Printer.has_details] and
 * [method@Bobgui.Printer.request_details].
 *
 * Return value: %TRUE iff the hard margins were retrieved
 */
gboolean
bobgui_printer_get_hard_margins_for_paper_size (BobguiPrinter   *printer,
					     BobguiPaperSize *paper_size,
					     double       *top,
					     double       *bottom,
					     double       *left,
					     double       *right)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);
  BobguiPrintBackendClass *backend_class = BOBGUI_PRINT_BACKEND_GET_CLASS (priv->backend);

  return backend_class->printer_get_hard_margins_for_paper_size (printer, paper_size, top, bottom, left, right);
}

/**
 * bobgui_printer_get_capabilities:
 * @printer: a `BobguiPrinter`
 * 
 * Returns the printer’s capabilities.
 *
 * This is useful when you’re using `BobguiPrintUnixDialog`’s
 * manual-capabilities setting and need to know which settings
 * the printer can handle and which you must handle yourself.
 *
 * This will return 0 unless the printer’s details are
 * available, see [method@Bobgui.Printer.has_details] and
 * [method@Bobgui.Printer.request_details].
 *
 * Returns: the printer’s capabilities
 */
BobguiPrintCapabilities
bobgui_printer_get_capabilities (BobguiPrinter *printer)
{
  BobguiPrinterPrivate *priv = bobgui_printer_get_instance_private (printer);
  BobguiPrintBackendClass *backend_class;

  g_return_val_if_fail (BOBGUI_IS_PRINTER (printer), 0);

  backend_class = BOBGUI_PRINT_BACKEND_GET_CLASS (priv->backend);
  return backend_class->printer_get_capabilities (printer);
}

/**
 * bobgui_printer_compare:
 * @a: a `BobguiPrinter`
 * @b: another `BobguiPrinter`
 *
 * Compares two printers.
 *
 * Returns: 0 if the printer match, a negative value if @a < @b,
 *   or a positive value if @a > @b
 */
int
bobgui_printer_compare (BobguiPrinter *a, 
                     BobguiPrinter *b)
{
  const char *name_a, *name_b;
  
  g_assert (BOBGUI_IS_PRINTER (a) && BOBGUI_IS_PRINTER (b));
  
  name_a = bobgui_printer_get_name (a);
  name_b = bobgui_printer_get_name (b);
  if (name_a == NULL  && name_b == NULL)
    return 0;
  else if (name_a == NULL)
    return G_MAXINT;
  else if (name_b == NULL)
    return G_MININT;
  else
    return g_ascii_strcasecmp (name_a, name_b);
}


typedef struct 
{
  GList *backends;
  BobguiPrinterFunc func;
  gpointer data;
  GDestroyNotify destroy;
  GMainLoop *loop;
} PrinterList;

static void list_done_cb (BobguiPrintBackend *backend, 
			  PrinterList     *printer_list);

static void
stop_enumeration (PrinterList *printer_list)
{
  GList *list, *next;
  BobguiPrintBackend *backend;

  for (list = printer_list->backends; list; list = next)
    {
      next = list->next;
      backend = BOBGUI_PRINT_BACKEND (list->data);
      list_done_cb (backend, printer_list);
    }
}

static void 
free_printer_list (PrinterList *printer_list)
{
  if (printer_list->destroy)
    printer_list->destroy (printer_list->data);

  if (printer_list->loop)
    {    
      g_main_loop_quit (printer_list->loop);
      g_main_loop_unref (printer_list->loop);
    }

  g_free (printer_list);
}

static gboolean
list_added_cb (BobguiPrintBackend *backend, 
	       BobguiPrinter      *printer, 
	       PrinterList     *printer_list)
{
  if (printer_list->func (printer, printer_list->data))
    {
      stop_enumeration (printer_list);
      return TRUE;
    }

  return FALSE;
}

static void
backend_status_changed (GObject    *object,
                        GParamSpec *pspec,
                        gpointer    data)
{
  BobguiPrintBackend *backend = BOBGUI_PRINT_BACKEND (object);
  PrinterList *printer_list = data;
  BobguiPrintBackendStatus status;

  g_object_get (backend, "status", &status, NULL);
 
  if (status == BOBGUI_PRINT_BACKEND_STATUS_UNAVAILABLE)
    list_done_cb (backend, printer_list);  
}

static gboolean
list_printers_remove_backend (PrinterList     *printer_list,
                              BobguiPrintBackend *backend)
{
  printer_list->backends = g_list_remove (printer_list->backends, backend);
  bobgui_print_backend_destroy (backend);
  g_object_unref (backend);

  if (printer_list->backends == NULL)
    {
      free_printer_list (printer_list);
      return TRUE;
    }

  return FALSE;
}

static void
list_done_cb (BobguiPrintBackend *backend,
	      PrinterList     *printer_list)
{
  g_signal_handlers_disconnect_by_func (backend, list_added_cb, printer_list);
  g_signal_handlers_disconnect_by_func (backend, list_done_cb, printer_list);
  g_signal_handlers_disconnect_by_func (backend, backend_status_changed, printer_list);

  list_printers_remove_backend(printer_list, backend);
}

static gboolean
list_printers_init (PrinterList     *printer_list,
		    BobguiPrintBackend *backend)
{
  GList *list, *node;
  BobguiPrintBackendStatus status;

  list = bobgui_print_backend_get_printer_list (backend);

  for (node = list; node != NULL; node = node->next)
    {
      if (list_added_cb (backend, node->data, printer_list))
        {
          g_list_free (list);
          return TRUE;
        }
    }

  g_list_free (list);

  g_object_get (backend, "status", &status, NULL);
  
  if (status == BOBGUI_PRINT_BACKEND_STATUS_UNAVAILABLE || 
      bobgui_print_backend_printer_list_is_done (backend))
    {
      if (list_printers_remove_backend (printer_list, backend))
        return TRUE;
    }
  else
    {
      g_signal_connect (backend, "printer-added", 
			(GCallback) list_added_cb, 
			printer_list);
      g_signal_connect (backend, "printer-list-done", 
			(GCallback) list_done_cb, 
			printer_list);
      g_signal_connect (backend, "notify::status", 
                        (GCallback) backend_status_changed,
                        printer_list);     
    }

  return FALSE;
}

/**
 * bobgui_enumerate_printers:
 * @func: a function to call for each printer
 * @data: user data to pass to @func
 * @destroy: function to call if @data is no longer needed
 * @wait: if true, wait in a recursive mainloop until
 *    all printers are enumerated; otherwise return early
 *
 * Calls a function for all printers that are known to BOBGUI.
 *
 * If @func returns true, the enumeration is stopped.
 */
void
bobgui_enumerate_printers (BobguiPrinterFunc func,
			gpointer       data,
			GDestroyNotify destroy,
			gboolean       wait)
{
  PrinterList *printer_list;
  GList *node, *next;
  BobguiPrintBackend *backend;

  printer_list = g_new0 (PrinterList, 1);

  printer_list->func = func;
  printer_list->data = data;
  printer_list->destroy = destroy;

  if (g_module_supported ())
    printer_list->backends = bobgui_print_backend_load_modules ();
  
  if (printer_list->backends == NULL)
    {
      free_printer_list (printer_list);
      return;
    }

  for (node = printer_list->backends; node != NULL; node = next)
    {
      next = node->next;
      backend = BOBGUI_PRINT_BACKEND (node->data);
      if (list_printers_init (printer_list, backend))
        return;
    }

  if (wait && printer_list->backends)
    {
      printer_list->loop = g_main_loop_new (NULL, FALSE);
      g_main_loop_run (printer_list->loop);
    }
}


static BobguiPrinter *found_printer;

static gboolean
match_printer_name (BobguiPrinter *printer,
                    gpointer    data)
{
  if (strcmp (bobgui_printer_get_name (printer), (const char *)data) == 0)
    {
      found_printer = g_object_ref (printer);
      return TRUE;
    }

  return FALSE;
}

BobguiPrinter *
bobgui_printer_find (const char *name)
{
  found_printer = NULL;
  bobgui_enumerate_printers (match_printer_name, (gpointer) name, NULL, TRUE);
  return g_steal_pointer (&found_printer);
}

GType
bobgui_print_capabilities_get_type (void)
{
  static GType etype = 0;

  if (G_UNLIKELY (etype == 0))
    {
      static const GFlagsValue values[] = {
        { BOBGUI_PRINT_CAPABILITY_PAGE_SET, "BOBGUI_PRINT_CAPABILITY_PAGE_SET", "page-set" },
        { BOBGUI_PRINT_CAPABILITY_COPIES, "BOBGUI_PRINT_CAPABILITY_COPIES", "copies" },
        { BOBGUI_PRINT_CAPABILITY_COLLATE, "BOBGUI_PRINT_CAPABILITY_COLLATE", "collate" },
        { BOBGUI_PRINT_CAPABILITY_REVERSE, "BOBGUI_PRINT_CAPABILITY_REVERSE", "reverse" },
        { BOBGUI_PRINT_CAPABILITY_SCALE, "BOBGUI_PRINT_CAPABILITY_SCALE", "scale" },
        { BOBGUI_PRINT_CAPABILITY_GENERATE_PDF, "BOBGUI_PRINT_CAPABILITY_GENERATE_PDF", "generate-pdf" },
        { BOBGUI_PRINT_CAPABILITY_GENERATE_PS, "BOBGUI_PRINT_CAPABILITY_GENERATE_PS", "generate-ps" },
        { BOBGUI_PRINT_CAPABILITY_PREVIEW, "BOBGUI_PRINT_CAPABILITY_PREVIEW", "preview" },
	{ BOBGUI_PRINT_CAPABILITY_NUMBER_UP, "BOBGUI_PRINT_CAPABILITY_NUMBER_UP", "number-up"},
        { BOBGUI_PRINT_CAPABILITY_NUMBER_UP_LAYOUT, "BOBGUI_PRINT_CAPABILITY_NUMBER_UP_LAYOUT", "number-up-layout" },
        { 0, NULL, NULL }
      };

      etype = g_flags_register_static ("BobguiPrintCapabilities", values);
    }

  return etype;
}
