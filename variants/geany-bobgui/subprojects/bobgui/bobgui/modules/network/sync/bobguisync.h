/* bobgui/modules/sync/bobguisync.h */
#ifndef BOBGUI_SYNC_H
#define BOBGUI_SYNC_H

#include <bobgui/bobgui.h>
#include <bobgui/modules/core/state/bobguistate.h>

G_BEGIN_DECLS

/* CRDT-based Synchronization Engine (Better than standard RPC) */
#define BOBGUI_TYPE_SYNC_ENGINE (bobgui_sync_engine_get_type ())
G_DECLARE_FINAL_TYPE (BobguiSyncEngine, bobgui_sync_engine, BOBGUI, SYNC_ENGINE, GObject)

BobguiSyncEngine * bobgui_sync_engine_new (BobguiStateMachine *state_machine);

/* Distributed Replication (Conflict-free UI state across instances) */
void bobgui_sync_engine_connect (BobguiSyncEngine *self, const char *peer_address);

/* Change Sets and Deltas (More efficient than full-state transfers) */
void bobgui_sync_engine_broadcast (BobguiSyncEngine *self, GBytes *delta);

/* Real-time Presence (Collaborative UI like Figma) */
void bobgui_sync_engine_track_cursor (BobguiSyncEngine *self, 
                                     BobguiWidget *cursor_widget, 
                                     int user_id);

G_END_DECLS

#endif /* BOBGUI_SYNC_H */
