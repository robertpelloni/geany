#pragma once

#include "inspectoroverlay.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_BASELINE_OVERLAY             (bobgui_baseline_overlay_get_type ())
G_DECLARE_FINAL_TYPE (BobguiBaselineOverlay, bobgui_baseline_overlay, BOBGUI, BASELINE_OVERLAY, BobguiInspectorOverlay)

BobguiInspectorOverlay *   bobgui_baseline_overlay_new                 (void);

G_END_DECLS



