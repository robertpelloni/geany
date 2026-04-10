/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2011 Benjamin Otte <otte@gnome.org>
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

#pragma once

#include <glib-object.h>
#include <bobgui/bobguistyleprovider.h>
#include "bobgui/bobguicountingbloomfilterprivate.h"
#include "bobgui/bobguicsskeyframesprivate.h"
#include "bobgui/bobguicsslookupprivate.h"
#include "bobgui/bobguicssnodeprivate.h"
#include "bobgui/bobguicssvalueprivate.h"
#include <bobgui/bobguitypes.h>

G_BEGIN_DECLS

#define BOBGUI_STYLE_PROVIDER_PRIORITY_INSPECTOR  1000

#define BOBGUI_STYLE_PROVIDER_GET_INTERFACE(o)  (G_TYPE_INSTANCE_GET_INTERFACE ((o), BOBGUI_TYPE_STYLE_PROVIDER, BobguiStyleProviderInterface))

typedef struct _BobguiStyleProviderInterface BobguiStyleProviderInterface;

struct _BobguiStyleProviderInterface
{
  GTypeInterface g_iface;

  BobguiCssValue *         (* get_color)           (BobguiStyleProvider        *provider,
                                                 const char              *name);
  BobguiSettings *         (* get_settings)        (BobguiStyleProvider        *provider);
  BobguiCssKeyframes *     (* get_keyframes)       (BobguiStyleProvider        *provider,
                                                 const char              *name);
  int                   (* get_scale)           (BobguiStyleProvider        *provider);
  void                  (* lookup)              (BobguiStyleProvider        *provider,
                                                 const BobguiCountingBloomFilter *filter,
                                                 BobguiCssNode              *node,
                                                 BobguiCssLookup            *lookup,
                                                 BobguiCssChange            *out_change);
  void                  (* emit_error)          (BobguiStyleProvider        *provider,
                                                 BobguiCssSection           *section,
                                                 const GError            *error);
  /* signal */
  void                  (* changed)             (BobguiStyleProvider        *provider);
  gboolean              (* has_section)         (BobguiStyleProvider        *provider,
                                                 BobguiCssSection           *section);
};

BobguiSettings *           bobgui_style_provider_get_settings          (BobguiStyleProvider        *provider);
BobguiCssValue *           bobgui_style_provider_get_color             (BobguiStyleProvider        *provider,
                                                                  const char              *name);
BobguiCssKeyframes *       bobgui_style_provider_get_keyframes         (BobguiStyleProvider        *provider,
                                                                  const char              *name);
int                     bobgui_style_provider_get_scale             (BobguiStyleProvider        *provider);
void                    bobgui_style_provider_lookup                (BobguiStyleProvider        *provider,
                                                                  const BobguiCountingBloomFilter *filter,
                                                                  BobguiCssNode              *node,
                                                                  BobguiCssLookup            *lookup,
                                                                  BobguiCssChange            *out_change);

void                    bobgui_style_provider_changed               (BobguiStyleProvider        *provider);

void                    bobgui_style_provider_emit_error            (BobguiStyleProvider        *provider,
                                                                  BobguiCssSection           *section,
                                                                  GError                  *error);
gboolean                bobgui_style_provider_has_section           (BobguiStyleProvider        *provider,
                                                                  BobguiCssSection           *section);

G_END_DECLS

