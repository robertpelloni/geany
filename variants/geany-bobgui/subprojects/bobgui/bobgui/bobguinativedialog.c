/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/* BOBGUI - The Bobgui Framework
 * bobguifilechoosernative.c: Native File selector dialog
 * Copyright (C) 2015, Red Hat, Inc.
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

#include "bobguinativedialogprivate.h"

#include "bobguiprivate.h"
#include "bobguisizerequest.h"
#include "bobguitypebuiltins.h"
#include "bobguisettings.h"
#include "bobguitogglebutton.h"
#include "bobguiheaderbar.h"
#include "bobguilabel.h"

/**
 * BobguiNativeDialog:
 *
 * Base class for platform dialogs that don't use `BobguiDialog`.
 *
 * Native dialogs are used in order to integrate better with a platform,
 * by looking the same as other native applications and supporting
 * platform specific features.
 *
 * The [class@Bobgui.Dialog] functions cannot be used on such objects,
 * but we need a similar API in order to drive them. The `BobguiNativeDialog`
 * object is an API that allows you to do this. It allows you to set
 * various common properties on the dialog, as well as show and hide
 * it and get a [signal@Bobgui.NativeDialog::response] signal when the user
 * finished with the dialog.
 *
 * Note that unlike `BobguiDialog`, `BobguiNativeDialog` objects are not
 * toplevel widgets, and BOBGUI does not keep them alive. It is your
 * responsibility to keep a reference until you are done with the
 * object.
 */

typedef struct _BobguiNativeDialogPrivate BobguiNativeDialogPrivate;

struct _BobguiNativeDialogPrivate
{
  BobguiWindow *transient_for;
  char *title;

  guint visible : 1;
  guint modal : 1;
};

enum {
  PROP_0,
  PROP_TITLE,
  PROP_VISIBLE,
  PROP_MODAL,
  PROP_TRANSIENT_FOR,

  LAST_ARG,
};

enum {
  RESPONSE,

  LAST_SIGNAL
};

static GParamSpec *native_props[LAST_ARG] = { NULL, };
static guint native_signals[LAST_SIGNAL];

G_DEFINE_ABSTRACT_TYPE_WITH_CODE (BobguiNativeDialog, bobgui_native_dialog, G_TYPE_OBJECT,
                                  G_ADD_PRIVATE (BobguiNativeDialog))

static void
bobgui_native_dialog_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)

{
  BobguiNativeDialog *self = BOBGUI_NATIVE_DIALOG (object);

  switch (prop_id)
    {
    case PROP_TITLE:
      bobgui_native_dialog_set_title (self, g_value_get_string (value));
      break;

    case PROP_MODAL:
      bobgui_native_dialog_set_modal (self, g_value_get_boolean (value));
      break;

    case PROP_VISIBLE:
      if (g_value_get_boolean (value))
        bobgui_native_dialog_show (self);
      else
        bobgui_native_dialog_hide (self);
      break;

    case PROP_TRANSIENT_FOR:
      bobgui_native_dialog_set_transient_for (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_native_dialog_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  BobguiNativeDialog *self = BOBGUI_NATIVE_DIALOG (object);
  BobguiNativeDialogPrivate *priv = bobgui_native_dialog_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;

    case PROP_MODAL:
      g_value_set_boolean (value, priv->modal);
      break;

    case PROP_VISIBLE:
      g_value_set_boolean (value, priv->visible);
      break;

    case PROP_TRANSIENT_FOR:
      g_value_set_object (value, priv->transient_for);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void parent_destroyed (BobguiWidget       *parent,
                              BobguiNativeDialog *self);

static void
bobgui_native_dialog_dispose (GObject *object)
{
  BobguiNativeDialog *self = BOBGUI_NATIVE_DIALOG (object);
  BobguiNativeDialogPrivate *priv = bobgui_native_dialog_get_instance_private (self);

  if (priv->transient_for)
    {
      g_signal_handlers_disconnect_by_func (priv->transient_for, parent_destroyed, self);
      priv->transient_for = NULL;
    }

  if (priv->visible)
    bobgui_native_dialog_hide (self);

  G_OBJECT_CLASS (bobgui_native_dialog_parent_class)->dispose (object);
}

static void
bobgui_native_dialog_finalize (GObject *object)
{
  BobguiNativeDialog *self = BOBGUI_NATIVE_DIALOG (object);
  BobguiNativeDialogPrivate *priv = bobgui_native_dialog_get_instance_private (self);

  g_clear_pointer (&priv->title, g_free);
  g_clear_object (&priv->transient_for);

  G_OBJECT_CLASS (bobgui_native_dialog_parent_class)->finalize (object);
}

static void
bobgui_native_dialog_class_init (BobguiNativeDialogClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->set_property = bobgui_native_dialog_set_property;
  gobject_class->get_property = bobgui_native_dialog_get_property;
  gobject_class->finalize = bobgui_native_dialog_finalize;
  gobject_class->dispose = bobgui_native_dialog_dispose;

  /**
   * BobguiNativeDialog:title:
   *
   * The title of the dialog window
   */
  native_props[PROP_TITLE] =
    g_param_spec_string ("title", NULL, NULL,
                         NULL,
                         BOBGUI_PARAM_READWRITE);

  /**
   * BobguiNativeDialog:modal:
   *
   * Whether the window should be modal with respect to its transient parent.
   */
  native_props[PROP_MODAL] =
    g_param_spec_boolean ("modal", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiNativeDialog:visible:
   *
   * Whether the window is currently visible.
   */
  native_props[PROP_VISIBLE] =
    g_param_spec_boolean ("visible", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiNativeDialog:transient-for:
   *
   * The transient parent of the dialog, or %NULL for none.
   */
  native_props[PROP_TRANSIENT_FOR] =
    g_param_spec_object ("transient-for", NULL, NULL,
                         BOBGUI_TYPE_WINDOW,
                         BOBGUI_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, LAST_ARG, native_props);

  /**
   * BobguiNativeDialog::response:
   * @self: the object on which the signal is emitted
   * @response_id: the response ID
   *
   * Emitted when the user responds to the dialog.
   *
   * When this is called the dialog has been hidden.
   *
   * If you call [method@Bobgui.NativeDialog.hide] before the user
   * responds to the dialog this signal will not be emitted.
   */
  native_signals[RESPONSE] =
    g_signal_new (I_("response"),
                  G_OBJECT_CLASS_TYPE (class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiNativeDialogClass, response),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_INT);
}

static void
bobgui_native_dialog_init (BobguiNativeDialog *self)
{
}

/**
 * bobgui_native_dialog_show:
 * @self: a `BobguiNativeDialog`
 *
 * Shows the dialog on the display.
 *
 * When the user accepts the state of the dialog the dialog will
 * be automatically hidden and the [signal@Bobgui.NativeDialog::response]
 * signal will be emitted.
 *
 * Multiple calls while the dialog is visible will be ignored.
 */
void
bobgui_native_dialog_show (BobguiNativeDialog *self)
{
  BobguiNativeDialogPrivate *priv = bobgui_native_dialog_get_instance_private (self);
  BobguiNativeDialogClass *klass;

  g_return_if_fail (BOBGUI_IS_NATIVE_DIALOG (self));

  if (priv->visible)
   return;

  klass = BOBGUI_NATIVE_DIALOG_GET_CLASS (self);

  g_return_if_fail (klass->show != NULL);

  klass->show (self);

  priv->visible = TRUE;
  g_object_notify_by_pspec (G_OBJECT (self), native_props[PROP_VISIBLE]);
}

/**
 * bobgui_native_dialog_hide:
 * @self: a `BobguiNativeDialog`
 *
 * Hides the dialog if it is visible, aborting any interaction.
 *
 * Once this is called the [signal@Bobgui.NativeDialog::response] signal
 * will *not* be emitted until after the next call to
 * [method@Bobgui.NativeDialog.show].
 *
 * If the dialog is not visible this does nothing.
 */
void
bobgui_native_dialog_hide (BobguiNativeDialog *self)
{
  BobguiNativeDialogPrivate *priv = bobgui_native_dialog_get_instance_private (self);
  BobguiNativeDialogClass *klass;

  g_return_if_fail (BOBGUI_IS_NATIVE_DIALOG (self));

  if (!priv->visible)
    return;

  priv->visible = FALSE;

  klass = BOBGUI_NATIVE_DIALOG_GET_CLASS (self);

  g_return_if_fail (klass->hide != NULL);

  klass->hide (self);

  g_object_notify_by_pspec (G_OBJECT (self), native_props[PROP_VISIBLE]);
}

/**
 * bobgui_native_dialog_destroy:
 * @self: a `BobguiNativeDialog`
 *
 * Destroys a dialog.
 *
 * When a dialog is destroyed, it will break any references it holds
 * to other objects.
 *
 * If it is visible it will be hidden and any underlying window system
 * resources will be destroyed.
 *
 * Note that this does not release any reference to the object (as opposed
 * to destroying a `BobguiWindow`) because there is no reference from the
 * windowing system to the `BobguiNativeDialog`.
 */
void
bobgui_native_dialog_destroy (BobguiNativeDialog *self)
{
  g_return_if_fail (BOBGUI_IS_NATIVE_DIALOG (self));

  g_object_run_dispose (G_OBJECT (self));
}

void
_bobgui_native_dialog_emit_response (BobguiNativeDialog *self,
                                  int response_id)
{
  BobguiNativeDialogPrivate *priv = bobgui_native_dialog_get_instance_private (self);
  priv->visible = FALSE;
  g_object_notify_by_pspec (G_OBJECT (self), native_props[PROP_VISIBLE]);

  g_signal_emit (self, native_signals[RESPONSE], 0, response_id);
}

/**
 * bobgui_native_dialog_get_visible:
 * @self: a `BobguiNativeDialog`
 *
 * Determines whether the dialog is visible.
 *
 * Returns: %TRUE if the dialog is visible
 */
gboolean
bobgui_native_dialog_get_visible (BobguiNativeDialog *self)
{
  BobguiNativeDialogPrivate *priv = bobgui_native_dialog_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_NATIVE_DIALOG (self), FALSE);

  return priv->visible;
}

/**
 * bobgui_native_dialog_set_modal:
 * @self: a `BobguiNativeDialog`
 * @modal: whether the window is modal
 *
 * Sets a dialog modal or non-modal.
 *
 * Modal dialogs prevent interaction with other windows in the same
 * application. To keep modal dialogs on top of main application
 * windows, use [method@Bobgui.NativeDialog.set_transient_for] to make
 * the dialog transient for the parent; most window managers will
 * then disallow lowering the dialog below the parent.
 */
void
bobgui_native_dialog_set_modal (BobguiNativeDialog *self,
                             gboolean modal)
{
  BobguiNativeDialogPrivate *priv = bobgui_native_dialog_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_NATIVE_DIALOG (self));

  modal = modal != FALSE;

  if (priv->modal == modal)
    return;

  priv->modal = modal;
  g_object_notify_by_pspec (G_OBJECT (self), native_props[PROP_MODAL]);
}

/**
 * bobgui_native_dialog_get_modal:
 * @self: a `BobguiNativeDialog`
 *
 * Returns whether the dialog is modal.
 *
 * Returns: %TRUE if the dialog is set to be modal
 */
gboolean
bobgui_native_dialog_get_modal (BobguiNativeDialog *self)
{
  BobguiNativeDialogPrivate *priv = bobgui_native_dialog_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_NATIVE_DIALOG (self), FALSE);

  return priv->modal;
}

/**
 * bobgui_native_dialog_set_title:
 * @self: a `BobguiNativeDialog`
 * @title: title of the dialog
 *
 * Sets the title of the `BobguiNativeDialog.`
 */
void
bobgui_native_dialog_set_title (BobguiNativeDialog *self,
                                   const char *title)
{
  BobguiNativeDialogPrivate *priv = bobgui_native_dialog_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_NATIVE_DIALOG (self));

  g_free (priv->title);
  priv->title = g_strdup (title);

  g_object_notify_by_pspec (G_OBJECT (self), native_props[PROP_TITLE]);
}

/**
 * bobgui_native_dialog_get_title:
 * @self: a `BobguiNativeDialog`
 *
 * Gets the title of the `BobguiNativeDialog`.
 *
 * Returns: (nullable): the title of the dialog, or %NULL if none has
 *    been set explicitly. The returned string is owned by the widget
 *    and must not be modified or freed.
 */
const char *
bobgui_native_dialog_get_title (BobguiNativeDialog *self)
{
  BobguiNativeDialogPrivate *priv = bobgui_native_dialog_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_NATIVE_DIALOG (self), NULL);

  return priv->title;
}

static void
parent_destroyed (BobguiWidget       *parent,
                  BobguiNativeDialog *self)
{
  BobguiNativeDialogPrivate *priv = bobgui_native_dialog_get_instance_private (self);

  priv->transient_for = NULL;
}

/**
 * bobgui_native_dialog_set_transient_for:
 * @self: a `BobguiNativeDialog`
 * @parent: (nullable): parent window
 *
 * Dialog windows should be set transient for the main application
 * window they were spawned from.
 *
 * This allows window managers to e.g. keep the dialog on top of the
 * main window, or center the dialog over the main window.
 *
 * Passing %NULL for @parent unsets the current transient window.
 */
void
bobgui_native_dialog_set_transient_for (BobguiNativeDialog *self,
                                     BobguiWindow       *parent)
{
  BobguiNativeDialogPrivate *priv = bobgui_native_dialog_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_NATIVE_DIALOG (self));

  if (parent == priv->transient_for)
    return;

  if (priv->transient_for)
    g_signal_handlers_disconnect_by_func (priv->transient_for, parent_destroyed, self);

  priv->transient_for = parent;

  if (parent)
    g_signal_connect (parent, "destroy", G_CALLBACK (parent_destroyed), self);

  g_object_notify_by_pspec (G_OBJECT (self), native_props[PROP_TRANSIENT_FOR]);
}

/**
 * bobgui_native_dialog_get_transient_for:
 * @self: a `BobguiNativeDialog`
 *
 * Fetches the transient parent for this window.
 *
 * Returns: (nullable) (transfer none): the transient parent for this window,
 *   or %NULL if no transient parent has been set.
 */
BobguiWindow *
bobgui_native_dialog_get_transient_for (BobguiNativeDialog *self)
{
  BobguiNativeDialogPrivate *priv = bobgui_native_dialog_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_NATIVE_DIALOG (self), NULL);

  return priv->transient_for;
}
