/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#pragma once

#if !defined (__BOBGUI_CSS_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/css/bobguicss.h> can be included directly."
#endif

#include <glib.h>
#include <gdk/version/gdkversionmacros.h>

/**
 * BobguiCssParserError:
 * @BOBGUI_CSS_PARSER_ERROR_FAILED: Unknown failure.
 * @BOBGUI_CSS_PARSER_ERROR_SYNTAX: The given text does not form valid syntax
 * @BOBGUI_CSS_PARSER_ERROR_IMPORT: Failed to import a resource
 * @BOBGUI_CSS_PARSER_ERROR_NAME: The given name has not been defined
 * @BOBGUI_CSS_PARSER_ERROR_UNKNOWN_VALUE: The given value is not correct
 *
 * Errors that can occur while parsing CSS.
 *
 * These errors are unexpected and will cause parts of the given CSS
 * to be ignored.
 */
typedef enum
{
  BOBGUI_CSS_PARSER_ERROR_FAILED,
  BOBGUI_CSS_PARSER_ERROR_SYNTAX,
  BOBGUI_CSS_PARSER_ERROR_IMPORT,
  BOBGUI_CSS_PARSER_ERROR_NAME,
  BOBGUI_CSS_PARSER_ERROR_UNKNOWN_VALUE
} BobguiCssParserError;

/**
 * BobguiCssParserWarning:
 * @BOBGUI_CSS_PARSER_WARNING_DEPRECATED: The given construct is
 *   deprecated and will be removed in a future version
 * @BOBGUI_CSS_PARSER_WARNING_SYNTAX: A syntax construct was used
 *   that should be avoided
 * @BOBGUI_CSS_PARSER_WARNING_UNIMPLEMENTED: A feature is not implemented
 *
 * Warnings that can occur while parsing CSS.
 *
 * Unlike `BobguiCssParserError`s, warnings do not cause the parser to
 * skip any input, but they indicate issues that should be fixed.
 */
typedef enum
{
  BOBGUI_CSS_PARSER_WARNING_DEPRECATED,
  BOBGUI_CSS_PARSER_WARNING_SYNTAX,
  BOBGUI_CSS_PARSER_WARNING_UNIMPLEMENTED
} BobguiCssParserWarning;

