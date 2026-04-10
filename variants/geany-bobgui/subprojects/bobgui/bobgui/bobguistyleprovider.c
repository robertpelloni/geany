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

#include "bobguisettingsprivate.h"
#include "bobguistyleproviderprivate.h"

#include "bobguiprivate.h"

/**
 * BobguiStyleProvider:
 *
 * An interface for style information used by [class@Bobgui.StyleContext].
 *
 * See [method@Bobgui.StyleContext.add_provider] and
 * [func@Bobgui.StyleContext.add_provider_for_display] for
 * adding `BobguiStyleProviders`.
 *
 * BOBGUI uses the `BobguiStyleProvider` implementation for CSS in
 * [class@Bobgui.CssProvider].
 */

enum {
  CHANGED,
  LAST_SIGNAL
};

G_DEFINE_INTERFACE (BobguiStyleProvider, bobgui_style_provider, G_TYPE_OBJECT)

static guint signals[LAST_SIGNAL];

static void
bobgui_style_provider_default_init (BobguiStyleProviderInterface *iface)
{
  signals[CHANGED] = g_signal_new (I_("bobgui-private-changed"),
                                   G_TYPE_FROM_INTERFACE (iface),
                                   G_SIGNAL_RUN_LAST,
                                   G_STRUCT_OFFSET (BobguiStyleProviderInterface, changed),
                                   NULL, NULL,
                                   NULL,
                                   G_TYPE_NONE, 0);

}

BobguiCssValue *
bobgui_style_provider_get_color (BobguiStyleProvider *provider,
                              const char       *name)
{
  BobguiStyleProviderInterface *iface;

  /* for compat with bobgui_symbolic_color_resolve() */
  if (provider == NULL)
    return NULL;

  bobgui_internal_return_val_if_fail (BOBGUI_IS_STYLE_PROVIDER (provider), NULL);

  iface = BOBGUI_STYLE_PROVIDER_GET_INTERFACE (provider);

  if (!iface->get_color)
    return NULL;

  return iface->get_color (provider, name);
}

BobguiCssKeyframes *
bobgui_style_provider_get_keyframes (BobguiStyleProvider *provider,
                                  const char       *name)
{
  BobguiStyleProviderInterface *iface;

  bobgui_internal_return_val_if_fail (BOBGUI_IS_STYLE_PROVIDER (provider), NULL);
  bobgui_internal_return_val_if_fail (name != NULL, NULL);

  iface = BOBGUI_STYLE_PROVIDER_GET_INTERFACE (provider);

  if (!iface->get_keyframes)
    return NULL;

  return iface->get_keyframes (provider, name);
}

void
bobgui_style_provider_lookup (BobguiStyleProvider             *provider,
                           const BobguiCountingBloomFilter *filter,
                           BobguiCssNode                   *node,
                           BobguiCssLookup                 *lookup,
                           BobguiCssChange                 *out_change)
{
  BobguiStyleProviderInterface *iface;

  bobgui_internal_return_if_fail (BOBGUI_IS_STYLE_PROVIDER (provider));
  bobgui_internal_return_if_fail (BOBGUI_IS_CSS_NODE (node));
  bobgui_internal_return_if_fail (lookup != NULL);

  if (out_change)
    *out_change = 0;

  iface = BOBGUI_STYLE_PROVIDER_GET_INTERFACE (provider);

  if (!iface->lookup)
    return;

  iface->lookup (provider, filter, node, lookup, out_change);
}

void
bobgui_style_provider_changed (BobguiStyleProvider *provider)
{
  bobgui_internal_return_if_fail (BOBGUI_IS_STYLE_PROVIDER (provider));

  g_signal_emit (provider, signals[CHANGED], 0);
}

BobguiSettings *
bobgui_style_provider_get_settings (BobguiStyleProvider *provider)
{
  BobguiStyleProviderInterface *iface;

  bobgui_internal_return_val_if_fail (BOBGUI_IS_STYLE_PROVIDER (provider), NULL);

  iface = BOBGUI_STYLE_PROVIDER_GET_INTERFACE (provider);

  if (!iface->get_settings)
    return NULL;

  return iface->get_settings (provider);
}

int
bobgui_style_provider_get_scale (BobguiStyleProvider *provider)
{
  BobguiStyleProviderInterface *iface;

  bobgui_internal_return_val_if_fail (BOBGUI_IS_STYLE_PROVIDER (provider), 1);

  iface = BOBGUI_STYLE_PROVIDER_GET_INTERFACE (provider);

  if (!iface->get_scale)
    return 1;

  return iface->get_scale (provider);
}

void
bobgui_style_provider_emit_error (BobguiStyleProvider *provider,
                               BobguiCssSection    *section,
                               GError           *error)
{
  BobguiStyleProviderInterface *iface;

  iface = BOBGUI_STYLE_PROVIDER_GET_INTERFACE (provider);

  if (iface->emit_error)
    iface->emit_error (provider, section, error);
}

gboolean
bobgui_style_provider_has_section (BobguiStyleProvider *provider,
                                BobguiCssSection    *section)
{
  BobguiStyleProviderInterface *iface;

  iface = BOBGUI_STYLE_PROVIDER_GET_INTERFACE (provider);

  if (iface->has_section)
    return iface->has_section (provider, section);

  return FALSE;
}

/* These apis are misnamed, and the rest of BobguiStyleContext is deprecated,
 * so put them here for now
 */

/**
 * bobgui_style_context_add_provider_for_display:
 * @display: a `GdkDisplay`
 * @provider: a `BobguiStyleProvider`
 * @priority: the priority of the style provider. The lower
 *   it is, the earlier it will be used in the style construction.
 *   Typically this will be in the range between
 *   %BOBGUI_STYLE_PROVIDER_PRIORITY_FALLBACK and
 *   %BOBGUI_STYLE_PROVIDER_PRIORITY_USER
 *
 * Adds a global style provider to @display, which will be used
 * in style construction for all `BobguiStyleContexts` under @display.
 *
 * BOBGUI uses this to make styling information from `BobguiSettings`
 * available.
 *
 * Note: If both priorities are the same, A `BobguiStyleProvider`
 * added through [method@Bobgui.StyleContext.add_provider] takes
 * precedence over another added through this function.
 */
void
bobgui_style_context_add_provider_for_display (GdkDisplay       *display,
                                            BobguiStyleProvider *provider,
                                            guint             priority)
{
  BobguiStyleCascade *cascade;

  g_return_if_fail (GDK_IS_DISPLAY (display));
  g_return_if_fail (BOBGUI_IS_STYLE_PROVIDER (provider));
  g_return_if_fail (!BOBGUI_IS_SETTINGS (provider) || _bobgui_settings_get_display (BOBGUI_SETTINGS (provider)) == display);

  cascade = _bobgui_settings_get_style_cascade (bobgui_settings_get_for_display (display), 1);
  _bobgui_style_cascade_add_provider (cascade, provider, priority);
}

/**
 * bobgui_style_context_remove_provider_for_display:
 * @display: a `GdkDisplay`
 * @provider: a `BobguiStyleProvider`
 *
 * Removes @provider from the global style providers list in @display.
 */
void
bobgui_style_context_remove_provider_for_display (GdkDisplay       *display,
                                               BobguiStyleProvider *provider)
{
  BobguiStyleCascade *cascade;

  g_return_if_fail (GDK_IS_DISPLAY (display));
  g_return_if_fail (BOBGUI_IS_STYLE_PROVIDER (provider));
  g_return_if_fail (!BOBGUI_IS_SETTINGS (provider));

  cascade = _bobgui_settings_get_style_cascade (bobgui_settings_get_for_display (display), 1);
  _bobgui_style_cascade_remove_provider (cascade, provider);
}
