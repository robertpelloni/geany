#pragma once

#include <bobgui/bobgui.h>

#define IMAGE_TYPE_VIEW (image_view_get_type ())
G_DECLARE_FINAL_TYPE (ImageView, image_view, IMAGE, VIEW, BobguiWidget)

BobguiWidget * image_view_new (const char *resource);
