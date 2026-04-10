

#pragma once

#include "inspectoroverlay.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_SUBSURFACE_OVERLAY             (bobgui_subsurface_overlay_get_type ())
G_DECLARE_FINAL_TYPE (BobguiSubsurfaceOverlay, bobgui_subsurface_overlay, BOBGUI, SUBSURFACE_OVERLAY, BobguiInspectorOverlay)

BobguiInspectorOverlay *   bobgui_subsurface_overlay_new                 (void);

G_END_DECLS



