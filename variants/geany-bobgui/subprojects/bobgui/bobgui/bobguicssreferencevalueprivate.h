/*
 * Copyright (C) 2023 GNOME Foundation Inc.
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
 * Authors: Alice Mikhaylenko <alicem@gnome.org>
 */

#pragma once

#include <bobgui/css/bobguicss.h>
#include "bobguicssvalueprivate.h"
#include "bobguistylepropertyprivate.h"
#include "css/bobguicssvariablevalueprivate.h"

G_BEGIN_DECLS

BobguiCssValue *_bobgui_css_reference_value_new             (BobguiStyleProperty    *property,
                                                       BobguiCssVariableValue *value,
                                                       GFile               *file);
void         _bobgui_css_reference_value_set_subproperty (BobguiCssValue         *value,
                                                       guint                property);

G_END_DECLS
