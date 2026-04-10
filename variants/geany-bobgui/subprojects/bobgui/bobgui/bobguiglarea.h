/* BOBGUI - The Bobgui Framework
 *
 * bobguiglarea.h: A GL drawing area
 *
 * Copyright © 2014  Emmanuele Bassi
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

#define BOBGUI_TYPE_GL_AREA                (bobgui_gl_area_get_type ())
#define BOBGUI_GL_AREA(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_GL_AREA, BobguiGLArea))
#define BOBGUI_IS_GL_AREA(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_GL_AREA))
#define BOBGUI_GL_AREA_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_GL_AREA, BobguiGLAreaClass))
#define BOBGUI_IS_GL_AREA_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_GL_AREA))
#define BOBGUI_GL_AREA_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_GL_AREA, BobguiGLAreaClass))

typedef struct _BobguiGLArea               BobguiGLArea;
typedef struct _BobguiGLAreaClass          BobguiGLAreaClass;

struct _BobguiGLArea
{
  /*< private >*/
  BobguiWidget parent_instance;
};

/**
 * BobguiGLAreaClass:
 * @render: class closure for the `BobguiGLArea::render` signal
 * @resize: class closeure for the `BobguiGLArea::resize` signal
 * @create_context: class closure for the `BobguiGLArea::create-context` signal
 *
 * The `BobguiGLAreaClass` structure contains only private data.
 */
struct _BobguiGLAreaClass
{
  /*< private >*/
  BobguiWidgetClass parent_class;

  /*< public >*/
  gboolean       (* render)         (BobguiGLArea        *area,
                                     GdkGLContext     *context);
  void           (* resize)         (BobguiGLArea        *area,
                                     int               width,
                                     int               height);
  GdkGLContext * (* create_context) (BobguiGLArea        *area);

  /*< private >*/
  gpointer _padding[8];
};

GDK_AVAILABLE_IN_ALL
GType bobgui_gl_area_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiWidget *     bobgui_gl_area_new                         (void);

GDK_AVAILABLE_IN_4_12
void            bobgui_gl_area_set_allowed_apis            (BobguiGLArea    *area,
                                                         GdkGLAPI      apis);
GDK_AVAILABLE_IN_4_12
GdkGLAPI        bobgui_gl_area_get_allowed_apis            (BobguiGLArea    *area);
GDK_AVAILABLE_IN_4_12
GdkGLAPI        bobgui_gl_area_get_api                     (BobguiGLArea    *area);

GDK_DEPRECATED_IN_4_12_FOR(bobgui_gl_area_set_allowed_apis)
void            bobgui_gl_area_set_use_es                  (BobguiGLArea    *area,
                                                         gboolean      use_es);
GDK_DEPRECATED_IN_4_12_FOR(bobgui_gl_area_get_api)
gboolean        bobgui_gl_area_get_use_es                  (BobguiGLArea    *area);

GDK_AVAILABLE_IN_ALL
void            bobgui_gl_area_set_required_version        (BobguiGLArea    *area,
                                                         int           major,
                                                         int           minor);
GDK_AVAILABLE_IN_ALL
void            bobgui_gl_area_get_required_version        (BobguiGLArea    *area,
                                                         int          *major,
                                                         int          *minor);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_gl_area_get_has_depth_buffer        (BobguiGLArea    *area);
GDK_AVAILABLE_IN_ALL
void            bobgui_gl_area_set_has_depth_buffer        (BobguiGLArea    *area,
                                                         gboolean      has_depth_buffer);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_gl_area_get_has_stencil_buffer      (BobguiGLArea    *area);
GDK_AVAILABLE_IN_ALL
void            bobgui_gl_area_set_has_stencil_buffer      (BobguiGLArea    *area,
                                                         gboolean      has_stencil_buffer);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_gl_area_get_auto_render             (BobguiGLArea    *area);
GDK_AVAILABLE_IN_ALL
void            bobgui_gl_area_set_auto_render             (BobguiGLArea    *area,
                                                         gboolean      auto_render);
GDK_AVAILABLE_IN_ALL
void           bobgui_gl_area_queue_render                 (BobguiGLArea    *area);


GDK_AVAILABLE_IN_ALL
GdkGLContext *  bobgui_gl_area_get_context                 (BobguiGLArea    *area);

GDK_AVAILABLE_IN_ALL
void            bobgui_gl_area_make_current                (BobguiGLArea    *area);
GDK_AVAILABLE_IN_ALL
void            bobgui_gl_area_attach_buffers              (BobguiGLArea    *area);

GDK_AVAILABLE_IN_ALL
void            bobgui_gl_area_set_error                   (BobguiGLArea    *area,
                                                         const GError *error);
GDK_AVAILABLE_IN_ALL
GError *        bobgui_gl_area_get_error                   (BobguiGLArea    *area);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiGLArea, g_object_unref)

G_END_DECLS

