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

#include <bobgui/bobguiwidget.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_IM_CONTEXT              (bobgui_im_context_get_type ())
#define BOBGUI_IM_CONTEXT(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_IM_CONTEXT, BobguiIMContext))
#define BOBGUI_IM_CONTEXT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_IM_CONTEXT, BobguiIMContextClass))
#define BOBGUI_IS_IM_CONTEXT(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_IM_CONTEXT))
#define BOBGUI_IS_IM_CONTEXT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_IM_CONTEXT))
#define BOBGUI_IM_CONTEXT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_IM_CONTEXT, BobguiIMContextClass))


typedef struct _BobguiIMContext       BobguiIMContext;
typedef struct _BobguiIMContextClass  BobguiIMContextClass;

struct _BobguiIMContext
{
  GObject parent_instance;
};

struct _BobguiIMContextClass
{
  /*< private >*/
  GObjectClass parent_class;

  /*< public >*/
  /* Signals */
  void     (*preedit_start)        (BobguiIMContext *context);
  void     (*preedit_end)          (BobguiIMContext *context);
  void     (*preedit_changed)      (BobguiIMContext *context);
  void     (*commit)               (BobguiIMContext *context, const char *str);
  gboolean (*retrieve_surrounding) (BobguiIMContext *context);
  gboolean (*delete_surrounding)   (BobguiIMContext *context,
				    int           offset,
				    int           n_chars);

  /* Virtual functions */
  void     (*set_client_widget)   (BobguiIMContext   *context,
				   BobguiWidget      *widget);
  void     (*get_preedit_string)  (BobguiIMContext   *context,
				   char          **str,
				   PangoAttrList **attrs,
				   int            *cursor_pos);
  gboolean (*filter_keypress)     (BobguiIMContext   *context,
			           GdkEvent       *event);
  void     (*focus_in)            (BobguiIMContext   *context);
  void     (*focus_out)           (BobguiIMContext   *context);
  void     (*reset)               (BobguiIMContext   *context);
  void     (*set_cursor_location) (BobguiIMContext   *context,
				   GdkRectangle   *area);
  void     (*set_use_preedit)     (BobguiIMContext   *context,
				   gboolean        use_preedit);
  void     (*set_surrounding)     (BobguiIMContext   *context,
				   const char     *text,
				   int             len,
				   int             cursor_index);
  gboolean (*get_surrounding)     (BobguiIMContext   *context,
				   char          **text,
				   int            *cursor_index);
  void     (*set_surrounding_with_selection)
                                  (BobguiIMContext   *context,
				   const char     *text,
				   int             len,
				   int             cursor_index,
                                   int             anchor_index);
  gboolean (*get_surrounding_with_selection)
                                  (BobguiIMContext   *context,
				   char          **text,
				   int            *cursor_index,
                                   int            *anchor_index);

  /*< private >*/
  void (* activate_osk) (BobguiIMContext *context);
  gboolean (* activate_osk_with_event) (BobguiIMContext *context,
                                        GdkEvent     *event);

  /* another signal */
  gboolean (*invalid_composition)  (BobguiIMContext *context,
				    const char *str);

  /* Padding for future expansion */
  void (*_bobgui_reserved3) (void);
  void (*_bobgui_reserved4) (void);
};

GDK_AVAILABLE_IN_ALL
GType    bobgui_im_context_get_type            (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
void     bobgui_im_context_set_client_widget   (BobguiIMContext       *context,
                                             BobguiWidget          *widget);
GDK_AVAILABLE_IN_ALL
void     bobgui_im_context_get_preedit_string  (BobguiIMContext       *context,
					     char              **str,
					     PangoAttrList     **attrs,
					     int                *cursor_pos);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_im_context_filter_keypress     (BobguiIMContext       *context,
                                             GdkEvent           *event);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_im_context_filter_key          (BobguiIMContext       *context,
                                             gboolean            press,
                                             GdkSurface         *surface,
                                             GdkDevice          *device,
                                             guint32             time,
                                             guint               keycode,
                                             GdkModifierType     state,
                                             int                 group);

GDK_AVAILABLE_IN_ALL
void     bobgui_im_context_focus_in            (BobguiIMContext       *context);
GDK_AVAILABLE_IN_ALL
void     bobgui_im_context_focus_out           (BobguiIMContext       *context);
GDK_AVAILABLE_IN_ALL
void     bobgui_im_context_reset               (BobguiIMContext       *context);
GDK_AVAILABLE_IN_ALL
void     bobgui_im_context_set_cursor_location (BobguiIMContext       *context,
					     const GdkRectangle *area);
GDK_AVAILABLE_IN_ALL
void     bobgui_im_context_set_use_preedit     (BobguiIMContext       *context,
					     gboolean            use_preedit);
GDK_DEPRECATED_IN_4_2_FOR(bobgui_im_context_set_surrounding_with_selection)
void     bobgui_im_context_set_surrounding     (BobguiIMContext       *context,
                                             const char         *text,
                                             int                 len,
                                             int                 cursor_index);
GDK_DEPRECATED_IN_4_2_FOR(bobgui_im_context_get_surrounding_with_selection)
gboolean bobgui_im_context_get_surrounding     (BobguiIMContext       *context,
                                             char              **text,
                                             int                *cursor_index);
GDK_AVAILABLE_IN_4_2
void     bobgui_im_context_set_surrounding_with_selection
                                            (BobguiIMContext       *context,
                                             const char         *text,
                                             int                 len,
                                             int                 cursor_index,
                                             int                 anchor_index);
GDK_AVAILABLE_IN_4_2
gboolean bobgui_im_context_get_surrounding_with_selection
                                            (BobguiIMContext       *context,
                                             char              **text,
                                             int                *cursor_index,
                                             int                *anchor_index);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_im_context_delete_surrounding  (BobguiIMContext       *context,
					     int                 offset,
					     int                 n_chars);

GDK_AVAILABLE_IN_4_14
gboolean bobgui_im_context_activate_osk (BobguiIMContext *context,
                                      GdkEvent     *event);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiIMContext, g_object_unref)

G_END_DECLS

