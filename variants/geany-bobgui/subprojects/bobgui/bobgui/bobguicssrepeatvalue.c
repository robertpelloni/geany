/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2011 Red Hat, Inc.
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

#include "config.h"

#include "bobguicssrepeatvalueprivate.h"

#include "bobguicssnumbervalueprivate.h"

struct _BobguiCssValue {
  BOBGUI_CSS_VALUE_BASE
  BobguiCssRepeatStyle x;
  BobguiCssRepeatStyle y;
};

static void
bobgui_css_value_repeat_free (BobguiCssValue *value)
{
  g_free (value);
}

static BobguiCssValue *
bobgui_css_value_repeat_compute (BobguiCssValue          *value,
                              guint                 property_id,
                              BobguiCssComputeContext *context)
{
  return bobgui_css_value_ref (value);
}

static gboolean
bobgui_css_value_repeat_equal (const BobguiCssValue *repeat1,
                            const BobguiCssValue *repeat2)
{
  return repeat1->x == repeat2->x
      && repeat1->y == repeat2->y;
}

static BobguiCssValue *
bobgui_css_value_repeat_transition (BobguiCssValue *start,
                                 BobguiCssValue *end,
                                 guint        property_id,
                                 double       progress)
{
  return NULL;
}

static void
bobgui_css_value_background_repeat_print (const BobguiCssValue *repeat,
                                       GString           *string)
{
  static const char *names[] = {
    "no-repeat",
    "repeat",
    "round",
    "space"
  };

  if (repeat->x == repeat->y)
    {
      g_string_append (string, names[repeat->x]);
    }
  else if (repeat->x == BOBGUI_CSS_REPEAT_STYLE_REPEAT &&
           repeat->y == BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT)
    {
      g_string_append (string, "repeat-x");
    }
  else if (repeat->x == BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT &&
           repeat->y == BOBGUI_CSS_REPEAT_STYLE_REPEAT)
    {
      g_string_append (string, "repeat-y");
    }
  else
    {
      g_string_append (string, names[repeat->x]);
      g_string_append_c (string, ' ');
      g_string_append (string, names[repeat->y]);
    }
}

static void
bobgui_css_value_border_repeat_print (const BobguiCssValue *repeat,
                                   GString           *string)
{
  static const char *names[] = {
    "stretch",
    "repeat",
    "round",
    "space"
  };

  g_string_append (string, names[repeat->x]);
  if (repeat->x != repeat->y)
    {
      g_string_append_c (string, ' ');
      g_string_append (string, names[repeat->y]);
    }
}

static const BobguiCssValueClass BOBGUI_CSS_VALUE_BACKGROUND_REPEAT = {
  "BobguiCssBackgroundRepeatValue",
  bobgui_css_value_repeat_free,
  bobgui_css_value_repeat_compute,
  NULL,
  bobgui_css_value_repeat_equal,
  bobgui_css_value_repeat_transition,
  NULL,
  NULL,
  bobgui_css_value_background_repeat_print
};

static const BobguiCssValueClass BOBGUI_CSS_VALUE_BORDER_REPEAT = {
  "BobguiCssBorderRepeatValue",
  bobgui_css_value_repeat_free,
  bobgui_css_value_repeat_compute,
  NULL,
  bobgui_css_value_repeat_equal,
  bobgui_css_value_repeat_transition,
  NULL,
  NULL,
  bobgui_css_value_border_repeat_print
};
/* BACKGROUND REPEAT */

static struct {
  const char *name;
  BobguiCssValue values[4];
} background_repeat_values[4] = {
  { "no-repeat",
  { { &BOBGUI_CSS_VALUE_BACKGROUND_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT, BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT },
    { &BOBGUI_CSS_VALUE_BACKGROUND_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT, BOBGUI_CSS_REPEAT_STYLE_REPEAT    },
    { &BOBGUI_CSS_VALUE_BACKGROUND_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT, BOBGUI_CSS_REPEAT_STYLE_ROUND     },
    { &BOBGUI_CSS_VALUE_BACKGROUND_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT, BOBGUI_CSS_REPEAT_STYLE_SPACE     }
  } },
  { "repeat",
  { { &BOBGUI_CSS_VALUE_BACKGROUND_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_REPEAT,    BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT },
    { &BOBGUI_CSS_VALUE_BACKGROUND_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_REPEAT,    BOBGUI_CSS_REPEAT_STYLE_REPEAT    },
    { &BOBGUI_CSS_VALUE_BACKGROUND_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_REPEAT,    BOBGUI_CSS_REPEAT_STYLE_ROUND     },
    { &BOBGUI_CSS_VALUE_BACKGROUND_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_REPEAT,    BOBGUI_CSS_REPEAT_STYLE_SPACE     }
  } }, 
  { "round",
  { { &BOBGUI_CSS_VALUE_BACKGROUND_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_ROUND,     BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT },
    { &BOBGUI_CSS_VALUE_BACKGROUND_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_ROUND,     BOBGUI_CSS_REPEAT_STYLE_REPEAT    },
    { &BOBGUI_CSS_VALUE_BACKGROUND_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_ROUND,     BOBGUI_CSS_REPEAT_STYLE_ROUND     },
    { &BOBGUI_CSS_VALUE_BACKGROUND_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_ROUND,     BOBGUI_CSS_REPEAT_STYLE_SPACE     }
  } }, 
  { "space",
  { { &BOBGUI_CSS_VALUE_BACKGROUND_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_SPACE,     BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT },
    { &BOBGUI_CSS_VALUE_BACKGROUND_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_SPACE,     BOBGUI_CSS_REPEAT_STYLE_REPEAT    },
    { &BOBGUI_CSS_VALUE_BACKGROUND_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_SPACE,     BOBGUI_CSS_REPEAT_STYLE_ROUND     },
    { &BOBGUI_CSS_VALUE_BACKGROUND_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_SPACE,     BOBGUI_CSS_REPEAT_STYLE_SPACE     }
  } }
};

BobguiCssValue *
_bobgui_css_background_repeat_value_new (BobguiCssRepeatStyle x,
                                      BobguiCssRepeatStyle y)
{
  return bobgui_css_value_ref (&background_repeat_values[x].values[y]);
}

static gboolean
_bobgui_css_background_repeat_style_try (BobguiCssParser      *parser,
                                      BobguiCssRepeatStyle *result)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (background_repeat_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, background_repeat_values[i].name))
        {
          *result = i;
          return TRUE;
        }
    }

  return FALSE;
}

BobguiCssValue *
_bobgui_css_background_repeat_value_try_parse (BobguiCssParser *parser)
{
  BobguiCssRepeatStyle x, y;

  g_return_val_if_fail (parser != NULL, NULL);

  if (bobgui_css_parser_try_ident (parser, "repeat-x"))
    return _bobgui_css_background_repeat_value_new (BOBGUI_CSS_REPEAT_STYLE_REPEAT, BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT);
  if (bobgui_css_parser_try_ident (parser, "repeat-y"))
    return _bobgui_css_background_repeat_value_new (BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT, BOBGUI_CSS_REPEAT_STYLE_REPEAT);

  if (!_bobgui_css_background_repeat_style_try (parser, &x))
    return NULL;

  if (!_bobgui_css_background_repeat_style_try (parser, &y))
    y = x;

  return _bobgui_css_background_repeat_value_new (x, y);
}

BobguiCssRepeatStyle
_bobgui_css_background_repeat_value_get_x (const BobguiCssValue *repeat)
{
  g_return_val_if_fail (repeat->class == &BOBGUI_CSS_VALUE_BACKGROUND_REPEAT, BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT);

  return repeat->x;
}

BobguiCssRepeatStyle
_bobgui_css_background_repeat_value_get_y (const BobguiCssValue *repeat)
{
  g_return_val_if_fail (repeat->class == &BOBGUI_CSS_VALUE_BACKGROUND_REPEAT, BOBGUI_CSS_REPEAT_STYLE_NO_REPEAT);

  return repeat->y;
}

/* BORDER IMAGE REPEAT */

static struct {
  const char *name;
  BobguiCssValue values[4];
} border_repeat_values[4] = {
  { "stretch",
  { { &BOBGUI_CSS_VALUE_BORDER_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_STRETCH, BOBGUI_CSS_REPEAT_STYLE_STRETCH },
    { &BOBGUI_CSS_VALUE_BORDER_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_STRETCH, BOBGUI_CSS_REPEAT_STYLE_REPEAT  },
    { &BOBGUI_CSS_VALUE_BORDER_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_STRETCH, BOBGUI_CSS_REPEAT_STYLE_ROUND   },
    { &BOBGUI_CSS_VALUE_BORDER_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_STRETCH, BOBGUI_CSS_REPEAT_STYLE_SPACE   }
  } },
  { "repeat",
  { { &BOBGUI_CSS_VALUE_BORDER_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_REPEAT,  BOBGUI_CSS_REPEAT_STYLE_STRETCH },
    { &BOBGUI_CSS_VALUE_BORDER_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_REPEAT,  BOBGUI_CSS_REPEAT_STYLE_REPEAT  },
    { &BOBGUI_CSS_VALUE_BORDER_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_REPEAT,  BOBGUI_CSS_REPEAT_STYLE_ROUND   },
    { &BOBGUI_CSS_VALUE_BORDER_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_REPEAT,  BOBGUI_CSS_REPEAT_STYLE_SPACE   }
  } }, 
  { "round",
  { { &BOBGUI_CSS_VALUE_BORDER_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_ROUND,   BOBGUI_CSS_REPEAT_STYLE_STRETCH },
    { &BOBGUI_CSS_VALUE_BORDER_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_ROUND,   BOBGUI_CSS_REPEAT_STYLE_REPEAT  },
    { &BOBGUI_CSS_VALUE_BORDER_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_ROUND,   BOBGUI_CSS_REPEAT_STYLE_ROUND   },
    { &BOBGUI_CSS_VALUE_BORDER_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_ROUND,   BOBGUI_CSS_REPEAT_STYLE_SPACE   }
  } }, 
  { "space",
  { { &BOBGUI_CSS_VALUE_BORDER_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_SPACE,   BOBGUI_CSS_REPEAT_STYLE_STRETCH },
    { &BOBGUI_CSS_VALUE_BORDER_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_SPACE,   BOBGUI_CSS_REPEAT_STYLE_REPEAT  },
    { &BOBGUI_CSS_VALUE_BORDER_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_SPACE,   BOBGUI_CSS_REPEAT_STYLE_ROUND   },
    { &BOBGUI_CSS_VALUE_BORDER_REPEAT, 1, 1, 0, 0, BOBGUI_CSS_REPEAT_STYLE_SPACE,   BOBGUI_CSS_REPEAT_STYLE_SPACE   }
  } }
};

BobguiCssValue *
_bobgui_css_border_repeat_value_new (BobguiCssRepeatStyle x,
                                  BobguiCssRepeatStyle y)
{
  return bobgui_css_value_ref (&border_repeat_values[x].values[y]);
}

static gboolean
_bobgui_css_border_repeat_style_try (BobguiCssParser      *parser,
                                  BobguiCssRepeatStyle *result)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (border_repeat_values); i++)
    {
      if (bobgui_css_parser_try_ident (parser, border_repeat_values[i].name))
        {
          *result = i;
          return TRUE;
        }
    }

  return FALSE;
}

BobguiCssValue *
_bobgui_css_border_repeat_value_try_parse (BobguiCssParser *parser)
{
  BobguiCssRepeatStyle x, y;

  g_return_val_if_fail (parser != NULL, NULL);

  if (!_bobgui_css_border_repeat_style_try (parser, &x))
    return NULL;

  if (!_bobgui_css_border_repeat_style_try (parser, &y))
    y = x;

  return _bobgui_css_border_repeat_value_new (x, y);
}

BobguiCssRepeatStyle
_bobgui_css_border_repeat_value_get_x (const BobguiCssValue *repeat)
{
  g_return_val_if_fail (repeat->class == &BOBGUI_CSS_VALUE_BORDER_REPEAT, BOBGUI_CSS_REPEAT_STYLE_STRETCH);

  return repeat->x;
}

BobguiCssRepeatStyle
_bobgui_css_border_repeat_value_get_y (const BobguiCssValue *repeat)
{
  g_return_val_if_fail (repeat->class == &BOBGUI_CSS_VALUE_BORDER_REPEAT, BOBGUI_CSS_REPEAT_STYLE_STRETCH);

  return repeat->y;
}

