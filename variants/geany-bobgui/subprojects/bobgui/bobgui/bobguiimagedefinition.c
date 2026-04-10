/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2015 Benjamin Otte <otte@gnome.org>
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

#include "bobguiimagedefinitionprivate.h"

typedef struct _BobguiImageDefinitionEmpty BobguiImageDefinitionEmpty;
typedef struct _BobguiImageDefinitionIconName BobguiImageDefinitionIconName;
typedef struct _BobguiImageDefinitionGIcon BobguiImageDefinitionGIcon;
typedef struct _BobguiImageDefinitionPaintable BobguiImageDefinitionPaintable;

struct _BobguiImageDefinitionEmpty {
  BobguiImageType type;
  int ref_count;
};

struct _BobguiImageDefinitionIconName {
  BobguiImageType type;
  int ref_count;

  char *icon_name;
};

struct _BobguiImageDefinitionGIcon {
  BobguiImageType type;
  int ref_count;

  GIcon *gicon;
};

struct _BobguiImageDefinitionPaintable {
  BobguiImageType type;
  int ref_count;

  GdkPaintable *paintable;
};

union _BobguiImageDefinition
{
  BobguiImageType type;
  BobguiImageDefinitionEmpty empty;
  BobguiImageDefinitionIconName icon_name;
  BobguiImageDefinitionGIcon gicon;
  BobguiImageDefinitionPaintable paintable;
};

BobguiImageDefinition *
bobgui_image_definition_new_empty (void)
{
  static BobguiImageDefinitionEmpty empty = { BOBGUI_IMAGE_EMPTY, 1 };

  return bobgui_image_definition_ref ((BobguiImageDefinition *) &empty);
}

static inline BobguiImageDefinition *
bobgui_image_definition_alloc (BobguiImageType type)
{
  static gsize sizes[] = {
    sizeof (BobguiImageDefinitionEmpty),
    sizeof (BobguiImageDefinitionIconName),
    sizeof (BobguiImageDefinitionGIcon),
    sizeof (BobguiImageDefinitionPaintable)
  };
  BobguiImageDefinition *def;

  g_assert (type < G_N_ELEMENTS (sizes));

  def = g_malloc0 (sizes[type]);
  def->type = type;
  def->empty.ref_count = 1;

  return def;
}

BobguiImageDefinition *
bobgui_image_definition_new_icon_name (const char *icon_name)
{
  BobguiImageDefinition *def;

  if (icon_name == NULL || icon_name[0] == '\0')
    return NULL;

  def = bobgui_image_definition_alloc (BOBGUI_IMAGE_ICON_NAME);
  def->icon_name.icon_name = g_strdup (icon_name);

  return def;
}

BobguiImageDefinition *
bobgui_image_definition_new_gicon (GIcon *gicon)
{
  BobguiImageDefinition *def;

  if (gicon == NULL)
    return NULL;

  def = bobgui_image_definition_alloc (BOBGUI_IMAGE_GICON);
  def->gicon.gicon = g_object_ref (gicon);

  return def;
}

BobguiImageDefinition *
bobgui_image_definition_new_paintable (GdkPaintable *paintable)
{
  BobguiImageDefinition *def;

  if (paintable == NULL)
    return NULL;

  def = bobgui_image_definition_alloc (BOBGUI_IMAGE_PAINTABLE);
  def->paintable.paintable = g_object_ref (paintable);

  return def;
}

BobguiImageDefinition *
bobgui_image_definition_ref (BobguiImageDefinition *def)
{
  BobguiImageDefinitionEmpty *empty = (BobguiImageDefinitionEmpty *) def;

  empty->ref_count++;

  return def;
}

void
bobgui_image_definition_unref (BobguiImageDefinition *def)
{
  BobguiImageDefinitionEmpty *empty = (BobguiImageDefinitionEmpty *) def;

  empty->ref_count--;

  if (empty->ref_count > 0)
    return;

  switch (def->type)
    {
    default:
    case BOBGUI_IMAGE_EMPTY:
      g_assert_not_reached ();
      break;
    case BOBGUI_IMAGE_PAINTABLE:
      {
        BobguiImageDefinitionPaintable *paintable = (BobguiImageDefinitionPaintable *) def;
        g_object_unref (paintable->paintable);
      }
      break;
    case BOBGUI_IMAGE_ICON_NAME:
      {
        BobguiImageDefinitionIconName *icon_name = (BobguiImageDefinitionIconName *) def;
        g_free (icon_name->icon_name);
      }
      break;
    case BOBGUI_IMAGE_GICON:
      {
        BobguiImageDefinitionGIcon *gicon = (BobguiImageDefinitionGIcon *) def;
        g_object_unref (gicon->gicon);
      }
      break;
    }

  g_free (def);
}

BobguiImageType
bobgui_image_definition_get_storage_type (const BobguiImageDefinition *def)
{
  return def->type;
}

int
bobgui_image_definition_get_scale (const BobguiImageDefinition *def)
{
  switch (def->type)
    {
    default:
      g_assert_not_reached ();
    case BOBGUI_IMAGE_EMPTY:
    case BOBGUI_IMAGE_PAINTABLE:
    case BOBGUI_IMAGE_ICON_NAME:
    case BOBGUI_IMAGE_GICON:
      return 1;
    }
}

const char *
bobgui_image_definition_get_icon_name (const BobguiImageDefinition *def)
{
  const BobguiImageDefinitionIconName *icon_name = (const BobguiImageDefinitionIconName *) def;

  if (def->type != BOBGUI_IMAGE_ICON_NAME)
    return NULL;

  return icon_name->icon_name;
}

GIcon *
bobgui_image_definition_get_gicon (const BobguiImageDefinition *def)
{
  const BobguiImageDefinitionGIcon *gicon = (const BobguiImageDefinitionGIcon *) def;

  if (def->type != BOBGUI_IMAGE_GICON)
    return NULL;

  return gicon->gicon;
}

GdkPaintable *
bobgui_image_definition_get_paintable (const BobguiImageDefinition *def)
{
  const BobguiImageDefinitionPaintable *paintable = (const BobguiImageDefinitionPaintable *) def;

  if (def->type != BOBGUI_IMAGE_PAINTABLE)
    return NULL;

  return paintable->paintable;
}
