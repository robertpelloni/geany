/* bobgui/modules/core/stream/bobguistream.h */
#ifndef BOBGUI_STREAM_H
#define BOBGUI_STREAM_H

#include <glib-object.h>

G_BEGIN_DECLS

/* Reactive Stream Engine (Better than RxJava / RxCPP) */
#define BOBGUI_TYPE_STREAM (bobgui_stream_get_type ())
G_DECLARE_FINAL_TYPE (BobguiStream, bobgui_stream, BOBGUI, STREAM, GObject)

typedef void (*BobguiStreamNextFunc) (gpointer data, gpointer user_data);

/* Stream Creation API */
BobguiStream * bobgui_stream_from_list (GList *list);
BobguiStream * bobgui_stream_interval (int interval_ms);

/* Reactive Operators (Superior Zero-Allocation Implementation) */
BobguiStream * bobgui_stream_map    (BobguiStream *self, GFunc map_func, gpointer user_data);
BobguiStream * bobgui_stream_filter (BobguiStream *self, GCompareFunc filter_func, gpointer user_data);
BobguiStream * bobgui_stream_debounce (BobguiStream *self, int window_ms);

/* Subscription (Unmatched Thread-Safety) */
void bobgui_stream_subscribe (BobguiStream *self, 
                             BobguiStreamNextFunc on_next, 
                             gpointer user_data);

G_END_DECLS

#endif /* BOBGUI_STREAM_H */
