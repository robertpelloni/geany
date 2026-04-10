/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* BOBGUI - The Bobgui Framework
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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

#include "config.h"

#include "bobguitextprivate.h"

#include "bobguiaccessibletextprivate.h"
#include "bobguiactionable.h"
#include "bobguiactionmuxerprivate.h"
#include "bobguiadjustment.h"
#include "bobguibox.h"
#include "bobguibutton.h"
#include "bobguidebug.h"
#include "bobguidragicon.h"
#include "bobguidragsourceprivate.h"
#include "bobguidroptarget.h"
#include "bobguieditable.h"
#include "bobguiemojichooser.h"
#include "bobguiemojicompletion.h"
#include "bobguientrybuffer.h"
#include "bobguieventcontrollerfocus.h"
#include "bobguieventcontrollerkey.h"
#include "bobguieventcontrollermotion.h"
#include "bobguigesturedrag.h"
#include "bobguigestureclick.h"
#include "bobguigesturesingle.h"
#include "bobguiimageprivate.h"
#include "bobguiimcontextsimple.h"
#include "bobguiimmulticontext.h"
#include "bobguijoinedmenuprivate.h"
#include "bobguilabel.h"
#include "bobguimagnifierprivate.h"
#include "bobguimain.h"
#include "bobguimarshalers.h"
#include "bobguinative.h"
#include "bobguipangoprivate.h"
#include "bobguipopovermenu.h"
#include "bobguiprivate.h"
#include "bobguirenderbackgroundprivate.h"
#include "bobguirenderborderprivate.h"
#include "bobguirenderlayoutprivate.h"
#include "bobguisettings.h"
#include "bobguisnapshot.h"
#include "bobguitexthandleprivate.h"
#include "bobguitexthistoryprivate.h"
#include "bobguitextutilprivate.h"
#include "bobguitooltip.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"
#include "bobguiwindow.h"

#include "deprecated/bobguirender.h"
#include "a11y/bobguiatspipangoprivate.h"

#include <string.h>
#include <cairo-gobject.h>
#include <glib/gi18n-lib.h>

/**
 * BobguiText:
 *
 * A single-line text entry.
 *
 * `BobguiText` is the common implementation of single-line text editing
 * that is shared between [class@Bobgui.Entry], [class@Bobgui.PasswordEntry],
 * [class@Bobgui.SpinButton], and other widgets. In all of these, a `BobguiText`
 * instance is used as the delegate for the [iface@Bobgui.Editable] implementation.
 *
 * A large number of key bindings s supported by default. If the entered
 * text is longer than the allocation of the widget, the widget will scroll
 * so that the cursor position is visible.
 *
 * When using an entry for passwords and other sensitive information,
 * it can be put into “password mode” using [method@Bobgui.Text.set_visibility].
 * In this mode, entered text is displayed using an “invisible” character.
 * By default, BOBGUI picks the best invisible character that is available
 * in the current font, but it can be changed with
 * [method@Bobgui.Text.set_invisible_char].
 *
 * If you want to add icons or progress display in an entry, look at
 * [class@Bobgui.Entry]. There are other alternatives for more specialized
 * use cases, such as [class@Bobgui.SearchEntry].
 *
 * If you need multi-line editable text, use [class@Bobgui.TextView].
 *
 * # Shortcuts and Gestures
 *
 * `BobguiText` supports the following keyboard shortcuts:
 *
 * - <kbd>Shift</kbd>+<kbd>F10</kbd> or <kbd>Menu</kbd> opens the context menu.
 * - <kbd>Ctrl</kbd>+<kbd>A</kbd> or <kbd>Ctrl</kbd>+<kbd>&sol;</kbd>
 *   selects all the text.
 * - <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>A</kbd> or
 *   <kbd>Ctrl</kbd>+<kbd>&bsol;</kbd> unselects all.
 * - <kbd>Ctrl</kbd>+<kbd>Z</kbd> undoes the last modification.
 * - <kbd>Ctrl</kbd>+<kbd>Y</kbd> or <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>Z</kbd>
 *   redoes the last undone modification.
 * - <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>T</kbd> toggles the text direction.
 * - <kbd>Clear</kbd> clears the content.
 *
 * Additionally, the following signals have default keybindings:
 *
 * - [signal@Bobgui.Text::activate]
 * - [signal@Bobgui.Text::backspace]
 * - [signal@Bobgui.Text::copy-clipboard]
 * - [signal@Bobgui.Text::cut-clipboard]
 * - [signal@Bobgui.Text::delete-from-cursor]
 * - [signal@Bobgui.Text::insert-emoji]
 * - [signal@Bobgui.Text::move-cursor]
 * - [signal@Bobgui.Text::paste-clipboard]
 * - [signal@Bobgui.Text::toggle-overwrite]
 *
 * # Actions
 *
 * `BobguiText` defines a set of built-in actions:
 *
 * - `clipboard.copy` copies the contents to the clipboard.
 * - `clipboard.cut` copies the contents to the clipboard and deletes it from
 *   the widget.
 * - `clipboard.paste` inserts the contents of the clipboard into the widget.
 * - `menu.popup` opens the context menu.
 * - `misc.insert-emoji` opens the Emoji chooser.
 * - `misc.toggle-visibility` toggles the `BobguiText`:visibility property.
 * - `misc.toggle-direction` toggles the text direction.
 * - `selection.delete` deletes the current selection.
 * - `selection.select-all` selects all of the widgets content.
 * - `text.redo` redoes the last change to the contents.
 * - `text.undo` undoes the last change to the contents.
 * - `text.clear` removes all content.
 *
 * # CSS nodes
 *
 * ```
 * text[.read-only]
 * ├── placeholder
 * ├── undershoot.left
 * ├── undershoot.right
 * ├── [selection]
 * ├── [cursor-handle[.top]
 * ├── [cursor-handle.bottom]
 * ├── [block-cursor]
 * ├── [cursor-handle[.top/.bottom][.insertion-cursor]]
 * ╰── [window.popup]
 * ```
 *
 * `BobguiText` has a main node with the name `text`. Depending on the properties
 * of the widget, the `.read-only` style class may appear.
 *
 * When the entry has a selection, it adds a subnode with the name `selection`.
 *
 * When the entry is in overwrite mode, it adds a subnode with the name
 * `block-cursor` that determines how the block cursor is drawn.
 *
 * The CSS node for a context menu is added as a subnode with the name `popup`.
 *
 * The `undershoot` nodes are used to draw the underflow indication when content
 * is scrolled out of view. These nodes get the `.left` or `.right` style class
 * added depending on where the indication is drawn.
 *
 * When touch is used and touch selection handles are shown, they are using
 * CSS nodes with name `cursor-handle`. They get the `.top` or `.bottom` style
 * class depending on where they are shown in relation to the selection. If
 * there is just a single handle for the text cursor, it gets the style class
 * `.insertion-cursor`.
 *
 * # Accessibility
 *
 * `BobguiText` uses the [enum@Bobgui.AccessibleRole.none] role, which causes it to be
 * skipped for accessibility. This is because `BobguiText` is expected to be used
 * as a delegate for a `BobguiEditable` implementation that will be represented
 * to accessibility.
 */

#define NAT_ENTRY_WIDTH  150

#define UNDERSHOOT_SIZE 20

#define DEFAULT_MAX_UNDO 200

static GQuark          quark_password_hint  = 0;

enum
{
  TEXT_HANDLE_CURSOR,
  TEXT_HANDLE_SELECTION_BOUND,
  TEXT_HANDLE_N_HANDLES
};

typedef struct _BobguiTextPasswordHint BobguiTextPasswordHint;

typedef struct _BobguiTextPrivate BobguiTextPrivate;
struct _BobguiTextPrivate
{
  BobguiEntryBuffer *buffer;
  BobguiIMContext   *im_context;

  int             text_baseline;

  PangoLayout    *cached_layout;
  PangoAttrList  *attrs;
  PangoTabArray  *tabs;

  GdkContentProvider *selection_content;

  char         *im_module;

  BobguiWidget     *emoji_completion;
  BobguiTextHandle *text_handles[TEXT_HANDLE_N_HANDLES];
  BobguiWidget     *selection_bubble;
  guint          selection_bubble_timeout_id;

  BobguiWidget     *magnifier_popover;
  BobguiWidget     *magnifier;

  BobguiWidget     *placeholder;

  BobguiGesture    *drag_gesture;
  BobguiEventController *key_controller;
  BobguiEventController *focus_controller;

  BobguiCssNode    *selection_node;
  BobguiCssNode    *block_cursor_node;
  BobguiCssNode    *undershoot_node[2];

  BobguiWidget     *popup_menu;
  GMenuModel    *extra_menu;

  BobguiTextHistory *history;

  GdkDrag       *drag;

  float         xalign;

  int           ascent;                     /* font ascent in pango units  */
  int           current_pos;
  int           descent;                    /* font descent in pango units */
  int           dnd_position;               /* In chars, -1 == no DND cursor */
  int           drag_start_x;
  int           drag_start_y;
  int           insert_pos;
  int           selection_bound;
  int           scroll_offset;
  int           width_chars;
  int           max_width_chars;
  guint32       obscured_cursor_timestamp;

  gunichar      invisible_char;

  guint64       blink_start_time;
  guint         blink_tick;
  float         cursor_alpha;

  guint16       preedit_length;              /* length of preedit string, in bytes */
  guint16       preedit_cursor;              /* offset of cursor within preedit string, in chars */

  gint64        handle_place_time;

  guint         editable                : 1;
  guint         enable_emoji_completion : 1;
  guint         in_drag                 : 1;
  guint         overwrite_mode          : 1;
  guint         visible                 : 1;

  guint         activates_default       : 1;
  guint         cache_includes_preedit  : 1;
  guint         change_count            : 8;
  guint         in_click                : 1; /* Flag so we don't select all when clicking in entry to focus in */
  guint         invisible_char_set      : 1;
  guint         mouse_cursor_obscured   : 1;
  guint         need_im_reset           : 1;
  guint         real_changed            : 1;
  guint         resolved_dir            : 4; /* PangoDirection */
  guint         select_words            : 1;
  guint         select_lines            : 1;
  guint         truncate_multiline      : 1;
  guint         cursor_handle_dragged   : 1;
  guint         selection_handle_dragged : 1;
  guint         populate_all            : 1;
  guint         propagate_text_width    : 1;
  guint         text_handles_enabled    : 1;
  guint         enable_undo             : 1;
};

struct _BobguiTextPasswordHint
{
  int position;      /* Position (in text) of the last password hint */
  guint source_id;    /* Timeout source id */
};

enum {
  ACTIVATE,
  MOVE_CURSOR,
  INSERT_AT_CURSOR,
  DELETE_FROM_CURSOR,
  BACKSPACE,
  CUT_CLIPBOARD,
  COPY_CLIPBOARD,
  PASTE_CLIPBOARD,
  TOGGLE_OVERWRITE,
  PREEDIT_CHANGED,
  INSERT_EMOJI,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_BUFFER,
  PROP_MAX_LENGTH,
  PROP_VISIBILITY,
  PROP_INVISIBLE_CHAR,
  PROP_INVISIBLE_CHAR_SET,
  PROP_ACTIVATES_DEFAULT,
  PROP_SCROLL_OFFSET,
  PROP_TRUNCATE_MULTILINE,
  PROP_OVERWRITE_MODE,
  PROP_IM_MODULE,
  PROP_PLACEHOLDER_TEXT,
  PROP_INPUT_PURPOSE,
  PROP_INPUT_HINTS,
  PROP_ATTRIBUTES,
  PROP_TABS,
  PROP_ENABLE_EMOJI_COMPLETION,
  PROP_PROPAGATE_TEXT_WIDTH,
  PROP_EXTRA_MENU,
  NUM_PROPERTIES
};

static GParamSpec *text_props[NUM_PROPERTIES] = { NULL, };

static guint signals[LAST_SIGNAL] = { 0 };

typedef enum {
  CURSOR_STANDARD,
  CURSOR_DND
} CursorType;

typedef enum
{
  DISPLAY_NORMAL,       /* The text is being shown */
  DISPLAY_INVISIBLE,    /* In invisible mode, text replaced by (eg) bullets */
  DISPLAY_BLANK         /* In invisible mode, nothing shown at all */
} DisplayMode;

/* GObject methods
 */
static void   bobgui_text_set_property         (GObject      *object,
                                             guint         prop_id,
                                             const GValue *value,
                                             GParamSpec   *pspec);
static void   bobgui_text_get_property         (GObject      *object,
                                             guint         prop_id,
                                             GValue       *value,
                                             GParamSpec   *pspec);
static void   bobgui_text_notify               (GObject      *object,
                                             GParamSpec   *pspec);
static void   bobgui_text_finalize             (GObject      *object);
static void   bobgui_text_dispose              (GObject      *object);

/* BobguiWidget methods
 */
static void   bobgui_text_realize              (BobguiWidget        *widget);
static void   bobgui_text_unrealize            (BobguiWidget        *widget);
static void   bobgui_text_map                  (BobguiWidget        *widget);
static void   bobgui_text_unmap                (BobguiWidget        *widget);
static void   bobgui_text_measure              (BobguiWidget        *widget,
                                             BobguiOrientation    orientation,
                                             int               for_size,
                                             int              *minimum,
                                             int              *natural,
                                             int              *minimum_baseline,
                                             int              *natural_baseline);
static void   bobgui_text_size_allocate        (BobguiWidget        *widget,
                                             int               width,
                                             int               height,
                                             int               baseline);
static void   bobgui_text_snapshot             (BobguiWidget        *widget,
                                             BobguiSnapshot      *snapshot);
static void   bobgui_text_focus_changed        (BobguiEventControllerFocus *focus,
                                             GParamSpec              *pspec,
                                             BobguiWidget               *widget);
static gboolean bobgui_text_grab_focus         (BobguiWidget        *widget);
static void   bobgui_text_css_changed          (BobguiWidget        *widget,
                                             BobguiCssStyleChange *change);
static void   bobgui_text_direction_changed    (BobguiWidget        *widget,
                                             BobguiTextDirection  previous_dir);
static void   bobgui_text_state_flags_changed  (BobguiWidget        *widget,
                                             BobguiStateFlags     previous_state);

static gboolean bobgui_text_drag_drop          (BobguiDropTarget    *dest,
                                             const GValue     *value,
                                             double            x,
                                             double            y,
                                             BobguiText          *text);
static gboolean bobgui_text_drag_accept        (BobguiDropTarget    *dest,
                                             GdkDrop          *drop,
                                             BobguiText          *self);
static GdkDragAction bobgui_text_drag_motion   (BobguiDropTarget    *dest,
                                             double            x,
                                             double            y,
                                             BobguiText          *text);
static void     bobgui_text_drag_leave         (BobguiDropTarget    *dest,
                                             BobguiText          *text);


/* BobguiEditable method implementations
 */
static void        bobgui_text_editable_init        (BobguiEditableInterface *iface);
static void        bobgui_text_insert_text          (BobguiText    *self,
                                                  const char *text,
                                                  int         length,
                                                  int        *position);
static void        bobgui_text_delete_text          (BobguiText    *self,
                                                  int         start_pos,
                                                  int         end_pos);
static void        bobgui_text_delete_selection     (BobguiText    *self);
static void        bobgui_text_set_selection_bounds (BobguiText    *self,
                                                  int         start,
                                                  int         end);
static gboolean    bobgui_text_get_selection_bounds (BobguiText    *self,
                                                  int        *start,
                                                  int        *end);

static void        bobgui_text_set_editable         (BobguiText    *self,
                                                  gboolean    is_editable);
static void        bobgui_text_set_text             (BobguiText    *self,
                                                  const char *text);
static void        bobgui_text_set_width_chars      (BobguiText    *self,
                                                  int         n_chars);
static void        bobgui_text_set_max_width_chars  (BobguiText    *self,
                                                  int         n_chars);
static void        bobgui_text_set_alignment        (BobguiText    *self,
                                                  float       xalign);

static void        bobgui_text_set_enable_undo      (BobguiText    *self,
                                                  gboolean    enable_undo);

/* Default signal handlers
 */
static GMenuModel *bobgui_text_get_menu_model  (BobguiText         *self);
static void     bobgui_text_popup_menu         (BobguiWidget       *widget,
                                             const char      *action_name,
                                             GVariant        *parameters);
static void     bobgui_text_move_cursor        (BobguiText         *self,
                                             BobguiMovementStep  step,
                                             int              count,
                                             gboolean         extend);
static void     bobgui_text_insert_at_cursor   (BobguiText         *self,
                                             const char      *str);
static void     bobgui_text_delete_from_cursor (BobguiText         *self,
                                             BobguiDeleteType    type,
                                             int              count);
static void     bobgui_text_backspace          (BobguiText         *self);
static void     bobgui_text_cut_clipboard      (BobguiText         *self);
static void     bobgui_text_copy_clipboard     (BobguiText         *self);
static void     bobgui_text_paste_clipboard    (BobguiText         *self);
static void     bobgui_text_toggle_overwrite   (BobguiText         *self);
static void     bobgui_text_insert_emoji       (BobguiText         *self);
static gboolean bobgui_text_select_all         (BobguiText         *self);
static void     bobgui_text_real_activate      (BobguiText         *self);

/* IM Context Callbacks
 */
static void     bobgui_text_commit_cb               (BobguiIMContext *context,
                                                  const char   *str,
                                                  BobguiText      *self);
static void     bobgui_text_preedit_start_cb        (BobguiIMContext *context,
                                                  BobguiText      *self);
static void     bobgui_text_preedit_changed_cb      (BobguiIMContext *context,
                                                  BobguiText      *self);
static gboolean bobgui_text_retrieve_surrounding_cb (BobguiIMContext *context,
                                                  BobguiText      *self);
static gboolean bobgui_text_delete_surrounding_cb   (BobguiIMContext *context,
                                                  int           offset,
                                                  int           n_chars,
                                                  BobguiText      *self);

/* Entry buffer signal handlers
 */
static void buffer_inserted_text     (BobguiEntryBuffer *buffer,
                                      guint           position,
                                      const char     *chars,
                                      guint           n_chars,
                                      BobguiText        *self);
static void buffer_deleted_text      (BobguiEntryBuffer *buffer,
                                      guint           position,
                                      guint           n_chars,
                                      BobguiText        *self);
static void buffer_notify_text       (BobguiEntryBuffer *buffer,
                                      GParamSpec     *spec,
                                      BobguiText        *self);
static void buffer_notify_max_length (BobguiEntryBuffer *buffer,
                                      GParamSpec     *spec,
                                      BobguiText        *self);

/* Event controller callbacks
 */
static void     bobgui_text_motion_controller_motion   (BobguiEventControllerMotion *controller,
                                                     double                    x,
                                                     double                    y,
                                                     BobguiText                  *self);
static void     bobgui_text_click_gesture_pressed (BobguiGestureClick     *gesture,
                                                int                       n_press,
                                                double                    x,
                                                double                    y,
                                                BobguiText                  *self);
static void     bobgui_text_click_gesture_released (BobguiGestureClick     *gesture,
                                                 int                       n_press,
                                                 double                    x,
                                                 double                    y,
                                                 BobguiText                  *self);
static void     bobgui_text_drag_gesture_update        (BobguiGestureDrag           *gesture,
                                                     double                    offset_x,
                                                     double                    offset_y,
                                                     BobguiText                  *self);
static void     bobgui_text_drag_gesture_end           (BobguiGestureDrag           *gesture,
                                                     double                     offset_x,
                                                     double                    offset_y,
                                                     BobguiText                  *self);
static gboolean bobgui_text_key_controller_key_pressed  (BobguiEventControllerKey   *controller,
                                                      guint                    keyval,
                                                      guint                    keycode,
                                                      GdkModifierType          state,
                                                      BobguiText                 *self);


/* BobguiTextHandle handlers */
static void bobgui_text_handle_drag_started  (BobguiTextHandle          *handle,
                                           BobguiText                *self);
static void bobgui_text_handle_dragged       (BobguiTextHandle          *handle,
                                           int                     x,
                                           int                     y,
                                           BobguiText                *self);
static void bobgui_text_handle_drag_finished (BobguiTextHandle          *handle,
                                           BobguiText                *self);

/* Internal routines
 */
static void         bobgui_text_draw_text                (BobguiText       *self,
                                                       BobguiSnapshot   *snapshot);
static void         bobgui_text_draw_cursor              (BobguiText       *self,
                                                       BobguiSnapshot   *snapshot,
                                                       CursorType     type);
static PangoLayout *bobgui_text_ensure_layout            (BobguiText       *self,
                                                       gboolean       include_preedit);
static void         bobgui_text_reset_layout             (BobguiText       *self);
static void         bobgui_text_recompute                (BobguiText       *self);
static int          bobgui_text_find_position            (BobguiText       *self,
                                                       int            x);
static void         bobgui_text_get_cursor_locations     (BobguiText       *self,
                                                       int           *strong_x,
                                                       int           *weak_x);
static void         bobgui_text_adjust_scroll            (BobguiText       *self);
static int          bobgui_text_move_visually            (BobguiText        *editable,
                                                       int             start,
                                                       int             count);
static int          bobgui_text_move_logically           (BobguiText        *self,
                                                       int             start,
                                                       int             count);
static int          bobgui_text_move_forward_word        (BobguiText        *self,
                                                       int             start,
                                                       gboolean        allow_whitespace);
static int          bobgui_text_move_backward_word       (BobguiText        *self,
                                                       int             start,
                                                       gboolean        allow_whitespace);
static void         bobgui_text_delete_whitespace        (BobguiText        *self);
static void         bobgui_text_select_word              (BobguiText        *self);
static void         bobgui_text_select_line              (BobguiText        *self);
static void         bobgui_text_paste                    (BobguiText        *self,
                                                       GdkClipboard   *clipboard);
static void         bobgui_text_update_primary_selection (BobguiText        *self);
static void         bobgui_text_schedule_im_reset        (BobguiText        *self);
static gboolean     bobgui_text_mnemonic_activate        (BobguiWidget      *widget,
                                                       gboolean        group_cycling);
static void         bobgui_text_check_cursor_blink       (BobguiText        *self);
static void         remove_blink_timeout              (BobguiText        *self);
static void         bobgui_text_pend_cursor_blink        (BobguiText        *self);
static void         bobgui_text_reset_blink_time         (BobguiText        *self);
static void         bobgui_text_update_cached_style_values(BobguiText       *self);
static gboolean     get_middle_click_paste             (BobguiText       *self);
static void         bobgui_text_get_scroll_limits        (BobguiText        *self,
                                                       int            *min_offset,
                                                       int            *max_offset);
static BobguiEntryBuffer *get_buffer                      (BobguiText       *self);
static void         set_text_cursor                    (BobguiWidget     *widget);
static void         update_placeholder_visibility      (BobguiText       *self);

static void         buffer_connect_signals             (BobguiText       *self);
static void         buffer_disconnect_signals          (BobguiText       *self);

static void         bobgui_text_selection_bubble_popup_set   (BobguiText *self);
static void         bobgui_text_selection_bubble_popup_unset (BobguiText *self);

static void         begin_change                       (BobguiText *self);
static void         end_change                         (BobguiText *self);
static void         emit_changed                       (BobguiText *self);

static void         bobgui_text_update_history           (BobguiText *self);
static void         bobgui_text_update_clipboard_actions (BobguiText *self);
static void         bobgui_text_update_emoji_action      (BobguiText *self);
static void         bobgui_text_update_clear_action      (BobguiText *self);
static void         bobgui_text_update_handles           (BobguiText *self);

static void bobgui_text_activate_clipboard_cut          (BobguiWidget  *widget,
                                                      const char *action_name,
                                                      GVariant   *parameter);
static void bobgui_text_activate_clipboard_copy         (BobguiWidget  *widget,
                                                      const char *action_name,
                                                      GVariant   *parameter);
static void bobgui_text_activate_clipboard_paste        (BobguiWidget  *widget,
                                                      const char *action_name,
                                                      GVariant   *parameter);
static void bobgui_text_activate_selection_delete       (BobguiWidget  *widget,
                                                      const char *action_name,
                                                      GVariant   *parameter);
static void bobgui_text_activate_selection_select_all   (BobguiWidget  *widget,
                                                      const char *action_name,
                                                      GVariant   *parameter);
static void bobgui_text_clear                           (BobguiWidget  *widget,
                                                      const char *action_name,
                                                      GVariant   *parameter);
static void bobgui_text_activate_misc_toggle_direction  (BobguiWidget  *widget,
                                                      const char *action_name,
                                                      GVariant   *parameter);
static void bobgui_text_activate_misc_insert_emoji      (BobguiWidget  *widget,
                                                      const char *action_name,
                                                      GVariant   *parameter);
static void bobgui_text_real_undo                       (BobguiWidget  *widget,
                                                      const char *action_name,
                                                      GVariant   *parameters);
static void bobgui_text_real_redo                       (BobguiWidget  *widget,
                                                      const char *action_name,
                                                      GVariant   *parameters);
static void bobgui_text_history_change_state_cb         (gpointer    funcs_data,
                                                      gboolean    is_modified,
                                                      gboolean    can_undo,
                                                      gboolean    can_redo);
static void bobgui_text_history_insert_cb               (gpointer    funcs_data,
                                                      guint       begin,
                                                      guint       end,
                                                      const char *text,
                                                      guint       len);
static void bobgui_text_history_delete_cb               (gpointer    funcs_data,
                                                      guint       begin,
                                                      guint       end,
                                                      const char *expected_text,
                                                      guint       len);
static void bobgui_text_history_select_cb               (gpointer    funcs_data,
                                                      int         selection_insert,
                                                      int         selection_bound);

/* BobguiTextContent implementation
 */
#define BOBGUI_TYPE_TEXT_CONTENT            (bobgui_text_content_get_type ())
#define BOBGUI_TEXT_CONTENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TEXT_CONTENT, BobguiTextContent))
#define BOBGUI_IS_TEXT_CONTENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TEXT_CONTENT))
#define BOBGUI_TEXT_CONTENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_TEXT_CONTENT, BobguiTextContentClass))
#define BOBGUI_IS_TEXT_CONTENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_TEXT_CONTENT))
#define BOBGUI_TEXT_CONTENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_TEXT_CONTENT, BobguiTextContentClass))

typedef struct _BobguiTextContent BobguiTextContent;
typedef struct _BobguiTextContentClass BobguiTextContentClass;

struct _BobguiTextContent
{
  GdkContentProvider parent;

  BobguiText *self;
};

struct _BobguiTextContentClass
{
  GdkContentProviderClass parent_class;
};

GType bobgui_text_content_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE (BobguiTextContent, bobgui_text_content, GDK_TYPE_CONTENT_PROVIDER)

static GdkContentFormats *
bobgui_text_content_ref_formats (GdkContentProvider *provider)
{
  return gdk_content_formats_new_for_gtype (G_TYPE_STRING);
}

static gboolean
bobgui_text_content_get_value (GdkContentProvider  *provider,
                            GValue              *value,
                            GError             **error)
{
  BobguiTextContent *content = BOBGUI_TEXT_CONTENT (provider);

  if (G_VALUE_HOLDS (value, G_TYPE_STRING))
    {
      int start, end;

      if (bobgui_text_get_selection_bounds (content->self, &start, &end))
        {
          char *str = bobgui_text_get_display_text (content->self, start, end);
          g_value_take_string (value, str);
        }
      return TRUE;
    }

  return GDK_CONTENT_PROVIDER_CLASS (bobgui_text_content_parent_class)->get_value (provider, value, error);
}

static void
bobgui_text_content_detach (GdkContentProvider *provider,
                          GdkClipboard       *clipboard)
{
  BobguiTextContent *content = BOBGUI_TEXT_CONTENT (provider);
  int current_pos, selection_bound;

  bobgui_text_get_selection_bounds (content->self, &current_pos, &selection_bound);
  bobgui_text_set_selection_bounds (content->self, current_pos, current_pos);
}

static void
bobgui_text_content_class_init (BobguiTextContentClass *class)
{
  GdkContentProviderClass *provider_class = GDK_CONTENT_PROVIDER_CLASS (class);

  provider_class->ref_formats = bobgui_text_content_ref_formats;
  provider_class->get_value = bobgui_text_content_get_value;
  provider_class->detach_clipboard = bobgui_text_content_detach;
}

static void
bobgui_text_content_init (BobguiTextContent *content)
{
}

/* BobguiText
 */

static const BobguiTextHistoryFuncs history_funcs = {
  bobgui_text_history_change_state_cb,
  bobgui_text_history_insert_cb,
  bobgui_text_history_delete_cb,
  bobgui_text_history_select_cb,
};

static void     bobgui_text_accessible_text_init (BobguiAccessibleTextInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiText, bobgui_text, BOBGUI_TYPE_WIDGET,
                         G_ADD_PRIVATE (BobguiText)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_EDITABLE, bobgui_text_editable_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACCESSIBLE_TEXT,
                                                bobgui_text_accessible_text_init))


static void
add_move_binding (BobguiWidgetClass *widget_class,
                  guint           keyval,
                  guint           modmask,
                  BobguiMovementStep step,
                  int             count)
{
  g_return_if_fail ((modmask & GDK_SHIFT_MASK) == 0);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       keyval, modmask,
                                       "move-cursor",
                                       "(iib)", step, count, FALSE);
  /* Selection-extending version */
  bobgui_widget_class_add_binding_signal (widget_class,
                                       keyval, modmask | GDK_SHIFT_MASK,
                                       "move-cursor",
                                       "(iib)", step, count, TRUE);
}

static void
bobgui_text_class_init (BobguiTextClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  gobject_class->dispose = bobgui_text_dispose;
  gobject_class->finalize = bobgui_text_finalize;
  gobject_class->set_property = bobgui_text_set_property;
  gobject_class->get_property = bobgui_text_get_property;
  gobject_class->notify = bobgui_text_notify;

  widget_class->map = bobgui_text_map;
  widget_class->unmap = bobgui_text_unmap;
  widget_class->realize = bobgui_text_realize;
  widget_class->unrealize = bobgui_text_unrealize;
  widget_class->measure = bobgui_text_measure;
  widget_class->size_allocate = bobgui_text_size_allocate;
  widget_class->snapshot = bobgui_text_snapshot;
  widget_class->grab_focus = bobgui_text_grab_focus;
  widget_class->css_changed = bobgui_text_css_changed;
  widget_class->direction_changed = bobgui_text_direction_changed;
  widget_class->state_flags_changed = bobgui_text_state_flags_changed;
  widget_class->mnemonic_activate = bobgui_text_mnemonic_activate;

  class->move_cursor = bobgui_text_move_cursor;
  class->insert_at_cursor = bobgui_text_insert_at_cursor;
  class->delete_from_cursor = bobgui_text_delete_from_cursor;
  class->backspace = bobgui_text_backspace;
  class->cut_clipboard = bobgui_text_cut_clipboard;
  class->copy_clipboard = bobgui_text_copy_clipboard;
  class->paste_clipboard = bobgui_text_paste_clipboard;
  class->toggle_overwrite = bobgui_text_toggle_overwrite;
  class->insert_emoji = bobgui_text_insert_emoji;
  class->activate = bobgui_text_real_activate;

  quark_password_hint = g_quark_from_static_string ("bobgui-entry-password-hint");

  /**
   * BobguiText:buffer:
   *
   * The `BobguiEntryBuffer` object which stores the text.
   */
  text_props[PROP_BUFFER] =
      g_param_spec_object ("buffer", NULL, NULL,
                           BOBGUI_TYPE_ENTRY_BUFFER,
                           BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiText:max-length:
   *
   * Maximum number of characters that are allowed.
   *
   * Zero indicates no limit.
   */
  text_props[PROP_MAX_LENGTH] =
      g_param_spec_int ("max-length", NULL, NULL,
                        0, BOBGUI_ENTRY_BUFFER_MAX_SIZE,
                        0,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiText:invisible-char:
   *
   * The character to used when masking contents (in “password mode”).
   */
  text_props[PROP_INVISIBLE_CHAR] =
      g_param_spec_unichar ("invisible-char", NULL, NULL,
                            '*',
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiText:activates-default:
   *
   * Whether to activate the default widget when <kbd>Enter</kbd> is pressed.
   */
  text_props[PROP_ACTIVATES_DEFAULT] =
      g_param_spec_boolean ("activates-default", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiText:scroll-offset:
   *
   * Number of pixels scrolled of the screen to the left.
   */
  text_props[PROP_SCROLL_OFFSET] =
      g_param_spec_int ("scroll-offset", NULL, NULL,
                        0, G_MAXINT,
                        0,
                        BOBGUI_PARAM_READABLE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiText:truncate-multiline:
   *
   * When true, pasted multi-line text is truncated to the first line.
   */
  text_props[PROP_TRUNCATE_MULTILINE] =
      g_param_spec_boolean ("truncate-multiline", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiText:overwrite-mode:
   *
   * If text is overwritten when typing.
   */
  text_props[PROP_OVERWRITE_MODE] =
      g_param_spec_boolean ("overwrite-mode", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiText:invisible-char-set:
   *
   * Whether the invisible char has been set.
   */
  text_props[PROP_INVISIBLE_CHAR_SET] =
      g_param_spec_boolean ("invisible-char-set", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE);

  /**
  * BobguiText:placeholder-text:
  *
  * The text that will be displayed in the `BobguiText` when it is empty
  * and unfocused.
  */
  text_props[PROP_PLACEHOLDER_TEXT] =
      g_param_spec_string ("placeholder-text", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiText:im-module:
   *
   * Which input method module should be used.
   *
   * See [class@Bobgui.IMMulticontext].
   *
   * Setting this to a non-`NULL` value overrides the system-wide
   * input method. See the [property@Bobgui.Settings:bobgui-im-module]
   * setting.
   */
  text_props[PROP_IM_MODULE] =
      g_param_spec_string ("im-module", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiText:input-purpose:
   *
   * The purpose of this text field.
   *
   * This information can be used by on-screen keyboards and other input
   * methods to adjust their behaviour.
   *
   * Note that setting the purpose to [enum@Bobgui.InputPurpose.password]
   * or [enum@Bobgui.InputPurpose.pin] is independent from setting
   * [property@Bobgui.Text:visibility].
   */
  text_props[PROP_INPUT_PURPOSE] =
      g_param_spec_enum ("input-purpose", NULL, NULL,
                         BOBGUI_TYPE_INPUT_PURPOSE,
                         BOBGUI_INPUT_PURPOSE_FREE_FORM,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiText:input-hints:
   *
   * Additional hints that allow input methods to fine-tune
   * their behaviour.
   */
  text_props[PROP_INPUT_HINTS] =
      g_param_spec_flags ("input-hints", NULL, NULL,
                          BOBGUI_TYPE_INPUT_HINTS,
                          BOBGUI_INPUT_HINT_NONE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiText:attributes:
   *
   * A list of Pango attributes to apply to the text.
   *
   * This is mainly useful to change the size or weight of the text.
   *
   * The `PangoAttribute`'s @start_index and @end_index must refer to the
   * `BobguiEntryBuffer` text, i.e. without the preedit string.
   */
  text_props[PROP_ATTRIBUTES] =
      g_param_spec_boxed ("attributes", NULL, NULL,
                          PANGO_TYPE_ATTR_LIST,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiText:tabs:
   *
   * Custom tabs for this text widget.
   */
  text_props[PROP_TABS] =
      g_param_spec_boxed ("tabs", NULL, NULL,
                          PANGO_TYPE_TAB_ARRAY,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiText:enable-emoji-completion:
   *
   * Whether to suggest Emoji replacements.
   */
  text_props[PROP_ENABLE_EMOJI_COMPLETION] =
      g_param_spec_boolean ("enable-emoji-completion", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiText:visibility:
   *
   * If false, the text is masked with the “invisible char”.
   */
  text_props[PROP_VISIBILITY] =
      g_param_spec_boolean ("visibility", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiText:propagate-text-width:
   *
   * Whether the widget should grow and shrink with the content.
   */
  text_props[PROP_PROPAGATE_TEXT_WIDTH] =
      g_param_spec_boolean ("propagate-text-width", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiText:extra-menu:
   *
   * A menu model whose contents will be appended to the context menu.
   */
  text_props[PROP_EXTRA_MENU] =
      g_param_spec_object ("extra-menu", NULL, NULL,
                          G_TYPE_MENU_MODEL,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, text_props);

  bobgui_editable_install_properties (gobject_class, NUM_PROPERTIES);

 /* Action signals */

  /**
   * BobguiText::activate:
   * @self: the text widget which emitted the signal
   *
   * Emitted when the user hits the <kbd>Enter</kbd> key.
   *
   * The default bindings for this signal are all forms
   * of the <kbd>Enter</kbd> key.
   */
  signals[ACTIVATE] =
    g_signal_new (I_("activate"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiTextClass, activate),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiText::move-cursor:
   * @self: the text widget which emitted the signal
   * @step: the granularity of the move
   * @count: the number of @step units to move
   * @extend: true if the move should extend the selection
   *
   * Emitted when the user initiates a cursor movement.
   *
   * If the cursor is not visible in @self, this signal causes
   * the viewport to be moved instead.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * Applications should not connect to it, but may emit it with
   * [func@GObject.signal_emit_by_name] if they need to control
   * the cursor programmatically.
   *
   * The default bindings for this signal come in two variants,
   * the variant with the <kbd>Shift</kbd> modifier extends the
   * selection, the variant without it does not.
   * There are too many key combinations to list them all here.
   *
   * - <kbd>←</kbd>, <kbd>→</kbd>, <kbd>↑</kbd>, <kbd>↓</kbd>
   *   move by individual characters/lines
   * - <kbd>Ctrl</kbd>+<kbd>←</kbd>, etc. move by words/paragraphs
   * - <kbd>Home</kbd> and <kbd>End</kbd> move to the ends of the buffer
   */
  signals[MOVE_CURSOR] =
    g_signal_new (I_("move-cursor"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiTextClass, move_cursor),
                  NULL, NULL,
                  _bobgui_marshal_VOID__ENUM_INT_BOOLEAN,
                  G_TYPE_NONE, 3,
                  BOBGUI_TYPE_MOVEMENT_STEP,
                  G_TYPE_INT,
                  G_TYPE_BOOLEAN);
  g_signal_set_va_marshaller (signals[MOVE_CURSOR],
                              G_OBJECT_CLASS_TYPE (gobject_class),
                              _bobgui_marshal_VOID__ENUM_INT_BOOLEANv);

  /**
   * BobguiText::insert-at-cursor:
   * @self: the text widget which emitted the signal
   * @string: the string to insert
   *
   * Emitted when the user initiates the insertion of a
   * fixed string at the cursor.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * This signal has no default bindings.
   */
  signals[INSERT_AT_CURSOR] =
    g_signal_new (I_("insert-at-cursor"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiTextClass, insert_at_cursor),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  /**
   * BobguiText::delete-from-cursor:
   * @self: the text widget which emitted the signal
   * @type: the granularity of the deletion
   * @count: the number of @type units to delete
   *
   * Emitted when the user initiates a text deletion.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * If the @type is [enum@Bobgui.DeleteType.chars], BOBGUI deletes the
   * selection if there is one, otherwise it deletes the requested
   * number of characters.
   *
   * The default bindings for this signal are <kbd>Delete</kbd>
   * for deleting a character and <kbd>Ctrl</kbd>+<kbd>Delete</kbd>
   * for deleting a word.
   */
  signals[DELETE_FROM_CURSOR] =
    g_signal_new (I_("delete-from-cursor"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiTextClass, delete_from_cursor),
                  NULL, NULL,
                  _bobgui_marshal_VOID__ENUM_INT,
                  G_TYPE_NONE, 2,
                  BOBGUI_TYPE_DELETE_TYPE,
                  G_TYPE_INT);
  g_signal_set_va_marshaller (signals[DELETE_FROM_CURSOR],
                              G_OBJECT_CLASS_TYPE (gobject_class),
                              _bobgui_marshal_VOID__ENUM_INTv);

  /**
   * BobguiText::backspace:
   * @self: the text widget which emitted the signal
   *
   * Emitted when the user asks for it.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default bindings for this signal are
   * <kbd>Backspace</kbd> and <kbd>Shift</kbd>+<kbd>Backspace</kbd>.
   */
  signals[BACKSPACE] =
    g_signal_new (I_("backspace"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiTextClass, backspace),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiText::cut-clipboard:
   * @self: the text widget which emitted the signal
   *
   * Emitted to cut the selection to the clipboard.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default bindings for this signal are
   * <kbd>Ctrl</kbd>+<kbd>x</kbd> and
   * <kbd>Shift</kbd>+<kbd>Delete</kbd>.
   */
  signals[CUT_CLIPBOARD] =
    g_signal_new (I_("cut-clipboard"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiTextClass, cut_clipboard),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiText::copy-clipboard:
   * @self: the text widget which emitted the signal
   *
   * Emitted to copy the selection to the clipboard.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default bindings for this signal are
   * <kbd>Ctrl</kbd>+<kbd>c</kbd> and
   * <kbd>Ctrl</kbd>+<kbd>Insert</kbd>.
   */
  signals[COPY_CLIPBOARD] =
    g_signal_new (I_("copy-clipboard"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiTextClass, copy_clipboard),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiText::paste-clipboard:
   * @self: the text widget which emitted the signal
   *
   * Emitted to paste the contents of the clipboard.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default bindings for this signal are
   * <kbd>Ctrl</kbd>+<kbd>v</kbd> and <kbd>Shift</kbd>+<kbd>Insert</kbd>.
   */
  signals[PASTE_CLIPBOARD] =
    g_signal_new (I_("paste-clipboard"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiTextClass, paste_clipboard),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiText::toggle-overwrite:
   * @self: the text widget which emitted the signal
   *
   * Emitted to toggle the overwrite mode.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default bindings for this signal is <kbd>Insert</kbd>.
   */
  signals[TOGGLE_OVERWRITE] =
    g_signal_new (I_("toggle-overwrite"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiTextClass, toggle_overwrite),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiText::preedit-changed:
   * @self: the text widget which emitted the signal
   * @preedit: the current preedit string
   *
   * Emitted when the preedit text changes.
   *
   * If an input method is used, the typed text will not immediately
   * be committed to the buffer. So if you are interested in the text,
   * connect to this signal.
   */
  signals[PREEDIT_CHANGED] =
    g_signal_new_class_handler (I_("preedit-changed"),
                                G_OBJECT_CLASS_TYPE (gobject_class),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                NULL,
                                NULL, NULL,
                                NULL,
                                G_TYPE_NONE, 1,
                                G_TYPE_STRING);

  /**
   * BobguiText::insert-emoji:
   * @self: the text widget which emitted the signal
   *
   * Emitted to present the Emoji chooser.
   *
   * This is a [keybinding signal](class.SignalAction.html).
   *
   * The default bindings for this signal are
   * <kbd>Ctrl</kbd>+<kbd>.</kbd> and
   * <kbd>Ctrl</kbd>+<kbd>;</kbd>
   */
  signals[INSERT_EMOJI] =
    g_signal_new (I_("insert-emoji"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiTextClass, insert_emoji),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /*
   * Actions
   */

  /**
   * BobguiText|clipboard.cut:
   *
   * Copies the contents to the clipboard and deletes it from the widget.
   */
  bobgui_widget_class_install_action (widget_class, "clipboard.cut", NULL,
                                   bobgui_text_activate_clipboard_cut);

  /**
   * BobguiText|clipboard.copy:
   *
   * Copies the contents to the clipboard.
   */
  bobgui_widget_class_install_action (widget_class, "clipboard.copy", NULL,
                                   bobgui_text_activate_clipboard_copy);

  /**
   * BobguiText|clipboard.paste:
   *
   * Inserts the contents of the clipboard into the widget.
   */
  bobgui_widget_class_install_action (widget_class, "clipboard.paste", NULL,
                                   bobgui_text_activate_clipboard_paste);

  /**
   * BobguiText|selection.delete:
   *
   * Deletes the current selection.
   */
  bobgui_widget_class_install_action (widget_class, "selection.delete", NULL,
                                   bobgui_text_activate_selection_delete);

  /**
   * BobguiText|selection.select-all:
   *
   * Selects all of the widgets content.
   */
  bobgui_widget_class_install_action (widget_class, "selection.select-all", NULL,
                                   bobgui_text_activate_selection_select_all);

  bobgui_widget_class_install_action (widget_class, "text.clear", NULL, bobgui_text_clear);

  /**
   * BobguiText|misc.insert-emoji:
   *
   * Opens the Emoji chooser.
   */
  bobgui_widget_class_install_action (widget_class, "misc.insert-emoji", NULL,
                                   bobgui_text_activate_misc_insert_emoji);

  /**
   * BobguiText|misc.toggle-direction:
   *
   * Toggles the text direction.
   */
  bobgui_widget_class_install_action (widget_class, "misc.toggle-direction", NULL,
                                   bobgui_text_activate_misc_toggle_direction);

  /**
   * BobguiText|misc.toggle-visibility:
   *
   * Toggles the `BobguiText`:visibility property.
   */
  bobgui_widget_class_install_property_action (widget_class,
                                            "misc.toggle-visibility",
                                            "visibility");


  /**
   * BobguiText|text.undo:
   *
   * Undoes the last change to the contents.
   */
  bobgui_widget_class_install_action (widget_class, "text.undo", NULL, bobgui_text_real_undo);

  /**
   * BobguiText|text.redo:
   *
   * Redoes the last change to the contents.
   */
  bobgui_widget_class_install_action (widget_class, "text.redo", NULL, bobgui_text_real_redo);

  /**
   * BobguiText|menu.popup:
   *
   * Opens the context menu.
   */
  bobgui_widget_class_install_action (widget_class, "menu.popup", NULL, bobgui_text_popup_menu);

  /*
   * Key bindings
   */

  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_F10, GDK_SHIFT_MASK,
                                       "menu.popup",
                                       NULL);
  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_Menu, 0,
                                       "menu.popup",
                                       NULL);

  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_t, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                       "misc.toggle-direction",
                                       NULL);

  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_Clear, GDK_NO_MODIFIER_MASK,
                                       "text.clear",
                                       NULL);

  /* Moving the insertion point */
  add_move_binding (widget_class, GDK_KEY_Right, 0,
                    BOBGUI_MOVEMENT_VISUAL_POSITIONS, 1);

  add_move_binding (widget_class, GDK_KEY_Left, 0,
                    BOBGUI_MOVEMENT_VISUAL_POSITIONS, -1);

  add_move_binding (widget_class, GDK_KEY_KP_Right, 0,
                    BOBGUI_MOVEMENT_VISUAL_POSITIONS, 1);

  add_move_binding (widget_class, GDK_KEY_KP_Left, 0,
                    BOBGUI_MOVEMENT_VISUAL_POSITIONS, -1);

  add_move_binding (widget_class, GDK_KEY_Right, GDK_CONTROL_MASK,
                    BOBGUI_MOVEMENT_WORDS, 1);

  add_move_binding (widget_class, GDK_KEY_Left, GDK_CONTROL_MASK,
                    BOBGUI_MOVEMENT_WORDS, -1);

  add_move_binding (widget_class, GDK_KEY_KP_Right, GDK_CONTROL_MASK,
                    BOBGUI_MOVEMENT_WORDS, 1);

  add_move_binding (widget_class, GDK_KEY_KP_Left, GDK_CONTROL_MASK,
                    BOBGUI_MOVEMENT_WORDS, -1);

  add_move_binding (widget_class, GDK_KEY_Home, 0,
                    BOBGUI_MOVEMENT_DISPLAY_LINE_ENDS, -1);

  add_move_binding (widget_class, GDK_KEY_End, 0,
                    BOBGUI_MOVEMENT_DISPLAY_LINE_ENDS, 1);

  add_move_binding (widget_class, GDK_KEY_KP_Home, 0,
                    BOBGUI_MOVEMENT_DISPLAY_LINE_ENDS, -1);

  add_move_binding (widget_class, GDK_KEY_KP_End, 0,
                    BOBGUI_MOVEMENT_DISPLAY_LINE_ENDS, 1);

  add_move_binding (widget_class, GDK_KEY_Home, GDK_CONTROL_MASK,
                    BOBGUI_MOVEMENT_BUFFER_ENDS, -1);

  add_move_binding (widget_class, GDK_KEY_End, GDK_CONTROL_MASK,
                    BOBGUI_MOVEMENT_BUFFER_ENDS, 1);

  add_move_binding (widget_class, GDK_KEY_KP_Home, GDK_CONTROL_MASK,
                    BOBGUI_MOVEMENT_BUFFER_ENDS, -1);

  add_move_binding (widget_class, GDK_KEY_KP_End, GDK_CONTROL_MASK,
                    BOBGUI_MOVEMENT_BUFFER_ENDS, 1);

#ifdef __APPLE__
  add_move_binding (widget_class, GDK_KEY_Right, GDK_ALT_MASK,
                    BOBGUI_MOVEMENT_WORDS, 1);

  add_move_binding (widget_class, GDK_KEY_Left, GDK_ALT_MASK,
                    BOBGUI_MOVEMENT_WORDS, -1);

  add_move_binding (widget_class, GDK_KEY_KP_Right, GDK_ALT_MASK,
                    BOBGUI_MOVEMENT_WORDS, 1);

  add_move_binding (widget_class, GDK_KEY_KP_Left, GDK_ALT_MASK,
                    BOBGUI_MOVEMENT_WORDS, -1);

  add_move_binding (widget_class, GDK_KEY_Right, GDK_META_MASK,
                    BOBGUI_MOVEMENT_DISPLAY_LINE_ENDS, 1);

  add_move_binding (widget_class, GDK_KEY_Left, GDK_META_MASK,
                    BOBGUI_MOVEMENT_DISPLAY_LINE_ENDS, -1);

  add_move_binding (widget_class, GDK_KEY_KP_Right, GDK_META_MASK,
                    BOBGUI_MOVEMENT_DISPLAY_LINE_ENDS, 1);

  add_move_binding (widget_class, GDK_KEY_KP_Left, GDK_META_MASK,
                    BOBGUI_MOVEMENT_DISPLAY_LINE_ENDS, -1);

  add_move_binding (widget_class, GDK_KEY_Up, GDK_META_MASK,
                    BOBGUI_MOVEMENT_BUFFER_ENDS, -1);

  add_move_binding (widget_class, GDK_KEY_Down, GDK_META_MASK,
                    BOBGUI_MOVEMENT_BUFFER_ENDS, 1);

  add_move_binding (widget_class, GDK_KEY_KP_Up, GDK_META_MASK,
                    BOBGUI_MOVEMENT_BUFFER_ENDS, -1);

  add_move_binding (widget_class, GDK_KEY_KP_Down, GDK_META_MASK,
                    BOBGUI_MOVEMENT_BUFFER_ENDS, 1);
#endif

  /* Select all */
#ifdef __APPLE__
  bobgui_widget_class_add_binding (widget_class,
                                GDK_KEY_a, GDK_META_MASK,
                                (BobguiShortcutFunc) bobgui_text_select_all,
                                NULL);
#else
  bobgui_widget_class_add_binding (widget_class,
                                GDK_KEY_a, GDK_CONTROL_MASK,
                                (BobguiShortcutFunc) bobgui_text_select_all,
                                NULL);

  bobgui_widget_class_add_binding (widget_class,
                                GDK_KEY_slash, GDK_CONTROL_MASK,
                                (BobguiShortcutFunc) bobgui_text_select_all,
                                NULL);
#endif

  /* Unselect all */
#ifdef __APPLE__
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_a, GDK_SHIFT_MASK | GDK_META_MASK,
                                       "move-cursor",
                                       "(iib)", BOBGUI_MOVEMENT_VISUAL_POSITIONS, 0, FALSE);
#else
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_backslash, GDK_CONTROL_MASK,
                                       "move-cursor",
                                       "(iib)", BOBGUI_MOVEMENT_VISUAL_POSITIONS, 0, FALSE);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_a, GDK_SHIFT_MASK | GDK_CONTROL_MASK,
                                       "move-cursor",
                                       "(iib)", BOBGUI_MOVEMENT_VISUAL_POSITIONS, 0, FALSE);
#endif

  /* Activate
   */
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_Return, 0,
                                       "activate",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_ISO_Enter, 0,
                                       "activate",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class, GDK_KEY_KP_Enter, 0,
                                       "activate",
                                       NULL);

  /* Deleting text */
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Delete, 0,
                                       "delete-from-cursor",
                                       "(ii)", BOBGUI_DELETE_CHARS, 1);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Delete, 0,
                                       "delete-from-cursor",
                                       "(ii)", BOBGUI_DELETE_CHARS, 1);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_BackSpace, 0,
                                       "backspace",
                                       NULL);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_u, GDK_CONTROL_MASK,
                                       "delete-from-cursor",
                                       "(ii)", BOBGUI_DELETE_PARAGRAPH_ENDS, -1);

  /* Make this do the same as Backspace, to help with mis-typing */
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_BackSpace, GDK_SHIFT_MASK,
                                       "backspace",
                                       NULL);

#ifdef __APPLE__
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Delete, GDK_ALT_MASK,
                                       "delete-from-cursor",
                                       "(ii)", BOBGUI_DELETE_WORD_ENDS, 1);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_BackSpace, GDK_ALT_MASK,
                                       "delete-from-cursor",
                                       "(ii)", BOBGUI_DELETE_WORD_ENDS, -1);
#else
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Delete, GDK_CONTROL_MASK,
                                       "delete-from-cursor",
                                       "(ii)", BOBGUI_DELETE_WORD_ENDS, 1);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Delete, GDK_CONTROL_MASK,
                                       "delete-from-cursor",
                                       "(ii)", BOBGUI_DELETE_WORD_ENDS, 1);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_BackSpace, GDK_CONTROL_MASK,
                                       "delete-from-cursor",
                                       "(ii)", BOBGUI_DELETE_WORD_ENDS, -1);
#endif

  /* Cut/copy/paste */
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Cut, GDK_NO_MODIFIER_MASK,
                                       "cut-clipboard",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Copy, GDK_NO_MODIFIER_MASK,
                                       "copy-clipboard",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Paste, GDK_NO_MODIFIER_MASK,
                                       "paste-clipboard",
                                       NULL);
#ifdef __APPLE__
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_x, GDK_META_MASK,
                                       "cut-clipboard",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_c, GDK_META_MASK,
                                       "copy-clipboard",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_v, GDK_META_MASK,
                                       "paste-clipboard",
                                       NULL);
#else
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_x, GDK_CONTROL_MASK,
                                       "cut-clipboard",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_c, GDK_CONTROL_MASK,
                                       "copy-clipboard",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_v, GDK_CONTROL_MASK,
                                       "paste-clipboard",
                                       NULL);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Delete, GDK_SHIFT_MASK,
                                       "cut-clipboard",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Insert, GDK_CONTROL_MASK,
                                       "copy-clipboard",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Insert, GDK_SHIFT_MASK,
                                       "paste-clipboard",
                                       NULL);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Delete, GDK_SHIFT_MASK,
                                       "cut-clipboard",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Insert, GDK_CONTROL_MASK,
                                       "copy-clipboard",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Insert, GDK_SHIFT_MASK,
                                       "paste-clipboard",
                                       NULL);
#endif

  /* Overwrite */
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Insert, 0,
                                       "toggle-overwrite",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Insert, 0,
                                       "toggle-overwrite",
                                       NULL);

  /* Emoji */
  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_period, GDK_CONTROL_MASK,
                                       "misc.insert-emoji",
                                       NULL);
  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_semicolon, GDK_CONTROL_MASK,
                                       "misc.insert-emoji",
                                       NULL);

  /* Undo/Redo */
  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_Undo, GDK_NO_MODIFIER_MASK,
                                       "text.undo", NULL);
  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_Redo, GDK_NO_MODIFIER_MASK,
                                       "text.redo", NULL);
#ifdef __APPLE__
  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_z, GDK_META_MASK,
                                       "text.undo", NULL);
  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_z, GDK_META_MASK | GDK_SHIFT_MASK,
                                       "text.redo", NULL);
#else
  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_z, GDK_CONTROL_MASK,
                                       "text.undo", NULL);
  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_y, GDK_CONTROL_MASK,
                                       "text.redo", NULL);
  bobgui_widget_class_add_binding_action (widget_class,
                                       GDK_KEY_z, GDK_CONTROL_MASK | GDK_SHIFT_MASK,
                                       "text.redo", NULL);
#endif

  bobgui_widget_class_set_css_name (widget_class, I_("text"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_NONE);
}

static void
editable_insert_text (BobguiEditable *editable,
                      const char  *text,
                      int          length,
                      int         *position)
{
  bobgui_text_insert_text (BOBGUI_TEXT (editable), text, length, position);
}

static void
editable_delete_text (BobguiEditable *editable,
                      int          start_pos,
                      int          end_pos)
{
  bobgui_text_delete_text (BOBGUI_TEXT (editable), start_pos, end_pos);
}

static const char *
editable_get_text (BobguiEditable *editable)
{
  return bobgui_entry_buffer_get_text (get_buffer (BOBGUI_TEXT (editable)));
}

static void
editable_set_selection_bounds (BobguiEditable *editable,
                               int          start_pos,
                               int          end_pos)
{
  bobgui_text_set_selection_bounds (BOBGUI_TEXT (editable), start_pos, end_pos);
}

static gboolean
editable_get_selection_bounds (BobguiEditable *editable,
                               int         *start_pos,
                               int         *end_pos)
{
  return bobgui_text_get_selection_bounds (BOBGUI_TEXT (editable), start_pos, end_pos);
}

static void
bobgui_text_editable_init (BobguiEditableInterface *iface)
{
  iface->insert_text = editable_insert_text;
  iface->delete_text = editable_delete_text;
  iface->get_text = editable_get_text;
  iface->set_selection_bounds = editable_set_selection_bounds;
  iface->get_selection_bounds = editable_get_selection_bounds;
}

static void
bobgui_text_set_property (GObject      *object,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  BobguiText *self = BOBGUI_TEXT (object);
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  switch (prop_id)
    {
    /* BobguiEditable properties */
    case NUM_PROPERTIES + BOBGUI_EDITABLE_PROP_EDITABLE:
      bobgui_text_set_editable (self, g_value_get_boolean (value));
      break;

    case NUM_PROPERTIES + BOBGUI_EDITABLE_PROP_WIDTH_CHARS:
      bobgui_text_set_width_chars (self, g_value_get_int (value));
      break;

    case NUM_PROPERTIES + BOBGUI_EDITABLE_PROP_MAX_WIDTH_CHARS:
      bobgui_text_set_max_width_chars (self, g_value_get_int (value));
      break;

    case NUM_PROPERTIES + BOBGUI_EDITABLE_PROP_TEXT:
      bobgui_text_set_text (self, g_value_get_string (value));
      break;

    case NUM_PROPERTIES + BOBGUI_EDITABLE_PROP_XALIGN:
      bobgui_text_set_alignment (self, g_value_get_float (value));
      break;

    case NUM_PROPERTIES + BOBGUI_EDITABLE_PROP_ENABLE_UNDO:
      bobgui_text_set_enable_undo (self, g_value_get_boolean (value));
      break;

    /* BobguiText properties */
    case PROP_BUFFER:
      bobgui_text_set_buffer (self, g_value_get_object (value));
      break;

    case PROP_MAX_LENGTH:
      bobgui_text_set_max_length (self, g_value_get_int (value));
      break;

    case PROP_VISIBILITY:
      bobgui_text_set_visibility (self, g_value_get_boolean (value));
      break;

    case PROP_INVISIBLE_CHAR:
      bobgui_text_set_invisible_char (self, g_value_get_uint (value));
      break;

    case PROP_ACTIVATES_DEFAULT:
      bobgui_text_set_activates_default (self, g_value_get_boolean (value));
      break;

    case PROP_TRUNCATE_MULTILINE:
      bobgui_text_set_truncate_multiline (self, g_value_get_boolean (value));
      break;

    case PROP_OVERWRITE_MODE:
      bobgui_text_set_overwrite_mode (self, g_value_get_boolean (value));
      break;

    case PROP_INVISIBLE_CHAR_SET:
      if (g_value_get_boolean (value))
        priv->invisible_char_set = TRUE;
      else
        bobgui_text_unset_invisible_char (self);
      break;

    case PROP_PLACEHOLDER_TEXT:
      bobgui_text_set_placeholder_text (self, g_value_get_string (value));
      break;

    case PROP_IM_MODULE:
      g_free (priv->im_module);
      priv->im_module = g_value_dup_string (value);
      if (BOBGUI_IS_IM_MULTICONTEXT (priv->im_context))
        bobgui_im_multicontext_set_context_id (BOBGUI_IM_MULTICONTEXT (priv->im_context), priv->im_module);
      g_object_notify_by_pspec (object, pspec);
      break;

    case PROP_INPUT_PURPOSE:
      bobgui_text_set_input_purpose (self, g_value_get_enum (value));
      break;

    case PROP_INPUT_HINTS:
      bobgui_text_set_input_hints (self, g_value_get_flags (value));
      break;

    case PROP_ATTRIBUTES:
      bobgui_text_set_attributes (self, g_value_get_boxed (value));
      break;

    case PROP_TABS:
      bobgui_text_set_tabs (self, g_value_get_boxed (value));
      break;

    case PROP_ENABLE_EMOJI_COMPLETION:
      bobgui_text_set_enable_emoji_completion (self, g_value_get_boolean (value));
      break;

    case PROP_PROPAGATE_TEXT_WIDTH:
      bobgui_text_set_propagate_text_width (self, g_value_get_boolean (value));
      break;

    case PROP_EXTRA_MENU:
      bobgui_text_set_extra_menu (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_text_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  BobguiText *self = BOBGUI_TEXT (object);
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  switch (prop_id)
    {
    /* BobguiEditable properties */
    case NUM_PROPERTIES + BOBGUI_EDITABLE_PROP_CURSOR_POSITION:
      g_value_set_int (value, priv->current_pos);
      break;

    case NUM_PROPERTIES + BOBGUI_EDITABLE_PROP_SELECTION_BOUND:
      g_value_set_int (value, priv->selection_bound);
      break;

    case NUM_PROPERTIES + BOBGUI_EDITABLE_PROP_EDITABLE:
      g_value_set_boolean (value, priv->editable);
      break;

    case NUM_PROPERTIES + BOBGUI_EDITABLE_PROP_WIDTH_CHARS:
      g_value_set_int (value, priv->width_chars);
      break;

    case NUM_PROPERTIES + BOBGUI_EDITABLE_PROP_MAX_WIDTH_CHARS:
      g_value_set_int (value, priv->max_width_chars);
      break;

    case NUM_PROPERTIES + BOBGUI_EDITABLE_PROP_TEXT:
      g_value_set_string (value, bobgui_entry_buffer_get_text (get_buffer (self)));
      break;

    case NUM_PROPERTIES + BOBGUI_EDITABLE_PROP_XALIGN:
      g_value_set_float (value, priv->xalign);
      break;

    case NUM_PROPERTIES + BOBGUI_EDITABLE_PROP_ENABLE_UNDO:
      g_value_set_boolean (value, priv->enable_undo);
      break;

    /* BobguiText properties */
    case PROP_BUFFER:
      g_value_set_object (value, get_buffer (self));
      break;

    case PROP_MAX_LENGTH:
      g_value_set_int (value, bobgui_entry_buffer_get_max_length (get_buffer (self)));
      break;

    case PROP_VISIBILITY:
      g_value_set_boolean (value, priv->visible);
      break;

    case PROP_INVISIBLE_CHAR:
      g_value_set_uint (value, priv->invisible_char);
      break;

    case PROP_ACTIVATES_DEFAULT:
      g_value_set_boolean (value, priv->activates_default);
      break;

    case PROP_SCROLL_OFFSET:
      g_value_set_int (value, priv->scroll_offset);
      break;

    case PROP_TRUNCATE_MULTILINE:
      g_value_set_boolean (value, priv->truncate_multiline);
      break;

    case PROP_OVERWRITE_MODE:
      g_value_set_boolean (value, priv->overwrite_mode);
      break;

    case PROP_INVISIBLE_CHAR_SET:
      g_value_set_boolean (value, priv->invisible_char_set);
      break;

    case PROP_IM_MODULE:
      g_value_set_string (value, priv->im_module);
      break;

    case PROP_PLACEHOLDER_TEXT:
      g_value_set_string (value, bobgui_text_get_placeholder_text (self));
      break;

    case PROP_INPUT_PURPOSE:
      g_value_set_enum (value, bobgui_text_get_input_purpose (self));
      break;

    case PROP_INPUT_HINTS:
      g_value_set_flags (value, bobgui_text_get_input_hints (self));
      break;

    case PROP_ATTRIBUTES:
      g_value_set_boxed (value, priv->attrs);
      break;

    case PROP_TABS:
      g_value_set_boxed (value, priv->tabs);
      break;

    case PROP_ENABLE_EMOJI_COMPLETION:
      g_value_set_boolean (value, priv->enable_emoji_completion);
      break;

    case PROP_PROPAGATE_TEXT_WIDTH:
      g_value_set_boolean (value, priv->propagate_text_width);
      break;

    case PROP_EXTRA_MENU:
      g_value_set_object (value, priv->extra_menu);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_text_notify (GObject    *object,
                 GParamSpec *pspec)
{
  if (pspec->name == I_("has-focus"))
    bobgui_text_check_cursor_blink (BOBGUI_TEXT (object));

  if (G_OBJECT_CLASS (bobgui_text_parent_class)->notify)
    G_OBJECT_CLASS (bobgui_text_parent_class)->notify (object, pspec);
}

static void
bobgui_text_ensure_text_handles (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  int i;

  for (i = 0; i < TEXT_HANDLE_N_HANDLES; i++)
    {
      if (priv->text_handles[i])
        continue;
      priv->text_handles[i] = bobgui_text_handle_new (BOBGUI_WIDGET (self));
      g_signal_connect (priv->text_handles[i], "drag-started",
                        G_CALLBACK (bobgui_text_handle_drag_started), self);
      g_signal_connect (priv->text_handles[i], "handle-dragged",
                        G_CALLBACK (bobgui_text_handle_dragged), self);
      g_signal_connect (priv->text_handles[i], "drag-finished",
                        G_CALLBACK (bobgui_text_handle_drag_finished), self);
    }
}

static void
bobgui_text_init (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  BobguiCssNode *widget_node;
  BobguiGesture *gesture;
  BobguiEventController *controller;
  int i;
  BobguiDropTarget *target;

  bobgui_widget_set_focusable (BOBGUI_WIDGET (self), TRUE);
  bobgui_widget_set_overflow (BOBGUI_WIDGET (self), BOBGUI_OVERFLOW_HIDDEN);

  priv->editable = TRUE;
  priv->visible = TRUE;
  priv->dnd_position = -1;
  priv->width_chars = -1;
  priv->max_width_chars = -1;
  priv->truncate_multiline = FALSE;
  priv->xalign = 0.0;
  priv->insert_pos = -1;
  priv->cursor_alpha = 1.0;
  priv->invisible_char = 0;
  priv->history = bobgui_text_history_new (&history_funcs, self);
  priv->enable_undo = TRUE;

  bobgui_text_history_set_max_undo_levels (priv->history, DEFAULT_MAX_UNDO);

  priv->selection_content = g_object_new (BOBGUI_TYPE_TEXT_CONTENT, NULL);
  BOBGUI_TEXT_CONTENT (priv->selection_content)->self = self;

  target = bobgui_drop_target_new (G_TYPE_STRING, GDK_ACTION_COPY | GDK_ACTION_MOVE);
  bobgui_event_controller_set_static_name (BOBGUI_EVENT_CONTROLLER (target), "bobgui-text-drop-target");
  g_signal_connect (target, "accept", G_CALLBACK (bobgui_text_drag_accept), self);
  g_signal_connect (target, "enter", G_CALLBACK (bobgui_text_drag_motion), self);
  g_signal_connect (target, "motion", G_CALLBACK (bobgui_text_drag_motion), self);
  g_signal_connect (target, "leave", G_CALLBACK (bobgui_text_drag_leave), self);
  g_signal_connect (target, "drop", G_CALLBACK (bobgui_text_drag_drop), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (target));

  /* This object is completely private. No external entity can gain a reference
   * to it; so we create it here and destroy it in finalize().
   */
  priv->im_context = bobgui_im_multicontext_new ();

  g_signal_connect (priv->im_context, "preedit-start",
                    G_CALLBACK (bobgui_text_preedit_start_cb), self);
  g_signal_connect (priv->im_context, "commit",
                    G_CALLBACK (bobgui_text_commit_cb), self);
  g_signal_connect (priv->im_context, "preedit-changed",
                    G_CALLBACK (bobgui_text_preedit_changed_cb), self);
  g_signal_connect (priv->im_context, "retrieve-surrounding",
                    G_CALLBACK (bobgui_text_retrieve_surrounding_cb), self);
  g_signal_connect (priv->im_context, "delete-surrounding",
                    G_CALLBACK (bobgui_text_delete_surrounding_cb), self);

  priv->drag_gesture = bobgui_gesture_drag_new ();
  bobgui_event_controller_set_static_name (BOBGUI_EVENT_CONTROLLER (priv->drag_gesture), "bobgui-text-drag-gesture");
  g_signal_connect (priv->drag_gesture, "drag-update",
                    G_CALLBACK (bobgui_text_drag_gesture_update), self);
  g_signal_connect (priv->drag_gesture, "drag-end",
                    G_CALLBACK (bobgui_text_drag_gesture_end), self);
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (priv->drag_gesture), 0);
  bobgui_gesture_single_set_exclusive (BOBGUI_GESTURE_SINGLE (priv->drag_gesture), TRUE);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (priv->drag_gesture));

  gesture = bobgui_gesture_click_new ();
  bobgui_event_controller_set_static_name (BOBGUI_EVENT_CONTROLLER (gesture), "bobgui-text-click-gesture");
  g_signal_connect (gesture, "pressed",
                    G_CALLBACK (bobgui_text_click_gesture_pressed), self);
  g_signal_connect (gesture, "released",
                    G_CALLBACK (bobgui_text_click_gesture_released), self);
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture), 0);
  bobgui_gesture_single_set_exclusive (BOBGUI_GESTURE_SINGLE (gesture), TRUE);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (gesture));

  controller = bobgui_event_controller_motion_new ();
  bobgui_event_controller_set_static_name (controller, "bobgui-text-motion-controller");
  g_signal_connect (controller, "motion",
                    G_CALLBACK (bobgui_text_motion_controller_motion), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), controller);

  priv->key_controller = bobgui_event_controller_key_new ();
  bobgui_event_controller_set_propagation_phase (priv->key_controller, BOBGUI_PHASE_TARGET);
  bobgui_event_controller_set_static_name (priv->key_controller, "bobgui-text-key-controller");
  g_signal_connect (priv->key_controller, "key-pressed",
                    G_CALLBACK (bobgui_text_key_controller_key_pressed), self);
  g_signal_connect_swapped (priv->key_controller, "im-update",
                            G_CALLBACK (bobgui_text_schedule_im_reset), self);
  bobgui_event_controller_key_set_im_context (BOBGUI_EVENT_CONTROLLER_KEY (priv->key_controller),
                                           priv->im_context);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), priv->key_controller);

  priv->focus_controller = bobgui_event_controller_focus_new ();
  bobgui_event_controller_set_static_name (priv->focus_controller, "bobgui-text-focus-controller");
  g_signal_connect (priv->focus_controller, "notify::is-focus",
                    G_CALLBACK (bobgui_text_focus_changed), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), priv->focus_controller);

  widget_node = bobgui_widget_get_css_node (BOBGUI_WIDGET (self));
  for (i = 0; i < 2; i++)
    {
      priv->undershoot_node[i] = bobgui_css_node_new ();
      bobgui_css_node_set_name (priv->undershoot_node[i], g_quark_from_static_string ("undershoot"));
      bobgui_css_node_add_class (priv->undershoot_node[i], g_quark_from_static_string (i == 0 ? "left" : "right"));
      bobgui_css_node_set_parent (priv->undershoot_node[i], widget_node);
      bobgui_css_node_set_state (priv->undershoot_node[i], bobgui_css_node_get_state (widget_node) & ~BOBGUI_STATE_FLAG_DROP_ACTIVE);
      g_object_unref (priv->undershoot_node[i]);
    }

  set_text_cursor (BOBGUI_WIDGET (self));
}

static void
bobgui_text_dispose (GObject *object)
{
  BobguiText *self = BOBGUI_TEXT (object);
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  BobguiWidget *chooser;

  priv->current_pos = priv->selection_bound = 0;
  bobgui_text_reset_im_context (self);
  bobgui_text_reset_layout (self);

  if (priv->blink_tick)
    {
      bobgui_widget_remove_tick_callback (BOBGUI_WIDGET (object), priv->blink_tick);
      priv->blink_tick = 0;
    }

  if (priv->magnifier)
    _bobgui_magnifier_set_inspected (BOBGUI_MAGNIFIER (priv->magnifier), NULL);

  if (priv->buffer)
    {
      buffer_disconnect_signals (self);
      g_object_unref (priv->buffer);
      priv->buffer = NULL;
    }

  g_clear_pointer (&priv->emoji_completion, bobgui_widget_unparent);
  chooser = g_object_get_data (object, "bobgui-emoji-chooser");
  if (chooser)
    bobgui_widget_unparent (chooser);

  g_clear_pointer (&priv->selection_bubble, bobgui_widget_unparent);
  g_clear_pointer (&priv->popup_menu, bobgui_widget_unparent);
  g_clear_pointer ((BobguiWidget **) &priv->text_handles[TEXT_HANDLE_CURSOR], bobgui_widget_unparent);
  g_clear_pointer ((BobguiWidget **) &priv->text_handles[TEXT_HANDLE_SELECTION_BOUND], bobgui_widget_unparent);
  g_clear_object (&priv->extra_menu);

  g_clear_pointer (&priv->magnifier_popover, bobgui_widget_unparent);
  g_clear_pointer (&priv->placeholder, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_text_parent_class)->dispose (object);
}

static void
bobgui_text_finalize (GObject *object)
{
  BobguiText *self = BOBGUI_TEXT (object);
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_clear_object (&priv->selection_content);

  g_clear_object (&priv->history);
  g_clear_object (&priv->cached_layout);
  g_clear_object (&priv->im_context);
  g_free (priv->im_module);

  if (priv->tabs)
    pango_tab_array_free (priv->tabs);

  if (priv->attrs)
    pango_attr_list_unref (priv->attrs);


  G_OBJECT_CLASS (bobgui_text_parent_class)->finalize (object);
}

static void
bobgui_text_ensure_magnifier (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->magnifier_popover)
    return;

  priv->magnifier = _bobgui_magnifier_new (BOBGUI_WIDGET (self));
  bobgui_widget_set_size_request (priv->magnifier, 100, 60);
  _bobgui_magnifier_set_magnification (BOBGUI_MAGNIFIER (priv->magnifier), 2.0);
  priv->magnifier_popover = bobgui_popover_new ();
  bobgui_popover_set_position (BOBGUI_POPOVER (priv->magnifier_popover), BOBGUI_POS_TOP);
  bobgui_widget_set_parent (priv->magnifier_popover, BOBGUI_WIDGET (self));
  bobgui_widget_add_css_class (priv->magnifier_popover, "magnifier");
  bobgui_popover_set_autohide (BOBGUI_POPOVER (priv->magnifier_popover), FALSE);
  bobgui_popover_set_child (BOBGUI_POPOVER (priv->magnifier_popover), priv->magnifier);
  bobgui_widget_set_visible (priv->magnifier, TRUE);
}

static void
begin_change (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  priv->change_count++;

  g_object_freeze_notify (G_OBJECT (self));
}

static void
end_change (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_if_fail (priv->change_count > 0);

  g_object_thaw_notify (G_OBJECT (self));

  priv->change_count--;

  if (priv->change_count == 0)
    {
       if (priv->real_changed)
         {
           g_signal_emit_by_name (self, "changed");
           priv->real_changed = FALSE;
         }
    }
}

static void
emit_changed (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->change_count == 0)
    g_signal_emit_by_name (self, "changed");
  else
    priv->real_changed = TRUE;
}

static DisplayMode
bobgui_text_get_display_mode (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->visible)
    return DISPLAY_NORMAL;

  if (priv->invisible_char == 0 && priv->invisible_char_set)
    return DISPLAY_BLANK;

  return DISPLAY_INVISIBLE;
}

char *
bobgui_text_get_display_text (BobguiText *self,
                           int      start_pos,
                           int      end_pos)
{
  BobguiTextPasswordHint *password_hint;
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  gunichar invisible_char;
  const char *start;
  const char *end;
  const char *text;
  char char_str[7];
  int char_len;
  GString *str;
  guint length;
  int i;

  text = bobgui_entry_buffer_get_text (get_buffer (self));
  length = bobgui_entry_buffer_get_length (get_buffer (self));

  if (end_pos < 0 || end_pos > length)
    end_pos = length;
  if (start_pos > length)
    start_pos = length;

  if (end_pos <= start_pos)
      return g_strdup ("");
  else if (priv->visible)
    {
      start = g_utf8_offset_to_pointer (text, start_pos);
      end = g_utf8_offset_to_pointer (start, end_pos - start_pos);
      return g_strndup (start, end - start);
    }
  else
    {
      str = g_string_sized_new (length * 2);

      /* Figure out what our invisible char is and encode it */
      if (!priv->invisible_char)
          invisible_char = priv->invisible_char_set ? ' ' : '*';
      else
          invisible_char = priv->invisible_char;
      char_len = g_unichar_to_utf8 (invisible_char, char_str);

      /*
       * Add hidden characters for each character in the text
       * buffer. If there is a password hint, then keep that
       * character visible.
       */

      password_hint = g_object_get_qdata (G_OBJECT (self), quark_password_hint);
      for (i = start_pos; i < end_pos; ++i)
        {
          if (password_hint && i == password_hint->position)
            {
              start = g_utf8_offset_to_pointer (text, i);
              g_string_append_len (str, start, g_utf8_next_char (start) - start);
            }
          else
            {
              g_string_append_len (str, char_str, char_len);
            }
        }

      return g_string_free (str, FALSE);
    }
}

static void
bobgui_text_map (BobguiWidget *widget)
{
  BobguiText *self = BOBGUI_TEXT (widget);

  BOBGUI_WIDGET_CLASS (bobgui_text_parent_class)->map (widget);

  bobgui_text_recompute (self);
}

static void
bobgui_text_unmap (BobguiWidget *widget)
{
  BobguiText *self = BOBGUI_TEXT (widget);
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  priv->text_handles_enabled = FALSE;
  bobgui_text_update_handles (self);
  priv->cursor_alpha = 1.0;

  BOBGUI_WIDGET_CLASS (bobgui_text_parent_class)->unmap (widget);
}

static void
bobgui_text_im_set_focus_in (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (!priv->editable)
    return;

  bobgui_text_schedule_im_reset (self);
  bobgui_im_context_focus_in (priv->im_context);
}

static void
bobgui_text_realize (BobguiWidget *widget)
{
  BobguiText *self = BOBGUI_TEXT (widget);
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  BOBGUI_WIDGET_CLASS (bobgui_text_parent_class)->realize (widget);

  bobgui_im_context_set_client_widget (priv->im_context, widget);
  if (bobgui_widget_is_focus (BOBGUI_WIDGET (self)))
    bobgui_text_im_set_focus_in (self);

  bobgui_text_adjust_scroll (self);
  bobgui_text_update_primary_selection (self);
}

static void
bobgui_text_unrealize (BobguiWidget *widget)
{
  BobguiText *self = BOBGUI_TEXT (widget);
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  GdkClipboard *clipboard;

  bobgui_text_reset_layout (self);

  bobgui_im_context_set_client_widget (priv->im_context, NULL);

  clipboard = bobgui_widget_get_primary_clipboard (widget);
  if (gdk_clipboard_get_content (clipboard) == priv->selection_content)
    gdk_clipboard_set_content (clipboard, NULL);

  BOBGUI_WIDGET_CLASS (bobgui_text_parent_class)->unrealize (widget);
}

static void
update_im_cursor_location (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  const int text_width = bobgui_widget_get_width (BOBGUI_WIDGET (self));
  GdkRectangle area;
  int strong_x;
  int strong_xoffset;

  bobgui_text_get_cursor_locations (self, &strong_x, NULL);

  strong_xoffset = strong_x - priv->scroll_offset;
  if (strong_xoffset < 0)
    strong_xoffset = 0;
  else if (strong_xoffset > text_width)
    strong_xoffset = text_width;

  area.x = strong_xoffset;
  area.y = 0;
  area.width = 0;
  area.height = bobgui_widget_get_height (BOBGUI_WIDGET (self));

  bobgui_im_context_set_cursor_location (priv->im_context, &area);
}

static void
bobgui_text_move_handle (BobguiText       *self,
                      BobguiTextHandle *handle,
                      int            x,
                      int            y,
                      int            height)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (!bobgui_text_handle_get_is_dragged (handle) &&
      (x < 0 || x > bobgui_widget_get_width (BOBGUI_WIDGET (self))))
    {
      /* Hide the handle if it's not being manipulated
       * and fell outside of the visible text area.
       */
      bobgui_widget_set_visible (BOBGUI_WIDGET (handle), FALSE);
    }
  else
    {
      GdkRectangle rect;

      rect.x = x;
      rect.y = y;
      rect.width = 1;
      rect.height = height;

      bobgui_text_handle_set_position (handle, &rect);
      bobgui_widget_set_direction (BOBGUI_WIDGET (handle), priv->resolved_dir);
      bobgui_widget_set_visible (BOBGUI_WIDGET (handle), TRUE);
    }
}

static int
bobgui_text_get_selection_bound_location (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  PangoLayout *layout;
  PangoRectangle pos;
  int x;
  const char *text;
  int index;

  layout = bobgui_text_ensure_layout (self, FALSE);
  text = pango_layout_get_text (layout);
  index = g_utf8_offset_to_pointer (text, priv->selection_bound) - text;
  pango_layout_index_to_pos (layout, index, &pos);

  if (bobgui_widget_get_direction (BOBGUI_WIDGET (self)) == BOBGUI_TEXT_DIR_RTL)
    x = (pos.x + pos.width) / PANGO_SCALE;
  else
    x = pos.x / PANGO_SCALE;

  return x;
}

static void
bobgui_text_update_handles (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  const int text_height = bobgui_widget_get_height (BOBGUI_WIDGET (self));
  int strong_x;
  int cursor, bound;

  if (!priv->text_handles_enabled)
    {
      if (priv->text_handles[TEXT_HANDLE_CURSOR])
        bobgui_widget_set_visible (BOBGUI_WIDGET (priv->text_handles[TEXT_HANDLE_CURSOR]), FALSE);
      if (priv->text_handles[TEXT_HANDLE_SELECTION_BOUND])
        bobgui_widget_set_visible (BOBGUI_WIDGET (priv->text_handles[TEXT_HANDLE_SELECTION_BOUND]), FALSE);
    }
  else
    {
      bobgui_text_ensure_text_handles (self);
      bobgui_text_get_cursor_locations (self, &strong_x, NULL);
      cursor = strong_x - priv->scroll_offset;

      if (priv->selection_bound != priv->current_pos)
        {
          int start, end;

          bound = bobgui_text_get_selection_bound_location (self) - priv->scroll_offset;

          if (priv->selection_bound > priv->current_pos)
            {
              start = cursor;
              end = bound;
            }
          else
            {
              start = bound;
              end = cursor;
            }

          /* Update start selection bound */
          bobgui_text_handle_set_role (priv->text_handles[TEXT_HANDLE_SELECTION_BOUND],
                                    BOBGUI_TEXT_HANDLE_ROLE_SELECTION_END);
          bobgui_text_move_handle (self,
                                priv->text_handles[TEXT_HANDLE_SELECTION_BOUND],
                                end, 0, text_height);
          bobgui_text_handle_set_role (priv->text_handles[TEXT_HANDLE_CURSOR],
                                    BOBGUI_TEXT_HANDLE_ROLE_SELECTION_START);
          bobgui_text_move_handle (self,
                                priv->text_handles[TEXT_HANDLE_CURSOR],
                                start, 0, text_height);
        }
      else
        {
          bobgui_widget_set_visible (BOBGUI_WIDGET (priv->text_handles[TEXT_HANDLE_SELECTION_BOUND]), FALSE);
          bobgui_text_handle_set_role (priv->text_handles[TEXT_HANDLE_CURSOR],
                                    BOBGUI_TEXT_HANDLE_ROLE_CURSOR);
          bobgui_text_move_handle (self,
                                priv->text_handles[TEXT_HANDLE_CURSOR],
                                cursor, 0, text_height);
        }
    }
}

static void
bobgui_text_measure (BobguiWidget      *widget,
                  BobguiOrientation  orientation,
                  int             for_size,
                  int            *minimum,
                  int            *natural,
                  int            *minimum_baseline,
                  int            *natural_baseline)
{
  BobguiText *self = BOBGUI_TEXT (widget);
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  PangoContext *context;
  PangoFontMetrics *metrics;

  context = bobgui_widget_get_pango_context (widget);
  metrics = pango_context_get_metrics (context, NULL, NULL);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      int min, nat;
      int char_width;
      int digit_width;
      int char_pixels;

      char_width = pango_font_metrics_get_approximate_char_width (metrics);
      digit_width = pango_font_metrics_get_approximate_digit_width (metrics);
      char_pixels = (MAX (char_width, digit_width) + PANGO_SCALE - 1) / PANGO_SCALE;

      if (priv->width_chars >= 0)
        min = char_pixels * priv->width_chars;
      else
        min = 0;

      if (priv->max_width_chars < 0)
        nat = NAT_ENTRY_WIDTH;
      else
        nat = char_pixels * priv->max_width_chars;

      if (priv->propagate_text_width)
        {
          PangoLayout *layout;
          int act;

          layout = bobgui_text_ensure_layout (self, TRUE);
          pango_layout_get_pixel_size (layout, &act, NULL);

          nat = MIN (act, nat);
        }

      nat = MAX (min, nat);

      if (priv->placeholder)
        {
          int pmin, pnat;

          bobgui_widget_measure (priv->placeholder, BOBGUI_ORIENTATION_HORIZONTAL, -1,
                              &pmin, &pnat, NULL, NULL);
          min = MAX (min, pmin);
          nat = MAX (nat, pnat);
        }

      *minimum = min;
      *natural = nat;
    }
  else
    {
      int height, baseline;
      PangoLayout *layout;

      layout = bobgui_text_ensure_layout (self, TRUE);

      priv->ascent = pango_font_metrics_get_ascent (metrics);
      priv->descent = pango_font_metrics_get_descent (metrics);

      pango_layout_get_pixel_size (layout, NULL, &height);

      height = MAX (height, PANGO_PIXELS (priv->ascent + priv->descent));

      baseline = pango_layout_get_baseline (layout) / PANGO_SCALE;

      *minimum = *natural = height;

      if (priv->placeholder)
        {
          int min, nat;

          bobgui_widget_measure (priv->placeholder, BOBGUI_ORIENTATION_VERTICAL, -1,
                              &min, &nat, NULL, NULL);
          *minimum = MAX (*minimum, min);
          *natural = MAX (*natural, nat);
        }

      if (minimum_baseline)
        *minimum_baseline = baseline;
      if (natural_baseline)
        *natural_baseline = baseline;
    }

  pango_font_metrics_unref (metrics);
}

static void
bobgui_text_size_allocate (BobguiWidget *widget,
                        int        width,
                        int        height,
                        int        baseline)
{
  BobguiText *self = BOBGUI_TEXT (widget);
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  BobguiEmojiChooser *chooser;

  priv->text_baseline = baseline;

  if (priv->placeholder)
    {
      bobgui_widget_size_allocate (priv->placeholder,
                                &(BobguiAllocation) { 0, 0, width, height },
                                -1);
    }

  bobgui_text_adjust_scroll (self);
  bobgui_text_check_cursor_blink (self);
  update_im_cursor_location (self);

  chooser = g_object_get_data (G_OBJECT (self), "bobgui-emoji-chooser");
  if (chooser)
    bobgui_popover_present (BOBGUI_POPOVER (chooser));

  bobgui_text_update_handles (self);

  if (priv->emoji_completion)
    bobgui_popover_present (BOBGUI_POPOVER (priv->emoji_completion));

  if (priv->magnifier_popover)
    bobgui_popover_present (BOBGUI_POPOVER (priv->magnifier_popover));

  if (priv->popup_menu)
    bobgui_popover_present (BOBGUI_POPOVER (priv->popup_menu));

  if (priv->selection_bubble)
    bobgui_popover_present (BOBGUI_POPOVER (priv->selection_bubble));

  if (priv->text_handles[TEXT_HANDLE_CURSOR])
    bobgui_text_handle_present (priv->text_handles[TEXT_HANDLE_CURSOR]);

  if (priv->text_handles[TEXT_HANDLE_SELECTION_BOUND])
    bobgui_text_handle_present (priv->text_handles[TEXT_HANDLE_SELECTION_BOUND]);
}

static void
bobgui_text_draw_undershoot (BobguiText     *self,
                          BobguiSnapshot *snapshot)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  const int text_width = bobgui_widget_get_width (BOBGUI_WIDGET (self));
  const int text_height = bobgui_widget_get_height (BOBGUI_WIDGET (self));
  BobguiCssStyle *style;
  int min_offset, max_offset;
  BobguiCssBoxes boxes;

  bobgui_text_get_scroll_limits (self, &min_offset, &max_offset);

  if (priv->scroll_offset > min_offset)
    {
      style = bobgui_css_node_get_style (priv->undershoot_node[0]);
      bobgui_css_boxes_init_border_box (&boxes, style, 0, 0, UNDERSHOOT_SIZE, text_height);
      bobgui_css_style_snapshot_background (&boxes, snapshot);
      bobgui_css_style_snapshot_border (&boxes, snapshot);
    }

  if (priv->scroll_offset < max_offset)
    {
      style = bobgui_css_node_get_style (priv->undershoot_node[1]);
      bobgui_css_boxes_init_border_box (&boxes, style,
                                     text_width - UNDERSHOOT_SIZE, 0, UNDERSHOOT_SIZE, text_height);
      bobgui_css_style_snapshot_background (&boxes, snapshot);
      bobgui_css_style_snapshot_border (&boxes, snapshot);
    }
}

static void
bobgui_text_snapshot (BobguiWidget   *widget,
                   BobguiSnapshot *snapshot)
{
  BobguiText *self = BOBGUI_TEXT (widget);
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  /* Draw text and cursor */
  if (priv->dnd_position != -1)
    bobgui_text_draw_cursor (self, snapshot, CURSOR_DND);

  if (priv->placeholder)
    bobgui_widget_snapshot_child (widget, priv->placeholder, snapshot);

  bobgui_text_draw_text (self, snapshot);

  /* When no text is being displayed at all, don't show the cursor */
  if (bobgui_text_get_display_mode (self) != DISPLAY_BLANK &&
      bobgui_widget_has_focus (widget) &&
      priv->selection_bound == priv->current_pos)
    {
      bobgui_snapshot_push_opacity (snapshot, priv->cursor_alpha);
      bobgui_text_draw_cursor (self, snapshot, CURSOR_STANDARD);
      bobgui_snapshot_pop (snapshot);
    }

  bobgui_text_draw_undershoot (self, snapshot);
}

static void
bobgui_text_get_pixel_ranges (BobguiText  *self,
                           int     **ranges,
                           int      *n_ranges)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->selection_bound != priv->current_pos)
    {
      PangoLayout *layout = bobgui_text_ensure_layout (self, TRUE);
      PangoLayoutLine *line = pango_layout_get_lines_readonly (layout)->data;
      const char *text = pango_layout_get_text (layout);
      int start_index = g_utf8_offset_to_pointer (text, priv->selection_bound) - text;
      int end_index = g_utf8_offset_to_pointer (text, priv->current_pos) - text;
      int real_n_ranges, i;

      pango_layout_line_get_x_ranges (line,
                                      MIN (start_index, end_index),
                                      MAX (start_index, end_index),
                                      ranges,
                                      &real_n_ranges);

      if (ranges)
        {
          int *r = *ranges;

          for (i = 0; i < real_n_ranges; ++i)
            {
              r[2 * i + 1] = (r[2 * i + 1] - r[2 * i]) / PANGO_SCALE;
              r[2 * i] = r[2 * i] / PANGO_SCALE;
            }
        }

      if (n_ranges)
        *n_ranges = real_n_ranges;
    }
  else
    {
      if (n_ranges)
        *n_ranges = 0;
      if (ranges)
        *ranges = NULL;
    }
}

static gboolean
in_selection (BobguiText *self,
              int      x)
{
  int *ranges;
  int n_ranges, i;
  int retval = FALSE;

  bobgui_text_get_pixel_ranges (self, &ranges, &n_ranges);

  for (i = 0; i < n_ranges; ++i)
    {
      if (x >= ranges[2 * i] && x < ranges[2 * i] + ranges[2 * i + 1])
        {
          retval = TRUE;
          break;
        }
    }

  g_free (ranges);
  return retval;
}

static int
gesture_get_current_point_in_layout (BobguiGestureSingle *gesture,
                                     BobguiText          *self)
{
  int tx;
  GdkEventSequence *sequence;
  double px;

  sequence = bobgui_gesture_single_get_current_sequence (gesture);
  bobgui_gesture_get_point (BOBGUI_GESTURE (gesture), sequence, &px, NULL);
  bobgui_text_get_layout_offsets (self, &tx, NULL);

  return px - tx;
}

static void
bobgui_text_do_popup (BobguiText *self,
                   double   x,
                   double   y)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  bobgui_text_update_clipboard_actions (self);
  bobgui_text_update_emoji_action (self);

  if (!priv->popup_menu)
    {
      GMenuModel *model;

      model = bobgui_text_get_menu_model (self);
      priv->popup_menu = bobgui_popover_menu_new_from_model (model);
      bobgui_widget_set_parent (priv->popup_menu, BOBGUI_WIDGET (self));
      bobgui_popover_set_position (BOBGUI_POPOVER (priv->popup_menu), BOBGUI_POS_BOTTOM);

      bobgui_popover_set_has_arrow (BOBGUI_POPOVER (priv->popup_menu), FALSE);
      bobgui_widget_set_halign (priv->popup_menu, BOBGUI_ALIGN_START);

      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (priv->popup_menu),
                                      BOBGUI_ACCESSIBLE_PROPERTY_LABEL, _("Context menu"),
                                      -1);

      g_object_unref (model);
    }

  if (x != -1 && y != -1)
    {
      GdkRectangle rect = { x, y, 1, 1 };
      bobgui_popover_set_pointing_to (BOBGUI_POPOVER (priv->popup_menu), &rect);
    }
  else
    bobgui_popover_set_pointing_to (BOBGUI_POPOVER (priv->popup_menu), NULL);

  bobgui_popover_popup (BOBGUI_POPOVER (priv->popup_menu));
}

static void
bobgui_text_click_gesture_pressed (BobguiGestureClick *gesture,
                                int              n_press,
                                double           widget_x,
                                double           widget_y,
                                BobguiText         *self)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  GdkEventSequence *current;
  GdkEvent *event;
  int x, y, sel_start, sel_end;
  guint button;
  int tmp_pos;

  if (!bobgui_widget_has_focus (widget))
    {
      if (!bobgui_widget_get_focus_on_click (widget))
        return;
      priv->in_click = TRUE;
      bobgui_widget_grab_focus (widget);
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
      priv->in_click = FALSE;
    }

  button = bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (gesture));
  current = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  event = bobgui_gesture_get_last_event (BOBGUI_GESTURE (gesture), current);

  x = gesture_get_current_point_in_layout (BOBGUI_GESTURE_SINGLE (gesture), self);
  y = widget_y;
  bobgui_text_reset_blink_time (self);

  tmp_pos = bobgui_text_find_position (self, x);

  if (gdk_event_triggers_context_menu (event))
    {
      bobgui_text_do_popup (self, widget_x, widget_y);
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
    }
  else if (n_press == 1 && button == GDK_BUTTON_MIDDLE &&
           get_middle_click_paste (self))
    {
      if (priv->editable)
        {
          priv->insert_pos = tmp_pos;
          bobgui_text_paste (self, bobgui_widget_get_primary_clipboard (widget));
        }
      else
        {
          bobgui_widget_error_bell (widget);
        }

      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
    }
  else if (button == GDK_BUTTON_PRIMARY)
    {
      gboolean have_selection;
      gboolean is_touchscreen, extend_selection;
      GdkDisplay *display;
      GdkDevice *source;
      guint state;

      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);

      sel_start = priv->selection_bound;
      sel_end = priv->current_pos;
      have_selection = sel_start != sel_end;

      display = bobgui_widget_get_display (widget);
      source = gdk_event_get_device (event);
      is_touchscreen = BOBGUI_DISPLAY_DEBUG_CHECK (display, TOUCHSCREEN) ||
                       gdk_device_get_source (source) == GDK_SOURCE_TOUCHSCREEN;

      priv->text_handles_enabled = is_touchscreen;

      priv->in_drag = FALSE;
      priv->select_words = FALSE;
      priv->select_lines = FALSE;

      state = gdk_event_get_modifier_state (event);

      extend_selection = (state & GDK_SHIFT_MASK) != 0;

      /* Always emit reset when preedit is shown */
      priv->need_im_reset = TRUE;
      bobgui_text_reset_im_context (self);

      switch (n_press)
        {
        case 1:
          if (in_selection (self, x))
            {
              if (is_touchscreen)
                {
                  if (priv->selection_bubble &&
                      bobgui_widget_get_visible (priv->selection_bubble))
                    bobgui_text_selection_bubble_popup_unset (self);
                  else
                    bobgui_text_selection_bubble_popup_set (self);
                }
              else if (extend_selection)
                {
                  /* Truncate current selection, but keep it as big as possible */
                  if (tmp_pos - sel_start > sel_end - tmp_pos)
                    bobgui_text_set_positions (self, sel_start, tmp_pos);
                  else
                    bobgui_text_set_positions (self, tmp_pos, sel_end);

                  /* all done, so skip the extend_to_left stuff later */
                  extend_selection = FALSE;
                }
              else
                {
                  /* We'll either start a drag, or clear the selection */
                  priv->in_drag = TRUE;
                  priv->drag_start_x = x;
                  priv->drag_start_y = y;
                }
            }
          else
            {
              bobgui_text_selection_bubble_popup_unset (self);

              if (!extend_selection)
                {
                  bobgui_text_set_selection_bounds (self, tmp_pos, tmp_pos);
                  priv->handle_place_time = g_get_monotonic_time ();
                }
              else
                {
                  /* select from the current position to the clicked position */
                  if (!have_selection)
                    sel_start = sel_end = priv->current_pos;

                  bobgui_text_set_positions (self, tmp_pos, tmp_pos);
                }
            }

          break;

        case 2:
          priv->select_words = TRUE;
          bobgui_text_select_word (self);
          break;

        case 3:
          priv->select_lines = TRUE;
          bobgui_text_select_line (self);
          break;

        default:
          break;
        }

      if (extend_selection)
        {
          gboolean extend_to_left;
          int start, end;

          start = MIN (priv->current_pos, priv->selection_bound);
          start = MIN (sel_start, start);

          end = MAX (priv->current_pos, priv->selection_bound);
          end = MAX (sel_end, end);

          if (tmp_pos == sel_start || tmp_pos == sel_end)
            extend_to_left = (tmp_pos == start);
          else
            extend_to_left = (end == sel_end);

          if (extend_to_left)
            bobgui_text_set_positions (self, start, end);
          else
            bobgui_text_set_positions (self, end, start);
        }

      bobgui_text_update_handles (self);
    }

  if (n_press >= 3)
    bobgui_event_controller_reset (BOBGUI_EVENT_CONTROLLER (gesture));
}

static void
bobgui_text_click_gesture_released (BobguiGestureClick *gesture,
                                 int              n_press,
                                 double           widget_x,
                                 double           widget_y,
                                 BobguiText         *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  GdkEvent *event =
    bobgui_event_controller_get_current_event (BOBGUI_EVENT_CONTROLLER (gesture));

  if (n_press == 1 &&
      !priv->in_drag &&
      priv->current_pos == priv->selection_bound)
    bobgui_im_context_activate_osk (priv->im_context, event);
}

static char *
_bobgui_text_get_selected_text (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->selection_bound != priv->current_pos)
    {
      const int start = MIN (priv->selection_bound, priv->current_pos);
      const int end = MAX (priv->selection_bound, priv->current_pos);
      const char *text = bobgui_entry_buffer_get_text (get_buffer (self));
      const int start_index = g_utf8_offset_to_pointer (text, start) - text;
      const int end_index = g_utf8_offset_to_pointer (text, end) - text;

      return g_strndup (text + start_index, end_index - start_index);
    }

  return NULL;
}

static void
bobgui_text_show_magnifier (BobguiText *self,
                         int      x,
                         int      y)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  const int text_height = bobgui_widget_get_height (BOBGUI_WIDGET (self));
  cairo_rectangle_int_t rect;

  bobgui_text_ensure_magnifier (self);

  rect.x = x;
  rect.width = 1;
  rect.y = 0;
  rect.height = text_height;

  _bobgui_magnifier_set_coords (BOBGUI_MAGNIFIER (priv->magnifier), rect.x,
                             rect.y + rect.height / 2);
  bobgui_popover_set_pointing_to (BOBGUI_POPOVER (priv->magnifier_popover),
                               &rect);
  bobgui_popover_popup (BOBGUI_POPOVER (priv->magnifier_popover));
}

static void
bobgui_text_motion_controller_motion (BobguiEventControllerMotion *controller,
                                   double                    x,
                                   double                    y,
                                   BobguiText                  *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  GdkDevice *device;

  device = bobgui_event_controller_get_current_event_device (BOBGUI_EVENT_CONTROLLER (controller));

  if (priv->mouse_cursor_obscured &&
      gdk_device_get_timestamp (device) != priv->obscured_cursor_timestamp)
    {
      set_text_cursor (BOBGUI_WIDGET (self));
      priv->mouse_cursor_obscured = FALSE;
    }
}

static void
dnd_finished_cb (GdkDrag *drag,
                 BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (gdk_drag_get_selected_action (drag) == GDK_ACTION_MOVE)
    bobgui_text_delete_selection (self);

  priv->drag = NULL;
}

static void
dnd_cancel_cb (GdkDrag             *drag,
               GdkDragCancelReason  reason,
               BobguiText             *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  priv->drag = NULL;
}

static void
bobgui_text_drag_gesture_update (BobguiGestureDrag *gesture,
                              double          offset_x,
                              double          offset_y,
                              BobguiText        *self)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  GdkEventSequence *sequence;
  GdkEvent *event;
  int x, y;
  double start_y;

  bobgui_text_selection_bubble_popup_unset (self);

  x = gesture_get_current_point_in_layout (BOBGUI_GESTURE_SINGLE (gesture), self);
  bobgui_gesture_drag_get_start_point (gesture, NULL, &start_y);
  y = start_y + offset_y;

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  event = bobgui_gesture_get_last_event (BOBGUI_GESTURE (gesture), sequence);

  if (priv->mouse_cursor_obscured)
    {
      set_text_cursor (widget);
      priv->mouse_cursor_obscured = FALSE;
    }

  if (priv->select_lines)
    return;

  if (priv->in_drag)
    {
      if (bobgui_text_get_display_mode (self) == DISPLAY_NORMAL &&
          bobgui_drag_check_threshold_double (widget, 0, 0, offset_x, offset_y))
        {
          int *ranges;
          int n_ranges;
          char *text;
          GdkDragAction actions;
          GdkDrag *drag;
          GdkPaintable *paintable;
          GdkContentProvider *content;

          text = _bobgui_text_get_selected_text (self);
          bobgui_text_get_pixel_ranges (self, &ranges, &n_ranges);

          g_assert (n_ranges > 0);

          if (priv->editable)
            actions = GDK_ACTION_COPY|GDK_ACTION_MOVE;
          else
            actions = GDK_ACTION_COPY;

          content = gdk_content_provider_new_typed (G_TYPE_STRING, text);

          drag = gdk_drag_begin (gdk_event_get_surface ((GdkEvent*) event),
                                 gdk_event_get_device ((GdkEvent*) event),
                                 content,
                                 actions,
                                 priv->drag_start_x,
                                 priv->drag_start_y);
          g_object_unref (content);

          g_signal_connect (drag, "dnd-finished", G_CALLBACK (dnd_finished_cb), self);
          g_signal_connect (drag, "cancel", G_CALLBACK (dnd_cancel_cb), self);

          paintable = bobgui_text_util_create_drag_icon (widget, text, -1);
          bobgui_drag_icon_set_from_paintable (drag, paintable,
                                            (priv->drag_start_x - ranges[0]),
                                            priv->drag_start_y);
          g_clear_object (&paintable);

          priv->drag = drag;

          g_object_unref (drag);

          g_free (ranges);
          g_free (text);

          priv->in_drag = FALSE;

          /* Deny the gesture so we don't get further updates */
          bobgui_gesture_set_state (priv->drag_gesture, BOBGUI_EVENT_SEQUENCE_DENIED);
        }
    }
  else
    {
      GdkInputSource input_source;
      GdkDisplay *display;
      GdkDevice *source;
      guint length;
      int tmp_pos;
      int pos, bound;

      length = bobgui_entry_buffer_get_length (get_buffer (self));

      if (y < 0)
        tmp_pos = 0;
      else if (y >= bobgui_widget_get_height (BOBGUI_WIDGET (self)))
        tmp_pos = length;
      else
        tmp_pos = bobgui_text_find_position (self, x);

      display = bobgui_widget_get_display (BOBGUI_WIDGET (self));
      source = gdk_event_get_device (event);
      input_source = gdk_device_get_source (source);

      if (priv->select_words)
        {
          int min, max;
          int old_min, old_max;

          min = bobgui_text_move_backward_word (self, tmp_pos, TRUE);
          max = bobgui_text_move_forward_word (self, tmp_pos, TRUE);

          pos = priv->current_pos;
          bound = priv->selection_bound;

          old_min = MIN (priv->current_pos, priv->selection_bound);
          old_max = MAX (priv->current_pos, priv->selection_bound);

          if (min < old_min)
            {
              pos = min;
              bound = old_max;
            }
          else if (old_max < max)
            {
              pos = max;
              bound = old_min;
            }
          else if (pos == old_min)
            {
              if (priv->current_pos != min)
                pos = max;
            }
          else
            {
              if (priv->current_pos != max)
                pos = min;
            }
        }
      else
        {
          pos = tmp_pos;
          bound = -1;
        }

      if (pos != priv->current_pos)
        bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);

      bobgui_text_set_positions (self, pos, bound);

      /* Update touch handles' position */
      if (BOBGUI_DISPLAY_DEBUG_CHECK (display, TOUCHSCREEN) ||
          input_source == GDK_SOURCE_TOUCHSCREEN)
        {
          priv->text_handles_enabled = TRUE;
          bobgui_text_update_handles (self);
          bobgui_text_show_magnifier (self, x - priv->scroll_offset, y);
        }
    }
}

static void
bobgui_text_drag_gesture_end (BobguiGestureDrag *gesture,
                           double          offset_x,
                           double          offset_y,
                           BobguiText        *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  gboolean in_drag;
  GdkEventSequence *sequence;

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  in_drag = priv->in_drag;
  priv->in_drag = FALSE;

  if (priv->magnifier_popover)
    bobgui_popover_popdown (BOBGUI_POPOVER (priv->magnifier_popover));

  /* Check whether the drag was cancelled rather than finished */
  if (!bobgui_gesture_handles_sequence (BOBGUI_GESTURE (gesture), sequence))
    return;

  if (in_drag)
    {
      int tmp_pos = bobgui_text_find_position (self, priv->drag_start_x);

      bobgui_text_set_selection_bounds (self, tmp_pos, tmp_pos);
    }

  bobgui_text_update_handles (self);

  bobgui_text_update_primary_selection (self);
}

static void
bobgui_text_obscure_mouse_cursor (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  GdkDisplay *display;
  GdkSeat *seat;
  GdkDevice *device;

  if (priv->mouse_cursor_obscured)
    return;

  bobgui_widget_set_cursor_from_name (BOBGUI_WIDGET (self), "none");

  display = bobgui_widget_get_display (BOBGUI_WIDGET (self));
  seat = gdk_display_get_default_seat (display);
  device = gdk_seat_get_pointer (seat);

  priv->obscured_cursor_timestamp = gdk_device_get_timestamp (device);
  priv->mouse_cursor_obscured = TRUE;
}

static gboolean
bobgui_text_key_controller_key_pressed (BobguiEventControllerKey *controller,
                                      guint                  keyval,
                                      guint                  keycode,
                                      GdkModifierType        state,
                                      BobguiText               *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  gunichar unichar;

  bobgui_text_reset_blink_time (self);
  bobgui_text_pend_cursor_blink (self);

  bobgui_text_selection_bubble_popup_unset (self);

  priv->text_handles_enabled = FALSE;
  bobgui_text_update_handles (self);

  if (keyval == GDK_KEY_Return ||
      keyval == GDK_KEY_KP_Enter ||
      keyval == GDK_KEY_ISO_Enter ||
      keyval == GDK_KEY_Escape)
    bobgui_text_reset_im_context (self);

  unichar = gdk_keyval_to_unicode (keyval);

  if (!priv->editable && unichar != 0)
    bobgui_widget_error_bell (BOBGUI_WIDGET (self));

  bobgui_text_obscure_mouse_cursor (self);

  return FALSE;
}

static void
bobgui_text_focus_changed (BobguiEventControllerFocus *controller,
                        GParamSpec              *pspec,
                        BobguiWidget               *widget)
{
  BobguiText *self = BOBGUI_TEXT (widget);
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  bobgui_widget_queue_draw (widget);

  if (bobgui_event_controller_focus_is_focus (controller))
    {
      bobgui_text_im_set_focus_in (self);
      bobgui_text_reset_blink_time (self);
      bobgui_text_check_cursor_blink (self);
      bobgui_text_update_primary_selection (self);
    }
  else /* Focus out */
    {
      bobgui_text_selection_bubble_popup_unset (self);

      priv->text_handles_enabled = FALSE;
      bobgui_text_update_handles (self);

      if (priv->editable)
        {
          bobgui_text_schedule_im_reset (self);
          bobgui_im_context_focus_out (priv->im_context);
        }

      if (priv->blink_tick)
        remove_blink_timeout (self);
    }
}

static gboolean
bobgui_text_grab_focus (BobguiWidget *widget)
{
  BobguiText *self = BOBGUI_TEXT (widget);
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  gboolean select_on_focus;
  BobguiWidget *prev_focus;
  gboolean prev_focus_was_child;

  prev_focus = bobgui_root_get_focus (bobgui_widget_get_root (widget));
  prev_focus_was_child = prev_focus && bobgui_widget_is_ancestor (prev_focus, widget);

  if (!BOBGUI_WIDGET_CLASS (bobgui_text_parent_class)->grab_focus (BOBGUI_WIDGET (self)))
    return FALSE;

  if (priv->editable && !priv->in_click && !prev_focus_was_child)
    {
      g_object_get (bobgui_widget_get_settings (widget),
                    "bobgui-entry-select-on-focus",
                    &select_on_focus,
                    NULL);

      if (select_on_focus)
        bobgui_text_set_selection_bounds (self, 0, -1);
    }

  return TRUE;
}

/**
 * bobgui_text_grab_focus_without_selecting:
 * @self: a text widget
 *
 * Causes the text widget to have the keyboard focus.
 *
 * It behaves like [method@Bobgui.Widget.grab_focus],
 * except that it does not select the contents of @self.
 *
 * You only want to call this on some special entries
 * which the user usually doesn't want to replace all
 * text in, such as search-as-you-type entries.
 *
 * Returns: true if focus is now inside @self
 */
gboolean
bobgui_text_grab_focus_without_selecting (BobguiText *self)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT (self), FALSE);

  return BOBGUI_WIDGET_CLASS (bobgui_text_parent_class)->grab_focus (BOBGUI_WIDGET (self));
}

static void
bobgui_text_direction_changed (BobguiWidget        *widget,
                            BobguiTextDirection  previous_dir)
{
  BobguiText *self = BOBGUI_TEXT (widget);

  bobgui_text_recompute (self);

  BOBGUI_WIDGET_CLASS (bobgui_text_parent_class)->direction_changed (widget, previous_dir);
}

static void
bobgui_text_state_flags_changed (BobguiWidget     *widget,
                              BobguiStateFlags  previous_state)
{
  BobguiText *self = BOBGUI_TEXT (widget);
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  BobguiStateFlags state;

  state = bobgui_widget_get_state_flags (BOBGUI_WIDGET (self));

  if (bobgui_widget_get_realized (widget))
    {
      set_text_cursor (widget);
      priv->mouse_cursor_obscured = FALSE;
    }

  if (!bobgui_widget_is_sensitive (widget))
    {
      /* Clear any selection */
      bobgui_text_set_selection_bounds (self, priv->current_pos, priv->current_pos);

      /* Stop blinking */
      remove_blink_timeout (self);
    }

  state &= ~BOBGUI_STATE_FLAG_DROP_ACTIVE;
  if (priv->selection_node)
    bobgui_css_node_set_state (priv->selection_node, state);

  if (priv->block_cursor_node)
    bobgui_css_node_set_state (priv->block_cursor_node, state);

  bobgui_css_node_set_state (priv->undershoot_node[0], state);
  bobgui_css_node_set_state (priv->undershoot_node[1], state);

  bobgui_text_update_cached_style_values (self);

  bobgui_widget_queue_draw (widget);
}

/* BobguiEditable method implementations
 */
static void
bobgui_text_insert_text (BobguiText    *self,
                      const char *text,
                      int         length,
                      int        *position)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  int n_inserted;
  int n_chars;

  if (length == 0)
    return;

  n_chars = g_utf8_strlen (text, length);

  /*
   * The incoming text may a password or other secret. We make sure
   * not to copy it into temporary buffers.
   */
  if (priv->change_count == 0)
    bobgui_text_history_begin_irreversible_action (priv->history);
  begin_change (self);

  n_inserted = bobgui_entry_buffer_insert_text (get_buffer (self), *position, text, n_chars);

  end_change (self);
  if (priv->change_count == 0)
    bobgui_text_history_end_irreversible_action (priv->history);

  if (n_inserted != n_chars)
    bobgui_widget_error_bell (BOBGUI_WIDGET (self));

  bobgui_accessible_text_update_contents (BOBGUI_ACCESSIBLE_TEXT (self),
                                       BOBGUI_ACCESSIBLE_TEXT_CONTENT_CHANGE_INSERT,
                                       *position,
                                       *position + n_inserted);

  *position += n_inserted;

  update_placeholder_visibility (self);
  if (priv->propagate_text_width)
    bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
}

static void
bobgui_text_delete_text (BobguiText *self,
                      int      start_pos,
                      int      end_pos)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (end_pos < 0)
    end_pos = bobgui_entry_buffer_get_length (get_buffer (self));

  if (start_pos == end_pos)
    return;

  bobgui_accessible_text_update_contents (BOBGUI_ACCESSIBLE_TEXT (self),
                                       BOBGUI_ACCESSIBLE_TEXT_CONTENT_CHANGE_REMOVE,
                                       start_pos,
                                       end_pos);

  if (priv->change_count == 0)
    bobgui_text_history_begin_irreversible_action (priv->history);
  begin_change (self);

  bobgui_entry_buffer_delete_text (get_buffer (self), start_pos, end_pos - start_pos);

  end_change (self);
  if (priv->change_count == 0)
    bobgui_text_history_end_irreversible_action (priv->history);

  update_placeholder_visibility (self);
  if (priv->propagate_text_width)
    bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
}

static void
bobgui_text_delete_selection (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  int start_pos = MIN (priv->selection_bound, priv->current_pos);
  int end_pos = MAX (priv->selection_bound, priv->current_pos);

  bobgui_editable_delete_text (BOBGUI_EDITABLE (self), start_pos, end_pos);
  bobgui_accessible_text_update_contents (BOBGUI_ACCESSIBLE_TEXT (self),
                                       BOBGUI_ACCESSIBLE_TEXT_CONTENT_CHANGE_REMOVE,
                                       start_pos, end_pos);
}

static void
bobgui_text_set_selection_bounds (BobguiText *self,
                               int      start,
                               int      end)
{
  guint length;

  length = bobgui_entry_buffer_get_length (get_buffer (self));
  if (start < 0)
    start = length;
  if (end < 0)
    end = length;

  bobgui_text_reset_im_context (self);

  bobgui_text_set_positions (self, MIN (end, length), MIN (start, length));
}

static gboolean
bobgui_text_get_selection_bounds (BobguiText *self,
                               int     *start,
                               int     *end)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  *start = priv->selection_bound;
  *end = priv->current_pos;

  return (priv->selection_bound != priv->current_pos);
}

static gunichar
find_invisible_char (BobguiWidget *widget)
{
  PangoLayout *layout;
  PangoAttrList *attr_list;
  int i;
  gunichar invisible_chars [] = {
    0x25cf, /* BLACK CIRCLE */
    0x2022, /* BULLET */
    0x2731, /* HEAVY ASTERISK */
    0x273a  /* SIXTEEN POINTED ASTERISK */
  };

  layout = bobgui_widget_create_pango_layout (widget, NULL);

  attr_list = pango_attr_list_new ();
  pango_attr_list_insert (attr_list, pango_attr_fallback_new (FALSE));

  pango_layout_set_attributes (layout, attr_list);
  pango_attr_list_unref (attr_list);

  for (i = 0; i < G_N_ELEMENTS (invisible_chars); i++)
    {
      char text[7] = { 0, };
      int len, count;

      len = g_unichar_to_utf8 (invisible_chars[i], text);
      pango_layout_set_text (layout, text, len);

      count = pango_layout_get_unknown_glyphs_count (layout);

      if (count == 0)
        {
          g_object_unref (layout);
          return invisible_chars[i];
        }
    }

  g_object_unref (layout);

  return '*';
}

static void
bobgui_text_update_cached_style_values (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (!priv->visible && !priv->invisible_char_set)
    {
      gunichar ch = find_invisible_char (BOBGUI_WIDGET (self));

      if (priv->invisible_char != ch)
        {
          priv->invisible_char = ch;
          g_object_notify_by_pspec (G_OBJECT (self), text_props[PROP_INVISIBLE_CHAR]);
        }
    }
}

static void
bobgui_text_css_changed (BobguiWidget         *widget,
                      BobguiCssStyleChange *change)
{
  BobguiText *self = BOBGUI_TEXT (widget);

  BOBGUI_WIDGET_CLASS (bobgui_text_parent_class)->css_changed (widget, change);

  bobgui_text_update_cached_style_values (self);

  if (bobgui_css_style_change_affects (change, BOBGUI_CSS_AFFECTS_TEXT |
                                            BOBGUI_CSS_AFFECTS_BACKGROUND |
                                            BOBGUI_CSS_AFFECTS_CONTENT))
    bobgui_widget_queue_draw (widget);

  if (bobgui_css_style_change_affects (change, BOBGUI_CSS_AFFECTS_TEXT_ATTRS))
    bobgui_text_recompute (self);
}

static void
bobgui_text_password_hint_free (BobguiTextPasswordHint *password_hint)
{
  if (password_hint->source_id)
    g_source_remove (password_hint->source_id);

  g_free (password_hint);
}


static gboolean
bobgui_text_remove_password_hint (gpointer data)
{
  BobguiTextPasswordHint *password_hint = g_object_get_qdata (data, quark_password_hint);
  password_hint->position = -1;
  password_hint->source_id = 0;

  /* Force the string to be redrawn, but now without a visible character */
  bobgui_text_recompute (BOBGUI_TEXT (data));

  return G_SOURCE_REMOVE;
}

static void
update_placeholder_visibility (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->placeholder)
    bobgui_widget_set_child_visible (priv->placeholder,
                                  priv->preedit_length == 0 &&
                                  (priv->buffer == NULL ||
                                   bobgui_entry_buffer_get_length (priv->buffer) == 0));
}

/* BobguiEntryBuffer signal handlers
 */
static void
buffer_inserted_text (BobguiEntryBuffer *buffer,
                      guint           position,
                      const char     *chars,
                      guint           n_chars,
                      BobguiText        *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  guint password_hint_timeout;
  guint current_pos;
  int selection_bound;

  current_pos = priv->current_pos;
  if (current_pos > position)
    current_pos += n_chars;

  selection_bound = priv->selection_bound;
  if (selection_bound > position)
    selection_bound += n_chars;

  bobgui_text_set_positions (self, current_pos, selection_bound);
  bobgui_text_recompute (self);

  bobgui_text_history_text_inserted (priv->history, position, chars, -1);

  /* Calculate the password hint if it needs to be displayed. */
  if (n_chars == 1 && !priv->visible)
    {
      g_object_get (bobgui_widget_get_settings (BOBGUI_WIDGET (self)),
                    "bobgui-entry-password-hint-timeout", &password_hint_timeout,
                    NULL);

      if (password_hint_timeout > 0)
        {
          BobguiTextPasswordHint *password_hint = g_object_get_qdata (G_OBJECT (self),
                                                                    quark_password_hint);
          if (!password_hint)
            {
              password_hint = g_new0 (BobguiTextPasswordHint, 1);
              g_object_set_qdata_full (G_OBJECT (self), quark_password_hint, password_hint,
                                       (GDestroyNotify)bobgui_text_password_hint_free);
            }

          password_hint->position = position;
          if (password_hint->source_id)
            g_source_remove (password_hint->source_id);
          password_hint->source_id = g_timeout_add (password_hint_timeout,
                                                    (GSourceFunc)bobgui_text_remove_password_hint,
                                                    self);
          gdk_source_set_static_name_by_id (password_hint->source_id, "[bobgui] bobgui_text_remove_password_hint");
        }
    }
}

static void
buffer_deleted_text (BobguiEntryBuffer *buffer,
                     guint           position,
                     guint           n_chars,
                     BobguiText        *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  guint end_pos = position + n_chars;

  if (bobgui_text_history_get_enabled (priv->history))
    {
      char *deleted_text;

      deleted_text = bobgui_editable_get_chars (BOBGUI_EDITABLE (self),
                                             position,
                                             end_pos);
      bobgui_text_history_selection_changed (priv->history,
                                          priv->current_pos,
                                          priv->selection_bound);
      bobgui_text_history_text_deleted (priv->history,
                                     position,
                                     end_pos,
                                     deleted_text,
                                     -1);

      g_free (deleted_text);
    }
}

static void
buffer_deleted_text_after (BobguiEntryBuffer *buffer,
                           guint           position,
                           guint           n_chars,
                           BobguiText        *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  guint end_pos = position + n_chars;
  int selection_bound;
  guint current_pos;

  current_pos = priv->current_pos;
  if (current_pos > position)
    current_pos -= MIN (current_pos, end_pos) - position;

  selection_bound = priv->selection_bound;
  if (selection_bound > position)
    selection_bound -= MIN (selection_bound, end_pos) - position;

  bobgui_text_set_positions (self, current_pos, selection_bound);
  bobgui_text_recompute (self);

  /* We might have deleted the selection */
  bobgui_text_update_primary_selection (self);

  /* Disable the password hint if one exists. */
  if (!priv->visible)
    {
      BobguiTextPasswordHint *password_hint = g_object_get_qdata (G_OBJECT (self),
                                                                quark_password_hint);
      if (password_hint)
        {
          if (password_hint->source_id)
            g_source_remove (password_hint->source_id);
          password_hint->source_id = 0;
          password_hint->position = -1;
        }
    }
}

static void
buffer_notify_text (BobguiEntryBuffer *buffer,
                    GParamSpec     *spec,
                    BobguiText        *self)
{
  emit_changed (self);
  update_placeholder_visibility (self);
  g_object_notify (G_OBJECT (self), "text");
}

static void
buffer_notify_max_length (BobguiEntryBuffer *buffer,
                          GParamSpec     *spec,
                          BobguiText        *self)
{
  g_object_notify_by_pspec (G_OBJECT (self), text_props[PROP_MAX_LENGTH]);
}

static void
buffer_connect_signals (BobguiText *self)
{
  g_signal_connect (get_buffer (self), "inserted-text", G_CALLBACK (buffer_inserted_text), self);
  g_signal_connect (get_buffer (self), "deleted-text", G_CALLBACK (buffer_deleted_text), self);
  g_signal_connect_after (get_buffer (self), "deleted-text", G_CALLBACK (buffer_deleted_text_after), self);
  g_signal_connect (get_buffer (self), "notify::text", G_CALLBACK (buffer_notify_text), self);
  g_signal_connect (get_buffer (self), "notify::max-length", G_CALLBACK (buffer_notify_max_length), self);
}

static void
buffer_disconnect_signals (BobguiText *self)
{
  g_signal_handlers_disconnect_by_func (get_buffer (self), buffer_inserted_text, self);
  g_signal_handlers_disconnect_by_func (get_buffer (self), buffer_deleted_text, self);
  g_signal_handlers_disconnect_by_func (get_buffer (self), buffer_deleted_text_after, self);
  g_signal_handlers_disconnect_by_func (get_buffer (self), buffer_notify_text, self);
  g_signal_handlers_disconnect_by_func (get_buffer (self), buffer_notify_max_length, self);
}

/* Compute the X position for an offset that corresponds to the "more important
 * cursor position for that offset. We use this when trying to guess to which
 * end of the selection we should go to when the user hits the left or
 * right arrow key.
 */
static int
get_better_cursor_x (BobguiText *self,
                     int      offset)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  GdkSeat *seat;
  GdkDevice *keyboard = NULL;
  PangoDirection direction = PANGO_DIRECTION_LTR;
  gboolean split_cursor;
  PangoLayout *layout = bobgui_text_ensure_layout (self, TRUE);
  const char *text = pango_layout_get_text (layout);
  int index = g_utf8_offset_to_pointer (text, offset) - text;
  PangoRectangle strong_pos, weak_pos;

  seat = gdk_display_get_default_seat (bobgui_widget_get_display (BOBGUI_WIDGET (self)));
  if (seat)
    keyboard = gdk_seat_get_keyboard (seat);
  if (keyboard)
    direction = gdk_device_get_direction (keyboard);

  g_object_get (bobgui_widget_get_settings (BOBGUI_WIDGET (self)),
                "bobgui-split-cursor", &split_cursor,
                NULL);

  pango_layout_get_cursor_pos (layout, index, &strong_pos, &weak_pos);

  if (split_cursor)
    return strong_pos.x / PANGO_SCALE;
  else
    return (direction == priv->resolved_dir) ? strong_pos.x / PANGO_SCALE : weak_pos.x / PANGO_SCALE;
}

static void
selection_style_changed_cb (BobguiCssNode        *node,
                            BobguiCssStyleChange *change,
                            BobguiText           *self)
{
  if (bobgui_css_style_change_affects (change, BOBGUI_CSS_AFFECTS_REDRAW))
    bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
}

static void
bobgui_text_move_cursor (BobguiText         *self,
                      BobguiMovementStep  step,
                      int              count,
                      gboolean         extend_selection)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  int new_pos = priv->current_pos;

  if (priv->current_pos != priv->selection_bound && !extend_selection)
    {
      /* If we have a current selection and aren't extending it, move to the
       * start/or end of the selection as appropriate
       */
      switch (step)
        {
        case BOBGUI_MOVEMENT_VISUAL_POSITIONS:
          {
            int current_x = get_better_cursor_x (self, priv->current_pos);
            int bound_x = get_better_cursor_x (self, priv->selection_bound);

            if (count <= 0)
              new_pos = current_x < bound_x ? priv->current_pos : priv->selection_bound;
            else
              new_pos = current_x > bound_x ? priv->current_pos : priv->selection_bound;
          }
          break;

        case BOBGUI_MOVEMENT_WORDS:
          if (priv->resolved_dir == PANGO_DIRECTION_RTL)
            count *= -1;
          G_GNUC_FALLTHROUGH;

        case BOBGUI_MOVEMENT_LOGICAL_POSITIONS:
          if (count < 0)
            new_pos = MIN (priv->current_pos, priv->selection_bound);
          else
            new_pos = MAX (priv->current_pos, priv->selection_bound);

          break;

        case BOBGUI_MOVEMENT_DISPLAY_LINE_ENDS:
        case BOBGUI_MOVEMENT_PARAGRAPH_ENDS:
        case BOBGUI_MOVEMENT_BUFFER_ENDS:
          new_pos = count < 0 ? 0 : bobgui_entry_buffer_get_length (get_buffer (self));
          break;

        case BOBGUI_MOVEMENT_DISPLAY_LINES:
        case BOBGUI_MOVEMENT_PARAGRAPHS:
        case BOBGUI_MOVEMENT_PAGES:
        case BOBGUI_MOVEMENT_HORIZONTAL_PAGES:
        default:
          break;
        }
    }
  else
    {
      switch (step)
        {
        case BOBGUI_MOVEMENT_LOGICAL_POSITIONS:
          new_pos = bobgui_text_move_logically (self, new_pos, count);
          break;

        case BOBGUI_MOVEMENT_VISUAL_POSITIONS:
          new_pos = bobgui_text_move_visually (self, new_pos, count);

          if (priv->current_pos == new_pos)
            {
              if (!extend_selection)
                {
                  if (!bobgui_widget_keynav_failed (BOBGUI_WIDGET (self),
                                                 count > 0 ?
                                                 BOBGUI_DIR_RIGHT : BOBGUI_DIR_LEFT))
                    {
                      BobguiWidget *toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (BOBGUI_WIDGET (self)));

                      if (toplevel)
                        bobgui_widget_child_focus (toplevel,
                                                count > 0 ?
                                                BOBGUI_DIR_RIGHT : BOBGUI_DIR_LEFT);
                    }
                }
              else
                {
                  bobgui_widget_error_bell (BOBGUI_WIDGET (self));
                }
            }
          break;

        case BOBGUI_MOVEMENT_WORDS:
          if (priv->resolved_dir == PANGO_DIRECTION_RTL)
            count *= -1;

          while (count > 0)
            {
              new_pos = bobgui_text_move_forward_word (self, new_pos, FALSE);
              count--;
            }

          while (count < 0)
            {
              new_pos = bobgui_text_move_backward_word (self, new_pos, FALSE);
              count++;
            }

          if (priv->current_pos == new_pos)
            bobgui_widget_error_bell (BOBGUI_WIDGET (self));

          break;

        case BOBGUI_MOVEMENT_DISPLAY_LINE_ENDS:
        case BOBGUI_MOVEMENT_PARAGRAPH_ENDS:
        case BOBGUI_MOVEMENT_BUFFER_ENDS:
          new_pos = count < 0 ? 0 : bobgui_entry_buffer_get_length (get_buffer (self));

          if (priv->current_pos == new_pos)
            bobgui_widget_error_bell (BOBGUI_WIDGET (self));

          break;

        case BOBGUI_MOVEMENT_DISPLAY_LINES:
        case BOBGUI_MOVEMENT_PARAGRAPHS:
        case BOBGUI_MOVEMENT_PAGES:
        case BOBGUI_MOVEMENT_HORIZONTAL_PAGES:
        default:
          break;
        }
    }

  if (extend_selection)
    bobgui_text_set_selection_bounds (self, priv->selection_bound, new_pos);
  else
    bobgui_text_set_selection_bounds (self, new_pos, new_pos);

  bobgui_text_pend_cursor_blink (self);

  priv->need_im_reset = TRUE;
  bobgui_text_reset_im_context (self);
}

static void
bobgui_text_insert_at_cursor (BobguiText    *self,
                           const char *str)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  int pos = priv->current_pos;

  if (priv->editable)
    {
      begin_change (self);
      bobgui_text_reset_im_context (self);
      bobgui_editable_insert_text (BOBGUI_EDITABLE (self), str, -1, &pos);
      bobgui_accessible_text_update_contents (BOBGUI_ACCESSIBLE_TEXT (self),
                                           BOBGUI_ACCESSIBLE_TEXT_CONTENT_CHANGE_INSERT,
                                           pos,
                                           pos + g_utf8_strlen (str, -1));
      bobgui_text_set_selection_bounds (self, pos, pos);
      end_change (self);
    }
}

static void
bobgui_text_delete_from_cursor (BobguiText       *self,
                             BobguiDeleteType  type,
                             int            count)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  int start_pos = priv->current_pos;
  int end_pos = priv->current_pos;
  int old_n_bytes = bobgui_entry_buffer_get_bytes (get_buffer (self));

  if (!priv->editable)
    {
      bobgui_widget_error_bell (BOBGUI_WIDGET (self));
      return;
    }

  begin_change (self);

  if (priv->selection_bound != priv->current_pos)
    {
      bobgui_text_delete_selection (self);
      bobgui_text_schedule_im_reset (self);
      bobgui_text_reset_im_context (self);
      goto done;
    }

  switch (type)
    {
    case BOBGUI_DELETE_CHARS:
      end_pos = bobgui_text_move_logically (self, priv->current_pos, count);
      bobgui_editable_delete_text (BOBGUI_EDITABLE (self), MIN (start_pos, end_pos), MAX (start_pos, end_pos));
      break;

    case BOBGUI_DELETE_WORDS:
      if (count < 0)
        {
          /* Move to end of current word, or if not on a word, end of previous word */
          end_pos = bobgui_text_move_backward_word (self, end_pos, FALSE);
          end_pos = bobgui_text_move_forward_word (self, end_pos, FALSE);
        }
      else if (count > 0)
        {
          /* Move to beginning of current word, or if not on a word, beginning of next word */
          start_pos = bobgui_text_move_forward_word (self, start_pos, FALSE);
          start_pos = bobgui_text_move_backward_word (self, start_pos, FALSE);
        }
      G_GNUC_FALLTHROUGH;
    case BOBGUI_DELETE_WORD_ENDS:
      while (count < 0)
        {
          start_pos = bobgui_text_move_backward_word (self, start_pos, FALSE);
          count++;
        }

      while (count > 0)
        {
          end_pos = bobgui_text_move_forward_word (self, end_pos, FALSE);
          count--;
        }

      bobgui_editable_delete_text (BOBGUI_EDITABLE (self), start_pos, end_pos);
      break;

    case BOBGUI_DELETE_DISPLAY_LINE_ENDS:
    case BOBGUI_DELETE_PARAGRAPH_ENDS:
      if (count < 0)
        bobgui_editable_delete_text (BOBGUI_EDITABLE (self), 0, priv->current_pos);
      else
        bobgui_editable_delete_text (BOBGUI_EDITABLE (self), priv->current_pos, -1);
      break;

    case BOBGUI_DELETE_DISPLAY_LINES:
    case BOBGUI_DELETE_PARAGRAPHS:
      bobgui_editable_delete_text (BOBGUI_EDITABLE (self), 0, -1);
      break;

    case BOBGUI_DELETE_WHITESPACE:
      bobgui_text_delete_whitespace (self);
      break;

    default:
      break;
    }

  if (bobgui_entry_buffer_get_bytes (get_buffer (self)) == old_n_bytes)
    bobgui_widget_error_bell (BOBGUI_WIDGET (self));
  else
    {
      bobgui_text_schedule_im_reset (self);
      bobgui_text_reset_im_context (self);
    }

done:
  end_change (self);
  bobgui_text_pend_cursor_blink (self);
}

static void
bobgui_text_backspace (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  int prev_pos;

  if (!priv->editable)
    {
      bobgui_widget_error_bell (BOBGUI_WIDGET (self));
      return;
    }

  begin_change (self);

  if (priv->selection_bound != priv->current_pos)
    {
      bobgui_text_delete_selection (self);
      bobgui_text_schedule_im_reset (self);
      bobgui_text_reset_im_context (self);
      goto done;
    }

  prev_pos = bobgui_text_move_logically (self, priv->current_pos, -1);

  if (prev_pos < priv->current_pos)
    {
      PangoLayout *layout = bobgui_text_ensure_layout (self, FALSE);
      const PangoLogAttr *log_attrs;
      int n_attrs;

      log_attrs = pango_layout_get_log_attrs_readonly (layout, &n_attrs);

      /* Deleting parts of characters */
      if (log_attrs[priv->current_pos].backspace_deletes_character)
        {
          char *cluster_text;
          char *normalized_text;
          glong  len;

          cluster_text = bobgui_text_get_display_text (self, prev_pos, priv->current_pos);
          normalized_text = g_utf8_normalize (cluster_text,
                                              strlen (cluster_text),
                                              G_NORMALIZE_NFD);
          len = g_utf8_strlen (normalized_text, -1);

          bobgui_editable_delete_text (BOBGUI_EDITABLE (self), prev_pos, priv->current_pos);
          if (len > 1)
            {
              int pos = priv->current_pos;

              bobgui_editable_insert_text (BOBGUI_EDITABLE (self), normalized_text,
                                        g_utf8_offset_to_pointer (normalized_text, len - 1) - normalized_text,
                                        &pos);
              bobgui_accessible_text_update_contents (BOBGUI_ACCESSIBLE_TEXT (self),
                                                   BOBGUI_ACCESSIBLE_TEXT_CONTENT_CHANGE_INSERT,
                                                   pos,
                                                   pos + len);
              bobgui_text_set_selection_bounds (self, pos, pos);
            }

          g_free (normalized_text);
          g_free (cluster_text);
        }
      else
        {
          bobgui_editable_delete_text (BOBGUI_EDITABLE (self), prev_pos, priv->current_pos);
        }

      bobgui_text_schedule_im_reset (self);
      bobgui_text_reset_im_context (self);
    }
  else
    {
      bobgui_widget_error_bell (BOBGUI_WIDGET (self));
    }

done:
  end_change (self);
  bobgui_text_pend_cursor_blink (self);
}

static void
bobgui_text_copy_clipboard (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->selection_bound != priv->current_pos)
    {
      char *str;

      if (!priv->visible)
        {
          bobgui_widget_error_bell (BOBGUI_WIDGET (self));
          return;
        }

      if (priv->selection_bound < priv->current_pos)
        str = bobgui_text_get_display_text (self, priv->selection_bound, priv->current_pos);
      else
        str = bobgui_text_get_display_text (self, priv->current_pos, priv->selection_bound);

      gdk_clipboard_set_text (bobgui_widget_get_clipboard (BOBGUI_WIDGET (self)), str);
      g_free (str);
    }
}

static void
bobgui_text_cut_clipboard (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (!priv->visible)
    {
      bobgui_widget_error_bell (BOBGUI_WIDGET (self));
      return;
    }

  bobgui_text_copy_clipboard (self);

  if (priv->editable)
    {
      if (priv->selection_bound != priv->current_pos)
        {
          begin_change (self);
          bobgui_text_delete_selection (self);
          end_change (self);
        }
    }
  else
    {
      bobgui_widget_error_bell (BOBGUI_WIDGET (self));
    }

  bobgui_text_selection_bubble_popup_unset (self);

  bobgui_text_update_handles (self);
}

static void
bobgui_text_paste_clipboard (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->editable)
    {
      begin_change (self);
      bobgui_text_paste (self, bobgui_widget_get_clipboard (BOBGUI_WIDGET (self)));
      end_change (self);
    }
  else
    {
      bobgui_widget_error_bell (BOBGUI_WIDGET (self));
    }

  bobgui_text_update_handles (self);
}

static void
bobgui_text_delete_cb (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->editable)
    {
      if (priv->selection_bound != priv->current_pos)
        bobgui_text_delete_selection (self);
    }
}

static void
bobgui_text_toggle_overwrite (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  priv->overwrite_mode = !priv->overwrite_mode;

  if (priv->overwrite_mode)
    {
      if (!priv->block_cursor_node)
        {
          BobguiCssNode *widget_node = bobgui_widget_get_css_node (BOBGUI_WIDGET (self));

          priv->block_cursor_node = bobgui_css_node_new ();
          bobgui_css_node_set_name (priv->block_cursor_node, g_quark_from_static_string ("block-cursor"));
          bobgui_css_node_set_parent (priv->block_cursor_node, widget_node);
          bobgui_css_node_set_state (priv->block_cursor_node, bobgui_css_node_get_state (widget_node));
          g_object_unref (priv->block_cursor_node);
        }
    }
  else
    {
      if (priv->block_cursor_node)
        {
          bobgui_css_node_set_parent (priv->block_cursor_node, NULL);
          priv->block_cursor_node = NULL;
        }
    }

  bobgui_text_pend_cursor_blink (self);
  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
}

static gboolean
bobgui_text_select_all (BobguiText *self)
{
  bobgui_text_select_line (self);
  return TRUE;
}

static void
bobgui_text_real_activate (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->activates_default)
    bobgui_widget_activate_default (BOBGUI_WIDGET (self));
}

/* IM Context Callbacks
 */

static void
bobgui_text_preedit_start_cb (BobguiIMContext *context,
                           BobguiText      *self)
{
  bobgui_text_delete_selection (self);
}

static void
bobgui_text_commit_cb (BobguiIMContext *context,
                    const char   *str,
                    BobguiText      *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->editable)
    {
      bobgui_text_enter_text (self, str);
      bobgui_text_obscure_mouse_cursor (self);
    }
}

static void
bobgui_text_preedit_changed_cb (BobguiIMContext *context,
                             BobguiText      *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->editable)
    {
      char *preedit_string;
      int cursor_pos;

      bobgui_text_obscure_mouse_cursor (self);

      bobgui_im_context_get_preedit_string (priv->im_context,
                                         &preedit_string, NULL,
                                         &cursor_pos);
      g_signal_emit (self, signals[PREEDIT_CHANGED], 0, preedit_string);
      priv->preedit_length = strlen (preedit_string);
      cursor_pos = CLAMP (cursor_pos, 0, g_utf8_strlen (preedit_string, -1));
      priv->preedit_cursor = cursor_pos;
      g_free (preedit_string);

      bobgui_text_recompute (self);
      update_placeholder_visibility (self);
    }
}

static gboolean
bobgui_text_retrieve_surrounding_cb (BobguiIMContext *context,
                                  BobguiText      *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  char *text;

  /* XXXX ??? does this even make sense when text is not visible? Should we return FALSE? */
  text = bobgui_text_get_display_text (self, 0, -1);
  bobgui_im_context_set_surrounding_with_selection (context, text, strlen (text), /* Length in bytes */
                                                 g_utf8_offset_to_pointer (text, priv->current_pos) - text,
                                                 g_utf8_offset_to_pointer (text, priv->selection_bound) - text);
  g_free (text);

  return TRUE;
}

static gboolean
bobgui_text_delete_surrounding_cb (BobguiIMContext *context,
                                int           offset,
                                int           n_chars,
                                BobguiText      *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->editable)
    {
      bobgui_editable_delete_text (BOBGUI_EDITABLE (self),
                                priv->current_pos + offset,
                                priv->current_pos + offset + n_chars);
    }

  return TRUE;
}

/* Internal functions
 */

/* Used for im_commit_cb and inserting Unicode chars */
void
bobgui_text_enter_text (BobguiText    *self,
                     const char *str)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  int tmp_pos;
  guint text_length;

  bobgui_text_history_begin_user_action (priv->history);
  begin_change (self);

  priv->need_im_reset = FALSE;

  if (priv->selection_bound != priv->current_pos)
    bobgui_text_delete_selection (self);
  else
    {
      if (priv->overwrite_mode)
        {
          text_length = bobgui_entry_buffer_get_length (get_buffer (self));
          if (priv->current_pos < text_length)
            bobgui_text_delete_from_cursor (self, BOBGUI_DELETE_CHARS, 1);
        }
    }

  tmp_pos = priv->current_pos;
  /* We do not need to notify the accessible text contents change
   * here, the insert_text() call will take care of it
   */
  bobgui_editable_insert_text (BOBGUI_EDITABLE (self), str, strlen (str), &tmp_pos);
  bobgui_text_set_selection_bounds (self, tmp_pos, tmp_pos);

  end_change (self);
  bobgui_text_history_end_user_action (priv->history);
}

/* All changes to priv->current_pos and priv->selection_bound
 * should go through this function.
 */
void
bobgui_text_set_positions (BobguiText *self,
                        int      current_pos,
                        int      selection_bound)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  gboolean changed = FALSE;

  g_object_freeze_notify (G_OBJECT (self));

  if (current_pos != -1 &&
      priv->current_pos != current_pos)
    {
      priv->current_pos = current_pos;
      changed = TRUE;

      g_object_notify (G_OBJECT (self), "cursor-position");
    }

  if (selection_bound != -1 &&
      priv->selection_bound != selection_bound)
    {
      priv->selection_bound = selection_bound;
      changed = TRUE;

      g_object_notify (G_OBJECT (self), "selection-bound");
    }

  g_object_thaw_notify (G_OBJECT (self));

  if (priv->current_pos != priv->selection_bound)
    {
      if (!priv->selection_node)
        {
          BobguiCssNode *widget_node = bobgui_widget_get_css_node (BOBGUI_WIDGET (self));

          priv->selection_node = bobgui_css_node_new ();
          bobgui_css_node_set_name (priv->selection_node, g_quark_from_static_string ("selection"));
          bobgui_css_node_set_parent (priv->selection_node, widget_node);
          bobgui_css_node_set_state (priv->selection_node, bobgui_css_node_get_state (widget_node));
          g_signal_connect (priv->selection_node, "style-changed",
                            G_CALLBACK (selection_style_changed_cb), self);
          g_object_unref (priv->selection_node);
        }
    }
  else
    {
      if (priv->selection_node)
        {
          bobgui_css_node_set_parent (priv->selection_node, NULL);
          priv->selection_node = NULL;
        }
    }

  if (changed)
    {
      bobgui_text_update_clipboard_actions (self);
      bobgui_text_recompute (self);

      bobgui_text_update_primary_selection (self);
      bobgui_accessible_text_update_caret_position (BOBGUI_ACCESSIBLE_TEXT (self));
      bobgui_accessible_text_update_selection_bound (BOBGUI_ACCESSIBLE_TEXT (self));
    }
}

static void
bobgui_text_reset_layout (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->cached_layout)
    {
      g_object_unref (priv->cached_layout);
      priv->cached_layout = NULL;
    }
}

static void
bobgui_text_recompute (BobguiText *self)
{
  bobgui_text_reset_layout (self);
  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));

  if (!bobgui_widget_get_mapped (BOBGUI_WIDGET (self)))
    return;

  bobgui_text_check_cursor_blink (self);
  bobgui_text_adjust_scroll (self);
  update_im_cursor_location (self);
  bobgui_text_update_handles (self);
}

static void
update_resolved_dir (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (bobgui_widget_get_direction (BOBGUI_WIDGET (self)) == BOBGUI_TEXT_DIR_RTL)
    priv->resolved_dir = PANGO_DIRECTION_RTL;
  else
    priv->resolved_dir = PANGO_DIRECTION_LTR;
}

static PangoLayout *
bobgui_text_create_layout (BobguiText  *self,
                        gboolean  include_preedit)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  PangoLayout *layout;
  PangoAttrList *tmp_attrs = NULL;
  char *preedit_string = NULL;
  int preedit_length = 0;
  PangoAttrList *preedit_attrs = NULL;
  char *display_text;
  guint n_bytes;

  layout = bobgui_widget_create_pango_layout (widget, NULL);
  pango_layout_set_single_paragraph_mode (layout, TRUE);

  tmp_attrs = bobgui_css_style_get_pango_attributes (bobgui_css_node_get_style (bobgui_widget_get_css_node (widget)));
  if (!tmp_attrs)
    tmp_attrs = pango_attr_list_new ();
  tmp_attrs = _bobgui_pango_attr_list_merge (tmp_attrs, priv->attrs);

  display_text = bobgui_text_get_display_text (self, 0, -1);

  n_bytes = strlen (display_text);

  if (include_preedit)
    {
      bobgui_im_context_get_preedit_string (priv->im_context,
                                         &preedit_string, &preedit_attrs, NULL);
      preedit_length = priv->preedit_length;
    }

  if (preedit_length)
    {
      GString *tmp_string = g_string_new (display_text);
      int pos;

      pos = g_utf8_offset_to_pointer (display_text, priv->current_pos) - display_text;
      g_string_insert (tmp_string, pos, preedit_string);
      pango_layout_set_text (layout, tmp_string->str, tmp_string->len);
      pango_attr_list_splice (tmp_attrs, preedit_attrs, pos, preedit_length);
      g_string_free (tmp_string, TRUE);
    }
  else
    {
      pango_layout_set_text (layout, display_text, n_bytes);
    }

  update_resolved_dir (self);

  pango_layout_set_attributes (layout, tmp_attrs);

  if (priv->tabs)
    pango_layout_set_tabs (layout, priv->tabs);

  g_free (preedit_string);
  g_free (display_text);

  pango_attr_list_unref (preedit_attrs);
  pango_attr_list_unref (tmp_attrs);

  return layout;
}

static PangoLayout *
bobgui_text_ensure_layout (BobguiText  *self,
                        gboolean  include_preedit)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->preedit_length > 0 &&
      !include_preedit != !priv->cache_includes_preedit)
    bobgui_text_reset_layout (self);

  if (!priv->cached_layout)
    {
      priv->cached_layout = bobgui_text_create_layout (self, include_preedit);
      priv->cache_includes_preedit = include_preedit;
    }

  return priv->cached_layout;
}

static void
get_layout_position (BobguiText *self,
                     int     *x,
                     int     *y)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  const int text_height = bobgui_widget_get_height (BOBGUI_WIDGET (self));
  PangoLayout *layout;
  PangoRectangle logical_rect;
  int y_pos, area_height;
  PangoLayoutLine *line;

  layout = bobgui_text_ensure_layout (self, TRUE);

  area_height = PANGO_SCALE * text_height;

  line = pango_layout_get_lines_readonly (layout)->data;
  pango_layout_line_get_extents (line, NULL, &logical_rect);

  /* Align primarily for locale's ascent/descent */
  if (priv->text_baseline < 0)
    y_pos = ((area_height - priv->ascent - priv->descent) / 2 +
             priv->ascent + logical_rect.y);
  else
    y_pos = PANGO_SCALE * priv->text_baseline - pango_layout_get_baseline (layout);

  /* Now see if we need to adjust to fit in actual drawn string */
  if (logical_rect.height > area_height)
    y_pos = (area_height - logical_rect.height) / 2;
  else if (y_pos < 0)
    y_pos = 0;
  else if (y_pos + logical_rect.height > area_height)
    y_pos = area_height - logical_rect.height;

  y_pos = y_pos / PANGO_SCALE;

  if (x)
    *x = - priv->scroll_offset;

  if (y)
    *y = y_pos;
}

#define GRAPHENE_RECT_FROM_RECT(_r) (GRAPHENE_RECT_INIT ((_r)->x, (_r)->y, (_r)->width, (_r)->height))

static void
bobgui_text_draw_text (BobguiText     *self,
                    BobguiSnapshot *snapshot)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  BobguiCssStyle *style;
  PangoLayout *layout;
  int x, y;
  BobguiCssBoxes boxes;

  /* Nothing to display at all */
  if (bobgui_text_get_display_mode (self) == DISPLAY_BLANK)
    return;

  layout = bobgui_text_ensure_layout (self, TRUE);

  bobgui_text_get_layout_offsets (self, &x, &y);

  bobgui_css_boxes_init (&boxes, widget);
  bobgui_css_style_snapshot_layout (&boxes, snapshot, x, y, layout);

  if (priv->selection_bound != priv->current_pos)
    {
      const char *text = pango_layout_get_text (layout);
      int start_index = g_utf8_offset_to_pointer (text, priv->selection_bound) - text;
      int end_index = g_utf8_offset_to_pointer (text, priv->current_pos) - text;
      cairo_region_t *clip;
      cairo_rectangle_int_t clip_extents;
      int range[2];
      int width, height;

      width = bobgui_widget_get_width (widget);
      height = bobgui_widget_get_height (widget);

      range[0] = MIN (start_index, end_index);
      range[1] = MAX (start_index, end_index);

      style = bobgui_css_node_get_style (priv->selection_node);

      clip = gdk_pango_layout_get_clip_region (layout, x, y, range, 1);
      cairo_region_get_extents (clip, &clip_extents);

      bobgui_css_boxes_init_border_box (&boxes, style, 0, 0, width, height);
      bobgui_snapshot_push_clip (snapshot, &GRAPHENE_RECT_FROM_RECT (&clip_extents));
      bobgui_css_style_snapshot_background (&boxes, snapshot);
      bobgui_css_style_snapshot_layout (&boxes, snapshot, x, y, layout);
      bobgui_snapshot_pop (snapshot);

      cairo_region_destroy (clip);
    }
}

static void
bobgui_text_draw_cursor (BobguiText     *self,
                      BobguiSnapshot *snapshot,
                      CursorType   type)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  BobguiCssStyle *style;
  PangoRectangle cursor_rect;
  int cursor_index;
  gboolean block_at_line_end;
  PangoLayout *layout;
  const char *text;
  int x, y;
  BobguiCssBoxes boxes;
  GdkDisplay *display;

  display = bobgui_widget_get_display (widget);

  layout = g_object_ref (bobgui_text_ensure_layout (self, TRUE));
  text = pango_layout_get_text (layout);
  bobgui_text_get_layout_offsets (self, &x, &y);

  if (type == CURSOR_DND)
    cursor_index = g_utf8_offset_to_pointer (text, priv->dnd_position) - text;
  else
    cursor_index = g_utf8_offset_to_pointer (text, priv->current_pos + priv->preedit_cursor) - text;

  if (!priv->overwrite_mode ||
      !_bobgui_text_util_get_block_cursor_location (layout,
                                                 cursor_index, &cursor_rect, &block_at_line_end))
    {
      bobgui_css_boxes_init (&boxes, widget);
      bobgui_css_style_snapshot_caret (&boxes, display, snapshot,
                                    x, y,
                                    layout, cursor_index, priv->resolved_dir);
    }
  else /* overwrite_mode */
    {
      int width = bobgui_widget_get_width (widget);
      int height = bobgui_widget_get_height (widget);
      graphene_rect_t bounds;

      bounds.origin.x = PANGO_PIXELS (cursor_rect.x) + x;
      bounds.origin.y = PANGO_PIXELS (cursor_rect.y) + y;
      bounds.size.width = PANGO_PIXELS (cursor_rect.width);
      bounds.size.height = PANGO_PIXELS (cursor_rect.height);

      style = bobgui_css_node_get_style (priv->block_cursor_node);
      bobgui_css_boxes_init_border_box (&boxes, style, 0, 0, width, height);
      bobgui_snapshot_push_clip (snapshot, &bounds);
      bobgui_css_style_snapshot_background (&boxes, snapshot);
      bobgui_css_style_snapshot_layout (&boxes, snapshot, x, y, layout);
      bobgui_snapshot_pop (snapshot);
    }

  g_object_unref (layout);
}

static void
bobgui_text_handle_dragged (BobguiTextHandle *handle,
                         int            x,
                         int            y,
                         BobguiText       *self)
{
  int cursor_pos, selection_bound_pos, tmp_pos, *old_pos;
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  bobgui_text_selection_bubble_popup_unset (self);

  cursor_pos = priv->current_pos;
  selection_bound_pos = priv->selection_bound;

  tmp_pos = bobgui_text_find_position (self, x + priv->scroll_offset);

  if (handle == priv->text_handles[TEXT_HANDLE_CURSOR])
    {
      /* Avoid running past the other handle in selection mode */
      if (tmp_pos >= selection_bound_pos &&
          bobgui_widget_is_visible (BOBGUI_WIDGET (priv->text_handles[TEXT_HANDLE_SELECTION_BOUND])))
        {
          tmp_pos = selection_bound_pos - 1;
        }

      old_pos = &cursor_pos;
    }
  else if (handle == priv->text_handles[TEXT_HANDLE_SELECTION_BOUND])
    {
      /* Avoid running past the other handle */
      if (tmp_pos <= cursor_pos)
        tmp_pos = cursor_pos + 1;

      old_pos = &selection_bound_pos;
    }
  else
    g_assert_not_reached ();

  if (tmp_pos != *old_pos)
    {
      *old_pos = tmp_pos;

      if (handle == priv->text_handles[TEXT_HANDLE_CURSOR] &&
          !bobgui_widget_is_visible (BOBGUI_WIDGET (priv->text_handles[TEXT_HANDLE_SELECTION_BOUND])))
        bobgui_text_set_positions (self, cursor_pos, cursor_pos);
      else
        bobgui_text_set_positions (self, cursor_pos, selection_bound_pos);

      if (handle == priv->text_handles[TEXT_HANDLE_CURSOR])
        priv->cursor_handle_dragged = TRUE;
      else if (handle == priv->text_handles[TEXT_HANDLE_SELECTION_BOUND])
        priv->selection_handle_dragged = TRUE;

      bobgui_text_update_handles (self);
    }

  bobgui_text_show_magnifier (self, x, y);
}

static void
bobgui_text_handle_drag_started (BobguiTextHandle *handle,
                              BobguiText       *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  priv->cursor_handle_dragged = FALSE;
  priv->selection_handle_dragged = FALSE;
}

static void
bobgui_text_handle_drag_finished (BobguiTextHandle *handle,
                               BobguiText       *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (!priv->cursor_handle_dragged && !priv->selection_handle_dragged)
    {
      BobguiSettings *settings;
      guint double_click_time;

      settings = bobgui_widget_get_settings (BOBGUI_WIDGET (self));
      g_object_get (settings, "bobgui-double-click-time", &double_click_time, NULL);
      if (g_get_monotonic_time() - priv->handle_place_time < double_click_time * 1000)
        {
          bobgui_text_select_word (self);
          bobgui_text_update_handles (self);
        }
      else
        bobgui_text_selection_bubble_popup_set (self);
    }

  if (priv->magnifier_popover)
    bobgui_popover_popdown (BOBGUI_POPOVER (priv->magnifier_popover));
}

static void
bobgui_text_schedule_im_reset (BobguiText *self)
{
  BobguiTextPrivate *priv;

  priv = bobgui_text_get_instance_private (self);

  priv->need_im_reset = TRUE;
}

void
bobgui_text_reset_im_context (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_TEXT (self));

  if (priv->need_im_reset)
    {
      priv->need_im_reset = FALSE;
      bobgui_im_context_reset (priv->im_context);
    }
}

static int
bobgui_text_find_position (BobguiText *self,
                        int      x)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  PangoLayout *layout;
  PangoLayoutLine *line;
  int index;
  int pos;
  int trailing;
  const char *text;
  int cursor_index;

  layout = bobgui_text_ensure_layout (self, TRUE);
  text = pango_layout_get_text (layout);
  cursor_index = g_utf8_offset_to_pointer (text, priv->current_pos) - text;

  line = pango_layout_get_lines_readonly (layout)->data;
  pango_layout_line_x_to_index (line, x * PANGO_SCALE, &index, &trailing);

  if (index >= cursor_index && priv->preedit_length)
    {
      if (index >= cursor_index + priv->preedit_length)
        index -= priv->preedit_length;
      else
        {
          index = cursor_index;
          trailing = 0;
        }
    }

  pos = g_utf8_pointer_to_offset (text, text + index);
  pos += trailing;

  return pos;
}

static void
bobgui_text_get_caret_locations (BobguiText        *self,
                              PangoRectangle *strong,
                              PangoRectangle *weak)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  DisplayMode mode = bobgui_text_get_display_mode (self);

  if (mode == DISPLAY_BLANK)
    {
      if (strong)
        {
          strong->x = 0;
          strong->width = 0;
        }

      if (weak)
        {
          weak->x = 0;
          weak->width = 0;
        }
    }
  else
    {
      PangoLayout *layout = bobgui_text_ensure_layout (self, TRUE);
      const char *text = pango_layout_get_text (layout);
      int index;

      index = g_utf8_offset_to_pointer (text, priv->current_pos + priv->preedit_cursor) - text;

      pango_layout_get_caret_pos (layout, index, strong, weak);

      if (strong)
        pango_extents_to_pixels (strong, NULL);

      if (weak)
        pango_extents_to_pixels (weak, NULL);
    }
}

static void
bobgui_text_get_cursor_locations (BobguiText   *self,
                               int       *strong_x,
                               int       *weak_x)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  DisplayMode mode = bobgui_text_get_display_mode (self);

  /* Nothing to display at all, so no cursor is relevant */
  if (mode == DISPLAY_BLANK)
    {
      if (strong_x)
        *strong_x = 0;

      if (weak_x)
        *weak_x = 0;
    }
  else
    {
      PangoLayout *layout = bobgui_text_ensure_layout (self, TRUE);
      const char *text = pango_layout_get_text (layout);
      PangoRectangle strong_pos, weak_pos;
      int index;

      index = g_utf8_offset_to_pointer (text, priv->current_pos + priv->preedit_cursor) - text;

      pango_layout_get_cursor_pos (layout, index, &strong_pos, &weak_pos);

      if (strong_x)
        *strong_x = strong_pos.x / PANGO_SCALE;

      if (weak_x)
        *weak_x = weak_pos.x / PANGO_SCALE;
    }
}

static gboolean
bobgui_text_get_is_selection_handle_dragged (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  BobguiTextHandle *handle;

  if (priv->current_pos >= priv->selection_bound)
    handle = priv->text_handles[TEXT_HANDLE_CURSOR];
  else
    handle = priv->text_handles[TEXT_HANDLE_SELECTION_BOUND];

  return handle && bobgui_text_handle_get_is_dragged (handle);
}

static void
bobgui_text_get_scroll_limits (BobguiText *self,
                            int     *min_offset,
                            int     *max_offset)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  float xalign;
  PangoLayout *layout;
  PangoLayoutLine *line;
  PangoRectangle logical_rect;
  int text_width, width;

  layout = bobgui_text_ensure_layout (self, TRUE);
  line = pango_layout_get_lines_readonly (layout)->data;

  pango_layout_line_get_extents (line, NULL, &logical_rect);

  /* Display as much text as we can */

  if (priv->resolved_dir == PANGO_DIRECTION_LTR)
      xalign = priv->xalign;
  else
      xalign = 1.0 - priv->xalign;

  text_width = PANGO_PIXELS(logical_rect.width);
  width = bobgui_widget_get_width (BOBGUI_WIDGET (self));

  if (text_width > width)
    {
      *min_offset = 0;
      *max_offset = text_width - width;
    }
  else
    {
      *min_offset = (text_width - width) * xalign;
      *max_offset = *min_offset;
    }
}

static void
bobgui_text_adjust_scroll (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  const int text_width = bobgui_widget_get_width (BOBGUI_WIDGET (self));
  int min_offset, max_offset;
  PangoRectangle strong, weak;

  if (!bobgui_widget_get_realized (BOBGUI_WIDGET (self)))
    return;

  bobgui_text_get_scroll_limits (self, &min_offset, &max_offset);

  priv->scroll_offset = CLAMP (priv->scroll_offset, min_offset, max_offset);

  if (bobgui_text_get_is_selection_handle_dragged (self))
    {
      /* The text handle corresponding to the selection bound is
       * being dragged, ensure it stays onscreen even if we scroll
       * cursors away, this is so both handles can cause content
       * to scroll.
       */
      strong.x = weak.x = bobgui_text_get_selection_bound_location (self);
      strong.width = weak.width = 1;
    }
  else
    {
      /* And make sure cursors are on screen.
       *
       * We always make sure that the strong cursor is on screen, and
       * put the weak cursor on screen if possible.
       */
      bobgui_text_get_caret_locations (self, &strong, &weak);
    }

  if (strong.x - priv->scroll_offset < 0)
    {
      priv->scroll_offset = strong.x;
    }
  else if (strong.x + strong.width - priv->scroll_offset > text_width)
    {
      priv->scroll_offset = strong.x + strong.width - text_width;
    }

  if (weak.x - priv->scroll_offset < 0 &&
      strong.x + strong.width - weak.x <= text_width)
    {
      priv->scroll_offset = weak.x;
    }
  else if (weak.x + weak.width - priv->scroll_offset > text_width &&
           weak.x + weak.width - strong.x <= text_width)
    {
      priv->scroll_offset = weak.x + weak.width - text_width;
    }

  g_object_notify_by_pspec (G_OBJECT (self), text_props[PROP_SCROLL_OFFSET]);

  bobgui_text_update_handles (self);
}

static int
bobgui_text_move_visually (BobguiText *self,
                        int      start,
                        int      count)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  int index;
  PangoLayout *layout = bobgui_text_ensure_layout (self, FALSE);
  const char *text;
  gboolean split_cursor;
  gboolean strong;

  text = pango_layout_get_text (layout);

  index = g_utf8_offset_to_pointer (text, start) - text;


  g_object_get (bobgui_widget_get_settings (BOBGUI_WIDGET (self)),
                "bobgui-split-cursor", &split_cursor,
                NULL);

  if (split_cursor)
    strong = TRUE;
  else
    {
      GdkDisplay *display;
      GdkSeat *seat;
      GdkDevice *keyboard = NULL;
      PangoDirection direction = PANGO_DIRECTION_LTR;

      display = bobgui_widget_get_display (BOBGUI_WIDGET (self));
      seat = gdk_display_get_default_seat (display);
      if (seat)
        keyboard = gdk_seat_get_keyboard (seat);
      if (keyboard)
        direction = gdk_device_get_direction (keyboard);

      strong = direction == priv->resolved_dir;
    }

  while (count != 0)
    {
      int new_index, new_trailing;

      if (count > 0)
        {
          pango_layout_move_cursor_visually (layout, strong, index, 0, 1, &new_index, &new_trailing);
          count--;
        }
      else
        {
          pango_layout_move_cursor_visually (layout, strong, index, 0, -1, &new_index, &new_trailing);
          count++;
        }

      if (new_index < 0)
        index = 0;
      else if (new_index != G_MAXINT)
        index = new_index;

      while (new_trailing--)
        index = g_utf8_next_char (text + index) - text;
    }

  return g_utf8_pointer_to_offset (text, text + index);
}

static int
bobgui_text_move_logically (BobguiText *self,
                         int      start,
                         int      count)
{
  int new_pos = start;
  guint length;

  length = bobgui_entry_buffer_get_length (get_buffer (self));

  /* Prevent any leak of information */
  if (bobgui_text_get_display_mode (self) != DISPLAY_NORMAL)
    {
      new_pos = CLAMP (start + count, 0, length);
    }
  else
    {
      PangoLayout *layout = bobgui_text_ensure_layout (self, FALSE);
      const PangoLogAttr *log_attrs;
      int n_attrs;

      log_attrs = pango_layout_get_log_attrs_readonly (layout, &n_attrs);

      while (count > 0 && new_pos < length)
        {
          do
            new_pos++;
          while (new_pos < length && !log_attrs[new_pos].is_cursor_position);

          count--;
        }
      while (count < 0 && new_pos > 0)
        {
          do
            new_pos--;
          while (new_pos > 0 && !log_attrs[new_pos].is_cursor_position);

          count++;
        }
    }

  return new_pos;
}

static int
bobgui_text_move_forward_word (BobguiText  *self,
                            int       start,
                            gboolean  allow_whitespace)
{
  int new_pos = start;
  guint length;

  length = bobgui_entry_buffer_get_length (get_buffer (self));

  /* Prevent any leak of information */
  if (bobgui_text_get_display_mode (self) != DISPLAY_NORMAL)
    {
      new_pos = length;
    }
  else if (new_pos < length)
    {
      PangoLayout *layout = bobgui_text_ensure_layout (self, FALSE);
      const PangoLogAttr *log_attrs;
      int n_attrs;

      log_attrs = pango_layout_get_log_attrs_readonly (layout, &n_attrs);

      /* Find the next word boundary */
      new_pos++;
      while (new_pos < n_attrs - 1 && !(log_attrs[new_pos].is_word_end ||
                                        (log_attrs[new_pos].is_word_start && allow_whitespace)))
        new_pos++;
    }

  return new_pos;
}


static int
bobgui_text_move_backward_word (BobguiText  *self,
                             int       start,
                             gboolean  allow_whitespace)
{
  int new_pos = start;

  /* Prevent any leak of information */
  if (bobgui_text_get_display_mode (self) != DISPLAY_NORMAL)
    {
      new_pos = 0;
    }
  else if (start > 0)
    {
      PangoLayout *layout = bobgui_text_ensure_layout (self, FALSE);
      const PangoLogAttr *log_attrs;
      int n_attrs;

      log_attrs = pango_layout_get_log_attrs_readonly (layout, &n_attrs);

      new_pos = start - 1;

      /* Find the previous word boundary */
      while (new_pos > 0 && !(log_attrs[new_pos].is_word_start ||
                              (log_attrs[new_pos].is_word_end && allow_whitespace)))
        new_pos--;
    }

  return new_pos;
}

static void
bobgui_text_delete_whitespace (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  PangoLayout *layout = bobgui_text_ensure_layout (self, FALSE);
  const PangoLogAttr *log_attrs;
  int n_attrs;
  int start, end;

  log_attrs = pango_layout_get_log_attrs_readonly (layout, &n_attrs);

  start = end = priv->current_pos;

  while (start > 0 && log_attrs[start-1].is_white)
    start--;

  while (end < n_attrs && log_attrs[end].is_white)
    end++;

  if (start != end)
    bobgui_editable_delete_text (BOBGUI_EDITABLE (self), start, end);
}


static void
bobgui_text_select_word (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  int start_pos = bobgui_text_move_backward_word (self, priv->current_pos, TRUE);
  int end_pos = bobgui_text_move_forward_word (self, priv->current_pos, TRUE);

  bobgui_text_set_selection_bounds (self, start_pos, end_pos);
}

static void
bobgui_text_select_line (BobguiText *self)
{
  bobgui_text_set_selection_bounds (self, 0, -1);
}

static int
truncate_multiline (const char *text)
{
  int length;

  for (length = 0;
       text[length] && text[length] != '\n' && text[length] != '\r';
       length++);

  return length;
}

static void
paste_received (GObject      *clipboard,
                GAsyncResult *result,
                gpointer      data)
{
  BobguiText *self = BOBGUI_TEXT (data);
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  char *text;
  int pos, start, end;
  int length = -1;

  text = gdk_clipboard_read_text_finish (GDK_CLIPBOARD (clipboard), result, NULL);
  if (text == NULL)
    {
      bobgui_widget_error_bell (BOBGUI_WIDGET (self));
      return;
    }

  if (priv->insert_pos >= 0)
    {
      pos = priv->insert_pos;
      start = priv->selection_bound;
      end = priv->current_pos;
      if (!((start <= pos && pos <= end) || (end <= pos && pos <= start)))
        bobgui_text_set_selection_bounds (self, pos, pos);
      priv->insert_pos = -1;
    }

  if (priv->truncate_multiline)
    length = truncate_multiline (text);
  else
    length = strlen (text);

  begin_change (self);
  if (priv->selection_bound != priv->current_pos)
    bobgui_text_delete_selection (self);

  pos = priv->current_pos;
  bobgui_editable_insert_text (BOBGUI_EDITABLE (self), text, length, &pos);
  bobgui_accessible_text_update_contents (BOBGUI_ACCESSIBLE_TEXT (self),
                                       BOBGUI_ACCESSIBLE_TEXT_CONTENT_CHANGE_INSERT,
                                       pos, pos + length);
  bobgui_text_set_selection_bounds (self, pos, pos);
  end_change (self);

  g_free (text);
  g_object_unref (self);
}

static void
bobgui_text_paste (BobguiText      *self,
                GdkClipboard *clipboard)
{
  gdk_clipboard_read_text_async (clipboard, NULL, paste_received, g_object_ref (self));
}

static void
bobgui_text_update_primary_selection (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  GdkClipboard *clipboard;

  if (!bobgui_widget_get_realized (BOBGUI_WIDGET (self)))
    return;

  if (!bobgui_widget_has_focus (BOBGUI_WIDGET (self)))
    return;

  clipboard = bobgui_widget_get_primary_clipboard (BOBGUI_WIDGET (self));

  if (priv->selection_bound != priv->current_pos)
    {
      gdk_clipboard_set_content (clipboard, priv->selection_content);
    }
  else
    {
      if (gdk_clipboard_get_content (clipboard) == priv->selection_content)
        gdk_clipboard_set_content (clipboard, NULL);
    }
}

/* Public API
 */

/**
 * bobgui_text_new:
 *
 * Creates a new `BobguiText`.
 *
 * Returns: the new `BobguiText`
 */
BobguiWidget *
bobgui_text_new (void)
{
  return g_object_new (BOBGUI_TYPE_TEXT, NULL);
}

/**
 * bobgui_text_new_with_buffer:
 * @buffer: the buffer to use
 *
 * Creates a new `BobguiText` with the specified buffer.
 *
 * Returns: a new `BobguiText`
 */
BobguiWidget *
bobgui_text_new_with_buffer (BobguiEntryBuffer *buffer)
{
  g_return_val_if_fail (BOBGUI_IS_ENTRY_BUFFER (buffer), NULL);

  return g_object_new (BOBGUI_TYPE_TEXT, "buffer", buffer, NULL);
}

static BobguiEntryBuffer *
get_buffer (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->buffer == NULL)
    {
      BobguiEntryBuffer *buffer;
      buffer = bobgui_entry_buffer_new (NULL, 0);
      bobgui_text_set_buffer (self, buffer);
      g_object_unref (buffer);
    }

  return priv->buffer;
}

/**
 * bobgui_text_get_buffer:
 * @self: a text widget
 *
 * Get the entry buffer object which holds the text for
 * this widget.
 *
 * Returns: (transfer none): the entry buffer object
 */
BobguiEntryBuffer *
bobgui_text_get_buffer (BobguiText *self)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT (self), NULL);

  return get_buffer (self);
}

/**
 * bobgui_text_set_buffer:
 * @self: a text widget
 * @buffer: an entry buffer object
 *
 * Set the entry buffer object which holds the text for
 * this widget.
 */
void
bobgui_text_set_buffer (BobguiText        *self,
                     BobguiEntryBuffer *buffer)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  GObject *obj;
  gboolean had_buffer = FALSE;
  guint old_length = 0;
  guint new_length = 0;

  g_return_if_fail (BOBGUI_IS_TEXT (self));

  if (buffer)
    {
      g_return_if_fail (BOBGUI_IS_ENTRY_BUFFER (buffer));
      g_object_ref (buffer);
    }

  if (priv->buffer)
    {
      had_buffer = TRUE;
      old_length = bobgui_entry_buffer_get_length (priv->buffer);
      buffer_disconnect_signals (self);
      g_object_unref (priv->buffer);
    }

  priv->buffer = buffer;

  if (priv->buffer)
    {
      new_length = bobgui_entry_buffer_get_length (priv->buffer);
      buffer_connect_signals (self);
    }

  update_placeholder_visibility (self);

  obj = G_OBJECT (self);
  g_object_freeze_notify (obj);
  g_object_notify_by_pspec (obj, text_props[PROP_BUFFER]);
  g_object_notify_by_pspec (obj, text_props[PROP_MAX_LENGTH]);
  if (old_length != 0 || new_length != 0)
    g_object_notify (obj, "text");

  if (had_buffer)
    {
      bobgui_text_set_selection_bounds (self, 0, 0);
      bobgui_text_recompute (self);
    }

  g_object_thaw_notify (obj);
}

static void
bobgui_text_set_editable (BobguiText  *self,
                       gboolean  is_editable)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (is_editable != priv->editable)
    {
      BobguiWidget *widget = BOBGUI_WIDGET (self);

      if (!is_editable)
        {
          bobgui_text_reset_im_context (self);
          if (bobgui_widget_has_focus (widget))
            bobgui_im_context_focus_out (priv->im_context);

          priv->preedit_length = 0;
          priv->preedit_cursor = 0;

          bobgui_widget_remove_css_class (BOBGUI_WIDGET (self), "read-only");
        }
      else
        {
          bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "read-only");
        }

      priv->editable = is_editable;

      if (is_editable && bobgui_widget_has_focus (widget))
        bobgui_im_context_focus_in (priv->im_context);

      bobgui_event_controller_key_set_im_context (BOBGUI_EVENT_CONTROLLER_KEY (priv->key_controller),
                                               is_editable ? priv->im_context : NULL);

      bobgui_text_update_history (self);
      bobgui_text_update_clipboard_actions (self);
      bobgui_text_update_emoji_action (self);
      bobgui_text_update_clear_action (self);

      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                      BOBGUI_ACCESSIBLE_PROPERTY_READ_ONLY, !priv->editable,
                                      -1);

      g_object_notify (G_OBJECT (self), "editable");
    }
}

static void
bobgui_text_set_text (BobguiText     *self,
                   const char *text)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  int tmp_pos;

  g_return_if_fail (BOBGUI_IS_TEXT (self));
  g_return_if_fail (text != NULL);

  /* Actually setting the text will affect the cursor and selection;
   * if the contents don't actually change, this will look odd to the user.
   */
  if (strcmp (bobgui_entry_buffer_get_text (get_buffer (self)), text) == 0)
    return;

  bobgui_text_history_begin_irreversible_action (priv->history);

  begin_change (self);
  bobgui_editable_delete_text (BOBGUI_EDITABLE (self), 0, -1);
  bobgui_accessible_text_update_contents (BOBGUI_ACCESSIBLE_TEXT (self),
                                       BOBGUI_ACCESSIBLE_TEXT_CONTENT_CHANGE_REMOVE,
                                       0, G_MAXUINT);
  tmp_pos = 0;
  bobgui_editable_insert_text (BOBGUI_EDITABLE (self), text, strlen (text), &tmp_pos);
  bobgui_accessible_text_update_contents (BOBGUI_ACCESSIBLE_TEXT (self),
                                       BOBGUI_ACCESSIBLE_TEXT_CONTENT_CHANGE_INSERT,
                                       tmp_pos, tmp_pos + g_utf8_strlen (text, -1));
  end_change (self);

  bobgui_text_history_end_irreversible_action (priv->history);
}

/**
 * bobgui_text_set_visibility:
 * @self: a text widget
 * @visible: true if the contents of the text widget are displayed
 *   as plain text
 *
 * Sets whether the contents of the text widget are visible or not.
 *
 * When visibility is set to false, characters are displayed
 * as the invisible char, and it will also appear that way when
 * the text in the widget is copied to the clipboard.
 *
 * By default, BOBGUI picks the best invisible character available
 * in the current font, but it can be changed with
 * [method@Bobgui.Text.set_invisible_char].
 *
 * Note that you probably want to set [property@Bobgui.Text:input-purpose]
 * to [enum@Bobgui.InputPurpose.password] or [enum@Bobgui.InputPurpose.pin]
 * to inform input methods about the purpose of this widget, in addition
 * to setting visibility to false.
 */
void
bobgui_text_set_visibility (BobguiText  *self,
                         gboolean  visible)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_TEXT (self));

  visible = visible != FALSE;

  if (priv->visible != visible)
    {
      priv->visible = visible;

      g_object_notify (G_OBJECT (self), "visibility");
      bobgui_text_update_cached_style_values (self);
      bobgui_text_recompute (self);

      /* disable undo when invisible text is used */
      bobgui_text_update_history (self);

      bobgui_text_update_clipboard_actions (self);
    }
}

/**
 * bobgui_text_get_visibility:
 * @self: a text widget
 *
 * Retrieves whether the text is visible.
 *
 * Returns: true if the text is visible
 */
gboolean
bobgui_text_get_visibility (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_TEXT (self), FALSE);

  return priv->visible;
}

/**
 * bobgui_text_set_invisible_char:
 * @self: a text widget
 * @ch: a Unicode character
 *
 * Sets the character to use when in “password mode”.
 *
 * By default, BOBGUI picks the best invisible char available in the
 * current font. If you set the invisible char to 0, then the user
 * will get no feedback at all; there will be no text on the screen
 * as they type.
 */
void
bobgui_text_set_invisible_char (BobguiText  *self,
                             gunichar  ch)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_TEXT (self));

  if (!priv->invisible_char_set)
    {
      priv->invisible_char_set = TRUE;
      g_object_notify_by_pspec (G_OBJECT (self), text_props[PROP_INVISIBLE_CHAR_SET]);
    }

  if (ch == priv->invisible_char)
    return;

  priv->invisible_char = ch;
  g_object_notify_by_pspec (G_OBJECT (self), text_props[PROP_INVISIBLE_CHAR]);
  bobgui_text_recompute (self);
}

/**
 * bobgui_text_get_invisible_char:
 * @self: a text widget
 *
 * Retrieves the character displayed when visibility is set to false.
 *
 * Note that BOBGUI does not compute this value unless it needs it,
 * so the value returned by this function is not very useful unless
 * it has been explicitly set with [method@Bobgui.Text.set_invisible_char].
 *
 * Returns: the current invisible char, or 0, if @text does not
 *   show invisible text at all
 */
gunichar
bobgui_text_get_invisible_char (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_TEXT (self), 0);

  return priv->invisible_char;
}

/**
 * bobgui_text_unset_invisible_char:
 * @self: a text widget
 *
 * Unsets the invisible char.
 *
 * After calling this, the default invisible char is used again.
 */
void
bobgui_text_unset_invisible_char (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  gunichar ch;

  g_return_if_fail (BOBGUI_IS_TEXT (self));

  if (!priv->invisible_char_set)
    return;

  priv->invisible_char_set = FALSE;
  ch = find_invisible_char (BOBGUI_WIDGET (self));

  if (priv->invisible_char != ch)
    {
      priv->invisible_char = ch;
      g_object_notify_by_pspec (G_OBJECT (self), text_props[PROP_INVISIBLE_CHAR]);
    }

  g_object_notify_by_pspec (G_OBJECT (self), text_props[PROP_INVISIBLE_CHAR_SET]);
  bobgui_text_recompute (self);
}

/**
 * bobgui_text_set_overwrite_mode:
 * @self: a text widget
 * @overwrite: new value
 *
 * Sets whether the text is overwritten when typing.
 */
void
bobgui_text_set_overwrite_mode (BobguiText  *self,
                             gboolean  overwrite)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_TEXT (self));

  if (priv->overwrite_mode == overwrite)
    return;

  bobgui_text_toggle_overwrite (self);

  g_object_notify_by_pspec (G_OBJECT (self), text_props[PROP_OVERWRITE_MODE]);
}

/**
 * bobgui_text_get_overwrite_mode:
 * @self: a text widget
 *
 * Gets whether text is overwritten when typing.
 *
 * See [method@Bobgui.Text.set_overwrite_mode].
 *
 * Returns: whether text is overwritten when typing
 */
gboolean
bobgui_text_get_overwrite_mode (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_TEXT (self), FALSE);

  return priv->overwrite_mode;

}

/**
 * bobgui_text_set_max_length:
 * @self: a text widget
 * @length: the maximum length of the text, or 0 for no maximum.
 *   (other than the maximum length of entries.) The value passed
 *   in will be clamped to the range 0-65536
 *
 * Sets the maximum allowed length of the contents.
 *
 * If the current contents are longer than the given length,
 * they will be truncated to fit.
 *
 * This is equivalent to getting @self's `BobguiEntryBuffer` and
 * calling [method@Bobgui.EntryBuffer.set_max_length] on it.
 */
void
bobgui_text_set_max_length (BobguiText *self,
                         int      length)
{
  g_return_if_fail (BOBGUI_IS_TEXT (self));
  bobgui_entry_buffer_set_max_length (get_buffer (self), length);
}

/**
 * bobgui_text_get_max_length:
 * @self: a text widget
 *
 * Retrieves the maximum allowed length of the contents.
 *
 * See [method@Bobgui.Text.set_max_length].
 *
 * This is equivalent to getting @self's `BobguiEntryBuffer` and
 * calling [method@Bobgui.EntryBuffer.get_max_length] on it.
 *
 * Returns: the maximum allowed number of characters, or 0 if
 *   there is no limit
 */
int
bobgui_text_get_max_length (BobguiText *self)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT (self), 0);

  return bobgui_entry_buffer_get_max_length (get_buffer (self));
}

/**
 * bobgui_text_get_text_length:
 * @self: a text widget
 *
 * Retrieves the length of the contents.
 *
 * This is equivalent to getting @self's `BobguiEntryBuffer`
 * and calling [method@Bobgui.EntryBuffer.get_length] on it.
 *
 * Returns: the length of the contents, in characters
 */
guint16
bobgui_text_get_text_length (BobguiText *self)
{
  g_return_val_if_fail (BOBGUI_IS_TEXT (self), 0);

  return bobgui_entry_buffer_get_length (get_buffer (self));
}

/**
 * bobgui_text_set_activates_default:
 * @self: a text widget
 * @activates: true to activate window’s default widget on
 *   <kbd>Enter</kbd> keypress
 *
 * Sets whether pressing <kbd>Enter</kbd> will activate
 * the default widget.
 *
 * This usually means that the dialog containing @self will
 * be closed, since the default widget is usually one of
 * the dialog buttons.
 */
void
bobgui_text_set_activates_default (BobguiText  *self,
                                gboolean  activates)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_TEXT (self));

  activates = activates != FALSE;

  if (priv->activates_default != activates)
    {
      priv->activates_default = activates;
      g_object_notify_by_pspec (G_OBJECT (self), text_props[PROP_ACTIVATES_DEFAULT]);
    }
}

/**
 * bobgui_text_get_activates_default:
 * @self: a text widget
 *
 * Returns whether pressing <kbd>Enter</kbd> will activate
 * the default widget for the window containing the widget.
 *
 * See [method@Bobgui.Text.set_activates_default].
 *
 * Returns: true if @self will activate the default widget
 */
gboolean
bobgui_text_get_activates_default (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_TEXT (self), FALSE);

  return priv->activates_default;
}

static void
bobgui_text_set_width_chars (BobguiText *self,
                          int      n_chars)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->width_chars != n_chars)
    {
      priv->width_chars = n_chars;
      g_object_notify (G_OBJECT (self), "width-chars");
      bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
    }
}

static void
bobgui_text_set_max_width_chars (BobguiText *self,
                              int      n_chars)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->max_width_chars != n_chars)
    {
      priv->max_width_chars = n_chars;
      g_object_notify (G_OBJECT (self), "max-width-chars");
      bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
    }
}

PangoLayout *
bobgui_text_get_layout (BobguiText *self)
{
  PangoLayout *layout;

  g_return_val_if_fail (BOBGUI_IS_TEXT (self), NULL);

  layout = bobgui_text_ensure_layout (self, TRUE);

  return layout;
}

void
bobgui_text_get_layout_offsets (BobguiText *self,
                             int     *x,
                             int     *y)
{
  g_return_if_fail (BOBGUI_IS_TEXT (self));

  get_layout_position (self, x, y);
}

static void
bobgui_text_set_alignment (BobguiText *self,
                        float    xalign)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (xalign < 0.0)
    xalign = 0.0;
  else if (xalign > 1.0)
    xalign = 1.0;

  if (xalign != priv->xalign)
    {
      priv->xalign = xalign;
      bobgui_text_recompute (self);
      if (priv->placeholder)
        bobgui_label_set_xalign (BOBGUI_LABEL (priv->placeholder), xalign);

      g_object_notify (G_OBJECT (self), "xalign");
    }
}

static void
hide_selection_bubble (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->selection_bubble && bobgui_widget_get_visible (priv->selection_bubble))
    bobgui_widget_set_visible (priv->selection_bubble, FALSE);
}

static void
bobgui_text_activate_clipboard_cut (BobguiWidget  *widget,
                                 const char *action_name,
                                 GVariant   *parameter)
{
  BobguiText *self = BOBGUI_TEXT (widget);
  g_signal_emit_by_name (self, "cut-clipboard");
  hide_selection_bubble (self);
}

static void
bobgui_text_activate_clipboard_copy (BobguiWidget  *widget,
                                  const char *action_name,
                                  GVariant   *parameter)
{
  BobguiText *self = BOBGUI_TEXT (widget);
  g_signal_emit_by_name (self, "copy-clipboard");
  hide_selection_bubble (self);
}

static void
bobgui_text_activate_clipboard_paste (BobguiWidget  *widget,
                                   const char *action_name,
                                   GVariant   *parameter)
{
  BobguiText *self = BOBGUI_TEXT (widget);
  g_signal_emit_by_name (self, "paste-clipboard");
  hide_selection_bubble (self);
}

static void
bobgui_text_activate_selection_delete (BobguiWidget  *widget,
                                    const char *action_name,
                                    GVariant   *parameter)
{
  BobguiText *self = BOBGUI_TEXT (widget);
  bobgui_text_delete_cb (self);
  hide_selection_bubble (self);
}

static void
bobgui_text_activate_selection_select_all (BobguiWidget  *widget,
                                        const char *action_name,
                                        GVariant   *parameter)
{
  BobguiText *self = BOBGUI_TEXT (widget);
  bobgui_text_select_line (self);
}

static void
bobgui_text_clear (BobguiWidget  *widget,
                const char *action_name,
                GVariant   *parameter)
{
  bobgui_editable_delete_text (BOBGUI_EDITABLE (widget), 0, -1);
}

static void
bobgui_text_activate_misc_insert_emoji (BobguiWidget  *widget,
                                     const char *action_name,
                                     GVariant   *parameter)
{
  BobguiText *self = BOBGUI_TEXT (widget);
  bobgui_text_insert_emoji (self);
  hide_selection_bubble (self);
}

static void
bobgui_text_activate_misc_toggle_direction (BobguiWidget  *widget,
                                         const char *action_name,
                                         GVariant   *parameter)
{
  BobguiText *self = BOBGUI_TEXT (widget);
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL)
    bobgui_widget_set_direction (widget, BOBGUI_TEXT_DIR_LTR);
  else
    bobgui_widget_set_direction (widget, BOBGUI_TEXT_DIR_RTL);

  update_resolved_dir (self);

  if (priv->cached_layout)
    pango_layout_context_changed (priv->cached_layout);
}

static void
bobgui_text_update_clipboard_actions (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  DisplayMode mode;
  GdkClipboard *clipboard;
  gboolean has_clipboard;
  gboolean has_selection;
  gboolean has_content;
  gboolean visible;

  clipboard = bobgui_widget_get_clipboard (BOBGUI_WIDGET (self));
  mode = bobgui_text_get_display_mode (self);
  has_clipboard = gdk_content_formats_contain_gtype (gdk_clipboard_get_formats (clipboard), G_TYPE_STRING);
  has_selection = priv->current_pos != priv->selection_bound;
  has_content = priv->buffer && (bobgui_entry_buffer_get_length (priv->buffer) > 0);
  visible = mode == DISPLAY_NORMAL;

  bobgui_widget_action_set_enabled (BOBGUI_WIDGET (self), "clipboard.cut",
                                 visible && priv->editable && has_selection);
  bobgui_widget_action_set_enabled (BOBGUI_WIDGET (self), "clipboard.copy",
                                 visible && has_selection);
  bobgui_widget_action_set_enabled (BOBGUI_WIDGET (self), "clipboard.paste",
                                 priv->editable && has_clipboard);

  bobgui_widget_action_set_enabled (BOBGUI_WIDGET (self), "selection.delete",
                                 priv->editable && has_selection);
  bobgui_widget_action_set_enabled (BOBGUI_WIDGET (self), "selection.select-all",
                                 has_content);
}

static void
bobgui_text_update_emoji_action (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  bobgui_widget_action_set_enabled (BOBGUI_WIDGET (self), "misc.insert-emoji",
                                 priv->editable &&
                                 (bobgui_text_get_input_hints (self) & BOBGUI_INPUT_HINT_NO_EMOJI) == 0);
}

static void
bobgui_text_update_clear_action (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  bobgui_widget_action_set_enabled (BOBGUI_WIDGET (self), "text.clear", priv->editable);
}

static GMenuModel *
bobgui_text_get_menu_model (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  BobguiJoinedMenu *joined;
  GMenu *menu, *section;
  GMenuItem *item;

  joined = bobgui_joined_menu_new ();
  menu = g_menu_new ();

  section = g_menu_new ();
  item = g_menu_item_new (_("Cu_t"), "clipboard.cut");
  g_menu_item_set_attribute (item, "touch-icon", "s", "edit-cut-symbolic");
  g_menu_append_item (section, item);
  g_object_unref (item);
  item = g_menu_item_new (_("_Copy"), "clipboard.copy");
  g_menu_item_set_attribute (item, "touch-icon", "s", "edit-copy-symbolic");
  g_menu_append_item (section, item);
  g_object_unref (item);
  item = g_menu_item_new (_("_Paste"), "clipboard.paste");
  g_menu_item_set_attribute (item, "touch-icon", "s", "edit-paste-symbolic");
  g_menu_append_item (section, item);
  g_object_unref (item);
  item = g_menu_item_new (_("_Delete"), "selection.delete");
  g_menu_item_set_attribute (item, "touch-icon", "s", "edit-delete-symbolic");
  g_menu_append_item (section, item);
  g_object_unref (item);
  g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
  g_object_unref (section);

  section = g_menu_new ();

  item = g_menu_item_new (_("Select _All"), "selection.select-all");
  g_menu_item_set_attribute (item, "touch-icon", "s", "edit-select-all-symbolic");
  g_menu_append_item (section, item);
  g_object_unref (item);

  item = g_menu_item_new (_("Change di_rection"), "misc.toggle-direction");
  g_menu_item_set_attribute (item, "hidden-when", "s", "action-disabled");
  g_menu_append_item (section, item);
  g_object_unref (item);

  item = g_menu_item_new ( _("Insert _Emoji"), "misc.insert-emoji");
  g_menu_item_set_attribute (item, "hidden-when", "s", "action-disabled");
  g_menu_item_set_attribute (item, "touch-icon", "s", "face-smile-symbolic");
  g_menu_append_item (section, item);
  g_object_unref (item);
  g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
  g_object_unref (section);

  bobgui_joined_menu_append_menu (joined, G_MENU_MODEL (menu));
  g_object_unref (menu);

  if (priv->extra_menu)
    bobgui_joined_menu_append_menu (joined, priv->extra_menu);

  return G_MENU_MODEL (joined);
}

static gboolean
bobgui_text_mnemonic_activate (BobguiWidget *widget,
                            gboolean   group_cycling)
{
  bobgui_widget_grab_focus (widget);
  return GDK_EVENT_STOP;
}

static void
bobgui_text_popup_menu (BobguiWidget  *widget,
                     const char *action_name,
                     GVariant   *parameters)
{
  bobgui_text_do_popup (BOBGUI_TEXT (widget), -1, -1);
}

static void
show_or_hide_handles (BobguiWidget  *popover,
                      GParamSpec *pspec,
                      BobguiText    *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  gboolean visible;

  visible = bobgui_widget_get_visible (popover);
  priv->text_handles_enabled = !visible;
  bobgui_text_update_handles (self);
}

static void
append_bubble_item (BobguiText    *self,
                    BobguiWidget  *toolbar,
                    GMenuModel *model,
                    int         index)
{
  BobguiActionMuxer *muxer;
  BobguiWidget *item, *image;
  GVariant *att;
  const char *icon_name;
  const char *action_name;
  GMenuModel *link;
  gboolean enabled;

  link = g_menu_model_get_item_link (model, index, "section");
  if (link)
    {
      int i;
      for (i = 0; i < g_menu_model_get_n_items (link); i++)
        append_bubble_item (self, toolbar, link, i);
      g_object_unref (link);
      return;
    }

  att = g_menu_model_get_item_attribute_value (model, index, "touch-icon", G_VARIANT_TYPE_STRING);
  if (att == NULL)
    return;

  icon_name = g_variant_get_string (att, NULL);
  g_variant_unref (att);

  att = g_menu_model_get_item_attribute_value (model, index, "action", G_VARIANT_TYPE_STRING);
  if (att == NULL)
    return;
  action_name = g_variant_get_string (att, NULL);
  g_variant_unref (att);

  muxer = _bobgui_widget_get_action_muxer (BOBGUI_WIDGET (self), FALSE);
  if (!bobgui_action_muxer_query_action (muxer, action_name, &enabled,
                                      NULL, NULL, NULL, NULL) ||
      !enabled)
    return;

  item = bobgui_button_new ();
  bobgui_widget_set_focus_on_click (item, FALSE);
  image = bobgui_image_new_from_icon_name (icon_name);
  bobgui_button_set_child (BOBGUI_BUTTON (item), image);
  bobgui_widget_add_css_class (item, "image-button");
  bobgui_actionable_set_action_name (BOBGUI_ACTIONABLE (item), action_name);
  bobgui_box_append (BOBGUI_BOX (toolbar), item);
}

static gboolean
bobgui_text_selection_bubble_popup_show (gpointer user_data)
{
  BobguiText *self = user_data;
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  const int text_width = bobgui_widget_get_width (BOBGUI_WIDGET (self));
  const int text_height = bobgui_widget_get_height (BOBGUI_WIDGET (self));
  cairo_rectangle_int_t rect;
  graphene_point_t p;
  gboolean has_selection;
  int start_x, end_x;
  BobguiWidget *box;
  BobguiWidget *toolbar;
  GMenuModel *model;
  int i;

  bobgui_text_update_clipboard_actions (self);

  has_selection = priv->selection_bound != priv->current_pos;

  if (!has_selection && !priv->editable)
    {
      priv->selection_bubble_timeout_id = 0;
      return G_SOURCE_REMOVE;
    }

  g_clear_pointer (&priv->selection_bubble, bobgui_widget_unparent);

  priv->selection_bubble = bobgui_popover_new ();
  bobgui_widget_set_parent (priv->selection_bubble, BOBGUI_WIDGET (self));
  bobgui_widget_add_css_class (priv->selection_bubble, "touch-selection");
  bobgui_popover_set_position (BOBGUI_POPOVER (priv->selection_bubble), BOBGUI_POS_BOTTOM);
  bobgui_popover_set_autohide (BOBGUI_POPOVER (priv->selection_bubble), FALSE);
  g_signal_connect (priv->selection_bubble, "notify::visible",
                    G_CALLBACK (show_or_hide_handles), self);

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);
  bobgui_widget_set_margin_start (box, 10);
  bobgui_widget_set_margin_end (box, 10);
  bobgui_widget_set_margin_top (box, 10);
  bobgui_widget_set_margin_bottom (box, 10);
  toolbar = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_add_css_class (toolbar, "linked");
  bobgui_popover_set_child (BOBGUI_POPOVER (priv->selection_bubble), box);
  bobgui_box_append (BOBGUI_BOX (box), toolbar);

  model = bobgui_text_get_menu_model (self);

  for (i = 0; i < g_menu_model_get_n_items (model); i++)
    append_bubble_item (self, toolbar, model, i);

  g_object_unref (model);

  if (!bobgui_widget_compute_point (BOBGUI_WIDGET (self), bobgui_widget_get_parent (BOBGUI_WIDGET (self)),
                                 &GRAPHENE_POINT_INIT (0, 0), &p))
    graphene_point_init (&p, 0, 0);

  bobgui_text_get_cursor_locations (self, &start_x, NULL);

  start_x -= priv->scroll_offset;
  start_x = CLAMP (start_x, 0, text_width);
  rect.y = - p.y;
  rect.height = text_height;

  if (has_selection)
    {
      end_x = bobgui_text_get_selection_bound_location (self) - priv->scroll_offset;
      end_x = CLAMP (end_x, 0, text_width);

      rect.x = - p.x + MIN (start_x, end_x);
      rect.width = ABS (end_x - start_x);
    }
  else
    {
      rect.x = - p.x + start_x;
      rect.width = 0;
    }

  rect.x -= 5;
  rect.y -= 5;
  rect.width += 10;
  rect.height += 10;

  bobgui_popover_set_pointing_to (BOBGUI_POPOVER (priv->selection_bubble), &rect);
  bobgui_popover_popup (BOBGUI_POPOVER (priv->selection_bubble));

  priv->selection_bubble_timeout_id = 0;

  return G_SOURCE_REMOVE;
}

static void
bobgui_text_selection_bubble_popup_unset (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->selection_bubble)
    bobgui_widget_set_visible (priv->selection_bubble, FALSE);

  if (priv->selection_bubble_timeout_id)
    {
      g_source_remove (priv->selection_bubble_timeout_id);
      priv->selection_bubble_timeout_id = 0;
    }
}

static void
bobgui_text_selection_bubble_popup_set (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->selection_bubble_timeout_id)
    g_source_remove (priv->selection_bubble_timeout_id);

  priv->selection_bubble_timeout_id =
    g_timeout_add (50, bobgui_text_selection_bubble_popup_show, self);
  gdk_source_set_static_name_by_id (priv->selection_bubble_timeout_id, "[bobgui] bobgui_text_selection_bubble_popup_cb");
}

static void
bobgui_text_drag_leave (BobguiDropTarget *dest,
                     BobguiText       *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  BobguiWidget *widget = BOBGUI_WIDGET (self);

  priv->dnd_position = -1;
  bobgui_widget_queue_draw (widget);
}

static gboolean
bobgui_text_drag_drop (BobguiDropTarget *dest,
                    const GValue  *value,
                    double         x,
                    double         y,
                    BobguiText       *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  int drop_position;
  int length;
  const char *str;

  if (!priv->editable)
    return FALSE;

  drop_position = bobgui_text_find_position (self, x + priv->scroll_offset);

  str = g_value_get_string (value);
  if (str == NULL)
    str = "";

  if (priv->truncate_multiline)
    length = truncate_multiline (str);
  else
    length = -1;

  if (priv->selection_bound == priv->current_pos ||
      drop_position < priv->selection_bound ||
      drop_position > priv->current_pos)
    {
      bobgui_editable_insert_text (BOBGUI_EDITABLE (self), str, length, &drop_position);
      bobgui_accessible_text_update_contents (BOBGUI_ACCESSIBLE_TEXT (self),
                                           BOBGUI_ACCESSIBLE_TEXT_CONTENT_CHANGE_INSERT,
                                           drop_position, drop_position + g_utf8_strlen (str, length));
    }
  else
    {
      int pos;
      /* Replacing selection */
      begin_change (self);
      bobgui_text_delete_selection (self);
      pos = MIN (priv->selection_bound, priv->current_pos);
      bobgui_editable_insert_text (BOBGUI_EDITABLE (self), str, length, &pos);
      bobgui_accessible_text_update_contents (BOBGUI_ACCESSIBLE_TEXT (self),
                                           BOBGUI_ACCESSIBLE_TEXT_CONTENT_CHANGE_INSERT,
                                           pos, pos + g_utf8_strlen (str, length));
      end_change (self);
    }

  return TRUE;
}

static gboolean
bobgui_text_drag_accept (BobguiDropTarget *dest,
                      GdkDrop       *drop,
                      BobguiText       *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (!priv->editable)
    return FALSE;

  if ((gdk_drop_get_actions (drop) & bobgui_drop_target_get_actions (dest)) == GDK_ACTION_NONE)
    return FALSE;

  return gdk_content_formats_match (bobgui_drop_target_get_formats (dest), gdk_drop_get_formats (drop));
}

static GdkDragAction
bobgui_text_drag_motion (BobguiDropTarget *target,
                      double         x,
                      double         y,
                      BobguiText       *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  int new_position, old_position;

  if (!priv->editable)
    {
      bobgui_drop_target_reject (target);
      return 0;
    }

  old_position = priv->dnd_position;
  new_position = bobgui_text_find_position (self, x + priv->scroll_offset);

  if (priv->selection_bound == priv->current_pos ||
      new_position < priv->selection_bound ||
      new_position > priv->current_pos)
    {
      priv->dnd_position = new_position;
    }
  else
    {
      priv->dnd_position = -1;
    }

  if (priv->dnd_position != old_position)
    bobgui_widget_queue_draw (BOBGUI_WIDGET (self));

  if (priv->drag)
    return GDK_ACTION_MOVE;
  else
    return GDK_ACTION_COPY;
}

/* We display the cursor when
 *
 *  - the selection is empty, AND
 *  - the widget has focus
 */

#define CURSOR_ON_MULTIPLIER 2
#define CURSOR_OFF_MULTIPLIER 1
#define CURSOR_PEND_MULTIPLIER 3
#define CURSOR_DIVIDER 3

static gboolean
cursor_blinks (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  BobguiRoot *root = bobgui_widget_get_root (BOBGUI_WIDGET (self));

  if (bobgui_widget_get_mapped (BOBGUI_WIDGET (self)) &&
      bobgui_window_is_active (BOBGUI_WINDOW (root)) &&
      bobgui_event_controller_focus_is_focus (BOBGUI_EVENT_CONTROLLER_FOCUS (priv->focus_controller)) &&
      priv->editable &&
      priv->selection_bound == priv->current_pos)
    {
      BobguiSettings *settings;
      gboolean blink;

      settings = bobgui_widget_get_settings (BOBGUI_WIDGET (self));
      g_object_get (settings, "bobgui-cursor-blink", &blink, NULL);

      return blink;
    }
  else
    return FALSE;
}

static gboolean
get_middle_click_paste (BobguiText *self)
{
  BobguiSettings *settings;
  gboolean paste;

  settings = bobgui_widget_get_settings (BOBGUI_WIDGET (self));
  g_object_get (settings, "bobgui-enable-primary-paste", &paste, NULL);

  return paste;
}

static int
get_cursor_time (BobguiText *self)
{
  BobguiSettings *settings = bobgui_widget_get_settings (BOBGUI_WIDGET (self));
  int time;

  g_object_get (settings, "bobgui-cursor-blink-time", &time, NULL);

  return time;
}

static int
get_cursor_blink_timeout (BobguiText *self)
{
  BobguiSettings *settings = bobgui_widget_get_settings (BOBGUI_WIDGET (self));
  int timeout;

  g_object_get (settings, "bobgui-cursor-blink-timeout", &timeout, NULL);

  return timeout;
}

typedef struct {
  guint64 start;
  guint64 end;
} BlinkData;

static gboolean blink_cb (BobguiWidget     *widget,
                          GdkFrameClock *clock,
                          gpointer       user_data);

static void
add_blink_timeout (BobguiText  *self,
                   gboolean  delay)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  BlinkData *data;
  int blink_time;

  priv->blink_start_time = g_get_monotonic_time ();
  priv->cursor_alpha = 1.0;

  blink_time = get_cursor_time (self);

  data = g_new (BlinkData, 1);
  data->start = priv->blink_start_time;
  if (delay)
    data->start += blink_time * 1000 / 2;
  data->end = data->start + blink_time * 1000;

  priv->blink_tick = bobgui_widget_add_tick_callback (BOBGUI_WIDGET (self),
                                                   blink_cb,
                                                   data,
                                                   g_free);
}

static void
remove_blink_timeout (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->blink_tick)
    {
      bobgui_widget_remove_tick_callback (BOBGUI_WIDGET (self), priv->blink_tick);
      priv->blink_tick = 0;
    }
}
/*
 * Blink!
 */

static float
blink_alpha (float phase)
{
  /* keep it simple, and split the blink cycle evenly
   * into visible, fading out, invisible, fading in
   */
  if (phase < 0.25)
    return 1;
  else if (phase < 0.5)
    return 1 - 4 * (phase - 0.25);
  else if (phase < 0.75)
    return 0;
  else
    return 4 * (phase - 0.75);
}

static gboolean
blink_cb (BobguiWidget     *widget,
          GdkFrameClock *clock,
          gpointer       user_data)
{
  BobguiText *self = BOBGUI_TEXT (widget);
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  BlinkData *data = user_data;
  int blink_timeout;
  int blink_time;
  guint64 now;
  float phase;
  float alpha;

  if (!bobgui_widget_has_focus (BOBGUI_WIDGET (self)))
    {
      g_warning ("BobguiText - did not receive a focus-out event.\n"
                 "If you handle this event, you must return\n"
                 "GDK_EVENT_PROPAGATE so the default handler\n"
                 "gets the event as well");

      bobgui_text_check_cursor_blink (self);
      return G_SOURCE_REMOVE;
    }

  if (priv->selection_bound != priv->current_pos)
    {
      g_warning ("BobguiText - unexpected blinking selection. Removing");

      bobgui_text_check_cursor_blink (self);
      return G_SOURCE_REMOVE;
    }

  blink_timeout = get_cursor_blink_timeout (self);
  blink_time = get_cursor_time (self);

  now = g_get_monotonic_time ();

  if (now > priv->blink_start_time + blink_timeout * 1000000)
    {
      /* we've blinked enough without the user doing anything, stop blinking */
      priv->cursor_alpha = 1.0;
      remove_blink_timeout (self);
      bobgui_widget_queue_draw (widget);

      return G_SOURCE_REMOVE;
    }

  phase = (now - data->start) / (float) (data->end - data->start);

  if (now >= data->end)
    {
      data->start = data->end;
      data->end = data->start + blink_time * 1000;
    }

  alpha = blink_alpha (phase);

  if (priv->cursor_alpha != alpha)
    {
      priv->cursor_alpha = alpha;
      bobgui_widget_queue_draw (widget);
    }

  return G_SOURCE_CONTINUE;
}

static void
bobgui_text_check_cursor_blink (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (cursor_blinks (self))
    {
      if (!priv->blink_tick)
        add_blink_timeout (self, FALSE);
    }
  else
    {
      if (priv->blink_tick)
        remove_blink_timeout (self);
    }
}

static void
bobgui_text_pend_cursor_blink (BobguiText *self)
{
  if (cursor_blinks (self))
    {
      remove_blink_timeout (self);
      add_blink_timeout (self, TRUE);
    }
}

static void
bobgui_text_reset_blink_time (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  priv->blink_start_time = g_get_monotonic_time ();
}

/**
 * bobgui_text_set_placeholder_text:
 * @self: a text widget
 * @text: (nullable): a string to be displayed when @self
 *   is empty and unfocused
 *
 * Sets the text to be displayed when the text widget is
 * empty and unfocused.
 *
 * This can be used to give a visual hint of the expected
 * contents of the text widget.
 */
void
bobgui_text_set_placeholder_text (BobguiText    *self,
                               const char *text)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_TEXT (self));

  if (priv->placeholder == NULL)
    {
      priv->placeholder = g_object_new (BOBGUI_TYPE_LABEL,
                                        "label", text,
                                        "css-name", "placeholder",
                                        "xalign", priv->xalign,
                                        "ellipsize", PANGO_ELLIPSIZE_END,
                                        "max-width-chars", 3,
                                        NULL);
      bobgui_label_set_attributes (BOBGUI_LABEL (priv->placeholder), priv->attrs);
      bobgui_widget_insert_after (priv->placeholder, BOBGUI_WIDGET (self), NULL);
    }
  else
    {
      bobgui_label_set_text (BOBGUI_LABEL (priv->placeholder), text);
    }

  update_placeholder_visibility (self);

  g_object_notify_by_pspec (G_OBJECT (self), text_props[PROP_PLACEHOLDER_TEXT]);
}

/**
 * bobgui_text_get_placeholder_text:
 * @self: a text widget
 *
 * Retrieves the text that will be displayed when the text widget
 * is empty and unfocused
 *
 * See [method@Bobgui.Text.set_placeholder_text].
 *
 * Returns: (nullable) (transfer none): the placeholder text
 */
const char *
bobgui_text_get_placeholder_text (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_TEXT (self), NULL);

  if (!priv->placeholder)
    return NULL;

  return bobgui_label_get_text (BOBGUI_LABEL (priv->placeholder));
}

/**
 * bobgui_text_set_input_purpose:
 * @self: a text widget
 * @purpose: the input purpose
 *
 * Sets the input purpose of the text widget.
 *
 * The input purpose can be used by on-screen keyboards
 * and other input methods to adjust their behaviour.
 */
void
bobgui_text_set_input_purpose (BobguiText         *self,
                            BobguiInputPurpose  purpose)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_TEXT (self));

  if (bobgui_text_get_input_purpose (self) != purpose)
    {
      g_object_set (G_OBJECT (priv->im_context),
                    "input-purpose", purpose,
                    NULL);

      g_object_notify_by_pspec (G_OBJECT (self), text_props[PROP_INPUT_PURPOSE]);
    }
}

/**
 * bobgui_text_get_input_purpose:
 * @self: a text widget
 *
 * Gets the input purpose of the text widget.
 *
 * Returns: the input purpose
 */
BobguiInputPurpose
bobgui_text_get_input_purpose (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  BobguiInputPurpose purpose;

  g_return_val_if_fail (BOBGUI_IS_TEXT (self), BOBGUI_INPUT_PURPOSE_FREE_FORM);

  g_object_get (G_OBJECT (priv->im_context),
                "input-purpose", &purpose,
                NULL);

  return purpose;
}

/**
 * bobgui_text_set_input_hints:
 * @self: a text widget
 * @hints: input hints
 *
 * Sets hints that allow input methods to fine-tune their behaviour.
 */
void
bobgui_text_set_input_hints (BobguiText       *self,
                          BobguiInputHints  hints)

{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_TEXT (self));

  if (bobgui_text_get_input_hints (self) != hints)
    {
      g_object_set (G_OBJECT (priv->im_context),
                    "input-hints", hints,
                    NULL);

      g_object_notify_by_pspec (G_OBJECT (self), text_props[PROP_INPUT_HINTS]);
      bobgui_text_update_emoji_action (self);
    }
}

/**
 * bobgui_text_get_input_hints:
 * @self: a text widget
 *
 * Gets the input hints of the text widget.
 *
 * Returns: the input hints
 */
BobguiInputHints
bobgui_text_get_input_hints (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);
  BobguiInputHints hints;

  g_return_val_if_fail (BOBGUI_IS_TEXT (self), BOBGUI_INPUT_HINT_NONE);

  g_object_get (G_OBJECT (priv->im_context),
                "input-hints", &hints,
                NULL);

  return hints;
}

/**
 * bobgui_text_set_attributes:
 * @self: a text widget
 * @attrs: (nullable): a list of style attributes
 *
 * Apply attributes to the contents of the text widget.
 */
void
bobgui_text_set_attributes (BobguiText       *self,
                         PangoAttrList *attrs)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_TEXT (self));

  if (attrs)
    pango_attr_list_ref (attrs);

  if (priv->attrs)
    pango_attr_list_unref (priv->attrs);
  priv->attrs = attrs;

  if (priv->placeholder)
    bobgui_label_set_attributes (BOBGUI_LABEL (priv->placeholder), attrs);

  g_object_notify_by_pspec (G_OBJECT (self), text_props[PROP_ATTRIBUTES]);

  bobgui_text_recompute (self);
  bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
}

/**
 * bobgui_text_get_attributes:
 * @self: a text widget
 *
 * Gets the attribute list that was set on the text widget.
 *
 * See [method@Bobgui.Text.set_attributes].
 *
 * Returns: (transfer none) (nullable): the attribute list
 */
PangoAttrList *
bobgui_text_get_attributes (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_TEXT (self), NULL);

  return priv->attrs;
}

/**
 * bobgui_text_set_tabs:
 * @self: a text widget
 * @tabs: (nullable): tab stops
 *
 * Sets tab stops for the text widget.
 */
void
bobgui_text_set_tabs (BobguiText       *self,
                   PangoTabArray *tabs)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_TEXT (self));

  if (priv->tabs)
    pango_tab_array_free(priv->tabs);

  if (tabs)
    priv->tabs = pango_tab_array_copy (tabs);
  else
    priv->tabs = NULL;

  g_object_notify_by_pspec (G_OBJECT (self), text_props[PROP_TABS]);

  bobgui_text_recompute (self);
  bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
}

/**
 * bobgui_text_get_tabs:
 * @self: a text widget
 *
 * Gets the tab stops for the text widget.
 *
 * See [method@Bobgui.Text.set_tabs].
 *
 * Returns: (nullable) (transfer none): the tab stops
 */
PangoTabArray *
bobgui_text_get_tabs (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_TEXT (self), NULL);

  return priv->tabs;
}

static void
emoji_picked (BobguiEmojiChooser *chooser,
              const char      *text,
              BobguiText         *self)
{
  int pos;

  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  begin_change (self);
  if (priv->selection_bound != priv->current_pos)
    bobgui_text_delete_selection (self);

  pos = priv->current_pos;
  bobgui_editable_insert_text (BOBGUI_EDITABLE (self), text, -1, &pos);
  bobgui_accessible_text_update_contents (BOBGUI_ACCESSIBLE_TEXT (self),
                                       BOBGUI_ACCESSIBLE_TEXT_CONTENT_CHANGE_INSERT,
                                       pos, pos + g_utf8_strlen (text, -1));
  bobgui_text_set_selection_bounds (self, pos, pos);
  end_change (self);
}

static void
bobgui_text_insert_emoji (BobguiText *self)
{
  BobguiWidget *chooser;

  if (bobgui_widget_get_ancestor (BOBGUI_WIDGET (self), BOBGUI_TYPE_EMOJI_CHOOSER) != NULL)
    return;

  chooser = BOBGUI_WIDGET (g_object_get_data (G_OBJECT (self), "bobgui-emoji-chooser"));
  if (!chooser)
    {
      chooser = bobgui_emoji_chooser_new ();
      g_object_set_data (G_OBJECT (self), "bobgui-emoji-chooser", chooser);

      bobgui_widget_set_parent (chooser, BOBGUI_WIDGET (self));
      g_signal_connect (chooser, "emoji-picked", G_CALLBACK (emoji_picked), self);
    }

  bobgui_popover_popup (BOBGUI_POPOVER (chooser));
}

static void
set_text_cursor (BobguiWidget *widget)
{
  bobgui_widget_set_cursor_from_name (widget, "text");
}

BobguiEventController *
bobgui_text_get_key_controller (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  return priv->key_controller;
}

/**
 * bobgui_text_set_extra_menu:
 * @self: a text widget
 * @model: (nullable): a menu model
 *
 * Sets a menu model to add to the context menu of the text widget.
 */
void
bobgui_text_set_extra_menu (BobguiText    *self,
                         GMenuModel *model)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_TEXT (self));

  if (g_set_object (&priv->extra_menu, model))
    {
      g_clear_pointer (&priv->popup_menu, bobgui_widget_unparent);

      g_object_notify_by_pspec (G_OBJECT (self), text_props[PROP_EXTRA_MENU]);
    }
}

/**
 * bobgui_text_get_extra_menu:
 * @self: a text widget
 *
 * Gets the extra menu model of the text widget.
 *
 * See [method@Bobgui.Text.set_extra_menu].
 *
 * Returns: (transfer none) (nullable): the menu model
 */
GMenuModel *
bobgui_text_get_extra_menu (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_TEXT (self), NULL);

  return priv->extra_menu;
}

/**
 * bobgui_text_set_enable_emoji_completion:
 * @self: a text widget
 * @enable_emoji_completion: true to enable Emoji completion
 *
 * Sets whether Emoji completion is enabled.
 *
 * If it is, typing ':', followed by a recognized keyword,
 * will pop up a window with suggested Emojis matching the
 * keyword.
 */
void
bobgui_text_set_enable_emoji_completion (BobguiText  *self,
                                      gboolean  enable_emoji_completion)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_TEXT (self));

  if (priv->enable_emoji_completion == enable_emoji_completion)
    return;

  priv->enable_emoji_completion = enable_emoji_completion;

  if (priv->enable_emoji_completion)
    priv->emoji_completion = bobgui_emoji_completion_new (self);
  else
    g_clear_pointer (&priv->emoji_completion, bobgui_widget_unparent);

  g_object_notify_by_pspec (G_OBJECT (self), text_props[PROP_ENABLE_EMOJI_COMPLETION]);
}

/**
 * bobgui_text_get_enable_emoji_completion:
 * @self: a text widget
 *
 * Returns whether Emoji completion is enabled.
 *
 * Returns: true if Emoji completion is enabled
 */
gboolean
bobgui_text_get_enable_emoji_completion (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_TEXT (self), FALSE);

  return priv->enable_emoji_completion;
}

/**
 * bobgui_text_set_propagate_text_width:
 * @self: a text widget
 * @propagate_text_width: true to propagate the text width
 *
 * Sets whether the text widget should grow and shrink with the content.
 */
void
bobgui_text_set_propagate_text_width (BobguiText  *self,
                                   gboolean  propagate_text_width)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_TEXT (self));

  if (priv->propagate_text_width == propagate_text_width)
    return;

  priv->propagate_text_width = propagate_text_width;

  bobgui_widget_queue_resize (BOBGUI_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), text_props[PROP_PROPAGATE_TEXT_WIDTH]);
}

/**
 * bobgui_text_get_propagate_text_width:
 * @self: a text widget
 *
 * Returns whether the text widget will grow and shrink
 * with the content.
 *
 * Returns: true if @self will propagate the text width
 */
gboolean
bobgui_text_get_propagate_text_width (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_TEXT (self), FALSE);

  return priv->propagate_text_width;
}

/**
 * bobgui_text_set_truncate_multiline:
 * @self: a text widget
 * @truncate_multiline: true to truncate multi-line text
 *
 * Sets whether pasted text should be truncated to the first line.
 */
void
bobgui_text_set_truncate_multiline (BobguiText  *self,
                                 gboolean  truncate_multiline)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_if_fail (BOBGUI_IS_TEXT (self));

  if (priv->truncate_multiline == truncate_multiline)
    return;

  priv->truncate_multiline = truncate_multiline;

  g_object_notify_by_pspec (G_OBJECT (self), text_props[PROP_TRUNCATE_MULTILINE]);
}

/**
 * bobgui_text_get_truncate_multiline:
 * @self: a text widget
 *
 * Returns whether pasted text will be truncated to the first line.
 *
 * Returns: true if @self will truncate pasted multi-line text
 */
gboolean
bobgui_text_get_truncate_multiline (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  g_return_val_if_fail (BOBGUI_IS_TEXT (self), FALSE);

  return priv->truncate_multiline;
}

/**
 * bobgui_text_compute_cursor_extents:
 * @self: a text widget
 * @position: the character position
 * @strong: (out) (optional): location to store the strong cursor position
 * @weak: (out) (optional): location to store the weak cursor position
 *
 * Determines the positions of the strong and weak cursors for a
 * given character position.
 *
 * The position of each cursor is stored as a zero-width rectangle.
 * The strong cursor location is the location where characters of
 * the directionality equal to the base direction are inserted.
 * The weak cursor location is the location where characters of
 * the directionality opposite to the base direction are inserted.
 *
 * The rectangle positions are in widget coordinates.
 *
 * Since: 4.4
 */
void
bobgui_text_compute_cursor_extents (BobguiText         *self,
                                 gsize            position,
                                 graphene_rect_t *strong,
                                 graphene_rect_t *weak)
{
  PangoLayout *layout;
  PangoRectangle pango_strong_pos;
  PangoRectangle pango_weak_pos;
  int offset_x, offset_y, index;
  const char *text;

  g_return_if_fail (BOBGUI_IS_TEXT (self));

  layout = bobgui_text_ensure_layout (self, TRUE);
  text = pango_layout_get_text (layout);
  position = CLAMP (position, 0, g_utf8_strlen (text, -1));
  index = g_utf8_offset_to_pointer (text, position) - text;

  pango_layout_get_cursor_pos (layout, index,
                               strong ? &pango_strong_pos : NULL,
                               weak ? &pango_weak_pos : NULL);
  bobgui_text_get_layout_offsets (self, &offset_x, &offset_y);

  if (strong)
    {
      graphene_rect_init (strong,
                          offset_x + pango_strong_pos.x / PANGO_SCALE,
                          offset_y + pango_strong_pos.y / PANGO_SCALE,
                          0,
                          pango_strong_pos.height / PANGO_SCALE);
    }

  if (weak)
    {
      graphene_rect_init (weak,
                          offset_x + pango_weak_pos.x / PANGO_SCALE,
                          offset_y + pango_weak_pos.y / PANGO_SCALE,
                          0,
                          pango_weak_pos.height / PANGO_SCALE);
    }
}

static void
bobgui_text_real_undo (BobguiWidget  *widget,
                    const char *action_name,
                    GVariant   *parameters)
{
  BobguiText *text = BOBGUI_TEXT (widget);
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (text);

  bobgui_text_history_undo (priv->history);
}

static void
bobgui_text_real_redo (BobguiWidget  *widget,
                    const char *action_name,
                    GVariant   *parameters)
{
  BobguiText *text = BOBGUI_TEXT (widget);
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (text);

  bobgui_text_history_redo (priv->history);
}

static void
bobgui_text_history_change_state_cb (gpointer funcs_data,
                                  gboolean is_modified,
                                  gboolean can_undo,
                                  gboolean can_redo)
{
  /* Do nothing */
}

static void
bobgui_text_history_insert_cb (gpointer    funcs_data,
                            guint       begin,
                            guint       end,
                            const char *str,
                            guint       len)
{
  BobguiText *text = funcs_data;
  int location = begin;

  bobgui_editable_insert_text (BOBGUI_EDITABLE (text), str, len, &location);
  bobgui_accessible_text_update_contents (BOBGUI_ACCESSIBLE_TEXT (text),
                                       BOBGUI_ACCESSIBLE_TEXT_CONTENT_CHANGE_INSERT,
                                       location, location + g_utf8_strlen (str, len));
}

static void
bobgui_text_history_delete_cb (gpointer    funcs_data,
                            guint       begin,
                            guint       end,
                            const char *expected_text,
                            guint       len)
{
  BobguiText *text = funcs_data;

  bobgui_editable_delete_text (BOBGUI_EDITABLE (text), begin, end);
}

static void
bobgui_text_history_select_cb (gpointer funcs_data,
                            int      selection_insert,
                            int      selection_bound)
{
  BobguiText *text = funcs_data;

  bobgui_editable_select_region (BOBGUI_EDITABLE (text),
                              selection_insert,
                              selection_bound);
}

static void
bobgui_text_set_enable_undo (BobguiText  *self,
                          gboolean  enable_undo)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  if (priv->enable_undo == enable_undo)
    return;

  priv->enable_undo = enable_undo;
  bobgui_text_update_history (self);
  g_object_notify (G_OBJECT (self), "enable-undo");
}

static void
bobgui_text_update_history (BobguiText *self)
{
  BobguiTextPrivate *priv = bobgui_text_get_instance_private (self);

  bobgui_text_history_set_enabled (priv->history,
                                priv->enable_undo &&
                                priv->visible &&
                                priv->editable);
}

 /* {{{ BobguiAccessibleText implementation */

static GBytes *
bobgui_text_accessible_text_get_contents (BobguiAccessibleText *self,
                                       unsigned int       start,
                                       unsigned int       end)
{
  char *text = bobgui_text_get_display_text (BOBGUI_TEXT (self), 0, -1);
  int len = g_utf8_strlen (text, -1);
  char *string;
  gsize size;

  start = CLAMP (start, 0, len);
  end = CLAMP (end, 0, len);

  if (end <= start)
    {
      string = g_strdup ("");
      size = 1;
    }
  else
    {
      const char *p, *q;
      p = g_utf8_offset_to_pointer (text, start);
      q = g_utf8_offset_to_pointer (text, end);
      size = q - p + 1;
      string = g_strndup (p, q - p);
    }

  g_free (text);

  return g_bytes_new_take (string, size);
}

static GBytes *
bobgui_text_accessible_text_get_contents_at (BobguiAccessibleText            *self,
                                          unsigned int                  offset,
                                          BobguiAccessibleTextGranularity  granularity,
                                          unsigned int                 *start,
                                          unsigned int                 *end)
{
  PangoLayout *layout = bobgui_text_get_layout (BOBGUI_TEXT (self));
  char *string = bobgui_pango_get_string_at (layout, offset, granularity, start, end);

  return g_bytes_new_take (string, strlen (string));
}

static unsigned int
bobgui_text_accessible_text_get_caret_position (BobguiAccessibleText *self)
{
  return bobgui_editable_get_position (BOBGUI_EDITABLE (self));
}

static gboolean
bobgui_text_accessible_text_get_selection (BobguiAccessibleText       *self,
                                        gsize                   *n_ranges,
                                        BobguiAccessibleTextRange **ranges)
{
  int start, end;

  if (!bobgui_editable_get_selection_bounds (BOBGUI_EDITABLE (self), &start, &end))
    return FALSE;

  *n_ranges = 1;
  *ranges = g_new (BobguiAccessibleTextRange, 1);
  (*ranges)[0].start = start;
  (*ranges)[0].length = end - start;

  return TRUE;
}

static gboolean
bobgui_text_accessible_text_get_attributes (BobguiAccessibleText        *self,
                                         unsigned int              offset,
                                         gsize                    *n_ranges,
                                         BobguiAccessibleTextRange  **ranges,
                                         char                   ***attribute_names,
                                         char                   ***attribute_values)
{
  PangoLayout *layout = bobgui_text_get_layout (BOBGUI_TEXT (self));
  unsigned int start, end;
  char **names, **values;

  bobgui_pango_get_run_attributes (layout, offset, &names, &values, &start, &end);

  *n_ranges = g_strv_length (names);
  *ranges = g_new (BobguiAccessibleTextRange, *n_ranges);

  for (unsigned i = 0; i < *n_ranges; i++)
    {
      BobguiAccessibleTextRange *range = &(*ranges)[i];

      range->start = start;
      range->length = end - start;
    }

  *attribute_names = names;
  *attribute_values = values;

  return TRUE;
}

static void
bobgui_text_accessible_text_get_default_attributes (BobguiAccessibleText   *self,
                                                 char              ***attribute_names,
                                                 char              ***attribute_values)
{
  PangoLayout *layout = bobgui_text_get_layout (BOBGUI_TEXT (self));
  char **names, **values;

  bobgui_pango_get_default_attributes (layout, &names, &values);

  *attribute_names = names;
  *attribute_values = values;
}

static gboolean
bobgui_text_accessible_text_get_extents (BobguiAccessibleText *self,
                                      unsigned int       start,
                                      unsigned int       end,
                                      graphene_rect_t   *extents)
{
  PangoLayout *layout = bobgui_text_get_layout (BOBGUI_TEXT (self));
  const char *text;
  int lx, ly;
  int range[2];
  cairo_region_t *range_clip;
  cairo_rectangle_int_t clip_rect;

  layout = bobgui_text_get_layout (BOBGUI_TEXT (self));
  text = bobgui_entry_buffer_get_text (get_buffer (BOBGUI_TEXT (self)));
  get_layout_position (BOBGUI_TEXT (self), &lx, &ly);

  range[0] = g_utf8_pointer_to_offset (text, text + start);
  range[1] = g_utf8_pointer_to_offset (text, text + end);

  range_clip = gdk_pango_layout_get_clip_region (layout, lx, ly, range, 1);
  cairo_region_get_extents (range_clip, &clip_rect);
  cairo_region_destroy (range_clip);

  extents->origin.x = clip_rect.x;
  extents->origin.y = clip_rect.y;
  extents->size.width = clip_rect.width;
  extents->size.height = clip_rect.height;

  return TRUE;
}

static gboolean
bobgui_text_accessible_text_get_offset (BobguiAccessibleText      *self,
                                     const graphene_point_t *point,
                                     unsigned int           *offset)
{
  int lx;
  int index;
  const char *text;

  bobgui_text_get_layout_offsets (BOBGUI_TEXT (self), &lx, NULL);
  index = bobgui_text_find_position (BOBGUI_TEXT (self), point->x - lx);
  text = bobgui_entry_buffer_get_text (get_buffer (BOBGUI_TEXT (self)));

  *offset = (unsigned int) g_utf8_pointer_to_offset (text, text + index);

  return TRUE;
}

static gboolean
bobgui_text_accessible_text_set_caret_position (BobguiAccessibleText *self,
                                             unsigned int       offset)
{
  bobgui_text_set_selection_bounds (BOBGUI_TEXT (self), offset, offset);
  return TRUE;
}

static gboolean
bobgui_text_accessible_text_set_selection (BobguiAccessibleText      *self,
                                        gsize                   i,
                                        BobguiAccessibleTextRange *range)
{
  if (i != 0)
    return FALSE;

  bobgui_text_set_selection_bounds (BOBGUI_TEXT (self), range->start, range->start + range->length);
  return TRUE;
}

static void
bobgui_text_accessible_text_init (BobguiAccessibleTextInterface *iface)
{
  iface->get_contents = bobgui_text_accessible_text_get_contents;
  iface->get_contents_at = bobgui_text_accessible_text_get_contents_at;
  iface->get_caret_position = bobgui_text_accessible_text_get_caret_position;
  iface->get_selection = bobgui_text_accessible_text_get_selection;
  iface->get_attributes = bobgui_text_accessible_text_get_attributes;
  iface->get_default_attributes = bobgui_text_accessible_text_get_default_attributes;
  iface->get_extents = bobgui_text_accessible_text_get_extents;
  iface->get_offset = bobgui_text_accessible_text_get_offset;
  iface->set_caret_position = bobgui_text_accessible_text_set_caret_position;
  iface->set_selection = bobgui_text_accessible_text_set_selection;
}

/* }}} */

/* vim:set foldmethod=marker: */
