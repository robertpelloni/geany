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

#include <bobgui/bobguibuilder.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_BUILDABLE            (bobgui_buildable_get_type ())
#define BOBGUI_BUILDABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_BUILDABLE, BobguiBuildable))
#define BOBGUI_IS_BUILDABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_BUILDABLE))
#define BOBGUI_BUILDABLE_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BOBGUI_TYPE_BUILDABLE, BobguiBuildableIface))

typedef struct _BobguiBuildable      BobguiBuildable; /* Dummy typedef */
typedef struct _BobguiBuildableIface BobguiBuildableIface;

typedef struct _BobguiBuildableParseContext      BobguiBuildableParseContext;
typedef struct _BobguiBuildableParser BobguiBuildableParser;

/**
 * BobguiBuildableParseContext:
 *
 * Provides context for parsing BobguiBuilder UI files.
 *
 * `BobguiBuildableParseContext` is an opaque struct.
 */

/**
 * BobguiBuildableParser:
 * @start_element: function called for open elements
 * @end_element: function called for close elements
 * @text: function called for character data
 * @error: function called on error
 *
 * A sub-parser for `BobguiBuildable` implementations.
 */
struct _BobguiBuildableParser
{
  /* Called for open tags <foo bar="baz"> */
  void (*start_element)  (BobguiBuildableParseContext *context,
                          const char               *element_name,
                          const char              **attribute_names,
                          const char              **attribute_values,
                          gpointer                  user_data,
                          GError                  **error);

  /* Called for close tags </foo> */
  void (*end_element)    (BobguiBuildableParseContext *context,
                          const char               *element_name,
                          gpointer                  user_data,
                          GError                  **error);

  /* Called for character data */
  /* text is not nul-terminated */
  void (*text)           (BobguiBuildableParseContext *context,
                          const char               *text,
                          gsize                     text_len,
                          gpointer                  user_data,
                          GError                  **error);

  /* Called on error, including one set by other
   * methods in the vtable. The GError should not be freed.
   */
  void (*error)          (BobguiBuildableParseContext *context,
                          GError                   *error,
                          gpointer                 user_data);

  /*< private >*/
  gpointer padding[4];
};

/**
 * BobguiBuildableIface:
 * @g_iface: the parent class
 * @set_id: Stores the id attribute given in the `BobguiBuilder` UI definition.
 *   `BobguiWidget` stores the name as object data. Implement this method if your
 *   object has some notion of “ID” and it makes sense to map the XML id
 *   attribute to it.
 * @get_id: The getter corresponding to @set_id. Implement this
 *   if you implement @set_id.
 * @add_child: Adds a child. The @type parameter can be used to
 *   differentiate the kind of child. `BobguiWidget` implements this
 *   to add event controllers to the widget, `BobguiNotebook` uses
 *   the @type to distinguish between page labels (of type "page-label")
 *   and normal children.
 * @set_buildable_property: Sets a property of a buildable object.
 *  It is normally not necessary to implement this, g_object_set_property()
 *  is used by default. `BobguiWindow` implements this to delay showing itself
 *  (i.e. setting the [property@Bobgui.Widget:visible] property) until the whole
 *  interface is created.
 * @construct_child: Constructs a child of a buildable that has been
 *  specified as “constructor” in the UI definition. This can be used to
 *  reference a widget created in a `<ui>` tag which is outside
 *  of the normal BobguiBuilder UI definition hierarchy.  A reference to the
 *  constructed object is returned and becomes owned by the caller.
 * @custom_tag_start: Implement this if the buildable needs to parse
 *  content below `<child>`. To handle an element, the implementation
 *  must fill in the @parser and @user_data and return %TRUE.
 *  `BobguiWidget` implements this to parse accessible attributes specified
 *  in `<accessibility>` elements.
 *  Note that @user_data must be freed in @custom_tag_end or @custom_finished.
 * @custom_tag_end: Called for the end tag of each custom element that is
 *  handled by the buildable (see @custom_tag_start).
 * @custom_finished: Called for each custom tag handled by the buildable
 *  when the builder finishes parsing (see @custom_tag_start)
 * @parser_finished: Called when a builder finishes the parsing
 *  of a UI definition. It is normally not necessary to implement this,
 *  unless you need to perform special cleanup actions. `BobguiWindow` sets
 *  the `BobguiWidget:visible` property here.
 * @get_internal_child: Returns an internal child of a buildable.
 *  `BobguiDialog` implements this to give access to its @vbox, making
 *  it possible to add children to the vbox in a UI definition.
 *  Implement this if the buildable has internal children that may
 *  need to be accessed from a UI definition.
 *
 * Contains methods to let `BobguiBuilder` construct an object from
 * a `BobguiBuilder` UI definition.
 */
struct _BobguiBuildableIface
{
  GTypeInterface g_iface;

  /* virtual table */
  void          (* set_id)                 (BobguiBuildable       *buildable,
                                            const char         *id);
  const char *  (* get_id)                 (BobguiBuildable       *buildable);

  /**
   * BobguiBuildableIface::add_child:
   * @buildable: a `BobguiBuildable`
   * @builder: a `BobguiBuilder`
   * @child: child to add
   * @type: (nullable): kind of child or %NULL
   *
   * Adds a child to @buildable. @type is an optional string
   * describing how the child should be added.
   */
  void          (* add_child)              (BobguiBuildable       *buildable,
                                            BobguiBuilder         *builder,
                                            GObject            *child,
                                            const char         *type);
  void          (* set_buildable_property) (BobguiBuildable       *buildable,
                                            BobguiBuilder         *builder,
                                            const char         *name,
                                            const GValue       *value);
  GObject *     (* construct_child)        (BobguiBuildable       *buildable,
                                            BobguiBuilder         *builder,
                                            const char         *name);

  /**
   * BobguiBuildableIface::custom_tag_start:
   * @buildable: a `BobguiBuildable`
   * @builder: a `BobguiBuilder` used to construct this object
   * @child: (nullable): child object or %NULL for non-child tags
   * @tagname: name of tag
   * @parser: (out): a `BobguiBuildableParser` to fill in
   * @data: (out): return location for user data that will be passed in
   *   to parser functions
   *
   * Called for each unknown element under `<child>`.
   *
   * Returns: %TRUE if an object has a custom implementation, %FALSE
   *   if it doesn't.
   */
  gboolean      (* custom_tag_start)       (BobguiBuildable       *buildable,
                                            BobguiBuilder         *builder,
                                            GObject            *child,
                                            const char         *tagname,
                                            BobguiBuildableParser *parser,
                                            gpointer           *data);
  /**
   * BobguiBuildableIface::custom_tag_end:
   * @buildable: A `BobguiBuildable`
   * @builder: `BobguiBuilder` used to construct this object
   * @child: (nullable): child object or %NULL for non-child tags
   * @tagname: name of tag
   * @data: user data that will be passed in to parser functions
   *
   * Called at the end of each custom element handled by
   * the buildable.
   */
  void          (* custom_tag_end)         (BobguiBuildable       *buildable,
                                            BobguiBuilder         *builder,
                                            GObject            *child,
                                            const char         *tagname,
                                            gpointer            data);
   /**
    * BobguiBuildableIface::custom_finished:
    * @buildable: a `BobguiBuildable`
    * @builder: a `BobguiBuilder`
    * @child: (nullable): child object or %NULL for non-child tags
    * @tagname: the name of the tag
    * @data: user data created in custom_tag_start
    *
    * Similar to bobgui_buildable_parser_finished() but is
    * called once for each custom tag handled by the @buildable.
    */
  void          (* custom_finished)        (BobguiBuildable       *buildable,
                                            BobguiBuilder         *builder,
                                            GObject            *child,
                                            const char         *tagname,
                                            gpointer            data);
  void          (* parser_finished)        (BobguiBuildable       *buildable,
                                            BobguiBuilder         *builder);

  /**
   * BobguiBuildableIface::get_internal_child:
   * @buildable: a `BobguiBuildable`
   * @builder: a `BobguiBuilder`
   * @childname: name of child
   *
   * Retrieves the internal child called @childname of the @buildable object.
   *
   * Returns: (transfer none): the internal child of the buildable object
   */
  GObject *     (* get_internal_child)     (BobguiBuildable       *buildable,
                                            BobguiBuilder         *builder,
                                            const char         *childname);
};


GDK_AVAILABLE_IN_ALL
GType     bobgui_buildable_get_type               (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
const char * bobgui_buildable_get_buildable_id    (BobguiBuildable        *buildable);

GDK_AVAILABLE_IN_ALL
void          bobgui_buildable_parse_context_push              (BobguiBuildableParseContext *context,
                                                             const BobguiBuildableParser *parser,
                                                             gpointer                  user_data);
GDK_AVAILABLE_IN_ALL
gpointer      bobgui_buildable_parse_context_pop               (BobguiBuildableParseContext *context);
GDK_AVAILABLE_IN_ALL
const char *  bobgui_buildable_parse_context_get_element       (BobguiBuildableParseContext *context);
GDK_AVAILABLE_IN_ALL
GPtrArray    *bobgui_buildable_parse_context_get_element_stack (BobguiBuildableParseContext *context);
GDK_AVAILABLE_IN_ALL
void          bobgui_buildable_parse_context_get_position      (BobguiBuildableParseContext *context,
                                                             int                      *line_number,
                                                             int                      *char_number);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiBuildable, g_object_unref)

G_END_DECLS

