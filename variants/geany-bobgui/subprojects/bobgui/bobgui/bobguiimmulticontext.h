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

#define BOBGUI_TYPE_IM_MULTICONTEXT              (bobgui_im_multicontext_get_type ())
#define BOBGUI_IM_MULTICONTEXT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_IM_MULTICONTEXT, BobguiIMMulticontext))
#define BOBGUI_IM_MULTICONTEXT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_IM_MULTICONTEXT, BobguiIMMulticontextClass))
#define BOBGUI_IS_IM_MULTICONTEXT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_IM_MULTICONTEXT))
#define BOBGUI_IS_IM_MULTICONTEXT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_IM_MULTICONTEXT))
#define BOBGUI_IM_MULTICONTEXT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_IM_MULTICONTEXT, BobguiIMMulticontextClass))


typedef struct _BobguiIMMulticontext        BobguiIMMulticontext;
typedef struct _BobguiIMMulticontextClass   BobguiIMMulticontextClass;
typedef struct _BobguiIMMulticontextPrivate BobguiIMMulticontextPrivate;

struct _BobguiIMMulticontext
{
  BobguiIMContext object;

  /*< private >*/
  BobguiIMMulticontextPrivate *priv;
};

struct _BobguiIMMulticontextClass
{
  BobguiIMContextClass parent_class;

  /* Padding for future expansion */
  void (*_bobgui_reserved1) (void);
  void (*_bobgui_reserved2) (void);
  void (*_bobgui_reserved3) (void);
  void (*_bobgui_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType         bobgui_im_multicontext_get_type (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiIMContext *bobgui_im_multicontext_new      (void);

GDK_AVAILABLE_IN_ALL
const char  * bobgui_im_multicontext_get_context_id   (BobguiIMMulticontext *context);

GDK_AVAILABLE_IN_ALL
void          bobgui_im_multicontext_set_context_id   (BobguiIMMulticontext *context,
                                                    const char        *context_id);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiIMMulticontext, g_object_unref)

G_END_DECLS

