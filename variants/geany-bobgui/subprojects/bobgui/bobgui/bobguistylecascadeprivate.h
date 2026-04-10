/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2012 Benjamin Otte <otte@gnome.org>
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

#include <gdk/gdk.h>
#include <bobgui/bobguistyleproviderprivate.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_STYLE_CASCADE           (_bobgui_style_cascade_get_type ())
#define BOBGUI_STYLE_CASCADE(obj)           (G_TYPE_CHECK_INSTANCE_CAST (obj, BOBGUI_TYPE_STYLE_CASCADE, BobguiStyleCascade))
#define BOBGUI_STYLE_CASCADE_CLASS(cls)     (G_TYPE_CHECK_CLASS_CAST (cls, BOBGUI_TYPE_STYLE_CASCADE, BobguiStyleCascadeClass))
#define BOBGUI_IS_STYLE_CASCADE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE (obj, BOBGUI_TYPE_STYLE_CASCADE))
#define BOBGUI_IS_STYLE_CASCADE_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE (obj, BOBGUI_TYPE_STYLE_CASCADE))
#define BOBGUI_STYLE_CASCADE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_STYLE_CASCADE, BobguiStyleCascadeClass))

typedef struct _BobguiStyleCascade           BobguiStyleCascade;
typedef struct _BobguiStyleCascadeClass      BobguiStyleCascadeClass;

struct _BobguiStyleCascade
{
  GObject object;

  BobguiStyleCascade *parent;
  GArray *providers;
  int scale;
};

struct _BobguiStyleCascadeClass
{
  GObjectClass  parent_class;
};

GType                 _bobgui_style_cascade_get_type               (void) G_GNUC_CONST;

BobguiStyleCascade *     _bobgui_style_cascade_new                    (void);

void                  _bobgui_style_cascade_set_parent             (BobguiStyleCascade     *cascade,
                                                                 BobguiStyleCascade     *parent);
void                  _bobgui_style_cascade_set_scale              (BobguiStyleCascade     *cascade,
                                                                 int                  scale);
int                   _bobgui_style_cascade_get_scale              (BobguiStyleCascade     *cascade);

void                  _bobgui_style_cascade_add_provider           (BobguiStyleCascade     *cascade,
                                                                 BobguiStyleProvider    *provider,
                                                                 guint                priority);
void                  _bobgui_style_cascade_remove_provider        (BobguiStyleCascade     *cascade,
                                                                 BobguiStyleProvider    *provider);


G_END_DECLS

