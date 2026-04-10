#pragma once

#include <bobgui/bobgui.h>

G_DECLARE_FINAL_TYPE (PathExplorer, path_explorer, PATH, EXPLORER, BobguiWidget)

BobguiWidget * path_explorer_new (void);
