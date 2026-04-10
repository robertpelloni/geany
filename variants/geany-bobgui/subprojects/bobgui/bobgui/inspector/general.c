/*
 * Copyright (c) 2014 Red Hat, Inc.
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
#include <glib/gi18n-lib.h>

#include "general.h"
#include "window.h"

#include "bobguidebug.h"
#include "bobguilabel.h"
#include "bobguiscale.h"
#include "bobguiswitch.h"
#include "bobguilistbox.h"
#include "bobguiprivate.h"
#include "bobguisizegroup.h"
#include "bobguiimage.h"
#include "bobguiadjustment.h"
#include "bobguibox.h"
#include "bobguibinlayout.h"
#include "bobguimediafileprivate.h"
#include "bobguiimmoduleprivate.h"
#include "bobguistringpairprivate.h"

#include "gdk/gdkdebugprivate.h"
#include "gdk/gdkdisplayprivate.h"
#include "gdk/gdkmonitorprivate.h"
#include "gdk/gdkglcontextprivate.h"
#include "gdk/gdkvulkancontextprivate.h"

#include "profile_conf.h"

#include <epoxy/gl.h>

#ifdef GDK_WINDOWING_X11
#include "x11/gdkx.h"
#include <epoxy/glx.h>
#include <epoxy/egl.h>
#endif

#ifdef GDK_WINDOWING_WIN32
#include "win32/gdkwin32.h"
#include "gdkglcontextprivate.h"
#include <epoxy/wgl.h>
#ifdef GDK_WIN32_ENABLE_EGL
#include <epoxy/egl.h>
#endif
#endif

#ifdef GDK_WINDOWING_MACOS
#include "macos/gdkmacos.h"
#endif

#ifdef GDK_WINDOWING_WAYLAND
#include "wayland/gdkwayland.h"
#include <epoxy/egl.h>
#include <xkbcommon/xkbcommon.h>
#include "wayland/gdkdisplay-wayland.h"
#include "wayland/gdkwaylandcolor-private.h"
#include "bobgui/bobguiimcontextwaylandprivate.h"
#endif

#ifdef GDK_WINDOWING_BROADWAY
#include "broadway/gdkbroadway.h"
#endif

#ifdef GDK_RENDERING_VULKAN
#include <vulkan/vulkan.h>
#endif

static void bobgui_inspector_general_clip (BobguiButton *button, BobguiInspectorGeneral *gen);

struct _BobguiInspectorGeneral
{
  BobguiWidget parent;

  BobguiWidget *swin;
  BobguiWidget *box;
  BobguiWidget *version_box;
  BobguiWidget *env_box;
  BobguiWidget *display_box;
  BobguiWidget *display_extensions_row;
  BobguiWidget *display_extensions_box;
  BobguiWidget *monitor_box;
  BobguiWidget *gl_box;
  BobguiWidget *gl_features_row;
  BobguiWidget *gl_features_box;
  BobguiWidget *gl_extensions_row;
  BobguiStringList *gl_extensions_list;
  BobguiWidget *egl_extensions_row;
  BobguiWidget *egl_extensions_row_name;
  BobguiStringList *egl_extensions_list;
  BobguiWidget *vulkan_box;
  BobguiWidget *vulkan_features_row;
  BobguiWidget *vulkan_features_box;
  BobguiWidget *vulkan_extensions_row;
  BobguiStringList *vulkan_extensions_list;
  BobguiWidget *vulkan_layers_row;
  BobguiStringList *vulkan_layers_list;
  BobguiWidget *device_box;
  BobguiWidget *os_info;
  BobguiWidget *bobgui_version;
  BobguiWidget *gdk_backend;
  BobguiWidget *gsk_renderer;
  BobguiWidget *pango_fontmap;
  BobguiWidget *media_backend;
  BobguiWidget *im_module;
  BobguiWidget *a11y_backend;
  BobguiWidget *gl_backend_version;
  BobguiWidget *gl_backend_version_row;
  BobguiWidget *gl_backend_vendor;
  BobguiWidget *gl_backend_vendor_row;
  BobguiWidget *gl_error;
  BobguiWidget *gl_error_row;
  BobguiWidget *gl_version;
  BobguiWidget *gl_version_row;
  BobguiWidget *gl_vendor;
  BobguiWidget *gl_vendor_row;
  BobguiWidget *gl_renderer;
  BobguiWidget *gl_renderer_row;
  BobguiWidget *gl_full_version;
  BobguiWidget *gl_full_version_row;
  BobguiWidget *glsl_version;
  BobguiWidget *glsl_version_row;
  BobguiWidget *vk_device;
  BobguiWidget *vk_api_version;
  BobguiWidget *vk_api_version_row;
  BobguiWidget *vk_driver_version;
  BobguiWidget *vk_driver_version_row;
  BobguiWidget *vk_error;
  BobguiWidget *vk_error_row;
  BobguiWidget *app_id_box;
  BobguiWidget *app_id;
  BobguiWidget *resource_path;
  BobguiWidget *prefix;
  BobguiWidget *environment_row;
  GListStore *environment_list;
  BobguiWidget *display_name;
  BobguiWidget *display_rgba;
  BobguiWidget *display_composited;
  BobguiWidget *overlay;

  GdkDisplay *display;
};

typedef struct _BobguiInspectorGeneralClass
{
  BobguiWidgetClass parent_class;
} BobguiInspectorGeneralClass;

G_DEFINE_TYPE (BobguiInspectorGeneral, bobgui_inspector_general, BOBGUI_TYPE_WIDGET)

/* Note that all the information collection functions come in two variants:
 * init_foo() and dump_foo(). The former updates the widgets of the inspector
 * page, the latter creates a markdown dump, to copy-pasted into a gitlab
 * issue.
 *
 * Please keep the two in sync when making changes.
 */

/* {{{ Utilities */

static G_GNUC_UNUSED void
add_check_row (BobguiInspectorGeneral *gen,
               BobguiListBox          *list,
               const char          *name,
               gboolean             value,
               int                  indent)
{
  BobguiWidget *row, *box, *label, *check;

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 40);
  g_object_set (box, "margin-start", indent, NULL);

  label = bobgui_label_new (name);
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (label, BOBGUI_ALIGN_BASELINE_FILL);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.0);
  bobgui_widget_set_hexpand (label, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), label);

  check = bobgui_image_new_from_icon_name ("object-select-symbolic");
  bobgui_widget_set_halign (check, BOBGUI_ALIGN_END);
  bobgui_widget_set_valign (check, BOBGUI_ALIGN_BASELINE_FILL);
  bobgui_widget_set_opacity (check, value ? 1.0 : 0.0);
  bobgui_box_append (BOBGUI_BOX (box), check);

  row = bobgui_list_box_row_new ();
  bobgui_list_box_row_set_child (BOBGUI_LIST_BOX_ROW (row), box);
  bobgui_list_box_row_set_activatable (BOBGUI_LIST_BOX_ROW (row), FALSE);

  bobgui_widget_set_hexpand (box, FALSE);
  bobgui_list_box_insert (list, row, -1);
}

static BobguiWidget *
add_label_row (BobguiInspectorGeneral *gen,
               BobguiListBox          *list,
               const char          *name,
               const char          *value,
               int                  indent)
{
  BobguiWidget *box;
  BobguiWidget *label;
  BobguiWidget *row;

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 40);
  g_object_set (box, "margin-start", indent, NULL);

  label = bobgui_label_new (name);
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
  bobgui_widget_set_valign (label, BOBGUI_ALIGN_BASELINE_FILL);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 0.0);
  bobgui_widget_set_hexpand (label, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), label);

  label = bobgui_label_new (value);
  bobgui_label_set_selectable (BOBGUI_LABEL (label), TRUE);
  bobgui_widget_set_halign (label, BOBGUI_ALIGN_END);
  bobgui_widget_set_valign (label, BOBGUI_ALIGN_BASELINE_FILL);
  bobgui_label_set_xalign (BOBGUI_LABEL (label), 1.0);
  bobgui_label_set_ellipsize (BOBGUI_LABEL (label), PANGO_ELLIPSIZE_END);
  bobgui_label_set_width_chars (BOBGUI_LABEL (label), 25);
  bobgui_box_append (BOBGUI_BOX (box), label);

  row = bobgui_list_box_row_new ();
  bobgui_list_box_row_set_child (BOBGUI_LIST_BOX_ROW (row), box);
  bobgui_list_box_row_set_activatable (BOBGUI_LIST_BOX_ROW (row), FALSE);

  bobgui_widget_set_hexpand (box, FALSE);
  bobgui_list_box_insert (BOBGUI_LIST_BOX (list), row, -1);

  return label;
}

static void
set_monospace_font (BobguiWidget *w)
{
  PangoAttrList *attrs;

  attrs = pango_attr_list_new ();
  pango_attr_list_insert (attrs, pango_attr_fallback_new (FALSE));
  pango_attr_list_insert (attrs, pango_attr_family_new ("Monospace"));
  bobgui_label_set_attributes (BOBGUI_LABEL (w), attrs);
  pango_attr_list_unref (attrs);
}

/* }}} */
/* {{{ OS Info */

static void
init_os_info (BobguiInspectorGeneral *gen)
{
  char *os_info = g_get_os_info (G_OS_INFO_KEY_PRETTY_NAME);
  bobgui_label_set_text (BOBGUI_LABEL (gen->os_info), os_info);
  g_free (os_info);
}

static void
dump_os_info (GdkDisplay *display,
              GString    *string)
{
  char *os_info = g_get_os_info (G_OS_INFO_KEY_PRETTY_NAME);
  g_string_append_printf (string, "| Operating System | %s |\n", os_info);
  g_free (os_info);
}


/* }}} */
/* {{{ Version */

static const char *
get_display_kind (GdkDisplay *display)
{
#ifdef GDK_WINDOWING_X11
  if (GDK_IS_X11_DISPLAY (display))
    return "X11";
  else
#endif
#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (display))
    return "Wayland";
  else
#endif
#ifdef GDK_WINDOWING_BROADWAY
  if (GDK_IS_BROADWAY_DISPLAY (display))
    return "Broadway";
  else
#endif
#ifdef GDK_WINDOWING_WIN32
  if (GDK_IS_WIN32_DISPLAY (display))
    return "Windows";
  else
#endif
#ifdef GDK_WINDOWING_MACOS
  if (GDK_IS_MACOS_DISPLAY (display))
    return "MacOS";
  else
#endif
    return "Unknown";
}

static const char *
get_renderer_kind (GdkDisplay *display)
{
  GdkSurface *surface;
  GskRenderer *gsk_renderer;
  const char *renderer;

  surface = gdk_surface_new_toplevel (display);
  gsk_renderer = gsk_renderer_new_for_surface (surface);

  if (strcmp (G_OBJECT_TYPE_NAME (gsk_renderer), "GskVulkanRenderer") == 0)
    renderer = "Vulkan";
  else if (strcmp (G_OBJECT_TYPE_NAME (gsk_renderer), "GskGLRenderer") == 0)
    renderer = "GL";
  else if (strcmp (G_OBJECT_TYPE_NAME (gsk_renderer), "GskCairoRenderer") == 0)
    renderer = "Cairo";
  else if (strcmp (G_OBJECT_TYPE_NAME (gsk_renderer), "GskNglRenderer") == 0)
    renderer = "GL (new)";
  else
    renderer = "Unknown";

  gsk_renderer_unrealize (gsk_renderer);
  g_object_unref (gsk_renderer);
  gdk_surface_destroy (surface);

  return renderer;
}

static char *
get_version_string (void)
{
  if (g_strcmp0 (PROFILE, "devel") == 0)
    return g_strdup_printf ("%s-%s", BOBGUI_VERSION, VCS_TAG);
  else
    return g_strdup (BOBGUI_VERSION);
}

static void
init_version (BobguiInspectorGeneral *gen)
{
  char *version;

  version = get_version_string ();
  bobgui_label_set_text (BOBGUI_LABEL (gen->bobgui_version), version);
  g_free (version);

  bobgui_label_set_text (BOBGUI_LABEL (gen->gdk_backend), get_display_kind (gen->display));
  bobgui_label_set_text (BOBGUI_LABEL (gen->gsk_renderer), get_renderer_kind (gen->display));
}

static void
dump_version (GdkDisplay *display,
              GString    *string)
{
  char *version;

  version = get_version_string ();
  g_string_append_printf (string, "| BOBGUI Version | %s |\n", version);
  g_free (version);
  g_string_append_printf (string, "| GDK Backend | %s |\n", get_display_kind (display));
  g_string_append_printf (string, "| GSK Renderer | %s |\n", get_renderer_kind (display));
}

/* }}} */
/* {{{ Pango */

static const char *
get_fontmap_kind (void)
{
  PangoFontMap *fontmap;
  const char *type;

  fontmap = pango_cairo_font_map_get_default ();
  type = G_OBJECT_TYPE_NAME (fontmap);

  if (strcmp (type, "PangoCairoFcFontMap") == 0)
    return "fontconfig";
  else if (strcmp (type, "PangoCairoCoreTextFontMap") == 0)
    return "coretext";
  else if (strcmp (type, "PangoCairoWin32FontMap") == 0)
    return  "win32";
  else
    return type;
}

static void
init_pango (BobguiInspectorGeneral *gen)
{
  bobgui_label_set_label (BOBGUI_LABEL (gen->pango_fontmap), get_fontmap_kind ());
}

static void
dump_pango (GdkDisplay *display,
            GString    *string)
{
  g_string_append_printf (string, "| Pango Fontmap | %s |\n", get_fontmap_kind ());
}

/* }}} */
/* {{{ Media */

static char *
get_media_backend_description (void)
{
  GIOExtension *e;
  const char *name;

  e = bobgui_media_file_get_extension ();
  name = g_io_extension_get_name (e);

#ifdef HAVE_GSTREAMER
  if (g_str_equal (name, "gstreamer"))
    {
      return gst_version_string ();
    }
  else
#endif
    {
      return g_strdup (name);
    }
}

static void
init_media (BobguiInspectorGeneral *gen)
{
  char *backend = get_media_backend_description ();
  bobgui_label_set_label (BOBGUI_LABEL (gen->media_backend), backend);
  g_free (backend);
}

static void
dump_media (GdkDisplay *display,
            GString    *string)
{
  char *backend = get_media_backend_description ();
  g_string_append_printf (string, "| Media Backend | %s |\n", backend);
  g_free (backend);
}

/* }}} */
/* {{{ Input Method */

static void
im_module_changed (BobguiSettings         *settings,
                   GParamSpec          *pspec,
                   BobguiInspectorGeneral *gen)
{
  if (!gen->display)
    return;

  bobgui_label_set_label (BOBGUI_LABEL (gen->im_module),
                       _bobgui_im_module_get_default_context_id (gen->display));
}

static const char *
get_im_module_kind (GdkDisplay *display)
{
  return _bobgui_im_module_get_default_context_id (display);
}

static void
init_im_module (BobguiInspectorGeneral *gen)
{
  BobguiSettings *settings = bobgui_settings_get_for_display (gen->display);

  bobgui_label_set_label (BOBGUI_LABEL (gen->im_module), get_im_module_kind (gen->display));

  if (g_getenv ("BOBGUI_IM_MODULE") != NULL)
    {
      /* This can't update if BOBGUI_IM_MODULE envvar is set */
      bobgui_widget_set_tooltip_text (gen->im_module,
                                   _("IM Context is hardcoded by BOBGUI_IM_MODULE"));
      bobgui_widget_set_sensitive (gen->im_module, FALSE);
      return;
    }

  g_signal_connect_object (settings,
                           "notify::bobgui-im-module",
                           G_CALLBACK (im_module_changed),
                           gen, 0);
}

static void
dump_im_module (GdkDisplay *display,
                GString    *string)
{
  g_string_append_printf (string, "| Input Method | %s |\n", get_im_module_kind (display));
}

/* }}} */
/* {{{ Accessibility */

static const char *
get_a11y_backend (GdkDisplay *display)
{
  BobguiWidget *widget;
  BobguiATContext *ctx;
  const char *backend;

  widget = bobgui_label_new ("");
  g_object_ref_sink (widget);
  ctx = bobgui_at_context_create (BOBGUI_ACCESSIBLE_ROLE_LABEL, BOBGUI_ACCESSIBLE (widget), display);

  if (ctx == NULL)
    backend = "none";
  else if (strcmp (G_OBJECT_TYPE_NAME (ctx), "BobguiAtSpiContext") == 0)
    backend = "atspi";
  else if (strcmp (G_OBJECT_TYPE_NAME (ctx), "BobguiAccessKitContext") == 0)
    backend = "accesskit";
  else if (strcmp (G_OBJECT_TYPE_NAME (ctx), "BobguiTestATContext") == 0)
    backend = "test";
  else
    backend = "unknown";

  g_clear_object (&ctx);
  g_clear_object (&widget);

  return backend;
}

static void
init_a11y_backend (BobguiInspectorGeneral *gen)
{
  bobgui_label_set_label (BOBGUI_LABEL (gen->a11y_backend), get_a11y_backend (gen->display));
}

static void
dump_a11y_backend (GdkDisplay *display,
                   GString    *string)
{
  g_string_append_printf (string, "| Accessibility backend | %s |\n", get_a11y_backend (display));
}

/* }}} */
/* {{{ Application data */

static void
init_app_id (BobguiInspectorGeneral *gen)
{
  GApplication *app;

  app = g_application_get_default ();
  if (!app)
    {
      bobgui_widget_set_visible (gen->app_id_box, FALSE);
      return;
    }

  bobgui_label_set_text (BOBGUI_LABEL (gen->app_id),
                      g_application_get_application_id (app));
  bobgui_label_set_text (BOBGUI_LABEL (gen->resource_path),
                      g_application_get_resource_base_path (app));
}

static void
dump_app_id (GdkDisplay *display,
             GString    *string)
{
  GApplication *app;

  app = g_application_get_default ();
  if (!app)
    return;

  g_string_append_printf (string, "| Application ID | %s |\n", g_application_get_application_id (app));
  g_string_append_printf (string, "| Resource Path | %s |\n", g_application_get_resource_base_path (app));
}

/* }}} */
/* {{{ GL */

static void
add_gl_features (BobguiInspectorGeneral *gen)
{
  GdkGLContext *context;

  context = gdk_display_get_gl_context (gen->display);

  for (int i = 0; i < GDK_GL_N_FEATURES; i++)
    {
      add_check_row (gen, BOBGUI_LIST_BOX (gen->gl_features_box),
                     gdk_gl_feature_keys[i].key,
                     gdk_gl_context_has_feature (context, gdk_gl_feature_keys[i].value),
                     0);
    }
}

/* unused on some setup, like Mac */
static void G_GNUC_UNUSED
append_extensions (BobguiStringList *list,
                   const char    *extensions)
{
  char **items;
  gsize i;

  if (extensions == NULL)
    return;
  
  items = g_strsplit (extensions, " ", -1);
  for (i = 0; items[i]; i++)
    {
      if (items[i] == NULL || items[i][0] == 0)
        continue;

      bobgui_string_list_append (list, items[i]);
    }

  g_strfreev (items);
}

#if defined(GDK_WINDOWING_X11) || defined(GDK_WINDOWING_WAYLAND) || (defined(GDK_WINDOWING_WIN32) && defined(GDK_WIN32_ENABLE_EGL))
static EGLDisplay
get_egl_display (GdkDisplay *display)
{
#ifdef GDK_WINDOWING_WAYLAND
  if (GDK_IS_WAYLAND_DISPLAY (display))
    return gdk_wayland_display_get_egl_display (display);
  else
#endif
#ifdef GDK_WINDOWING_X11
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  if (GDK_IS_X11_DISPLAY (display))
    return gdk_x11_display_get_egl_display (display);
  else
G_GNUC_END_IGNORE_DEPRECATIONS
#endif
#ifdef GDK_WINDOWING_WIN32
  if (GDK_IS_WIN32_DISPLAY (display))
    return gdk_win32_display_get_egl_display (display);
  else
#endif
   return NULL;
}
#endif

static void
init_gl (BobguiInspectorGeneral *gen)
{
  GdkGLContext *context;
  GError *error = NULL;
  int major, minor;
  int num_extensions, i;
  char *s;

  if (!gdk_display_prepare_gl (gen->display, &error))
    {
      bobgui_label_set_text (BOBGUI_LABEL (gen->gl_renderer), C_("GL renderer", "None"));
      bobgui_widget_set_visible (gen->gl_error_row, TRUE);
      bobgui_widget_set_visible (gen->gl_version_row, FALSE);
      bobgui_widget_set_visible (gen->gl_backend_version_row, FALSE);
      bobgui_widget_set_visible (gen->gl_backend_vendor_row, FALSE);
      bobgui_widget_set_visible (gen->gl_vendor_row, FALSE);
      bobgui_widget_set_visible (gen->gl_full_version_row, FALSE);
      bobgui_widget_set_visible (gen->glsl_version_row, FALSE);
      bobgui_widget_set_visible (gen->gl_features_row, FALSE);
      bobgui_widget_set_visible (gen->gl_extensions_row, FALSE);
      bobgui_widget_set_visible (gen->egl_extensions_row, FALSE);
      bobgui_label_set_text (BOBGUI_LABEL (gen->gl_error), error->message);
      g_error_free (error);
      return;
    }

  gdk_gl_context_make_current (gdk_display_get_gl_context (gen->display));

  glGetIntegerv (GL_NUM_EXTENSIONS, &num_extensions);
  for (i = 0; i < num_extensions; i++)
    {
      const char *gl_ext = (const char *) glGetStringi (GL_EXTENSIONS, i);
      if (!gl_ext)
        break;
      bobgui_string_list_append (BOBGUI_STRING_LIST (gen->gl_extensions_list), gl_ext);
    }

#if defined(GDK_WINDOWING_X11) || defined(GDK_WINDOWING_WAYLAND) || (defined(GDK_WINDOWING_WIN32) && defined(GDK_WIN32_ENABLE_EGL))
  EGLDisplay egl_display = get_egl_display (gen->display);
  if (egl_display)
    {
      char *version;

      version = g_strconcat ("EGL ", eglQueryString (egl_display, EGL_VERSION), NULL);
      bobgui_label_set_text (BOBGUI_LABEL (gen->gl_backend_version), version);
      g_free (version);

      bobgui_label_set_text (BOBGUI_LABEL (gen->gl_backend_vendor), eglQueryString (egl_display, EGL_VENDOR));

      bobgui_label_set_text (BOBGUI_LABEL (gen->egl_extensions_row_name), "EGL extensions");
      append_extensions (gen->egl_extensions_list, eglQueryString (egl_display, EGL_EXTENSIONS));
    }
  else
#endif
#ifdef GDK_WINDOWING_X11

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

  if (GDK_IS_X11_DISPLAY (gen->display))
    {
      Display *dpy = GDK_DISPLAY_XDISPLAY (gen->display);
      int error_base, event_base, screen;
      char *version;

      if (!glXQueryExtension (dpy, &error_base, &event_base))
        return;

      version = g_strconcat ("GLX ", glXGetClientString (dpy, GLX_VERSION), NULL);
      bobgui_label_set_text (BOBGUI_LABEL (gen->gl_backend_version), version);
      g_free (version);
      bobgui_label_set_text (BOBGUI_LABEL (gen->gl_backend_vendor), glXGetClientString (dpy, GLX_VENDOR));

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      screen = XScreenNumberOfScreen (gdk_x11_display_get_xscreen (gen->display));
G_GNUC_END_IGNORE_DEPRECATIONS
      bobgui_label_set_text (BOBGUI_LABEL (gen->egl_extensions_row_name), "GLX extensions");
      append_extensions (gen->egl_extensions_list, glXQueryExtensionsString (dpy, screen));
    }
  else

G_GNUC_END_IGNORE_DEPRECATIONS

#endif
#ifdef GDK_WINDOWING_WIN32
  if (GDK_IS_WIN32_DISPLAY (gen->display) &&
      gdk_gl_backend_can_be_used (GDK_GL_WGL, NULL))
    {
      PFNWGLGETEXTENSIONSSTRINGARBPROC my_wglGetExtensionsStringARB;

      bobgui_label_set_text (BOBGUI_LABEL (gen->gl_backend_vendor), "Microsoft WGL");
      bobgui_widget_set_visible (gen->gl_backend_version, FALSE);

      my_wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC) wglGetProcAddress("wglGetExtensionsStringARB");

      if (my_wglGetExtensionsStringARB)
        {
          bobgui_label_set_text (BOBGUI_LABEL (gen->egl_extensions_row_name), "WGL extensions");
          append_extensions (gen->egl_extensions_list, my_wglGetExtensionsStringARB (wglGetCurrentDC ()));
        }
      else
        {
          bobgui_label_set_text (BOBGUI_LABEL (gen->egl_extensions_row_name), "WGL extensions: none");
        }
    }
  else
#endif
    {
      bobgui_label_set_text (BOBGUI_LABEL (gen->gl_backend_version), C_("GL version", "Unknown"));
      bobgui_widget_set_visible (gen->egl_extensions_row, FALSE);
    }

  context = gdk_display_get_gl_context (gen->display);
  gdk_gl_context_make_current (context);
  gdk_gl_context_get_version (context, &major, &minor);
  s = g_strdup_printf ("%s %u.%u",
                       gdk_gl_context_get_use_es (context) ? "GLES " : "OpenGL ",
                       major, minor);
  bobgui_label_set_text (BOBGUI_LABEL (gen->gl_version), s);
  g_free (s);
  bobgui_label_set_text (BOBGUI_LABEL (gen->gl_vendor), (const char *) glGetString (GL_VENDOR));
  bobgui_label_set_text (BOBGUI_LABEL (gen->gl_renderer), (const char *) glGetString (GL_RENDERER));
  bobgui_label_set_text (BOBGUI_LABEL (gen->gl_full_version), (const char *) glGetString (GL_VERSION));
  bobgui_label_set_text (BOBGUI_LABEL (gen->glsl_version), (const char *) glGetString (GL_SHADING_LANGUAGE_VERSION));

  add_gl_features (gen);
}

static void
dump_gl (GdkDisplay *display,
         GString    *string)
{
  GdkGLContext *context;
  GError *error = NULL;
  int major, minor;
  int num_extensions;
  char *s;
  GString *ext;

  if (!gdk_display_prepare_gl (display, &error))
    {
      g_string_append (string, "| GL Renderer | None |\n");
      g_string_append_printf (string, "| | %s |\n", error->message);
      g_error_free (error);
      return;
    }

  gdk_gl_context_make_current (gdk_display_get_gl_context (display));

  ext = g_string_new ("");

#if defined(GDK_WINDOWING_X11) || defined(GDK_WINDOWING_WAYLAND) || (defined(GDK_WINDOWING_WIN32) && defined(GDK_WIN32_ENABLE_EGL))
  EGLDisplay egl_display = get_egl_display (display);
  if (egl_display)
    {
      char *version;
      guint count;
      char *prefix;

      version = g_strconcat ("EGL ", eglQueryString (egl_display, EGL_VERSION), NULL);
      g_string_append_printf (string, "| GL Backend Version | %s |\n", version);
      g_free (version);

      g_string_append_printf (string, "| GL Backend Vendor | %s |\n", eglQueryString (egl_display, EGL_VENDOR));

      g_string_assign (ext, eglQueryString (egl_display, EGL_EXTENSIONS));
      count = g_string_replace (ext, " ", "<br>", 0);
      prefix = g_strdup_printf ("| EGL Extensions | <details><summary>%u Extensions</summary>", count + 1);
      g_string_prepend (ext, prefix);
      g_string_append (ext, "</details> |\n");
      g_free (prefix);
    }
  else
#endif
#ifdef GDK_WINDOWING_X11

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

  if (GDK_IS_X11_DISPLAY (display))
    {
      Display *dpy = GDK_DISPLAY_XDISPLAY (display);
      int error_base, event_base, screen;
      char *version;
      guint count;
      char *prefix;

      if (!glXQueryExtension (dpy, &error_base, &event_base))
        return;

      version = g_strconcat ("GLX ", glXGetClientString (dpy, GLX_VERSION), NULL);
      g_string_append_printf (string, "| GL Backend Version | %s |\n", version);
      g_free (version);

      g_string_append_printf (string, "| GL Backend Vendor | %s |\n", glXGetClientString (dpy, GLX_VENDOR));

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      screen = XScreenNumberOfScreen (gdk_x11_display_get_xscreen (display));
G_GNUC_END_IGNORE_DEPRECATIONS
      g_string_assign (ext, glXQueryExtensionsString (dpy, screen));
      count = g_string_replace (ext, " ", "<br>", 0);
      prefix = g_strdup_printf ("| GLX Extensions | <details><summary>%u Extensions</summary>", count + 1);
      g_string_prepend (ext, prefix);
      g_string_append (ext, "</details> |\n");
      g_free (prefix);
    }
  else

G_GNUC_END_IGNORE_DEPRECATIONS

#endif
#ifdef GDK_WINDOWING_WIN32
  if (GDK_IS_WIN32_DISPLAY (display) &&
      gdk_gl_backend_can_be_used (GDK_GL_WGL, NULL))
    {
      PFNWGLGETEXTENSIONSSTRINGARBPROC my_wglGetExtensionsStringARB;

      g_string_append (string, "| GL Backend Vendor | Microsoft WGL |\n");

      my_wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC) wglGetProcAddress("wglGetExtensionsStringARB");

      if (my_wglGetExtensionsStringARB)
        {
          guint count;
          char *prefix;

          g_string_assign (ext, my_wglGetExtensionsStringARB (wglGetCurrentDC ()));
          count = g_string_replace (ext, " ", "<br>", 0);
          prefix = g_strdup_printf ("| WGL Extensions | <details><summary>%u Extensions</summary>", count + 1);
          g_string_prepend (ext, prefix);
          g_string_append (ext, "</details> |\n");
          g_free (prefix);
        }
      else
        {
          g_string_append (ext, "| WGL Extensions | None |\n");
        }
    }
  else
#endif
    {
      g_string_append (string, "| GL Backend Version | Unknown |\n");
    }

  context = gdk_display_get_gl_context (display);
  gdk_gl_context_make_current (context);
  gdk_gl_context_get_version (context, &major, &minor);
  s = g_strdup_printf ("%s %u.%u",
                       gdk_gl_context_get_use_es (context) ? "GLES " : "OpenGL ",
                       major, minor);
  g_string_append_printf (string, "| GL Version | %s |\n", s);
  g_free (s);
  g_string_append_printf (string, "| GL Vendor | %s |\n", (const char *) glGetString (GL_VENDOR));
  g_string_append_printf (string, "| GL Renderer | %s |\n", (const char *) glGetString (GL_RENDERER));
  g_string_append_printf (string, "| GL Full Version | %s |\n", (const char *) glGetString (GL_VERSION));
  g_string_append_printf (string, "| GLSL Version | %s |\n", (const char *) glGetString (GL_SHADING_LANGUAGE_VERSION));

  glGetIntegerv (GL_NUM_EXTENSIONS, &num_extensions);
  g_string_append_printf (string, "| GL Extensions | <details><summary>%d Extensions</summary>", num_extensions);
  for (int i = 0; i < num_extensions; i++)
    {
      const char *gl_ext = (const char *) glGetStringi (GL_EXTENSIONS, i);
      if (gl_ext)
        g_string_append_printf (string, "%s%s", i > 0 ? "<br>" : "", gl_ext);
    }
  g_string_append (string, "</details> |\n");

  g_string_append (string, ext->str);
  g_string_free (ext, TRUE);

  g_string_append (string, "| Features | ");
  for (int i = 0; i < GDK_GL_N_FEATURES; i++)
    {
      if (gdk_gl_context_has_feature (context, gdk_gl_feature_keys[i].value))
        g_string_append_printf (string, "%s%s", i > 0 ? "<br>" : "", gdk_gl_feature_keys[i].key);
    }
  g_string_append (string, " |\n");
}

/* }}} */
/* {{{ Vulkan */

#ifdef GDK_RENDERING_VULKAN
static gboolean
gdk_vulkan_has_feature (GdkDisplay        *display,
                        GdkVulkanFeatures  feature)
{
  return (display->vulkan_features & feature) ? TRUE : FALSE;
}

static void
add_vulkan_features (BobguiInspectorGeneral *gen)
{
  for (int i = 0; i < GDK_VULKAN_N_FEATURES; i++)
    {
      add_check_row (gen, BOBGUI_LIST_BOX (gen->vulkan_features_box),
                     gdk_vulkan_feature_keys[i].key,
                     gdk_vulkan_has_feature (gen->display, gdk_vulkan_feature_keys[i].value),
                     0);
    }
}

static void
add_instance_extensions (BobguiStringList *list)
{
  uint32_t i;
  uint32_t n_extensions;
  VkExtensionProperties *extensions;

  vkEnumerateInstanceExtensionProperties (NULL, &n_extensions, NULL);
  extensions = g_newa (VkExtensionProperties, n_extensions);
  vkEnumerateInstanceExtensionProperties (NULL, &n_extensions, extensions);

  for (i = 0; i < n_extensions; i++)
    bobgui_string_list_append (list, extensions[i].extensionName);
}

static void
add_device_extensions (VkPhysicalDevice  device,
                       BobguiStringList    *list)
{
  uint32_t i;
  uint32_t n_extensions;
  VkExtensionProperties *extensions;

  vkEnumerateDeviceExtensionProperties (device, NULL, &n_extensions, NULL);
  extensions = g_newa (VkExtensionProperties, n_extensions);
  vkEnumerateDeviceExtensionProperties (device, NULL, &n_extensions, extensions);

  for (i = 0; i < n_extensions; i++)
    bobgui_string_list_append (list, extensions[i].extensionName);
}

static void
add_layers (BobguiStringList *list)
{
  uint32_t i;
  uint32_t n_layers;
  VkLayerProperties *layers;

  vkEnumerateInstanceLayerProperties (&n_layers, NULL);
  layers = g_newa (VkLayerProperties, n_layers);
  vkEnumerateInstanceLayerProperties (&n_layers, layers);

  for (i = 0; i < n_layers; i++)
    bobgui_string_list_append (list, layers[i].layerName);
}
#endif

static void
init_vulkan (BobguiInspectorGeneral *gen)
{
#ifdef GDK_RENDERING_VULKAN
  VkPhysicalDevice vk_device;
  VkPhysicalDeviceProperties props;
  char *device_name;
  char *api_version;
  char *driver_version;
  const char *types[] = { "other", "integrated GPU", "discrete GPU", "virtual GPU", "CPU" };
  GError *error = NULL;

  if (!gdk_display_prepare_vulkan (gen->display, &error))
    {
      bobgui_label_set_text (BOBGUI_LABEL (gen->vk_device), C_("Vulkan device", "None"));
      bobgui_widget_set_visible (gen->vk_error_row, TRUE);
      bobgui_label_set_text (BOBGUI_LABEL (gen->vk_error), error->message);
      g_error_free (error);

      bobgui_widget_set_visible (gen->vk_api_version_row, FALSE);
      bobgui_widget_set_visible (gen->vk_driver_version_row, FALSE);
      bobgui_widget_set_visible (gen->vulkan_features_row, FALSE);
      bobgui_widget_set_visible (gen->vulkan_layers_row, FALSE);
      bobgui_widget_set_visible (gen->vulkan_extensions_row, FALSE);
      return;
    }

  vk_device = gen->display->vk_physical_device;
  vkGetPhysicalDeviceProperties (vk_device, &props);

  device_name = g_strdup_printf ("%s (%s)", props.deviceName, types[props.deviceType]);
  api_version = g_strdup_printf ("%d.%d.%d",
                                 VK_VERSION_MAJOR (props.apiVersion),
                                 VK_VERSION_MINOR (props.apiVersion),
                                 VK_VERSION_PATCH (props.apiVersion));
  driver_version = g_strdup_printf ("%d.%d.%d",
                                    VK_VERSION_MAJOR (props.driverVersion),
                                    VK_VERSION_MINOR (props.driverVersion),
                                    VK_VERSION_PATCH (props.driverVersion));

  bobgui_label_set_text (BOBGUI_LABEL (gen->vk_device), device_name);
  bobgui_label_set_text (BOBGUI_LABEL (gen->vk_api_version), api_version);
  bobgui_label_set_text (BOBGUI_LABEL (gen->vk_driver_version), driver_version);

  g_free (device_name);
  g_free (api_version);
  g_free (driver_version);

  add_vulkan_features (gen);
  add_instance_extensions (gen->vulkan_extensions_list);
  add_device_extensions (gen->display->vk_physical_device, gen->vulkan_extensions_list);
  add_layers (gen->vulkan_layers_list);
#else
  bobgui_label_set_text (BOBGUI_LABEL (gen->vk_device), C_("Vulkan device", "None"));
  bobgui_widget_set_visible (gen->vk_api_version_row, FALSE);
  bobgui_widget_set_visible (gen->vk_driver_version_row, FALSE);
  bobgui_widget_set_visible (gen->vulkan_layers_row, FALSE);
  bobgui_widget_set_visible (gen->vulkan_extensions_row, FALSE);
#endif
}

static void
dump_vulkan (GdkDisplay *display,
             GString    *string)
{
#ifdef GDK_RENDERING_VULKAN
  VkPhysicalDevice vk_device;
  VkPhysicalDeviceProperties props;
  char *device_name;
  char *api_version;
  char *driver_version;
  const char *types[] = { "other", "integrated GPU", "discrete GPU", "virtual GPU", "CPU" };
  GError *error = NULL;
  uint32_t n_layers;
  VkLayerProperties *layers;
  uint32_t n_instance_extensions;
  uint32_t n_device_extensions;
  VkExtensionProperties *instance_extensions;
  VkExtensionProperties *device_extensions;

  if (!gdk_display_prepare_vulkan (display, &error))
    {
      g_string_append (string, "| Vulkan Device | None |\n");
      g_string_append_printf (string, "| | %s |\n", error->message);
      g_error_free (error);
      return;
    }

  vk_device = display->vk_physical_device;
  vkGetPhysicalDeviceProperties (vk_device, &props);

  device_name = g_strdup_printf ("%s (%s)", props.deviceName, types[props.deviceType]);
  api_version = g_strdup_printf ("%d.%d.%d",
                                 VK_VERSION_MAJOR (props.apiVersion),
                                 VK_VERSION_MINOR (props.apiVersion),
                                 VK_VERSION_PATCH (props.apiVersion));
  driver_version = g_strdup_printf ("%d.%d.%d",
                                    VK_VERSION_MAJOR (props.driverVersion),
                                    VK_VERSION_MINOR (props.driverVersion),
                                    VK_VERSION_PATCH (props.driverVersion));

  g_string_append_printf (string, "| Vulkan Device | %s |\n", device_name);
  g_string_append_printf (string, "| Vulkan API Version | %s |\n", api_version);
  g_string_append_printf (string, "| Vulkan Driver Version | %s |\n", driver_version);

  g_free (device_name);
  g_free (api_version);
  g_free (driver_version);

  vkEnumerateInstanceLayerProperties (&n_layers, NULL);
  layers = g_newa (VkLayerProperties, n_layers);
  vkEnumerateInstanceLayerProperties (&n_layers, layers);

  g_string_append (string, "| Layers | ");
  for (uint32_t i = 0; i < n_layers; i++)
    g_string_append_printf (string, "%s%s", i > 0 ? "<br>" : "", layers[i].layerName);
  g_string_append (string, " |\n");

  vkEnumerateInstanceExtensionProperties (NULL, &n_instance_extensions, NULL);
  instance_extensions = g_newa (VkExtensionProperties, n_instance_extensions);
  vkEnumerateInstanceExtensionProperties (NULL, &n_instance_extensions, instance_extensions);

  vkEnumerateDeviceExtensionProperties (vk_device, NULL, &n_device_extensions, NULL);
  device_extensions = g_newa (VkExtensionProperties, n_device_extensions);
  vkEnumerateDeviceExtensionProperties (vk_device, NULL, &n_device_extensions, device_extensions);

  g_string_append_printf (string, "| Vulkan Extensions | <details><summary>%u Extensions</summary>", n_instance_extensions + n_device_extensions);

  for (uint32_t i = 0; i < n_instance_extensions; i++)
    g_string_append_printf (string, "%s%s", i > 0 ? "<br>" : "", instance_extensions[i].extensionName);

  for (uint32_t i = 0; i < n_device_extensions; i++)
    g_string_append_printf (string, "%s%s", i > 0 ? "<br>" : "", device_extensions[i].extensionName);

  g_string_append (string, "</details> |\n");

  g_string_append (string, "| Features | ");
  for (int i = 0; i < GDK_VULKAN_N_FEATURES; i++)
    {
      if (gdk_vulkan_has_feature (display, gdk_vulkan_feature_keys[i].value))
        g_string_append_printf (string, "%s%s", i > 0 ? "<br>" : "", gdk_vulkan_feature_keys[i].key);
    }
  g_string_append (string, " |\n");
#endif
}

/* }}} */
/* {{{ Environment */

static const char *env_list[] = {
  "AT_SPI_BUS_ADDRESS",
  "BROADWAY_DISPLAY",
  "DESKTOP_AUTOSTART_ID",
  "DISPLAY",
  "GDK_BACKEND",
  "GDK_DEBUG",
  "GDK_DISABLE",
  "GDK_GL_DISABLE",
  "GDK_SCALE",
  "GDK_SYNCHRONIZE",
  "GDK_VULKAN_DISABLE",
  "GDK_WAYLAND_DISABLE",
  "GDK_WIN32_CAIRO_DB",
  "GDK_WIN32_DISABLE_HIDPI",
  "GDK_WIN32_PER_MONITOR_HIDPI",
  "GDK_WIN32_TABLET_INPUT_API",
  "GOBJECT_DEBUG",
  "GSETINGS_SCHEMA_DIR",
  "GSK_CACHE_TIMEOUT",
  "GSK_DEBUG",
  "GSK_GPU_DISABLE",
  "GSK_RENDERER",
  "GSK_SUBSET_FONTS",
  "BOBGUI_A11Y",
  "BOBGUI_CSD",
  "BOBGUI_CSS_DEBUG",
  "BOBGUI_DATA_PREFIX",
  "BOBGUI_DEBUG",
  "BOBGUI_DEBUG_AUTO_QUIT",
  "BOBGUI_EXE_PREFIX",
  "BOBGUI_IM_MODULE",
  "BOBGUI_INSPECTOR_DISPLAY",
  "BOBGUI_INSPECTOR_RENDERER",
  "BOBGUI_MEDIA",
  "BOBGUI_PATH",
  "BOBGUI_RTL",
  "BOBGUI_SLOWDOWN",
  "BOBGUI_THEME",
  "BOBGUI_WIDGET_ASSERT_COMPONENTS",
  "LANG",
  "LANGUAGE",
  "LC_ALL",
  "LC_CTYPE",
  "LIBGL_ALWAYS_SOFTWARE",
  "LPDEST",
  "MESA_VK_DEVICE_SELECT",
  "PANGOCAIRO_BACKEND",
  "PANGO_LANGUAGE",
  "PRINTER",
  "SECMEM_FORCE_FALLBACK",
  "WAYLAND_DISPLAY",
  "XDG_ACTIVATION_TOKEN",
  "XDG_DATA_HOME",
  "XDG_DATA_DIRS",
  "XDG_RUNTIME_DIR",
};

static void
init_env (BobguiInspectorGeneral *gen)
{
  set_monospace_font (gen->prefix);
  bobgui_label_set_text (BOBGUI_LABEL (gen->prefix), _bobgui_get_data_prefix ());

  for (guint i = 0; i < G_N_ELEMENTS (env_list); i++)
    {
      const char *val = g_getenv (env_list[i]);

      if (val)
        {
          BobguiStringPair *pair = bobgui_string_pair_new (env_list[i], val);
          g_list_store_append (gen->environment_list, pair);
          g_object_unref (pair);
        }
    }
}

static void
dump_env (GdkDisplay *display,
          GString    *s)
{
  g_string_append_printf (s, "| Prefix | %s |\n", _bobgui_get_data_prefix ());

  g_string_append (s, "| Environment| ");
  for (guint i = 0; i < G_N_ELEMENTS (env_list); i++)
    {
      const char *val = g_getenv (env_list[i]);

      if (val)
        g_string_append_printf (s, "%s%s=%s", i > 0 ? "<br>" : "", env_list[i], val);
    }
  g_string_append (s, " |\n");
}

/* }}} */
/* {{{ Display */

static const char *
translate_subpixel_layout (GdkSubpixelLayout subpixel)
{
  switch (subpixel)
    {
    case GDK_SUBPIXEL_LAYOUT_NONE: return "none";
    case GDK_SUBPIXEL_LAYOUT_UNKNOWN: return "unknown";
    case GDK_SUBPIXEL_LAYOUT_HORIZONTAL_RGB: return "horizontal rgb";
    case GDK_SUBPIXEL_LAYOUT_HORIZONTAL_BGR: return "horizontal bgr";
    case GDK_SUBPIXEL_LAYOUT_VERTICAL_RGB: return "vertical rgb";
    case GDK_SUBPIXEL_LAYOUT_VERTICAL_BGR: return "vertical bgr";
    default: g_assert_not_reached (); return "none";
    }
}

#ifdef GDK_WINDOWING_WAYLAND
static void
append_wayland_protocol_row (BobguiInspectorGeneral *gen,
                             struct wl_proxy     *proxy)
{
  const char *id;
  unsigned int version;
  char buf[64];
  BobguiListBox *list;

  if (proxy == NULL)
    return;

  list = BOBGUI_LIST_BOX (gen->display_extensions_box);

  id = wl_proxy_get_class (proxy);
  version = wl_proxy_get_version (proxy);

  g_snprintf (buf, sizeof (buf), "%u", version);

  add_label_row (gen, list, id, buf, 10);
}

static void
append_wayland_protocol (GString         *string,
                         struct wl_proxy *proxy,
                         guint           *count)
{
  if (proxy == NULL)
    return;

  g_string_append_printf (string, "%s%s (%u)",
                          *count == 0 ? "" : "<br>",
                          wl_proxy_get_class (proxy),
                          wl_proxy_get_version (proxy));
  (*count)++;
}

static void
add_wayland_protocols (GdkDisplay          *display,
                       BobguiInspectorGeneral *gen)
{
  if (GDK_IS_WAYLAND_DISPLAY (display))
    {
      GdkWaylandDisplay *d = (GdkWaylandDisplay *) display;

      append_wayland_protocol_row (gen, (struct wl_proxy *)d->compositor);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->shm);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->linux_dmabuf);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->xdg_wm_base);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->zxdg_shell_v6);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->bobgui_shell);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->data_device_manager);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->subcompositor);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->pointer_gestures);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->primary_selection_manager);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->tablet_manager);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->xdg_exporter);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->xdg_exporter_v2);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->xdg_importer);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->xdg_importer_v2);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->keyboard_shortcuts_inhibit);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->server_decoration_manager);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->xdg_output_manager);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->idle_inhibit_manager);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->xdg_activation);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->fractional_scale);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->viewporter);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->presentation);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->single_pixel_buffer);
      append_wayland_protocol_row (gen, d->color ? gdk_wayland_color_get_color_manager (d->color) : NULL);
      append_wayland_protocol_row (gen, d->color ? gdk_wayland_color_get_color_representation_manager (d->color) : NULL);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->system_bell);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->cursor_shape);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->toplevel_icon);
      append_wayland_protocol_row (gen, (struct wl_proxy *)d->xx_session_manager);
      append_wayland_protocol_row (gen, bobgui_im_context_wayland_get_text_protocol (display));
    }
}

static void
dump_wayland_protocols (GdkDisplay *display,
                        GString    *string)
{
  if (GDK_IS_WAYLAND_DISPLAY (display))
    {
      GdkWaylandDisplay *d = (GdkWaylandDisplay *) display;
      guint count = 0;

      g_string_append (string, "| Protocols | ");

      append_wayland_protocol (string, (struct wl_proxy *)d->compositor, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->shm, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->linux_dmabuf, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->xdg_wm_base, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->zxdg_shell_v6, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->bobgui_shell, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->data_device_manager, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->subcompositor, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->pointer_gestures, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->primary_selection_manager, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->tablet_manager, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->xdg_exporter, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->xdg_exporter_v2, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->xdg_importer, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->xdg_importer_v2, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->keyboard_shortcuts_inhibit, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->server_decoration_manager, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->xdg_output_manager, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->idle_inhibit_manager, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->xdg_activation, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->fractional_scale, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->viewporter, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->presentation, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->single_pixel_buffer, &count);
      append_wayland_protocol (string, d->color ? gdk_wayland_color_get_color_manager (d->color) : NULL, &count);
      append_wayland_protocol (string, d->color ? gdk_wayland_color_get_color_representation_manager (d->color) : NULL, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->system_bell, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->cursor_shape, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->toplevel_icon, &count);
      append_wayland_protocol (string, (struct wl_proxy *)d->xx_session_manager, &count);
      append_wayland_protocol (string , bobgui_im_context_wayland_get_text_protocol (display), &count);

      g_string_append (string, " |\n");
    }
}
#endif

static void
populate_display (GdkDisplay *display, BobguiInspectorGeneral *gen)
{
  GList *children, *l;
  BobguiWidget *child;
  BobguiListBox *list;

  list = BOBGUI_LIST_BOX (gen->display_box);
  children = NULL;
  for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (list));
       child != NULL;
       child = bobgui_widget_get_next_sibling (child))
    {
      if (BOBGUI_IS_LIST_BOX_ROW (child))
        children = g_list_prepend (children, child);
    }
  for (l = children; l; l = l->next)
    {
      child = l->data;
      if (bobgui_widget_is_ancestor (gen->display_name, child) ||
          bobgui_widget_is_ancestor (gen->display_rgba, child) ||
          bobgui_widget_is_ancestor (gen->display_composited, child) ||
          child == gen->display_extensions_row)
        continue;

      bobgui_list_box_remove (list, child);
    }
  g_list_free (children);

  bobgui_label_set_label (BOBGUI_LABEL (gen->display_name), gdk_display_get_name (display));

  bobgui_widget_set_visible (gen->display_rgba,
                          gdk_display_is_rgba (display));
  bobgui_widget_set_visible (gen->display_composited,
                          gdk_display_is_composited (display));

  list = BOBGUI_LIST_BOX (gen->display_extensions_box);
  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (list))) != NULL)
    bobgui_list_box_remove (list, child);

#ifdef GDK_WINDOWING_WAYLAND
  add_wayland_protocols (display, gen);
#endif

  bobgui_widget_set_visible (gen->display_extensions_row,
                          bobgui_widget_get_first_child (gen->display_extensions_box) != NULL);
}

static void
populate_display_notify_cb (GdkDisplay          *display,
                            GParamSpec          *pspec,
                            BobguiInspectorGeneral *gen)
{
  populate_display (display, gen);
}

static void
init_display (BobguiInspectorGeneral *gen)
{
  g_signal_connect (gen->display, "notify", G_CALLBACK (populate_display_notify_cb), gen);

  populate_display (gen->display, gen);
}

static void
dump_display (GdkDisplay *display,
              GString    *string)
{
  g_string_append_printf (string, "| Display | %s |\n", gdk_display_get_name (display));
  g_string_append_printf (string, "| RGBA Visual | %s |\n", gdk_display_is_rgba (display) ? "✓" : "✗");
  g_string_append_printf (string, "| Composited | %s |\n", gdk_display_is_composited (display) ? "✓" : "✗");
#ifdef GDK_WINDOWING_WAYLAND
  dump_wayland_protocols (display, string);
#endif
}

/* }}} */
/* {{{ Monitors */

static void
add_monitor (BobguiInspectorGeneral *gen,
             GdkMonitor          *monitor,
             guint                i)
{
  BobguiListBox *list;
  char *value;
  GdkRectangle rect;
  double scale;
  char *name;
  char *scale_str = NULL;
  const char *manufacturer;
  const char *model;

  list = BOBGUI_LIST_BOX (gen->monitor_box);

  name = g_strdup_printf ("Monitor %u", i);
  manufacturer = gdk_monitor_get_manufacturer (monitor);
  model = gdk_monitor_get_model (monitor);
  value = g_strdup_printf ("%s%s%s",
                           manufacturer ? manufacturer : "",
                           manufacturer || model ? " " : "",
                           model ? model : "");
  add_label_row (gen, list, name, value, 0);
  g_free (value);
  g_free (name);

  add_label_row (gen, list, "Description", gdk_monitor_get_description (monitor), 10);
  add_label_row (gen, list, "Connector", gdk_monitor_get_connector (monitor), 10);

  gdk_monitor_get_geometry (monitor, &rect);
  scale = gdk_monitor_get_scale (monitor);
  if (scale != 1.0)
    scale_str = g_strdup_printf (" @ %.2f", scale);

  value = g_strdup_printf ("%d × %d%s at %d, %d",
                           rect.width, rect.height,
                           scale_str ? scale_str : "",
                           rect.x, rect.y);
  add_label_row (gen, list, "Geometry", value, 10);
  g_free (value);
  g_free (scale_str);

  value = g_strdup_printf ("%d × %d",
                           (int) (rect.width * scale),
                           (int) (rect.height * scale));
  add_label_row (gen, list, "Pixels", value, 10);
  g_free (value);

  value = g_strdup_printf ("%d × %d mm²",
                           gdk_monitor_get_width_mm (monitor),
                           gdk_monitor_get_height_mm (monitor));
  add_label_row (gen, list, "Size", value, 10);
  g_free (value);

  value = g_strdup_printf ("%.1f dpi", gdk_monitor_get_dpi (monitor));
  add_label_row (gen, list, "Resolution", value, 10);
  g_free (value);

  if (gdk_monitor_get_refresh_rate (monitor) != 0)
    {
      value = g_strdup_printf ("%.2f Hz",
                               0.001 * gdk_monitor_get_refresh_rate (monitor));
      add_label_row (gen, list, "Refresh rate", value, 10);
      g_free (value);
    }

  if (gdk_monitor_get_subpixel_layout (monitor) != GDK_SUBPIXEL_LAYOUT_UNKNOWN)
    {
      add_label_row (gen, list, "Subpixel layout",
                     translate_subpixel_layout (gdk_monitor_get_subpixel_layout (monitor)),
                     10);
    }
}

static void
dump_monitor (GdkMonitor *monitor,
              guint       i,
              GString    *string)
{
  const char *manufacturer;
  const char *model;
  char *value;
  GdkRectangle rect;
  double scale;
  char *scale_str = NULL;

  manufacturer = gdk_monitor_get_manufacturer (monitor);
  model = gdk_monitor_get_model (monitor);
  value = g_strdup_printf ("%s%s%s",
                           manufacturer ? manufacturer : "",
                           manufacturer || model ? " " : "",
                           model ? model : "");
  g_string_append_printf (string, "| Monitor %u | %s |\n", i, value);
  g_free (value);

  g_string_append_printf (string, "| Description | %s |\n", gdk_monitor_get_description (monitor));
  g_string_append_printf (string, "| Connector | %s |\n", gdk_monitor_get_connector (monitor));

  gdk_monitor_get_geometry (monitor, &rect);
  scale = gdk_monitor_get_scale (monitor);
  if (scale != 1.0)
    scale_str = g_strdup_printf (" @ %.2f", scale);

  value = g_strdup_printf ("%d × %d%s at %d, %d",
                           rect.width, rect.height,
                           scale_str ? scale_str : "",
                           rect.x, rect.y);
  g_string_append_printf (string, "| Geometry | %s |\n", value);
  g_free (value);
  g_free (scale_str);

  value = g_strdup_printf ("%d × %d",
                           (int) (rect.width * scale),
                           (int) (rect.height * scale));
  g_string_append_printf (string, "| Pixels | %s |\n", value);
  g_free (value);

  value = g_strdup_printf ("%d × %d mm²",
                           gdk_monitor_get_width_mm (monitor),
                           gdk_monitor_get_height_mm (monitor));
  g_string_append_printf (string, "| Size | %s |\n", value);
  g_free (value);

  value = g_strdup_printf ("%.1f dpi", gdk_monitor_get_dpi (monitor));
  g_string_append_printf (string, "| Resolution | %s |\n", value);
  g_free (value);

  if (gdk_monitor_get_refresh_rate (monitor) != 0)
    {
      value = g_strdup_printf ("%.2f Hz",
                               0.001 * gdk_monitor_get_refresh_rate (monitor));
      g_string_append_printf (string, "| Refresh Rate | %s |\n", value);
      g_free (value);
    }

  if (gdk_monitor_get_subpixel_layout (monitor) != GDK_SUBPIXEL_LAYOUT_UNKNOWN)
    {
      g_string_append_printf (string, "| Subpixel Layout | %s |\n",
                              translate_subpixel_layout (gdk_monitor_get_subpixel_layout (monitor)));
    }
}

static void
populate_monitors (GdkDisplay          *display,
                   BobguiInspectorGeneral *gen)
{
  BobguiWidget *child;
  GListModel *list;

  while ((child = bobgui_widget_get_first_child (gen->monitor_box)))
    bobgui_list_box_remove (BOBGUI_LIST_BOX (gen->monitor_box), child);

  list = gdk_display_get_monitors (gen->display);

  for (guint i = 0; i < g_list_model_get_n_items (list); i++)
    {
      GdkMonitor *monitor = g_list_model_get_item (list, i);
      add_monitor (gen, monitor, i);
      g_object_unref (monitor);
    }
}

static void
dump_monitors (GdkDisplay *display,
               GString    *string)
{
  GListModel *list;

  list = gdk_display_get_monitors (display);

  for (guint i = 0; i < g_list_model_get_n_items (list); i++)
    {
      GdkMonitor *monitor = g_list_model_get_item (list, i);
      dump_monitor (monitor, i, string);
      g_object_unref (monitor);
    }
}

static void
monitors_changed_cb (GListModel          *monitors,
                     guint                position,
                     guint                removed,
                     guint                added,
                     BobguiInspectorGeneral *gen)
{
  populate_monitors (gen->display, gen);
}

static void
init_monitors (BobguiInspectorGeneral *gen)
{
  g_signal_connect (gdk_display_get_monitors (gen->display), "items-changed",
                    G_CALLBACK (monitors_changed_cb), gen);

  populate_monitors (gen->display, gen);
}

/* }}} */
/* {{{ Seats */

static void populate_seats (BobguiInspectorGeneral *gen);

static void
add_tool (BobguiInspectorGeneral *gen,
          GdkDeviceTool       *tool)
{
  GdkAxisFlags axes;
  GString *str;
  char *val;
  GEnumClass *eclass;
  GEnumValue *evalue;
  GFlagsClass *fclass;
  GFlagsValue *fvalue;

  val = g_strdup_printf ("Serial %" G_GUINT64_FORMAT, gdk_device_tool_get_serial (tool));
  add_label_row (gen, BOBGUI_LIST_BOX (gen->device_box), "Tool", val, 10);
  g_free (val);

  eclass = g_type_class_ref (GDK_TYPE_DEVICE_TOOL_TYPE);
  evalue = g_enum_get_value (eclass, gdk_device_tool_get_tool_type (tool));
  add_label_row (gen, BOBGUI_LIST_BOX (gen->device_box), "Type", evalue->value_nick, 20);
  g_type_class_unref (eclass);

  fclass = g_type_class_ref (GDK_TYPE_AXIS_FLAGS);
  str = g_string_new ("");
  axes = gdk_device_tool_get_axes (tool);
  while ((fvalue = g_flags_get_first_value (fclass, axes)))
    {
      if (str->len > 0)
        g_string_append (str, ", ");
      g_string_append (str, fvalue->value_nick);
      axes &= ~fvalue->value;
    }
  g_type_class_unref (fclass);

  if (str->len > 0)
    add_label_row (gen, BOBGUI_LIST_BOX (gen->device_box), "Axes", str->str, 20);

  g_string_free (str, TRUE);
}

static void
dump_tool (GdkDeviceTool *tool,
           GString       *string)
{
  GdkAxisFlags axes;
  GString *str;
  char *val;
  GEnumClass *eclass;
  GEnumValue *evalue;
  GFlagsClass *fclass;
  GFlagsValue *fvalue;

  val = g_strdup_printf ("Serial %" G_GUINT64_FORMAT, gdk_device_tool_get_serial (tool));
  g_string_append_printf (string, "| Tool | %s |\n", val);
  g_free (val);

  eclass = g_type_class_ref (GDK_TYPE_DEVICE_TOOL_TYPE);
  evalue = g_enum_get_value (eclass, gdk_device_tool_get_tool_type (tool));
  g_string_append_printf (string, "| Type | %s |\n", evalue->value_nick);
  g_type_class_unref (eclass);

  fclass = g_type_class_ref (GDK_TYPE_AXIS_FLAGS);
  str = g_string_new ("");
  axes = gdk_device_tool_get_axes (tool);
  while ((fvalue = g_flags_get_first_value (fclass, axes)))
    {
      if (str->len > 0)
        g_string_append (str, "<br>");
      g_string_append (str, fvalue->value_nick);
      axes &= ~fvalue->value;
    }
  g_type_class_unref (fclass);

  if (str->len > 0)
    g_string_append_printf (string, "| Axes | %s |\n", str->str);

  g_string_free (str, TRUE);
}

static char *
layout_names_from_device (GdkDevice *device)
{
  GString *s = g_string_new ("");
  char **layout_names = gdk_device_get_layout_names (device);

  if (layout_names)
    {
      int n_layouts, active_layout;

      active_layout = gdk_device_get_active_layout_index (device);
      n_layouts = g_strv_length (layout_names);
      for (int i = 0; i < n_layouts; i++)
        {
          if (s->len > 0)
            g_string_append (s, ", ");
          g_string_append (s, layout_names[i]);
          if (i == active_layout)
            g_string_append (s, "*");
        }
    }
  else
    {
      g_string_append (s, "Unknown");
    }

  return g_string_free_and_steal (s);
}

static void
on_keyboard_device_notify (GdkDevice  *keyboard,
                           GParamSpec *pspec,
                           BobguiWidget  *label)
{
  if (g_strcmp0 (pspec->name, "layout-names") == 0 || g_strcmp0 (pspec->name, "active-layout-index") == 0)
    {
      gchar *layouts = layout_names_from_device (keyboard);
      bobgui_label_set_label (BOBGUI_LABEL (label), layouts);
      g_free (layouts);
    }
}

static void
add_device (BobguiInspectorGeneral *gen,
            GdkDevice           *device)
{
  const char *name;
  guint n_touches;
  char *text;
  GEnumClass *class;
  GEnumValue *value;

  name = gdk_device_get_name (device);

  class = g_type_class_ref (GDK_TYPE_INPUT_SOURCE);
  value = g_enum_get_value (class, gdk_device_get_source (device));

  add_label_row (gen, BOBGUI_LIST_BOX (gen->device_box), name, value->value_nick, 10);

  g_object_get (device, "num-touches", &n_touches, NULL);
  if (n_touches > 0)
    {
      text = g_strdup_printf ("%d", n_touches);
      add_label_row (gen, BOBGUI_LIST_BOX (gen->device_box), "Touches", text, 20);
      g_free (text);
    }

  if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
    {
      BobguiWidget *label;

      text = layout_names_from_device (device);
      label = add_label_row (gen, BOBGUI_LIST_BOX (gen->device_box), "Layouts", text, 20);
      g_free (text);

      /* Connect to associated device if exists to get layout changes */
      /* NOTE: there is no public or private API to get the associated device */
      g_signal_connect_object (device->associated ? device->associated : device,
                               "notify",
                               G_CALLBACK (on_keyboard_device_notify),
                               label,
                               G_CONNECT_DEFAULT);
    }

  g_type_class_unref (class);
}

static void
dump_device (GdkDevice *device,
             GString   *string)
{
  const char *name;
  guint n_touches;
  GEnumClass *class;
  GEnumValue *value;

  name = gdk_device_get_name (device);

  class = g_type_class_ref (GDK_TYPE_INPUT_SOURCE);
  value = g_enum_get_value (class, gdk_device_get_source (device));

  g_string_append_printf (string, "| %s | %s |\n", name, value->value_nick);

  g_object_get (device, "num-touches", &n_touches, NULL);
  if (n_touches > 0)
    g_string_append_printf (string, "| Touches | %d |\n", n_touches);

  if (gdk_device_get_source (device) == GDK_SOURCE_KEYBOARD)
    {
      char **layout_names;
      int n_layouts, active_layout;
      GString *s;

      layout_names = gdk_device_get_layout_names (device);
      active_layout = gdk_device_get_active_layout_index (device);
      n_layouts = g_strv_length (layout_names);
      s = g_string_new ("");
      for (int i = 0; i < n_layouts; i++)
        {
          if (s->len > 0)
            g_string_append (s, "<br>");
          g_string_append (s, layout_names[i]);
          if (i == active_layout)
            g_string_append (s, "*");
        }

       g_string_append_printf (string, "| Layouts | %s |\n", s->str);
       g_string_free (s, TRUE);
    }

  g_type_class_unref (class);
}

static char *
get_seat_capabilities (GdkSeat *seat)
{
  struct {
    GdkSeatCapabilities cap;
    const char *name;
  } caps[] = {
    { GDK_SEAT_CAPABILITY_POINTER,       "Pointer" },
    { GDK_SEAT_CAPABILITY_TOUCH,         "Touch" },
    { GDK_SEAT_CAPABILITY_TABLET_STYLUS, "Tablet" },
    { GDK_SEAT_CAPABILITY_KEYBOARD,      "Keyboard" },
    { 0, NULL }
  };
  GString *str;
  GdkSeatCapabilities capabilities;
  int i;

  str = g_string_new ("");
  capabilities = gdk_seat_get_capabilities (seat);
  for (i = 0; caps[i].cap != 0; i++)
    {
      if (capabilities & caps[i].cap)
        {
          if (str->len > 0)
            g_string_append (str, ", ");
          g_string_append (str, caps[i].name);
        }
    }

  return g_string_free (str, FALSE);
}

static void
add_seat (BobguiInspectorGeneral *gen,
          GdkSeat             *seat,
          int                  num)
{
  char *text;
  char *caps;
  GList *list, *l;

  if (!g_object_get_data (G_OBJECT (seat), "inspector-connected"))
    {
      g_object_set_data (G_OBJECT (seat), "inspector-connected", GINT_TO_POINTER (1));
      g_signal_connect_swapped (seat, "device-added", G_CALLBACK (populate_seats), gen);
      g_signal_connect_swapped (seat, "device-removed", G_CALLBACK (populate_seats), gen);
      g_signal_connect_swapped (seat, "tool-added", G_CALLBACK (populate_seats), gen);
      g_signal_connect_swapped (seat, "tool-removed", G_CALLBACK (populate_seats), gen);
    }

  text = g_strdup_printf ("Seat %d", num);
  caps = get_seat_capabilities (seat);

  add_label_row (gen, BOBGUI_LIST_BOX (gen->device_box), text, caps, 0);
  g_free (text);
  g_free (caps);

  list = gdk_seat_get_devices (seat, GDK_SEAT_CAPABILITY_ALL);

  for (l = list; l; l = l->next)
    add_device (gen, GDK_DEVICE (l->data));

  g_list_free (list);

  list = gdk_seat_get_tools (seat);

  for (l = list; l; l = l->next)
    add_tool (gen, l->data);

  g_list_free (list);
}

static void
dump_seat (GdkSeat *seat,
           int      i,
           GString *string)
{
  char *caps;
  GList *list, *l;

  caps = get_seat_capabilities (seat);
  g_string_append_printf (string, "| Seat %d | %s |\n", i, caps);
  g_free (caps);

  list = gdk_seat_get_devices (seat, GDK_SEAT_CAPABILITY_ALL);

  for (l = list; l; l = l->next)
    dump_device (GDK_DEVICE (l->data), string);

  g_list_free (list);

  list = gdk_seat_get_tools (seat);

  for (l = list; l; l = l->next)
    dump_tool (l->data, string);

  g_list_free (list);
}

static void
disconnect_seat (BobguiInspectorGeneral *gen,
                 GdkSeat             *seat)
{
  g_signal_handlers_disconnect_by_func (seat, G_CALLBACK (populate_seats), gen);
}

static void
populate_seats (BobguiInspectorGeneral *gen)
{
  BobguiWidget *child;
  GList *list, *l;
  int i;

  while ((child = bobgui_widget_get_first_child (gen->device_box)))
    bobgui_list_box_remove (BOBGUI_LIST_BOX (gen->device_box), child);

  list = gdk_display_list_seats (gen->display);

  for (l = list, i = 0; l; l = l->next, i++)
    add_seat (gen, GDK_SEAT (l->data), i);

  g_list_free (list);
}

static void
dump_seats (GdkDisplay *display,
            GString    *string)
{
  GList *list, *l;
  int i;

  list = gdk_display_list_seats (display);

  for (l = list, i = 0; l; l = l->next, i++)
    dump_seat (GDK_SEAT (l->data), i, string);

  g_list_free (list);
}

static void
seat_added (GdkDisplay          *display,
            GdkSeat             *seat,
            BobguiInspectorGeneral *gen)
{
  populate_seats (gen);
}

static void
seat_removed (GdkDisplay          *display,
              GdkSeat             *seat,
              BobguiInspectorGeneral *gen)
{
  disconnect_seat (gen, seat);
  populate_seats (gen);
}

static void
init_seats (BobguiInspectorGeneral *gen)
{
  g_signal_connect (gen->display, "seat-added", G_CALLBACK (seat_added), gen);
  g_signal_connect (gen->display, "seat-removed", G_CALLBACK (seat_removed), gen);

  populate_seats (gen);
}

 /* }}} */

static void
bobgui_inspector_general_init (BobguiInspectorGeneral *gen)
{
  bobgui_widget_init_template (BOBGUI_WIDGET (gen));
}

static gboolean
keynav_failed (BobguiWidget *widget, BobguiDirectionType direction, BobguiInspectorGeneral *gen)
{
  BobguiWidget *next;

  if (direction == BOBGUI_DIR_DOWN && widget == gen->version_box)
    next = gen->env_box;
  else if (direction == BOBGUI_DIR_DOWN && widget == gen->env_box)
    next = gen->display_box;
  else if (direction == BOBGUI_DIR_DOWN && widget == gen->display_box)
    next = gen->monitor_box;
  else if (direction == BOBGUI_DIR_DOWN && widget == gen->monitor_box)
    next = gen->device_box;
  else if (direction == BOBGUI_DIR_DOWN && widget == gen->device_box)
    next = gen->gl_box;
  else if (direction == BOBGUI_DIR_DOWN && widget == gen->gl_box)
    next = gen->vulkan_box;
  else if (direction == BOBGUI_DIR_UP && widget == gen->vulkan_box)
    next = gen->gl_box;
  else if (direction == BOBGUI_DIR_UP && widget == gen->gl_box)
    next = gen->device_box;
  else if (direction == BOBGUI_DIR_UP && widget == gen->device_box)
    next = gen->monitor_box;
  else if (direction == BOBGUI_DIR_UP && widget == gen->monitor_box)
    next = gen->display_box;
  else if (direction == BOBGUI_DIR_UP && widget == gen->display_box)
    next = gen->env_box;
  else if (direction == BOBGUI_DIR_UP && widget == gen->env_box)
    next = gen->version_box;
  else
    next = NULL;

  if (next)
    {
      bobgui_widget_child_focus (next, direction);
      return TRUE;
    }

  return FALSE;
}

static void
bobgui_inspector_general_constructed (GObject *object)
{
  BobguiInspectorGeneral *gen = BOBGUI_INSPECTOR_GENERAL (object);

  G_OBJECT_CLASS (bobgui_inspector_general_parent_class)->constructed (object);

   g_signal_connect (gen->version_box, "keynav-failed", G_CALLBACK (keynav_failed), gen);
   g_signal_connect (gen->env_box, "keynav-failed", G_CALLBACK (keynav_failed), gen);
   g_signal_connect (gen->display_box, "keynav-failed", G_CALLBACK (keynav_failed), gen);
   g_signal_connect (gen->monitor_box, "keynav-failed", G_CALLBACK (keynav_failed), gen);
   g_signal_connect (gen->gl_box, "keynav-failed", G_CALLBACK (keynav_failed), gen);
   g_signal_connect (gen->vulkan_box, "keynav-failed", G_CALLBACK (keynav_failed), gen);
   g_signal_connect (gen->device_box, "keynav-failed", G_CALLBACK (keynav_failed), gen);
}

static void
bobgui_inspector_general_dispose (GObject *object)
{
  BobguiInspectorGeneral *gen = BOBGUI_INSPECTOR_GENERAL (object);
  GList *list, *l;

  g_signal_handlers_disconnect_by_func (gen->display, G_CALLBACK (seat_added), gen);
  g_signal_handlers_disconnect_by_func (gen->display, G_CALLBACK (seat_removed), gen);
  g_signal_handlers_disconnect_by_func (gen->display, G_CALLBACK (populate_display_notify_cb), gen);
  g_signal_handlers_disconnect_by_func (gdk_display_get_monitors (gen->display), G_CALLBACK (monitors_changed_cb), gen);

  list = gdk_display_list_seats (gen->display);
  for (l = list; l; l = l->next)
    disconnect_seat (gen, GDK_SEAT (l->data));
  g_list_free (list);

  bobgui_widget_dispose_template (BOBGUI_WIDGET (gen), BOBGUI_TYPE_INSPECTOR_GENERAL);

  G_OBJECT_CLASS (bobgui_inspector_general_parent_class)->dispose (object);
}

static void
bobgui_inspector_general_class_init (BobguiInspectorGeneralClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  g_type_ensure (BOBGUI_TYPE_STRING_PAIR);

  object_class->constructed = bobgui_inspector_general_constructed;
  object_class->dispose = bobgui_inspector_general_dispose;

  bobgui_widget_class_set_template_from_resource (widget_class, "/org/bobgui/libbobgui/inspector/general.ui");
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, swin);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, version_box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, env_box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, display_box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, display_extensions_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, display_extensions_box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, monitor_box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, gl_box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, gl_features_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, gl_features_box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, gl_extensions_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, gl_extensions_list);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, egl_extensions_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, egl_extensions_row_name);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, egl_extensions_list);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, vulkan_box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, vulkan_features_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, vulkan_features_box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, vulkan_extensions_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, vulkan_extensions_list);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, vulkan_layers_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, vulkan_layers_list);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, os_info);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, bobgui_version);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, gdk_backend);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, gsk_renderer);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, pango_fontmap);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, media_backend);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, im_module);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, a11y_backend);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, gl_error);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, gl_error_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, gl_version);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, gl_version_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, gl_backend_version);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, gl_backend_version_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, gl_backend_vendor);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, gl_backend_vendor_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, gl_vendor);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, gl_vendor_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, gl_renderer);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, gl_renderer_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, gl_full_version);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, gl_full_version_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, glsl_version);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, glsl_version_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, vk_device);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, vk_api_version);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, vk_api_version_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, vk_driver_version);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, vk_driver_version_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, vk_error);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, vk_error_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, app_id_box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, app_id);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, resource_path);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, prefix);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, environment_row);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, environment_list);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, display_name);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, display_composited);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, display_rgba);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, device_box);
  bobgui_widget_class_bind_template_child (widget_class, BobguiInspectorGeneral, overlay);

  bobgui_widget_class_bind_template_callback (widget_class, bobgui_inspector_general_clip);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BIN_LAYOUT);
}

void
bobgui_inspector_general_set_display (BobguiInspectorGeneral *gen,
                                   GdkDisplay *display)
{
  gen->display = display;

  init_os_info (gen);
  init_version (gen);
  init_pango (gen);
  init_media (gen);
  init_im_module (gen);
  init_a11y_backend (gen);
  init_app_id (gen);
  init_env (gen);
  init_display (gen);
  init_monitors (gen);
  init_seats (gen);
  init_gl (gen);
  init_vulkan (gen);
}

static char *
generate_dump (GdkDisplay *display)
{
  GString *string;

  string = g_string_new ("");

  g_string_append (string, "\n<details open=\"true\"><summary>General Information</summary>\n\n");
  g_string_append (string, "| Name | Value |\n");
  g_string_append (string, "| - | - |\n");
  dump_os_info (display, string);
  dump_version (display, string);
  dump_pango (display, string);
  dump_media (display, string);
  dump_im_module (display, string);
  dump_a11y_backend (display, string);
  g_string_append (string, "\n</details>\n");

  g_string_append (string, "\n<details><summary>Application</summary>\n\n");
  g_string_append (string, "| Name | Value |\n");
  g_string_append (string, "| - | - |\n");
  dump_app_id (display, string);
  g_string_append (string, "\n</details>\n");

  g_string_append (string, "<details><summary>Environment</summary>\n\n");
  g_string_append (string, "| Name | Value |\n");
  g_string_append (string, "| - | - |\n");
  dump_env (display, string);
  g_string_append (string, "\n</details>\n");

  g_string_append (string, "<details><summary>Display</summary>\n\n");
  g_string_append (string, "| Name | Value |\n");
  g_string_append (string, "| - | - |\n");
  dump_display (display, string);
  g_string_append (string, "\n</details>\n");

  g_string_append (string, "<details><summary>Monitors</summary>\n\n");
  g_string_append (string, "| Name | Value |\n");
  g_string_append (string, "| - | - |\n");
  dump_monitors (display, string);
  g_string_append (string, "\n</details>\n");

  g_string_append (string, "<details><summary>Seats</summary>\n\n");
  g_string_append (string, "| Name | Value |\n");
  g_string_append (string, "| - | - |\n");
  dump_seats (display, string);
  g_string_append (string, "\n</details>\n");

  g_string_append (string, "<details><summary>OpenGL</summary>\n\n");
  g_string_append (string, "| Name | Value |\n");
  g_string_append (string, "| - | - |\n");
  dump_gl (display, string);
  g_string_append (string, "\n</details>\n");

  g_string_append (string, "<details><summary>Vulkan</summary>\n\n");
  g_string_append (string, "| Name | Value |\n");
  g_string_append (string, "| - | - |\n");
  dump_vulkan (display, string);
  g_string_append (string, "\n</details>\n");

  return g_string_free (string, FALSE);
}

static void
bobgui_inspector_general_clip (BobguiButton           *button,
                            BobguiInspectorGeneral *gen)
{
  char *text;
  GdkClipboard *clipboard;

  text = generate_dump (gen->display);

  clipboard = bobgui_widget_get_clipboard (BOBGUI_WIDGET (gen));
  gdk_clipboard_set_text (clipboard, text);

  g_free (text);
}

/* vim:set foldmethod=marker: */
