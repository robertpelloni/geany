/* bobguibuilderprivate.h
 * Copyright (C) 2006-2007 Async Open Source,
 *                         Johan Dahlin <jdahlin@async.com.br>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "bobguibuilder.h"
#include "bobguibuildable.h"
#include "bobguiexpression.h"

enum {
  TAG_PROPERTY,
  TAG_BINDING,
  TAG_BINDING_EXPRESSION,
  TAG_REQUIRES,
  TAG_OBJECT,
  TAG_CHILD,
  TAG_SIGNAL,
  TAG_INTERFACE,
  TAG_TEMPLATE,
  TAG_EXPRESSION,
};

typedef struct {
  guint tag_type;
} CommonInfo;

typedef struct {
  guint tag_type;
  GType type;
  GObjectClass *oclass;
  char *id;
  char *constructor;

  GPtrArray *properties;
  GPtrArray *signals;
  GSList *bindings;

  GObject *object;
  CommonInfo *parent;
} ObjectInfo;

typedef struct {
  guint tag_type;
  GSList *packing_properties;
  GObject *object;
  CommonInfo *parent;
  char *type;
  char *internal_child;
  gboolean added;
} ChildInfo;

typedef struct {
  guint tag_type;
  GParamSpec *pspec;
  gpointer value;
  GString *text;
  unsigned int translatable : 1;
  unsigned int bound        : 1;
  unsigned int applied      : 1;
  char *context;
  int line;
  int col;
} PropertyInfo;

typedef struct _ExpressionInfo ExpressionInfo;
struct _ExpressionInfo {
  guint tag_type;
  enum {
    EXPRESSION_EXPRESSION,
    EXPRESSION_CONSTANT,
    EXPRESSION_CLOSURE,
    EXPRESSION_PROPERTY,
    EXPRESSION_TRY
  } expression_type;
  union {
    BobguiExpression *expression;
    struct {
      GType type;
      gboolean initial;
      GString *text;
      gboolean translatable;
      char *context;
    } constant;
    struct {
      GType type;
      char *function_name;
      char *object_name;
      gboolean swapped;
      GSList *params;
    } closure;
    struct {
      GType this_type;
      char *property_name;
      ExpressionInfo *expression;
    } property;
    struct {
      GSList *expressions;
    } try;
  };
};

typedef struct {
  guint tag_type;
  char *object_name;
  guint  id;
  GQuark detail;
  char *handler;
  GConnectFlags flags;
  char *connect_object_name;
} SignalInfo;

typedef struct
{
  guint tag_type;
  GObject *target;
  GParamSpec *target_pspec;
  char *source;
  char *source_property;
  GBindingFlags flags;
  int line;
  int col;
} BindingInfo;

typedef struct
{
  guint tag_type;
  GObject *target;
  GParamSpec *target_pspec;
  char *object_name;
  ExpressionInfo *expr;
  int line;
  int col;
} BindingExpressionInfo;

typedef struct {
  guint    tag_type;
  char    *library;
  int      major;
  int      minor;
} RequiresInfo;

struct _BobguiBuildableParseContext {
  const GMarkupParser *internal_callbacks;
  GMarkupParseContext *ctx;

  const BobguiBuildableParser *parser;
  gpointer user_data;

  GPtrArray *tag_stack;

  GArray *subparser_stack;
  gpointer held_user_data;
  gboolean awaiting_pop;
};

typedef struct {
  BobguiBuildableParser *parser;
  char *tagname;
  int level;
  const char *start;
  gpointer data;
  GObject *object;
  GObject *child;
} SubParser;

typedef struct {
  const char *last_element;
  BobguiBuilder *builder;
  char *domain;
  GPtrArray *stack;
  SubParser *subparser;
  BobguiBuildableParseContext ctx;
  const char *filename;
  GPtrArray *finalizers;
  GSList *custom_finalizers;

  const char **requested_objects; /* NULL if all the objects are requested */
  gboolean inside_requested_object;
  int requested_object_level;
  int cur_object_level;

  int object_counter;

  GHashTable *object_ids;
} ParserData;

/* Things only BobguiBuilder should use */
GBytes * _bobgui_buildable_parser_precompile (const char               *text,
                                           gssize                    text_len,
                                           GError                  **error);
gboolean _bobgui_buildable_parser_is_precompiled (const char           *data,
                                               gssize                data_len);
gboolean _bobgui_buildable_parser_replay_precompiled (BobguiBuildableParseContext *context,
                                                   const char           *data,
                                                   gssize                data_len,
                                                   GError              **error);
void _bobgui_builder_parser_parse_buffer (BobguiBuilder *builder,
                                       const char *filename,
                                       const char *buffer,
                                       gssize length,
                                       const char **requested_objs,
                                       GError **error);
GObject * _bobgui_builder_construct (BobguiBuilder *builder,
                                  ObjectInfo *info,
				  GError    **error);
void      _bobgui_builder_apply_properties (BobguiBuilder *builder,
					 ObjectInfo *info,
					 GError **error);
void      _bobgui_builder_add_object (BobguiBuilder  *builder,
                                   const char *id,
                                   GObject     *object);
void      _bobgui_builder_add (BobguiBuilder *builder,
                            ChildInfo *child_info);
void      _bobgui_builder_add_signals (BobguiBuilder *builder,
                                    GPtrArray  *signals);
void       bobgui_builder_take_bindings (BobguiBuilder *builder,
                                      GObject    *target,
                                      GSList     *bindings);

gboolean  _bobgui_builder_finish (BobguiBuilder  *builder,
                               GError     **error);
void _free_signal_info (SignalInfo *info,
                        gpointer user_data);
void _free_binding_info (BindingInfo *info,
                         gpointer user_data);
void free_binding_expression_info (BindingExpressionInfo *info);
BobguiExpression * expression_info_construct (BobguiBuilder      *builder,
                                           const char      *domain,
                                           ExpressionInfo  *info,
                                           GError         **error);

/* Internal API which might be made public at some point */
gboolean _bobgui_builder_enum_from_string (GType         type,
                                        const char   *string,
                                        int          *enum_value,
                                        GError      **error);
gboolean  _bobgui_builder_flags_from_string (GType         type,
                                          const char   *string,
                                          guint        *value,
                                          GError      **error);
gboolean _bobgui_builder_boolean_from_string (const char   *string,
                                           gboolean     *value,
                                           GError      **error);

gboolean bobgui_builder_parse_translatable (const char  *string,
                                         gboolean    *value,
                                         GError     **error);

const char * _bobgui_builder_parser_translate (const char *domain,
                                             const char *context,
                                             const char *text);
char *   _bobgui_builder_get_resource_path (BobguiBuilder *builder,
					  const char *string) G_GNUC_MALLOC;
char *   _bobgui_builder_get_absolute_filename (BobguiBuilder *builder,
					      const char *string) G_GNUC_MALLOC;

void      _bobgui_builder_menu_start (ParserData   *parser_data,
                                   const char   *element_name,
                                   const char **attribute_names,
                                   const char **attribute_values,
                                   GError      **error);
char *    _bobgui_builder_menu_end   (ParserData  *parser_data);

GType     bobgui_builder_get_template_type (BobguiBuilder *builder,
                                         gboolean *out_allow_parents);
void      bobgui_builder_set_allow_template_parents (BobguiBuilder *builder,
                                                  gboolean    allow_parents);

void     _bobgui_builder_prefix_error        (BobguiBuilder                *builder,
                                           BobguiBuildableParseContext  *context,
                                           GError                   **error);
void     _bobgui_builder_error_unhandled_tag (BobguiBuilder                *builder,
                                           BobguiBuildableParseContext  *context,
                                           const char                *object,
                                           const char                *element_name,
                                           GError                   **error);
gboolean _bobgui_builder_check_parent        (BobguiBuilder                *builder,
                                           BobguiBuildableParseContext  *context,
                                           const char                *parent_name,
                                           GError                   **error);
gboolean _bobgui_builder_check_parents       (BobguiBuilder                *builder,
                                           BobguiBuildableParseContext  *context,
                                           GError                   **error,
                                           ...);
GObject *bobgui_builder_lookup_object        (BobguiBuilder                *builder,
                                           const char                *name,
                                           int                        line,
                                           int                        col,
                                           GError                   **error);
GObject *_bobgui_builder_lookup_object       (BobguiBuilder                *builder,
                                           const char                *name,
                                           int                        line,
                                           int                        col);
gboolean _bobgui_builder_lookup_failed       (BobguiBuilder                *builder,
                                           GError                   **error);

void     bobgui_buildable_child_deprecation_warning (BobguiBuildable *buildable,
                                                  BobguiBuilder   *builder,
                                                  const char   *type,
                                                  const char   *prop);
void     bobgui_buildable_tag_deprecation_warning   (BobguiBuildable *buildable,
                                                  BobguiBuilder   *builder,
                                                  const char   *tag,
                                                  const char   *prop);
