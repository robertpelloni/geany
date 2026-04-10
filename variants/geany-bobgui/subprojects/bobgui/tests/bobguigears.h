#pragma once

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

enum {
  BOBGUI_GEARS_X_AXIS,
  BOBGUI_GEARS_Y_AXIS,
  BOBGUI_GEARS_Z_AXIS,

  BOBGUI_GEARS_N_AXIS
};

#define BOBGUI_TYPE_GEARS      (bobgui_gears_get_type ())
#define BOBGUI_GEARS(inst)     (G_TYPE_CHECK_INSTANCE_CAST ((inst), \
                             BOBGUI_TYPE_GEARS,             \
                             BobguiGears))
#define BOBGUI_IS_GEARS(inst)  (G_TYPE_CHECK_INSTANCE_TYPE ((inst), \
                             BOBGUI_TYPE_GEARS))

typedef struct _BobguiGears BobguiGears;
typedef struct _BobguiGearsClass BobguiGearsClass;

struct _BobguiGears {
  BobguiGLArea parent;
};

struct _BobguiGearsClass {
  BobguiGLAreaClass parent_class;
};

GType      bobgui_gears_get_type      (void) G_GNUC_CONST;

BobguiWidget *bobgui_gears_new           (void);
void       bobgui_gears_set_axis      (BobguiGears *gears,
                                    int       axis,
                                    double    value);
double     bobgui_gears_get_axis      (BobguiGears *gears,
                                    int       axis);
void       bobgui_gears_set_fps_label (BobguiGears *gears,
                                    BobguiLabel *label);


G_END_DECLS
