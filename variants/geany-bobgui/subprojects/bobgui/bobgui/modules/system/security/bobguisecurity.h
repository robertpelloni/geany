#ifndef BOBGUI_SECURITY_H
#define BOBGUI_SECURITY_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _BobguiSecureBuffer BobguiSecureBuffer;

#define BOBGUI_TYPE_SECURE_ENTRY (bobgui_secure_entry_get_type ())
G_DECLARE_FINAL_TYPE (BobguiSecureEntry, bobgui_secure_entry, BOBGUI, SECURE_ENTRY, GObject)

BobguiSecureEntry * bobgui_secure_entry_new (void);

G_END_DECLS

#endif
