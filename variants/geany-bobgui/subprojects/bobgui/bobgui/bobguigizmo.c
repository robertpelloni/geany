
#include "bobguigizmoprivate.h"
#include "bobguiwidgetprivate.h"


G_DEFINE_TYPE (BobguiGizmo, bobgui_gizmo, BOBGUI_TYPE_WIDGET);

static void
bobgui_gizmo_measure (BobguiWidget      *widget,
                   BobguiOrientation  orientation,
                   int             for_size,
                   int            *minimum,
                   int            *natural,
                   int            *minimum_baseline,
                   int            *natural_baseline)
{
  BobguiGizmo *self = BOBGUI_GIZMO (widget);

  if (self->measure_func)
    self->measure_func (self, orientation, for_size,
                        minimum, natural,
                        minimum_baseline, natural_baseline);
}

static void
bobgui_gizmo_size_allocate (BobguiWidget *widget,
                         int        width,
                         int        height,
                         int        baseline)
{
  BobguiGizmo *self = BOBGUI_GIZMO (widget);

  if (self->allocate_func)
    self->allocate_func (self, width, height, baseline);
}

static void
bobgui_gizmo_snapshot (BobguiWidget   *widget,
                    BobguiSnapshot *snapshot)
{
  BobguiGizmo *self = BOBGUI_GIZMO (widget);

  if (self->snapshot_func)
    self->snapshot_func (self, snapshot);
  else
    BOBGUI_WIDGET_CLASS (bobgui_gizmo_parent_class)->snapshot (widget, snapshot);
}

static gboolean
bobgui_gizmo_contains (BobguiWidget *widget,
                    double     x,
                    double     y)
{
  BobguiGizmo *self = BOBGUI_GIZMO (widget);

  if (self->contains_func)
    return self->contains_func (self, x, y);
  else
    return BOBGUI_WIDGET_CLASS (bobgui_gizmo_parent_class)->contains (widget, x, y);
}

static gboolean
bobgui_gizmo_focus (BobguiWidget        *widget,
                 BobguiDirectionType  direction)
{
  BobguiGizmo *self = BOBGUI_GIZMO (widget);

  if (self->focus_func)
    return self->focus_func (self, direction);

  return FALSE;
}

static gboolean
bobgui_gizmo_grab_focus (BobguiWidget *widget)
{
  BobguiGizmo *self = BOBGUI_GIZMO (widget);

  if (self->grab_focus_func)
    return self->grab_focus_func (self);

  return FALSE;
}

static void
bobgui_gizmo_finalize (GObject *object)
{
  BobguiGizmo *self = BOBGUI_GIZMO (object);
  BobguiWidget *widget;

  widget = _bobgui_widget_get_first_child (BOBGUI_WIDGET (self));
  while (widget != NULL)
    {
      BobguiWidget *next = _bobgui_widget_get_next_sibling (widget);

      bobgui_widget_unparent (widget);

      widget = next;
    }

  G_OBJECT_CLASS (bobgui_gizmo_parent_class)->finalize (object);
}

static void
bobgui_gizmo_class_init (BobguiGizmoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->finalize = bobgui_gizmo_finalize;

  widget_class->measure = bobgui_gizmo_measure;
  widget_class->size_allocate = bobgui_gizmo_size_allocate;
  widget_class->snapshot = bobgui_gizmo_snapshot;
  widget_class->contains = bobgui_gizmo_contains;
  widget_class->grab_focus = bobgui_gizmo_grab_focus;
  widget_class->focus = bobgui_gizmo_focus;
}

static void
bobgui_gizmo_init (BobguiGizmo *self)
{
}

BobguiWidget *
bobgui_gizmo_new (const char            *css_name,
               BobguiGizmoMeasureFunc    measure_func,
               BobguiGizmoAllocateFunc   allocate_func,
               BobguiGizmoSnapshotFunc   snapshot_func,
               BobguiGizmoContainsFunc   contains_func,
               BobguiGizmoFocusFunc      focus_func,
               BobguiGizmoGrabFocusFunc  grab_focus_func)
{
  return bobgui_gizmo_new_with_role (css_name,
                                  BOBGUI_ACCESSIBLE_ROLE_GENERIC,
                                  measure_func,
                                  allocate_func,
                                  snapshot_func,
                                  contains_func,
                                  focus_func,
                                  grab_focus_func);
}

BobguiWidget *
bobgui_gizmo_new_with_role (const char            *css_name,
                         BobguiAccessibleRole      role,
                         BobguiGizmoMeasureFunc    measure_func,
                         BobguiGizmoAllocateFunc   allocate_func,
                         BobguiGizmoSnapshotFunc   snapshot_func,
                         BobguiGizmoContainsFunc   contains_func,
                         BobguiGizmoFocusFunc      focus_func,
                         BobguiGizmoGrabFocusFunc  grab_focus_func)
{
  BobguiGizmo *gizmo = BOBGUI_GIZMO (g_object_new (BOBGUI_TYPE_GIZMO,
                                             "css-name", css_name,
                                             "accessible-role", role,
                                             NULL));

  gizmo->measure_func  = measure_func;
  gizmo->allocate_func = allocate_func;
  gizmo->snapshot_func = snapshot_func;
  gizmo->contains_func = contains_func;
  gizmo->focus_func = focus_func;
  gizmo->grab_focus_func = grab_focus_func;

  return BOBGUI_WIDGET (gizmo);
}
