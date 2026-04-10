#pragma once

#include "bobguiroot.h"

#include "bobguiconstraintsolverprivate.h"

G_BEGIN_DECLS

/**
 * BobguiRootIface:
 *
 * The list of functions that must be implemented for the `BobguiRoot` interface.
 */
struct _BobguiRootInterface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/
  GdkDisplay * (* get_display)  (BobguiRoot *self);

  BobguiConstraintSolver * (* get_constraint_solver) (BobguiRoot *self);

  BobguiWidget *  (* get_focus)    (BobguiRoot   *self);
  void         (* set_focus)    (BobguiRoot   *self,
                                 BobguiWidget *focus);

};

BobguiConstraintSolver *   bobgui_root_get_constraint_solver  (BobguiRoot *self);

void             bobgui_root_start_layout  (BobguiRoot *self);
void             bobgui_root_stop_layout   (BobguiRoot *self);
void             bobgui_root_queue_restyle (BobguiRoot *self);

G_END_DECLS

