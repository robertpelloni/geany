/*
 * BOBGUI - The Bobgui Framework
 * Copyright (C) 2022 Red Hat, Inc.
 * All rights reserved.
 *
 * This Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "bobguifiledialog.h"

#include "deprecated/bobguidialog.h"
#include "bobguifilechoosernativeprivate.h"
#include "bobguidialogerror.h"
#include <glib/gi18n-lib.h>
#include "gdk/gdkprivate.h"
#include "gdk/gdkdebugprivate.h"
#include "bobguitextencodingprivate.h"

/**
 * BobguiFileDialog:
 *
 * Asynchronous API to present a file chooser dialog.
 *
 * `BobguiFileDialog` collects the arguments that are needed to present
 * the dialog to the user, such as a title for the dialog and whether
 * it should be modal.
 *
 * The dialog is shown with [method@Bobgui.FileDialog.open],
 * [method@Bobgui.FileDialog.save], etc.
 *
 * Since: 4.10
 */

/* {{{ GObject implementation */

struct _BobguiFileDialog
{
  GObject parent_instance;

  char *title;
  char *accept_label;
  unsigned int modal : 1;

  GListModel *filters;
  BobguiFileFilter *default_filter;
  GFile *initial_folder;
  char *initial_name;
  GFile *initial_file;
};

enum
{
  PROP_0,
  PROP_ACCEPT_LABEL,
  PROP_DEFAULT_FILTER,
  PROP_FILTERS,
  PROP_INITIAL_FILE,
  PROP_INITIAL_FOLDER,
  PROP_INITIAL_NAME,
  PROP_MODAL,
  PROP_TITLE,

  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES];

G_DEFINE_TYPE (BobguiFileDialog, bobgui_file_dialog, G_TYPE_OBJECT)

static void
bobgui_file_dialog_init (BobguiFileDialog *self)
{
  self->modal = TRUE;
}

static void
bobgui_file_dialog_finalize (GObject *object)
{
  BobguiFileDialog *self = BOBGUI_FILE_DIALOG (object);

  g_free (self->title);
  g_free (self->accept_label);
  g_clear_object (&self->filters);
  g_clear_object (&self->default_filter);
  g_clear_object (&self->initial_folder);
  g_clear_object (&self->initial_file);
  g_free (self->initial_name);

  G_OBJECT_CLASS (bobgui_file_dialog_parent_class)->finalize (object);
}

static void
bobgui_file_dialog_get_property (GObject      *object,
                              unsigned int  property_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  BobguiFileDialog *self = BOBGUI_FILE_DIALOG (object);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;

    case PROP_MODAL:
      g_value_set_boolean (value, self->modal);
      break;

    case PROP_FILTERS:
      g_value_set_object (value, self->filters);
      break;

    case PROP_DEFAULT_FILTER:
      g_value_set_object (value, self->default_filter);
      break;

    case PROP_INITIAL_FILE:
      g_value_set_object (value, self->initial_file);
      break;

    case PROP_INITIAL_FOLDER:
      g_value_set_object (value, self->initial_folder);
      break;

    case PROP_INITIAL_NAME:
      g_value_set_string (value, self->initial_name);
      break;

    case PROP_ACCEPT_LABEL:
      g_value_set_string (value, self->accept_label);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_file_dialog_set_property (GObject      *object,
                              unsigned int  prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BobguiFileDialog *self = BOBGUI_FILE_DIALOG (object);

  switch (prop_id)
    {
    case PROP_TITLE:
      bobgui_file_dialog_set_title (self, g_value_get_string (value));
      break;

    case PROP_MODAL:
      bobgui_file_dialog_set_modal (self, g_value_get_boolean (value));
      break;

    case PROP_FILTERS:
      bobgui_file_dialog_set_filters (self, g_value_get_object (value));
      break;

    case PROP_DEFAULT_FILTER:
      bobgui_file_dialog_set_default_filter (self, g_value_get_object (value));
      break;

    case PROP_INITIAL_FILE:
      bobgui_file_dialog_set_initial_file (self, g_value_get_object (value));
      break;

    case PROP_INITIAL_FOLDER:
      bobgui_file_dialog_set_initial_folder (self, g_value_get_object (value));
      break;

    case PROP_INITIAL_NAME:
      bobgui_file_dialog_set_initial_name (self, g_value_get_string (value));
      break;

    case PROP_ACCEPT_LABEL:
      bobgui_file_dialog_set_accept_label (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_file_dialog_class_init (BobguiFileDialogClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = bobgui_file_dialog_finalize;
  object_class->get_property = bobgui_file_dialog_get_property;
  object_class->set_property = bobgui_file_dialog_set_property;

  /**
   * BobguiFileDialog:title:
   *
   * A title that may be shown on the file chooser dialog.
   *
   * Since: 4.10
   */
  properties[PROP_TITLE] =
      g_param_spec_string ("title", NULL, NULL,
                           NULL,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFileDialog:modal:
   *
   * Whether the file chooser dialog is modal.
   *
   * Since: 4.10
   */
  properties[PROP_MODAL] =
      g_param_spec_boolean ("modal", NULL, NULL,
                            TRUE,
                            G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFileDialog:filters:
   *
   * The list of filters.
   *
   * See [property@Bobgui.FileDialog:default-filter] about how these
   * two properties interact.
   *
   * Since: 4.10
   */
  properties[PROP_FILTERS] =
      g_param_spec_object ("filters", NULL, NULL,
                           G_TYPE_LIST_MODEL,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFileDialog:default-filter:
   *
   * The default filter.
   *
   * This filter is initially active in the file chooser dialog.
   *
   * If the default filter is `NULL`, the first filter of [property@Bobgui.FileDialog:filters]
   * is used as the default filter. If that property contains no filter, the dialog will
   * be unfiltered.
   *
   * If [property@Bobgui.FileDialog:filters] is not `NULL`, the default filter should be
   * part of the list. If it is not, the dialog may choose to not make it available.
   *
   * Since: 4.10
   */
  properties[PROP_DEFAULT_FILTER] =
      g_param_spec_object ("default-filter", NULL, NULL,
                           BOBGUI_TYPE_FILE_FILTER,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFileDialog:initial-file:
   *
   * The initial file.
   *
   * This file is initially selected in the file chooser dialog
   *
   * This is a utility property that sets both [property@Bobgui.FileDialog:initial-folder]
   * and [property@Bobgui.FileDialog:initial-name].
   *
   * Since: 4.10
   */
  properties[PROP_INITIAL_FILE] =
      g_param_spec_object ("initial-file", NULL, NULL,
                           G_TYPE_FILE,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFileDialog:initial-folder:
   *
   * The initial folder.
   *
   * This is the directory that is initially opened in the file chooser dialog.
   *
   * Since: 4.10
   */
  properties[PROP_INITIAL_FOLDER] =
      g_param_spec_object ("initial-folder", NULL, NULL,
                           G_TYPE_FILE,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFileDialog:initial-name:
   *
   * The initial name.
   *
   * This is the name of the file that is initially selected in the file chooser dialog.
   *
   * Since: 4.10
   */
  properties[PROP_INITIAL_NAME] =
      g_param_spec_string ("initial-name", NULL, NULL,
                           NULL,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFileDialog:accept-label:
   *
   * Label for the file chooser's accept button.
   *
   * Since: 4.10
   */
  properties[PROP_ACCEPT_LABEL] =
      g_param_spec_string ("accept-label", NULL, NULL,
                           NULL,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);
}

/* }}} */
/* {{{ Utilities */

static void
file_chooser_set_filters (BobguiFileChooser *chooser,
                          GListModel     *filters)
{
  if (!filters)
    return;

  for (unsigned int i = 0; i < g_list_model_get_n_items (filters); i++)
    {
      BobguiFileFilter *filter = g_list_model_get_item (filters, i);
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      bobgui_file_chooser_add_filter (chooser, filter);
G_GNUC_END_IGNORE_DEPRECATIONS
      g_object_unref (filter);
    }
}

/* }}} */
/* {{{ API: Constructor */

/**
 * bobgui_file_dialog_new:
 *
 * Creates a new `BobguiFileDialog` object.
 *
 * Returns: the new `BobguiFileDialog`
 *
 * Since: 4.10
 */
BobguiFileDialog *
bobgui_file_dialog_new (void)
{
  return g_object_new (BOBGUI_TYPE_FILE_DIALOG, NULL);
}

/* }}} */
/* {{{ API: Getters and setters */

/**
 * bobgui_file_dialog_get_title:
 * @self: a file dialog
 *
 * Returns the title that will be shown on the file chooser dialog.
 *
 * Returns: the title
 *
 * Since: 4.10
 */
const char *
bobgui_file_dialog_get_title (BobguiFileDialog *self)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_DIALOG (self), NULL);

  return self->title;
}

/**
 * bobgui_file_dialog_set_title:
 * @self: a file dialog
 * @title: the new title
 *
 * Sets the title that will be shown on the file chooser dialog.
 *
 * Since: 4.10
 */
void
bobgui_file_dialog_set_title (BobguiFileDialog *self,
                           const char    *title)
{
  char *new_title;

  g_return_if_fail (BOBGUI_IS_FILE_DIALOG (self));
  g_return_if_fail (title != NULL);

  if (g_strcmp0 (self->title, title) == 0)
    return;

  new_title = g_strdup (title);
  g_free (self->title);
  self->title = new_title;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TITLE]);
}

/**
 * bobgui_file_dialog_get_modal:
 * @self: a file dialog
 *
 * Returns whether the file chooser dialog blocks interaction
 * with the parent window while it is presented.
 *
 * Returns: true if the file chooser dialog is modal
 *
 * Since: 4.10
 */
gboolean
bobgui_file_dialog_get_modal (BobguiFileDialog *self)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_DIALOG (self), TRUE);

  return self->modal;
}

/**
 * bobgui_file_dialog_set_modal:
 * @self: a file dialog
 * @modal: the new value
 *
 * Sets whether the file chooser dialog blocks interaction
 * with the parent window while it is presented.
 *
 * Since: 4.10
 */
void
bobgui_file_dialog_set_modal (BobguiFileDialog *self,
                           gboolean       modal)
{
  g_return_if_fail (BOBGUI_IS_FILE_DIALOG (self));

  if (self->modal == modal)
    return;

  self->modal = modal;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODAL]);
}

/**
 * bobgui_file_dialog_get_filters:
 * @self: a file dialog
 *
 * Gets the filters that will be offered to the user
 * in the file chooser dialog.
 *
 * Returns: (transfer none) (nullable): the filters,
 *   as a list model of [class@Bobgui.FileFilter]
 *
 * Since: 4.10
 */
GListModel *
bobgui_file_dialog_get_filters (BobguiFileDialog *self)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_DIALOG (self), NULL);

  return self->filters;
}

/**
 * bobgui_file_dialog_set_filters:
 * @self: a file dialog
 * @filters: (nullable): a list model of [class@Bobgui.FileFilter]
 *
 * Sets the filters that will be offered to the user
 * in the file chooser dialog.
 *
 * Since: 4.10
 */
void
bobgui_file_dialog_set_filters (BobguiFileDialog *self,
                             GListModel    *filters)
{
  g_return_if_fail (BOBGUI_IS_FILE_DIALOG (self));
  g_return_if_fail (filters == NULL || G_IS_LIST_MODEL (filters));

  if (!g_set_object (&self->filters, filters))
    return;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FILTERS]);
}

/**
 * bobgui_file_dialog_get_default_filter:
 * @self: a file dialog
 *
 * Gets the filter that will be selected by default
 * in the file chooser dialog.
 *
 * Returns: (transfer none) (nullable): the default filter
 *
 * Since: 4.10
 */
BobguiFileFilter *
bobgui_file_dialog_get_default_filter (BobguiFileDialog *self)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_DIALOG (self), NULL);

  return self->default_filter;
}

/**
 * bobgui_file_dialog_set_default_filter:
 * @self: a file dialog
 * @filter: (nullable): the file filter
 *
 * Sets the filter that will be selected by default
 * in the file chooser dialog.
 *
 * If set to `NULL`, the first item in [property@Bobgui.FileDialog:filters]
 * will be used as the default filter. If that list is empty, the dialog
 * will be unfiltered.
 *
 * Since: 4.10
 */
void
bobgui_file_dialog_set_default_filter (BobguiFileDialog *self,
                                    BobguiFileFilter *filter)
{
  g_return_if_fail (BOBGUI_IS_FILE_DIALOG (self));
  g_return_if_fail (filter == NULL || BOBGUI_IS_FILE_FILTER (filter));

  if (!g_set_object (&self->default_filter, filter))
    return;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_DEFAULT_FILTER]);
}

/**
 * bobgui_file_dialog_get_initial_folder:
 * @self: a file dialog
 *
 * Gets the folder that will be set as the
 * initial folder in the file chooser dialog.
 *
 * Returns: (nullable) (transfer none): the folder
 *
 * Since: 4.10
 */
GFile *
bobgui_file_dialog_get_initial_folder (BobguiFileDialog *self)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_DIALOG (self), NULL);

  return self->initial_folder;
}

/**
 * bobgui_file_dialog_set_initial_folder:
 * @self: a file dialog
 * @folder: (nullable): a file
 *
 * Sets the folder that will be set as the
 * initial folder in the file chooser dialog.
 *
 * Since: 4.10
 */
void
bobgui_file_dialog_set_initial_folder (BobguiFileDialog *self,
                                    GFile         *folder)
{
  g_return_if_fail (BOBGUI_IS_FILE_DIALOG (self));
  g_return_if_fail (folder == NULL || G_IS_FILE (folder));

  if (!g_set_object (&self->initial_folder, folder))
    return;

  if (self->initial_name && self->initial_folder)
    {
      g_clear_object (&self->initial_file);
      self->initial_file = g_file_get_child_for_display_name (self->initial_folder,
                                                              self->initial_name,
                                                              NULL);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INITIAL_FILE]);
    }
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INITIAL_FOLDER]);
}

/**
 * bobgui_file_dialog_get_initial_name:
 * @self: a file dialog
 *
 * Gets the filename that will be initially selected.
 *
 * Returns: (nullable) (transfer none): the name
 *
 * Since: 4.10
 */
const char *
bobgui_file_dialog_get_initial_name (BobguiFileDialog *self)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_DIALOG (self), NULL);

  return self->initial_name;
}

/**
 * bobgui_file_dialog_set_initial_name:
 * @self: a file dialog
 * @name: (nullable): a string
 *
 * Sets the filename that will be initially selected.
 *
 * For save dialogs, @name will usually be pre-entered into the
 * name field.
 *
 * If a file with this name already exists in the directory set
 * via [property@Bobgui.FileDialog:initial-folder], the dialog will
 * preselect it.
 *
 * Since: 4.10
 */
void
bobgui_file_dialog_set_initial_name (BobguiFileDialog *self,
                                  const char    *name)
{
  g_return_if_fail (BOBGUI_IS_FILE_DIALOG (self));

  if (!g_set_str (&self->initial_name, name))
    return;

  if (self->initial_name && self->initial_folder)
    {
      g_clear_object (&self->initial_file);
      self->initial_file = g_file_get_child_for_display_name (self->initial_folder,
                                                              self->initial_name,
                                                              NULL);
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INITIAL_FILE]);
    }
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INITIAL_NAME]);
}

/**
 * bobgui_file_dialog_get_initial_file:
 * @self: a file dialog
 *
 * Gets the file that will be initially selected in
 * the file chooser dialog.
 *
 * Returns: (nullable) (transfer none): the file
 *
 * Since: 4.10
 */
GFile *
bobgui_file_dialog_get_initial_file (BobguiFileDialog *self)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_DIALOG (self), NULL);

  return self->initial_file;
}

/**
 * bobgui_file_dialog_set_initial_file:
 * @self: a file dialog
 * @file: (nullable): a file
 *
 * Sets the file that will be initially selected in
 * the file chooser dialog.
 *
 * This function is a shortcut for calling both
 * [method@Bobgui.FileDialog.set_initial_folder] and
 * [method@Bobgui.FileDialog.set_initial_name] with the
 * directory and name of @file, respectively.
 *
 * Since: 4.10
 */
void
bobgui_file_dialog_set_initial_file (BobguiFileDialog *self,
                                  GFile         *file)
{
  g_return_if_fail (BOBGUI_IS_FILE_DIALOG (self));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  g_object_freeze_notify (G_OBJECT (self));

  if (file != NULL)
    {
      GFile *folder;
      GFileInfo *info;

      if (self->initial_file && g_file_equal (self->initial_file, file))
        return;

      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INITIAL_FILE]);

      folder = g_file_get_parent (file);
      if (folder == NULL)
        goto invalid_file;

      if (g_set_object (&self->initial_folder, folder))
        g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INITIAL_FOLDER]);

      info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_EDIT_NAME, 0, NULL, NULL);
      if (info && g_file_info_get_edit_name (info) != NULL)
        {
          if (g_set_str (&self->initial_name, g_file_info_get_edit_name (info)))
            g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INITIAL_NAME]);
        }
      else
        {
          char *relative, *name;

          relative = g_file_get_relative_path (folder, file);
          name = g_filename_display_name (relative);
          if (g_set_str (&self->initial_name, name))
            g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INITIAL_NAME]);

          g_free (name);
          g_free (relative);
        }
      g_clear_object (&info);
      g_object_unref (folder);
    }
  else
    {
invalid_file:
      if (g_set_object (&self->initial_file, NULL))
        g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INITIAL_FILE]);
      if (g_set_object (&self->initial_folder, NULL))
        g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INITIAL_FOLDER]);
      if (g_set_str (&self->initial_name, NULL))
        g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_INITIAL_NAME]);
    }

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_file_dialog_get_accept_label:
 * @self: a file dialog
 *
 * Retrieves the text used by the dialog on its accept button.
 *
 * Returns: (nullable): the label shown on the file chooser's accept button
 *
 * Since: 4.10
 */
const char *
bobgui_file_dialog_get_accept_label (BobguiFileDialog *self)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_DIALOG (self), NULL);

  return self->accept_label;
}

/**
 * bobgui_file_dialog_set_accept_label:
 * @self: a file dialog
 * @accept_label: (nullable): the new accept label
 *
 * Sets the label shown on the file chooser's accept button.
 *
 * Leaving the accept label unset or setting it as `NULL` will
 * fall back to a default label, depending on what API is used
 * to launch the file dialog.
 *
 * Since: 4.10
 */
void
bobgui_file_dialog_set_accept_label (BobguiFileDialog *self,
                                  const char    *accept_label)
{
  g_return_if_fail (BOBGUI_IS_FILE_DIALOG (self));

  if (g_set_str (&self->accept_label, accept_label))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACCEPT_LABEL]);
}

/* }}} */
/* {{{ Async implementation */

static void response_cb (GTask *task,
                         int    response);

static void
cancelled_cb (GCancellable *cancellable,
              GTask        *task)
{
  response_cb (task, BOBGUI_RESPONSE_CLOSE);
}

typedef struct {
  GListModel *files;
  char **choices;
} TaskResult;

static void
task_result_free (gpointer data)
{
  TaskResult *res = data;

  g_object_unref (res->files);
  g_strfreev (res->choices);
  g_free (res);
}

static void
response_cb (GTask *task,
             int    response)
{
  GCancellable *cancellable;

  cancellable = g_task_get_cancellable (task);

  if (cancellable)
    g_signal_handlers_disconnect_by_func (cancellable, cancelled_cb, task);

  if (response == BOBGUI_RESPONSE_ACCEPT)
    {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      BobguiFileChooser *chooser;
      TaskResult *res;
      char **choices;

      res = g_new0 (TaskResult, 1);

      chooser = BOBGUI_FILE_CHOOSER (g_task_get_task_data (task));
      res->files = bobgui_file_chooser_get_files (chooser);

      choices = (char **) g_object_get_data (G_OBJECT (chooser), "choices");
      if (choices)
        {
          res->choices = g_new0 (char *, g_strv_length (choices) + 1);
          for (guint i = 0; choices[i]; i++)
            res->choices[i] = g_strdup (bobgui_file_chooser_get_choice (chooser, choices[i]));
        }

      g_task_return_pointer (task, res, task_result_free);
G_GNUC_END_IGNORE_DEPRECATIONS
    }
  else if (response == BOBGUI_RESPONSE_CLOSE)
    g_task_return_new_error (task, BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_CANCELLED, "Cancelled by application");
  else if (response == BOBGUI_RESPONSE_CANCEL ||
           response == BOBGUI_RESPONSE_DELETE_EVENT)
    g_task_return_new_error (task, BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_DISMISSED, "Dismissed by user");
  else
    g_task_return_new_error (task, BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED, "Unknown failure (%d)", response);

  bobgui_native_dialog_destroy (BOBGUI_NATIVE_DIALOG (g_task_get_task_data (task)));

  g_object_unref (task);
}

static void
dialog_response (BobguiDialog *dialog,
                 int        response,
                 GTask     *task)
{
  response_cb (task, response);
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static BobguiFileChooserNative *
create_file_chooser (BobguiFileDialog        *self,
                     BobguiWindow            *parent,
                     BobguiFileChooserAction  action,
                     gboolean              select_multiple)
{
  BobguiFileChooserNative *chooser;
  const char *default_accept_label, *accept;
  const char *default_title, *title;

  switch (action)
    {
    case BOBGUI_FILE_CHOOSER_ACTION_OPEN:
      default_accept_label = _("_Open");
      default_title = select_multiple ? _("Pick Files") : _("Pick a File");
      break;

    case BOBGUI_FILE_CHOOSER_ACTION_SAVE:
      default_accept_label = _("_Save");
      default_title = _("Save a File");
      break;

    case BOBGUI_FILE_CHOOSER_ACTION_SELECT_FOLDER:
      default_accept_label = _("_Select");
      default_title = select_multiple ? _("Select Folders") : _("Select a Folder");
      break;

    default:
      g_assert_not_reached ();
    }

  if (self->title)
    title = self->title;
  else
    title = default_title;

  if (self->accept_label)
    accept = self->accept_label;
  else
    accept = default_accept_label;

  chooser = bobgui_file_chooser_native_new (title, parent, action, accept, _("_Cancel"));

  bobgui_native_dialog_set_modal (BOBGUI_NATIVE_DIALOG (chooser), self->modal);
  bobgui_file_chooser_set_select_multiple (BOBGUI_FILE_CHOOSER (chooser), select_multiple);

  file_chooser_set_filters (BOBGUI_FILE_CHOOSER (chooser), self->filters);
  if (self->default_filter)
    bobgui_file_chooser_set_filter (BOBGUI_FILE_CHOOSER (chooser), self->default_filter);
  else if (self->filters)
    {
      BobguiFileFilter *filter = g_list_model_get_item (self->filters, 0);
      if (filter)
        {
          bobgui_file_chooser_set_filter (BOBGUI_FILE_CHOOSER (chooser), filter);
          g_object_unref (filter);
        }
    }

  if (self->initial_folder)
    bobgui_file_chooser_set_current_folder (BOBGUI_FILE_CHOOSER (chooser), self->initial_folder, NULL);
  if (self->initial_name && action == BOBGUI_FILE_CHOOSER_ACTION_SAVE)
    bobgui_file_chooser_set_current_name (BOBGUI_FILE_CHOOSER (chooser), self->initial_name);

  return chooser;
}
G_GNUC_END_IGNORE_DEPRECATIONS

static GFile *
finish_file_op (BobguiFileDialog   *self,
                GTask           *task,
                char          ***choices,
                GError         **error)
{
  TaskResult *res;

  res = g_task_propagate_pointer (task, error);
  if (res)
    {
      GFile *file = NULL;

      if (g_list_model_get_n_items (res->files) > 0)
        file = g_list_model_get_item (res->files, 0);
      else
        g_set_error_literal (error,
                             BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED,
                             "No file selected");

      if (choices)
        *choices = g_strdupv (res->choices);

      task_result_free (res);

      return file;
    }

  return NULL;
}

static GListModel *
finish_multiple_files_op (BobguiFileDialog   *self,
                          GTask           *task,
                          char          ***choices,
                          GError         **error)
{
  TaskResult *res;

  res = g_task_propagate_pointer (task, error);

  if (res)
    {
      GListModel *files;

      files = G_LIST_MODEL (g_object_ref (res->files));

      if (choices)
        *choices = g_strdupv (res->choices);

      task_result_free (res);

      return files;
    }

  return NULL;
}

/* }}} */
/* {{{ Public API */

/**
 * bobgui_file_dialog_open:
 * @self: a file dialog
 * @parent: (nullable): the parent window
 * @cancellable: (nullable): a cancellable to cancel the operation
 * @callback: (scope async) (closure user_data): a callback to call when the
 *   operation is complete
 * @user_data: data to pass to @callback
 *
 * Presents a file chooser dialog to the user.
 *
 * The file chooser dialog will be set up to select a single file.
 *
 * The @callback will be called when the dialog is closed.
 *
 * Since: 4.10
 */
void
bobgui_file_dialog_open (BobguiFileDialog       *self,
                      BobguiWindow           *parent,
                      GCancellable        *cancellable,
                      GAsyncReadyCallback  callback,
                      gpointer             user_data)
{
  BobguiFileChooserNative *chooser;
  GTask *task;

  g_return_if_fail (BOBGUI_IS_FILE_DIALOG (self));

  chooser = create_file_chooser (self, parent, BOBGUI_FILE_CHOOSER_ACTION_OPEN, FALSE);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_check_cancellable (task, FALSE);
  g_task_set_source_tag (task, bobgui_file_dialog_open);
  g_task_set_task_data (task, chooser, g_object_unref);

  if (cancellable)
    g_signal_connect (cancellable, "cancelled", G_CALLBACK (cancelled_cb), task);

  g_signal_connect (chooser, "response", G_CALLBACK (dialog_response), task);

  bobgui_native_dialog_show (BOBGUI_NATIVE_DIALOG (chooser));
}

/**
 * bobgui_file_dialog_open_finish:
 * @self: a file dialog
 * @result: the result
 * @error: return location for a [enum@Bobgui.DialogError] error
 *
 * Finishes the [method@Bobgui.FileDialog.open] call.
 *
 * Note that this function returns a [error@Bobgui.DialogError.DISMISSED]
 * error if the user cancels the dialog.
 *
 * Returns: (transfer full): the file that was selected
 *
 * Since: 4.10
 */
GFile *
bobgui_file_dialog_open_finish (BobguiFileDialog   *self,
                             GAsyncResult    *result,
                             GError         **error)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_DIALOG (self), NULL);
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == bobgui_file_dialog_open, NULL);

  return finish_file_op (self, G_TASK (result), NULL, error);
}

/**
 * bobgui_file_dialog_select_folder:
 * @self: a file dialog
 * @parent: (nullable): the parent window
 * @cancellable: (nullable): a cancellable to cancel the operation
 * @callback: (scope async) (closure user_data): a callback to call when the
 *   operation is complete
 * @user_data: data to pass to @callback
 *
 * Presents a file chooser dialog to the user.
 *
 * The file chooser dialog will be set up to select a single folder.
 *
 * If you pass @initial_folder, the file chooser dialog will initially
 * be opened in the parent directory of that folder, otherwise, it
 * will be in the directory [property@Bobgui.FileDialog:initial-folder].
 *
 * The @callback will be called when the dialog is closed.
 *
 * Since: 4.10
 */
void
bobgui_file_dialog_select_folder (BobguiFileDialog       *self,
                               BobguiWindow           *parent,
                               GCancellable        *cancellable,
                               GAsyncReadyCallback  callback,
                               gpointer             user_data)
{
  BobguiFileChooserNative *chooser;
  GTask *task;

  g_return_if_fail (BOBGUI_IS_FILE_DIALOG (self));

  chooser = create_file_chooser (self, parent, BOBGUI_FILE_CHOOSER_ACTION_SELECT_FOLDER, FALSE);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_check_cancellable (task, FALSE);
  g_task_set_source_tag (task, bobgui_file_dialog_select_folder);
  g_task_set_task_data (task, chooser, g_object_unref);

  if (cancellable)
    g_signal_connect (cancellable, "cancelled", G_CALLBACK (cancelled_cb), task);

  g_signal_connect (chooser, "response", G_CALLBACK (dialog_response), task);

  bobgui_native_dialog_show (BOBGUI_NATIVE_DIALOG (chooser));
}

/**
 * bobgui_file_dialog_select_folder_finish:
 * @self: a file dialog
 * @result: the result
 * @error: return location for a [enum@Bobgui.DialogError] error
 *
 * Finishes the [method@Bobgui.FileDialog.select_folder] call.
 *
 * Note that this function returns a [error@Bobgui.DialogError.DISMISSED]
 * error if the user cancels the dialog.
 *
 * Returns: (transfer full): the folder that was selected
 *
 * Since: 4.10
 */
GFile *
bobgui_file_dialog_select_folder_finish (BobguiFileDialog  *self,
                                      GAsyncResult   *result,
                                      GError        **error)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_DIALOG (self), NULL);
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == bobgui_file_dialog_select_folder, NULL);

  return finish_file_op (self, G_TASK (result), NULL, error);
}

/**
 * bobgui_file_dialog_save:
 * @self: a file dialog
 * @parent: (nullable): the parent window
 * @cancellable: (nullable): a cancellable to cancel the operation
 * @callback: (scope async) (closure user_data): a callback to call when the
 *   operation is complete
 * @user_data: data to pass to @callback
 *
 * Presents a file chooser dialog to the user.
 *
 * The file chooser dialog will be save mode.
 *
 * The @callback will be called when the dialog is closed.
 *
 * Since: 4.10
 */
void
bobgui_file_dialog_save (BobguiFileDialog       *self,
                      BobguiWindow           *parent,
                      GCancellable        *cancellable,
                      GAsyncReadyCallback  callback,
                      gpointer             user_data)
{
  BobguiFileChooserNative *chooser;
  GTask *task;

  g_return_if_fail (BOBGUI_IS_FILE_DIALOG (self));

  chooser = create_file_chooser (self, parent, BOBGUI_FILE_CHOOSER_ACTION_SAVE, FALSE);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_check_cancellable (task, FALSE);
  g_task_set_source_tag (task, bobgui_file_dialog_save);
  g_task_set_task_data (task, chooser, g_object_unref);

  if (cancellable)
    g_signal_connect (cancellable, "cancelled", G_CALLBACK (cancelled_cb), task);

  g_signal_connect (chooser, "response", G_CALLBACK (dialog_response), task);

  bobgui_native_dialog_show (BOBGUI_NATIVE_DIALOG (chooser));
}

/**
 * bobgui_file_dialog_save_finish:
 * @self: a file dialog
 * @result: the result
 * @error: return location for a [enum@Bobgui.DialogError] error
 *
 * Finishes the [method@Bobgui.FileDialog.save] call.
 *
 * Note that this function returns a [error@Bobgui.DialogError.DISMISSED]
 * error if the user cancels the dialog.
 *
 * Returns: (transfer full): the file that was selected
 *
 * Since: 4.10
 */
GFile *
bobgui_file_dialog_save_finish (BobguiFileDialog   *self,
                             GAsyncResult    *result,
                             GError         **error)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_DIALOG (self), NULL);
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == bobgui_file_dialog_save, NULL);

  return finish_file_op (self, G_TASK (result), NULL, error);
}

/**
 * bobgui_file_dialog_open_multiple:
 * @self: a file dialog
 * @parent: (nullable): the parent window
 * @cancellable: (nullable): a cancellable to cancel the operation
 * @callback: (scope async) (closure user_data): a callback to call when the
 *   operation is complete
 * @user_data: data to pass to @callback
 *
 * Presents a file chooser dialog to the user.
 *
 * The file chooser dialog will be set up to select multiple files.
 *
 * The file chooser dialog will initially be opened in the directory
 * [property@Bobgui.FileDialog:initial-folder].
 *
 * The @callback will be called when the dialog is closed.
 *
 * Since: 4.10
 */
void
bobgui_file_dialog_open_multiple (BobguiFileDialog       *self,
                               BobguiWindow           *parent,
                               GCancellable        *cancellable,
                               GAsyncReadyCallback  callback,
                               gpointer             user_data)
{
  BobguiFileChooserNative *chooser;
  GTask *task;

  g_return_if_fail (BOBGUI_IS_FILE_DIALOG (self));

  chooser = create_file_chooser (self, parent, BOBGUI_FILE_CHOOSER_ACTION_OPEN, TRUE);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_check_cancellable (task, FALSE);
  g_task_set_source_tag (task, bobgui_file_dialog_open_multiple);
  g_task_set_task_data (task, chooser, g_object_unref);

  if (cancellable)
    g_signal_connect (cancellable, "cancelled", G_CALLBACK (cancelled_cb), task);

  g_signal_connect (chooser, "response", G_CALLBACK (dialog_response), task);

  bobgui_native_dialog_show (BOBGUI_NATIVE_DIALOG (chooser));
}

/**
 * bobgui_file_dialog_open_multiple_finish:
 * @self: a file dialog
 * @result: the result
 * @error: return location for a [enum@Bobgui.DialogError] error
 *
 * Finishes the [method@Bobgui.FileDialog.open] call.
 *
 * Note that this function returns a [error@Bobgui.DialogError.DISMISSED]
 * error if the user cancels the dialog.
 *
 * Returns: (transfer full): the files that were selected,
 *   as a list model of [iface@Gio.File]
 *
 * Since: 4.10
 */
GListModel *
bobgui_file_dialog_open_multiple_finish (BobguiFileDialog   *self,
                                      GAsyncResult    *result,
                                      GError         **error)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_DIALOG (self), NULL);
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == bobgui_file_dialog_open_multiple, NULL);

  return finish_multiple_files_op (self, G_TASK (result), NULL, error);
}

/**
 * bobgui_file_dialog_select_multiple_folders:
 * @self: a file dialog
 * @parent: (nullable): the parent window
 * @cancellable: (nullable): a cancellable to cancel the operation
 * @callback: (scope async) (closure user_data): a callback to call when the
 *   operation is complete
 * @user_data: data to pass to @callback
 *
 * Presents a file chooser dialog to the user.
 *
 * The file chooser dialog will be set up to allow selecting
 * multiple folders.
 *
 * The file chooser dialog will initially be opened in the
 * directory [property@Bobgui.FileDialog:initial-folder].
 *
 * The @callback will be called when the dialog is closed.
 *
 * Since: 4.10
 */
void
bobgui_file_dialog_select_multiple_folders (BobguiFileDialog       *self,
                                         BobguiWindow           *parent,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             user_data)
{
  BobguiFileChooserNative *chooser;
  GTask *task;

  g_return_if_fail (BOBGUI_IS_FILE_DIALOG (self));

  chooser = create_file_chooser (self, parent, BOBGUI_FILE_CHOOSER_ACTION_SELECT_FOLDER, TRUE);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_check_cancellable (task, FALSE);
  g_task_set_source_tag (task, bobgui_file_dialog_select_multiple_folders);
  g_task_set_task_data (task, chooser, g_object_unref);

  if (cancellable)
    g_signal_connect (cancellable, "cancelled", G_CALLBACK (cancelled_cb), task);

  g_signal_connect (chooser, "response", G_CALLBACK (dialog_response), task);

  bobgui_native_dialog_show (BOBGUI_NATIVE_DIALOG (chooser));
}

/**
 * bobgui_file_dialog_select_multiple_folders_finish:
 * @self: a file dialog
 * @result: the result
 * @error: return location for a [enum@Bobgui.DialogError] error
 *
 * Finishes the [method@Bobgui.FileDialog.select_multiple_folders] call.
 *
 * Note that this function returns a [error@Bobgui.DialogError.DISMISSED]
 * error if the user cancels the dialog.
 *
 * Returns: (transfer full): the folders that were selected,
 *   as a list model of [iface@Gio.File]
 *
 * Since: 4.10
 */
GListModel *
bobgui_file_dialog_select_multiple_folders_finish (BobguiFileDialog   *self,
                                                GAsyncResult    *result,
                                                GError         **error)
{
  g_return_val_if_fail (BOBGUI_IS_FILE_DIALOG (self), NULL);
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == bobgui_file_dialog_select_multiple_folders, NULL);

  return finish_multiple_files_op (self, G_TASK (result), NULL, error);
}

/* }}} */
/* {{{ Text file variants */

/**
 * bobgui_file_dialog_open_text_file:
 * @self: a `BobguiFileDialog`
 * @parent: (nullable): the parent `BobguiWindow`
 * @cancellable: (nullable): a `GCancellable` to cancel the operation
 * @callback: (scope async) (closure user_data): a callback to call when the
 *   operation is complete
 * @user_data: data to pass to @callback
 *
 * Initiates a file selection operation by presenting a file chooser
 * dialog to the user.
 *
 * In contrast to [method@Bobgui.FileDialog.open], this function
 * lets the user select the text encoding for the file, if possible.
 *
 * The @callback will be called when the dialog is closed.
 *
 * Since: 4.18
 */
void
bobgui_file_dialog_open_text_file (BobguiFileDialog       *self,
                                BobguiWindow           *parent,
                                GCancellable        *cancellable,
                                GAsyncReadyCallback  callback,
                                gpointer             user_data)
{
  BobguiFileChooserNative *chooser;
  GTask *task;
  char **names;
  char **labels;
  const char **choices;

  g_return_if_fail (BOBGUI_IS_FILE_DIALOG (self));

  chooser = create_file_chooser (self, parent, BOBGUI_FILE_CHOOSER_ACTION_OPEN, FALSE);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  names = bobgui_text_encoding_get_names ();
  labels = bobgui_text_encoding_get_labels ();
  bobgui_file_chooser_add_choice (BOBGUI_FILE_CHOOSER (chooser),
                               "encoding", _("Encoding"),
                               (const char **) names,
                               (const char **) labels);
  bobgui_file_chooser_set_choice (BOBGUI_FILE_CHOOSER (chooser),
                               "encoding", "automatic");
  g_free (names);
  g_free (labels);
G_GNUC_END_IGNORE_DEPRECATIONS

  choices = g_new0 (const char *, 2);
  choices[0] = "encoding";
  g_object_set_data_full (G_OBJECT (chooser), "choices", choices, g_free);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_check_cancellable (task, FALSE);
  g_task_set_source_tag (task, bobgui_file_dialog_open_text_file);
  g_task_set_task_data (task, chooser, g_object_unref);

  if (cancellable)
    g_signal_connect (cancellable, "cancelled", G_CALLBACK (cancelled_cb), task);

  g_signal_connect (chooser, "response", G_CALLBACK (dialog_response), task);

  bobgui_native_dialog_show (BOBGUI_NATIVE_DIALOG (chooser));
}

/**
 * bobgui_file_dialog_open_text_file_finish:
 * @self: a `BobguiFileDialog`
 * @result: a `GAsyncResult`
 * @encoding: (out) (transfer none): return location for the text encoding to use
 * @error: return location for a [enum@Bobgui.DialogError] error
 *
 * Finishes the [method@Bobgui.FileDialog.open_text_file] call
 * and returns the resulting file and text encoding.
 *
 * If the user has explicitly selected a text encoding to use
 * for the file, then @encoding will be set to a codeset name that
 * is suitable for passing to iconv_open(). Otherwise, it will
 * be `NULL`.
 *
 * Note that this function returns a [error@Bobgui.DialogError.DISMISSED]
 * error if the user cancels the dialog.
 *
 * Returns: (transfer full): the file that was selected
 *
 * Since: 4.18
 */
GFile *
bobgui_file_dialog_open_text_file_finish (BobguiFileDialog  *self,
                                       GAsyncResult   *result,
                                       const char    **encoding,
                                       GError        **error)
{
  char **choices = NULL;
  GFile *file;

  g_return_val_if_fail (BOBGUI_IS_FILE_DIALOG (self), NULL);
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == bobgui_file_dialog_open_text_file, NULL);

  file = finish_file_op (self, G_TASK (result), &choices, error);

  if (choices)
    {
      *encoding = bobgui_text_encoding_from_name (choices[0]);
      g_strfreev (choices);
    }
  else
    {
      *encoding = NULL;
    }

  return file;
}

/**
 * bobgui_file_dialog_open_multiple_text_files:
 * @self: a file dialog
 * @parent: (nullable): the parent window
 * @cancellable: (nullable): a cancellable to cancel the operation
 * @callback: (scope async) (closure user_data): a callback to call when the
 *   operation is complete
 * @user_data: data to pass to @callback
 *
 * Presents a file chooser dialog to the user.
 *
 * The file chooser dialog will be set up to select multiple files.
 *
 * The file chooser dialog will initially be opened in the directory
 * [property@Bobgui.FileDialog:initial-folder].
 *
 * In contrast to [method@Bobgui.FileDialog.open], this function
 * lets the user select the text encoding for the files, if possible.
 *
 * The @callback will be called when the dialog is closed.
 *
 * Since: 4.18
 */
void
bobgui_file_dialog_open_multiple_text_files (BobguiFileDialog       *self,
                                          BobguiWindow           *parent,
                                          GCancellable        *cancellable,
                                          GAsyncReadyCallback  callback,
                                          gpointer             user_data)
{
  BobguiFileChooserNative *chooser;
  GTask *task;
  char **names;
  char **labels;
  const char **choices;

  g_return_if_fail (BOBGUI_IS_FILE_DIALOG (self));

  chooser = create_file_chooser (self, parent, BOBGUI_FILE_CHOOSER_ACTION_OPEN, TRUE);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  names = bobgui_text_encoding_get_names ();
  labels = bobgui_text_encoding_get_labels ();
  bobgui_file_chooser_add_choice (BOBGUI_FILE_CHOOSER (chooser),
                               "encoding", _("Encoding"),
                               (const char **) names,
                               (const char **) labels);
  bobgui_file_chooser_set_choice (BOBGUI_FILE_CHOOSER (chooser),
                               "encoding", "automatic");
  g_free (names);
  g_free (labels);
G_GNUC_END_IGNORE_DEPRECATIONS

  choices = g_new0 (const char *, 2);
  choices[0] = "encoding";
  g_object_set_data_full (G_OBJECT (chooser), "choices", choices, g_free);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_check_cancellable (task, FALSE);
  g_task_set_source_tag (task, bobgui_file_dialog_open_multiple_text_files);
  g_task_set_task_data (task, chooser, g_object_unref);

  if (cancellable)
    g_signal_connect (cancellable, "cancelled", G_CALLBACK (cancelled_cb), task);

  g_signal_connect (chooser, "response", G_CALLBACK (dialog_response), task);

  bobgui_native_dialog_show (BOBGUI_NATIVE_DIALOG (chooser));
}

/**
 * bobgui_file_dialog_open_multiple_text_files_finish:
 * @self: a file dialog
 * @result: the result
 * @encoding: (out) (transfer none): return location for the text encoding to use
 * @error: return location for a [enum@Bobgui.DialogError] error
 *
 * Finishes the [method@Bobgui.FileDialog.open] call.
 *
 * Note that this function returns a [error@Bobgui.DialogError.DISMISSED]
 * error if the user cancels the dialog.
 *
 * Returns: (transfer full): the files that were selected,
 *   as a list model of [iface@Gio.File]
 *
 * Since: 4.18
 */
GListModel *
bobgui_file_dialog_open_multiple_text_files_finish (BobguiFileDialog   *self,
                                                 GAsyncResult    *result,
                                                 const char     **encoding,
                                                 GError         **error)
{
  char **choices = NULL;
  GListModel *files;

  g_return_val_if_fail (BOBGUI_IS_FILE_DIALOG (self), NULL);
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == bobgui_file_dialog_open_multiple_text_files, NULL);

  files = finish_multiple_files_op (self, G_TASK (result), &choices, error);

  if (choices)
    {
      *encoding = bobgui_text_encoding_from_name (choices[0]);
      g_strfreev (choices);
    }
  else
    {
      *encoding = NULL;
    }

  return files;
}

/**
 * bobgui_file_dialog_save_text_file:
 * @self: a `BobguiFileDialog`
 * @parent: (nullable): the parent `BobguiWindow`
 * @cancellable: (nullable): a `GCancellable` to cancel the operation
 * @callback: (scope async) (closure user_data): a callback to call when the
 *   operation is complete
 * @user_data: data to pass to @callback
 *
 * Initiates a file save operation by presenting a file chooser
 * dialog to the user.
 *
 * In contrast to [method@Bobgui.FileDialog.save], this function
 * lets the user select the text encoding and line endings for
 * the text file, if possible.
 *
 * The @callback will be called when the dialog is closed.
 *
 * Since: 4.18
 */
void
bobgui_file_dialog_save_text_file (BobguiFileDialog       *self,
                                BobguiWindow           *parent,
                                GCancellable        *cancellable,
                                GAsyncReadyCallback  callback,
                                gpointer             user_data)
{
  BobguiFileChooserNative *chooser;
  GTask *task;
  char **names;
  char **labels;
  const char **choices;

  g_return_if_fail (BOBGUI_IS_FILE_DIALOG (self));

  chooser = create_file_chooser (self, parent, BOBGUI_FILE_CHOOSER_ACTION_SAVE, FALSE);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  names = bobgui_text_encoding_get_names ();
  labels = bobgui_text_encoding_get_labels ();
  bobgui_file_chooser_add_choice (BOBGUI_FILE_CHOOSER (chooser),
                               "encoding", _("Encoding"),
                               (const char **) names,
                               (const char **) labels);
  bobgui_file_chooser_set_choice (BOBGUI_FILE_CHOOSER (chooser),
                               "encoding", "automatic");
  g_free (names);
  g_free (labels);

  names = bobgui_line_ending_get_names ();
  labels = bobgui_line_ending_get_labels ();
  bobgui_file_chooser_add_choice (BOBGUI_FILE_CHOOSER (chooser),
                               "line_ending", _("Line Ending"),
                               (const char **) names,
                               (const char **) labels);
  bobgui_file_chooser_set_choice (BOBGUI_FILE_CHOOSER (chooser),
                               "line_ending", "as-is");

  g_free (names);
  g_free (labels);

G_GNUC_END_IGNORE_DEPRECATIONS

  choices = g_new0 (const char *, 3);
  choices[0] = "encoding";
  choices[1] = "line_ending";
  g_object_set_data_full (G_OBJECT (chooser), "choices", choices, g_free);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_check_cancellable (task, FALSE);
  g_task_set_source_tag (task, bobgui_file_dialog_save_text_file);
  g_task_set_task_data (task, chooser, g_object_unref);

  if (cancellable)
    g_signal_connect (cancellable, "cancelled", G_CALLBACK (cancelled_cb), task);

  g_signal_connect (chooser, "response", G_CALLBACK (dialog_response), task);

  bobgui_native_dialog_show (BOBGUI_NATIVE_DIALOG (chooser));
}

/**
 * bobgui_file_dialog_save_text_file_finish:
 * @self: a `BobguiFileDialog`
 * @result: a `GAsyncResult`
 * @encoding: (out) (transfer none): return location for the text encoding to use
 * @line_ending: (out) (transfer none): return location for the line endings to use
 * @error: return location for a [enum@Bobgui.DialogError] error
 *
 * Finishes the [method@Bobgui.FileDialog.save_text_file] call
 * and returns the resulting file, text encoding and line endings.
 *
 * If the user has explicitly selected a text encoding to use
 * for the file, then @encoding will be set to a codeset name that
 * is suitable for passing to iconv_open(). Otherwise, it will
 * be `NULL`.
 *
 * The @line_ending will be set to one of "\n", "\r\n", "\r" or "",
 * where the latter means to preserve existing line endings.
 *
 * Note that this function returns a [error@Bobgui.DialogError.DISMISSED]
 * error if the user cancels the dialog.
 *
 * Returns: (transfer full): the file that was selected.
 *
 * Since: 4.18
 */
GFile *
bobgui_file_dialog_save_text_file_finish (BobguiFileDialog  *self,
                                       GAsyncResult   *result,
                                       const char    **encoding,
                                       const char    **line_ending,
                                       GError        **error)
{
  char **choices = NULL;
  GFile *file;

  g_return_val_if_fail (BOBGUI_IS_FILE_DIALOG (self), NULL);
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == bobgui_file_dialog_save_text_file, NULL);

  file = finish_file_op (self, G_TASK (result), &choices, error);

  if (choices)
    {
      *encoding = bobgui_text_encoding_from_name (choices[0]);
      *line_ending = bobgui_line_ending_from_name (choices[1]);
      g_strfreev (choices);
    }
  else
    {
      *encoding = NULL;
      *line_ending = "\n";
    }

  return file;
}

/* }}} */

/* vim:set foldmethod=marker: */
