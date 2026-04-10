#pragma once

#include <bobgui/bobgui.h>

#define PATH_TYPE_VIEW (path_view_get_type ())

G_DECLARE_FINAL_TYPE (PathView, path_view, PATH, VIEW, BobguiWidget)

BobguiWidget * path_view_new (GskPath *path1,
                           GskPath *path2);
