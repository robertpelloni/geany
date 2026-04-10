/*
 * Copyright © 2012 Red Hat Inc.
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
 * Authors: Alexander Larsson <alexl@gnome.org>
 */

#pragma once

#include <glib-object.h>

#include "bobguicsstypesprivate.h"
#include "bobguicssvariablesetprivate.h"
#include "bobguistyleprovider.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_CSS_VALUE           (bobgui_css_value_get_type ())

/* A BobguiCssValue is a refcounted immutable value type */

typedef struct _BobguiCssValue           BobguiCssValue;
typedef struct _BobguiCssValueClass      BobguiCssValueClass;

/* using define instead of struct here so compilers get the packing right */
#define BOBGUI_CSS_VALUE_BASE \
  const BobguiCssValueClass *class; \
  int ref_count; \
  guint is_computed: 1; \
  guint contains_variables: 1; \
  guint contains_current_color: 1;

typedef struct {
  BobguiStyleProvider   *provider;
  BobguiCssStyle        *style;
  BobguiCssStyle        *parent_style;
  BobguiCssVariableSet  *variables;
  BobguiCssValue       **shorthands;
} BobguiCssComputeContext;

struct _BobguiCssValueClass {
  const char *type_name;
  void          (* free)                              (BobguiCssValue                *value);

  BobguiCssValue * (* compute)                           (BobguiCssValue                *value,
                                                       guint                       property_id,
                                                       BobguiCssComputeContext       *context);
  BobguiCssValue * (* resolve)                           (BobguiCssValue                *value,
                                                       BobguiCssComputeContext       *context,
                                                       BobguiCssValue                *current);
  gboolean      (* equal)                             (const BobguiCssValue          *value1,
                                                       const BobguiCssValue          *value2);
  BobguiCssValue * (* transition)                        (BobguiCssValue                *start,
                                                       BobguiCssValue                *end,
                                                       guint                       property_id,
                                                       double                      progress);
  gboolean      (* is_dynamic)                        (const BobguiCssValue          *value);
  BobguiCssValue * (* get_dynamic_value)                 (BobguiCssValue                *value,
                                                       gint64                      monotonic_time);
  void          (* print)                             (const BobguiCssValue          *value,
                                                       GString                    *string);
};

GType         bobgui_css_value_get_type                  (void) G_GNUC_CONST;

BobguiCssValue * bobgui_css_value_alloc                     (const BobguiCssValueClass     *klass,
                                                       gsize                       size);
#define bobgui_css_value_new(name, klass) ((name *) bobgui_css_value_alloc ((klass), sizeof (name)))

BobguiCssValue * (bobgui_css_value_ref)                     (BobguiCssValue                *value);
void          (bobgui_css_value_unref)                   (BobguiCssValue                *value);

BobguiCssValue * bobgui_css_value_compute                   (BobguiCssValue                *value,
                                                       guint                       property_id,
                                                       BobguiCssComputeContext       *context) G_GNUC_PURE;
BobguiCssValue *  bobgui_css_value_resolve                  (BobguiCssValue                *value,
                                                       BobguiCssComputeContext       *context,
                                                       BobguiCssValue                *current) G_GNUC_PURE;
gboolean      bobgui_css_value_equal                     (const BobguiCssValue          *value1,
                                                       const BobguiCssValue          *value2) G_GNUC_PURE;
gboolean      bobgui_css_value_equal0                    (const BobguiCssValue          *value1,
                                                       const BobguiCssValue          *value2) G_GNUC_PURE;
BobguiCssValue * bobgui_css_value_transition                (BobguiCssValue                *start,
                                                       BobguiCssValue                *end,
                                                       guint                       property_id,
                                                       double                      progress);
gboolean       bobgui_css_value_is_dynamic               (const BobguiCssValue          *value) G_GNUC_PURE;
BobguiCssValue *  bobgui_css_value_get_dynamic_value        (BobguiCssValue                *value,
                                                       gint64                      monotonic_time);

char *         bobgui_css_value_to_string                (const BobguiCssValue          *value);
void           bobgui_css_value_print                    (const BobguiCssValue          *value,
                                                       GString                    *string);

typedef struct { BOBGUI_CSS_VALUE_BASE } BobguiCssValueBase;

static inline BobguiCssValue *
bobgui_css_value_ref_inline (BobguiCssValue *value)
{
  BobguiCssValueBase *value_base = (BobguiCssValueBase *) value;

  value_base->ref_count += 1;

  return value;
}

static inline void
bobgui_css_value_unref_inline (BobguiCssValue *value)
{
  BobguiCssValueBase *value_base = (BobguiCssValueBase *) value;

  if (value_base && value_base->ref_count > 1)
    {
      value_base->ref_count -= 1;
      return;
    }

  (bobgui_css_value_unref) (value);
}

#define bobgui_css_value_ref(value) bobgui_css_value_ref_inline (value)
#define bobgui_css_value_unref(value) bobgui_css_value_unref_inline (value)

static inline gboolean
bobgui_css_value_is_computed (const BobguiCssValue *value)
{
  BobguiCssValueBase *value_base = (BobguiCssValueBase *) value;

  return value_base->is_computed;
}

static inline gboolean
bobgui_css_value_contains_variables (const BobguiCssValue *value)
{
  BobguiCssValueBase *value_base = (BobguiCssValueBase *) value;

  return value_base->contains_variables;
}

static inline gboolean
bobgui_css_value_contains_current_color (const BobguiCssValue *value)
{
  BobguiCssValueBase *value_base = (BobguiCssValueBase *) value;

  return value_base->contains_current_color;
}

G_END_DECLS

