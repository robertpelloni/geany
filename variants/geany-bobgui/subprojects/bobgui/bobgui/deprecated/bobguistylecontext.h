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

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/css/bobguicss.h>

#include <bobgui/bobguiborder.h>
#include <bobgui/bobguistyleprovider.h>
#include <bobgui/bobguitypes.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_STYLE_CONTEXT         (bobgui_style_context_get_type ())
#define BOBGUI_STYLE_CONTEXT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_STYLE_CONTEXT, BobguiStyleContext))
#define BOBGUI_STYLE_CONTEXT_CLASS(c)     (G_TYPE_CHECK_CLASS_CAST    ((c), BOBGUI_TYPE_STYLE_CONTEXT, BobguiStyleContextClass))
#define BOBGUI_IS_STYLE_CONTEXT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_STYLE_CONTEXT))
#define BOBGUI_IS_STYLE_CONTEXT_CLASS(c)  (G_TYPE_CHECK_CLASS_TYPE    ((c), BOBGUI_TYPE_STYLE_CONTEXT))
#define BOBGUI_STYLE_CONTEXT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS  ((o), BOBGUI_TYPE_STYLE_CONTEXT, BobguiStyleContextClass))

typedef struct _BobguiStyleContextClass BobguiStyleContextClass;

struct _BobguiStyleContext
{
  GObject parent_object;
};

struct _BobguiStyleContextClass
{
  GObjectClass parent_class;

  void (* changed) (BobguiStyleContext *context);

  /* Padding for future expansion */
  void (*_bobgui_reserved1) (void);
  void (*_bobgui_reserved2) (void);
  void (*_bobgui_reserved3) (void);
  void (*_bobgui_reserved4) (void);
};


GDK_AVAILABLE_IN_ALL
GType bobgui_style_context_get_type (void) G_GNUC_CONST;

GDK_DEPRECATED_IN_4_10
void bobgui_style_context_add_provider    (BobguiStyleContext  *context,
                                        BobguiStyleProvider *provider,
                                        guint             priority);

GDK_DEPRECATED_IN_4_10
void bobgui_style_context_remove_provider (BobguiStyleContext  *context,
                                        BobguiStyleProvider *provider);

GDK_DEPRECATED_IN_4_10
void bobgui_style_context_save    (BobguiStyleContext *context);
GDK_DEPRECATED_IN_4_10
void bobgui_style_context_restore (BobguiStyleContext *context);

GDK_DEPRECATED_IN_4_10
void          bobgui_style_context_set_state    (BobguiStyleContext *context,
                                              BobguiStateFlags    flags);
GDK_DEPRECATED_IN_4_10
BobguiStateFlags bobgui_style_context_get_state    (BobguiStyleContext *context);

GDK_DEPRECATED_IN_4_10
void          bobgui_style_context_set_scale    (BobguiStyleContext *context,
                                              int              scale);
GDK_DEPRECATED_IN_4_10
int           bobgui_style_context_get_scale    (BobguiStyleContext *context);

GDK_DEPRECATED_IN_4_10
void     bobgui_style_context_add_class    (BobguiStyleContext *context,
                                         const char      *class_name);
GDK_DEPRECATED_IN_4_10
void     bobgui_style_context_remove_class (BobguiStyleContext *context,
                                         const char      *class_name);
GDK_DEPRECATED_IN_4_10
gboolean bobgui_style_context_has_class    (BobguiStyleContext *context,
                                         const char      *class_name);

GDK_DEPRECATED_IN_4_10
void        bobgui_style_context_set_display (BobguiStyleContext *context,
                                           GdkDisplay      *display);
GDK_DEPRECATED_IN_4_10
GdkDisplay *bobgui_style_context_get_display (BobguiStyleContext *context);

GDK_DEPRECATED_IN_4_10
gboolean bobgui_style_context_lookup_color (BobguiStyleContext *context,
                                         const char      *color_name,
                                         GdkRGBA         *color);

/* Some helper functions to retrieve most common properties */
GDK_DEPRECATED_IN_4_10
void bobgui_style_context_get_color            (BobguiStyleContext *context,
                                             GdkRGBA         *color);
GDK_DEPRECATED_IN_4_10
void bobgui_style_context_get_border           (BobguiStyleContext *context,
                                             BobguiBorder       *border);
GDK_DEPRECATED_IN_4_10
void bobgui_style_context_get_padding          (BobguiStyleContext *context,
                                             BobguiBorder       *padding);
GDK_DEPRECATED_IN_4_10
void bobgui_style_context_get_margin           (BobguiStyleContext *context,
                                             BobguiBorder       *margin);

typedef enum {
  BOBGUI_STYLE_CONTEXT_PRINT_NONE         = 0,
  BOBGUI_STYLE_CONTEXT_PRINT_RECURSE      = 1 << 0,
  BOBGUI_STYLE_CONTEXT_PRINT_SHOW_STYLE   = 1 << 1,
  BOBGUI_STYLE_CONTEXT_PRINT_SHOW_CHANGE  = 1 << 2
} BobguiStyleContextPrintFlags;

GDK_DEPRECATED_IN_4_10
char * bobgui_style_context_to_string (BobguiStyleContext           *context,
                                    BobguiStyleContextPrintFlags  flags);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiStyleContext, g_object_unref)

G_END_DECLS

