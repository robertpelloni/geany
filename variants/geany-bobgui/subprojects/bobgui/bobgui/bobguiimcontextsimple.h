/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2000 Red Hat Software
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

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiimcontext.h>


G_BEGIN_DECLS

/*
 * No longer used by BOBGUI, just left here on the off chance that some
 * 3rd party code used this define.
 */
/**
 * BOBGUI_MAX_COMPOSE_LEN:
 *
 * Evaluates to the maximum length of a compose sequence.
 *
 * This macro is longer used by BOBGUI.
 */
#define BOBGUI_MAX_COMPOSE_LEN 7

#define BOBGUI_TYPE_IM_CONTEXT_SIMPLE              (bobgui_im_context_simple_get_type ())
#define BOBGUI_IM_CONTEXT_SIMPLE(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_IM_CONTEXT_SIMPLE, BobguiIMContextSimple))
#define BOBGUI_IM_CONTEXT_SIMPLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_IM_CONTEXT_SIMPLE, BobguiIMContextSimpleClass))
#define BOBGUI_IS_IM_CONTEXT_SIMPLE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_IM_CONTEXT_SIMPLE))
#define BOBGUI_IS_IM_CONTEXT_SIMPLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_IM_CONTEXT_SIMPLE))
#define BOBGUI_IM_CONTEXT_SIMPLE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_IM_CONTEXT_SIMPLE, BobguiIMContextSimpleClass))


typedef struct _BobguiIMContextSimple              BobguiIMContextSimple;
typedef struct _BobguiIMContextSimplePrivate       BobguiIMContextSimplePrivate;
typedef struct _BobguiIMContextSimpleClass         BobguiIMContextSimpleClass;

struct _BobguiIMContextSimple
{
  BobguiIMContext object;

  /*< private >*/
  BobguiIMContextSimplePrivate *priv;
};

struct _BobguiIMContextSimpleClass
{
  BobguiIMContextClass parent_class;
};

GDK_AVAILABLE_IN_ALL
GType         bobgui_im_context_simple_get_type  (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiIMContext *bobgui_im_context_simple_new       (void);

GDK_DEPRECATED_IN_4_4_FOR(bobgui_im_context_simple_add_compose_file)
void          bobgui_im_context_simple_add_table (BobguiIMContextSimple *context_simple,
                                               guint16            *data,
                                               int                 max_seq_len,
                                               int                 n_seqs);
GDK_AVAILABLE_IN_ALL
void          bobgui_im_context_simple_add_compose_file (BobguiIMContextSimple *context_simple,
                                                      const char         *compose_file);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiIMContextSimple, g_object_unref)

G_END_DECLS


