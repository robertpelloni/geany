
#pragma once

#include "bobguiwidget.h"
#include "bobguienums.h"

#define BOBGUI_TYPE_GIZMO                 (bobgui_gizmo_get_type ())
#define BOBGUI_GIZMO(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_GIZMO, BobguiGizmo))
#define BOBGUI_GIZMO_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_GIZMO, BobguiGizmoClass))
#define BOBGUI_IS_GIZMO(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_GIZMO))
#define BOBGUI_IS_GIZMO_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_GIZMO))
#define BOBGUI_GIZMO_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_GIZMO, BobguiGizmoClass))

typedef struct _BobguiGizmo             BobguiGizmo;
typedef struct _BobguiGizmoClass        BobguiGizmoClass;

typedef void    (* BobguiGizmoMeasureFunc)   (BobguiGizmo       *gizmo,
                                           BobguiOrientation  orientation,
                                           int             for_size,
                                           int            *minimum,
                                           int            *natural,
                                           int            *minimum_baseline,
                                           int            *natural_baseline);
typedef void    (* BobguiGizmoAllocateFunc)  (BobguiGizmo *gizmo,
                                           int       width,
                                           int       height,
                                           int       baseline);
typedef void    (* BobguiGizmoSnapshotFunc)  (BobguiGizmo    *gizmo,
                                           BobguiSnapshot *snapshot);
typedef gboolean (* BobguiGizmoContainsFunc) (BobguiGizmo  *gizmo,
                                           double     x,
                                           double     y);
typedef gboolean (* BobguiGizmoFocusFunc)    (BobguiGizmo         *gizmo,
                                           BobguiDirectionType  direction);
typedef gboolean (* BobguiGizmoGrabFocusFunc)(BobguiGizmo         *gizmo);

struct _BobguiGizmo
{
  BobguiWidget parent_instance;

  BobguiGizmoMeasureFunc   measure_func;
  BobguiGizmoAllocateFunc  allocate_func;
  BobguiGizmoSnapshotFunc  snapshot_func;
  BobguiGizmoContainsFunc  contains_func;
  BobguiGizmoFocusFunc     focus_func;
  BobguiGizmoGrabFocusFunc grab_focus_func;
};

struct _BobguiGizmoClass
{
  BobguiWidgetClass parent_class;
};

GType      bobgui_gizmo_get_type (void) G_GNUC_CONST;

BobguiWidget *bobgui_gizmo_new (const char            *css_name,
                          BobguiGizmoMeasureFunc    measure_func,
                          BobguiGizmoAllocateFunc   allocate_func,
                          BobguiGizmoSnapshotFunc   snapshot_func,
                          BobguiGizmoContainsFunc   contains_func,
                          BobguiGizmoFocusFunc      focus_func,
                          BobguiGizmoGrabFocusFunc  grab_focus_func);

BobguiWidget *bobgui_gizmo_new_with_role (const char            *css_name,
                                    BobguiAccessibleRole      role,
                                    BobguiGizmoMeasureFunc    measure_func,
                                    BobguiGizmoAllocateFunc   allocate_func,
                                    BobguiGizmoSnapshotFunc   snapshot_func,
                                    BobguiGizmoContainsFunc   contains_func,
                                    BobguiGizmoFocusFunc      focus_func,
                                    BobguiGizmoGrabFocusFunc  grab_focus_func);


