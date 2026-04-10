#ifndef BOBGUI_COMMAND_PALETTE_H
#define BOBGUI_COMMAND_PALETTE_H

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

typedef void (*BobguiCommandPaletteFunc) (const char *command_id,
                                          gpointer    user_data);

#define BOBGUI_TYPE_COMMAND_PALETTE (bobgui_command_palette_get_type ())
G_DECLARE_FINAL_TYPE (BobguiCommandPalette, bobgui_command_palette, BOBGUI, COMMAND_PALETTE, GObject)

BobguiCommandPalette * bobgui_command_palette_new               (BobguiApplication *application);
void                   bobgui_command_palette_clear             (BobguiCommandPalette     *self);
void                   bobgui_command_palette_set_pinned        (BobguiCommandPalette     *self,
                                                                const char               *command_id,
                                                                gboolean                  pinned);
gboolean               bobgui_command_palette_get_pinned        (BobguiCommandPalette     *self,
                                                                const char               *command_id);
int                    bobgui_command_palette_get_recent_count  (BobguiCommandPalette     *self,
                                                                const char               *command_id);
void                   bobgui_command_palette_mark_used         (BobguiCommandPalette     *self,
                                                                const char               *command_id);
void                   bobgui_command_palette_clear_history     (BobguiCommandPalette     *self);
void                   bobgui_command_palette_add_command_visual(BobguiCommandPalette     *self,
                                                                const char               *command_id,
                                                                const char               *title,
                                                                const char               *subtitle,
                                                                const char               *section,
                                                                const char               *icon_name,
                                                                BobguiCommandPaletteFunc  callback,
                                                                gpointer                  user_data);
void                   bobgui_command_palette_add_command       (BobguiCommandPalette     *self,
                                                                const char               *command_id,
                                                                const char               *title,
                                                                const char               *subtitle,
                                                                BobguiCommandPaletteFunc  callback,
                                                                gpointer                  user_data);
void                   bobgui_command_palette_attach_to_window  (BobguiCommandPalette *self,
                                                                BobguiWindow         *window);
void                   bobgui_command_palette_present           (BobguiCommandPalette *self);

G_END_DECLS

#endif
