/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2000 Red Hat, Inc.
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
#include <locale.h>

#include "bobguiimmulticontext.h"
#include "bobguiimmoduleprivate.h"
#include "bobguilabel.h"
#include "bobguimain.h"
#include "bobguiprivate.h"
#include "bobguisettings.h"


/**
 * BobguiIMMulticontext:
 *
 * Supports switching between multiple input methods.
 *
 * Text widgets such as `BobguiText` or `BobguiTextView` use a `BobguiIMMultiContext`
 * to implement their `im-module` property for switching between different
 * input methods.
 */


struct _BobguiIMMulticontextPrivate
{
  BobguiIMContext          *delegate;

  BobguiWidget             *client_widget;
  GdkRectangle           cursor_location;

  char                  *context_id;
  char                  *context_id_aux;

  guint                  use_preedit          : 1;
  guint                  have_cursor_location : 1;
  guint                  focus_in             : 1;
};

static void     bobgui_im_multicontext_notify             (GObject                 *object,
                                                        GParamSpec              *pspec);
static void     bobgui_im_multicontext_finalize           (GObject                 *object);

static void     bobgui_im_multicontext_set_delegate       (BobguiIMMulticontext       *multicontext,
							BobguiIMContext            *delegate,
							gboolean                 finalizing);

static void     bobgui_im_multicontext_set_client_widget  (BobguiIMContext            *context,
							BobguiWidget               *widget);
static void     bobgui_im_multicontext_get_preedit_string (BobguiIMContext            *context,
							char                   **str,
							PangoAttrList          **attrs,
							int                    *cursor_pos);
static gboolean bobgui_im_multicontext_filter_keypress    (BobguiIMContext            *context,
							GdkEvent                *event);
static void     bobgui_im_multicontext_focus_in           (BobguiIMContext            *context);
static void     bobgui_im_multicontext_focus_out          (BobguiIMContext            *context);
static void     bobgui_im_multicontext_reset              (BobguiIMContext            *context);
static void     bobgui_im_multicontext_set_cursor_location (BobguiIMContext            *context,
							GdkRectangle		*area);
static void     bobgui_im_multicontext_set_use_preedit    (BobguiIMContext            *context,
							gboolean                 use_preedit);
static gboolean bobgui_im_multicontext_get_surrounding_with_selection
                                                       (BobguiIMContext            *context,
                                                        char                   **text,
                                                        int                     *cursor_index,
                                                        int                     *anchor_index);
static void     bobgui_im_multicontext_set_surrounding_with_selection
                                                       (BobguiIMContext            *context,
                                                        const char              *text,
                                                        int                      len,
                                                        int                      cursor_index,
                                                        int                      anchor_index);

static void     bobgui_im_multicontext_preedit_start_cb        (BobguiIMContext      *delegate,
							     BobguiIMMulticontext *multicontext);
static void     bobgui_im_multicontext_preedit_end_cb          (BobguiIMContext      *delegate,
							     BobguiIMMulticontext *multicontext);
static void     bobgui_im_multicontext_preedit_changed_cb      (BobguiIMContext      *delegate,
							     BobguiIMMulticontext *multicontext);
static void     bobgui_im_multicontext_commit_cb               (BobguiIMContext      *delegate,
							     const char        *str,
							     BobguiIMMulticontext *multicontext);
static gboolean bobgui_im_multicontext_retrieve_surrounding_cb (BobguiIMContext      *delegate,
							     BobguiIMMulticontext *multicontext);
static gboolean bobgui_im_multicontext_delete_surrounding_cb   (BobguiIMContext      *delegate,
							     int                offset,
							     int                n_chars,
							     BobguiIMMulticontext *multicontext);
static gboolean bobgui_im_multicontext_activate_osk_with_event (BobguiIMContext *context,
                                                             GdkEvent     *event);

static void propagate_purpose (BobguiIMMulticontext *context);

G_DEFINE_TYPE_WITH_PRIVATE (BobguiIMMulticontext, bobgui_im_multicontext, BOBGUI_TYPE_IM_CONTEXT)

static void
bobgui_im_multicontext_class_init (BobguiIMMulticontextClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BobguiIMContextClass *im_context_class = BOBGUI_IM_CONTEXT_CLASS (class);

  gobject_class->notify = bobgui_im_multicontext_notify;

  im_context_class->set_client_widget = bobgui_im_multicontext_set_client_widget;
  im_context_class->get_preedit_string = bobgui_im_multicontext_get_preedit_string;
  im_context_class->filter_keypress = bobgui_im_multicontext_filter_keypress;
  im_context_class->focus_in = bobgui_im_multicontext_focus_in;
  im_context_class->focus_out = bobgui_im_multicontext_focus_out;
  im_context_class->reset = bobgui_im_multicontext_reset;
  im_context_class->set_cursor_location = bobgui_im_multicontext_set_cursor_location;
  im_context_class->set_use_preedit = bobgui_im_multicontext_set_use_preedit;
  im_context_class->set_surrounding_with_selection = bobgui_im_multicontext_set_surrounding_with_selection;
  im_context_class->get_surrounding_with_selection = bobgui_im_multicontext_get_surrounding_with_selection;
  im_context_class->activate_osk_with_event = bobgui_im_multicontext_activate_osk_with_event;

  gobject_class->finalize = bobgui_im_multicontext_finalize;
}

static void
bobgui_im_multicontext_init (BobguiIMMulticontext *multicontext)
{
  BobguiIMMulticontextPrivate *priv;
  
  multicontext->priv = bobgui_im_multicontext_get_instance_private (multicontext);
  priv = multicontext->priv;

  priv->delegate = NULL;
  priv->use_preedit = TRUE;
  priv->have_cursor_location = FALSE;
  priv->focus_in = FALSE;
}

/**
 * bobgui_im_multicontext_new:
 *
 * Creates a new `BobguiIMMulticontext`.
 *
 * Returns: a new `BobguiIMMulticontext`.
 */
BobguiIMContext *
bobgui_im_multicontext_new (void)
{
  return g_object_new (BOBGUI_TYPE_IM_MULTICONTEXT, NULL);
}

static void
bobgui_im_multicontext_finalize (GObject *object)
{
  BobguiIMMulticontext *multicontext = BOBGUI_IM_MULTICONTEXT (object);
  BobguiIMMulticontextPrivate *priv = multicontext->priv;

  bobgui_im_multicontext_set_delegate (multicontext, NULL, TRUE);
  g_free (priv->context_id);
  g_free (priv->context_id_aux);

  G_OBJECT_CLASS (bobgui_im_multicontext_parent_class)->finalize (object);
}

static void
bobgui_im_multicontext_set_delegate (BobguiIMMulticontext *multicontext,
			          BobguiIMContext      *delegate,
			          gboolean           finalizing)
{
  BobguiIMMulticontextPrivate *priv = multicontext->priv;
  gboolean need_preedit_changed = FALSE;
  
  if (priv->delegate)
    {
      if (!finalizing)
	bobgui_im_context_reset (priv->delegate);
      
      g_signal_handlers_disconnect_by_func (priv->delegate,
					    bobgui_im_multicontext_preedit_start_cb,
					    multicontext);
      g_signal_handlers_disconnect_by_func (priv->delegate,
					    bobgui_im_multicontext_preedit_end_cb,
					    multicontext);
      g_signal_handlers_disconnect_by_func (priv->delegate,
					    bobgui_im_multicontext_preedit_changed_cb,
					    multicontext);
      g_signal_handlers_disconnect_by_func (priv->delegate,
					    bobgui_im_multicontext_commit_cb,
					    multicontext);
      g_signal_handlers_disconnect_by_func (priv->delegate,
					    bobgui_im_multicontext_retrieve_surrounding_cb,
					    multicontext);
      g_signal_handlers_disconnect_by_func (priv->delegate,
					    bobgui_im_multicontext_delete_surrounding_cb,
					    multicontext);

      if (priv->client_widget)
        bobgui_im_context_set_client_widget (priv->delegate, NULL);

      g_object_unref (priv->delegate);
      priv->delegate = NULL;

      if (!finalizing)
	need_preedit_changed = TRUE;
    }

  priv->delegate = delegate;

  if (priv->delegate)
    {
      g_object_ref (priv->delegate);

      propagate_purpose (multicontext);

      g_signal_connect (priv->delegate, "preedit-start",
			G_CALLBACK (bobgui_im_multicontext_preedit_start_cb),
			multicontext);
      g_signal_connect (priv->delegate, "preedit-end",
			G_CALLBACK (bobgui_im_multicontext_preedit_end_cb),
			multicontext);
      g_signal_connect (priv->delegate, "preedit-changed",
			G_CALLBACK (bobgui_im_multicontext_preedit_changed_cb),
			multicontext);
      g_signal_connect (priv->delegate, "commit",
			G_CALLBACK (bobgui_im_multicontext_commit_cb),
			multicontext);
      g_signal_connect (priv->delegate, "retrieve-surrounding",
			G_CALLBACK (bobgui_im_multicontext_retrieve_surrounding_cb),
			multicontext);
      g_signal_connect (priv->delegate, "delete-surrounding",
			G_CALLBACK (bobgui_im_multicontext_delete_surrounding_cb),
			multicontext);

      if (!priv->use_preedit)	/* Default is TRUE */
	bobgui_im_context_set_use_preedit (delegate, FALSE);
      if (priv->client_widget)
	bobgui_im_context_set_client_widget (delegate, priv->client_widget);
      if (priv->have_cursor_location)
	bobgui_im_context_set_cursor_location (delegate, &priv->cursor_location);
      if (priv->focus_in)
	bobgui_im_context_focus_in (delegate);
    }

  if (need_preedit_changed)
    g_signal_emit_by_name (multicontext, "preedit-changed");
}

static const char *
get_effective_context_id (BobguiIMMulticontext *multicontext)
{
  BobguiIMMulticontextPrivate *priv = multicontext->priv;
  GdkDisplay *display;

  if (priv->context_id_aux)
    return priv->context_id_aux;

  if (priv->client_widget)
    display = bobgui_widget_get_display (priv->client_widget);
  else
    display = gdk_display_get_default ();

  return _bobgui_im_module_get_default_context_id (display);
}

static BobguiIMContext *
bobgui_im_multicontext_get_delegate (BobguiIMMulticontext *multicontext)
{
  BobguiIMMulticontextPrivate *priv = multicontext->priv;

  if (!priv->delegate)
    {
      BobguiIMContext *delegate;

      g_free (priv->context_id);

      priv->context_id = g_strdup (get_effective_context_id (multicontext));

      delegate = _bobgui_im_module_create (priv->context_id);
      if (delegate)
        {
          bobgui_im_multicontext_set_delegate (multicontext, delegate, FALSE);
          g_object_unref (delegate);
        }
    }

  return priv->delegate;
}

static void
im_module_setting_changed (BobguiSettings       *settings, 
                           GParamSpec        *pspec,
                           BobguiIMMulticontext *self)
{
  bobgui_im_multicontext_set_delegate (self, NULL, FALSE);
}

static void
bobgui_im_multicontext_set_client_widget (BobguiIMContext *context,
				       BobguiWidget    *widget)
{
  BobguiIMMulticontext *self = BOBGUI_IM_MULTICONTEXT (context);
  BobguiIMMulticontextPrivate *priv = self->priv;
  BobguiIMContext *delegate;
  BobguiSettings *settings;

  if (priv->client_widget == widget)
    return;

  bobgui_im_multicontext_set_delegate (self, NULL, TRUE);

  if (priv->client_widget != NULL)
    {
      settings = bobgui_widget_get_settings (priv->client_widget);

      g_signal_handlers_disconnect_by_func (settings,
                                            im_module_setting_changed,
                                            self);
    }

  priv->client_widget = widget;

  if (widget)
    {
      settings = bobgui_widget_get_settings (widget);

      g_signal_connect (settings, "notify::bobgui-im-module",
                        G_CALLBACK (im_module_setting_changed),
                        self);

      delegate = bobgui_im_multicontext_get_delegate (self);
      if (delegate)
        bobgui_im_context_set_client_widget (delegate, widget);
    }
}

static void
bobgui_im_multicontext_get_preedit_string (BobguiIMContext   *context,
					char          **str,
					PangoAttrList **attrs,
					int            *cursor_pos)
{
  BobguiIMMulticontext *multicontext = BOBGUI_IM_MULTICONTEXT (context);
  BobguiIMContext *delegate = bobgui_im_multicontext_get_delegate (multicontext);

  if (delegate)
    bobgui_im_context_get_preedit_string (delegate, str, attrs, cursor_pos);
  else
    {
      if (str)
	*str = g_strdup ("");
      if (attrs)
	*attrs = pango_attr_list_new ();
    }
}

static gboolean
bobgui_im_multicontext_filter_keypress (BobguiIMContext *context,
				     GdkEvent     *event)
{
  BobguiIMMulticontext *multicontext = BOBGUI_IM_MULTICONTEXT (context);
  BobguiIMContext *delegate = bobgui_im_multicontext_get_delegate (multicontext);
  guint keyval, state;

  if (delegate)
    {
      return bobgui_im_context_filter_keypress (delegate, event);
    }
  else
    {
      GdkModifierType no_text_input_mask;

      keyval = gdk_key_event_get_keyval (event);
      state = gdk_event_get_modifier_state (event);

      no_text_input_mask = GDK_ALT_MASK|GDK_CONTROL_MASK;

      if (gdk_event_get_event_type (event) == GDK_KEY_PRESS &&
          (state & no_text_input_mask) == 0)
        {
          gunichar ch;

          ch = gdk_keyval_to_unicode (keyval);
          if (ch != 0 && !g_unichar_iscntrl (ch))
            {
              int len;
              char buf[10];

              len = g_unichar_to_utf8 (ch, buf);
              buf[len] = '\0';

              g_signal_emit_by_name (multicontext, "commit", buf);

              return TRUE;
            }
        }
    }

  return FALSE;
}

static void
bobgui_im_multicontext_focus_in (BobguiIMContext   *context)
{
  BobguiIMMulticontext *multicontext = BOBGUI_IM_MULTICONTEXT (context);
  BobguiIMMulticontextPrivate *priv = multicontext->priv;
  BobguiIMContext *delegate = bobgui_im_multicontext_get_delegate (multicontext);

  priv->focus_in = TRUE;

  if (delegate)
    bobgui_im_context_focus_in (delegate);
}

static void
bobgui_im_multicontext_focus_out (BobguiIMContext   *context)
{
  BobguiIMMulticontext *multicontext = BOBGUI_IM_MULTICONTEXT (context);
  BobguiIMMulticontextPrivate *priv = multicontext->priv;
  BobguiIMContext *delegate = bobgui_im_multicontext_get_delegate (multicontext);

  priv->focus_in = FALSE;

  if (delegate)
    bobgui_im_context_focus_out (delegate);
}

static void
bobgui_im_multicontext_reset (BobguiIMContext   *context)
{
  BobguiIMMulticontext *multicontext = BOBGUI_IM_MULTICONTEXT (context);
  BobguiIMContext *delegate = bobgui_im_multicontext_get_delegate (multicontext);

  if (delegate)
    bobgui_im_context_reset (delegate);
}

static void
bobgui_im_multicontext_set_cursor_location (BobguiIMContext   *context,
					 GdkRectangle   *area)
{
  BobguiIMMulticontext *multicontext = BOBGUI_IM_MULTICONTEXT (context);
  BobguiIMMulticontextPrivate *priv = multicontext->priv;
  BobguiIMContext *delegate = bobgui_im_multicontext_get_delegate (multicontext);

  priv->have_cursor_location = TRUE;
  priv->cursor_location = *area;

  if (delegate)
    bobgui_im_context_set_cursor_location (delegate, area);
}

static void
bobgui_im_multicontext_set_use_preedit (BobguiIMContext   *context,
				     gboolean	    use_preedit)
{
  BobguiIMMulticontext *multicontext = BOBGUI_IM_MULTICONTEXT (context);
  BobguiIMMulticontextPrivate *priv = multicontext->priv;
  BobguiIMContext *delegate = bobgui_im_multicontext_get_delegate (multicontext);

  use_preedit = use_preedit != FALSE;

  priv->use_preedit = use_preedit;

  if (delegate)
    bobgui_im_context_set_use_preedit (delegate, use_preedit);
}

static gboolean
bobgui_im_multicontext_get_surrounding_with_selection (BobguiIMContext  *context,
                                                    char         **text,
                                                    int           *cursor_index,
                                                    int           *anchor_index)
{
  BobguiIMMulticontext *multicontext = BOBGUI_IM_MULTICONTEXT (context);
  BobguiIMContext *delegate = bobgui_im_multicontext_get_delegate (multicontext);

  if (delegate)
    return bobgui_im_context_get_surrounding_with_selection (delegate, text, cursor_index, anchor_index);
  else
    {
      if (text)
        *text = NULL;
      if (cursor_index)
        *cursor_index = 0;
      if (anchor_index)
        *anchor_index = 0;

      return FALSE;
    }
}

static void
bobgui_im_multicontext_set_surrounding_with_selection (BobguiIMContext *context,
                                                    const char   *text,
                                                    int           len,
                                                    int           cursor_index,
                                                    int           anchor_index)
{
  BobguiIMMulticontext *multicontext = BOBGUI_IM_MULTICONTEXT (context);
  BobguiIMContext *delegate = bobgui_im_multicontext_get_delegate (multicontext);

  if (delegate)
    bobgui_im_context_set_surrounding_with_selection (delegate, text, len, cursor_index, anchor_index);
}

static gboolean
bobgui_im_multicontext_activate_osk_with_event (BobguiIMContext *context,
                                             GdkEvent     *event)
{
  BobguiIMMulticontext *multicontext = BOBGUI_IM_MULTICONTEXT (context);
  BobguiIMContext *delegate = bobgui_im_multicontext_get_delegate (multicontext);

  if (delegate)
    return bobgui_im_context_activate_osk (delegate, event);
  else
    return FALSE;
}

static void
bobgui_im_multicontext_preedit_start_cb   (BobguiIMContext      *delegate,
					BobguiIMMulticontext *multicontext)
{
  g_signal_emit_by_name (multicontext, "preedit-start");
}

static void
bobgui_im_multicontext_preedit_end_cb (BobguiIMContext      *delegate,
				    BobguiIMMulticontext *multicontext)
{
  g_signal_emit_by_name (multicontext, "preedit-end");
}

static void
bobgui_im_multicontext_preedit_changed_cb (BobguiIMContext      *delegate,
					BobguiIMMulticontext *multicontext)
{
  g_signal_emit_by_name (multicontext, "preedit-changed");
}

static void
bobgui_im_multicontext_commit_cb (BobguiIMContext      *delegate,
			       const char        *str,
			       BobguiIMMulticontext *multicontext)
{
  g_signal_emit_by_name (multicontext, "commit", str);
}

static gboolean
bobgui_im_multicontext_retrieve_surrounding_cb (BobguiIMContext      *delegate,
					     BobguiIMMulticontext *multicontext)
{
  gboolean result;
  
  g_signal_emit_by_name (multicontext, "retrieve-surrounding", &result);

  return result;
}

static gboolean
bobgui_im_multicontext_delete_surrounding_cb (BobguiIMContext      *delegate,
					   int                offset,
					   int                n_chars,
					   BobguiIMMulticontext *multicontext)
{
  gboolean result;
  
  g_signal_emit_by_name (multicontext, "delete-surrounding",
			 offset, n_chars, &result);

  return result;
}

/**
 * bobgui_im_multicontext_get_context_id:
 * @context: a `BobguiIMMulticontext`
 *
 * Gets the id of the currently active delegate of the @context.
 *
 * Returns: the id of the currently active delegate
 */
const char *
bobgui_im_multicontext_get_context_id (BobguiIMMulticontext *context)
{
  BobguiIMMulticontextPrivate *priv = context->priv;

  g_return_val_if_fail (BOBGUI_IS_IM_MULTICONTEXT (context), NULL);

  if (priv->context_id == NULL)
    bobgui_im_multicontext_get_delegate (context);

  return priv->context_id;
}

/**
 * bobgui_im_multicontext_set_context_id:
 * @context: a `BobguiIMMulticontext`
 * @context_id: (nullable): the id to use
 *
 * Sets the context id for @context.
 *
 * This causes the currently active delegate of @context to be
 * replaced by the delegate corresponding to the new context id.
 *
 * Setting this to a non-%NULL value overrides the system-wide
 * IM module setting. See the [property@Bobgui.Settings:bobgui-im-module]
 * property.
 */
void
bobgui_im_multicontext_set_context_id (BobguiIMMulticontext *context,
                                    const char        *context_id)
{
  BobguiIMMulticontextPrivate *priv;

  g_return_if_fail (BOBGUI_IS_IM_MULTICONTEXT (context));

  priv = context->priv;

  bobgui_im_context_reset (BOBGUI_IM_CONTEXT (context));
  g_free (priv->context_id_aux);
  priv->context_id_aux = g_strdup (context_id);
  bobgui_im_multicontext_set_delegate (context, NULL, FALSE);
}

static void
propagate_purpose (BobguiIMMulticontext *context)
{
  BobguiInputPurpose purpose;
  BobguiInputHints hints;

  if (context->priv->delegate == NULL)
    return;

  g_object_get (context, "input-purpose", &purpose, NULL);
  g_object_set (context->priv->delegate, "input-purpose", purpose, NULL);

  g_object_get (context, "input-hints", &hints, NULL);
  g_object_set (context->priv->delegate, "input-hints", hints, NULL);
}

static void
bobgui_im_multicontext_notify (GObject      *object,
                            GParamSpec   *pspec)
{
  propagate_purpose (BOBGUI_IM_MULTICONTEXT (object));
}
