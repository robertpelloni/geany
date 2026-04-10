#pragma once

#include <bobgui/bobgui.h>

void            bobgui_nuclear_snapshot           (BobguiSnapshot     *snapshot,
                                                const GdkRGBA   *foreground,
                                                const GdkRGBA   *background,
                                                double           width,
                                                double           height,
                                                double           rotation);

GdkPaintable *  bobgui_nuclear_icon_new            (double          rotation);
GdkPaintable *  bobgui_nuclear_animation_new       (gboolean        draw_background);
BobguiMediaStream *bobgui_nuclear_media_stream_new    (void);
