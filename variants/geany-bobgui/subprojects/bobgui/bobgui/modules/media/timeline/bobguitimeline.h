#ifndef BOBGUI_TIMELINE_H
#define BOBGUI_TIMELINE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_TIMELINE (bobgui_timeline_get_type ())
G_DECLARE_FINAL_TYPE (BobguiTimeline, bobgui_timeline, BOBGUI, TIMELINE, GObject)

BobguiTimeline * bobgui_timeline_new (void);

G_END_DECLS

#endif
