/*
 * Copyright © 2012 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#pragma once

#include "bobguicssanimatedstyleprivate.h"

G_BEGIN_DECLS

typedef struct _BobguiStyleAnimation           BobguiStyleAnimation;
typedef struct _BobguiStyleAnimationClass      BobguiStyleAnimationClass;

struct _BobguiStyleAnimation
{
  const BobguiStyleAnimationClass *class;
  guint ref_count;
};

struct _BobguiStyleAnimationClass
{
  const char *type_name;
  void          (* free)                                (BobguiStyleAnimation      *animation);
  gboolean      (* is_finished)                         (BobguiStyleAnimation      *animation);
  gboolean      (* is_static)                           (BobguiStyleAnimation      *animation);
  void          (* apply_values)                        (BobguiStyleAnimation      *animation,
                                                         BobguiCssAnimatedStyle    *style);
  BobguiStyleAnimation *  (* advance)                      (BobguiStyleAnimation      *animation,
                                                         gint64                  timestamp);
};

GType           _bobgui_style_animation_get_type           (void) G_GNUC_CONST;

BobguiStyleAnimation * _bobgui_style_animation_advance        (BobguiStyleAnimation      *animation,
                                                         gint64                  timestamp);
void            _bobgui_style_animation_apply_values       (BobguiStyleAnimation      *animation,
                                                         BobguiCssAnimatedStyle    *style);
gboolean        _bobgui_style_animation_is_finished        (BobguiStyleAnimation      *animation);
gboolean        _bobgui_style_animation_is_static          (BobguiStyleAnimation      *animation);

BobguiStyleAnimation * bobgui_style_animation_ref             (BobguiStyleAnimation      *animation);
BobguiStyleAnimation * bobgui_style_animation_unref           (BobguiStyleAnimation      *animation);


G_END_DECLS

