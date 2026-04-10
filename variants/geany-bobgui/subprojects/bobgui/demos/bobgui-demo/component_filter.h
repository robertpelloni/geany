#pragma once

#include <bobgui/bobgui.h>

G_DECLARE_FINAL_TYPE (ComponentFilter, component_filter, COMPONENT, FILTER, BobguiWidget)

BobguiWidget *            component_filter_new                    (void);

GskComponentTransfer * component_filter_get_component_transfer (ComponentFilter *self);
