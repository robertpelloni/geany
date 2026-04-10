/*
 * Copyright © 2019 Benjamin Otte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguitypes.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_BUILDER_SCOPE               (bobgui_builder_scope_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_INTERFACE (BobguiBuilderScope, bobgui_builder_scope, BOBGUI, BUILDER_SCOPE, GObject)

/**
 * BobguiBuilderClosureFlags:
 * @BOBGUI_BUILDER_CLOSURE_SWAPPED: The closure should be created swapped. See
 *   g_cclosure_new_swap() for details.
 *
 * The list of flags that can be passed to bobgui_builder_create_closure().
 *
 * New values may be added in the future for new features, so external
 * implementations of [iface@Bobgui.BuilderScope] should test the flags
 * for unknown values and raise a %BOBGUI_BUILDER_ERROR_INVALID_ATTRIBUTE error
 * when they encounter one.
 */
typedef enum { /*< prefix=BOBGUI_BUILDER_CLOSURE >*/
  BOBGUI_BUILDER_CLOSURE_SWAPPED = (1 << 0)
} BobguiBuilderClosureFlags;

/**
 * BobguiBuilderScopeInterface:
 * @get_type_from_name: Try to lookup a `GType` via the its name. See
 *   bobgui_builder_get_type_from_name() for more details.
 *   The C implementation will use g_type_from_name() and if that fails try to guess the
 *   correct function name for registering the type and then use dlsym() to load it.
 *   The default implementation just tries g_type_from_name() and otherwise fails.
 * @get_type_from_function: Try to lookup a `GType` via the given function name, specified
 *   explicitly in a BobguiBuilder file, like via the "type-func" attribute in the `<object>` tag.
 *   This function is very rarely used.
 *   The C implementation will use dlsym() and call the resulting function as a `GTypeFunc`.
 *   The default implementation will fail and just return %G_TYPE_INVALID.
 * @create_closure: Create a closure with the given arguments. See bobgui_builder_create_closure()
 *   for more details on those.
 *   The C implementation will try to use dlsym() to locate the function name and then
 *   g_cclosure_new() to create a closure for the symbol.
 *   The default implementation just fails and returns %NULL.
 *
 * The virtual function table to implement for `BobguiBuilderScope` implementations.
 * Default implementations for each function do exist, but they usually just fail,
 * so it is suggested that implementations implement all of them.
 */
struct _BobguiBuilderScopeInterface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/
  GType                 (* get_type_from_name)                  (BobguiBuilderScope        *self,
                                                                 BobguiBuilder             *builder,
                                                                 const char             *type_name);
  GType                 (* get_type_from_function)              (BobguiBuilderScope        *self,
                                                                 BobguiBuilder             *builder,
                                                                 const char             *function_name);

  GClosure *            (* create_closure)                      (BobguiBuilderScope        *self,
                                                                 BobguiBuilder             *builder,
                                                                 const char             *function_name,
                                                                 BobguiBuilderClosureFlags  flags,
                                                                 GObject                *object,
                                                                 GError                **error);
};



struct _BobguiBuilderCScopeClass
{
  GObjectClass parent_class;
};

#define BOBGUI_TYPE_BUILDER_CSCOPE               (bobgui_builder_cscope_get_type ())

GDK_AVAILABLE_IN_ALL
G_DECLARE_DERIVABLE_TYPE (BobguiBuilderCScope, bobgui_builder_cscope, BOBGUI, BUILDER_CSCOPE, GObject)

GDK_AVAILABLE_IN_ALL
BobguiBuilderScope *       bobgui_builder_cscope_new                  (void);
GDK_AVAILABLE_IN_ALL
void                    bobgui_builder_cscope_add_callback_symbol  (BobguiBuilderCScope       *self,
                                                                 const char             *callback_name,
                                                                 GCallback               callback_symbol);
GDK_AVAILABLE_IN_ALL
void                    bobgui_builder_cscope_add_callback_symbols (BobguiBuilderCScope       *self,
                                                                 const char             *first_callback_name,
                                                                 GCallback               first_callback_symbol,
                                                                 ...) G_GNUC_NULL_TERMINATED;

#define bobgui_builder_cscope_add_callback(scope, callback) \
  bobgui_builder_cscope_add_callback_symbol (BOBGUI_BUILDER_CSCOPE (scope), #callback, G_CALLBACK (callback))

GDK_AVAILABLE_IN_ALL
GCallback               bobgui_builder_cscope_lookup_callback_symbol(BobguiBuilderCScope      *self,
                                                                  const char            *callback_name);


G_END_DECLS

