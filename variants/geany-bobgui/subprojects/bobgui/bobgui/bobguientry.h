/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * Copyright (C) 2004-2006 Christian Hammond
 * Copyright (C) 2008 Cody Russell
 * Copyright (C) 2008 Red Hat, Inc.
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


#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguieditable.h>
#include <bobgui/bobguiimcontext.h>
#include <bobgui/bobguientrybuffer.h>
#include <bobgui/deprecated/bobguientrycompletion.h>
#include <bobgui/bobguiimage.h>


G_BEGIN_DECLS

#define BOBGUI_TYPE_ENTRY                  (bobgui_entry_get_type ())
#define BOBGUI_ENTRY(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_ENTRY, BobguiEntry))
#define BOBGUI_ENTRY_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_ENTRY, BobguiEntryClass))
#define BOBGUI_IS_ENTRY(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_ENTRY))
#define BOBGUI_IS_ENTRY_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_ENTRY))
#define BOBGUI_ENTRY_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_ENTRY, BobguiEntryClass))

/**
 * BobguiEntryIconPosition:
 * @BOBGUI_ENTRY_ICON_PRIMARY: At the beginning of the entry (depending on the text direction).
 * @BOBGUI_ENTRY_ICON_SECONDARY: At the end of the entry (depending on the text direction).
 *
 * Specifies the side of the entry at which an icon is placed.
 */
typedef enum
{
  BOBGUI_ENTRY_ICON_PRIMARY,
  BOBGUI_ENTRY_ICON_SECONDARY
} BobguiEntryIconPosition;

typedef struct _BobguiEntry              BobguiEntry;
typedef struct _BobguiEntryClass         BobguiEntryClass;

struct _BobguiEntry
{
  /*< private >*/
  BobguiWidget  parent_instance;
};

/**
 * BobguiEntryClass:
 * @parent_class: The parent class.
 * @activate: Class handler for the `BobguiEntry::activate` signal. The default
 *   implementation activates the bobgui.activate-default action.
 *
 * Class structure for `BobguiEntry`. All virtual functions have a default
 * implementation. Derived classes may set the virtual function pointers for the
 * signal handlers to %NULL, but must keep @get_text_area_size and
 * @get_frame_size non-%NULL; either use the default implementation, or provide
 * a custom one.
 */
struct _BobguiEntryClass
{
  BobguiWidgetClass parent_class;

  /* Action signals
   */
  void (* activate)           (BobguiEntry             *entry);

  /*< private >*/

  gpointer padding[8];
};

GDK_AVAILABLE_IN_ALL
GType      bobgui_entry_get_type       		(void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_entry_new            		(void);
GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_entry_new_with_buffer            (BobguiEntryBuffer *buffer);

GDK_AVAILABLE_IN_ALL
BobguiEntryBuffer* bobgui_entry_get_buffer            (BobguiEntry       *entry);
GDK_AVAILABLE_IN_ALL
void       bobgui_entry_set_buffer                 (BobguiEntry       *entry,
                                                 BobguiEntryBuffer *buffer);

GDK_AVAILABLE_IN_ALL
void       bobgui_entry_set_visibility 		(BobguiEntry      *entry,
						 gboolean       visible);
GDK_AVAILABLE_IN_ALL
gboolean   bobgui_entry_get_visibility             (BobguiEntry      *entry);

GDK_AVAILABLE_IN_ALL
void       bobgui_entry_set_invisible_char         (BobguiEntry      *entry,
                                                 gunichar       ch);
GDK_AVAILABLE_IN_ALL
gunichar   bobgui_entry_get_invisible_char         (BobguiEntry      *entry);
GDK_AVAILABLE_IN_ALL
void       bobgui_entry_unset_invisible_char       (BobguiEntry      *entry);

GDK_AVAILABLE_IN_ALL
void       bobgui_entry_set_has_frame              (BobguiEntry      *entry,
                                                 gboolean       setting);
GDK_AVAILABLE_IN_ALL
gboolean   bobgui_entry_get_has_frame              (BobguiEntry      *entry);

GDK_AVAILABLE_IN_ALL
void       bobgui_entry_set_overwrite_mode         (BobguiEntry      *entry,
                                                 gboolean       overwrite);
GDK_AVAILABLE_IN_ALL
gboolean   bobgui_entry_get_overwrite_mode         (BobguiEntry      *entry);

/* text is truncated if needed */
GDK_AVAILABLE_IN_ALL
void       bobgui_entry_set_max_length 		(BobguiEntry      *entry,
						 int            max);
GDK_AVAILABLE_IN_ALL
int        bobgui_entry_get_max_length             (BobguiEntry      *entry);
GDK_AVAILABLE_IN_ALL
guint16    bobgui_entry_get_text_length            (BobguiEntry      *entry);

GDK_AVAILABLE_IN_ALL
void       bobgui_entry_set_activates_default      (BobguiEntry      *entry,
                                                 gboolean       setting);
GDK_AVAILABLE_IN_ALL
gboolean   bobgui_entry_get_activates_default      (BobguiEntry      *entry);

GDK_AVAILABLE_IN_ALL
void       bobgui_entry_set_alignment              (BobguiEntry      *entry,
                                                 float          xalign);
GDK_AVAILABLE_IN_ALL
float      bobgui_entry_get_alignment              (BobguiEntry      *entry);

GDK_DEPRECATED_IN_4_10
void                bobgui_entry_set_completion (BobguiEntry           *entry,
                                              BobguiEntryCompletion *completion);
GDK_DEPRECATED_IN_4_10
BobguiEntryCompletion *bobgui_entry_get_completion (BobguiEntry           *entry);

/* Progress API
 */
GDK_AVAILABLE_IN_ALL
void           bobgui_entry_set_progress_fraction   (BobguiEntry     *entry,
                                                  double        fraction);
GDK_AVAILABLE_IN_ALL
double         bobgui_entry_get_progress_fraction   (BobguiEntry     *entry);

GDK_AVAILABLE_IN_ALL
void           bobgui_entry_set_progress_pulse_step (BobguiEntry     *entry,
                                                  double        fraction);
GDK_AVAILABLE_IN_ALL
double         bobgui_entry_get_progress_pulse_step (BobguiEntry     *entry);

GDK_AVAILABLE_IN_ALL
void           bobgui_entry_progress_pulse          (BobguiEntry     *entry);
GDK_AVAILABLE_IN_ALL
const char *   bobgui_entry_get_placeholder_text    (BobguiEntry             *entry);
GDK_AVAILABLE_IN_ALL
void           bobgui_entry_set_placeholder_text    (BobguiEntry             *entry,
                                                  const char           *text);
/* Setting and managing icons
 */
GDK_AVAILABLE_IN_ALL
void           bobgui_entry_set_icon_from_paintable         (BobguiEntry             *entry,
							  BobguiEntryIconPosition  icon_pos,
							  GdkPaintable         *paintable);
GDK_AVAILABLE_IN_ALL
void           bobgui_entry_set_icon_from_icon_name         (BobguiEntry             *entry,
							  BobguiEntryIconPosition  icon_pos,
							  const char           *icon_name);
GDK_AVAILABLE_IN_ALL
void           bobgui_entry_set_icon_from_gicon             (BobguiEntry             *entry,
							  BobguiEntryIconPosition  icon_pos,
							  GIcon                *icon);
GDK_AVAILABLE_IN_ALL
BobguiImageType   bobgui_entry_get_icon_storage_type           (BobguiEntry             *entry,
							  BobguiEntryIconPosition  icon_pos);
GDK_AVAILABLE_IN_ALL
GdkPaintable * bobgui_entry_get_icon_paintable              (BobguiEntry             *entry,
							  BobguiEntryIconPosition  icon_pos);
GDK_AVAILABLE_IN_ALL
const char * bobgui_entry_get_icon_name                     (BobguiEntry             *entry,
							  BobguiEntryIconPosition  icon_pos);
GDK_AVAILABLE_IN_ALL
GIcon*       bobgui_entry_get_icon_gicon                    (BobguiEntry             *entry,
							  BobguiEntryIconPosition  icon_pos);
GDK_AVAILABLE_IN_ALL
void         bobgui_entry_set_icon_activatable              (BobguiEntry             *entry,
							  BobguiEntryIconPosition  icon_pos,
							  gboolean              activatable);
GDK_AVAILABLE_IN_ALL
gboolean     bobgui_entry_get_icon_activatable              (BobguiEntry             *entry,
							  BobguiEntryIconPosition  icon_pos);
GDK_AVAILABLE_IN_ALL
void         bobgui_entry_set_icon_sensitive                (BobguiEntry             *entry,
							  BobguiEntryIconPosition  icon_pos,
							  gboolean              sensitive);
GDK_AVAILABLE_IN_ALL
gboolean     bobgui_entry_get_icon_sensitive                (BobguiEntry             *entry,
							  BobguiEntryIconPosition  icon_pos);
GDK_AVAILABLE_IN_ALL
int          bobgui_entry_get_icon_at_pos                   (BobguiEntry             *entry,
							  int                   x,
							  int                   y);
GDK_AVAILABLE_IN_ALL
void         bobgui_entry_set_icon_tooltip_text             (BobguiEntry             *entry,
							  BobguiEntryIconPosition  icon_pos,
							  const char           *tooltip);
GDK_AVAILABLE_IN_ALL
char *      bobgui_entry_get_icon_tooltip_text             (BobguiEntry             *entry,
                                                          BobguiEntryIconPosition  icon_pos);
GDK_AVAILABLE_IN_ALL
void         bobgui_entry_set_icon_tooltip_markup           (BobguiEntry             *entry,
							  BobguiEntryIconPosition  icon_pos,
							  const char           *tooltip);
GDK_AVAILABLE_IN_ALL
char *      bobgui_entry_get_icon_tooltip_markup           (BobguiEntry             *entry,
                                                          BobguiEntryIconPosition  icon_pos);
GDK_AVAILABLE_IN_ALL
void         bobgui_entry_set_icon_drag_source              (BobguiEntry             *entry,
							  BobguiEntryIconPosition  icon_pos,
							  GdkContentProvider   *provider,
							  GdkDragAction         actions);
GDK_AVAILABLE_IN_ALL
int          bobgui_entry_get_current_icon_drag_source      (BobguiEntry             *entry);
GDK_AVAILABLE_IN_ALL
void         bobgui_entry_get_icon_area                     (BobguiEntry             *entry,
                                                          BobguiEntryIconPosition  icon_pos,
                                                          GdkRectangle         *icon_area);

GDK_AVAILABLE_IN_ALL
void        bobgui_entry_reset_im_context                   (BobguiEntry             *entry);

GDK_AVAILABLE_IN_ALL
void            bobgui_entry_set_input_purpose                  (BobguiEntry             *entry,
                                                              BobguiInputPurpose       purpose);
GDK_AVAILABLE_IN_ALL
BobguiInputPurpose bobgui_entry_get_input_purpose                  (BobguiEntry             *entry);

GDK_AVAILABLE_IN_ALL
void            bobgui_entry_set_input_hints                    (BobguiEntry             *entry,
                                                              BobguiInputHints         hints);
GDK_AVAILABLE_IN_ALL
BobguiInputHints   bobgui_entry_get_input_hints                    (BobguiEntry             *entry);

GDK_AVAILABLE_IN_ALL
void            bobgui_entry_set_attributes                     (BobguiEntry             *entry,
                                                              PangoAttrList        *attrs);
GDK_AVAILABLE_IN_ALL
PangoAttrList  *bobgui_entry_get_attributes                     (BobguiEntry             *entry);

GDK_AVAILABLE_IN_ALL
void            bobgui_entry_set_tabs                           (BobguiEntry             *entry,
                                                              PangoTabArray        *tabs);

GDK_AVAILABLE_IN_ALL
PangoTabArray  *bobgui_entry_get_tabs                           (BobguiEntry             *entry);

GDK_AVAILABLE_IN_ALL
gboolean       bobgui_entry_grab_focus_without_selecting        (BobguiEntry             *entry);

GDK_AVAILABLE_IN_ALL
void           bobgui_entry_set_extra_menu                      (BobguiEntry             *entry,
                                                              GMenuModel           *model);
GDK_AVAILABLE_IN_ALL
GMenuModel *   bobgui_entry_get_extra_menu                      (BobguiEntry             *entry);

GDK_AVAILABLE_IN_4_20
const gchar *  bobgui_entry_get_menu_entry_icon_text            (BobguiEntry             *entry,
                                                              BobguiEntryIconPosition  icon_pos);

GDK_AVAILABLE_IN_4_20
void           bobgui_entry_set_menu_entry_icon_text            (BobguiEntry             *entry,
                                                              BobguiEntryIconPosition  icon_pos,
                                                              const gchar          *text);
G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiEntry, g_object_unref)

G_END_DECLS

