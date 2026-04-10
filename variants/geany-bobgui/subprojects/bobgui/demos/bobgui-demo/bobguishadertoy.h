#pragma once

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_SHADERTOY      (bobgui_shadertoy_get_type ())
#define BOBGUI_SHADERTOY(inst)     (G_TYPE_CHECK_INSTANCE_CAST ((inst), \
                                 BOBGUI_TYPE_SHADERTOY,                 \
                                 BobguiShadertoy))
#define BOBGUI_IS_SHADERTOY(inst)  (G_TYPE_CHECK_INSTANCE_TYPE ((inst), \
                                 BOBGUI_TYPE_SHADERTOY))

typedef struct _BobguiShadertoy BobguiShadertoy;
typedef struct _BobguiShadertoyClass BobguiShadertoyClass;

struct _BobguiShadertoy {
  BobguiGLArea parent;
};

struct _BobguiShadertoyClass {
  BobguiGLAreaClass parent_class;
};

GType       bobgui_shadertoy_get_type         (void) G_GNUC_CONST;
BobguiWidget  *bobgui_shadertoy_new              (void);
const char *bobgui_shadertoy_get_image_shader (BobguiShadertoy *shadertoy);
void        bobgui_shadertoy_set_image_shader (BobguiShadertoy *shadertoy,
                                            const char   *shader);

G_END_DECLS
