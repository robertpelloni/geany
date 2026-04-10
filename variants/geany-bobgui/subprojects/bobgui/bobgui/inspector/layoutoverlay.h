

#pragma once

#include "inspectoroverlay.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_LAYOUT_OVERLAY             (bobgui_layout_overlay_get_type ())
G_DECLARE_FINAL_TYPE (BobguiLayoutOverlay, bobgui_layout_overlay, BOBGUI, LAYOUT_OVERLAY, BobguiInspectorOverlay)

BobguiInspectorOverlay *   bobgui_layout_overlay_new                 (void);

G_END_DECLS



