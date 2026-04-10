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
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "config.h"

#include <math.h>
#include <string.h>

#include "bobguicssimagecrossfadeprivate.h"

#include "bobguicssnumbervalueprivate.h"
#include "bobguicssimagefallbackprivate.h"
#include "bobguicsscolorvalueprivate.h"


typedef struct _CrossFadeEntry CrossFadeEntry;

struct _CrossFadeEntry
{
  double progress;
  gboolean has_progress;
  BobguiCssImage *image;
};

G_DEFINE_TYPE (BobguiCssImageCrossFade, bobgui_css_image_cross_fade, BOBGUI_TYPE_CSS_IMAGE)

static void
cross_fade_entry_clear (gpointer data)
{
  CrossFadeEntry *entry = data;

  g_clear_object (&entry->image);
}

static void
bobgui_css_image_cross_fade_recalculate_progress (BobguiCssImageCrossFade *self)
{
  double total_progress;
  guint n_no_progress;
  guint i;

  total_progress = 0.0;
  n_no_progress = 0;

  for (i = 0; i < self->images->len; i++)
    {
      CrossFadeEntry *entry = &g_array_index (self->images, CrossFadeEntry, i);

      if (entry->has_progress)
        total_progress += entry->progress;
      else
        n_no_progress++;
    }

  if (n_no_progress)
    {
      double progress;
      if (total_progress >= 1.0)
        {
          progress = 0.0;
        }
      else
        {
          progress = (1.0 - total_progress) / n_no_progress;
          total_progress = 1.0;
        }
      for (i = 0; i < self->images->len; i++)
        {
          CrossFadeEntry *entry = &g_array_index (self->images, CrossFadeEntry, i);

          if (!entry->has_progress)
            entry->progress = progress;
        }
    }

  self->total_progress = total_progress;
}

static void
bobgui_css_image_cross_fade_add (BobguiCssImageCrossFade *self,
                              gboolean              has_progress,
                              double                progress,
                              BobguiCssImage          *image)
{
  CrossFadeEntry entry;

  entry.has_progress = has_progress;
  entry.progress = progress;
  entry.image = image;
  g_array_append_val (self->images, entry);

  bobgui_css_image_cross_fade_recalculate_progress (self);
}

static BobguiCssImageCrossFade *
bobgui_css_image_cross_fade_new_empty (void)
{
  return g_object_new (BOBGUI_TYPE_CSS_IMAGE_CROSS_FADE, NULL);
}

/* XXX: The following is not correct, it should actually run the
 * CSS sizing algorithm for every child, not just query height and
 * width independently.
 */
static int
bobgui_css_image_cross_fade_get_width (BobguiCssImage *image)
{
  BobguiCssImageCrossFade *self = BOBGUI_CSS_IMAGE_CROSS_FADE (image);
  double sum_width, sum_progress;
  guint i;

  sum_width = 0.0;
  sum_progress = 0.0;

  for (i = 0; i < self->images->len; i++)
    {
      CrossFadeEntry *entry = &g_array_index (self->images, CrossFadeEntry, i);
      int image_width;

      image_width = _bobgui_css_image_get_width (entry->image);
      if (image_width == 0)
        continue;
      sum_width += image_width * entry->progress;
      sum_progress += entry->progress;
    }

  if (sum_progress <= 0.0)
    return 0;

  return ceil (sum_width / sum_progress);
}

static int
bobgui_css_image_cross_fade_get_height (BobguiCssImage *image)
{
  BobguiCssImageCrossFade *self = BOBGUI_CSS_IMAGE_CROSS_FADE (image);
  double sum_height, sum_progress;
  guint i;

  sum_height = 0.0;
  sum_progress = 0.0;

  for (i = 0; i < self->images->len; i++)
    {
      CrossFadeEntry *entry = &g_array_index (self->images, CrossFadeEntry, i);
      int image_height;

      image_height = _bobgui_css_image_get_height (entry->image);
      if (image_height == 0)
        continue;
      sum_height += image_height * entry->progress;
      sum_progress += entry->progress;
    }

  if (sum_progress <= 0.0)
    return 0;

  return ceil (sum_height / sum_progress);
}

static gboolean
bobgui_css_image_cross_fade_equal (BobguiCssImage *image1,
                                BobguiCssImage *image2)
{
  BobguiCssImageCrossFade *cross_fade1 = BOBGUI_CSS_IMAGE_CROSS_FADE (image1);
  BobguiCssImageCrossFade *cross_fade2 = BOBGUI_CSS_IMAGE_CROSS_FADE (image2);
  guint i;

  if (cross_fade1->images->len != cross_fade2->images->len)
    return FALSE;

  for (i = 0; i < cross_fade1->images->len; i++)
    {
      CrossFadeEntry *entry1 = &g_array_index (cross_fade1->images, CrossFadeEntry, i);
      CrossFadeEntry *entry2 = &g_array_index (cross_fade2->images, CrossFadeEntry, i);

      if (entry1->progress != entry2->progress ||
          !_bobgui_css_image_equal (entry1->image, entry2->image))
        return FALSE;
    }
  
  return TRUE;
}

static gboolean
bobgui_css_image_cross_fade_is_dynamic (BobguiCssImage *image)
{
  BobguiCssImageCrossFade *self = BOBGUI_CSS_IMAGE_CROSS_FADE (image);
  guint i;

  for (i = 0; i < self->images->len; i++)
    {
      CrossFadeEntry *entry = &g_array_index (self->images, CrossFadeEntry, i);

      if (bobgui_css_image_is_dynamic (entry->image))
        return TRUE;
    }

  return FALSE;
}

static BobguiCssImage *
bobgui_css_image_cross_fade_get_dynamic_image (BobguiCssImage *image,
                                            gint64       monotonic_time)
{
  BobguiCssImageCrossFade *self = BOBGUI_CSS_IMAGE_CROSS_FADE (image);
  BobguiCssImageCrossFade *result;
  guint i;

  result = bobgui_css_image_cross_fade_new_empty ();

  for (i = 0; i < self->images->len; i++)
    {
      CrossFadeEntry *entry = &g_array_index (self->images, CrossFadeEntry, i);

      bobgui_css_image_cross_fade_add (result,
                                    entry->has_progress,
                                    entry->progress,
                                    bobgui_css_image_get_dynamic_image (entry->image, monotonic_time));
    }

  return BOBGUI_CSS_IMAGE (result);
}

static void
bobgui_css_image_cross_fade_snapshot (BobguiCssImage *image,
                                   BobguiSnapshot *snapshot,
                                   double       width,
                                   double       height)
{
  BobguiCssImageCrossFade *self = BOBGUI_CSS_IMAGE_CROSS_FADE (image);
  double remaining;
  guint i, n_cross_fades;

  if (self->total_progress < 1.0)
    {
      n_cross_fades = self->images->len;
      remaining = 1.0;
    }
  else
    {
      n_cross_fades = self->images->len - 1;
      remaining = self->total_progress;
    }

  for (i = 0; i < n_cross_fades; i++)
    {
      CrossFadeEntry *entry = &g_array_index (self->images, CrossFadeEntry, i);

      bobgui_snapshot_push_cross_fade (snapshot, 1.0 - entry->progress / remaining);
      remaining -= entry->progress;
      bobgui_css_image_snapshot (entry->image, snapshot, width, height);
      bobgui_snapshot_pop (snapshot);
    }

  if (n_cross_fades < self->images->len)
    {
      CrossFadeEntry *entry = &g_array_index (self->images, CrossFadeEntry, self->images->len - 1);
      bobgui_css_image_snapshot (entry->image, snapshot, width, height);
    }

  for (i = 0; i < n_cross_fades; i++)
    {
      bobgui_snapshot_pop (snapshot);
    }
}

static gboolean
parse_progress (BobguiCssParser *parser,
                gpointer      option_data,
                gpointer      user_data)
{
  double *progress = option_data;
  BobguiCssValue *number;
  
  number = bobgui_css_number_value_parse (parser, BOBGUI_CSS_PARSE_PERCENT | BOBGUI_CSS_POSITIVE_ONLY);
  if (number == NULL)
    return FALSE;
  *progress = bobgui_css_number_value_get (number, 1);
  bobgui_css_value_unref (number);

  if (*progress > 1.0)
    {
      bobgui_css_parser_error_value (parser, "Percentages over 100%% are not allowed. Given value: %f", *progress);
      return FALSE;
    }

  return TRUE;
}

static gboolean
parse_image (BobguiCssParser *parser,
             gpointer      option_data,
             gpointer      user_data)
{
  BobguiCssImage **image = option_data;

  if (_bobgui_css_image_can_parse (parser))
    {
      *image = _bobgui_css_image_new_parse (parser);
      if (*image == NULL)
        return FALSE;

      return TRUE;
    }
  else if (bobgui_css_color_value_can_parse (parser))
    {
      BobguiCssValue *color;

      color = bobgui_css_color_value_parse (parser);
      if (color == NULL)
        return FALSE;

      *image = _bobgui_css_image_fallback_new_for_color (color);

      return TRUE;
    }
  
  return FALSE;
}

static guint
bobgui_css_image_cross_fade_parse_arg (BobguiCssParser *parser,
                                    guint         arg,
                                    gpointer      data)
{
  BobguiCssImageCrossFade *self = data;
  double progress = -1.0;
  BobguiCssImage *image = NULL;
  BobguiCssParseOption options[] =
    {
      { (void *)bobgui_css_number_value_can_parse, parse_progress, &progress },
      { NULL, parse_image, &image },
    };

  if (!bobgui_css_parser_consume_any (parser, options, G_N_ELEMENTS (options), self))
    return 0;

  g_assert (image != NULL);

  if (progress < 0.0)
    bobgui_css_image_cross_fade_add (self, FALSE, 0.0, image);
  else
    bobgui_css_image_cross_fade_add (self, TRUE, progress, image);

  return 1;
}

static gboolean
bobgui_css_image_cross_fade_parse (BobguiCssImage  *image,
                                BobguiCssParser *parser)
{
  if (!bobgui_css_parser_has_function (parser, "cross-fade"))
    {
      bobgui_css_parser_error_syntax (parser, "Expected 'cross-fade('");
      return FALSE;
    }

  return bobgui_css_parser_consume_function (parser, 1, G_MAXUINT, bobgui_css_image_cross_fade_parse_arg, image);
}

static void
bobgui_css_image_cross_fade_print (BobguiCssImage *image,
                                GString     *string)
{
  BobguiCssImageCrossFade *self = BOBGUI_CSS_IMAGE_CROSS_FADE (image);
  guint i;

  g_string_append (string, "cross-fade(");

  for (i = 0; i < self->images->len; i++)
    {
      CrossFadeEntry *entry = &g_array_index (self->images, CrossFadeEntry, i);

      if (i > 0)
        g_string_append_printf (string, ", ");
      if (entry->has_progress)
        g_string_append_printf (string, "%g%% ", entry->progress * 100.0);
      _bobgui_css_image_print (entry->image, string);
    }

  g_string_append (string, ")");
}

static BobguiCssImage *
bobgui_css_image_cross_fade_compute (BobguiCssImage          *image,
                                  guint                 property_id,
                                  BobguiCssComputeContext *context)
{
  BobguiCssImageCrossFade *self = BOBGUI_CSS_IMAGE_CROSS_FADE (image);
  BobguiCssImageCrossFade *result;
  guint i;

  result = bobgui_css_image_cross_fade_new_empty ();

  for (i = 0; i < self->images->len; i++)
    {
      CrossFadeEntry *entry = &g_array_index (self->images, CrossFadeEntry, i);

      bobgui_css_image_cross_fade_add (result,
                                    entry->has_progress,
                                    entry->progress,
                                    _bobgui_css_image_compute (entry->image, property_id, context));
    }

  return BOBGUI_CSS_IMAGE (result);
}

static void
bobgui_css_image_cross_fade_dispose (GObject *object)
{
  BobguiCssImageCrossFade *cross_fade = BOBGUI_CSS_IMAGE_CROSS_FADE (object);

  g_clear_pointer (&cross_fade->images, g_array_unref);

  G_OBJECT_CLASS (bobgui_css_image_cross_fade_parent_class)->dispose (object);
}

static gboolean
bobgui_css_image_cross_fade_is_computed (BobguiCssImage *image)
{
  BobguiCssImageCrossFade *cross_fade = BOBGUI_CSS_IMAGE_CROSS_FADE (image);
  guint i;

  for (i = 0; i < cross_fade->images->len; i++)
    {
      const CrossFadeEntry *entry = &g_array_index (cross_fade->images, CrossFadeEntry, i);
      if (!bobgui_css_image_is_computed (entry->image))
        return FALSE;
    }

  return TRUE;
}

static gboolean
bobgui_css_image_cross_fade_contains_current_color (BobguiCssImage *image)
{
  BobguiCssImageCrossFade *cross_fade = BOBGUI_CSS_IMAGE_CROSS_FADE (image);
  guint i;

  for (i = 0; i < cross_fade->images->len; i++)
    {
      const CrossFadeEntry *entry = &g_array_index (cross_fade->images, CrossFadeEntry, i);
      if (bobgui_css_image_contains_current_color (entry->image))
        return TRUE;
    }

  return FALSE;
}

static BobguiCssImage *
bobgui_css_image_cross_fade_resolve (BobguiCssImage          *image,
                                  BobguiCssComputeContext *context,
                                  BobguiCssValue          *current)
{
  BobguiCssImageCrossFade *self = BOBGUI_CSS_IMAGE_CROSS_FADE (image);
  BobguiCssImageCrossFade *result;
  guint i;

  if (!bobgui_css_image_cross_fade_contains_current_color (image))
    return g_object_ref (image);

  result = bobgui_css_image_cross_fade_new_empty ();

  for (i = 0; i < self->images->len; i++)
    {
      CrossFadeEntry *entry = &g_array_index (self->images, CrossFadeEntry, i);

      bobgui_css_image_cross_fade_add (result,
                                    entry->has_progress,
                                    entry->progress,
                                    bobgui_css_image_resolve (entry->image, context, current));
    }

  return BOBGUI_CSS_IMAGE (result);
}

static void
bobgui_css_image_cross_fade_class_init (BobguiCssImageCrossFadeClass *klass)
{
  BobguiCssImageClass *image_class = BOBGUI_CSS_IMAGE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  image_class->get_width = bobgui_css_image_cross_fade_get_width;
  image_class->get_height = bobgui_css_image_cross_fade_get_height;
  image_class->compute = bobgui_css_image_cross_fade_compute;
  image_class->equal = bobgui_css_image_cross_fade_equal;
  image_class->snapshot = bobgui_css_image_cross_fade_snapshot;
  image_class->is_dynamic = bobgui_css_image_cross_fade_is_dynamic;
  image_class->get_dynamic_image = bobgui_css_image_cross_fade_get_dynamic_image;
  image_class->parse = bobgui_css_image_cross_fade_parse;
  image_class->print = bobgui_css_image_cross_fade_print;
  image_class->is_computed = bobgui_css_image_cross_fade_is_computed;
  image_class->contains_current_color = bobgui_css_image_cross_fade_contains_current_color;
  image_class->resolve = bobgui_css_image_cross_fade_resolve;

  object_class->dispose = bobgui_css_image_cross_fade_dispose;
}

static void
bobgui_css_image_cross_fade_init (BobguiCssImageCrossFade *self)
{
  self->images = g_array_new (FALSE, FALSE, sizeof (CrossFadeEntry));
  g_array_set_clear_func (self->images, cross_fade_entry_clear);
}

BobguiCssImage *
_bobgui_css_image_cross_fade_new (BobguiCssImage *start,
                               BobguiCssImage *end,
                               double       progress)
{
  BobguiCssImageCrossFade *self;

  g_return_val_if_fail (start == NULL || BOBGUI_IS_CSS_IMAGE (start), NULL);
  g_return_val_if_fail (end == NULL || BOBGUI_IS_CSS_IMAGE (end), NULL);

  self = bobgui_css_image_cross_fade_new_empty ();

  if (start)
    bobgui_css_image_cross_fade_add (self, TRUE, 1.0 - progress, g_object_ref (start));
  if (end)
    bobgui_css_image_cross_fade_add (self, TRUE, progress, g_object_ref (end));

  return BOBGUI_CSS_IMAGE (self);
}

