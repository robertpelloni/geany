/* BOBGUI - The GIMP Toolkit
 * Copyright (C) 2017 Benjamin Otte <otte@gnome.org>
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

#pragma once

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_FISHBOWL                  (bobgui_fishbowl_get_type ())
#define BOBGUI_FISHBOWL(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_FISHBOWL, BobguiFishbowl))
#define BOBGUI_FISHBOWL_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_FISHBOWL, BobguiFishbowlClass))
#define BOBGUI_IS_FISHBOWL(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_FISHBOWL))
#define BOBGUI_IS_FISHBOWL_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_FISHBOWL))
#define BOBGUI_FISHBOWL_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_FISHBOWL, BobguiFishbowlClass))

typedef struct _BobguiFishbowl              BobguiFishbowl;
typedef struct _BobguiFishbowlClass         BobguiFishbowlClass;

typedef BobguiWidget * (* BobguiFishCreationFunc) (void);

struct _BobguiFishbowl
{
  BobguiWidget parent;
};

struct _BobguiFishbowlClass
{
  BobguiWidgetClass parent_class;
};

GType      bobgui_fishbowl_get_type          (void) G_GNUC_CONST;

BobguiWidget* bobgui_fishbowl_new               (void);

guint      bobgui_fishbowl_get_count         (BobguiFishbowl       *fishbowl);
void       bobgui_fishbowl_set_count         (BobguiFishbowl       *fishbowl,
                                           guint              count);
gboolean   bobgui_fishbowl_get_animating     (BobguiFishbowl       *fishbowl);
void       bobgui_fishbowl_set_animating     (BobguiFishbowl       *fishbowl,
                                           gboolean           animating);
gboolean   bobgui_fishbowl_get_benchmark     (BobguiFishbowl       *fishbowl);
void       bobgui_fishbowl_set_benchmark     (BobguiFishbowl       *fishbowl,
                                           gboolean           benchmark);
double     bobgui_fishbowl_get_framerate     (BobguiFishbowl       *fishbowl);
gint64     bobgui_fishbowl_get_update_delay  (BobguiFishbowl       *fishbowl);
void       bobgui_fishbowl_set_update_delay  (BobguiFishbowl       *fishbowl,
                                           gint64             update_delay);
void       bobgui_fishbowl_set_creation_func (BobguiFishbowl       *fishbowl,
                                           BobguiFishCreationFunc creation_func);

G_END_DECLS
