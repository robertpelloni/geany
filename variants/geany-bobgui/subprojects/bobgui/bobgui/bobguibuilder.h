/* BOBGUI - The Bobgui Framework
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

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguibuilderscope.h>
#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_BUILDER                 (bobgui_builder_get_type ())
#define BOBGUI_BUILDER(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_BUILDER, BobguiBuilder))
#define BOBGUI_BUILDER_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_BUILDER, BobguiBuilderClass))
#define BOBGUI_IS_BUILDER(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_BUILDER))
#define BOBGUI_IS_BUILDER_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_BUILDER))
#define BOBGUI_BUILDER_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_BUILDER, BobguiBuilderClass))

#define BOBGUI_BUILDER_ERROR                (bobgui_builder_error_quark ())

typedef struct _BobguiBuilderClass   BobguiBuilderClass;

/**
 * BobguiBuilderError:
 * @BOBGUI_BUILDER_ERROR_INVALID_TYPE_FUNCTION: A type-func attribute didn’t name
 *  a function that returns a `GType`.
 * @BOBGUI_BUILDER_ERROR_UNHANDLED_TAG: The input contained a tag that `BobguiBuilder`
 *  can’t handle.
 * @BOBGUI_BUILDER_ERROR_MISSING_ATTRIBUTE: An attribute that is required by
 *  `BobguiBuilder` was missing.
 * @BOBGUI_BUILDER_ERROR_INVALID_ATTRIBUTE: `BobguiBuilder` found an attribute that
 *  it doesn’t understand.
 * @BOBGUI_BUILDER_ERROR_INVALID_TAG: `BobguiBuilder` found a tag that
 *  it doesn’t understand.
 * @BOBGUI_BUILDER_ERROR_MISSING_PROPERTY_VALUE: A required property value was
 *  missing.
 * @BOBGUI_BUILDER_ERROR_INVALID_VALUE: `BobguiBuilder` couldn’t parse
 *  some attribute value.
 * @BOBGUI_BUILDER_ERROR_VERSION_MISMATCH: The input file requires a newer version
 *  of BOBGUI.
 * @BOBGUI_BUILDER_ERROR_DUPLICATE_ID: An object id occurred twice.
 * @BOBGUI_BUILDER_ERROR_OBJECT_TYPE_REFUSED: A specified object type is of the same type or
 *  derived from the type of the composite class being extended with builder XML.
 * @BOBGUI_BUILDER_ERROR_TEMPLATE_MISMATCH: The wrong type was specified in a composite class’s template XML
 * @BOBGUI_BUILDER_ERROR_INVALID_PROPERTY: The specified property is unknown for the object class.
 * @BOBGUI_BUILDER_ERROR_INVALID_SIGNAL: The specified signal is unknown for the object class.
 * @BOBGUI_BUILDER_ERROR_INVALID_ID: An object id is unknown.
 * @BOBGUI_BUILDER_ERROR_INVALID_FUNCTION: A function could not be found. This often happens
 *   when symbols are set to be kept private. Compiling code with -rdynamic or using the
 *   `gmodule-export-2.0` pkgconfig module can fix this problem.
 *
 * Error codes that identify various errors that can occur while using
 * `BobguiBuilder`.
 */
typedef enum
{
  BOBGUI_BUILDER_ERROR_INVALID_TYPE_FUNCTION,
  BOBGUI_BUILDER_ERROR_UNHANDLED_TAG,
  BOBGUI_BUILDER_ERROR_MISSING_ATTRIBUTE,
  BOBGUI_BUILDER_ERROR_INVALID_ATTRIBUTE,
  BOBGUI_BUILDER_ERROR_INVALID_TAG,
  BOBGUI_BUILDER_ERROR_MISSING_PROPERTY_VALUE,
  BOBGUI_BUILDER_ERROR_INVALID_VALUE,
  BOBGUI_BUILDER_ERROR_VERSION_MISMATCH,
  BOBGUI_BUILDER_ERROR_DUPLICATE_ID,
  BOBGUI_BUILDER_ERROR_OBJECT_TYPE_REFUSED,
  BOBGUI_BUILDER_ERROR_TEMPLATE_MISMATCH,
  BOBGUI_BUILDER_ERROR_INVALID_PROPERTY,
  BOBGUI_BUILDER_ERROR_INVALID_SIGNAL,
  BOBGUI_BUILDER_ERROR_INVALID_ID,
  BOBGUI_BUILDER_ERROR_INVALID_FUNCTION
} BobguiBuilderError;

GDK_AVAILABLE_IN_ALL
GQuark bobgui_builder_error_quark (void);

GDK_AVAILABLE_IN_ALL
GType        bobgui_builder_get_type                (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiBuilder*  bobgui_builder_new                     (void);

GDK_AVAILABLE_IN_ALL
gboolean     bobgui_builder_add_from_file           (BobguiBuilder    *builder,
                                                  const char    *filename,
                                                  GError       **error);
GDK_AVAILABLE_IN_ALL
gboolean     bobgui_builder_add_from_resource       (BobguiBuilder    *builder,
                                                  const char    *resource_path,
                                                  GError       **error);
GDK_AVAILABLE_IN_ALL
gboolean     bobgui_builder_add_from_string         (BobguiBuilder    *builder,
                                                  const char    *buffer,
                                                  gssize         length,
                                                  GError       **error);
GDK_AVAILABLE_IN_ALL
gboolean     bobgui_builder_add_objects_from_file   (BobguiBuilder    *builder,
                                                  const char    *filename,
                                                  const char   **object_ids,
                                                  GError       **error);
GDK_AVAILABLE_IN_ALL
gboolean     bobgui_builder_add_objects_from_resource(BobguiBuilder    *builder,
                                                  const char    *resource_path,
                                                  const char   **object_ids,
                                                  GError       **error);
GDK_AVAILABLE_IN_ALL
gboolean     bobgui_builder_add_objects_from_string (BobguiBuilder    *builder,
                                                  const char    *buffer,
                                                  gssize         length,
                                                  const char   **object_ids,
                                                  GError       **error);
GDK_AVAILABLE_IN_ALL
GObject*     bobgui_builder_get_object              (BobguiBuilder    *builder,
                                                  const char    *name);
GDK_AVAILABLE_IN_ALL
GSList*      bobgui_builder_get_objects             (BobguiBuilder    *builder);
GDK_AVAILABLE_IN_ALL
void         bobgui_builder_expose_object           (BobguiBuilder    *builder,
                                                  const char    *name,
                                                  GObject       *object);
GDK_AVAILABLE_IN_ALL
GObject *    bobgui_builder_get_current_object      (BobguiBuilder    *builder);
GDK_AVAILABLE_IN_ALL
void         bobgui_builder_set_current_object      (BobguiBuilder    *builder,
                                                  GObject       *current_object);
GDK_AVAILABLE_IN_ALL
void         bobgui_builder_set_translation_domain  (BobguiBuilder   	*builder,
                                                  const char   	*domain);
GDK_AVAILABLE_IN_ALL
const char * bobgui_builder_get_translation_domain  (BobguiBuilder   	*builder);
GDK_AVAILABLE_IN_ALL
BobguiBuilderScope *bobgui_builder_get_scope           (BobguiBuilder    *builder);
GDK_AVAILABLE_IN_ALL
void         bobgui_builder_set_scope               (BobguiBuilder    *builder,
                                                  BobguiBuilderScope *scope);
GDK_AVAILABLE_IN_ALL
GType        bobgui_builder_get_type_from_name      (BobguiBuilder   	*builder,
                                                  const char   	*type_name);

GDK_AVAILABLE_IN_ALL
gboolean     bobgui_builder_value_from_string       (BobguiBuilder    *builder,
						  GParamSpec   	*pspec,
                                                  const char   	*string,
                                                  GValue       	*value,
						  GError       **error);
GDK_AVAILABLE_IN_ALL
gboolean     bobgui_builder_value_from_string_type  (BobguiBuilder    *builder,
						  GType        	 type,
                                                  const char   	*string,
                                                  GValue       	*value,
						  GError       **error);
GDK_AVAILABLE_IN_ALL
BobguiBuilder * bobgui_builder_new_from_file           (const char    *filename);
GDK_AVAILABLE_IN_ALL
BobguiBuilder * bobgui_builder_new_from_resource       (const char    *resource_path);
GDK_AVAILABLE_IN_ALL
BobguiBuilder * bobgui_builder_new_from_string         (const char    *string,
                                                  gssize         length);

GDK_AVAILABLE_IN_ALL
GClosure *   bobgui_builder_create_closure          (BobguiBuilder    *builder,
                                                  const char    *function_name,
                                                  BobguiBuilderClosureFlags flags,
                                                  GObject       *object,
                                                  GError       **error);



/**
 * BOBGUI_BUILDER_WARN_INVALID_CHILD_TYPE:
 * @object: the `BobguiBuildable` on which the warning occurred
 * @type: the unexpected type value
 *
 * This macro should be used to emit a warning about and unexpected @type value
 * in a `BobguiBuildable` add_child implementation.
 */
#define BOBGUI_BUILDER_WARN_INVALID_CHILD_TYPE(object, type) \
  g_warning ("'%s' is not a valid child type of '%s'", type, g_type_name (G_OBJECT_TYPE (object)))

GDK_AVAILABLE_IN_ALL
gboolean  bobgui_builder_extend_with_template  (BobguiBuilder    *builder,
                                             GObject       *object,
                                             GType          template_type,
                                             const char    *buffer,
                                             gssize         length,
                                             GError       **error);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiBuilder, g_object_unref)

G_END_DECLS

