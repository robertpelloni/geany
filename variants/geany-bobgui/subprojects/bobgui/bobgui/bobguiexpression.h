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

#include <bobgui/bobguitypes.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_EXPRESSION (bobgui_expression_get_type ())
#define BOBGUI_TYPE_EXPRESSION_WATCH (bobgui_expression_watch_get_type())

#define BOBGUI_IS_EXPRESSION(obj)  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_EXPRESSION))
#define BOBGUI_EXPRESSION(obj)     (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_EXPRESSION, BobguiExpression))

typedef struct _BobguiExpression BobguiExpression;
typedef struct _BobguiExpressionWatch BobguiExpressionWatch;

/**
 * BobguiExpressionNotify:
 * @user_data: data passed to bobgui_expression_watch()
 *
 * Callback called by bobgui_expression_watch() when the
 * expression value changes.
 */
typedef void            (* BobguiExpressionNotify)                 (gpointer                        user_data);

GDK_AVAILABLE_IN_ALL
GType                   bobgui_expression_get_type                 (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiExpression *         bobgui_expression_ref                      (BobguiExpression                  *self);
GDK_AVAILABLE_IN_ALL
void                    bobgui_expression_unref                    (BobguiExpression                  *self);
G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiExpression, bobgui_expression_unref)

GDK_AVAILABLE_IN_ALL
GType                   bobgui_expression_get_value_type           (BobguiExpression                  *self);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_expression_is_static                (BobguiExpression                  *self);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_expression_evaluate                 (BobguiExpression                  *self,
                                                                 gpointer                        this_,
                                                                 GValue                         *value);
GDK_AVAILABLE_IN_ALL
BobguiExpressionWatch *    bobgui_expression_watch                    (BobguiExpression                  *self,
                                                                 gpointer                        this_,
                                                                 BobguiExpressionNotify             notify,
                                                                 gpointer                        user_data,
                                                                 GDestroyNotify                  user_destroy);
GDK_AVAILABLE_IN_ALL
BobguiExpressionWatch *    bobgui_expression_bind                     (BobguiExpression                  *self,
                                                                 gpointer                        target,
                                                                 const char *                    property,
                                                                 gpointer                        this_);

GDK_AVAILABLE_IN_4_2
GType                   bobgui_expression_watch_get_type           (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiExpressionWatch *    bobgui_expression_watch_ref                (BobguiExpressionWatch             *watch);
GDK_AVAILABLE_IN_ALL
void                    bobgui_expression_watch_unref              (BobguiExpressionWatch             *watch);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_expression_watch_evaluate           (BobguiExpressionWatch             *watch,
                                                                 GValue                         *value);
GDK_AVAILABLE_IN_ALL
void                    bobgui_expression_watch_unwatch            (BobguiExpressionWatch             *watch);

#define BOBGUI_TYPE_PROPERTY_EXPRESSION (bobgui_property_expression_get_type())
typedef struct _BobguiPropertyExpression   BobguiPropertyExpression;

GDK_AVAILABLE_IN_ALL
GType                   bobgui_property_expression_get_type        (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiExpression *         bobgui_property_expression_new             (GType                           this_type,
                                                                 BobguiExpression                  *expression,
                                                                 const char                     *property_name);
GDK_AVAILABLE_IN_ALL
BobguiExpression *         bobgui_property_expression_new_for_pspec   (BobguiExpression                  *expression,
                                                                 GParamSpec                     *pspec);

GDK_AVAILABLE_IN_ALL
BobguiExpression *         bobgui_property_expression_get_expression  (BobguiExpression                  *expression);
GDK_AVAILABLE_IN_ALL
GParamSpec *            bobgui_property_expression_get_pspec       (BobguiExpression                  *expression);

#define BOBGUI_TYPE_CONSTANT_EXPRESSION (bobgui_constant_expression_get_type())
typedef struct _BobguiConstantExpression   BobguiConstantExpression;

GDK_AVAILABLE_IN_ALL
GType                   bobgui_constant_expression_get_type        (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiExpression *         bobgui_constant_expression_new             (GType                           value_type,
                                                                 ...);
GDK_AVAILABLE_IN_ALL
BobguiExpression *         bobgui_constant_expression_new_for_value   (const GValue                   *value);

GDK_AVAILABLE_IN_ALL
const GValue *          bobgui_constant_expression_get_value       (BobguiExpression                  *expression);

#define BOBGUI_TYPE_OBJECT_EXPRESSION (bobgui_object_expression_get_type())
typedef struct _BobguiObjectExpression     BobguiObjectExpression;

GDK_AVAILABLE_IN_ALL
GType                   bobgui_object_expression_get_type          (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiExpression *         bobgui_object_expression_new               (GObject                        *object);

GDK_AVAILABLE_IN_ALL
GObject *               bobgui_object_expression_get_object        (BobguiExpression                  *expression);

#define BOBGUI_TYPE_CLOSURE_EXPRESSION (bobgui_closure_expression_get_type())
typedef struct _BobguiClosureExpression    BobguiClosureExpression;

GDK_AVAILABLE_IN_ALL
GType                   bobgui_closure_expression_get_type         (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiExpression *         bobgui_closure_expression_new              (GType                           value_type,
                                                                 GClosure                       *closure,
                                                                 guint                           n_params,
                                                                 BobguiExpression                 **params);

#define BOBGUI_TYPE_CCLOSURE_EXPRESSION (bobgui_cclosure_expression_get_type())
typedef struct _BobguiCClosureExpression   BobguiCClosureExpression;

GDK_AVAILABLE_IN_ALL
GType                   bobgui_cclosure_expression_get_type        (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiExpression *         bobgui_cclosure_expression_new             (GType                           value_type,
                                                                 GClosureMarshal                 marshal,
                                                                 guint                           n_params,
                                                                 BobguiExpression                 **params,
                                                                 GCallback                       callback_func,
                                                                 gpointer                        user_data,
                                                                 GClosureNotify                  user_destroy);

#define BOBGUI_TYPE_TRY_EXPRESSION (bobgui_try_expression_get_type())
typedef struct _BobguiTryExpression   BobguiTryExpression;

GDK_AVAILABLE_IN_4_22
GType                   bobgui_try_expression_get_type             (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_4_22
BobguiExpression *         bobgui_try_expression_new                  (guint                           n_expressions,
                                                                 BobguiExpression                 **expressions);

/* GObject integration, so we can use BobguiBuilder */

/**
 * BOBGUI_VALUE_HOLDS_EXPRESSION:
 * @value: a `GValue`
 *
 * Evaluates to true if @value was initialized with `BOBGUI_TYPE_EXPRESSION`
 */
#define BOBGUI_VALUE_HOLDS_EXPRESSION(value)       (G_VALUE_HOLDS ((value), BOBGUI_TYPE_EXPRESSION))

GDK_AVAILABLE_IN_ALL
void            bobgui_value_set_expression        (GValue        *value,
                                                 BobguiExpression *expression);
GDK_AVAILABLE_IN_ALL
void            bobgui_value_take_expression       (GValue        *value,
                                                 BobguiExpression *expression);
GDK_AVAILABLE_IN_ALL
BobguiExpression * bobgui_value_get_expression        (const GValue  *value);
GDK_AVAILABLE_IN_ALL
BobguiExpression * bobgui_value_dup_expression        (const GValue  *value);

#define BOBGUI_TYPE_PARAM_SPEC_EXPRESSION (bobgui_param_expression_get_type())
#define BOBGUI_PARAM_SPEC_EXPRESSION(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_PARAM_SPEC_EXPRESSION, BobguiParamSpecExpression))
#define BOBGUI_IS_PARAM_SPEC_EXPRESSION(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_PARAM_SPEC_EXPRESSION))

/**
 * BobguiParamSpecExpression:
 *
 * A `GParamSpec` for properties holding a `BobguiExpression`.
 */
typedef struct {
  /*< private >*/
  GParamSpec parent_instance;
} BobguiParamSpecExpression;

GDK_AVAILABLE_IN_ALL
GType           bobgui_param_expression_get_type   (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
GParamSpec *    bobgui_param_spec_expression       (const char    *name,
                                                 const char    *nick,
                                                 const char    *blurb,
                                                 GParamFlags    flags);

G_END_DECLS

