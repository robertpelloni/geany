#pragma once

#include "bobguiicontheme.h"
#include "bobguiiconpaintable.h"
#include "bobgui/svg/bobguisvg.h"
#include "gsk/gsktypes.h"

typedef struct {
  char **icon_names;
  int size;
  int scale;
  BobguiIconLookupFlags flags;
} IconKey;

struct _BobguiIconPaintable
{
  GObject parent_instance;

  /* Information about the source
   */
  IconKey key;
  BobguiIconTheme *in_cache; /* Protected by icon_cache lock */

  char *icon_name;
  char *filename;
  GLoadableIcon *loadable;

  /* Parameters influencing the scaled icon
   */
  int desired_size;
  int desired_scale;
  guint is_svg          : 1;
  guint is_resource     : 1;
  guint is_symbolic     : 1;

  /* Cached information if we go ahead and try to load the icon.
   *
   * All access to these are protected by the texture_lock. Everything
   * above is immutable after construction and can be used without
   * locks.
   */
  GMutex texture_lock;

  GdkPaintable *paintable;
  double width;
  double height;
};

BobguiIconPaintable *bobgui_icon_paintable_new_for_texture (GdkTexture *texture,
                                                      int         desired_size,
                                                      int         desired_scale);
BobguiIconPaintable *bobgui_icon_paintable_new_for_path     (const char *path,
                                                       gboolean    is_resource,
                                                       int         desired_size,
                                                       int         desired_scale);
BobguiIconPaintable *bobgui_icon_paintable_new_for_loadable (GLoadableIcon *loadable,
                                                       int            desired_size,
                                                       int            desired_scale);

void bobgui_icon_paintable_set_debug (BobguiIconPaintable *icon,
                                   gboolean          allow_node,
                                   gboolean          allow_recolor,
                                   gboolean          allow_mask);
void bobgui_icon_paintable_set_icon_name (BobguiIconPaintable *icon,
                                       const char       *name);

void bobgui_icon_paintable_load_in_thread (BobguiIconPaintable *self);

