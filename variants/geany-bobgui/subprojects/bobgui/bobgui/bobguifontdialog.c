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

#include "bobguifontdialog.h"

#include "deprecated/bobguifontchooser.h"
#include "bobguifontchooserdialogprivate.h"
#include "bobguibutton.h"
#include "bobguidialogerror.h"
#include "bobguitypebuiltins.h"
#include <glib/gi18n-lib.h>

/**
 * BobguiFontDialog:
 *
 * Asynchronous API to present a font chooser dialog.
 *
 * `BobguiFontDialog` collects the arguments that are needed to present
 * the dialog to the user, such as a title for the dialog and whether
 * it should be modal.
 *
 * The dialog is shown with the [method@Bobgui.FontDialog.choose_font]
 * function or its variants.
 *
 * See [class@Bobgui.FontDialogButton] for a convenient control
 * that uses `BobguiFontDialog` and presents the results.
 *
 * Since: 4.10
 */

/* {{{ GObject implementation */

struct _BobguiFontDialog
{
  GObject parent_instance;

  char *title;
  PangoLanguage *language;
  PangoFontMap *fontmap;

  unsigned int modal : 1;

  BobguiFilter *filter;
};

enum
{
  PROP_TITLE = 1,
  PROP_MODAL,
  PROP_LANGUAGE,
  PROP_FONT_MAP,
  PROP_FILTER,

  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES];

G_DEFINE_TYPE (BobguiFontDialog, bobgui_font_dialog, G_TYPE_OBJECT)

static void
bobgui_font_dialog_init (BobguiFontDialog *self)
{
  self->modal = TRUE;
  self->language = pango_language_get_default ();
}

static void
bobgui_font_dialog_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  BobguiFontDialog *self = BOBGUI_FONT_DIALOG (object);

  switch (property_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;

    case PROP_MODAL:
      g_value_set_boolean (value, self->modal);
      break;

    case PROP_LANGUAGE:
      g_value_set_boxed (value, self->language);
      break;

    case PROP_FONT_MAP:
      g_value_set_object (value, self->fontmap);
      break;

    case PROP_FILTER:
      g_value_set_object (value, self->filter);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_font_dialog_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BobguiFontDialog *self = BOBGUI_FONT_DIALOG (object);

  switch (property_id)
    {
    case PROP_TITLE:
      bobgui_font_dialog_set_title (self, g_value_get_string (value));
      break;

    case PROP_MODAL:
      bobgui_font_dialog_set_modal (self, g_value_get_boolean (value));
      break;

    case PROP_LANGUAGE:
      bobgui_font_dialog_set_language (self, g_value_get_boxed (value));
      break;

    case PROP_FONT_MAP:
      bobgui_font_dialog_set_font_map (self, g_value_get_object (value));
      break;

    case PROP_FILTER:
      bobgui_font_dialog_set_filter (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_font_dialog_finalize (GObject *object)
{
  BobguiFontDialog *self = BOBGUI_FONT_DIALOG (object);

  g_free (self->title);
  g_clear_object (&self->fontmap);
  g_clear_object (&self->filter);

  G_OBJECT_CLASS (bobgui_font_dialog_parent_class)->finalize (object);
}

static void
bobgui_font_dialog_class_init (BobguiFontDialogClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->get_property = bobgui_font_dialog_get_property;
  object_class->set_property = bobgui_font_dialog_set_property;
  object_class->finalize = bobgui_font_dialog_finalize;

  /**
   * BobguiFontDialog:title:
   *
   * A title that may be shown on the font chooser
   * dialog that is presented by [method@Bobgui.FontDialog.choose_font].
   *
   * Since: 4.10
   */
  properties[PROP_TITLE] =
      g_param_spec_string ("title", NULL, NULL,
                           NULL,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFontDialog:modal:
   *
   * Whether the font chooser dialog is modal.
   *
   * Since: 4.10
   */
  properties[PROP_MODAL] =
      g_param_spec_boolean ("modal", NULL, NULL,
                            TRUE,
                            G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFontDialog:language:
   *
   * The language for which the font features are selected.
   *
   * Since: 4.10
   */
  properties[PROP_LANGUAGE] =
      g_param_spec_boxed ("language", NULL, NULL,
                          PANGO_TYPE_LANGUAGE,
                          G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFontDialog:font-map:
   *
   * A custom font map to select fonts from.
   *
   * A custom font map can be used to present application-specific
   * fonts instead of or in addition to the normal system fonts.
   *
   * Since: 4.10
   */
  properties[PROP_FONT_MAP] =
      g_param_spec_object ("font-map", NULL, NULL,
                           PANGO_TYPE_FONT_MAP,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiFontDialog:filter:
   *
   * A filter to restrict what fonts are shown in the font chooser dialog.
   *
   * Since: 4.10
   */
  properties[PROP_FILTER] =
      g_param_spec_object ("filter", NULL, NULL,
                           BOBGUI_TYPE_FILTER,
                           G_PARAM_READWRITE|G_PARAM_STATIC_STRINGS|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);
}

/* }}} */
/* {{{ API: Constructor */

/**
 * bobgui_font_dialog_new:
 *
 * Creates a new `BobguiFontDialog` object.
 *
 * Returns: the new `BobguiFontDialog`
 *
 * Since: 4.10
 */
BobguiFontDialog *
bobgui_font_dialog_new (void)
{
  return g_object_new (BOBGUI_TYPE_FONT_DIALOG, NULL);
}

/* }}} */
/* {{{ API: Getters and setters */

/**
 * bobgui_font_dialog_get_title:
 * @self: a font dialog
 *
 * Returns the title that will be shown on the font chooser dialog.
 *
 * Returns: the title
 *
 * Since: 4.10
 */
const char *
bobgui_font_dialog_get_title (BobguiFontDialog *self)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_DIALOG (self), NULL);

  return self->title;
}

/**
 * bobgui_font_dialog_set_title:
 * @self: a font dialog
 * @title: the new title
 *
 * Sets the title that will be shown on the font chooser dialog.
 *
 * Since: 4.10
 */
void
bobgui_font_dialog_set_title (BobguiFontDialog *self,
                           const char    *title)
{
  char *new_title;

  g_return_if_fail (BOBGUI_IS_FONT_DIALOG (self));
  g_return_if_fail (title != NULL);

  if (g_strcmp0 (self->title, title) == 0)
    return;

  new_title = g_strdup (title);
  g_free (self->title);
  self->title = new_title;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TITLE]);
}

/**
 * bobgui_font_dialog_get_modal:
 * @self: a font dialog
 *
 * Returns whether the font chooser dialog blocks interaction
 * with the parent window while it is presented.
 *
 * Returns: true if the font chooser dialog is modal
 *
 * Since: 4.10
 */
gboolean
bobgui_font_dialog_get_modal (BobguiFontDialog *self)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_DIALOG (self), TRUE);

  return self->modal;
}

/**
 * bobgui_font_dialog_set_modal:
 * @self: a font dialog
 * @modal: the new value
 *
 * Sets whether the font chooser dialog blocks interaction
 * with the parent window while it is presented.
 *
 * Since: 4.10
 */
void
bobgui_font_dialog_set_modal (BobguiFontDialog *self,
                           gboolean       modal)
{
  g_return_if_fail (BOBGUI_IS_FONT_DIALOG (self));

  if (self->modal == modal)
    return;

  self->modal = modal;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODAL]);
}

/**
 * bobgui_font_dialog_get_language:
 * @self: a font dialog
 *
 * Returns the language for which font features are applied.
 *
 * Returns: (nullable): the language for font features
 *
 * Since: 4.10
 */
PangoLanguage *
bobgui_font_dialog_get_language (BobguiFontDialog *self)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_DIALOG (self), NULL);

  return self->language;
}

/**
 * bobgui_font_dialog_set_language:
 * @self: a font dialog
 * @language: the language for font features
 *
 * Sets the language for which font features are applied.
 *
 * Since: 4.10
 */
void
bobgui_font_dialog_set_language (BobguiFontDialog *self,
                              PangoLanguage *language)
{
  g_return_if_fail (BOBGUI_IS_FONT_DIALOG (self));

  if (self->language == language)
    return;

  self->language = language;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_LANGUAGE]);
}

/**
 * bobgui_font_dialog_get_font_map:
 * @self: a font dialog
 *
 * Returns the fontmap from which fonts are selected,
 * or `NULL` for the default fontmap.
 *
 * Returns: (nullable) (transfer none): the fontmap
 *
 * Since: 4.10
 */
PangoFontMap *
bobgui_font_dialog_get_font_map (BobguiFontDialog *self)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_DIALOG (self), NULL);

  return self->fontmap;
}

/**
 * bobgui_font_dialog_set_font_map:
 * @self: a font dialog
 * @fontmap: (nullable): the fontmap
 *
 * Sets the fontmap from which fonts are selected.
 *
 * If @fontmap is `NULL`, the default fontmap is used.
 *
 * Since: 4.10
 */
void
bobgui_font_dialog_set_font_map (BobguiFontDialog *self,
                              PangoFontMap  *fontmap)
{
  g_return_if_fail (BOBGUI_IS_FONT_DIALOG (self));

  if (g_set_object (&self->fontmap, fontmap))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FONT_MAP]);
}

/**
 * bobgui_font_dialog_get_filter:
 * @self: a font dialog
 *
 * Returns the filter that decides which fonts to display
 * in the font chooser dialog.
 *
 * Returns: (nullable) (transfer none): the filter
 *
 * Since: 4.10
 */
BobguiFilter *
bobgui_font_dialog_get_filter (BobguiFontDialog *self)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_DIALOG (self), NULL);

  return self->filter;
}
/**
 * bobgui_font_dialog_set_filter:
 * @self: a font dialog
 * @filter: (nullable): the filter
 *
 * Adds a filter that decides which fonts to display
 * in the font chooser dialog.
 *
 * The filter must be able to handle both `PangoFontFamily`
 * and `PangoFontFace` objects.
 *
 * Since: 4.10
 */
void
bobgui_font_dialog_set_filter (BobguiFontDialog *self,
                            BobguiFilter     *filter)
{
  g_return_if_fail (BOBGUI_IS_FONT_DIALOG (self));
  g_return_if_fail (filter == NULL || BOBGUI_IS_FILTER (filter));

  if (g_set_object (&self->filter, filter))
    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FILTER]);
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

typedef struct
{
  PangoFontDescription *font_desc;
  char *font_features;
  PangoLanguage *language;
} FontResult;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static void
response_cb (GTask *task,
             int    response)
{
  GCancellable *cancellable;
  BobguiFontChooserDialog *window;
  BobguiFontChooserLevel level;

  cancellable = g_task_get_cancellable (task);

  if (cancellable)
    g_signal_handlers_disconnect_by_func (cancellable, cancelled_cb, task);


  window = BOBGUI_FONT_CHOOSER_DIALOG (g_task_get_task_data (task));
  level = bobgui_font_chooser_get_level (BOBGUI_FONT_CHOOSER (window));

  if (response == BOBGUI_RESPONSE_OK)
    {
      if (level & BOBGUI_FONT_CHOOSER_LEVEL_FEATURES)
        {
          FontResult font_result;

          font_result.font_desc = bobgui_font_chooser_get_font_desc (BOBGUI_FONT_CHOOSER (window));
          font_result.font_features = bobgui_font_chooser_get_font_features (BOBGUI_FONT_CHOOSER (window));
          font_result.language = pango_language_from_string (bobgui_font_chooser_get_language (BOBGUI_FONT_CHOOSER (window)));

          g_task_return_pointer (task, &font_result, NULL);

          g_clear_pointer (&font_result.font_desc, pango_font_description_free);
          g_clear_pointer (&font_result.font_features, g_free);
        }
      else if (level & BOBGUI_FONT_CHOOSER_LEVEL_SIZE)
        {
          PangoFontDescription *font_desc;

          font_desc = bobgui_font_chooser_get_font_desc (BOBGUI_FONT_CHOOSER (window));

          g_task_return_pointer (task, font_desc, (GDestroyNotify) pango_font_description_free);
        }
      else if (level & BOBGUI_FONT_CHOOSER_LEVEL_STYLE)
        {
          PangoFontFace *face;

          face = bobgui_font_chooser_get_font_face (BOBGUI_FONT_CHOOSER (window));

          g_task_return_pointer (task, g_object_ref (face), g_object_unref);
        }
      else
        {
          PangoFontFamily *family;

          family = bobgui_font_chooser_get_font_family (BOBGUI_FONT_CHOOSER (window));

          g_task_return_pointer (task, g_object_ref (family), g_object_unref);
        }
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
G_GNUC_END_IGNORE_DEPRECATIONS

static void
dialog_response (BobguiDialog *dialog,
                 int        response,
                 GTask     *task)
{
  response_cb (task, response);
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static BobguiWidget *
create_font_chooser (BobguiFontDialog        *self,
                     BobguiWindow            *parent,
                     PangoFontDescription *initial_value,
                     BobguiFontChooserLevel   level)
{
  BobguiWidget *window;
  const char *title;

  if (self->title)
    title = self->title;
  else
    title = _("Pick a Font");

  window = bobgui_font_chooser_dialog_new (title, parent);
  bobgui_font_chooser_set_level (BOBGUI_FONT_CHOOSER (window), level);
  bobgui_window_set_modal (BOBGUI_WINDOW (window), TRUE);
  if (self->language)
    bobgui_font_chooser_set_language (BOBGUI_FONT_CHOOSER (window),
                                   pango_language_to_string (self->language));
  if (self->fontmap)
    bobgui_font_chooser_set_font_map (BOBGUI_FONT_CHOOSER (window), self->fontmap);
  if (self->filter)
    bobgui_font_chooser_dialog_set_filter (BOBGUI_FONT_CHOOSER_DIALOG (window), self->filter);
  if (initial_value)
    bobgui_font_chooser_set_font_desc (BOBGUI_FONT_CHOOSER (window), initial_value);

  return window;
}
G_GNUC_END_IGNORE_DEPRECATIONS

/* }}} */
/* {{{ Async API */

/**
 * bobgui_font_dialog_choose_family:
 * @self: a font dialog
 * @parent: (nullable): the parent window
 * @initial_value: (nullable): the initial value
 * @cancellable: (nullable): a cancellable to cancel the operation
 * @callback: (scope async) (closure user_data): a callback to call when the
 *   operation is complete
 * @user_data: data to pass to @callback
 *
 * Presents a font chooser dialog to the user.
 *
 * The font chooser dialog will be set up for selecting a font family.
 *
 * Since: 4.10
 */
void
bobgui_font_dialog_choose_family (BobguiFontDialog        *self,
                               BobguiWindow            *parent,
                               PangoFontFamily      *initial_value,
                               GCancellable         *cancellable,
                               GAsyncReadyCallback   callback,
                               gpointer              user_data)
{
  BobguiWidget *window;
  PangoFontDescription *desc = NULL;
  GTask *task;

  g_return_if_fail (BOBGUI_IS_FONT_DIALOG (self));

  if (initial_value)
    {
      desc = pango_font_description_new ();
      pango_font_description_set_family (desc, pango_font_family_get_name (initial_value));
    }

  window = create_font_chooser (self, parent, desc,
                                BOBGUI_FONT_CHOOSER_LEVEL_FAMILY);

  g_clear_pointer (&desc, pango_font_description_free);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_check_cancellable (task, FALSE);
  g_task_set_source_tag (task, bobgui_font_dialog_choose_family);
  g_task_set_task_data (task, window, (GDestroyNotify) bobgui_window_destroy);

  if (cancellable)
    g_signal_connect (cancellable, "cancelled", G_CALLBACK (cancelled_cb), task);

  g_signal_connect (window, "response", G_CALLBACK (dialog_response), task);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

/**
 * bobgui_font_dialog_choose_family_finish:
 * @self: a font dialog
 * @result: the result
 * @error: return location for a [enum@Bobgui.DialogError] error
 *
 * Finishes the [method@Bobgui.FontDialog.choose_family] call.
 *
 * Note that this function returns a [error@Bobgui.DialogError.DISMISSED]
 * error if the user cancels the dialog.
 *
 * Returns: (transfer full): the selected [class@Pango.FontFamily]
 *
 * Since: 4.10
 */
PangoFontFamily *
bobgui_font_dialog_choose_family_finish (BobguiFontDialog  *self,
                                      GAsyncResult   *result,
                                      GError        **error)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_DIALOG (self), NULL);
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == bobgui_font_dialog_choose_family, NULL);

  /* Destroy the dialog window not to be bound to GTask lifecycle */
  g_task_set_task_data (G_TASK (result), NULL, NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}

/**
 * bobgui_font_dialog_choose_face:
 * @self: a font dialog
 * @parent: (nullable): the parent window
 * @initial_value: (nullable): the initial value
 * @cancellable: (nullable): a cancellable to cancel the operation
 * @callback: (scope async) (closure user_data): a callback to call when the
 *   operation is complete
 * @user_data: data to pass to @callback
 *
 * Presents a font chooser dialog to the user.
 *
 * The font chooser dialog will be set up for selecting a font face.
 *
 * A font face represents a font family and style, but no specific font size.
 *
 * Since: 4.10
 */
void
bobgui_font_dialog_choose_face (BobguiFontDialog       *self,
                             BobguiWindow           *parent,
                             PangoFontFace       *initial_value,
                             GCancellable        *cancellable,
                             GAsyncReadyCallback  callback,
                             gpointer             user_data)
{
  BobguiWidget *window;
  PangoFontDescription *desc = NULL;
  GTask *task;

  g_return_if_fail (BOBGUI_IS_FONT_DIALOG (self));

  if (initial_value)
    desc = pango_font_face_describe (initial_value);

  window = create_font_chooser (self, parent, desc,
                                BOBGUI_FONT_CHOOSER_LEVEL_FAMILY |
                                BOBGUI_FONT_CHOOSER_LEVEL_STYLE);

  g_clear_pointer (&desc, pango_font_description_free);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_check_cancellable (task, FALSE);
  g_task_set_source_tag (task, bobgui_font_dialog_choose_face);
  g_task_set_task_data (task, window, (GDestroyNotify) bobgui_window_destroy);

  if (cancellable)
    g_signal_connect (cancellable, "cancelled", G_CALLBACK (cancelled_cb), task);

  g_signal_connect (window, "response", G_CALLBACK (dialog_response), task);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

/**
 * bobgui_font_dialog_choose_face_finish:
 * @self: a font dialog
 * @result: the result
 * @error: return location for a [enum@Bobgui.DialogError] error
 *
 * Finishes the [method@Bobgui.FontDialog.choose_face] call.
 *
 * Note that this function returns a [error@Bobgui.DialogError.DISMISSED]
 * error if the user cancels the dialog.
 *
 * Returns: (transfer full): the selected [class@Pango.FontFace]
 *
 * Since: 4.10
 */
PangoFontFace *
bobgui_font_dialog_choose_face_finish (BobguiFontDialog  *self,
                                    GAsyncResult   *result,
                                    GError        **error)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_DIALOG (self), NULL);
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == bobgui_font_dialog_choose_face, NULL);

  /* Destroy the dialog window not to be bound to GTask lifecycle */
  g_task_set_task_data (G_TASK (result), NULL, NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}

/**
 * bobgui_font_dialog_choose_font:
 * @self: a font dialog
 * @parent: (nullable): the parent window
 * @initial_value: (nullable): the font to select initially
 * @cancellable: (nullable): a cancellable to cancel the operation
 * @callback: (scope async) (closure user_data): a callback to call when the
 *   operation is complete
 * @user_data: data to pass to @callback
 *
 * Presents a font chooser dialog to the user.
 *
 * The font chooser dialog will be set up for selecting a font.
 *
 * If you want to let the user select font features as well,
 * use [method@Bobgui.FontDialog.choose_font_and_features] instead.
 *
 * Since: 4.10
 */
void
bobgui_font_dialog_choose_font (BobguiFontDialog        *self,
                             BobguiWindow            *parent,
                             PangoFontDescription *initial_value,
                             GCancellable         *cancellable,
                             GAsyncReadyCallback   callback,
                             gpointer              user_data)
{
  BobguiWidget *window;
  GTask *task;

  g_return_if_fail (BOBGUI_IS_FONT_DIALOG (self));

  window = create_font_chooser (self, parent, initial_value,
                                BOBGUI_FONT_CHOOSER_LEVEL_FAMILY |
                                BOBGUI_FONT_CHOOSER_LEVEL_STYLE |
                                BOBGUI_FONT_CHOOSER_LEVEL_SIZE |
                                BOBGUI_FONT_CHOOSER_LEVEL_VARIATIONS);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_check_cancellable (task, FALSE);
  g_task_set_source_tag (task, bobgui_font_dialog_choose_font);
  g_task_set_task_data (task, window, (GDestroyNotify) bobgui_window_destroy);

  if (cancellable)
    g_signal_connect (cancellable, "cancelled", G_CALLBACK (cancelled_cb), task);

  g_signal_connect (window, "response", G_CALLBACK (dialog_response), task);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

/**
 * bobgui_font_dialog_choose_font_finish:
 * @self: a font dialog
 * @result: the result
 * @error: return location for a [enum@Bobgui.DialogError] error
 *
 * Finishes the [method@Bobgui.FontDialog.choose_font] call.
 *
 * Note that this function returns a [error@Bobgui.DialogError.DISMISSED]
 * error if the user cancels the dialog.
 *
 * Returns: (transfer full): a [struct@Pango.FontDescription] describing
 *   the selected font
 *
 * Since: 4.10
 */
PangoFontDescription *
bobgui_font_dialog_choose_font_finish (BobguiFontDialog  *self,
                                    GAsyncResult   *result,
                                    GError        **error)
{
  g_return_val_if_fail (BOBGUI_IS_FONT_DIALOG (self), NULL);
  g_return_val_if_fail (g_task_is_valid (result, self), NULL);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == bobgui_font_dialog_choose_font, NULL);

  /* Destroy the dialog window not to be bound to GTask lifecycle */
  g_task_set_task_data (G_TASK (result), NULL, NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}

/**
 * bobgui_font_dialog_choose_font_and_features:
 * @self: a font dialog
 * @parent: (nullable): the parent window
 * @initial_value: (nullable): the font to select initially
 * @cancellable: (nullable): a cancellable to cancel the operation
 * @callback: (scope async) (closure user_data): a callback to call when the
 *   operation is complete
 * @user_data: data to pass to @callback
 *
 * Presents a font chooser dialog to the user.
 *
 * The font chooser dialog will be set up for selecting a font
 * and specify features for the selected font.
 *
 * Font features affect how the font is rendered, for example
 * enabling glyph variants or ligatures.
 *
 * Since: 4.10
 */
void
bobgui_font_dialog_choose_font_and_features (BobguiFontDialog        *self,
                                          BobguiWindow            *parent,
                                          PangoFontDescription *initial_value,
                                          GCancellable         *cancellable,
                                          GAsyncReadyCallback   callback,
                                          gpointer              user_data)
{
  BobguiWidget *window;
  GTask *task;

  g_return_if_fail (BOBGUI_IS_FONT_DIALOG (self));

  window = create_font_chooser (self, parent, initial_value,
                                BOBGUI_FONT_CHOOSER_LEVEL_FAMILY |
                                BOBGUI_FONT_CHOOSER_LEVEL_STYLE |
                                BOBGUI_FONT_CHOOSER_LEVEL_SIZE |
                                BOBGUI_FONT_CHOOSER_LEVEL_VARIATIONS |
                                BOBGUI_FONT_CHOOSER_LEVEL_FEATURES);

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_check_cancellable (task, FALSE);
  g_task_set_source_tag (task, bobgui_font_dialog_choose_font_and_features);
  g_task_set_task_data (task, window, (GDestroyNotify) bobgui_window_destroy);

  if (cancellable)
    g_signal_connect (cancellable, "cancelled", G_CALLBACK (cancelled_cb), task);

  g_signal_connect (window, "response", G_CALLBACK (dialog_response), task);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

/**
 * bobgui_font_dialog_choose_font_and_features_finish:
 * @self: a font dialog
 * @result: the result
 * @font_desc: (out): return location for font description
 * @font_features: (out): return location for font features
 * @language: (out): return location for the language
 * @error: return location for a [enum@Bobgui.DialogError] error
 *
 * Finishes the [method@Bobgui.FontDialog.choose_font_and_features] call.
 *
 * The selected font and features are returned in @font_desc and
 * @font_features.
 *
 * Note that this function returns a [error@Bobgui.DialogError.DISMISSED]
 * error if the user cancels the dialog.
 *
 * Returns: true if a font was selected
 *
 * Since: 4.10
 */
gboolean
bobgui_font_dialog_choose_font_and_features_finish (BobguiFontDialog         *self,
                                                 GAsyncResult          *result,
                                                 PangoFontDescription **font_desc,
                                                 char                 **font_features,
                                                 PangoLanguage        **language,
                                                 GError               **error)
{
  FontResult *font_result;

  g_return_val_if_fail (BOBGUI_IS_FONT_DIALOG (self), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, self), FALSE);
  g_return_val_if_fail (g_task_get_source_tag (G_TASK (result)) == bobgui_font_dialog_choose_font_and_features, FALSE);

  /* Destroy the dialog window not to be bound to GTask lifecycle */
  g_task_set_task_data (G_TASK (result), NULL, NULL);
  font_result = g_task_propagate_pointer (G_TASK (result), error);

  if (font_result)
    {
      *font_desc = g_steal_pointer (&font_result->font_desc);
      *font_features = g_steal_pointer (&font_result->font_features);
      *language = g_steal_pointer (&font_result->language);
    }

  return font_result != NULL;
}

/* }}} */

/* vim:set foldmethod=marker: */
