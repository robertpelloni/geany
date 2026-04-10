/**
 * BobguiCustomLayout:
 *
 * Uses closures for size negotiation.
 *
 * A `BobguiCustomLayout` uses closures matching to the old `BobguiWidget`
 * virtual functions for size negotiation, as a convenience API to
 * ease the porting towards the corresponding `BobguiLayoutManager`
 * virtual functions.
 */

#include "config.h"

#include "bobguicustomlayout.h"

struct _BobguiCustomLayout
{
  BobguiLayoutManager parent_instance;

  BobguiCustomRequestModeFunc request_mode_func;
  BobguiCustomMeasureFunc measure_func;
  BobguiCustomAllocateFunc allocate_func;
};

G_DEFINE_TYPE (BobguiCustomLayout, bobgui_custom_layout, BOBGUI_TYPE_LAYOUT_MANAGER)

static BobguiSizeRequestMode
bobgui_custom_layout_get_request_mode (BobguiLayoutManager *manager,
                                    BobguiWidget        *widget)
{
  BobguiCustomLayout *self = BOBGUI_CUSTOM_LAYOUT (manager);

  if (self->request_mode_func != NULL)
    return self->request_mode_func (widget);

  return BOBGUI_LAYOUT_MANAGER_CLASS (bobgui_custom_layout_parent_class)->get_request_mode (manager, widget);
}

static void
bobgui_custom_layout_measure (BobguiLayoutManager *manager,
                           BobguiWidget        *widget,
                           BobguiOrientation    orientation,
                           int               for_size,
                           int              *minimum,
                           int              *natural,
                           int              *minimum_baseline,
                           int              *natural_baseline)
{
  BobguiCustomLayout *self = BOBGUI_CUSTOM_LAYOUT (manager);
  int min = 0, nat = 0;
  int min_baseline = -1, nat_baseline = -1;

  self->measure_func (widget, orientation, for_size,
                      &min, &nat,
                      &min_baseline, &nat_baseline);

  if (minimum != NULL)
    *minimum = min;
  if (natural != NULL)
    *natural = nat;

  if (minimum_baseline != NULL)
    *minimum_baseline = min_baseline;
  if (natural_baseline != NULL)
    *natural_baseline = nat_baseline;
}

static void
bobgui_custom_layout_allocate (BobguiLayoutManager *manager,
                            BobguiWidget        *widget,
                            int               width,
                            int               height,
                            int               baseline)
{
  BobguiCustomLayout *self = BOBGUI_CUSTOM_LAYOUT (manager);

  self->allocate_func (widget, width, height, baseline);
}

static void
bobgui_custom_layout_class_init (BobguiCustomLayoutClass *klass)
{
  BobguiLayoutManagerClass *layout_class = BOBGUI_LAYOUT_MANAGER_CLASS (klass);

  layout_class->get_request_mode = bobgui_custom_layout_get_request_mode;
  layout_class->measure = bobgui_custom_layout_measure;
  layout_class->allocate = bobgui_custom_layout_allocate;
}

static void
bobgui_custom_layout_init (BobguiCustomLayout *self)
{
}

/**
 * bobgui_custom_layout_new:
 * @request_mode: (nullable) (scope call): a function to retrieve
 *   the `BobguiSizeRequestMode` of the widget using the layout; the
 *   default request mode is %BOBGUI_SIZE_REQUEST_CONSTANT_SIZE
 * @measure: (not nullable) (scope call): a function to measure the widget using the layout manager
 * @allocate: (not nullable) (scope call): a function to allocate the children of the widget using
 *   the layout manager
 *
 * Creates a new legacy layout manager.
 *
 * Legacy layout managers map to the old `BobguiWidget` size negotiation
 * virtual functions, and are meant to be used during the transition
 * from layout containers to layout manager delegates.
 *
 * Returns: (transfer full): the newly created `BobguiCustomLayout`
 */
BobguiLayoutManager *
bobgui_custom_layout_new (BobguiCustomRequestModeFunc request_mode,
                       BobguiCustomMeasureFunc measure,
                       BobguiCustomAllocateFunc allocate)
{
  BobguiCustomLayout *self = g_object_new (BOBGUI_TYPE_CUSTOM_LAYOUT, NULL);

  g_return_val_if_fail (measure != NULL, NULL);
  g_return_val_if_fail (allocate != NULL, NULL);

  self->request_mode_func = request_mode;
  self->measure_func = measure;
  self->allocate_func = allocate;

  return BOBGUI_LAYOUT_MANAGER (self);
}
