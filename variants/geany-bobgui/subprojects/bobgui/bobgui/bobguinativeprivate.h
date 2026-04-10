#pragma once

#include "bobguinative.h"

G_BEGIN_DECLS

/**
 * BobguiNativeIface:
 *
 * The list of functions that must be implemented for the `BobguiNative` interface.
 */
struct _BobguiNativeInterface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/
  GdkSurface *  (* get_surface)           (BobguiNative    *self);
  GskRenderer * (* get_renderer)          (BobguiNative    *self);

  void          (* get_surface_transform) (BobguiNative    *self,
                                           double       *x,
                                           double       *y);

  void          (* layout)                (BobguiNative    *self,
                                           int           width,
                                           int           height);
};

void    bobgui_native_queue_relayout         (BobguiNative    *native);

G_END_DECLS

