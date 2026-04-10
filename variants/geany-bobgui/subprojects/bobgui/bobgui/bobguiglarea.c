/* BOBGUI - The Bobgui Framework
 *
 * bobguiglarea.c: A GL drawing area
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

#include "config.h"

#include "config.h"
#include "bobguiglarea.h"
#include <glib/gi18n-lib.h>
#include "bobguimarshalers.h"
#include "gdk/gdkmarshalers.h"
#include "bobguiprivate.h"
#include "bobguisnapshot.h"
#include "bobguinative.h"
#include "bobguiwidgetprivate.h"
#include "bobguisnapshot.h"
#include "bobguirenderlayoutprivate.h"
#include "bobguicssnodeprivate.h"
#include "gdk/gdkgltextureprivate.h"
#include "gdk/gdkglcontextprivate.h"
#include "gdk/gdkdmabuftexturebuilderprivate.h"
#include "gdk/gdkdmabuftextureprivate.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <epoxy/gl.h>

/**
 * BobguiGLArea:
 *
 * Allows drawing with OpenGL.
 *
 * <picture>
 *   <source srcset="glarea-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiGLArea" src="glarea.png">
 * </picture>
 *
 * `BobguiGLArea` sets up its own [class@Gdk.GLContext], and creates a custom
 * GL framebuffer that the widget will do GL rendering onto. It also ensures
 * that this framebuffer is the default GL rendering target when rendering.
 * The completed rendering is integrated into the larger BOBGUI scene graph as
 * a texture.
 *
 * In order to draw, you have to connect to the [signal@Bobgui.GLArea::render]
 * signal, or subclass `BobguiGLArea` and override the BobguiGLAreaClass.render
 * virtual function.
 *
 * The `BobguiGLArea` widget ensures that the `GdkGLContext` is associated with
 * the widget's drawing area, and it is kept updated when the size and
 * position of the drawing area changes.
 *
 * ## Drawing with BobguiGLArea
 *
 * The simplest way to draw using OpenGL commands in a `BobguiGLArea` is to
 * create a widget instance and connect to the [signal@Bobgui.GLArea::render] signal:
 *
 * The `render()` function will be called when the `BobguiGLArea` is ready
 * for you to draw its content:
 *
 * The initial contents of the framebuffer are transparent.
 *
 * ```c
 * static gboolean
 * render (BobguiGLArea *area, GdkGLContext *context)
 * {
 *   // inside this function it's safe to use GL; the given
 *   // GdkGLContext has been made current to the drawable
 *   // surface used by the `BobguiGLArea` and the viewport has
 *   // already been set to be the size of the allocation
 *
 *   // we can start by clearing the buffer
 *   glClearColor (0, 0, 0, 0);
 *   glClear (GL_COLOR_BUFFER_BIT);
 *
 *   // record the active framebuffer ID, so we can return to it
 *   // with `glBindFramebuffer (GL_FRAMEBUFFER, screen_fb)` should
 *   // we, for instance, intend on utilizing the results of an
 *   // intermediate render texture pass
 *   GLuint screen_fb = 0;
 *   glGetIntegerv (GL_FRAMEBUFFER_BINDING, &screen_fb);
 *
 *   // draw your object
 *   // draw_an_object ();
 *
 *   // we completed our drawing; the draw commands will be
 *   // flushed at the end of the signal emission chain, and
 *   // the buffers will be drawn on the window
 *   return TRUE;
 * }
 *
 * void setup_glarea (void)
 * {
 *   // create a BobguiGLArea instance
 *   BobguiWidget *gl_area = bobgui_gl_area_new ();
 *
 *   // connect to the "render" signal
 *   g_signal_connect (gl_area, "render", G_CALLBACK (render), NULL);
 * }
 * ```
 *
 * If you need to initialize OpenGL state, e.g. buffer objects or
 * shaders, you should use the [signal@Bobgui.Widget::realize] signal;
 * you can use the [signal@Bobgui.Widget::unrealize] signal to clean up.
 * Since the `GdkGLContext` creation and initialization may fail, you
 * will need to check for errors, using [method@Bobgui.GLArea.get_error].
 *
 * An example of how to safely initialize the GL state is:
 *
 * ```c
 * static void
 * on_realize (BobguiGLArea *area)
 * {
 *   // We need to make the context current if we want to
 *   // call GL API
 *   bobgui_gl_area_make_current (area);
 *
 *   // If there were errors during the initialization or
 *   // when trying to make the context current, this
 *   // function will return a GError for you to catch
 *   if (bobgui_gl_area_get_error (area) != NULL)
 *     return;
 *
 *   // You can also use bobgui_gl_area_set_error() in order
 *   // to show eventual initialization errors on the
 *   // BobguiGLArea widget itself
 *   GError *internal_error = NULL;
 *   init_buffer_objects (&error);
 *   if (error != NULL)
 *     {
 *       bobgui_gl_area_set_error (area, error);
 *       g_error_free (error);
 *       return;
 *     }
 *
 *   init_shaders (&error);
 *   if (error != NULL)
 *     {
 *       bobgui_gl_area_set_error (area, error);
 *       g_error_free (error);
 *       return;
 *     }
 * }
 * ```
 *
 * If you need to change the options for creating the `GdkGLContext`
 * you should use the [signal@Bobgui.GLArea::create-context] signal.
 */

typedef struct {
  GdkGLTextureBuilder *builder;
  GdkTexture *gl_texture;
  GdkTexture *dmabuf_texture;
} Texture;

typedef struct {
  GdkGLContext *context;
  GError *error;

  gboolean have_buffers;

  int required_gl_version;

  guint frame_buffer;
  guint depth_stencil_buffer;
  Texture *texture;
  GList *textures;

  gboolean has_depth_buffer;
  gboolean has_stencil_buffer;

  gboolean needs_resize;
  gboolean needs_render;
  gboolean auto_render;
  gboolean use_es;
  GdkGLAPI allowed_apis;
} BobguiGLAreaPrivate;

enum {
  PROP_0,

  PROP_CONTEXT,
  PROP_HAS_DEPTH_BUFFER,
  PROP_HAS_STENCIL_BUFFER,
  PROP_USE_ES,
  PROP_ALLOWED_APIS,
  PROP_API,

  PROP_AUTO_RENDER,

  LAST_PROP
};

static GParamSpec *obj_props[LAST_PROP] = { NULL, };

enum {
  RENDER,
  RESIZE,
  CREATE_CONTEXT,

  LAST_SIGNAL
};

static void bobgui_gl_area_allocate_buffers (BobguiGLArea *area);
static void bobgui_gl_area_allocate_texture (BobguiGLArea *area);

static guint area_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE_WITH_PRIVATE (BobguiGLArea, bobgui_gl_area, BOBGUI_TYPE_WIDGET)

static void
bobgui_gl_area_set_property (GObject      *gobject,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  BobguiGLArea *self = BOBGUI_GL_AREA (gobject);

  switch (prop_id)
    {
    case PROP_AUTO_RENDER:
      bobgui_gl_area_set_auto_render (self, g_value_get_boolean (value));
      break;

    case PROP_HAS_DEPTH_BUFFER:
      bobgui_gl_area_set_has_depth_buffer (self, g_value_get_boolean (value));
      break;

    case PROP_HAS_STENCIL_BUFFER:
      bobgui_gl_area_set_has_stencil_buffer (self, g_value_get_boolean (value));
      break;

    case PROP_USE_ES:
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      bobgui_gl_area_set_use_es (self, g_value_get_boolean (value));
G_GNUC_END_IGNORE_DEPRECATIONS
      break;

    case PROP_ALLOWED_APIS:
      bobgui_gl_area_set_allowed_apis (self, g_value_get_flags (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_gl_area_get_property (GObject    *gobject,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (BOBGUI_GL_AREA (gobject));

  switch (prop_id)
    {
    case PROP_AUTO_RENDER:
      g_value_set_boolean (value, priv->auto_render);
      break;

    case PROP_HAS_DEPTH_BUFFER:
      g_value_set_boolean (value, priv->has_depth_buffer);
      break;

    case PROP_HAS_STENCIL_BUFFER:
      g_value_set_boolean (value, priv->has_stencil_buffer);
      break;

    case PROP_CONTEXT:
      g_value_set_object (value, priv->context);
      break;

    case PROP_USE_ES:
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      g_value_set_boolean (value, bobgui_gl_area_get_use_es (BOBGUI_GL_AREA (gobject)));
G_GNUC_END_IGNORE_DEPRECATIONS
      break;

    case PROP_ALLOWED_APIS:
      g_value_set_flags (value, priv->allowed_apis);
      break;

    case PROP_API:
      g_value_set_flags (value, bobgui_gl_area_get_api (BOBGUI_GL_AREA (gobject)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_gl_area_realize (BobguiWidget *widget)
{
  BobguiGLArea *area = BOBGUI_GL_AREA (widget);
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  BOBGUI_WIDGET_CLASS (bobgui_gl_area_parent_class)->realize (widget);

  g_clear_error (&priv->error);
  priv->context = NULL;
  g_signal_emit (area, area_signals[CREATE_CONTEXT], 0, &priv->context);

  /* In case the signal failed, but did not set an error */
  if (priv->context == NULL && priv->error == NULL)
    g_set_error_literal (&priv->error, GDK_GL_ERROR,
                         GDK_GL_ERROR_NOT_AVAILABLE,
                         _("OpenGL context creation failed"));

  priv->needs_resize = TRUE;
}

static void
bobgui_gl_area_notify (GObject    *object,
                    GParamSpec *pspec)
{
  if (strcmp (pspec->name, "scale-factor") == 0)
    {
      BobguiGLArea *area = BOBGUI_GL_AREA (object);
      BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

      priv->needs_resize = TRUE;
    }

  if (G_OBJECT_CLASS (bobgui_gl_area_parent_class)->notify)
    G_OBJECT_CLASS (bobgui_gl_area_parent_class)->notify (object, pspec);
}

static GdkGLContext *
bobgui_gl_area_real_create_context (BobguiGLArea *area)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);
  BobguiWidget *widget = BOBGUI_WIDGET (area);
  GError *error = NULL;
  GdkGLContext *context;

  context = gdk_surface_create_gl_context (bobgui_native_get_surface (bobgui_widget_get_native (widget)), &error);
  if (error != NULL)
    {
      bobgui_gl_area_set_error (area, error);
      g_clear_object (&context);
      g_clear_error (&error);
      return NULL;
    }

  gdk_gl_context_set_allowed_apis (context, priv->allowed_apis);
  gdk_gl_context_set_required_version (context,
                                       priv->required_gl_version / 10,
                                       priv->required_gl_version % 10);

  gdk_gl_context_realize (context, &error);
  if (error != NULL)
    {
      bobgui_gl_area_set_error (area, error);
      g_clear_object (&context);
      g_clear_error (&error);
      return NULL;
    }

  return context;
}

static void
bobgui_gl_area_resize (BobguiGLArea *area, int width, int height)
{
  glViewport (0, 0, width, height);
}

/*
 * Creates all the buffer objects needed for rendering the scene
 */
static void
bobgui_gl_area_ensure_buffers (BobguiGLArea *area)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);
  BobguiWidget *widget = BOBGUI_WIDGET (area);

  bobgui_widget_realize (widget);

  if (priv->context == NULL)
    return;

  if (priv->have_buffers)
    return;

  priv->have_buffers = TRUE;

  glGenFramebuffers (1, &priv->frame_buffer);

  if ((priv->has_depth_buffer || priv->has_stencil_buffer))
    {
      if (priv->depth_stencil_buffer == 0)
        glGenRenderbuffers (1, &priv->depth_stencil_buffer);
    }
  else if (priv->depth_stencil_buffer != 0)
    {
      /* Delete old depth/stencil buffer */
      glDeleteRenderbuffers (1, &priv->depth_stencil_buffer);
      priv->depth_stencil_buffer = 0;
    }

  bobgui_gl_area_allocate_buffers (area);
}

static void
delete_one_texture (gpointer data)
{
  Texture *texture = data;
  guint id;

  if (texture->gl_texture)
    {
      gdk_gl_texture_release (GDK_GL_TEXTURE (texture->gl_texture));
      texture->gl_texture = NULL;
    }

  id = gdk_gl_texture_builder_get_id (texture->builder);
  if (id != 0)
    glDeleteTextures (1, &id);

  g_clear_object (&texture->builder);

  if (texture->gl_texture == NULL && texture->dmabuf_texture == NULL)
    g_free (texture);
}

static void
bobgui_gl_area_ensure_texture (BobguiGLArea *area)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);
  BobguiWidget *widget = BOBGUI_WIDGET (area);

  bobgui_widget_realize (widget);

  if (priv->context == NULL)
    return;

  if (priv->texture == NULL)
    {
      GList *l, *link;

      l = priv->textures;
      while (l)
        {
          Texture *texture = l->data;
          link = l;
          l = l->next;

          if (texture->gl_texture)
            continue;

          priv->textures = g_list_delete_link (priv->textures, link);

          if (priv->texture == NULL)
            priv->texture = texture;
          else
            delete_one_texture (texture);
        }
    }

  if (priv->texture == NULL)
    {
      GLuint id;

      priv->texture = g_new (Texture, 1);
      priv->texture->gl_texture = NULL;
      priv->texture->dmabuf_texture = NULL;

      priv->texture->builder = gdk_gl_texture_builder_new ();
      gdk_gl_texture_builder_set_context (priv->texture->builder, priv->context);
      if (gdk_gl_context_get_api (priv->context) == GDK_GL_API_GLES)
        gdk_gl_texture_builder_set_format (priv->texture->builder, GDK_MEMORY_R8G8B8A8_PREMULTIPLIED);
      else
        gdk_gl_texture_builder_set_format (priv->texture->builder, GDK_MEMORY_B8G8R8A8_PREMULTIPLIED);

      glGenTextures (1, &id);
      gdk_gl_texture_builder_set_id (priv->texture->builder, id);
    }

  bobgui_gl_area_allocate_texture (area);
}

/*
 * Allocates space of the right type and size for all the buffers
 */
static void
bobgui_gl_area_allocate_buffers (BobguiGLArea *area)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);
  BobguiWidget *widget = BOBGUI_WIDGET (area);
  int scale, width, height;

  if (priv->context == NULL)
    return;

  scale = bobgui_widget_get_scale_factor (widget);
  width = bobgui_widget_get_width (widget) * scale;
  height = bobgui_widget_get_height (widget) * scale;

  if (priv->has_depth_buffer || priv->has_stencil_buffer)
    {
      glBindRenderbuffer (GL_RENDERBUFFER, priv->depth_stencil_buffer);
      if (priv->has_stencil_buffer)
        glRenderbufferStorage (GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
      else
        glRenderbufferStorage (GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    }

  priv->needs_render = TRUE;
}

static void
bobgui_gl_area_allocate_texture (BobguiGLArea *area)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);
  BobguiWidget *widget = BOBGUI_WIDGET (area);
  int scale, width, height;

  if (priv->context == NULL)
    return;

  if (priv->texture == NULL)
    return;

  g_assert (priv->texture->gl_texture == NULL);

  scale = bobgui_widget_get_scale_factor (widget);
  width = bobgui_widget_get_width (widget) * scale;
  height = bobgui_widget_get_height (widget) * scale;

  if (gdk_gl_texture_builder_get_width (priv->texture->builder) != width ||
      gdk_gl_texture_builder_get_height (priv->texture->builder) != height)
    {
      glBindTexture (GL_TEXTURE_2D, gdk_gl_texture_builder_get_id (priv->texture->builder));
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      if (gdk_gl_context_get_api (priv->context) == GDK_GL_API_GLES)
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      else
        glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

      gdk_gl_texture_builder_set_width (priv->texture->builder, width);
      gdk_gl_texture_builder_set_height (priv->texture->builder, height);
    }
}

/**
 * bobgui_gl_area_attach_buffers:
 * @area: a `BobguiGLArea`
 *
 * Binds buffers to the framebuffer.
 *
 * Ensures that the @area framebuffer object is made the current draw
 * and read target, and that all the required buffers for the @area
 * are created and bound to the framebuffer.
 *
 * This function is automatically called before emitting the
 * [signal@Bobgui.GLArea::render] signal, and doesn't normally need to be
 * called by application code.
 */
void
bobgui_gl_area_attach_buffers (BobguiGLArea *area)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  g_return_if_fail (BOBGUI_IS_GL_AREA (area));

  if (priv->context == NULL)
    return;

  bobgui_gl_area_make_current (area);

  if (priv->texture == NULL)
    bobgui_gl_area_ensure_texture (area);
  else if (priv->needs_resize)
    bobgui_gl_area_allocate_texture (area);

  if (!priv->have_buffers)
    bobgui_gl_area_ensure_buffers (area);
  else if (priv->needs_resize)
    bobgui_gl_area_allocate_buffers (area);

  glBindFramebuffer (GL_FRAMEBUFFER, priv->frame_buffer);

  if (priv->texture != NULL)
    glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D, gdk_gl_texture_builder_get_id (priv->texture->builder), 0);

  if (priv->depth_stencil_buffer)
    {
      if (priv->has_depth_buffer)
        glFramebufferRenderbuffer (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                   GL_RENDERBUFFER, priv->depth_stencil_buffer);
      if (priv->has_stencil_buffer)
        glFramebufferRenderbuffer (GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                   GL_RENDERBUFFER, priv->depth_stencil_buffer);
    }
}

static void
bobgui_gl_area_delete_buffers (BobguiGLArea *area)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  if (priv->context == NULL)
    return;

  priv->have_buffers = FALSE;

  if (priv->depth_stencil_buffer != 0)
    {
      glDeleteRenderbuffers (1, &priv->depth_stencil_buffer);
      priv->depth_stencil_buffer = 0;
    }

  if (priv->frame_buffer != 0)
    {
      glBindFramebuffer (GL_FRAMEBUFFER, 0);
      glDeleteFramebuffers (1, &priv->frame_buffer);
      priv->frame_buffer = 0;
    }
}

static void
bobgui_gl_area_delete_textures (BobguiGLArea *area)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  if (priv->texture)
    {
      delete_one_texture (priv->texture);
      priv->texture = NULL;
    }

  /* FIXME: we need to explicitly release all outstanding
   * textures here, otherwise release_texture will get called
   * later and access freed memory.
   */
  g_list_free_full (priv->textures, delete_one_texture);
  priv->textures = NULL;
}

static void
bobgui_gl_area_unrealize (BobguiWidget *widget)
{
  BobguiGLArea *area = BOBGUI_GL_AREA (widget);
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  if (priv->context != NULL)
    {
      bobgui_gl_area_make_current (area);
      bobgui_gl_area_delete_buffers (area);
      bobgui_gl_area_delete_textures (area);

      /* Make sure to unset the context if current */
      if (priv->context == gdk_gl_context_get_current ())
        gdk_gl_context_clear_current ();
    }

  g_clear_object (&priv->context);
  g_clear_error (&priv->error);

  BOBGUI_WIDGET_CLASS (bobgui_gl_area_parent_class)->unrealize (widget);
}

static void
bobgui_gl_area_size_allocate (BobguiWidget *widget,
                           int        width,
                           int        height,
                           int        baseline)
{
  BobguiGLArea *area = BOBGUI_GL_AREA (widget);
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  BOBGUI_WIDGET_CLASS (bobgui_gl_area_parent_class)->size_allocate (widget, width, height, baseline);

  if (bobgui_widget_get_realized (widget))
    priv->needs_resize = TRUE;
}

static void
bobgui_gl_area_draw_error_screen (BobguiGLArea   *area,
                               BobguiSnapshot *snapshot,
                               int          width,
                               int          height)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);
  PangoLayout *layout;
  int layout_height;
  BobguiCssBoxes boxes;

  layout = bobgui_widget_create_pango_layout (BOBGUI_WIDGET (area), priv->error->message);
  pango_layout_set_width (layout, width * PANGO_SCALE);
  pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
  pango_layout_get_pixel_size (layout, NULL, &layout_height);

  bobgui_css_boxes_init (&boxes, BOBGUI_WIDGET (area));
  bobgui_css_style_snapshot_layout (&boxes, snapshot, 0, (height - layout_height) / 2, layout);

  g_object_unref (layout);
}

static void
release_gl_texture (gpointer data)
{
  Texture *texture = data;
  gpointer sync;

  sync = gdk_gl_texture_builder_get_sync (texture->builder);
  if (sync)
    {
      glDeleteSync (sync);
      gdk_gl_texture_builder_set_sync (texture->builder, NULL);
    }

  texture->gl_texture = NULL;
}

static void
release_dmabuf_texture (gpointer data)
{
  Texture *texture = data;

  g_clear_object (&texture->gl_texture);

  if (texture->dmabuf_texture == NULL)
    return;

  gdk_dmabuf_close_fds ((GdkDmabuf *) gdk_dmabuf_texture_get_dmabuf (GDK_DMABUF_TEXTURE (texture->dmabuf_texture)));

  texture->dmabuf_texture = NULL;
}

static void
bobgui_gl_area_snapshot (BobguiWidget   *widget,
                      BobguiSnapshot *snapshot)
{
  BobguiGLArea *area = BOBGUI_GL_AREA (widget);
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);
  gboolean unused;
  int w, h, scale;
  GLenum status;

  scale = bobgui_widget_get_scale_factor (widget);
  w = bobgui_widget_get_width (widget) * scale;
  h = bobgui_widget_get_height (widget) * scale;

  if (w == 0 || h == 0)
    return;

  if (priv->error != NULL)
    {
      bobgui_gl_area_draw_error_screen (area,
                                     snapshot,
                                     bobgui_widget_get_width (widget),
                                     bobgui_widget_get_height (widget));
      return;
    }

  if (priv->context == NULL)
    return;

  bobgui_gl_area_make_current (area);

  bobgui_gl_area_attach_buffers (area);

 if (priv->has_depth_buffer)
   glEnable (GL_DEPTH_TEST);
 else
   glDisable (GL_DEPTH_TEST);

  status = glCheckFramebufferStatus (GL_FRAMEBUFFER);
  if (status == GL_FRAMEBUFFER_COMPLETE)
    {
      Texture *texture;
      gpointer sync = NULL;
      GdkDmabuf dmabuf;
      GdkTexture *holder;

      if (priv->needs_render || priv->auto_render)
        {
          if (priv->needs_resize)
            {
              g_signal_emit (area, area_signals[RESIZE], 0, w, h, NULL);
              priv->needs_resize = FALSE;
            }

          g_signal_emit (area, area_signals[RENDER], 0, priv->context, &unused);
        }

      priv->needs_render = FALSE;

      texture = priv->texture;
      priv->texture = NULL;
      priv->textures = g_list_prepend (priv->textures, texture);

      sync = glFenceSync (GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
      gdk_gl_texture_builder_set_sync (texture->builder, sync);

      texture->gl_texture = gdk_gl_texture_builder_build (texture->builder,
                                                          release_gl_texture,
                                                          texture);
      holder = texture->gl_texture;

      if (gdk_gl_context_export_dmabuf (priv->context,
                                        gdk_gl_texture_builder_get_id (texture->builder),
                                        &dmabuf))
        {
          GdkDmabufTextureBuilder *builder = gdk_dmabuf_texture_builder_new ();

          gdk_dmabuf_texture_builder_set_display (builder, gdk_gl_context_get_display (priv->context));
          gdk_dmabuf_texture_builder_set_width (builder, gdk_texture_get_width (texture->gl_texture));
          gdk_dmabuf_texture_builder_set_height (builder, gdk_texture_get_height (texture->gl_texture));
          gdk_dmabuf_texture_builder_set_premultiplied (builder, TRUE);
          gdk_dmabuf_texture_builder_set_dmabuf (builder, &dmabuf);

          texture->dmabuf_texture = gdk_dmabuf_texture_builder_build (builder, release_dmabuf_texture, texture, NULL);

          g_object_unref (builder);

          if (texture->dmabuf_texture != NULL)
            holder = texture->dmabuf_texture;
          else
            gdk_dmabuf_close_fds (&dmabuf);
        }

      /* Our texture is rendered by OpenGL, so it is upside down,
       * compared to what GSK expects, so flip it back.
       */
      bobgui_snapshot_save (snapshot);
      bobgui_snapshot_translate (snapshot, &GRAPHENE_POINT_INIT (0, bobgui_widget_get_height (widget)));
      bobgui_snapshot_scale (snapshot, 1, -1);
      bobgui_snapshot_append_texture (snapshot,
                                   holder,
                                   &GRAPHENE_RECT_INIT (0, 0,
                                                        bobgui_widget_get_width (widget),
                                                        bobgui_widget_get_height (widget)));
      bobgui_snapshot_restore (snapshot);

      g_object_unref (holder);
    }
  else
    {
      g_warning ("fb setup not supported (%x)", status);
    }
}

static gboolean
create_context_accumulator (GSignalInvocationHint *ihint,
                            GValue *return_accu,
                            const GValue *handler_return,
                            gpointer data)
{
  g_value_copy (handler_return, return_accu);

  /* stop after the first handler returning a valid object */
  return g_value_get_object (handler_return) == NULL;
}

static void
bobgui_gl_area_class_init (BobguiGLAreaClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  klass->resize = bobgui_gl_area_resize;
  klass->create_context = bobgui_gl_area_real_create_context;

  widget_class->realize = bobgui_gl_area_realize;
  widget_class->unrealize = bobgui_gl_area_unrealize;
  widget_class->size_allocate = bobgui_gl_area_size_allocate;
  widget_class->snapshot = bobgui_gl_area_snapshot;

  /**
   * BobguiGLArea:context:
   *
   * The `GdkGLContext` used by the `BobguiGLArea` widget.
   *
   * The `BobguiGLArea` widget is responsible for creating the `GdkGLContext`
   * instance. If you need to render with other kinds of buffers (stencil,
   * depth, etc), use render buffers.
   */
  obj_props[PROP_CONTEXT] =
    g_param_spec_object ("context", NULL, NULL,
                         GDK_TYPE_GL_CONTEXT,
                         G_PARAM_READABLE |
                         G_PARAM_STATIC_STRINGS);

  /**
   * BobguiGLArea:auto-render:
   *
   * If set to %TRUE the ::render signal will be emitted every time
   * the widget draws.
   *
   * This is the default and is useful if drawing the widget is faster.
   *
   * If set to %FALSE the data from previous rendering is kept around and will
   * be used for drawing the widget the next time, unless the window is resized.
   * In order to force a rendering [method@Bobgui.GLArea.queue_render] must be called.
   * This mode is useful when the scene changes seldom, but takes a long time
   * to redraw.
   */
  obj_props[PROP_AUTO_RENDER] =
    g_param_spec_boolean ("auto-render", NULL, NULL,
                          TRUE,
                          BOBGUI_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiGLArea:has-depth-buffer:
   *
   * If set to %TRUE the widget will allocate and enable a depth buffer for the
   * target framebuffer.
   *
   * Setting this property will enable GL's depth testing as a side effect. If
   * you don't need depth testing, you should call `glDisable(GL_DEPTH_TEST)`
   * in your `BobguiGLArea::render` handler.
   */
  obj_props[PROP_HAS_DEPTH_BUFFER] =
    g_param_spec_boolean ("has-depth-buffer", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiGLArea:has-stencil-buffer:
   *
   * If set to %TRUE the widget will allocate and enable a stencil buffer for the
   * target framebuffer.
   */
  obj_props[PROP_HAS_STENCIL_BUFFER] =
    g_param_spec_boolean ("has-stencil-buffer", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiGLArea:use-es:
   *
   * If set to %TRUE the widget will try to create a `GdkGLContext` using
   * OpenGL ES instead of OpenGL.
   *
   * Deprecated: 4.12: Use [property@Bobgui.GLArea:allowed-apis]
   */
  obj_props[PROP_USE_ES] =
    g_param_spec_boolean ("use-es", NULL, NULL,
                          FALSE,
                          BOBGUI_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS |
                          G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiGLArea:allowed-apis:
   *
   * The allowed APIs.
   *
   * Since: 4.12
   */
  obj_props[PROP_ALLOWED_APIS] =
    g_param_spec_flags ("allowed-apis", NULL, NULL,
                        GDK_TYPE_GL_API,
                        GDK_GL_API_GL | GDK_GL_API_GLES,
                        G_PARAM_READWRITE |
                        G_PARAM_STATIC_STRINGS |
                        G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiGLArea:api:
   *
   * The API currently in use.
   *
   * Since: 4.12
   */
  obj_props[PROP_API] =
    g_param_spec_flags ("api", NULL, NULL,
                        GDK_TYPE_GL_API,
                        0,
                        G_PARAM_READABLE |
                        G_PARAM_STATIC_STRINGS |
                        G_PARAM_EXPLICIT_NOTIFY);

  gobject_class->set_property = bobgui_gl_area_set_property;
  gobject_class->get_property = bobgui_gl_area_get_property;
  gobject_class->notify = bobgui_gl_area_notify;

  g_object_class_install_properties (gobject_class, LAST_PROP, obj_props);

  /**
   * BobguiGLArea::render:
   * @area: the `BobguiGLArea` that emitted the signal
   * @context: the `GdkGLContext` used by @area
   *
   * Emitted every time the contents of the `BobguiGLArea` should be redrawn.
   *
   * The @context is bound to the @area prior to emitting this function,
   * and the buffers are painted to the window once the emission terminates.
   *
   * Returns: %TRUE to stop other handlers from being invoked for the event.
   *   %FALSE to propagate the event further.
   */
  area_signals[RENDER] =
    g_signal_new (I_("render"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiGLAreaClass, render),
                  _bobgui_boolean_handled_accumulator, NULL,
                  _gdk_marshal_BOOLEAN__OBJECT,
                  G_TYPE_BOOLEAN, 1,
                  GDK_TYPE_GL_CONTEXT);
  g_signal_set_va_marshaller (area_signals[RENDER],
                              G_TYPE_FROM_CLASS (klass),
                              _gdk_marshal_BOOLEAN__OBJECTv);

  /**
   * BobguiGLArea::resize:
   * @area: the `BobguiGLArea` that emitted the signal
   * @width: the width of the viewport
   * @height: the height of the viewport
   *
   * Emitted once when the widget is realized, and then each time the widget
   * is changed while realized.
   *
   * This is useful in order to keep GL state up to date with the widget size,
   * like for instance camera properties which may depend on the width/height
   * ratio.
   *
   * The GL context for the area is guaranteed to be current when this signal
   * is emitted.
   *
   * The default handler sets up the GL viewport.
   */
  area_signals[RESIZE] =
    g_signal_new (I_("resize"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiGLAreaClass, resize),
                  NULL, NULL,
                  _gdk_marshal_VOID__INT_INT,
                  G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_INT);
  g_signal_set_va_marshaller (area_signals[RESIZE],
                              G_TYPE_FROM_CLASS (klass),
                              _gdk_marshal_VOID__INT_INTv);

  /**
   * BobguiGLArea::create-context:
   * @area: the `BobguiGLArea` that emitted the signal
   * @error: (nullable): location to store error information on failure
   *
   * Emitted when the widget is being realized.
   *
   * This allows you to override how the GL context is created.
   * This is useful when you want to reuse an existing GL context,
   * or if you want to try creating different kinds of GL options.
   *
   * If context creation fails then the signal handler can use
   * [method@Bobgui.GLArea.set_error] to register a more detailed error
   * of how the construction failed.
   *
   * Returns: (transfer full): a newly created `GdkGLContext`;
   *     the `BobguiGLArea` widget will take ownership of the returned value.
   */
  area_signals[CREATE_CONTEXT] =
    g_signal_new (I_("create-context"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiGLAreaClass, create_context),
                  create_context_accumulator, NULL,
                  _bobgui_marshal_OBJECT__VOID,
                  GDK_TYPE_GL_CONTEXT, 0);
  g_signal_set_va_marshaller (area_signals[CREATE_CONTEXT],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_OBJECT__VOIDv);
}

static void
bobgui_gl_area_init (BobguiGLArea *area)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  priv->auto_render = TRUE;
  priv->needs_render = TRUE;
  priv->required_gl_version = 0;
  priv->allowed_apis = GDK_GL_API_GL | GDK_GL_API_GLES;
}

/**
 * bobgui_gl_area_new:
 *
 * Creates a new `BobguiGLArea` widget.
 *
 * Returns: a new `BobguiGLArea`
 */
BobguiWidget *
bobgui_gl_area_new (void)
{
  return g_object_new (BOBGUI_TYPE_GL_AREA, NULL);
}

/**
 * bobgui_gl_area_set_error:
 * @area: a `BobguiGLArea`
 * @error: (nullable): a new `GError`, or %NULL to unset the error
 *
 * Sets an error on the area which will be shown instead of the
 * GL rendering.
 *
 * This is useful in the [signal@Bobgui.GLArea::create-context]
 * signal if GL context creation fails.
 */
void
bobgui_gl_area_set_error (BobguiGLArea    *area,
                       const GError *error)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  g_return_if_fail (BOBGUI_IS_GL_AREA (area));

  g_clear_error (&priv->error);
  if (error)
    priv->error = g_error_copy (error);
}

/**
 * bobgui_gl_area_get_error:
 * @area: a `BobguiGLArea`
 *
 * Gets the current error set on the @area.
 *
 * Returns: (nullable) (transfer none): the `GError`
 */
GError *
bobgui_gl_area_get_error (BobguiGLArea *area)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  g_return_val_if_fail (BOBGUI_IS_GL_AREA (area), NULL);

  return priv->error;
}

/**
 * bobgui_gl_area_set_use_es:
 * @area: a `BobguiGLArea`
 * @use_es: whether to use OpenGL or OpenGL ES
 *
 * Sets whether the @area should create an OpenGL or an OpenGL ES context.
 *
 * You should check the capabilities of the `GdkGLContext` before drawing
 * with either API.
 *
 * Deprecated: 4.12: Use [method@Bobgui.GLArea.set_allowed_apis]
 */
void
bobgui_gl_area_set_use_es (BobguiGLArea *area,
                        gboolean   use_es)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  g_return_if_fail (BOBGUI_IS_GL_AREA (area));
  g_return_if_fail (!bobgui_widget_get_realized (BOBGUI_WIDGET (area)));

  if ((priv->allowed_apis == GDK_GL_API_GLES) == use_es)
    return;

  priv->allowed_apis = use_es ? GDK_GL_API_GLES : GDK_GL_API_GL;

  g_object_notify_by_pspec (G_OBJECT (area), obj_props[PROP_USE_ES]);
  g_object_notify_by_pspec (G_OBJECT (area), obj_props[PROP_ALLOWED_APIS]);
}

/**
 * bobgui_gl_area_get_use_es:
 * @area: a `BobguiGLArea`
 *
 * Returns whether the `BobguiGLArea` should use OpenGL ES.
 *
 * See [method@Bobgui.GLArea.set_use_es].
 *
 * Returns: %TRUE if the `BobguiGLArea` should create an OpenGL ES context
 *   and %FALSE otherwise
 *
 * Deprecated: 4.12: Use [method@Bobgui.GLArea.get_api]
 */
gboolean
bobgui_gl_area_get_use_es (BobguiGLArea *area)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  g_return_val_if_fail (BOBGUI_IS_GL_AREA (area), FALSE);

  if (priv->context)
    return gdk_gl_context_get_api (priv->context) == GDK_GL_API_GLES;
  else
    return priv->allowed_apis == GDK_GL_API_GLES;
}

/**
 * bobgui_gl_area_set_allowed_apis:
 * @area: a `BobguiGLArea`
 * @apis: the allowed APIs
 *
 * Sets the allowed APIs to create a context with.
 *
 * You should check [property@Bobgui.GLArea:api] before drawing
 * with either API.
 *
 * By default, all APIs are allowed.
 *
 * Since: 4.12
 */
void
bobgui_gl_area_set_allowed_apis (BobguiGLArea *area,
                              GdkGLAPI   apis)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);
  GdkGLAPI old_allowed_apis;

  g_return_if_fail (BOBGUI_IS_GL_AREA (area));
  g_return_if_fail (!bobgui_widget_get_realized (BOBGUI_WIDGET (area)));

  if (priv->allowed_apis == apis)
    return;

  old_allowed_apis = priv->allowed_apis;

  priv->allowed_apis = apis;

  if ((old_allowed_apis == GDK_GL_API_GLES) != (apis == GDK_GL_API_GLES))
    g_object_notify_by_pspec (G_OBJECT (area), obj_props[PROP_USE_ES]);
  g_object_notify_by_pspec (G_OBJECT (area), obj_props[PROP_ALLOWED_APIS]);
}

/**
 * bobgui_gl_area_get_allowed_apis:
 * @area: a `BobguiGLArea`
 *
 * Gets the allowed APIs.
 *
 * See [method@Bobgui.GLArea.set_allowed_apis].
 *
 * Returns: the allowed APIs
 *
 * Since: 4.12
 */
GdkGLAPI
bobgui_gl_area_get_allowed_apis (BobguiGLArea *area)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  g_return_val_if_fail (BOBGUI_IS_GL_AREA (area), 0);

  return priv->allowed_apis;
}

/**
 * bobgui_gl_area_get_api:
 * @area: a `BobguiGLArea`
 *
 * Gets the API that is currently in use.
 *
 * If the GL area has not been realized yet, 0 is returned.
 *
 * Returns: the currently used API
 *
 * Since: 4.12
 */
GdkGLAPI
bobgui_gl_area_get_api (BobguiGLArea *area)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  g_return_val_if_fail (BOBGUI_IS_GL_AREA (area), 0);

  if (priv->context)
    return gdk_gl_context_get_api (priv->context);
  else
    return 0;
}

/**
 * bobgui_gl_area_set_required_version:
 * @area: a `BobguiGLArea`
 * @major: the major version
 * @minor: the minor version
 *
 * Sets the required version of OpenGL to be used when creating
 * the context for the widget.
 *
 * This function must be called before the area has been realized.
 */
void
bobgui_gl_area_set_required_version (BobguiGLArea *area,
                                  int        major,
                                  int        minor)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  g_return_if_fail (BOBGUI_IS_GL_AREA (area));
  g_return_if_fail (!bobgui_widget_get_realized (BOBGUI_WIDGET (area)));

  priv->required_gl_version = major * 10 + minor;
}

/**
 * bobgui_gl_area_get_required_version:
 * @area: a `BobguiGLArea`
 * @major: (out): return location for the required major version
 * @minor: (out): return location for the required minor version
 *
 * Retrieves the required version of OpenGL.
 *
 * See [method@Bobgui.GLArea.set_required_version].
 */
void
bobgui_gl_area_get_required_version (BobguiGLArea *area,
                                  int       *major,
                                  int       *minor)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  g_return_if_fail (BOBGUI_IS_GL_AREA (area));

  if (major != NULL)
    *major = priv->required_gl_version / 10;
  if (minor != NULL)
    *minor = priv->required_gl_version % 10;
}

/**
 * bobgui_gl_area_get_has_depth_buffer:
 * @area: a `BobguiGLArea`
 *
 * Returns whether the area has a depth buffer.
 *
 * Returns: %TRUE if the @area has a depth buffer, %FALSE otherwise
 */
gboolean
bobgui_gl_area_get_has_depth_buffer (BobguiGLArea *area)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  g_return_val_if_fail (BOBGUI_IS_GL_AREA (area), FALSE);

  return priv->has_depth_buffer;
}

/**
 * bobgui_gl_area_set_has_depth_buffer:
 * @area: a `BobguiGLArea`
 * @has_depth_buffer: %TRUE to add a depth buffer
 *
 * Sets whether the `BobguiGLArea` should use a depth buffer.
 *
 * If @has_depth_buffer is %TRUE the widget will allocate and
 * enable a depth buffer for the target framebuffer. Otherwise
 * there will be none.
 */
void
bobgui_gl_area_set_has_depth_buffer (BobguiGLArea *area,
                                  gboolean   has_depth_buffer)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  g_return_if_fail (BOBGUI_IS_GL_AREA (area));

  has_depth_buffer = !!has_depth_buffer;

  if (priv->has_depth_buffer != has_depth_buffer)
    {
      priv->has_depth_buffer = has_depth_buffer;

      g_object_notify (G_OBJECT (area), "has-depth-buffer");

      priv->have_buffers = FALSE;
    }
}

/**
 * bobgui_gl_area_get_has_stencil_buffer:
 * @area: a `BobguiGLArea`
 *
 * Returns whether the area has a stencil buffer.
 *
 * Returns: %TRUE if the @area has a stencil buffer, %FALSE otherwise
 */
gboolean
bobgui_gl_area_get_has_stencil_buffer (BobguiGLArea *area)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  g_return_val_if_fail (BOBGUI_IS_GL_AREA (area), FALSE);

  return priv->has_stencil_buffer;
}

/**
 * bobgui_gl_area_set_has_stencil_buffer:
 * @area: a `BobguiGLArea`
 * @has_stencil_buffer: %TRUE to add a stencil buffer
 *
 * Sets whether the `BobguiGLArea` should use a stencil buffer.
 *
 * If @has_stencil_buffer is %TRUE the widget will allocate and
 * enable a stencil buffer for the target framebuffer. Otherwise
 * there will be none.
 */
void
bobgui_gl_area_set_has_stencil_buffer (BobguiGLArea *area,
                                    gboolean   has_stencil_buffer)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  g_return_if_fail (BOBGUI_IS_GL_AREA (area));

  has_stencil_buffer = !!has_stencil_buffer;

  if (priv->has_stencil_buffer != has_stencil_buffer)
    {
      priv->has_stencil_buffer = has_stencil_buffer;

      g_object_notify (G_OBJECT (area), "has-stencil-buffer");

      priv->have_buffers = FALSE;
    }
}

/**
 * bobgui_gl_area_queue_render:
 * @area: a `BobguiGLArea`
 *
 * Marks the currently rendered data (if any) as invalid, and queues
 * a redraw of the widget.
 *
 * This ensures that the [signal@Bobgui.GLArea::render] signal
 * is emitted during the draw.
 *
 * This is only needed when [method@Bobgui.GLArea.set_auto_render] has
 * been called with a %FALSE value. The default behaviour is to
 * emit [signal@Bobgui.GLArea::render] on each draw.
 */
void
bobgui_gl_area_queue_render (BobguiGLArea *area)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  g_return_if_fail (BOBGUI_IS_GL_AREA (area));

  priv->needs_render = TRUE;

  bobgui_widget_queue_draw (BOBGUI_WIDGET (area));
}


/**
 * bobgui_gl_area_get_auto_render:
 * @area: a `BobguiGLArea`
 *
 * Returns whether the area is in auto render mode or not.
 *
 * Returns: %TRUE if the @area is auto rendering, %FALSE otherwise
 */
gboolean
bobgui_gl_area_get_auto_render (BobguiGLArea *area)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  g_return_val_if_fail (BOBGUI_IS_GL_AREA (area), FALSE);

  return priv->auto_render;
}

/**
 * bobgui_gl_area_set_auto_render:
 * @area: a `BobguiGLArea`
 * @auto_render: a boolean
 *
 * Sets whether the `BobguiGLArea` is in auto render mode.
 *
 * If @auto_render is %TRUE the [signal@Bobgui.GLArea::render] signal will
 * be emitted every time the widget draws. This is the default and is
 * useful if drawing the widget is faster.
 *
 * If @auto_render is %FALSE the data from previous rendering is kept
 * around and will be used for drawing the widget the next time,
 * unless the window is resized. In order to force a rendering
 * [method@Bobgui.GLArea.queue_render] must be called. This mode is
 * useful when the scene changes seldom, but takes a long time to redraw.
 */
void
bobgui_gl_area_set_auto_render (BobguiGLArea *area,
                             gboolean   auto_render)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  g_return_if_fail (BOBGUI_IS_GL_AREA (area));

  auto_render = !!auto_render;

  if (priv->auto_render != auto_render)
    {
      priv->auto_render = auto_render;

      g_object_notify (G_OBJECT (area), "auto-render");

      if (auto_render)
        bobgui_widget_queue_draw (BOBGUI_WIDGET (area));
    }
}

/**
 * bobgui_gl_area_get_context:
 * @area: a `BobguiGLArea`
 *
 * Retrieves the `GdkGLContext` used by @area.
 *
 * Returns: (transfer none) (nullable): the `GdkGLContext`
 */
GdkGLContext *
bobgui_gl_area_get_context (BobguiGLArea *area)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  g_return_val_if_fail (BOBGUI_IS_GL_AREA (area), NULL);

  return priv->context;
}

/**
 * bobgui_gl_area_make_current:
 * @area: a `BobguiGLArea`
 *
 * Ensures that the `GdkGLContext` used by @area is associated with
 * the `BobguiGLArea`.
 *
 * This function is automatically called before emitting the
 * [signal@Bobgui.GLArea::render] signal, and doesn't normally need
 * to be called by application code.
 */
void
bobgui_gl_area_make_current (BobguiGLArea *area)
{
  BobguiGLAreaPrivate *priv = bobgui_gl_area_get_instance_private (area);

  g_return_if_fail (BOBGUI_IS_GL_AREA (area));
  g_return_if_fail (bobgui_widget_get_realized (BOBGUI_WIDGET (area)));

  if (priv->context != NULL)
    gdk_gl_context_make_current (priv->context);
}
