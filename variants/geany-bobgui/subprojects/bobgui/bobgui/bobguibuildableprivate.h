#pragma once

#include "bobguibuildable.h"

G_BEGIN_DECLS

void      bobgui_buildable_set_buildable_id       (BobguiBuildable        *buildable,
                                                const char          *id);
void      bobgui_buildable_add_child              (BobguiBuildable        *buildable,
                                                BobguiBuilder          *builder,
                                                GObject             *child,
                                                const char          *type);
GObject * bobgui_buildable_construct_child        (BobguiBuildable        *buildable,
                                                BobguiBuilder          *builder,
                                                const char          *name);
gboolean  bobgui_buildable_custom_tag_start       (BobguiBuildable        *buildable,
                                                BobguiBuilder          *builder,
                                                GObject             *child,
                                                const char          *tagname,
                                                BobguiBuildableParser  *parser,
                                                gpointer            *data);
void      bobgui_buildable_custom_tag_end         (BobguiBuildable        *buildable,
                                                BobguiBuilder          *builder,
                                                GObject             *child,
                                                const char          *tagname,
                                                gpointer             data);
void      bobgui_buildable_custom_finished        (BobguiBuildable        *buildable,
                                                BobguiBuilder          *builder,
                                                GObject             *child,
                                                const char          *tagname,
                                                gpointer             data);
void      bobgui_buildable_parser_finished        (BobguiBuildable        *buildable,
                                                BobguiBuilder          *builder);
GObject * bobgui_buildable_get_internal_child     (BobguiBuildable        *buildable,
                                                BobguiBuilder          *builder,
                                                const char          *childname);

G_END_DECLS

