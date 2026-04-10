#include "bobguidata.h"
BobguiSqlDatabase * bobgui_sql_database_add (const char *d) { return g_new0 (BobguiSqlDatabase, 1); }
BobguiProperty * bobgui_property_new_float (float i) { return g_object_new (G_TYPE_OBJECT, NULL); }
