#ifndef BOBGUI_LIVE_H
#define BOBGUI_LIVE_H

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

typedef struct _BobguiLiveContext BobguiLiveContext;

BobguiLiveContext * bobgui_live_context_new (const char *module_path);

G_END_DECLS

#endif
