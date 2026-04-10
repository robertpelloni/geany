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

#include "bobguicolordialog.h"

#include "deprecated/bobguicolorchooserdialog.h"
#include "deprecated/bobguicolorchooser.h"
#include "bobguibutton.h"
#include "bobguidialogerror.h"
#include <glib/gi18n-lib.h>

/**
 * BobguiColorDialog:
 *
 * Asynchronous API to present a color chooser dialog.
 *
 * `BobguiColorDialog` collects the arguments that are needed to present
 * the dialog to the user, such as a title for the dialog and whether
 * it should be modal.
 *
 * The dialog is shown with the [method@Bobgui.ColorDialog.choose_rgba]
 * function.
 *
 * See [class@Bobgui.ColorDialogButton] for a convenient control
 * that uses `BobguiColorDialog` and presents the results.
 *
 * Since: 4.10
 */
/* {{{ GObject implementation */

struct _BobguiColorDialog
{
  GObject parent_instance;

  char *title;

  unsigned int modal : 1;
  unsigned int with_alpha : 1;
};

enum
{
  PROP_TITLE = 1,
  PROP_MODAL,
  PROP_WITH_ALPHA,

  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES];

G_DEFINE_TYPE (BobguiColorDialog, bobgui_color_dialog, G_TYPE_OBJECT)

static void
bobgui_color_dialog_init (BobguiColorDialog *self)
{
  self->modal = TRUE;
  self->with_alpha = TRUE;
}

static void
bobgui_color_dialog_finalize (GObject *object)
{
  BobguiColorDialog *self = BOBGUI_COLOR_DIALOG (object);

  g_free (self->title);

  G_OBJECT_CLASS (bobgui_color_dialog_parent_class)->finalize (object);
}

static void
bobgui_color_dialog_get_property (GObject      *object,
                               unsigned int  property_id,
                               GValue       *value,
                               GParamSpec   *pspec)
{
  BobguiColorDialog *self = BOBGUI_COLOR_DIALOG (object);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;

    case PROP_MODAL:
      g_value_set_boolean (value, self->modal);
      break;

    case PROP_WITH_ALPHA:
      g_value_set_boolean (value, self->with_alpha);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_color_dialog_set_property (GObject      *object,
                               unsigned int  prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  BobguiColorDialog *self = BOBGUI_COLOR_DIALOG (object);

  switch (prop_id)
    {
    case PROP_TITLE:
      bobgui_color_dialog_set_title (self, g_value_get_string (value));
      break;

    case PROP_MODAL:
      bobgui_color_dialog_set_modal (self, g_value_get_boolean (value));
      break;

    case PROP_WITH_ALPHA:
      bobgui_color_dialog_set_with_alpha (self, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_color_dialog_class_init (BobguiColorDialogClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = bobgui_color_dialog_finalize;
  object_class->get_property = bobgui_color_dialog_get_property;
  object_class->set_property = bobgui_color_dialog_set_property;

  /**
   * BobguiColorDialog:title:
   *
   * A title that may be shown on the color chooser dialog.
   *
   * Since: 4.10
   */
  properties[PROP_TITLE] =
      g_param_spec_string ("title", NULL, NULL,
                           NULL,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiColorDialog:modal:
   *
   * Whether the color chooser dialog is modal.
   *
   * Since: 4.10
   */
  properties[PROP_MODAL] =
      g_param_spec_boolean ("modal", NULL, NULL,
                            TRUE,
                            G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiColorDialog:with-alpha:
   *
   * Whether colors may have alpha (translucency).
   *
   * When with-alpha is false, the color that is selected
   * will be forced to have alpha == 1.
   *
   * Since: 4.10
   */
  properties[PROP_WITH_ALPHA] =
      g_param_spec_boolean ("with-alpha", NULL, NULL,
                            TRUE,
                            G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);
}

/* }}} */
/* {{{ API: Constructor */

/**
 * bobgui_color_dialog_new:
 *
 * Creates a new `BobguiColorDialog` object.
 *
 * Returns: the new `BobguiColorDialog`
 *
 * Since: 4.10
 */
BobguiColorDialog *
bobgui_color_dialog_new (void)
{
  return g_object_new (BOBGUI_TYPE_COLOR_DIALOG, NULL);
}

/* }}} */
/* {{{ API: Getters and setters */

/**
 * bobgui_color_dialog_get_title:
 * @self: a color dialog
 *
 * Returns the title that will be shown on the
 * color chooser dialog.
 *
 * Returns: the title
 *
 * Since: 4.10
 */
const char *
bobgui_color_dialog_get_title (BobguiColorDialog *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLOR_DIALOG (self), NULL);

  return self->title;
}

/**
 * bobgui_color_dialog_set_title:
 * @self: a color dialog
 * @title: the new title
 *
 * Sets the title that will be shown on the
 * color chooser dialog.
 *
 * Since: 4.10
 */
void
bobgui_color_dialog_set_title (BobguiColorDialog *self,
                            const char     *title)
{
  char *new_title;

  g_return_if_fail (BOBGUI_IS_COLOR_DIALOG (self));
  g_return_if_fail (title != NULL);

  if (g_strcmp0 (self->title, title) == 0)
    return;

  new_title = g_strdup (title);
  g_free (self->title);
  self->title = new_title;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TITLE]);
}

/**
 * bobgui_color_dialog_get_modal:
 * @self: a color dialog
 *
 * Returns whether the color chooser dialog
 * blocks interaction with the parent window
 * while it is presented.
 *
 * Returns: true if the color chooser dialog is modal
 *
 * Since: 4.10
 */
gboolean
bobgui_color_dialog_get_modal (BobguiColorDialog *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLOR_DIALOG (self), TRUE);

  return self->modal;
}

/**
 * bobgui_color_dialog_set_modal:
 * @self: a color dialog
 * @modal: the new value
 *
 * Sets whether the color chooser dialog
 * blocks interaction with the parent window
 * while it is presented.
 *
 * Since: 4.10
 */
void
bobgui_color_dialog_set_modal (BobguiColorDialog *self,
                            gboolean        modal)
{
  g_return_if_fail (BOBGUI_IS_COLOR_DIALOG (self));

  if (self->modal == modal)
    return;

  self->modal = modal;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODAL]);
}

/**
 * bobgui_color_dialog_get_with_alpha:
 * @self: a color dailog
 *
 * Returns whether colors may have alpha.
 *
 * Returns: true if colors may have alpha
 *
 * Since: 4.10
 */
gboolean
bobgui_color_dialog_get_with_alpha (BobguiColorDialog *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLOR_DIALOG (self), TRUE);

  return self->with_alpha;
}

/**
 * bobgui_color_dialog_set_with_alpha:
 * @self: a color dialog
 * @with_alpha: the new value
 *
 * Sets whether colors may have alpha.
 *
 * Since: 4.10
 */
void
bobgui_color_dialog_set_with_alpha (BobguiColorDialog *self,
                                 gboolean        with_alpha)
{
  g_return_if_fail (BOBGUI_IS_COLOR_DIALOG (self));

  if (self->with_alpha == with_alpha)
    return;

  self->with_alpha = with_alpha;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_WITH_ALPHA]);
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

static void
response_cb (GTask *task,
             int    response)
{
  GCancellable *cancellable;

  cancellable = g_task_get_cancellable (task);

  if (cancellable)
    g_signal_handlers_disconnect_by_func (cancellable, cancelled_cb, task);

  if (response == BOBGUI_RESPONSE_OK)
    {
      BobguiColorChooserDialog *window;
      GdkRGBA color;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      window = BOBGUI_COLOR_CHOOSER_DIALOG (g_task_get_task_data (task));
      bobgui_color_chooser_get_rgba (BOBGUI_COLOR_CHOOSER (window), &color);
G_GNUC_END_IGNORE_DEPRECATIONS

      g_task_return_pointer (task, gdk_rgba_copy (&color), (GDestroyNotify) gdk_rgba_free);
    }
  else if (response == BOBGUI_RESPONSE_CLOSE)
    g_task_return_new_error (task, BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_CANCELLED, "Cancelled by application");
  else if (response == BOBGUI_RESPONSE_CANCEL ||
           response == BOBGUI_RESPONSE_DELETE_EVENT)
    g_task_return_new_error (task, BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_DISMISSED, "Dismissed by user");
  else
    g_task_return_new_error (task, BOBGUI_DIALOG_ERROR, BOBGUI_DIALOG_ERROR_FAILED, "Unknown failure (%d)", response);

  g_object_unref (task);
}

static BobguiWidget *
create_color_chooser (BobguiColorDialog *self,
                      BobguiWindow      *parent,
                      const GdkRGBA  *initial_color)
{
  BobguiWidget *window;
  char *title;

  if (self->title)
    title = self->title;
  else
    title = _("Pick a Color");

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  window = bobgui_color_chooser_dialog_new (title, parent);
  if (initial_color)
    bobgui_color_chooser_set_rgba (BOBGUI_COLOR_CHOOSER (window), initial_color);
  bobgui_color_chooser_set_use_alpha (BOBGUI_COLOR_CHOOSER (window), self->with_alpha);
  bobgui_window_set_modal (BOBGUI_WINDOW (window), self->modal);
G_GNUC_END_IGNORE_DEPRECATIONS

  return window;
}

/* }}} */
/* {{{ Async API */

/**
 * bobgui_color_dialog_choose_rgba:
 * @self: a color dialog
 * @parent: (nullable): the parent window
 * @initial_color: (nullable): the color to select initially
 * @cancellable: (nullable): a cancellable to cancel the operation
 * @callback: (scope async) (closure user_data): a callback to call
 *   when the operation is complete
 * @user_data: data to pass to @callback
 *
 * Presents a color chooser dialog to the user.
 *
 * Since: 4.10
 */
void
bobgui_color_dialog_choose_rgba (BobguiColorDialog       *self,
                              BobguiWindow            *parent,
                              const GdkRGBA        *initial_color,
                              GCancellable         *cancellable,
                              GAsyncReadyCallback   callback,
                              gpointer              user_data)
{
  BobguiWidget *window;
  GTask *task;

  g_return_if_fail (BOBGUI_IS_COLOR_DIALOG (self));

  window = create_color_chooser (self, parent, initial_color);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_check_cancellable (task, FALSE);
  g_task_set_source_tag (task, bobgui_color_dialog_choose_rgba);
  g_task_set_task_data (task, window, (GDestroyNotify) bobgui_window_destroy);

  if (cancellable)
    g_signal_connect (cancellable, "cancelled", G_CALLBACK (cancelled_cb), task);
  g_signal_connect_swapped (window, "response", G_CALLBACK (response_cb), task);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

/**
 * bobgui_color_dialog_choose_rgba_finish:
 * @self: a color dialog
 * @result: the result
 * @error: return location for a [enum@Bobgui.DialogError] error
 *
 * Finishes the [method@Bobgui.ColorDialog.choose_rgba] call
 *
 * Note that this function returns a [error@Bobgui.DialogError.DISMISSED]
 * error if the user cancels the dialog.
 *
 * Returns: (transfer full): the selected color
 *
 * Since: 4.10
 */
GdkRGBA *
bobgui_color_dialog_choose_rgba_finish (BobguiColorDialog  *self,
                                     GAsyncResult    *result,
                                     GError         **error)
{
  g_return_val_if_fail (BOBGUI_IS_COLOR_DIALOG (self), NULL);
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == bobgui_color_dialog_choose_rgba, NULL);

  /* Destroy the dialog window not to be bound to GTask lifecycle */
  g_task_set_task_data (G_TASK (result), NULL, NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}

/* }}} */

/* vim:set foldmethod=marker: */
