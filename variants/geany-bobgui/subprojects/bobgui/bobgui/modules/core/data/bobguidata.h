#ifndef BOBGUI_DATA_H
#define BOBGUI_DATA_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _BobguiSqlDatabase BobguiSqlDatabase;
typedef struct _BobguiProperty BobguiProperty;

BobguiSqlDatabase * bobgui_sql_database_add (const char *driver);
BobguiProperty * bobgui_property_new_float (float init_value);

G_END_DECLS

#endif
