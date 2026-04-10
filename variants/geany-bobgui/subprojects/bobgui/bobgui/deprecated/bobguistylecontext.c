/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2010 Carlos Garnacho <carlosg@gnome.org>
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

#include "bobguistylecontextprivate.h"

#include <gdk/gdk.h>

#include "bobguicsscolorvalueprivate.h"
#include "bobguicssnumbervalueprivate.h"
#include "bobguicsstransientnodeprivate.h"
#include "bobguidebug.h"
#include "bobguiprivate.h"
#include "bobguisettings.h"
#include "bobguisettingsprivate.h"
#include "deprecated/bobguirender.h"


/**
 * BobguiStyleContext:
 *
 * `BobguiStyleContext` stores styling information affecting a widget.
 *
 * In order to construct the final style information, `BobguiStyleContext`
 * queries information from all attached `BobguiStyleProviders`. Style
 * providers can be either attached explicitly to the context through
 * [method@Bobgui.StyleContext.add_provider], or to the display through
 * [func@Bobgui.StyleContext.add_provider_for_display]. The resulting
 * style is a combination of all providers’ information in priority order.
 *
 * For BOBGUI widgets, any `BobguiStyleContext` returned by
 * [method@Bobgui.Widget.get_style_context] will already have a `GdkDisplay`
 * and RTL/LTR information set. The style context will also be updated
 * automatically if any of these settings change on the widget.
 *
 * ## Style Classes
 *
 * Widgets can add style classes to their context, which can be used to associate
 * different styles by class. The documentation for individual widgets lists
 * which style classes it uses itself, and which style classes may be added by
 * applications to affect their appearance.
 *
 * # Custom styling in UI libraries and applications
 *
 * If you are developing a library with custom widgets that render differently
 * than standard components, you may need to add a `BobguiStyleProvider` yourself
 * with the %BOBGUI_STYLE_PROVIDER_PRIORITY_FALLBACK priority, either a
 * `BobguiCssProvider` or a custom object implementing the `BobguiStyleProvider`
 * interface. This way themes may still attempt to style your UI elements in
 * a different way if needed so.
 *
 * If you are using custom styling on an applications, you probably want then
 * to make your style information prevail to the theme’s, so you must use
 * a `BobguiStyleProvider` with the %BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION
 * priority, keep in mind that the user settings in
 * `XDG_CONFIG_HOME/bobgui-4.0/bobgui.css` will
 * still take precedence over your changes, as it uses the
 * %BOBGUI_STYLE_PROVIDER_PRIORITY_USER priority.
 *
 * Deprecated: 4.10: The relevant API has been moved to [class@Bobgui.Widget]
 *   where applicable; otherwise, there is no replacement for querying the
 *   style machinery. Stylable UI elements should use widgets.
 */

#define CURSOR_ASPECT_RATIO (0.04)

struct _BobguiStyleContextPrivate
{
  GdkDisplay *display;

  guint cascade_changed_id;
  BobguiStyleCascade *cascade;
  BobguiCssNode *cssnode;
  GSList *saved_nodes;
};
typedef struct _BobguiStyleContextPrivate BobguiStyleContextPrivate;

enum {
  PROP_0,
  PROP_DISPLAY,
  LAST_PROP
};

static GParamSpec *properties[LAST_PROP] = { NULL, };

static void bobgui_style_context_finalize (GObject *object);

static void bobgui_style_context_impl_set_property (GObject      *object,
                                                 guint         prop_id,
                                                 const GValue *value,
                                                 GParamSpec   *pspec);
static void bobgui_style_context_impl_get_property (GObject      *object,
                                                 guint         prop_id,
                                                 GValue       *value,
                                                 GParamSpec   *pspec);

static BobguiCssNode * bobgui_style_context_get_root (BobguiStyleContext *context);

G_DEFINE_TYPE_WITH_PRIVATE (BobguiStyleContext, bobgui_style_context, G_TYPE_OBJECT)

static void
bobgui_style_context_class_init (BobguiStyleContextClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bobgui_style_context_finalize;
  object_class->set_property = bobgui_style_context_impl_set_property;
  object_class->get_property = bobgui_style_context_impl_get_property;

  /**
   * BobguiStyleContext:display:
   *
   * The display of the style context.
   */
  properties[PROP_DISPLAY] =
      g_param_spec_object ("display", NULL, NULL,
                           GDK_TYPE_DISPLAY,
                           BOBGUI_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, properties);
}

static void
bobgui_style_context_pop_style_node (BobguiStyleContext *context)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);

  g_return_if_fail (priv->saved_nodes != NULL);

  if (BOBGUI_IS_CSS_TRANSIENT_NODE (priv->cssnode))
    bobgui_css_node_set_parent (priv->cssnode, NULL);
  g_object_unref (priv->cssnode);
  priv->cssnode = priv->saved_nodes->data;
  priv->saved_nodes = g_slist_remove (priv->saved_nodes, priv->cssnode);
}

static void
bobgui_style_context_cascade_changed (BobguiStyleCascade *cascade,
                                   BobguiStyleContext *context)
{
  bobgui_css_node_invalidate_style_provider (bobgui_style_context_get_root (context));
}

static void
bobgui_style_context_set_cascade (BobguiStyleContext *context,
                               BobguiStyleCascade *cascade)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);

  if (priv->cascade == cascade)
    return;

  if (priv->cascade)
    {
      g_signal_handler_disconnect (priv->cascade, priv->cascade_changed_id);
      priv->cascade_changed_id = 0;
      g_object_unref (priv->cascade);
    }

  if (cascade)
    {
      g_object_ref (cascade);
      priv->cascade_changed_id = g_signal_connect (cascade,
                                                   "bobgui-private-changed",
                                                   G_CALLBACK (bobgui_style_context_cascade_changed),
                                                   context);
    }

  priv->cascade = cascade;

  if (cascade && priv->cssnode != NULL)
    bobgui_style_context_cascade_changed (cascade, context);
}

static void
bobgui_style_context_init (BobguiStyleContext *context)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);

  priv->display = gdk_display_get_default ();

  if (priv->display == NULL)
    g_error ("Can't create a BobguiStyleContext without a display connection");

  bobgui_style_context_set_cascade (context,
                                 _bobgui_settings_get_style_cascade (bobgui_settings_get_for_display (priv->display), 1));
}

static void
bobgui_style_context_finalize (GObject *object)
{
  BobguiStyleContext *context = BOBGUI_STYLE_CONTEXT (object);
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);

  while (priv->saved_nodes)
    bobgui_style_context_pop_style_node (context);

  bobgui_style_context_set_cascade (context, NULL);

  if (priv->cssnode)
    g_object_unref (priv->cssnode);

  G_OBJECT_CLASS (bobgui_style_context_parent_class)->finalize (object);
}

static void
bobgui_style_context_impl_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  BobguiStyleContext *context = BOBGUI_STYLE_CONTEXT (object);

  switch (prop_id)
    {
    case PROP_DISPLAY:
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      bobgui_style_context_set_display (context, g_value_get_object (value));
G_GNUC_END_IGNORE_DEPRECATIONS
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_style_context_impl_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  BobguiStyleContext *context = BOBGUI_STYLE_CONTEXT (object);
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);

  switch (prop_id)
    {
    case PROP_DISPLAY:
      g_value_set_object (value, priv->display);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* returns TRUE if someone called bobgui_style_context_save() but hasn’t
 * called bobgui_style_context_restore() yet.
 * In those situations we don’t invalidate the context when somebody
 * changes state/classes.
 */
static gboolean
bobgui_style_context_is_saved (BobguiStyleContext *context)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);

  return priv->saved_nodes != NULL;
}

static BobguiCssNode *
bobgui_style_context_get_root (BobguiStyleContext *context)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);

  if (priv->saved_nodes != NULL)
    return g_slist_last (priv->saved_nodes)->data;
  else
    return priv->cssnode;
}

BobguiStyleProvider *
bobgui_style_context_get_style_provider (BobguiStyleContext *context)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);

  return BOBGUI_STYLE_PROVIDER (priv->cascade);
}

static gboolean
bobgui_style_context_has_custom_cascade (BobguiStyleContext *context)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);
  BobguiSettings *settings = bobgui_settings_get_for_display (priv->display);

  return priv->cascade != _bobgui_settings_get_style_cascade (settings, _bobgui_style_cascade_get_scale (priv->cascade));
}

BobguiCssStyle *
bobgui_style_context_lookup_style (BobguiStyleContext *context)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);

  /* Code will recreate style if it was changed */
  return bobgui_css_node_get_style (priv->cssnode);
}

BobguiCssNode*
bobgui_style_context_get_node (BobguiStyleContext *context)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);

  return priv->cssnode;
}

BobguiStyleContext *
bobgui_style_context_new_for_node (BobguiCssNode *node)
{
  BobguiStyleContext *context;
  BobguiStyleContextPrivate *priv;

  g_return_val_if_fail (BOBGUI_IS_CSS_NODE (node), NULL);

  context = g_object_new (BOBGUI_TYPE_STYLE_CONTEXT, NULL);
  priv = bobgui_style_context_get_instance_private (context);
  priv->cssnode = g_object_ref (node);

  return context;
}

/**
 * bobgui_style_context_add_provider:
 * @context: a `BobguiStyleContext`
 * @provider: a `BobguiStyleProvider`
 * @priority: the priority of the style provider. The lower
 *   it is, the earlier it will be used in the style construction.
 *   Typically this will be in the range between
 *   %BOBGUI_STYLE_PROVIDER_PRIORITY_FALLBACK and
 *   %BOBGUI_STYLE_PROVIDER_PRIORITY_USER
 *
 * Adds a style provider to @context, to be used in style construction.
 *
 * Note that a style provider added by this function only affects
 * the style of the widget to which @context belongs. If you want
 * to affect the style of all widgets, use
 * [func@Bobgui.StyleContext.add_provider_for_display].
 *
 * Note: If both priorities are the same, a `BobguiStyleProvider`
 * added through this function takes precedence over another added
 * through [func@Bobgui.StyleContext.add_provider_for_display].
 *
 * Deprecated: 4.10: Use style classes instead
 */
void
bobgui_style_context_add_provider (BobguiStyleContext  *context,
                                BobguiStyleProvider *provider,
                                guint             priority)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);

  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));
  g_return_if_fail (BOBGUI_IS_STYLE_PROVIDER (provider));

  if (!bobgui_style_context_has_custom_cascade (context))
    {
      BobguiStyleCascade *new_cascade;

      new_cascade = _bobgui_style_cascade_new ();
      _bobgui_style_cascade_set_scale (new_cascade, _bobgui_style_cascade_get_scale (priv->cascade));
      _bobgui_style_cascade_set_parent (new_cascade,
                                     _bobgui_settings_get_style_cascade (bobgui_settings_get_for_display (priv->display), 1));
      _bobgui_style_cascade_add_provider (new_cascade, provider, priority);
      bobgui_style_context_set_cascade (context, new_cascade);
      g_object_unref (new_cascade);
    }
  else
    {
      _bobgui_style_cascade_add_provider (priv->cascade, provider, priority);
    }
}

/**
 * bobgui_style_context_remove_provider:
 * @context: a `BobguiStyleContext`
 * @provider: a `BobguiStyleProvider`
 *
 * Removes @provider from the style providers list in @context.
 *
 * Deprecated: 4.10
 */
void
bobgui_style_context_remove_provider (BobguiStyleContext  *context,
                                   BobguiStyleProvider *provider)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);

  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));
  g_return_if_fail (BOBGUI_IS_STYLE_PROVIDER (provider));

  if (!bobgui_style_context_has_custom_cascade (context))
    return;

  _bobgui_style_cascade_remove_provider (priv->cascade, provider);
}

/**
 * bobgui_style_context_set_state:
 * @context: a `BobguiStyleContext`
 * @flags: state to represent
 *
 * Sets the state to be used for style matching.
 *
 * Deprecated: 4.10: You should not use this api
 */
void
bobgui_style_context_set_state (BobguiStyleContext *context,
                             BobguiStateFlags    flags)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);

  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));

  bobgui_css_node_set_state (priv->cssnode, flags);
}

/**
 * bobgui_style_context_get_state:
 * @context: a `BobguiStyleContext`
 *
 * Returns the state used for style matching.
 *
 * This method should only be used to retrieve the `BobguiStateFlags`
 * to pass to `BobguiStyleContext` methods, like
 * [method@Bobgui.StyleContext.get_padding].
 * If you need to retrieve the current state of a `BobguiWidget`, use
 * [method@Bobgui.Widget.get_state_flags].
 *
 * Returns: the state flags
 *
 * Deprecated: 4.10: Use [method@Bobgui.Widget.get_state_flags] instead
 **/
BobguiStateFlags
bobgui_style_context_get_state (BobguiStyleContext *context)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);

  g_return_val_if_fail (BOBGUI_IS_STYLE_CONTEXT (context), 0);

  return bobgui_css_node_get_state (priv->cssnode);
}

/**
 * bobgui_style_context_set_scale:
 * @context: a `BobguiStyleContext`
 * @scale: scale
 *
 * Sets the scale to use when getting image assets for the style.
 *
 * Deprecated: 4.10: You should not use this api
 **/
void
bobgui_style_context_set_scale (BobguiStyleContext *context,
                             int              scale)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);

  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));

  if (scale == _bobgui_style_cascade_get_scale (priv->cascade))
    return;

  if (bobgui_style_context_has_custom_cascade (context))
    {
      _bobgui_style_cascade_set_scale (priv->cascade, scale);
    }
  else
    {
      BobguiStyleCascade *new_cascade;

      new_cascade = _bobgui_settings_get_style_cascade (bobgui_settings_get_for_display (priv->display),
                                                     scale);
      bobgui_style_context_set_cascade (context, new_cascade);
    }
}

/**
 * bobgui_style_context_get_scale:
 * @context: a `BobguiStyleContext`
 *
 * Returns the scale used for assets.
 *
 * Returns: the scale
 *
 * Deprecated: 4.10: Use [method@Bobgui.Widget.get_scale_factor] instead
 **/
int
bobgui_style_context_get_scale (BobguiStyleContext *context)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);

  g_return_val_if_fail (BOBGUI_IS_STYLE_CONTEXT (context), 0);

  return _bobgui_style_cascade_get_scale (priv->cascade);
}

/*
 * bobgui_style_context_save_to_node:
 * @context: a `BobguiStyleContext`
 * @node: the node to save to
 *
 * Saves the @context state to a node.
 *
 * This allows temporary modifications done through
 * [method@Bobgui.StyleContext.add_class],
 * [method@Bobgui.StyleContext.remove_class],
 * [method@Bobgui.StyleContext.set_state] etc.
 *
 * Rendering using [func@Bobgui.render_background] or similar
 * functions are done using the given @node.
 *
 * To undo, call [method@Bobgui.StyleContext.restore].
 * The matching call to [method@Bobgui.StyleContext.restore]
 * must be done before BOBGUI returns to the main loop.
 */
void
bobgui_style_context_save_to_node (BobguiStyleContext *context,
                                BobguiCssNode      *node)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);

  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));
  g_return_if_fail (BOBGUI_IS_CSS_NODE (node));

  priv->saved_nodes = g_slist_prepend (priv->saved_nodes, priv->cssnode);
  priv->cssnode = g_object_ref (node);
}

/**
 * bobgui_style_context_save:
 * @context: a `BobguiStyleContext`
 *
 * Saves the @context state.
 *
 * This allows temporary modifications done through
 * [method@Bobgui.StyleContext.add_class],
 * [method@Bobgui.StyleContext.remove_class],
 * [method@Bobgui.StyleContext.set_state] to be quickly
 * reverted in one go through [method@Bobgui.StyleContext.restore].
 *
 * The matching call to [method@Bobgui.StyleContext.restore]
 * must be done before BOBGUI returns to the main loop.
 *
 * Deprecated: 4.10: This API will be removed in BOBGUI 5
 **/
void
bobgui_style_context_save (BobguiStyleContext *context)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);
  BobguiCssNode *cssnode;

  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));


  /* Make sure we have the style existing. It is the
   * parent of the new saved node after all.
   */
  if (!bobgui_style_context_is_saved (context))
    bobgui_style_context_lookup_style (context);

  cssnode = bobgui_css_transient_node_new (priv->cssnode);
  bobgui_css_node_set_parent (cssnode, bobgui_style_context_get_root (context));
  bobgui_style_context_save_to_node (context, cssnode);

  g_object_unref (cssnode);
}

/**
 * bobgui_style_context_restore:
 * @context: a `BobguiStyleContext`
 *
 * Restores @context state to a previous stage.
 *
 * See [method@Bobgui.StyleContext.save].
 *
 * Deprecated: 4.10: This API will be removed in BOBGUI 5
 **/
void
bobgui_style_context_restore (BobguiStyleContext *context)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);

  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));

  if (priv->saved_nodes == NULL)
    {
      g_warning ("Unpaired bobgui_style_context_restore() call");
      return;
    }

  bobgui_style_context_pop_style_node (context);
}

/**
 * bobgui_style_context_add_class:
 * @context: a `BobguiStyleContext`
 * @class_name: class name to use in styling
 *
 * Adds a style class to @context, so later uses of the
 * style context will make use of this new class for styling.
 *
 * In the CSS file format, a `BobguiEntry` defining a “search”
 * class, would be matched by:
 *
 * ```css
 * entry.search { ... }
 * ```
 *
 * While any widget defining a “search” class would be
 * matched by:
 * ```css
 * .search { ... }
 * ```
 * Deprecated: 4.10: Use [method@Bobgui.Widget.add_css_class] instead
 */
void
bobgui_style_context_add_class (BobguiStyleContext *context,
                             const char      *class_name)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);
  GQuark class_quark;

  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));
  g_return_if_fail (class_name != NULL);

  class_quark = g_quark_from_string (class_name);

  bobgui_css_node_add_class (priv->cssnode, class_quark);
}

/**
 * bobgui_style_context_remove_class:
 * @context: a `BobguiStyleContext`
 * @class_name: class name to remove
 *
 * Removes @class_name from @context.
 *
 * Deprecated: 4.10: Use [method@Bobgui.Widget.remove_css_class] instead
 */
void
bobgui_style_context_remove_class (BobguiStyleContext *context,
                                const char      *class_name)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);
  GQuark class_quark;

  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));
  g_return_if_fail (class_name != NULL);

  class_quark = g_quark_try_string (class_name);
  if (!class_quark)
    return;

  bobgui_css_node_remove_class (priv->cssnode, class_quark);
}

/**
 * bobgui_style_context_has_class:
 * @context: a `BobguiStyleContext`
 * @class_name: a class name
 *
 * Returns %TRUE if @context currently has defined the
 * given class name.
 *
 * Returns: %TRUE if @context has @class_name defined
 *
 * Deprecated: 4.10: Use [method@Bobgui.Widget.has_css_class] instead
 **/
gboolean
bobgui_style_context_has_class (BobguiStyleContext *context,
                             const char      *class_name)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);
  GQuark class_quark;

  g_return_val_if_fail (BOBGUI_IS_STYLE_CONTEXT (context), FALSE);
  g_return_val_if_fail (class_name != NULL, FALSE);

  class_quark = g_quark_try_string (class_name);
  if (!class_quark)
    return FALSE;

  return bobgui_css_node_has_class (priv->cssnode, class_quark);
}

BobguiCssValue *
_bobgui_style_context_peek_property (BobguiStyleContext *context,
                                  guint            property_id)
{
  BobguiCssStyle *values = bobgui_style_context_lookup_style (context);

  return bobgui_css_style_get_value (values, property_id);
}

/**
 * bobgui_style_context_set_display:
 * @context: a `BobguiStyleContext`
 * @display: a `GdkDisplay`
 *
 * Attaches @context to the given display.
 *
 * The display is used to add style information from “global”
 * style providers, such as the display's `BobguiSettings` instance.
 *
 * If you are using a `BobguiStyleContext` returned from
 * [method@Bobgui.Widget.get_style_context], you do not need to
 * call this yourself.
 *
 * Deprecated: 4.10: You should not use this api
 */
void
bobgui_style_context_set_display (BobguiStyleContext *context,
                               GdkDisplay      *display)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);
  BobguiStyleCascade *display_cascade;

  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));
  g_return_if_fail (GDK_IS_DISPLAY (display));

  if (priv->display == display)
    return;

  if (bobgui_style_context_has_custom_cascade (context))
    {
      display_cascade = _bobgui_settings_get_style_cascade (bobgui_settings_get_for_display (display), 1);
      _bobgui_style_cascade_set_parent (priv->cascade, display_cascade);
    }
  else
    {
      display_cascade = _bobgui_settings_get_style_cascade (bobgui_settings_get_for_display (display),
                                                        _bobgui_style_cascade_get_scale (priv->cascade));
      bobgui_style_context_set_cascade (context, display_cascade);
    }

  priv->display = display;

  g_object_notify_by_pspec (G_OBJECT (context), properties[PROP_DISPLAY]);
}

/**
 * bobgui_style_context_get_display:
 * @context: a `BobguiStyleContext`
 *
 * Returns the `GdkDisplay` to which @context is attached.
 *
 * Returns: (transfer none): a `GdkDisplay`.
 *
 * Deprecated: 4.10: Use [method@Bobgui.Widget.get_display] instead
 */
GdkDisplay *
bobgui_style_context_get_display (BobguiStyleContext *context)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);

  g_return_val_if_fail (BOBGUI_IS_STYLE_CONTEXT (context), NULL);

  return priv->display;
}

static gboolean
bobgui_style_context_resolve_color (BobguiStyleContext    *context,
                                 BobguiCssValue        *color,
                                 GdkRGBA            *result)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);
  BobguiCssValue *val, *val2;
  BobguiCssComputeContext ctx = { NULL, };

  g_return_val_if_fail (BOBGUI_IS_STYLE_CONTEXT (context), FALSE);
  g_return_val_if_fail (color != NULL, FALSE);
  g_return_val_if_fail (result != NULL, FALSE);

  ctx.provider = BOBGUI_STYLE_PROVIDER (priv->cascade);
  ctx.style = bobgui_css_node_get_style (priv->cssnode);
  if (bobgui_css_node_get_parent (priv->cssnode))
    ctx.parent_style = bobgui_css_node_get_style (bobgui_css_node_get_parent (priv->cssnode));

  val = bobgui_css_value_compute (color, BOBGUI_CSS_PROPERTY_COLOR, &ctx);
  val2 = bobgui_css_value_resolve (val, &ctx, _bobgui_style_context_peek_property (context, BOBGUI_CSS_PROPERTY_COLOR));

  *result = *bobgui_css_color_value_get_rgba (val2);

  bobgui_css_value_unref (val);
  bobgui_css_value_unref (val2);

  return TRUE;
}

/**
 * bobgui_style_context_lookup_color:
 * @context: a `BobguiStyleContext`
 * @color_name: color name to lookup
 * @color: (out): Return location for the looked up color
 *
 * Looks up and resolves a color name in the @context color map.
 *
 * Returns: %TRUE if @color_name was found and resolved, %FALSE otherwise
 *
 * Deprecated: 4.10: This api will be removed in BOBGUI 5
 */
gboolean
bobgui_style_context_lookup_color (BobguiStyleContext *context,
                                const char      *color_name,
                                GdkRGBA         *color)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);
  BobguiCssValue *value;

  g_return_val_if_fail (BOBGUI_IS_STYLE_CONTEXT (context), FALSE);
  g_return_val_if_fail (color_name != NULL, FALSE);
  g_return_val_if_fail (color != NULL, FALSE);

  value = bobgui_style_provider_get_color (BOBGUI_STYLE_PROVIDER (priv->cascade), color_name);
  if (value == NULL)
    return FALSE;

  return bobgui_style_context_resolve_color (context, value, color);
}

/**
 * bobgui_style_context_get_color:
 * @context: a `BobguiStyleContext`
 * @color: (out): return value for the foreground color
 *
 * Gets the foreground color for a given state.
 *
 * Deprecated: 4.10: Use [method@Bobgui.Widget.get_color] instead
 */
void
bobgui_style_context_get_color (BobguiStyleContext *context,
                             GdkRGBA         *color)
{
  g_return_if_fail (color != NULL);
  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));

  *color = *bobgui_css_color_value_get_rgba (_bobgui_style_context_peek_property (context, BOBGUI_CSS_PROPERTY_COLOR));
}

/**
 * bobgui_style_context_get_border:
 * @context: a `BobguiStyleContext`
 * @border: (out): return value for the border settings
 *
 * Gets the border for a given state as a `BobguiBorder`.
 *
 * Deprecated: 4.10: This api will be removed in BOBGUI 5
 */
void
bobgui_style_context_get_border (BobguiStyleContext *context,
                              BobguiBorder       *border)
{
  BobguiCssStyle *style;

  g_return_if_fail (border != NULL);
  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));

  style = bobgui_style_context_lookup_style (context);

  border->top = round (bobgui_css_number_value_get (style->border->border_top_width, 100));
  border->right = round (bobgui_css_number_value_get (style->border->border_right_width, 100));
  border->bottom = round (bobgui_css_number_value_get (style->border->border_bottom_width, 100));
  border->left = round (bobgui_css_number_value_get (style->border->border_left_width, 100));
}

/**
 * bobgui_style_context_get_padding:
 * @context: a `BobguiStyleContext`
 * @padding: (out): return value for the padding settings
 *
 * Gets the padding for a given state as a `BobguiBorder`.
 *
 * Deprecated: 4.10: This api will be removed in BOBGUI 5
 */
void
bobgui_style_context_get_padding (BobguiStyleContext *context,
                               BobguiBorder       *padding)
{
  BobguiCssStyle *style;

  g_return_if_fail (padding != NULL);
  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));

  style = bobgui_style_context_lookup_style (context);

  padding->top = round (bobgui_css_number_value_get (style->size->padding_top, 100));
  padding->right = round (bobgui_css_number_value_get (style->size->padding_right, 100));
  padding->bottom = round (bobgui_css_number_value_get (style->size->padding_bottom, 100));
  padding->left = round (bobgui_css_number_value_get (style->size->padding_left, 100));
}

/**
 * bobgui_style_context_get_margin:
 * @context: a `BobguiStyleContext`
 * @margin: (out): return value for the margin settings
 *
 * Gets the margin for a given state as a `BobguiBorder`.
 *
 * Deprecated: 4.10: This api will be removed in BOBGUI 5
 */
void
bobgui_style_context_get_margin (BobguiStyleContext *context,
                              BobguiBorder       *margin)
{
  BobguiCssStyle *style;

  g_return_if_fail (margin != NULL);
  g_return_if_fail (BOBGUI_IS_STYLE_CONTEXT (context));

  style = bobgui_style_context_lookup_style (context);

  margin->top = round (bobgui_css_number_value_get (style->size->margin_top, 100));
  margin->right = round (bobgui_css_number_value_get (style->size->margin_right, 100));
  margin->bottom = round (bobgui_css_number_value_get (style->size->margin_bottom, 100));
  margin->left = round (bobgui_css_number_value_get (style->size->margin_left, 100));
}

void
_bobgui_style_context_get_cursor_color (BobguiStyleContext *context,
                                     GdkRGBA         *primary_color,
                                     GdkRGBA         *secondary_color)
{
  BobguiCssStyle *style;

  style = bobgui_style_context_lookup_style (context);

  if (primary_color)
    *primary_color = *bobgui_css_color_value_get_rgba (style->used->caret_color);

  if (secondary_color)
    *secondary_color = *bobgui_css_color_value_get_rgba (style->used->secondary_caret_color);
}

/**
 * BobguiStyleContextPrintFlags:
 * @BOBGUI_STYLE_CONTEXT_PRINT_NONE: Default value.
 * @BOBGUI_STYLE_CONTEXT_PRINT_RECURSE: Print the entire tree of
 *   CSS nodes starting at the style context's node
 * @BOBGUI_STYLE_CONTEXT_PRINT_SHOW_STYLE: Show the values of the
 *   CSS properties for each node
 * @BOBGUI_STYLE_CONTEXT_PRINT_SHOW_CHANGE: Show information about
 *   what changes affect the styles
 *
 * Flags that modify the behavior of bobgui_style_context_to_string().
 *
 * New values may be added to this enumeration.
 */

/**
 * bobgui_style_context_to_string:
 * @context: a `BobguiStyleContext`
 * @flags: Flags that determine what to print
 *
 * Converts the style context into a string representation.
 *
 * The string representation always includes information about
 * the name, state, id, visibility and style classes of the CSS
 * node that is backing @context. Depending on the flags, more
 * information may be included.
 *
 * This function is intended for testing and debugging of the
 * CSS implementation in BOBGUI. There are no guarantees about
 * the format of the returned string, it may change.
 *
 * Returns: a newly allocated string representing @context
 *
 * Deprecated: 4.10: This api will be removed in BOBGUI 5
 */
char *
bobgui_style_context_to_string (BobguiStyleContext           *context,
                             BobguiStyleContextPrintFlags  flags)
{
  BobguiStyleContextPrivate *priv = bobgui_style_context_get_instance_private (context);
  GString *string;

  g_return_val_if_fail (BOBGUI_IS_STYLE_CONTEXT (context), NULL);

  string = g_string_new ("");

  bobgui_css_node_print (priv->cssnode, (BobguiCssNodePrintFlags)flags, string, 0);

  return g_string_free (string, FALSE);
}
