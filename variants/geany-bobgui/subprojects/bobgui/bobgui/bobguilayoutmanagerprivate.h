#pragma once

#include "bobguilayoutmanager.h"

G_BEGIN_DECLS

void bobgui_layout_manager_set_widget (BobguiLayoutManager *manager,
                                    BobguiWidget        *widget);

void bobgui_layout_manager_remove_layout_child (BobguiLayoutManager *manager,
                                             BobguiWidget        *widget);

void bobgui_layout_manager_set_root (BobguiLayoutManager *manager,
                                  BobguiRoot          *root);

G_END_DECLS
