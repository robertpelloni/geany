#pragma once

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

#define DEMO_TYPE_IMAGE (demo_image_get_type ())

G_DECLARE_FINAL_TYPE(DemoImage, demo_image, DEMO, IMAGE, BobguiWidget)

BobguiWidget * demo_image_new (const char *icon_name);

G_END_DECLS
