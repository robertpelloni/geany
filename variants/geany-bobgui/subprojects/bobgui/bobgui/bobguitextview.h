/* BOBGUI - The Bobgui Framework
 * bobguitextview.h Copyright (C) 2000 Red Hat, Inc.
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

#include <bobgui/bobguiwidget.h>
#include <bobgui/bobguiimcontext.h>
#include <bobgui/bobguitextbuffer.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_TEXT_VIEW             (bobgui_text_view_get_type ())
#define BOBGUI_TEXT_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TEXT_VIEW, BobguiTextView))
#define BOBGUI_TEXT_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_TEXT_VIEW, BobguiTextViewClass))
#define BOBGUI_IS_TEXT_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TEXT_VIEW))
#define BOBGUI_IS_TEXT_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_TEXT_VIEW))
#define BOBGUI_TEXT_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_TEXT_VIEW, BobguiTextViewClass))

/**
 * BobguiTextWindowType:
 * @BOBGUI_TEXT_WINDOW_WIDGET: Window that floats over scrolling areas.
 * @BOBGUI_TEXT_WINDOW_TEXT: Scrollable text window.
 * @BOBGUI_TEXT_WINDOW_LEFT: Left side border window.
 * @BOBGUI_TEXT_WINDOW_RIGHT: Right side border window.
 * @BOBGUI_TEXT_WINDOW_TOP: Top border window.
 * @BOBGUI_TEXT_WINDOW_BOTTOM: Bottom border window.
 *
 * Used to reference the parts of `BobguiTextView`.
 */
typedef enum
{
  BOBGUI_TEXT_WINDOW_WIDGET = 1,
  BOBGUI_TEXT_WINDOW_TEXT,
  BOBGUI_TEXT_WINDOW_LEFT,
  BOBGUI_TEXT_WINDOW_RIGHT,
  BOBGUI_TEXT_WINDOW_TOP,
  BOBGUI_TEXT_WINDOW_BOTTOM
} BobguiTextWindowType;

/**
 * BobguiTextViewLayer:
 * @BOBGUI_TEXT_VIEW_LAYER_BELOW_TEXT: The layer rendered below the text (but above the background).
 * @BOBGUI_TEXT_VIEW_LAYER_ABOVE_TEXT: The layer rendered above the text.
 *
 * Used to reference the layers of `BobguiTextView` for the purpose of customized
 * drawing with the ::snapshot_layer vfunc.
 */
typedef enum
{
  BOBGUI_TEXT_VIEW_LAYER_BELOW_TEXT,
  BOBGUI_TEXT_VIEW_LAYER_ABOVE_TEXT
} BobguiTextViewLayer;

/**
 * BobguiTextExtendSelection:
 * @BOBGUI_TEXT_EXTEND_SELECTION_WORD: Selects the current word. It is triggered by
 *   a double-click for example.
 * @BOBGUI_TEXT_EXTEND_SELECTION_LINE: Selects the current line. It is triggered by
 *   a triple-click for example.
 *
 * Granularity types that extend the text selection. Use the
 * `BobguiTextView::extend-selection` signal to customize the selection.
 */
typedef enum
{
  BOBGUI_TEXT_EXTEND_SELECTION_WORD,
  BOBGUI_TEXT_EXTEND_SELECTION_LINE
} BobguiTextExtendSelection;

/**
 * BOBGUI_TEXT_VIEW_PRIORITY_VALIDATE: (value 125)
 *
 * The priority at which the text view validates onscreen lines
 * in an idle job in the background.
 */
#define BOBGUI_TEXT_VIEW_PRIORITY_VALIDATE (GDK_PRIORITY_REDRAW + 5)

typedef struct _BobguiTextView        BobguiTextView;
typedef struct _BobguiTextViewPrivate BobguiTextViewPrivate;
typedef struct _BobguiTextViewClass   BobguiTextViewClass;

struct _BobguiTextView
{
  BobguiWidget parent_instance;

  /*< private >*/

  BobguiTextViewPrivate *priv;
};

/**
 * BobguiTextViewClass:
 * @parent_class: The object class structure needs to be the first
 * @move_cursor: The class handler for the `BobguiTextView::move-cursor`
 *   keybinding signal.
 * @set_anchor: The class handler for the `BobguiTextView::set-anchor`
 *   keybinding signal.
 * @insert_at_cursor: The class handler for the `BobguiTextView::insert-at-cursor`
 *   keybinding signal.
 * @delete_from_cursor: The class handler for the `BobguiTextView::delete-from-cursor`
 *   keybinding signal.
 * @backspace: The class handler for the `BobguiTextView::backspace`
 *   keybinding signal.
 * @cut_clipboard: The class handler for the `BobguiTextView::cut-clipboard`
 *   keybinding signal
 * @copy_clipboard: The class handler for the `BobguiTextView::copy-clipboard`
 *   keybinding signal.
 * @paste_clipboard: The class handler for the `BobguiTextView::paste-clipboard`
 *   keybinding signal.
 * @toggle_overwrite: The class handler for the `BobguiTextView::toggle-overwrite`
 *   keybinding signal.
 * @create_buffer: The create_buffer vfunc is called to create a `BobguiTextBuffer`
 *   for the text view. The default implementation is to just call
 *   bobgui_text_buffer_new().
 * @snapshot_layer: The snapshot_layer vfunc is called before and after the text
 *   view is drawing its own text. Applications can override this vfunc
 *   in a subclass to draw customized content underneath or above the
 *   text. In the %BOBGUI_TEXT_VIEW_LAYER_BELOW_TEXT and %BOBGUI_TEXT_VIEW_LAYER_ABOVE_TEXT
 *   layers the drawing is done in the buffer coordinate space.
 * @extend_selection: The class handler for the `BobguiTextView::extend-selection` signal.
 * @insert_emoji: The class handler for the `BobguiTextView::insert-emoji` signal.
 */
struct _BobguiTextViewClass
{
  BobguiWidgetClass parent_class;

  /*< public >*/

  void (* move_cursor)           (BobguiTextView      *text_view,
                                  BobguiMovementStep   step,
                                  int               count,
                                  gboolean          extend_selection);
  void (* set_anchor)            (BobguiTextView      *text_view);
  void (* insert_at_cursor)      (BobguiTextView      *text_view,
                                  const char       *str);
  void (* delete_from_cursor)    (BobguiTextView      *text_view,
                                  BobguiDeleteType     type,
                                  int               count);
  void (* backspace)             (BobguiTextView      *text_view);
  void (* cut_clipboard)         (BobguiTextView      *text_view);
  void (* copy_clipboard)        (BobguiTextView      *text_view);
  void (* paste_clipboard)       (BobguiTextView      *text_view);
  void (* toggle_overwrite)      (BobguiTextView      *text_view);
  BobguiTextBuffer * (* create_buffer) (BobguiTextView   *text_view);
  void (* snapshot_layer)        (BobguiTextView      *text_view,
			          BobguiTextViewLayer  layer,
			          BobguiSnapshot      *snapshot);
  gboolean (* extend_selection)  (BobguiTextView            *text_view,
                                  BobguiTextExtendSelection  granularity,
                                  const BobguiTextIter      *location,
                                  BobguiTextIter            *start,
                                  BobguiTextIter            *end);
  void (* insert_emoji)          (BobguiTextView      *text_view);

  /*< private >*/

  gpointer padding[8];
};

GDK_AVAILABLE_IN_ALL
GType          bobgui_text_view_get_type              (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget *    bobgui_text_view_new                   (void);
GDK_AVAILABLE_IN_ALL
BobguiWidget *    bobgui_text_view_new_with_buffer       (BobguiTextBuffer *buffer);
GDK_AVAILABLE_IN_ALL
void           bobgui_text_view_set_buffer            (BobguiTextView   *text_view,
                                                    BobguiTextBuffer *buffer);
GDK_AVAILABLE_IN_ALL
BobguiTextBuffer *bobgui_text_view_get_buffer            (BobguiTextView   *text_view);
GDK_AVAILABLE_IN_ALL
gboolean       bobgui_text_view_scroll_to_iter        (BobguiTextView   *text_view,
                                                    BobguiTextIter   *iter,
                                                    double         within_margin,
                                                    gboolean       use_align,
                                                    double         xalign,
                                                    double         yalign);
GDK_AVAILABLE_IN_ALL
void           bobgui_text_view_scroll_to_mark        (BobguiTextView   *text_view,
                                                    BobguiTextMark   *mark,
                                                    double         within_margin,
                                                    gboolean       use_align,
                                                    double         xalign,
                                                    double         yalign);
GDK_AVAILABLE_IN_ALL
void           bobgui_text_view_scroll_mark_onscreen  (BobguiTextView   *text_view,
                                                    BobguiTextMark   *mark);
GDK_AVAILABLE_IN_ALL
gboolean       bobgui_text_view_move_mark_onscreen    (BobguiTextView   *text_view,
                                                    BobguiTextMark   *mark);
GDK_AVAILABLE_IN_ALL
gboolean       bobgui_text_view_place_cursor_onscreen (BobguiTextView   *text_view);

GDK_AVAILABLE_IN_ALL
void           bobgui_text_view_get_visible_rect      (BobguiTextView   *text_view,
                                                    GdkRectangle  *visible_rect);
GDK_AVAILABLE_IN_4_18
void           bobgui_text_view_get_visible_offset    (BobguiTextView   *text_view,
                                                    double        *x_offset,
                                                    double        *y_offset);
GDK_AVAILABLE_IN_ALL
void           bobgui_text_view_set_cursor_visible    (BobguiTextView   *text_view,
                                                    gboolean       setting);
GDK_AVAILABLE_IN_ALL
gboolean       bobgui_text_view_get_cursor_visible    (BobguiTextView   *text_view);

GDK_AVAILABLE_IN_ALL
void           bobgui_text_view_reset_cursor_blink    (BobguiTextView   *text_view);

GDK_AVAILABLE_IN_ALL
void           bobgui_text_view_get_cursor_locations  (BobguiTextView       *text_view,
                                                    const BobguiTextIter *iter,
                                                    GdkRectangle      *strong,
                                                    GdkRectangle      *weak);
GDK_AVAILABLE_IN_ALL
void           bobgui_text_view_get_iter_location     (BobguiTextView   *text_view,
                                                    const BobguiTextIter *iter,
                                                    GdkRectangle  *location);
GDK_AVAILABLE_IN_ALL
gboolean       bobgui_text_view_get_iter_at_location  (BobguiTextView   *text_view,
                                                    BobguiTextIter   *iter,
                                                    int            x,
                                                    int            y);
GDK_AVAILABLE_IN_ALL
gboolean       bobgui_text_view_get_iter_at_position  (BobguiTextView   *text_view,
                                                    BobguiTextIter   *iter,
						    int           *trailing,
                                                    int            x,
                                                    int            y);
GDK_AVAILABLE_IN_ALL
void           bobgui_text_view_get_line_yrange       (BobguiTextView       *text_view,
                                                    const BobguiTextIter *iter,
                                                    int               *y,
                                                    int               *height);

GDK_AVAILABLE_IN_ALL
void           bobgui_text_view_get_line_at_y         (BobguiTextView       *text_view,
                                                    BobguiTextIter       *target_iter,
                                                    int                y,
                                                    int               *line_top);

GDK_AVAILABLE_IN_ALL
void bobgui_text_view_buffer_to_window_coords (BobguiTextView       *text_view,
                                            BobguiTextWindowType  win,
                                            int                buffer_x,
                                            int                buffer_y,
                                            int               *window_x,
                                            int               *window_y);
GDK_AVAILABLE_IN_ALL
void bobgui_text_view_window_to_buffer_coords (BobguiTextView       *text_view,
                                            BobguiTextWindowType  win,
                                            int                window_x,
                                            int                window_y,
                                            int               *buffer_x,
                                            int               *buffer_y);

GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_view_forward_display_line           (BobguiTextView       *text_view,
                                                       BobguiTextIter       *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_view_backward_display_line          (BobguiTextView       *text_view,
                                                       BobguiTextIter       *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_view_forward_display_line_end       (BobguiTextView       *text_view,
                                                       BobguiTextIter       *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_view_backward_display_line_start    (BobguiTextView       *text_view,
                                                       BobguiTextIter       *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_view_starts_display_line            (BobguiTextView       *text_view,
                                                       const BobguiTextIter *iter);
GDK_AVAILABLE_IN_ALL
gboolean bobgui_text_view_move_visually                  (BobguiTextView       *text_view,
                                                       BobguiTextIter       *iter,
                                                       int                count);

GDK_AVAILABLE_IN_ALL
gboolean        bobgui_text_view_im_context_filter_keypress (BobguiTextView    *text_view,
                                                          GdkEvent       *event);
GDK_AVAILABLE_IN_ALL
void            bobgui_text_view_reset_im_context           (BobguiTextView    *text_view);

/* Adding child widgets */
GDK_AVAILABLE_IN_ALL
BobguiWidget *bobgui_text_view_get_gutter          (BobguiTextView          *text_view,
                                              BobguiTextWindowType     win);
GDK_AVAILABLE_IN_ALL
void       bobgui_text_view_set_gutter          (BobguiTextView          *text_view,
                                              BobguiTextWindowType     win,
                                              BobguiWidget            *widget);
GDK_AVAILABLE_IN_ALL
void       bobgui_text_view_add_child_at_anchor (BobguiTextView          *text_view,
                                              BobguiWidget            *child,
                                              BobguiTextChildAnchor   *anchor);

GDK_AVAILABLE_IN_ALL
void       bobgui_text_view_add_overlay         (BobguiTextView          *text_view,
                                              BobguiWidget            *child,
                                              int                   xpos,
                                              int                   ypos);

GDK_AVAILABLE_IN_ALL
void       bobgui_text_view_move_overlay        (BobguiTextView          *text_view,
                                              BobguiWidget            *child,
                                              int                   xpos,
                                              int                   ypos);

GDK_AVAILABLE_IN_ALL
void       bobgui_text_view_remove              (BobguiTextView          *text_view,
                                              BobguiWidget            *child);

/* Default style settings (fallbacks if no tag affects the property) */

GDK_AVAILABLE_IN_ALL
void             bobgui_text_view_set_wrap_mode          (BobguiTextView      *text_view,
                                                       BobguiWrapMode       wrap_mode);
GDK_AVAILABLE_IN_ALL
BobguiWrapMode      bobgui_text_view_get_wrap_mode          (BobguiTextView      *text_view);
GDK_AVAILABLE_IN_ALL
void             bobgui_text_view_set_editable           (BobguiTextView      *text_view,
                                                       gboolean          setting);
GDK_AVAILABLE_IN_ALL
gboolean         bobgui_text_view_get_editable           (BobguiTextView      *text_view);
GDK_AVAILABLE_IN_ALL
void             bobgui_text_view_set_overwrite          (BobguiTextView      *text_view,
						       gboolean          overwrite);
GDK_AVAILABLE_IN_ALL
gboolean         bobgui_text_view_get_overwrite          (BobguiTextView      *text_view);
GDK_AVAILABLE_IN_ALL
void		 bobgui_text_view_set_accepts_tab        (BobguiTextView	*text_view,
						       gboolean		 accepts_tab);
GDK_AVAILABLE_IN_ALL
gboolean	 bobgui_text_view_get_accepts_tab        (BobguiTextView	*text_view);
GDK_AVAILABLE_IN_ALL
void             bobgui_text_view_set_pixels_above_lines (BobguiTextView      *text_view,
                                                       int               pixels_above_lines);
GDK_AVAILABLE_IN_ALL
int              bobgui_text_view_get_pixels_above_lines (BobguiTextView      *text_view);
GDK_AVAILABLE_IN_ALL
void             bobgui_text_view_set_pixels_below_lines (BobguiTextView      *text_view,
                                                       int               pixels_below_lines);
GDK_AVAILABLE_IN_ALL
int              bobgui_text_view_get_pixels_below_lines (BobguiTextView      *text_view);
GDK_AVAILABLE_IN_ALL
void             bobgui_text_view_set_pixels_inside_wrap (BobguiTextView      *text_view,
                                                       int               pixels_inside_wrap);
GDK_AVAILABLE_IN_ALL
int              bobgui_text_view_get_pixels_inside_wrap (BobguiTextView      *text_view);
GDK_AVAILABLE_IN_ALL
void             bobgui_text_view_set_justification      (BobguiTextView      *text_view,
                                                       BobguiJustification  justification);
GDK_AVAILABLE_IN_ALL
BobguiJustification bobgui_text_view_get_justification      (BobguiTextView      *text_view);
GDK_AVAILABLE_IN_ALL
void             bobgui_text_view_set_left_margin        (BobguiTextView      *text_view,
                                                       int               left_margin);
GDK_AVAILABLE_IN_ALL
int              bobgui_text_view_get_left_margin        (BobguiTextView      *text_view);
GDK_AVAILABLE_IN_ALL
void             bobgui_text_view_set_right_margin       (BobguiTextView      *text_view,
                                                       int               right_margin);
GDK_AVAILABLE_IN_ALL
int              bobgui_text_view_get_right_margin       (BobguiTextView      *text_view);
GDK_AVAILABLE_IN_ALL
void             bobgui_text_view_set_top_margin         (BobguiTextView      *text_view,
                                                       int               top_margin);
GDK_AVAILABLE_IN_ALL
int              bobgui_text_view_get_top_margin         (BobguiTextView      *text_view);
GDK_AVAILABLE_IN_ALL
void             bobgui_text_view_set_bottom_margin      (BobguiTextView      *text_view,
                                                       int               bottom_margin);
GDK_AVAILABLE_IN_ALL
int              bobgui_text_view_get_bottom_margin       (BobguiTextView      *text_view);
GDK_AVAILABLE_IN_ALL
void             bobgui_text_view_set_indent             (BobguiTextView      *text_view,
                                                       int               indent);
GDK_AVAILABLE_IN_ALL
int              bobgui_text_view_get_indent             (BobguiTextView      *text_view);
GDK_AVAILABLE_IN_ALL
void             bobgui_text_view_set_tabs               (BobguiTextView      *text_view,
                                                       PangoTabArray    *tabs);
GDK_AVAILABLE_IN_ALL
PangoTabArray*   bobgui_text_view_get_tabs               (BobguiTextView      *text_view);

GDK_AVAILABLE_IN_ALL
void             bobgui_text_view_set_input_purpose      (BobguiTextView      *text_view,
                                                       BobguiInputPurpose   purpose);
GDK_AVAILABLE_IN_ALL
BobguiInputPurpose  bobgui_text_view_get_input_purpose      (BobguiTextView      *text_view);

GDK_AVAILABLE_IN_ALL
void             bobgui_text_view_set_input_hints        (BobguiTextView      *text_view,
                                                       BobguiInputHints     hints);
GDK_AVAILABLE_IN_ALL
BobguiInputHints    bobgui_text_view_get_input_hints        (BobguiTextView      *text_view);

GDK_AVAILABLE_IN_ALL
void             bobgui_text_view_set_monospace          (BobguiTextView      *text_view,
                                                       gboolean          monospace);
GDK_AVAILABLE_IN_ALL
gboolean         bobgui_text_view_get_monospace          (BobguiTextView      *text_view);

GDK_AVAILABLE_IN_ALL
void             bobgui_text_view_set_extra_menu         (BobguiTextView      *text_view,
                                                       GMenuModel       *model);
GDK_AVAILABLE_IN_ALL
GMenuModel *     bobgui_text_view_get_extra_menu         (BobguiTextView      *text_view);
GDK_AVAILABLE_IN_ALL
PangoContext    *bobgui_text_view_get_rtl_context        (BobguiTextView      *text_view);
GDK_AVAILABLE_IN_ALL
PangoContext    *bobgui_text_view_get_ltr_context        (BobguiTextView      *text_view);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiTextView, g_object_unref)

G_END_DECLS

