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

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <glib-object.h>
#include <gdk/version/gdkversionmacros.h>


G_BEGIN_DECLS

/**
 * BobguiAlign:
 * @BOBGUI_ALIGN_FILL: stretch to fill all space if possible, center if
 *   no meaningful way to stretch
 * @BOBGUI_ALIGN_START: snap to left or top side, leaving space on right or bottom
 * @BOBGUI_ALIGN_END: snap to right or bottom side, leaving space on left or top
 * @BOBGUI_ALIGN_CENTER: center natural width of widget inside the allocation
 *
 * Controls how a widget deals with extra space in a single dimension.
 *
 * Alignment only matters if the widget receives a “too large” allocation,
 * for example if you packed the widget with the [property@Bobgui.Widget:hexpand]
 * property inside a [class@Box], then the widget might get extra space.
 * If you have for example a 16x16 icon inside a 32x32 space, the icon
 * could be scaled and stretched, it could be centered, or it could be
 * positioned to one side of the space.
 *
 * Note that in horizontal context `BOBGUI_ALIGN_START` and `BOBGUI_ALIGN_END`
 * are interpreted relative to text direction.
 *
 * Baseline support is optional for containers and widgets, and is only available
 * for vertical alignment. `BOBGUI_ALIGN_BASELINE_CENTER` and `BOBGUI_ALIGN_BASELINE_FILL`
 * are treated similar to `BOBGUI_ALIGN_CENTER` and `BOBGUI_ALIGN_FILL`, except that it
 * positions the widget to line up the baselines, where that is supported.
 */
/**
 * BOBGUI_ALIGN_BASELINE:
 *
 * align the widget according to the baseline.
 *
 * Deprecated: 4.12: Use `BOBGUI_ALIGN_BASELINE_FILL` instead
 */
/**
 * BOBGUI_ALIGN_BASELINE_FILL:
 *
 * a different name for `BOBGUI_ALIGN_BASELINE`.
 *
 * Since: 4.12
 */
/**
 * BOBGUI_ALIGN_BASELINE_CENTER:
 *
 * stretch to fill all space, but align the baseline.
 *
 * Since: 4.12
 */
typedef enum
{
  BOBGUI_ALIGN_FILL,
  BOBGUI_ALIGN_START,
  BOBGUI_ALIGN_END,
  BOBGUI_ALIGN_CENTER,
  BOBGUI_ALIGN_BASELINE_FILL GDK_AVAILABLE_ENUMERATOR_IN_4_12,
  BOBGUI_ALIGN_BASELINE GDK_DEPRECATED_ENUMERATOR_IN_4_12_FOR(BOBGUI_ALIGN_BASELINE_FILL) = BOBGUI_ALIGN_CENTER + 1,
  BOBGUI_ALIGN_BASELINE_CENTER GDK_AVAILABLE_ENUMERATOR_IN_4_12,
} BobguiAlign;

/**
 * BobguiArrowType:
 * @BOBGUI_ARROW_UP: Represents an upward pointing arrow.
 * @BOBGUI_ARROW_DOWN: Represents a downward pointing arrow.
 * @BOBGUI_ARROW_LEFT: Represents a left pointing arrow.
 * @BOBGUI_ARROW_RIGHT: Represents a right pointing arrow.
 * @BOBGUI_ARROW_NONE: No arrow.
 *
 * Indicates the direction in which an arrow should point.
 */
typedef enum
{
  BOBGUI_ARROW_UP,
  BOBGUI_ARROW_DOWN,
  BOBGUI_ARROW_LEFT,
  BOBGUI_ARROW_RIGHT,
  BOBGUI_ARROW_NONE
} BobguiArrowType;

/**
 * BobguiBaselinePosition:
 * @BOBGUI_BASELINE_POSITION_TOP: Align the baseline at the top
 * @BOBGUI_BASELINE_POSITION_CENTER: Center the baseline
 * @BOBGUI_BASELINE_POSITION_BOTTOM: Align the baseline at the bottom
 *
 * Baseline position in a row of widgets.
 *
 * Whenever a container has some form of natural row it may align
 * children in that row along a common typographical baseline. If
 * the amount of vertical space in the row is taller than the total
 * requested height of the baseline-aligned children then it can use a
 * `BobguiBaselinePosition` to select where to put the baseline inside the
 * extra available space.
 */
typedef enum
{
  BOBGUI_BASELINE_POSITION_TOP,
  BOBGUI_BASELINE_POSITION_CENTER,
  BOBGUI_BASELINE_POSITION_BOTTOM
} BobguiBaselinePosition;

/**
 * BobguiContentFit:
 * @BOBGUI_CONTENT_FIT_FILL: Make the content fill the entire allocation,
 *   without taking its aspect ratio in consideration. The resulting
 *   content will appear as stretched if its aspect ratio is different
 *   from the allocation aspect ratio.
 * @BOBGUI_CONTENT_FIT_CONTAIN: Scale the content to fit the allocation,
 *   while taking its aspect ratio in consideration. The resulting
 *   content will appear as letterboxed if its aspect ratio is different
 *   from the allocation aspect ratio.
 * @BOBGUI_CONTENT_FIT_COVER: Cover the entire allocation, while taking
 *   the content aspect ratio in consideration. The resulting content
 *   will appear as clipped if its aspect ratio is different from the
 *   allocation aspect ratio.
 * @BOBGUI_CONTENT_FIT_SCALE_DOWN: The content is scaled down to fit the
 *   allocation, if needed, otherwise its original size is used.
 *
 * Controls how a content should be made to fit inside an allocation.
 *
 * Since: 4.8
 */
typedef enum
{
  BOBGUI_CONTENT_FIT_FILL,
  BOBGUI_CONTENT_FIT_CONTAIN,
  BOBGUI_CONTENT_FIT_COVER,
  BOBGUI_CONTENT_FIT_SCALE_DOWN,
} BobguiContentFit;

/**
 * BobguiDeleteType:
 * @BOBGUI_DELETE_CHARS: Delete characters.
 * @BOBGUI_DELETE_WORD_ENDS: Delete only the portion of the word to the
 *   left/right of cursor if we’re in the middle of a word.
 * @BOBGUI_DELETE_WORDS: Delete words.
 * @BOBGUI_DELETE_DISPLAY_LINES: Delete display-lines. Display-lines
 *   refers to the visible lines, with respect to the current line
 *   breaks. As opposed to paragraphs, which are defined by line
 *   breaks in the input.
 * @BOBGUI_DELETE_DISPLAY_LINE_ENDS: Delete only the portion of the
 *   display-line to the left/right of cursor.
 * @BOBGUI_DELETE_PARAGRAPH_ENDS: Delete to the end of the
 *   paragraph. Like C-k in Emacs (or its reverse).
 * @BOBGUI_DELETE_PARAGRAPHS: Delete entire line. Like C-k in pico.
 * @BOBGUI_DELETE_WHITESPACE: Delete only whitespace. Like M-\ in Emacs.
 *
 * Passed to various keybinding signals for deleting text.
 */
typedef enum
{
  BOBGUI_DELETE_CHARS,
  BOBGUI_DELETE_WORD_ENDS,
  BOBGUI_DELETE_WORDS,
  BOBGUI_DELETE_DISPLAY_LINES,
  BOBGUI_DELETE_DISPLAY_LINE_ENDS,
  BOBGUI_DELETE_PARAGRAPH_ENDS,
  BOBGUI_DELETE_PARAGRAPHS,
  BOBGUI_DELETE_WHITESPACE
} BobguiDeleteType;

/* Focus movement types */
/**
 * BobguiDirectionType:
 * @BOBGUI_DIR_TAB_FORWARD: Move forward.
 * @BOBGUI_DIR_TAB_BACKWARD: Move backward.
 * @BOBGUI_DIR_UP: Move up.
 * @BOBGUI_DIR_DOWN: Move down.
 * @BOBGUI_DIR_LEFT: Move left.
 * @BOBGUI_DIR_RIGHT: Move right.
 *
 * Focus movement types.
 */
typedef enum
{
  BOBGUI_DIR_TAB_FORWARD,
  BOBGUI_DIR_TAB_BACKWARD,
  BOBGUI_DIR_UP,
  BOBGUI_DIR_DOWN,
  BOBGUI_DIR_LEFT,
  BOBGUI_DIR_RIGHT
} BobguiDirectionType;

/**
 * BobguiIconSize:
 * @BOBGUI_ICON_SIZE_INHERIT: Keep the size of the parent element
 * @BOBGUI_ICON_SIZE_NORMAL: Size similar to text size
 * @BOBGUI_ICON_SIZE_LARGE: Large size, for example in an icon view
 *
 * Built-in icon sizes.
 *
 * Icon sizes default to being inherited. Where they cannot be
 * inherited, text size is the default.
 *
 * All widgets which use `BobguiIconSize` set the normal-icons or
 * large-icons style classes correspondingly, and let themes
 * determine the actual size to be used with the
 * `-bobgui-icon-size` CSS property.
 */
typedef enum
{
  BOBGUI_ICON_SIZE_INHERIT,
  BOBGUI_ICON_SIZE_NORMAL,
  BOBGUI_ICON_SIZE_LARGE
} BobguiIconSize;

/**
 * BobguiSensitivityType:
 * @BOBGUI_SENSITIVITY_AUTO: The control is made insensitive if no
 *   action can be triggered
 * @BOBGUI_SENSITIVITY_ON: The control is always sensitive
 * @BOBGUI_SENSITIVITY_OFF: The control is always insensitive
 *
 * Determines how BOBGUI handles the sensitivity of various controls,
 * such as combo box buttons.
 */
typedef enum
{
  BOBGUI_SENSITIVITY_AUTO,
  BOBGUI_SENSITIVITY_ON,
  BOBGUI_SENSITIVITY_OFF
} BobguiSensitivityType;

/* Reading directions for text */
/**
 * BobguiTextDirection:
 * @BOBGUI_TEXT_DIR_NONE: No direction.
 * @BOBGUI_TEXT_DIR_LTR: Left to right text direction.
 * @BOBGUI_TEXT_DIR_RTL: Right to left text direction.
 *
 * Reading directions for text.
 */
typedef enum
{
  BOBGUI_TEXT_DIR_NONE,
  BOBGUI_TEXT_DIR_LTR,
  BOBGUI_TEXT_DIR_RTL
} BobguiTextDirection;

/**
 * BobguiJustification:
 * @BOBGUI_JUSTIFY_LEFT: The text is placed at the left edge of the label.
 * @BOBGUI_JUSTIFY_RIGHT: The text is placed at the right edge of the label.
 * @BOBGUI_JUSTIFY_CENTER: The text is placed in the center of the label.
 * @BOBGUI_JUSTIFY_FILL: The text is placed is distributed across the label.
 *
 * Used for justifying the text inside a [class@Label] widget.
 */
typedef enum
{
  BOBGUI_JUSTIFY_LEFT,
  BOBGUI_JUSTIFY_RIGHT,
  BOBGUI_JUSTIFY_CENTER,
  BOBGUI_JUSTIFY_FILL
} BobguiJustification;

/**
 * BobguiListTabBehavior:
 * @BOBGUI_LIST_TAB_ALL: Cycle through all focusable items of the list
 * @BOBGUI_LIST_TAB_ITEM: Cycle through a single list element, then move
 *   focus out of the list. Moving focus between items needs to be
 *   done with the arrow keys.
 * @BOBGUI_LIST_TAB_CELL: Cycle only through a single cell, then
 *   move focus out of the list. Moving focus between cells needs to
 *   be done with the arrow keys. This is only relevant for
 *   cell-based widgets like #BobguiColumnView, otherwise it behaves
 *   like `BOBGUI_LIST_TAB_ITEM`.
 *
 * Used to configure the focus behavior in the `BOBGUI_DIR_TAB_FORWARD`
 * and `BOBGUI_DIR_TAB_BACKWARD` direction, like the <kbd>Tab</kbd> key
 * in a [class@Bobgui.ListView].
 *
 * Since: 4.12
 */
typedef enum
{
  BOBGUI_LIST_TAB_ALL,
  BOBGUI_LIST_TAB_ITEM,
  BOBGUI_LIST_TAB_CELL
} BobguiListTabBehavior;

/**
 * BobguiListScrollFlags:
 * @BOBGUI_LIST_SCROLL_NONE: Don't do anything extra
 * @BOBGUI_LIST_SCROLL_FOCUS: Focus the target item
 * @BOBGUI_LIST_SCROLL_SELECT: Select the target item and
 *   unselect all other items.
 *
 * List of actions to perform when scrolling to items in
 * a list widget.
 *
 * Since: 4.12
 */
typedef enum {
  BOBGUI_LIST_SCROLL_NONE      = 0,
  BOBGUI_LIST_SCROLL_FOCUS     = 1 << 0,
  BOBGUI_LIST_SCROLL_SELECT    = 1 << 1
} BobguiListScrollFlags;

/**
 * BobguiMessageType:
 * @BOBGUI_MESSAGE_INFO: Informational message
 * @BOBGUI_MESSAGE_WARNING: Non-fatal warning message
 * @BOBGUI_MESSAGE_QUESTION: Question requiring a choice
 * @BOBGUI_MESSAGE_ERROR: Fatal error message
 * @BOBGUI_MESSAGE_OTHER: None of the above
 *
 * The type of message being displayed in a [class@MessageDialog].
 */
typedef enum
{
  BOBGUI_MESSAGE_INFO,
  BOBGUI_MESSAGE_WARNING,
  BOBGUI_MESSAGE_QUESTION,
  BOBGUI_MESSAGE_ERROR,
  BOBGUI_MESSAGE_OTHER
} BobguiMessageType;

/**
 * BobguiMovementStep:
 * @BOBGUI_MOVEMENT_LOGICAL_POSITIONS: Move forward or back by graphemes
 * @BOBGUI_MOVEMENT_VISUAL_POSITIONS:  Move left or right by graphemes
 * @BOBGUI_MOVEMENT_WORDS:             Move forward or back by words
 * @BOBGUI_MOVEMENT_DISPLAY_LINES:     Move up or down lines (wrapped lines)
 * @BOBGUI_MOVEMENT_DISPLAY_LINE_ENDS: Move to either end of a line
 * @BOBGUI_MOVEMENT_PARAGRAPHS:        Move up or down paragraphs (newline-ended lines)
 * @BOBGUI_MOVEMENT_PARAGRAPH_ENDS:    Move to either end of a paragraph
 * @BOBGUI_MOVEMENT_PAGES:             Move by pages
 * @BOBGUI_MOVEMENT_BUFFER_ENDS:       Move to ends of the buffer
 * @BOBGUI_MOVEMENT_HORIZONTAL_PAGES:  Move horizontally by pages
 *
 * Passed as argument to various keybinding signals for moving the
 * cursor position.
 */
typedef enum
{
  BOBGUI_MOVEMENT_LOGICAL_POSITIONS,
  BOBGUI_MOVEMENT_VISUAL_POSITIONS,
  BOBGUI_MOVEMENT_WORDS,
  BOBGUI_MOVEMENT_DISPLAY_LINES,
  BOBGUI_MOVEMENT_DISPLAY_LINE_ENDS,
  BOBGUI_MOVEMENT_PARAGRAPHS,
  BOBGUI_MOVEMENT_PARAGRAPH_ENDS,
  BOBGUI_MOVEMENT_PAGES,
  BOBGUI_MOVEMENT_BUFFER_ENDS,
  BOBGUI_MOVEMENT_HORIZONTAL_PAGES
} BobguiMovementStep;

/**
 * BobguiNaturalWrapMode:
 * @BOBGUI_NATURAL_WRAP_INHERIT: Inherit the minimum size request.
 *   In particular, this should be used with %PANGO_WRAP_CHAR.
 * @BOBGUI_NATURAL_WRAP_NONE: Try not to wrap the text. This mode is the
 *   closest to BOBGUI3's behavior but can lead to a wide label leaving
 *   lots of empty space below the text.
 * @BOBGUI_NATURAL_WRAP_WORD: Attempt to wrap at word boundaries. This
 *   is useful in particular when using %PANGO_WRAP_WORD_CHAR as the
 *   wrap mode.
 *
 * Options for selecting a different wrap mode for natural size
 * requests.
 *
 * See for example the [property@Bobgui.Label:natural-wrap-mode] property.
 *
 * Since: 4.6
 */
typedef enum
{
  BOBGUI_NATURAL_WRAP_INHERIT,
  BOBGUI_NATURAL_WRAP_NONE,
  BOBGUI_NATURAL_WRAP_WORD
} BobguiNaturalWrapMode;

/**
 * BobguiScrollStep:
 * @BOBGUI_SCROLL_STEPS: Scroll in steps.
 * @BOBGUI_SCROLL_PAGES: Scroll by pages.
 * @BOBGUI_SCROLL_ENDS: Scroll to ends.
 * @BOBGUI_SCROLL_HORIZONTAL_STEPS: Scroll in horizontal steps.
 * @BOBGUI_SCROLL_HORIZONTAL_PAGES: Scroll by horizontal pages.
 * @BOBGUI_SCROLL_HORIZONTAL_ENDS: Scroll to the horizontal ends.
 *
 * Passed as argument to various keybinding signals.
 */
typedef enum
{
  BOBGUI_SCROLL_STEPS,
  BOBGUI_SCROLL_PAGES,
  BOBGUI_SCROLL_ENDS,
  BOBGUI_SCROLL_HORIZONTAL_STEPS,
  BOBGUI_SCROLL_HORIZONTAL_PAGES,
  BOBGUI_SCROLL_HORIZONTAL_ENDS
} BobguiScrollStep;

/**
 * BobguiOrientation:
 * @BOBGUI_ORIENTATION_HORIZONTAL: The element is in horizontal orientation.
 * @BOBGUI_ORIENTATION_VERTICAL: The element is in vertical orientation.
 *
 * Represents the orientation of widgets and other objects.
 *
 * Typical examples are [class@Box] or [class@GesturePan].
 */
typedef enum
{
  BOBGUI_ORIENTATION_HORIZONTAL,
  BOBGUI_ORIENTATION_VERTICAL
} BobguiOrientation;

/**
 * BobguiOverflow:
 * @BOBGUI_OVERFLOW_VISIBLE: No change is applied. Content is drawn at the specified
 *   position.
 * @BOBGUI_OVERFLOW_HIDDEN: Content is clipped to the bounds of the area. Content
 *   outside the area is not drawn and cannot be interacted with.
 *
 * Defines how content overflowing a given area should be handled.
 *
 * This is used in [method@Bobgui.Widget.set_overflow]. The
 * [property@Bobgui.Widget:overflow] property is modeled after the
 * CSS overflow property, but implements it only partially.
 */
typedef enum
{
  BOBGUI_OVERFLOW_VISIBLE,
  BOBGUI_OVERFLOW_HIDDEN
} BobguiOverflow;

/**
 * BobguiPackType:
 * @BOBGUI_PACK_START: The child is packed into the start of the widget
 * @BOBGUI_PACK_END: The child is packed into the end of the widget
 *
 * Represents the packing location of a children in its parent.
 *
 * See [class@WindowControls] for example.
 */
typedef enum
{
  BOBGUI_PACK_START,
  BOBGUI_PACK_END
} BobguiPackType;

/**
 * BobguiPositionType:
 * @BOBGUI_POS_LEFT: The feature is at the left edge.
 * @BOBGUI_POS_RIGHT: The feature is at the right edge.
 * @BOBGUI_POS_TOP: The feature is at the top edge.
 * @BOBGUI_POS_BOTTOM: The feature is at the bottom edge.
 *
 * Describes which edge of a widget a certain feature is positioned at.
 *
 * For examples, see the tabs of a [class@Notebook], or the label
 * of a [class@Scale].
 */
typedef enum
{
  BOBGUI_POS_LEFT,
  BOBGUI_POS_RIGHT,
  BOBGUI_POS_TOP,
  BOBGUI_POS_BOTTOM
} BobguiPositionType;

/**
 * BobguiScrollType:
 * @BOBGUI_SCROLL_NONE: No scrolling.
 * @BOBGUI_SCROLL_JUMP: Jump to new location.
 * @BOBGUI_SCROLL_STEP_BACKWARD: Step backward.
 * @BOBGUI_SCROLL_STEP_FORWARD: Step forward.
 * @BOBGUI_SCROLL_PAGE_BACKWARD: Page backward.
 * @BOBGUI_SCROLL_PAGE_FORWARD: Page forward.
 * @BOBGUI_SCROLL_STEP_UP: Step up.
 * @BOBGUI_SCROLL_STEP_DOWN: Step down.
 * @BOBGUI_SCROLL_PAGE_UP: Page up.
 * @BOBGUI_SCROLL_PAGE_DOWN: Page down.
 * @BOBGUI_SCROLL_STEP_LEFT: Step to the left.
 * @BOBGUI_SCROLL_STEP_RIGHT: Step to the right.
 * @BOBGUI_SCROLL_PAGE_LEFT: Page to the left.
 * @BOBGUI_SCROLL_PAGE_RIGHT: Page to the right.
 * @BOBGUI_SCROLL_START: Scroll to start.
 * @BOBGUI_SCROLL_END: Scroll to end.
 *
 * Scrolling types.
 */
typedef enum
{
  BOBGUI_SCROLL_NONE,
  BOBGUI_SCROLL_JUMP,
  BOBGUI_SCROLL_STEP_BACKWARD,
  BOBGUI_SCROLL_STEP_FORWARD,
  BOBGUI_SCROLL_PAGE_BACKWARD,
  BOBGUI_SCROLL_PAGE_FORWARD,
  BOBGUI_SCROLL_STEP_UP,
  BOBGUI_SCROLL_STEP_DOWN,
  BOBGUI_SCROLL_PAGE_UP,
  BOBGUI_SCROLL_PAGE_DOWN,
  BOBGUI_SCROLL_STEP_LEFT,
  BOBGUI_SCROLL_STEP_RIGHT,
  BOBGUI_SCROLL_PAGE_LEFT,
  BOBGUI_SCROLL_PAGE_RIGHT,
  BOBGUI_SCROLL_START,
  BOBGUI_SCROLL_END
} BobguiScrollType;

/**
 * BobguiSelectionMode:
 * @BOBGUI_SELECTION_NONE: No selection is possible.
 * @BOBGUI_SELECTION_SINGLE: Zero or one element may be selected.
 * @BOBGUI_SELECTION_BROWSE: Exactly one element is selected.
 *   In some circumstances, such as initially or during a search
 *   operation, it’s possible for no element to be selected with
 *   %BOBGUI_SELECTION_BROWSE. What is really enforced is that the user
 *   can’t deselect a currently selected element except by selecting
 *   another element.
 * @BOBGUI_SELECTION_MULTIPLE: Any number of elements may be selected.
 *   The Ctrl key may be used to enlarge the selection, and Shift
 *   key to select between the focus and the child pointed to.
 *   Some widgets may also allow Click-drag to select a range of elements.
 *
 * Used to control what selections users are allowed to make.
 */
typedef enum
{
  BOBGUI_SELECTION_NONE,
  BOBGUI_SELECTION_SINGLE,
  BOBGUI_SELECTION_BROWSE,
  BOBGUI_SELECTION_MULTIPLE
} BobguiSelectionMode;

/* Widget states */

/**
 * BobguiWrapMode:
 * @BOBGUI_WRAP_NONE: do not wrap lines; just make the text area wider
 * @BOBGUI_WRAP_CHAR: wrap text, breaking lines anywhere the cursor can
 *   appear (between characters, usually - if you want to be technical,
 *   between graphemes, see pango_get_log_attrs())
 * @BOBGUI_WRAP_WORD: wrap text, breaking lines in between words
 * @BOBGUI_WRAP_WORD_CHAR: wrap text, breaking lines in between words, or if
 *   that is not enough, also between graphemes
 *
 * Describes a type of line wrapping.
 */
typedef enum
{
  BOBGUI_WRAP_NONE,
  BOBGUI_WRAP_CHAR,
  BOBGUI_WRAP_WORD,
  BOBGUI_WRAP_WORD_CHAR
} BobguiWrapMode;

/**
 * BobguiSortType:
 * @BOBGUI_SORT_ASCENDING: Sorting is in ascending order.
 * @BOBGUI_SORT_DESCENDING: Sorting is in descending order.
 *
 * Determines the direction of a sort.
 */
typedef enum
{
  BOBGUI_SORT_ASCENDING,
  BOBGUI_SORT_DESCENDING
} BobguiSortType;

/**
 * BobguiPrintPages:
 * @BOBGUI_PRINT_PAGES_ALL: All pages.
 * @BOBGUI_PRINT_PAGES_CURRENT: Current page.
 * @BOBGUI_PRINT_PAGES_RANGES: Range of pages.
 * @BOBGUI_PRINT_PAGES_SELECTION: Selected pages.
 *
 * See also bobgui_print_job_set_pages()
 */
typedef enum
{
  BOBGUI_PRINT_PAGES_ALL,
  BOBGUI_PRINT_PAGES_CURRENT,
  BOBGUI_PRINT_PAGES_RANGES,
  BOBGUI_PRINT_PAGES_SELECTION
} BobguiPrintPages;

/**
 * BobguiPageSet:
 * @BOBGUI_PAGE_SET_ALL: All pages.
 * @BOBGUI_PAGE_SET_EVEN: Even pages.
 * @BOBGUI_PAGE_SET_ODD: Odd pages.
 *
 * See also bobgui_print_job_set_page_set().
 */
typedef enum
{
  BOBGUI_PAGE_SET_ALL,
  BOBGUI_PAGE_SET_EVEN,
  BOBGUI_PAGE_SET_ODD
} BobguiPageSet;

/**
 * BobguiNumberUpLayout:
 * @BOBGUI_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM: ![](layout-lrtb.png)
 * @BOBGUI_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_BOTTOM_TO_TOP: ![](layout-lrbt.png)
 * @BOBGUI_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_TOP_TO_BOTTOM: ![](layout-rltb.png)
 * @BOBGUI_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_BOTTOM_TO_TOP: ![](layout-rlbt.png)
 * @BOBGUI_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_LEFT_TO_RIGHT: ![](layout-tblr.png)
 * @BOBGUI_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_RIGHT_TO_LEFT: ![](layout-tbrl.png)
 * @BOBGUI_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_LEFT_TO_RIGHT: ![](layout-btlr.png)
 * @BOBGUI_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_RIGHT_TO_LEFT: ![](layout-btrl.png)
 *
 * Used to determine the layout of pages on a sheet when printing
 * multiple pages per sheet.
 */
typedef enum
{
  BOBGUI_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_TOP_TO_BOTTOM, /*< nick=lrtb >*/
  BOBGUI_NUMBER_UP_LAYOUT_LEFT_TO_RIGHT_BOTTOM_TO_TOP, /*< nick=lrbt >*/
  BOBGUI_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_TOP_TO_BOTTOM, /*< nick=rltb >*/
  BOBGUI_NUMBER_UP_LAYOUT_RIGHT_TO_LEFT_BOTTOM_TO_TOP, /*< nick=rlbt >*/
  BOBGUI_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_LEFT_TO_RIGHT, /*< nick=tblr >*/
  BOBGUI_NUMBER_UP_LAYOUT_TOP_TO_BOTTOM_RIGHT_TO_LEFT, /*< nick=tbrl >*/
  BOBGUI_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_LEFT_TO_RIGHT, /*< nick=btlr >*/
  BOBGUI_NUMBER_UP_LAYOUT_BOTTOM_TO_TOP_RIGHT_TO_LEFT  /*< nick=btrl >*/
} BobguiNumberUpLayout;

/**
 * BobguiOrdering:
 * @BOBGUI_ORDERING_SMALLER: the first value is smaller than the second
 * @BOBGUI_ORDERING_EQUAL: the two values are equal
 * @BOBGUI_ORDERING_LARGER: the first value is larger than the second
 *
 * Describes the way two values can be compared.
 *
 * These values can be used with a [callback@GLib.CompareFunc]. However,
 * a `GCompareFunc` is allowed to return any integer values.
 * For converting such a value to a `BobguiOrdering` value, use
 * [func@Bobgui.Ordering.from_cmpfunc].
 */
typedef enum {
  BOBGUI_ORDERING_SMALLER = -1,
  BOBGUI_ORDERING_EQUAL = 0,
  BOBGUI_ORDERING_LARGER = 1
} BobguiOrdering;

/* The GI scanner does not handle static inline functions, because
 * of the `static` keyword; so we clip this out when parsing the
 * header, and we replace it with a real function in bobguisorter.c
 * that only exists when parsing the source for introspection.
 */
#ifdef __GI_SCANNER__
BobguiOrdering     bobgui_ordering_from_cmpfunc       (int cmpfunc_result);
#else
/**
 * bobgui_ordering_from_cmpfunc: (skip)
 * @cmpfunc_result: Result of a comparison function
 *
 * Converts the result of a `GCompareFunc` like strcmp() to a
 * `BobguiOrdering` value.
 *
 * Returns: the corresponding `BobguiOrdering`
 **/
static inline BobguiOrdering
bobgui_ordering_from_cmpfunc (int cmpfunc_result)
{
  return (BobguiOrdering) ((cmpfunc_result > 0) - (cmpfunc_result < 0));
}
#endif

/**
 * BobguiPageOrientation:
 * @BOBGUI_PAGE_ORIENTATION_PORTRAIT: Portrait mode.
 * @BOBGUI_PAGE_ORIENTATION_LANDSCAPE: Landscape mode.
 * @BOBGUI_PAGE_ORIENTATION_REVERSE_PORTRAIT: Reverse portrait mode.
 * @BOBGUI_PAGE_ORIENTATION_REVERSE_LANDSCAPE: Reverse landscape mode.
 *
 * See also bobgui_print_settings_set_orientation().
 */
typedef enum
{
  BOBGUI_PAGE_ORIENTATION_PORTRAIT,
  BOBGUI_PAGE_ORIENTATION_LANDSCAPE,
  BOBGUI_PAGE_ORIENTATION_REVERSE_PORTRAIT,
  BOBGUI_PAGE_ORIENTATION_REVERSE_LANDSCAPE
} BobguiPageOrientation;

/**
 * BobguiPrintQuality:
 * @BOBGUI_PRINT_QUALITY_LOW: Low quality.
 * @BOBGUI_PRINT_QUALITY_NORMAL: Normal quality.
 * @BOBGUI_PRINT_QUALITY_HIGH: High quality.
 * @BOBGUI_PRINT_QUALITY_DRAFT: Draft quality.
 *
 * See also bobgui_print_settings_set_quality().
 */
typedef enum
{
  BOBGUI_PRINT_QUALITY_LOW,
  BOBGUI_PRINT_QUALITY_NORMAL,
  BOBGUI_PRINT_QUALITY_HIGH,
  BOBGUI_PRINT_QUALITY_DRAFT
} BobguiPrintQuality;

/**
 * BobguiPrintDuplex:
 * @BOBGUI_PRINT_DUPLEX_SIMPLEX: No duplex.
 * @BOBGUI_PRINT_DUPLEX_HORIZONTAL: Horizontal duplex.
 * @BOBGUI_PRINT_DUPLEX_VERTICAL: Vertical duplex.
 *
 * See also bobgui_print_settings_set_duplex().
 */
typedef enum
{
  BOBGUI_PRINT_DUPLEX_SIMPLEX,
  BOBGUI_PRINT_DUPLEX_HORIZONTAL,
  BOBGUI_PRINT_DUPLEX_VERTICAL
} BobguiPrintDuplex;


/**
 * BobguiUnit:
 * @BOBGUI_UNIT_NONE: No units.
 * @BOBGUI_UNIT_POINTS: Dimensions in points.
 * @BOBGUI_UNIT_INCH: Dimensions in inches.
 * @BOBGUI_UNIT_MM: Dimensions in millimeters
 *
 * See also bobgui_print_settings_set_paper_width().
 */
typedef enum
{
  BOBGUI_UNIT_NONE,
  BOBGUI_UNIT_POINTS,
  BOBGUI_UNIT_INCH,
  BOBGUI_UNIT_MM
} BobguiUnit;

#define BOBGUI_UNIT_PIXEL BOBGUI_UNIT_NONE

/**
 * BobguiTreeViewGridLines:
 * @BOBGUI_TREE_VIEW_GRID_LINES_NONE: No grid lines.
 * @BOBGUI_TREE_VIEW_GRID_LINES_HORIZONTAL: Horizontal grid lines.
 * @BOBGUI_TREE_VIEW_GRID_LINES_VERTICAL: Vertical grid lines.
 * @BOBGUI_TREE_VIEW_GRID_LINES_BOTH: Horizontal and vertical grid lines.
 *
 * Used to indicate which grid lines to draw in a tree view.
 *
 * Deprecated: 4.20: There is no replacement
 */
typedef enum
{
  BOBGUI_TREE_VIEW_GRID_LINES_NONE,
  BOBGUI_TREE_VIEW_GRID_LINES_HORIZONTAL,
  BOBGUI_TREE_VIEW_GRID_LINES_VERTICAL,
  BOBGUI_TREE_VIEW_GRID_LINES_BOTH
} BobguiTreeViewGridLines;

/**
 * BobguiSizeGroupMode:
 * @BOBGUI_SIZE_GROUP_NONE: group has no effect
 * @BOBGUI_SIZE_GROUP_HORIZONTAL: group affects horizontal requisition
 * @BOBGUI_SIZE_GROUP_VERTICAL: group affects vertical requisition
 * @BOBGUI_SIZE_GROUP_BOTH: group affects both horizontal and vertical requisition
 *
 * The mode of the size group determines the directions in which the size
 * group affects the requested sizes of its component widgets.
 **/
typedef enum {
  BOBGUI_SIZE_GROUP_NONE,
  BOBGUI_SIZE_GROUP_HORIZONTAL,
  BOBGUI_SIZE_GROUP_VERTICAL,
  BOBGUI_SIZE_GROUP_BOTH
} BobguiSizeGroupMode;

/**
 * BobguiSizeRequestMode:
 * @BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH: Prefer height-for-width geometry management
 * @BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT: Prefer width-for-height geometry management
 * @BOBGUI_SIZE_REQUEST_CONSTANT_SIZE: Don’t trade height-for-width or width-for-height
 *
 * Specifies a preference for height-for-width or
 * width-for-height geometry management.
 */
typedef enum
{
  BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH = 0,
  BOBGUI_SIZE_REQUEST_WIDTH_FOR_HEIGHT,
  BOBGUI_SIZE_REQUEST_CONSTANT_SIZE
} BobguiSizeRequestMode;

/**
 * BobguiScrollablePolicy:
 * @BOBGUI_SCROLL_MINIMUM: Scrollable adjustments are based on the minimum size
 * @BOBGUI_SCROLL_NATURAL: Scrollable adjustments are based on the natural size
 *
 * Defines the policy to be used in a scrollable widget when updating
 * the scrolled window adjustments in a given orientation.
 */
typedef enum
{
  BOBGUI_SCROLL_MINIMUM = 0,
  BOBGUI_SCROLL_NATURAL
} BobguiScrollablePolicy;

/**
 * BobguiStateFlags:
 * @BOBGUI_STATE_FLAG_NORMAL: State during normal operation
 * @BOBGUI_STATE_FLAG_ACTIVE: Widget is active
 * @BOBGUI_STATE_FLAG_PRELIGHT: Widget has a mouse pointer over it
 * @BOBGUI_STATE_FLAG_SELECTED: Widget is selected
 * @BOBGUI_STATE_FLAG_INSENSITIVE: Widget is insensitive
 * @BOBGUI_STATE_FLAG_INCONSISTENT: Widget is inconsistent
 * @BOBGUI_STATE_FLAG_FOCUSED: Widget has the keyboard focus
 * @BOBGUI_STATE_FLAG_BACKDROP: Widget is in a background toplevel window
 * @BOBGUI_STATE_FLAG_DIR_LTR: Widget is in left-to-right text direction
 * @BOBGUI_STATE_FLAG_DIR_RTL: Widget is in right-to-left text direction
 * @BOBGUI_STATE_FLAG_LINK: Widget is a link
 * @BOBGUI_STATE_FLAG_VISITED: The location the widget points to has already been visited
 * @BOBGUI_STATE_FLAG_CHECKED: Widget is checked
 * @BOBGUI_STATE_FLAG_DROP_ACTIVE: Widget is highlighted as a drop target for DND
 * @BOBGUI_STATE_FLAG_FOCUS_VISIBLE: Widget has the visible focus
 * @BOBGUI_STATE_FLAG_FOCUS_WITHIN: Widget contains the keyboard focus
 *
 * Describes a widget state.
 *
 * Widget states are used to match the widget against CSS pseudo-classes.
 * Note that BOBGUI extends the regular CSS classes and sometimes uses
 * different names.
 */
typedef enum
{
  BOBGUI_STATE_FLAG_NORMAL        = 0,
  BOBGUI_STATE_FLAG_ACTIVE        = 1 << 0,
  BOBGUI_STATE_FLAG_PRELIGHT      = 1 << 1,
  BOBGUI_STATE_FLAG_SELECTED      = 1 << 2,
  BOBGUI_STATE_FLAG_INSENSITIVE   = 1 << 3,
  BOBGUI_STATE_FLAG_INCONSISTENT  = 1 << 4,
  BOBGUI_STATE_FLAG_FOCUSED       = 1 << 5,
  BOBGUI_STATE_FLAG_BACKDROP      = 1 << 6,
  BOBGUI_STATE_FLAG_DIR_LTR       = 1 << 7,
  BOBGUI_STATE_FLAG_DIR_RTL       = 1 << 8,
  BOBGUI_STATE_FLAG_LINK          = 1 << 9,
  BOBGUI_STATE_FLAG_VISITED       = 1 << 10,
  BOBGUI_STATE_FLAG_CHECKED       = 1 << 11,
  BOBGUI_STATE_FLAG_DROP_ACTIVE   = 1 << 12,
  BOBGUI_STATE_FLAG_FOCUS_VISIBLE = 1 << 13,
  BOBGUI_STATE_FLAG_FOCUS_WITHIN  = 1 << 14
} BobguiStateFlags;

/**
 * BobguiBorderStyle:
 * @BOBGUI_BORDER_STYLE_NONE: No visible border
 * @BOBGUI_BORDER_STYLE_HIDDEN: Same as %BOBGUI_BORDER_STYLE_NONE
 * @BOBGUI_BORDER_STYLE_SOLID: A single line segment
 * @BOBGUI_BORDER_STYLE_INSET: Looks as if the content is sunken into the canvas
 * @BOBGUI_BORDER_STYLE_OUTSET: Looks as if the content is coming out of the canvas
 * @BOBGUI_BORDER_STYLE_DOTTED: A series of round dots
 * @BOBGUI_BORDER_STYLE_DASHED: A series of square-ended dashes
 * @BOBGUI_BORDER_STYLE_DOUBLE: Two parallel lines with some space between them
 * @BOBGUI_BORDER_STYLE_GROOVE: Looks as if it were carved in the canvas
 * @BOBGUI_BORDER_STYLE_RIDGE: Looks as if it were coming out of the canvas
 *
 * Describes how the border of a UI element should be rendered.
 */
typedef enum {
  BOBGUI_BORDER_STYLE_NONE,
  BOBGUI_BORDER_STYLE_HIDDEN,
  BOBGUI_BORDER_STYLE_SOLID,
  BOBGUI_BORDER_STYLE_INSET,
  BOBGUI_BORDER_STYLE_OUTSET,
  BOBGUI_BORDER_STYLE_DOTTED,
  BOBGUI_BORDER_STYLE_DASHED,
  BOBGUI_BORDER_STYLE_DOUBLE,
  BOBGUI_BORDER_STYLE_GROOVE,
  BOBGUI_BORDER_STYLE_RIDGE
} BobguiBorderStyle;

/**
 * BobguiLevelBarMode:
 * @BOBGUI_LEVEL_BAR_MODE_CONTINUOUS: the bar has a continuous mode
 * @BOBGUI_LEVEL_BAR_MODE_DISCRETE: the bar has a discrete mode
 *
 * Describes how [class@LevelBar] contents should be rendered.
 *
 * Note that this enumeration could be extended with additional modes
 * in the future.
 */
typedef enum {
  BOBGUI_LEVEL_BAR_MODE_CONTINUOUS,
  BOBGUI_LEVEL_BAR_MODE_DISCRETE
} BobguiLevelBarMode;

/**
 * BobguiInputPurpose:
 * @BOBGUI_INPUT_PURPOSE_FREE_FORM: Allow any character
 * @BOBGUI_INPUT_PURPOSE_ALPHA: Allow only alphabetic characters
 * @BOBGUI_INPUT_PURPOSE_DIGITS: Allow only digits
 * @BOBGUI_INPUT_PURPOSE_NUMBER: Edited field expects numbers
 * @BOBGUI_INPUT_PURPOSE_PHONE: Edited field expects phone number
 * @BOBGUI_INPUT_PURPOSE_URL: Edited field expects URL
 * @BOBGUI_INPUT_PURPOSE_EMAIL: Edited field expects email address
 * @BOBGUI_INPUT_PURPOSE_NAME: Edited field expects the name of a person
 * @BOBGUI_INPUT_PURPOSE_PASSWORD: Like %BOBGUI_INPUT_PURPOSE_FREE_FORM, but characters are hidden
 * @BOBGUI_INPUT_PURPOSE_PIN: Like %BOBGUI_INPUT_PURPOSE_DIGITS, but characters are hidden
 * @BOBGUI_INPUT_PURPOSE_TERMINAL: Allow any character, in addition to control codes
 *
 * Describes primary purpose of the input widget.
 *
 * This information is useful for on-screen keyboards and similar input
 * methods to decide which keys should be presented to the user.
 *
 * Note that the purpose is not meant to impose a totally strict rule
 * about allowed characters, and does not replace input validation.
 * It is fine for an on-screen keyboard to let the user override the
 * character set restriction that is expressed by the purpose. The
 * application is expected to validate the entry contents, even if
 * it specified a purpose.
 *
 * The difference between %BOBGUI_INPUT_PURPOSE_DIGITS and
 * %BOBGUI_INPUT_PURPOSE_NUMBER is that the former accepts only digits
 * while the latter also some punctuation (like commas or points, plus,
 * minus) and “e” or “E” as in 3.14E+000.
 *
 * This enumeration may be extended in the future; input methods should
 * interpret unknown values as “free form”.
 */
typedef enum
{
  BOBGUI_INPUT_PURPOSE_FREE_FORM,
  BOBGUI_INPUT_PURPOSE_ALPHA,
  BOBGUI_INPUT_PURPOSE_DIGITS,
  BOBGUI_INPUT_PURPOSE_NUMBER,
  BOBGUI_INPUT_PURPOSE_PHONE,
  BOBGUI_INPUT_PURPOSE_URL,
  BOBGUI_INPUT_PURPOSE_EMAIL,
  BOBGUI_INPUT_PURPOSE_NAME,
  BOBGUI_INPUT_PURPOSE_PASSWORD,
  BOBGUI_INPUT_PURPOSE_PIN,
  BOBGUI_INPUT_PURPOSE_TERMINAL,
} BobguiInputPurpose;

/**
 * BobguiInputHints:
 * @BOBGUI_INPUT_HINT_NONE: No special behaviour suggested
 * @BOBGUI_INPUT_HINT_SPELLCHECK: Suggest checking for typos
 * @BOBGUI_INPUT_HINT_NO_SPELLCHECK: Suggest not checking for typos
 * @BOBGUI_INPUT_HINT_WORD_COMPLETION: Suggest word completion
 * @BOBGUI_INPUT_HINT_LOWERCASE: Suggest to convert all text to lowercase
 * @BOBGUI_INPUT_HINT_UPPERCASE_CHARS: Suggest to capitalize all text
 * @BOBGUI_INPUT_HINT_UPPERCASE_WORDS: Suggest to capitalize the first
 *   character of each word
 * @BOBGUI_INPUT_HINT_UPPERCASE_SENTENCES: Suggest to capitalize the
 *   first word of each sentence
 * @BOBGUI_INPUT_HINT_INHIBIT_OSK: Suggest to not show an onscreen keyboard
 *   (e.g for a calculator that already has all the keys).
 * @BOBGUI_INPUT_HINT_VERTICAL_WRITING: The text is vertical
 * @BOBGUI_INPUT_HINT_EMOJI: Suggest offering Emoji support
 * @BOBGUI_INPUT_HINT_NO_EMOJI: Suggest not offering Emoji support
 * @BOBGUI_INPUT_HINT_PRIVATE: Request that the input method should not
 *    update personalized data (like typing history)
 *
 * Describes hints that might be taken into account by input methods
 * or applications.
 *
 * Note that input methods may already tailor their behaviour according
 * to the [enum@InputPurpose] of the entry.
 *
 * Some common sense is expected when using these flags - mixing
 * %BOBGUI_INPUT_HINT_LOWERCASE with any of the uppercase hints makes no sense.
 *
 * This enumeration may be extended in the future; input methods should
 * ignore unknown values.
 */
typedef enum
{
  BOBGUI_INPUT_HINT_NONE                = 0,
  BOBGUI_INPUT_HINT_SPELLCHECK          = 1 << 0,
  BOBGUI_INPUT_HINT_NO_SPELLCHECK       = 1 << 1,
  BOBGUI_INPUT_HINT_WORD_COMPLETION     = 1 << 2,
  BOBGUI_INPUT_HINT_LOWERCASE           = 1 << 3,
  BOBGUI_INPUT_HINT_UPPERCASE_CHARS     = 1 << 4,
  BOBGUI_INPUT_HINT_UPPERCASE_WORDS     = 1 << 5,
  BOBGUI_INPUT_HINT_UPPERCASE_SENTENCES = 1 << 6,
  BOBGUI_INPUT_HINT_INHIBIT_OSK         = 1 << 7,
  BOBGUI_INPUT_HINT_VERTICAL_WRITING    = 1 << 8,
  BOBGUI_INPUT_HINT_EMOJI               = 1 << 9,
  BOBGUI_INPUT_HINT_NO_EMOJI            = 1 << 10,
  BOBGUI_INPUT_HINT_PRIVATE             = 1 << 11,
} BobguiInputHints;

/**
 * BobguiPropagationPhase:
 * @BOBGUI_PHASE_NONE: Events are not delivered.
 * @BOBGUI_PHASE_CAPTURE: Events are delivered in the capture phase. The
 *   capture phase happens before the bubble phase, runs from the toplevel down
 *   to the event widget. This option should only be used on containers that
 *   might possibly handle events before their children do.
 * @BOBGUI_PHASE_BUBBLE: Events are delivered in the bubble phase. The bubble
 *   phase happens after the capture phase, and before the default handlers
 *   are run. This phase runs from the event widget, up to the toplevel.
 * @BOBGUI_PHASE_TARGET: Events are delivered in the default widget event handlers,
 *   note that widget implementations must chain up on button, motion, touch and
 *   grab broken handlers for controllers in this phase to be run.
 *
 * Describes the stage at which events are fed into a [class@EventController].
 */
typedef enum
{
  BOBGUI_PHASE_NONE,
  BOBGUI_PHASE_CAPTURE,
  BOBGUI_PHASE_BUBBLE,
  BOBGUI_PHASE_TARGET
} BobguiPropagationPhase;

/**
 * BobguiPropagationLimit:
 * @BOBGUI_LIMIT_NONE: Events are handled regardless of what their
 *   target is.
 * @BOBGUI_LIMIT_SAME_NATIVE: Events are only handled if their target is in
 *   the same [iface@Native] (or widget with [property@Bobgui.Widget:limit-events]
 *   set) as the event controllers widget.
 *   Note that some event types have two targets (origin and destination).
 *
 * Describes limits of a [class@EventController] for handling events
 * targeting other widgets.
 */
typedef enum
{
  BOBGUI_LIMIT_NONE,
  BOBGUI_LIMIT_SAME_NATIVE
} BobguiPropagationLimit;

/**
 * BobguiEventSequenceState:
 * @BOBGUI_EVENT_SEQUENCE_NONE: The sequence is handled, but not grabbed.
 * @BOBGUI_EVENT_SEQUENCE_CLAIMED: The sequence is handled and grabbed.
 * @BOBGUI_EVENT_SEQUENCE_DENIED: The sequence is denied.
 *
 * Describes the state of a [struct@Gdk.EventSequence] in a [class@Gesture].
 */
typedef enum
{
  BOBGUI_EVENT_SEQUENCE_NONE,
  BOBGUI_EVENT_SEQUENCE_CLAIMED,
  BOBGUI_EVENT_SEQUENCE_DENIED
} BobguiEventSequenceState;

/**
 * BobguiPanDirection:
 * @BOBGUI_PAN_DIRECTION_LEFT: panned towards the left
 * @BOBGUI_PAN_DIRECTION_RIGHT: panned towards the right
 * @BOBGUI_PAN_DIRECTION_UP: panned upwards
 * @BOBGUI_PAN_DIRECTION_DOWN: panned downwards
 *
 * Describes the panning direction of a [class@GesturePan].
 */
typedef enum
{
  BOBGUI_PAN_DIRECTION_LEFT,
  BOBGUI_PAN_DIRECTION_RIGHT,
  BOBGUI_PAN_DIRECTION_UP,
  BOBGUI_PAN_DIRECTION_DOWN
} BobguiPanDirection;

/**
 * BobguiShortcutScope:
 * @BOBGUI_SHORTCUT_SCOPE_LOCAL: Shortcuts are handled inside
 *   the widget the controller belongs to.
 * @BOBGUI_SHORTCUT_SCOPE_MANAGED: Shortcuts are handled by
 *   the first ancestor that is a [iface@ShortcutManager]
 * @BOBGUI_SHORTCUT_SCOPE_GLOBAL: Shortcuts are handled by
 *   the root widget.
 *
 * Describes where [class@Shortcut]s added to a
 * [class@ShortcutController] get handled.
 */
typedef enum
{
  BOBGUI_SHORTCUT_SCOPE_LOCAL,
  BOBGUI_SHORTCUT_SCOPE_MANAGED,
  BOBGUI_SHORTCUT_SCOPE_GLOBAL
} BobguiShortcutScope;

/**
 * BobguiPickFlags:
 * @BOBGUI_PICK_DEFAULT: The default behavior, include widgets that are receiving events
 * @BOBGUI_PICK_INSENSITIVE: Include widgets that are insensitive
 * @BOBGUI_PICK_NON_TARGETABLE: Include widgets that are marked as non-targetable. See [property@Widget:can-target]
 *
 * Flags that influence the behavior of [method@Widget.pick].
 */
typedef enum {
  BOBGUI_PICK_DEFAULT        = 0,
  BOBGUI_PICK_INSENSITIVE    = 1 << 0,
  BOBGUI_PICK_NON_TARGETABLE = 1 << 1
} BobguiPickFlags;

/**
 * BobguiConstraintRelation:
 * @BOBGUI_CONSTRAINT_RELATION_EQ: Equal
 * @BOBGUI_CONSTRAINT_RELATION_LE: Less than, or equal
 * @BOBGUI_CONSTRAINT_RELATION_GE: Greater than, or equal
 *
 * The relation between two terms of a constraint.
 */
typedef enum {
  BOBGUI_CONSTRAINT_RELATION_LE = -1,
  BOBGUI_CONSTRAINT_RELATION_EQ = 0,
  BOBGUI_CONSTRAINT_RELATION_GE = 1
} BobguiConstraintRelation;

/**
 * BobguiConstraintStrength:
 * @BOBGUI_CONSTRAINT_STRENGTH_REQUIRED: The constraint is required towards solving the layout
 * @BOBGUI_CONSTRAINT_STRENGTH_STRONG: A strong constraint
 * @BOBGUI_CONSTRAINT_STRENGTH_MEDIUM: A medium constraint
 * @BOBGUI_CONSTRAINT_STRENGTH_WEAK: A weak constraint
 *
 * The strength of a constraint, expressed as a symbolic constant.
 *
 * The strength of a [class@Constraint] can be expressed with any positive
 * integer; the values of this enumeration can be used for readability.
 */
typedef enum {
  BOBGUI_CONSTRAINT_STRENGTH_REQUIRED = 1001001000,
  BOBGUI_CONSTRAINT_STRENGTH_STRONG   = 1000000000,
  BOBGUI_CONSTRAINT_STRENGTH_MEDIUM   = 1000,
  BOBGUI_CONSTRAINT_STRENGTH_WEAK     = 1
} BobguiConstraintStrength;

/**
 * BobguiConstraintAttribute:
 * @BOBGUI_CONSTRAINT_ATTRIBUTE_NONE: No attribute, used for constant
 *   relations
 * @BOBGUI_CONSTRAINT_ATTRIBUTE_LEFT: The left edge of a widget, regardless of
 *   text direction
 * @BOBGUI_CONSTRAINT_ATTRIBUTE_RIGHT: The right edge of a widget, regardless
 *   of text direction
 * @BOBGUI_CONSTRAINT_ATTRIBUTE_TOP: The top edge of a widget
 * @BOBGUI_CONSTRAINT_ATTRIBUTE_BOTTOM: The bottom edge of a widget
 * @BOBGUI_CONSTRAINT_ATTRIBUTE_START: The leading edge of a widget, depending
 *   on text direction; equivalent to %BOBGUI_CONSTRAINT_ATTRIBUTE_LEFT for LTR
 *   languages, and %BOBGUI_CONSTRAINT_ATTRIBUTE_RIGHT for RTL ones
 * @BOBGUI_CONSTRAINT_ATTRIBUTE_END: The trailing edge of a widget, depending
 *   on text direction; equivalent to %BOBGUI_CONSTRAINT_ATTRIBUTE_RIGHT for LTR
 *   languages, and %BOBGUI_CONSTRAINT_ATTRIBUTE_LEFT for RTL ones
 * @BOBGUI_CONSTRAINT_ATTRIBUTE_WIDTH: The width of a widget
 * @BOBGUI_CONSTRAINT_ATTRIBUTE_HEIGHT: The height of a widget
 * @BOBGUI_CONSTRAINT_ATTRIBUTE_CENTER_X: The center of a widget, on the
 *   horizontal axis
 * @BOBGUI_CONSTRAINT_ATTRIBUTE_CENTER_Y: The center of a widget, on the
 *   vertical axis
 * @BOBGUI_CONSTRAINT_ATTRIBUTE_BASELINE: The baseline of a widget
 *
 * The widget attributes that can be used when creating a [class@Constraint].
 */
typedef enum {
  BOBGUI_CONSTRAINT_ATTRIBUTE_NONE,
  BOBGUI_CONSTRAINT_ATTRIBUTE_LEFT,
  BOBGUI_CONSTRAINT_ATTRIBUTE_RIGHT,
  BOBGUI_CONSTRAINT_ATTRIBUTE_TOP,
  BOBGUI_CONSTRAINT_ATTRIBUTE_BOTTOM,
  BOBGUI_CONSTRAINT_ATTRIBUTE_START,
  BOBGUI_CONSTRAINT_ATTRIBUTE_END,
  BOBGUI_CONSTRAINT_ATTRIBUTE_WIDTH,
  BOBGUI_CONSTRAINT_ATTRIBUTE_HEIGHT,
  BOBGUI_CONSTRAINT_ATTRIBUTE_CENTER_X,
  BOBGUI_CONSTRAINT_ATTRIBUTE_CENTER_Y,
  BOBGUI_CONSTRAINT_ATTRIBUTE_BASELINE
} BobguiConstraintAttribute;

/**
 * BobguiConstraintVflParserError:
 * @BOBGUI_CONSTRAINT_VFL_PARSER_ERROR_INVALID_SYMBOL: Invalid or unknown symbol
 * @BOBGUI_CONSTRAINT_VFL_PARSER_ERROR_INVALID_ATTRIBUTE: Invalid or unknown attribute
 * @BOBGUI_CONSTRAINT_VFL_PARSER_ERROR_INVALID_VIEW: Invalid or unknown view
 * @BOBGUI_CONSTRAINT_VFL_PARSER_ERROR_INVALID_METRIC: Invalid or unknown metric
 * @BOBGUI_CONSTRAINT_VFL_PARSER_ERROR_INVALID_PRIORITY: Invalid or unknown priority
 * @BOBGUI_CONSTRAINT_VFL_PARSER_ERROR_INVALID_RELATION: Invalid or unknown relation
 *
 * Domain for VFL parsing errors.
 */
typedef enum { /*< prefix=BOBGUI_CONSTRAINT_VFL_PARSER_ERROR >*/
  BOBGUI_CONSTRAINT_VFL_PARSER_ERROR_INVALID_SYMBOL,
  BOBGUI_CONSTRAINT_VFL_PARSER_ERROR_INVALID_ATTRIBUTE,
  BOBGUI_CONSTRAINT_VFL_PARSER_ERROR_INVALID_VIEW,
  BOBGUI_CONSTRAINT_VFL_PARSER_ERROR_INVALID_METRIC,
  BOBGUI_CONSTRAINT_VFL_PARSER_ERROR_INVALID_PRIORITY,
  BOBGUI_CONSTRAINT_VFL_PARSER_ERROR_INVALID_RELATION
} BobguiConstraintVflParserError;

/**
 * BobguiSystemSetting:
 * @BOBGUI_SYSTEM_SETTING_DPI: the [property@Bobgui.Settings:bobgui-xft-dpi] setting has changed
 * @BOBGUI_SYSTEM_SETTING_FONT_NAME: The [property@Bobgui.Settings:bobgui-font-name] setting has changed
 * @BOBGUI_SYSTEM_SETTING_FONT_CONFIG: The font configuration has changed in a way that
 *   requires text to be redrawn. This can be any of the
 *   [property@Bobgui.Settings:bobgui-xft-antialias],
 *   [property@Bobgui.Settings:bobgui-xft-hinting],
 *   [property@Bobgui.Settings:bobgui-xft-hintstyle],
 *   [property@Bobgui.Settings:bobgui-xft-rgba] or
 *   [property@Bobgui.Settings:bobgui-fontconfig-timestamp] settings
 * @BOBGUI_SYSTEM_SETTING_DISPLAY: The display has changed
 * @BOBGUI_SYSTEM_SETTING_ICON_THEME: The icon theme has changed in a way that requires
 *   icons to be looked up again
 *
 * Values that can be passed to the [vfunc@Bobgui.Widget.system_setting_changed]
 * vfunc.
 *
 * The values indicate which system setting has changed.
 * Widgets may need to drop caches, or react otherwise.
 *
 * Most of the values correspond to [class@Settings] properties.
 *
 * More values may be added over time.
 */
typedef enum {
  BOBGUI_SYSTEM_SETTING_DPI,
  BOBGUI_SYSTEM_SETTING_FONT_NAME,
  BOBGUI_SYSTEM_SETTING_FONT_CONFIG,
  BOBGUI_SYSTEM_SETTING_DISPLAY,
  BOBGUI_SYSTEM_SETTING_ICON_THEME
} BobguiSystemSetting;

/**
 * BobguiSymbolicColor:
 * @BOBGUI_SYMBOLIC_COLOR_FOREGROUND: The default foreground color
 * @BOBGUI_SYMBOLIC_COLOR_ERROR: Indication color for errors
 * @BOBGUI_SYMBOLIC_COLOR_WARNING: Indication color for warnings
 * @BOBGUI_SYMBOLIC_COLOR_SUCCESS: Indication color for success
 *
 * The indexes of colors passed to symbolic color rendering, such as
 * [vfunc@Bobgui.SymbolicPaintable.snapshot_symbolic].
 *
 * More values may be added over time.
 *
 * Since: 4.6
 */
/**
 * BOBGUI_SYMBOLIC_COLOR_ACCENT:
 *
 * The system accent color.
 *
 * Since: 4.22
 */
typedef enum {
  BOBGUI_SYMBOLIC_COLOR_FOREGROUND,
  BOBGUI_SYMBOLIC_COLOR_ERROR,
  BOBGUI_SYMBOLIC_COLOR_WARNING,
  BOBGUI_SYMBOLIC_COLOR_SUCCESS,
  BOBGUI_SYMBOLIC_COLOR_ACCENT,
} BobguiSymbolicColor;

/**
 * BobguiAccessibleRole:
 * @BOBGUI_ACCESSIBLE_ROLE_ALERT: An element with important, and usually
 *   time-sensitive, information
 * @BOBGUI_ACCESSIBLE_ROLE_ALERT_DIALOG: A type of dialog that contains an
 *   alert message
 * @BOBGUI_ACCESSIBLE_ROLE_BANNER: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_BUTTON: An input element that allows for
 *   user-triggered actions when clicked or pressed
 * @BOBGUI_ACCESSIBLE_ROLE_CAPTION: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_CELL: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_CHECKBOX: A checkable input element that has
 *   three possible values: `true`, `false`, or `mixed`
 * @BOBGUI_ACCESSIBLE_ROLE_COLUMN_HEADER: A header in a columned list.
 * @BOBGUI_ACCESSIBLE_ROLE_COMBO_BOX: An input that controls another element,
 *   such as a list or a grid, that can dynamically pop up to help the user
 *   set the value of the input
 * @BOBGUI_ACCESSIBLE_ROLE_COMMAND: Abstract role.
 * @BOBGUI_ACCESSIBLE_ROLE_COMPOSITE: Abstract role.
 * @BOBGUI_ACCESSIBLE_ROLE_DIALOG: A dialog is a window that is designed to interrupt
 *   the current processing of an application in order to prompt the user to enter
 *   information or require a response.
 * @BOBGUI_ACCESSIBLE_ROLE_DOCUMENT: Content that assistive technology users may want to
 *   browse in a reading mode.
 * @BOBGUI_ACCESSIBLE_ROLE_FEED: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_FORM: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_GENERIC: A nameless container that has no semantic meaning
 *   of its own. This is the role that BOBGUI uses by default for widgets.
 * @BOBGUI_ACCESSIBLE_ROLE_GRID: A grid of items.
 * @BOBGUI_ACCESSIBLE_ROLE_GRID_CELL: An item in a grid or tree grid.
 * @BOBGUI_ACCESSIBLE_ROLE_GROUP: An element that groups multiple related widgets. BOBGUI uses
 *   this role for various containers, like [class@Bobgui.HeaderBar] or [class@Bobgui.Notebook].
 * @BOBGUI_ACCESSIBLE_ROLE_HEADING: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_IMG: An image.
 * @BOBGUI_ACCESSIBLE_ROLE_INPUT: Abstract role.
 * @BOBGUI_ACCESSIBLE_ROLE_LABEL: A visible name or caption for a user interface component.
 * @BOBGUI_ACCESSIBLE_ROLE_LANDMARK: Abstract role.
 * @BOBGUI_ACCESSIBLE_ROLE_LEGEND: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_LINK: A clickable link.
 * @BOBGUI_ACCESSIBLE_ROLE_LIST: A list of items.
 * @BOBGUI_ACCESSIBLE_ROLE_LIST_BOX: Unused.
 * @BOBGUI_ACCESSIBLE_ROLE_LIST_ITEM: An item in a list.
 * @BOBGUI_ACCESSIBLE_ROLE_LOG: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_MAIN: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_MARQUEE: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_MATH: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_METER: An element that represents a value within a known range.
 * @BOBGUI_ACCESSIBLE_ROLE_MENU: A menu.
 * @BOBGUI_ACCESSIBLE_ROLE_MENU_BAR: A menubar.
 * @BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM: An item in a menu.
 * @BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_CHECKBOX: A check item in a menu.
 * @BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_RADIO: A radio item in a menu.
 * @BOBGUI_ACCESSIBLE_ROLE_NAVIGATION: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_NONE: An element that is not represented to accessibility technologies.
 *   This role is synonymous to @BOBGUI_ACCESSIBLE_ROLE_PRESENTATION.
 * @BOBGUI_ACCESSIBLE_ROLE_NOTE: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_OPTION: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_PRESENTATION: An element that is not represented to accessibility technologies.
 *   This role is synonymous to @BOBGUI_ACCESSIBLE_ROLE_NONE.
 * @BOBGUI_ACCESSIBLE_ROLE_PROGRESS_BAR: An element that displays the progress
 *   status for tasks that take a long time.
 * @BOBGUI_ACCESSIBLE_ROLE_RADIO: A checkable input in a group of radio roles,
 *   only one of which can be checked at a time.
 * @BOBGUI_ACCESSIBLE_ROLE_RADIO_GROUP: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_RANGE: Abstract role.
 * @BOBGUI_ACCESSIBLE_ROLE_REGION: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_ROW: A row in a columned list.
 * @BOBGUI_ACCESSIBLE_ROLE_ROW_GROUP: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_ROW_HEADER: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_SCROLLBAR: A graphical object that controls the scrolling
 *   of content within a viewing area, regardless of whether the content is fully
 *   displayed within the viewing area.
 * @BOBGUI_ACCESSIBLE_ROLE_SEARCH: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_SEARCH_BOX: A type of textbox intended for specifying
 *   search criteria.
 * @BOBGUI_ACCESSIBLE_ROLE_SECTION: Abstract role.
 * @BOBGUI_ACCESSIBLE_ROLE_SECTION_HEAD: Abstract role.
 * @BOBGUI_ACCESSIBLE_ROLE_SELECT: Abstract role.
 * @BOBGUI_ACCESSIBLE_ROLE_SEPARATOR: A divider that separates and distinguishes
 *   sections of content or groups of menuitems.
 * @BOBGUI_ACCESSIBLE_ROLE_SLIDER: A user input where the user selects a value
 *   from within a given range.
 * @BOBGUI_ACCESSIBLE_ROLE_SPIN_BUTTON: A form of range that expects the user to
 *   select from among discrete choices.
 * @BOBGUI_ACCESSIBLE_ROLE_STATUS: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_STRUCTURE: Abstract role.
 * @BOBGUI_ACCESSIBLE_ROLE_SWITCH: A type of checkbox that represents on/off values,
 *   as opposed to checked/unchecked values.
 * @BOBGUI_ACCESSIBLE_ROLE_TAB: An item in a list of tab used for switching pages.
 * @BOBGUI_ACCESSIBLE_ROLE_TABLE: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_TAB_LIST: A list of tabs for switching pages.
 * @BOBGUI_ACCESSIBLE_ROLE_TAB_PANEL: A page in a notebook or stack.
 * @BOBGUI_ACCESSIBLE_ROLE_TEXT_BOX: A type of input that allows free-form text
 *   as its value.
 * @BOBGUI_ACCESSIBLE_ROLE_TIME: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_TIMER: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_TOOLBAR: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_TOOLTIP: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_TREE: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_TREE_GRID: A treeview-like, columned list.
 * @BOBGUI_ACCESSIBLE_ROLE_TREE_ITEM: Unused
 * @BOBGUI_ACCESSIBLE_ROLE_WIDGET: Abstract role for interactive components of a
 *   graphical user interface
 * @BOBGUI_ACCESSIBLE_ROLE_WINDOW: Abstract role for windows.
 *
 * The accessible role for a [iface@Accessible] implementation.
 *
 * Abstract roles are only used as part of the ontology; application
 * developers must not use abstract roles in their code.
 */

/**
 * BOBGUI_ACCESSIBLE_ROLE_TOGGLE_BUTTON:
 *
 * A type of push button which stays pressed until depressed by a second
 * activation.
 *
 * Since: 4.10
 */

/**
 * BOBGUI_ACCESSIBLE_ROLE_APPLICATION:
 *
 * A toplevel element of a graphical user interface.
 *
 * This is the role that BOBGUI uses by default for windows.
 *
 * Since: 4.12
 */

/**
 * BOBGUI_ACCESSIBLE_ROLE_PARAGRAPH:
 *
 * A paragraph of content.
 *
 * Since: 4.14
 */

/**
 * BOBGUI_ACCESSIBLE_ROLE_BLOCK_QUOTE:
 *
 * A section of content that is quoted from another source.
 *
 * Since: 4.14
 */

/**
 * BOBGUI_ACCESSIBLE_ROLE_ARTICLE:
 *
 * A section of a page that consists of a composition that forms an independent
 * part of a document, page, or site.
 *
 * Since: 4.14
 */

/**
 * BOBGUI_ACCESSIBLE_ROLE_COMMENT:
 *
 * A comment contains content expressing reaction to other content.
 *
 * Since: 4.14
 */

/**
 * BOBGUI_ACCESSIBLE_ROLE_TERMINAL:
 *
 * A virtual terminal.
 *
 * Since: 4.14
 */
typedef enum {
  BOBGUI_ACCESSIBLE_ROLE_ALERT,
  BOBGUI_ACCESSIBLE_ROLE_ALERT_DIALOG,
  BOBGUI_ACCESSIBLE_ROLE_BANNER,
  BOBGUI_ACCESSIBLE_ROLE_BUTTON,
  BOBGUI_ACCESSIBLE_ROLE_CAPTION,
  BOBGUI_ACCESSIBLE_ROLE_CELL,
  BOBGUI_ACCESSIBLE_ROLE_CHECKBOX,
  BOBGUI_ACCESSIBLE_ROLE_COLUMN_HEADER,
  BOBGUI_ACCESSIBLE_ROLE_COMBO_BOX,
  BOBGUI_ACCESSIBLE_ROLE_COMMAND,
  BOBGUI_ACCESSIBLE_ROLE_COMPOSITE,
  BOBGUI_ACCESSIBLE_ROLE_DIALOG,
  BOBGUI_ACCESSIBLE_ROLE_DOCUMENT,
  BOBGUI_ACCESSIBLE_ROLE_FEED,
  BOBGUI_ACCESSIBLE_ROLE_FORM,
  BOBGUI_ACCESSIBLE_ROLE_GENERIC,
  BOBGUI_ACCESSIBLE_ROLE_GRID,
  BOBGUI_ACCESSIBLE_ROLE_GRID_CELL,
  BOBGUI_ACCESSIBLE_ROLE_GROUP,
  BOBGUI_ACCESSIBLE_ROLE_HEADING,
  BOBGUI_ACCESSIBLE_ROLE_IMG,
  BOBGUI_ACCESSIBLE_ROLE_INPUT,
  BOBGUI_ACCESSIBLE_ROLE_LABEL,
  BOBGUI_ACCESSIBLE_ROLE_LANDMARK,
  BOBGUI_ACCESSIBLE_ROLE_LEGEND,
  BOBGUI_ACCESSIBLE_ROLE_LINK,
  BOBGUI_ACCESSIBLE_ROLE_LIST,
  BOBGUI_ACCESSIBLE_ROLE_LIST_BOX,
  BOBGUI_ACCESSIBLE_ROLE_LIST_ITEM,
  BOBGUI_ACCESSIBLE_ROLE_LOG,
  BOBGUI_ACCESSIBLE_ROLE_MAIN,
  BOBGUI_ACCESSIBLE_ROLE_MARQUEE,
  BOBGUI_ACCESSIBLE_ROLE_MATH,
  BOBGUI_ACCESSIBLE_ROLE_METER,
  BOBGUI_ACCESSIBLE_ROLE_MENU,
  BOBGUI_ACCESSIBLE_ROLE_MENU_BAR,
  BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM,
  BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_CHECKBOX,
  BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_RADIO,
  BOBGUI_ACCESSIBLE_ROLE_NAVIGATION,
  BOBGUI_ACCESSIBLE_ROLE_NONE,
  BOBGUI_ACCESSIBLE_ROLE_NOTE,
  BOBGUI_ACCESSIBLE_ROLE_OPTION,
  BOBGUI_ACCESSIBLE_ROLE_PRESENTATION,
  BOBGUI_ACCESSIBLE_ROLE_PROGRESS_BAR,
  BOBGUI_ACCESSIBLE_ROLE_RADIO,
  BOBGUI_ACCESSIBLE_ROLE_RADIO_GROUP,
  BOBGUI_ACCESSIBLE_ROLE_RANGE,
  BOBGUI_ACCESSIBLE_ROLE_REGION,
  BOBGUI_ACCESSIBLE_ROLE_ROW,
  BOBGUI_ACCESSIBLE_ROLE_ROW_GROUP,
  BOBGUI_ACCESSIBLE_ROLE_ROW_HEADER,
  BOBGUI_ACCESSIBLE_ROLE_SCROLLBAR,
  BOBGUI_ACCESSIBLE_ROLE_SEARCH,
  BOBGUI_ACCESSIBLE_ROLE_SEARCH_BOX,
  BOBGUI_ACCESSIBLE_ROLE_SECTION,
  BOBGUI_ACCESSIBLE_ROLE_SECTION_HEAD,
  BOBGUI_ACCESSIBLE_ROLE_SELECT,
  BOBGUI_ACCESSIBLE_ROLE_SEPARATOR,
  BOBGUI_ACCESSIBLE_ROLE_SLIDER,
  BOBGUI_ACCESSIBLE_ROLE_SPIN_BUTTON,
  BOBGUI_ACCESSIBLE_ROLE_STATUS,
  BOBGUI_ACCESSIBLE_ROLE_STRUCTURE,
  BOBGUI_ACCESSIBLE_ROLE_SWITCH,
  BOBGUI_ACCESSIBLE_ROLE_TAB,
  BOBGUI_ACCESSIBLE_ROLE_TABLE,
  BOBGUI_ACCESSIBLE_ROLE_TAB_LIST,
  BOBGUI_ACCESSIBLE_ROLE_TAB_PANEL,
  BOBGUI_ACCESSIBLE_ROLE_TEXT_BOX,
  BOBGUI_ACCESSIBLE_ROLE_TIME,
  BOBGUI_ACCESSIBLE_ROLE_TIMER,
  BOBGUI_ACCESSIBLE_ROLE_TOOLBAR,
  BOBGUI_ACCESSIBLE_ROLE_TOOLTIP,
  BOBGUI_ACCESSIBLE_ROLE_TREE,
  BOBGUI_ACCESSIBLE_ROLE_TREE_GRID,
  BOBGUI_ACCESSIBLE_ROLE_TREE_ITEM,
  BOBGUI_ACCESSIBLE_ROLE_WIDGET,
  BOBGUI_ACCESSIBLE_ROLE_WINDOW,
  BOBGUI_ACCESSIBLE_ROLE_TOGGLE_BUTTON GDK_AVAILABLE_ENUMERATOR_IN_4_10,
  BOBGUI_ACCESSIBLE_ROLE_APPLICATION GDK_AVAILABLE_ENUMERATOR_IN_4_12,
  BOBGUI_ACCESSIBLE_ROLE_PARAGRAPH GDK_AVAILABLE_ENUMERATOR_IN_4_14,
  BOBGUI_ACCESSIBLE_ROLE_BLOCK_QUOTE GDK_AVAILABLE_ENUMERATOR_IN_4_14,
  BOBGUI_ACCESSIBLE_ROLE_ARTICLE GDK_AVAILABLE_ENUMERATOR_IN_4_14,
  BOBGUI_ACCESSIBLE_ROLE_COMMENT GDK_AVAILABLE_ENUMERATOR_IN_4_14,
  BOBGUI_ACCESSIBLE_ROLE_TERMINAL GDK_AVAILABLE_ENUMERATOR_IN_4_14
} BobguiAccessibleRole;

/**
 * BobguiAccessibleState:
 * @BOBGUI_ACCESSIBLE_STATE_BUSY: A “busy” state. This state has boolean values
 * @BOBGUI_ACCESSIBLE_STATE_CHECKED: A “checked” state; indicates the current
 *   state of a [class@CheckButton]. Value type: [enum@AccessibleTristate]
 * @BOBGUI_ACCESSIBLE_STATE_DISABLED: A “disabled” state; corresponds to the
 *   [property@Widget:sensitive] property. It indicates a UI element
 *   that is perceivable, but not editable or operable. Value type: boolean
 * @BOBGUI_ACCESSIBLE_STATE_EXPANDED: An “expanded” state; corresponds to the
 *   [property@Expander:expanded] property. Value type: boolean
 *   or undefined
 * @BOBGUI_ACCESSIBLE_STATE_HIDDEN: A “hidden” state; corresponds to the
 *   [property@Widget:visible] property. You can use this state
 *   explicitly on UI elements that should not be exposed to an assistive
 *   technology. Value type: boolean
 *   See also: %BOBGUI_ACCESSIBLE_STATE_DISABLED
 * @BOBGUI_ACCESSIBLE_STATE_INVALID: An “invalid” state; set when a widget
 *   is showing an error. Value type: [enum@AccessibleInvalidState]
 * @BOBGUI_ACCESSIBLE_STATE_PRESSED: A “pressed” state; indicates the current
 *   state of a [class@ToggleButton]. Value type: [enum@AccessibleTristate]
 *   enumeration
 * @BOBGUI_ACCESSIBLE_STATE_SELECTED: A “selected” state; set when a widget
 *   is selected. Value type: boolean or undefined
 *
 * The possible accessible states of a [iface@Accessible].
 */
/**
  * BOBGUI_ACCESSIBLE_STATE_VISITED:
  *
  * Indicates that a widget with the BOBGUI_ACCESSIBLE_ROLE_LINK has been visited.
  * Value type: boolean.
  *
  * Since: 4.12
  */
typedef enum {
  BOBGUI_ACCESSIBLE_STATE_BUSY,
  BOBGUI_ACCESSIBLE_STATE_CHECKED,
  BOBGUI_ACCESSIBLE_STATE_DISABLED,
  BOBGUI_ACCESSIBLE_STATE_EXPANDED,
  BOBGUI_ACCESSIBLE_STATE_HIDDEN,
  BOBGUI_ACCESSIBLE_STATE_INVALID,
  BOBGUI_ACCESSIBLE_STATE_PRESSED,
  BOBGUI_ACCESSIBLE_STATE_SELECTED,
  BOBGUI_ACCESSIBLE_STATE_VISITED GDK_AVAILABLE_ENUMERATOR_IN_4_12
} BobguiAccessibleState;

/**
 * BOBGUI_ACCESSIBLE_VALUE_UNDEFINED:
 *
 * An undefined value. The accessible attribute is either unset, or its
 * value is undefined.
 */
#define BOBGUI_ACCESSIBLE_VALUE_UNDEFINED  (-1)

/**
 * BobguiAccessibleProperty:
 * @BOBGUI_ACCESSIBLE_PROPERTY_AUTOCOMPLETE: Indicates whether inputting text
 *    could trigger display of one or more predictions of the user's intended
 *    value for a combobox, searchbox, or textbox and specifies how predictions
 *    would be presented if they were made. Value type: [enum@AccessibleAutocomplete]
 * @BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION: Defines a string value that describes
 *    or annotates the current element. Value type: string
 * @BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP: Indicates the availability of interactive
 *    popup element, such as menu or popover, that can be triggered by an
 *    element. Contrary to “aria-haspopup”, it doesn't indicate the type of the
 *    element, as such it cannot be used to indicate the availability of more
 *    complex elements such as dialog. Value type: boolean
 * @BOBGUI_ACCESSIBLE_PROPERTY_KEY_SHORTCUTS: Indicates keyboard shortcuts that an
 *    author has implemented to activate or give focus to an element. Value type:
 *    string. The format of the value is a space-separated list of shortcuts, with
 *    each shortcut consisting of one or more modifiers (`Control`, `Alt` or `Shift`),
 *    followed by a non-modifier key, all separated by `+`. The
 *    [WAI-ARIA](https://www.w3.org/TR/wai-aria/#aria-keyshortcuts) reference
 *    specifies how to build keyboard shortcuts strings, with specific values
 *    for each key which are the same regardless of the language, so these
 *    strings can't be built from localized key names. You can convert an
 *    accelerator into the matching key shortcuts label with
 *    [func@Bobgui.accelerator_get_accessible_label].
 *    Examples: `F2`, `Alt+F`, `Control+Shift+N`
 * @BOBGUI_ACCESSIBLE_PROPERTY_LABEL: Defines a string value that labels the current
 *    element. Value type: string
 * @BOBGUI_ACCESSIBLE_PROPERTY_LEVEL: Defines the hierarchical level of an element
 *    within a structure. Value type: integer
 * @BOBGUI_ACCESSIBLE_PROPERTY_MODAL: Indicates whether an element is modal when
 *    displayed. Value type: boolean
 * @BOBGUI_ACCESSIBLE_PROPERTY_MULTI_LINE: Indicates whether a text box accepts
 *    multiple lines of input or only a single line. Value type: boolean
 * @BOBGUI_ACCESSIBLE_PROPERTY_MULTI_SELECTABLE: Indicates that the user may select
 *    more than one item from the current selectable descendants. Value type:
 *    boolean
 * @BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION: Indicates whether the element's
 *    orientation is horizontal, vertical, or unknown/ambiguous. Value type:
 *    [enum@Orientation]
 * @BOBGUI_ACCESSIBLE_PROPERTY_PLACEHOLDER: Defines a short hint (a word or short
 *    phrase) intended to aid the user with data entry when the control has no
 *    value. A hint could be a sample value or a brief description of the expected
 *    format. Value type: string
 * @BOBGUI_ACCESSIBLE_PROPERTY_READ_ONLY: Indicates that the element is not editable,
 *    but is otherwise operable. Value type: boolean
 * @BOBGUI_ACCESSIBLE_PROPERTY_REQUIRED: Indicates that user input is required on
 *    the element before a form may be submitted. Value type: boolean
 * @BOBGUI_ACCESSIBLE_PROPERTY_ROLE_DESCRIPTION: Defines a human-readable,
 *    author-localized description for the role of an element. Value type: string
 * @BOBGUI_ACCESSIBLE_PROPERTY_SORT: Indicates if items in a table or grid are
 *    sorted in ascending or descending order. Value type: [enum@AccessibleSort]
 * @BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX: Defines the maximum allowed value for a
 *    range widget. Value type: double
 * @BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN: Defines the minimum allowed value for a
 *    range widget. Value type: double
 * @BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW: Defines the current value for a range widget.
 *    Value type: double
 * @BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT: Defines the human readable text alternative
 *    of [enum@Bobgui.AccessibleProperty.VALUE_NOW] for a range widget. Value type: string
 *
 * The possible accessible properties of a [iface@Accessible].
 */

/**
 * BOBGUI_ACCESSIBLE_PROPERTY_HELP_TEXT:
 *
 * Defines a string value that provides a description of non-standard keyboard
 * interactions of the current element. Value type: string
 *
 * Since: 4.16
 */
typedef enum {
  BOBGUI_ACCESSIBLE_PROPERTY_AUTOCOMPLETE,
  BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION,
  BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP,
  BOBGUI_ACCESSIBLE_PROPERTY_KEY_SHORTCUTS,
  BOBGUI_ACCESSIBLE_PROPERTY_LABEL,
  BOBGUI_ACCESSIBLE_PROPERTY_LEVEL,
  BOBGUI_ACCESSIBLE_PROPERTY_MODAL,
  BOBGUI_ACCESSIBLE_PROPERTY_MULTI_LINE,
  BOBGUI_ACCESSIBLE_PROPERTY_MULTI_SELECTABLE,
  BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION,
  BOBGUI_ACCESSIBLE_PROPERTY_PLACEHOLDER,
  BOBGUI_ACCESSIBLE_PROPERTY_READ_ONLY,
  BOBGUI_ACCESSIBLE_PROPERTY_REQUIRED,
  BOBGUI_ACCESSIBLE_PROPERTY_ROLE_DESCRIPTION,
  BOBGUI_ACCESSIBLE_PROPERTY_SORT,
  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX,
  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN,
  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW,
  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT,
  BOBGUI_ACCESSIBLE_PROPERTY_HELP_TEXT
} BobguiAccessibleProperty;

/**
 * BobguiAccessibleRelation:
 * @BOBGUI_ACCESSIBLE_RELATION_ACTIVE_DESCENDANT: Identifies the currently active
 *    element when focus is on a composite widget, combobox, textbox, group,
 *    or application. Value type: reference
 * @BOBGUI_ACCESSIBLE_RELATION_COL_COUNT: Defines the total number of columns
 *    in a table, grid, or treegrid. Value type: integer
 * @BOBGUI_ACCESSIBLE_RELATION_COL_INDEX: Defines an element's column index or
 *    position with respect to the total number of columns within a table,
 *    grid, or treegrid. Value type: integer
 * @BOBGUI_ACCESSIBLE_RELATION_COL_INDEX_TEXT: Defines a human readable text
 *   alternative of %BOBGUI_ACCESSIBLE_RELATION_COL_INDEX. Value type: string
 * @BOBGUI_ACCESSIBLE_RELATION_COL_SPAN: Defines the number of columns spanned
 *   by a cell or gridcell within a table, grid, or treegrid. Value type: integer
 * @BOBGUI_ACCESSIBLE_RELATION_CONTROLS: Identifies the element (or elements) whose
 *    contents or presence are controlled by the current element. Value type: reference
 * @BOBGUI_ACCESSIBLE_RELATION_DESCRIBED_BY: Identifies the element (or elements)
 *    that describes the object. Value type: reference
 * @BOBGUI_ACCESSIBLE_RELATION_DETAILS: Identifies the element (or elements) that
 *    provide additional information related to the object. Value type: reference
 * @BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE: Identifies the element (or elements) that
 *    provide an error message for an object. Value type: reference
 * @BOBGUI_ACCESSIBLE_RELATION_FLOW_TO: Identifies the next element (or elements)
 *    in an alternate reading order of content which, at the user's discretion,
 *    allows assistive technology to override the general default of reading in
 *    document source order. Value type: reference
 * @BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY: Identifies the element (or elements)
 *    that labels the current element. Value type: reference
 * @BOBGUI_ACCESSIBLE_RELATION_OWNS: Identifies an element (or elements) in order
 *    to define a visual, functional, or contextual parent/child relationship
 *    between elements where the widget hierarchy cannot be used to represent
 *    the relationship. Value type: reference
 * @BOBGUI_ACCESSIBLE_RELATION_POS_IN_SET: Defines an element's number or position
 *    in the current set of listitems or treeitems. Value type: integer
 * @BOBGUI_ACCESSIBLE_RELATION_ROW_COUNT: Defines the total number of rows in a table,
 *    grid, or treegrid. Value type: integer
 * @BOBGUI_ACCESSIBLE_RELATION_ROW_INDEX: Defines an element's row index or position
 *    with respect to the total number of rows within a table, grid, or treegrid.
 *    Value type: integer
 * @BOBGUI_ACCESSIBLE_RELATION_ROW_INDEX_TEXT: Defines a human readable text
 *    alternative of [enum@Bobgui.AccessibleRelation.ROW_INDEX]. Value type: string
 * @BOBGUI_ACCESSIBLE_RELATION_ROW_SPAN: Defines the number of rows spanned by a
 *    cell or gridcell within a table, grid, or treegrid. Value type: integer
 * @BOBGUI_ACCESSIBLE_RELATION_SET_SIZE: Defines the number of items in the current
 *    set of listitems or treeitems. Value type: integer
 *
 * The possible accessible relations of a [iface@Accessible].
 *
 * Accessible relations can be references to other widgets,
 * integers or strings.
 */

/**
 * BOBGUI_ACCESSIBLE_RELATION_LABEL_FOR:
 *
 * Identifies the element (or elements) that are labeled by the
 * current element. Value type: reference
 *
 * This relation is managed by BOBGUI and should not be set from application code.
 *
 * Since: 4.18
 */
/**
 * BOBGUI_ACCESSIBLE_RELATION_DESCRIPTION_FOR:
 *
 * Identifies the element (or elements) that are described by
 * the current element. Value type: reference
 *
 * This relation is managed by BOBGUI and should not be set from application code.
 *
 * Since: 4.18
 */
/**
 * BOBGUI_ACCESSIBLE_RELATION_CONTROLLED_BY:
 *
 * Identifies the element (or elements) that the current
 * element is controlled by. Value type: reference
 *
 * This relation is managed by BOBGUI and should not be set from application code.
 *
 * Since: 4.18
 */
/**
 * BOBGUI_ACCESSIBLE_RELATION_DETAILS_FOR:
 *
 * Identifies the element (or elements) for which the current
 * element provides additional information. Value type: reference
 *
 * This relation is managed by BOBGUI and should not be set from application code.
 *
 * Since: 4.18
 */
/**
 * BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE_FOR:
 *
 * Identifies the element (or elements) for which the current
 * element provides an error message. Value type: reference
 *
 * This relation is managed by BOBGUI and should not be set from application code.
 *
 * Since: 4.18
 */
/**
 * BOBGUI_ACCESSIBLE_RELATION_FLOW_FROM:
 *
 * Identifies the previous element (or elements) in an alternate
 * reading order of content which, at the user's discretion, allows
 * assistive technology to override the general default of reading in
 * document source order. Value type: reference
 *
 * This relation is managed by BOBGUI and should not be set from application code.
 *
 * Since: 4.18
 */
typedef enum {
  BOBGUI_ACCESSIBLE_RELATION_ACTIVE_DESCENDANT,
  BOBGUI_ACCESSIBLE_RELATION_COL_COUNT,
  BOBGUI_ACCESSIBLE_RELATION_COL_INDEX,
  BOBGUI_ACCESSIBLE_RELATION_COL_INDEX_TEXT,
  BOBGUI_ACCESSIBLE_RELATION_COL_SPAN,
  BOBGUI_ACCESSIBLE_RELATION_CONTROLS,
  BOBGUI_ACCESSIBLE_RELATION_DESCRIBED_BY,
  BOBGUI_ACCESSIBLE_RELATION_DETAILS,
  BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE,
  BOBGUI_ACCESSIBLE_RELATION_FLOW_TO,
  BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY,
  BOBGUI_ACCESSIBLE_RELATION_OWNS,
  BOBGUI_ACCESSIBLE_RELATION_POS_IN_SET,
  BOBGUI_ACCESSIBLE_RELATION_ROW_COUNT,
  BOBGUI_ACCESSIBLE_RELATION_ROW_INDEX,
  BOBGUI_ACCESSIBLE_RELATION_ROW_INDEX_TEXT,
  BOBGUI_ACCESSIBLE_RELATION_ROW_SPAN,
  BOBGUI_ACCESSIBLE_RELATION_SET_SIZE,
  BOBGUI_ACCESSIBLE_RELATION_LABEL_FOR,
  BOBGUI_ACCESSIBLE_RELATION_DESCRIPTION_FOR,
  BOBGUI_ACCESSIBLE_RELATION_CONTROLLED_BY,
  BOBGUI_ACCESSIBLE_RELATION_DETAILS_FOR,
  BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE_FOR,
  BOBGUI_ACCESSIBLE_RELATION_FLOW_FROM
} BobguiAccessibleRelation;

/**
 * BobguiAccessibleTristate:
 * @BOBGUI_ACCESSIBLE_TRISTATE_FALSE: The state is `false`
 * @BOBGUI_ACCESSIBLE_TRISTATE_TRUE: The state is `true`
 * @BOBGUI_ACCESSIBLE_TRISTATE_MIXED: The state is `mixed`
 *
 * The possible values for the %BOBGUI_ACCESSIBLE_STATE_PRESSED
 * accessible state.
 *
 * Note that the %BOBGUI_ACCESSIBLE_TRISTATE_FALSE and
 * %BOBGUI_ACCESSIBLE_TRISTATE_TRUE have the same values
 * as %FALSE and %TRUE.
 */
typedef enum {
  BOBGUI_ACCESSIBLE_TRISTATE_FALSE,
  BOBGUI_ACCESSIBLE_TRISTATE_TRUE,
  BOBGUI_ACCESSIBLE_TRISTATE_MIXED
} BobguiAccessibleTristate;

/**
 * BobguiAccessibleInvalidState:
 * @BOBGUI_ACCESSIBLE_INVALID_FALSE: There are no detected errors in the value
 * @BOBGUI_ACCESSIBLE_INVALID_TRUE: The value entered by the user has failed validation
 * @BOBGUI_ACCESSIBLE_INVALID_GRAMMAR: A grammatical error was detected
 * @BOBGUI_ACCESSIBLE_INVALID_SPELLING: A spelling error was detected
 *
 * The possible values for the %BOBGUI_ACCESSIBLE_STATE_INVALID
 * accessible state.
 *
 * Note that the %BOBGUI_ACCESSIBLE_INVALID_FALSE and
 * %BOBGUI_ACCESSIBLE_INVALID_TRUE have the same values
 * as %FALSE and %TRUE.
 */
typedef enum { /*< prefix=BOBGUI_ACCESSIBLE_INVALID >*/
  BOBGUI_ACCESSIBLE_INVALID_FALSE,
  BOBGUI_ACCESSIBLE_INVALID_TRUE,
  BOBGUI_ACCESSIBLE_INVALID_GRAMMAR,
  BOBGUI_ACCESSIBLE_INVALID_SPELLING,
} BobguiAccessibleInvalidState;

/**
 * BobguiAccessibleAutocomplete:
 * @BOBGUI_ACCESSIBLE_AUTOCOMPLETE_NONE: Automatic suggestions are not displayed.
 * @BOBGUI_ACCESSIBLE_AUTOCOMPLETE_INLINE: When a user is providing input, text
 *    suggesting one way to complete the provided input may be dynamically
 *    inserted after the caret.
 * @BOBGUI_ACCESSIBLE_AUTOCOMPLETE_LIST: When a user is providing input, an element
 *    containing a collection of values that could complete the provided input
 *    may be displayed.
 * @BOBGUI_ACCESSIBLE_AUTOCOMPLETE_BOTH: When a user is providing input, an element
 *    containing a collection of values that could complete the provided input
 *    may be displayed. If displayed, one value in the collection is automatically
 *    selected, and the text needed to complete the automatically selected value
 *    appears after the caret in the input.
 *
 * The possible values for the %BOBGUI_ACCESSIBLE_PROPERTY_AUTOCOMPLETE
 * accessible property.
 */
typedef enum { /*< prefix=BOBGUI_ACCESSIBLE_AUTOCOMPLETE >*/
  BOBGUI_ACCESSIBLE_AUTOCOMPLETE_NONE,
  BOBGUI_ACCESSIBLE_AUTOCOMPLETE_INLINE,
  BOBGUI_ACCESSIBLE_AUTOCOMPLETE_LIST,
  BOBGUI_ACCESSIBLE_AUTOCOMPLETE_BOTH
} BobguiAccessibleAutocomplete;

/**
 * BobguiAccessibleSort:
 * @BOBGUI_ACCESSIBLE_SORT_NONE: There is no defined sort applied to the column.
 * @BOBGUI_ACCESSIBLE_SORT_ASCENDING: Items are sorted in ascending order by this column.
 * @BOBGUI_ACCESSIBLE_SORT_DESCENDING: Items are sorted in descending order by this column.
 * @BOBGUI_ACCESSIBLE_SORT_OTHER: A sort algorithm other than ascending or
 *    descending has been applied.
 *
 * The possible values for the %BOBGUI_ACCESSIBLE_PROPERTY_SORT
 * accessible property.
 */
typedef enum { /*< prefix=BOBGUI_ACCESSIBLE_SORT >*/
  BOBGUI_ACCESSIBLE_SORT_NONE,
  BOBGUI_ACCESSIBLE_SORT_ASCENDING,
  BOBGUI_ACCESSIBLE_SORT_DESCENDING,
  BOBGUI_ACCESSIBLE_SORT_OTHER
} BobguiAccessibleSort;

/**
 * BobguiAccessibleAnnouncementPriority:
 * @BOBGUI_ACCESSIBLE_ANNOUNCEMENT_PRIORITY_LOW: The announcement is low priority,
 *   and might be read only on the user's request.
 * @BOBGUI_ACCESSIBLE_ANNOUNCEMENT_PRIORITY_MEDIUM: The announcement is of medium
 *   priority, and is usually spoken at the next opportunity, such as at the
 *   end of speaking the current sentence or when the user pauses typing.
 * @BOBGUI_ACCESSIBLE_ANNOUNCEMENT_PRIORITY_HIGH: The announcement is of high
 *   priority, and is usually spoken immediately. Because an interruption
 *   might disorient users or cause them to not complete their current task,
 *   authors SHOULD NOT use high priority announcements unless the
 *   interruption is imperative. An example would be a notification about a
 *   critical battery power level.
 *
 * The priority of an accessibility announcement.
 *
 * Since: 4.14
 */
typedef enum {
  BOBGUI_ACCESSIBLE_ANNOUNCEMENT_PRIORITY_LOW,
  BOBGUI_ACCESSIBLE_ANNOUNCEMENT_PRIORITY_MEDIUM,
  BOBGUI_ACCESSIBLE_ANNOUNCEMENT_PRIORITY_HIGH
} BobguiAccessibleAnnouncementPriority;

/**
 * BobguiPopoverMenuFlags:
 * @BOBGUI_POPOVER_MENU_NESTED: Submenus are presented as traditional, nested
 *   popovers.
 *
 * Flags that affect how [class@Bobgui.PopoverMenu] widgets built from
 * a [class@Gio.MenuModel] are created and displayed.
 */
/**
 * BOBGUI_POPOVER_MENU_SLIDING:
 *
 * Submenus are presented as sliding submenus that replace the main menu.
 *
 * Since: 4.14
 */
typedef enum { /*< prefix=BOBGUI_POPOVER_MENU >*/
  BOBGUI_POPOVER_MENU_SLIDING = 0,
  BOBGUI_POPOVER_MENU_NESTED = 1 << 0
} BobguiPopoverMenuFlags;

/**
 * BobguiFontRendering:
 * @BOBGUI_FONT_RENDERING_AUTOMATIC: Set up font rendering automatically,
 *   taking factors like screen resolution and scale into account
 * @BOBGUI_FONT_RENDERING_MANUAL: Follow low-level font-related settings
 *   when configuring font rendering
 *
 * Values for the [property@Bobgui.Settings:bobgui-font-rendering] setting
 * that influence how BOBGUI renders fonts.
 *
 * Since: 4.16
 */
typedef enum {
  BOBGUI_FONT_RENDERING_AUTOMATIC,
  BOBGUI_FONT_RENDERING_MANUAL,
} BobguiFontRendering;

/**
 * BobguiTextBufferNotifyFlags:
 * @BOBGUI_TEXT_BUFFER_NOTIFY_BEFORE_INSERT: Be notified before text
 *   is inserted into the underlying buffer.
 * @BOBGUI_TEXT_BUFFER_NOTIFY_AFTER_INSERT: Be notified after text
 *   has been inserted into the underlying buffer.
 * @BOBGUI_TEXT_BUFFER_NOTIFY_BEFORE_DELETE: Be notified before text
 *   is deleted from the underlying buffer.
 * @BOBGUI_TEXT_BUFFER_NOTIFY_AFTER_DELETE: Be notified after text
 *   has been deleted from the underlying buffer.
 *
 * Values for [callback@Bobgui.TextBufferCommitNotify] to denote the
 * point of the notification.
 *
 * Since: 4.16
 */
typedef enum {
  BOBGUI_TEXT_BUFFER_NOTIFY_BEFORE_INSERT = 1 << 0,
  BOBGUI_TEXT_BUFFER_NOTIFY_AFTER_INSERT  = 1 << 1,
  BOBGUI_TEXT_BUFFER_NOTIFY_BEFORE_DELETE = 1 << 2,
  BOBGUI_TEXT_BUFFER_NOTIFY_AFTER_DELETE  = 1 << 3,
} BobguiTextBufferNotifyFlags;

/**
 * BobguiInterfaceColorScheme:
 * @BOBGUI_INTERFACE_COLOR_SCHEME_UNSUPPORTED: The system doesn't support color schemes
 * @BOBGUI_INTERFACE_COLOR_SCHEME_DEFAULT: The default color scheme is used
 * @BOBGUI_INTERFACE_COLOR_SCHEME_DARK: A dark color scheme is used
 * @BOBGUI_INTERFACE_COLOR_SCHEME_LIGHT: A light color scheme is used
 *
 * Values for the [property@Bobgui.Settings:bobgui-interface-color-scheme]
 * and [property@Bobgui.CssProvider:prefers-color-scheme] properties
 * that indicates what color scheme is used.
 *
 * This information can be used inside CSS via media queries.
 *
 * More values may be added to this enumeration. Unknown values
 * should be treated the same as `BOBGUI_INTERFACE_COLOR_SCHEME_DEFAULT`.
 *
 * Since: 4.20
 */
typedef enum {
  BOBGUI_INTERFACE_COLOR_SCHEME_UNSUPPORTED,
  BOBGUI_INTERFACE_COLOR_SCHEME_DEFAULT,
  BOBGUI_INTERFACE_COLOR_SCHEME_DARK,
  BOBGUI_INTERFACE_COLOR_SCHEME_LIGHT,
} BobguiInterfaceColorScheme;

/**
 * BobguiInterfaceContrast
 * @BOBGUI_INTERFACE_CONTRAST_UNSUPPORTED: The system doesn't support contrast levels
 * @BOBGUI_INTERFACE_CONTRAST_NO_PREFERENCE: No particular preference for contrast
 * @BOBGUI_INTERFACE_CONTRAST_MORE: More contrast is preferred
 * @BOBGUI_INTERFACE_CONTRAST_LESS: Less contrast is preferred
 *
 * Values for the [property@Bobgui.Settings:bobgui-interface-contrast]
 * and [property@Bobgui.CssProvider:prefers-contrast] properties
 * that indicates the preferred level of contrast.
 *
 * This information can be used inside CSS via media queries.
 *
 * More values may be added to this enumeration. Unknown values
 * should be treated the same as `BOBGUI_INTERFACE_CONTRAST_NO_PREFERENCE`.
 *
 * Since: 4.20
 */
typedef enum {
  BOBGUI_INTERFACE_CONTRAST_UNSUPPORTED,
  BOBGUI_INTERFACE_CONTRAST_NO_PREFERENCE,
  BOBGUI_INTERFACE_CONTRAST_MORE,
  BOBGUI_INTERFACE_CONTRAST_LESS,
} BobguiInterfaceContrast;

/**
 * BobguiRestoreReason:
 * @BOBGUI_RESTORE_REASON_PRISTINE: Don't restore anything
 * @BOBGUI_RESTORE_REASON_LAUNCH: This is normal launch. Restore as little as is reasonable
 * @BOBGUI_RESTORE_REASON_RECOVER: The application has crashed before. Try to restore the previous state
 * @BOBGUI_RESTORE_REASON_RESTORE: This is a session restore. Restore the previous state as far as possible
 *
 * Enumerates possible reasons for an application to restore saved state.
 *
 * See [signal@Bobgui.Application::restore-state].
 *
 * Since: 4.24
 */
typedef enum
{
  BOBGUI_RESTORE_REASON_PRISTINE,
  BOBGUI_RESTORE_REASON_LAUNCH,
  BOBGUI_RESTORE_REASON_RECOVER,
  BOBGUI_RESTORE_REASON_RESTORE,
} BobguiRestoreReason;

/**
 * BobguiReducedMotion:
 * @BOBGUI_REDUCED_MOTION_NO_PREFERENCE: The user has made no preference known to the system
 * @BOBGUI_REDUCED_MOTION_REDUCE: The user has notified the system that they
 *   prefer an interface that removes or replaces the types of motion-based
 *   animation that either trigger discomfort for those with vestibular
 *   motion sensitivity, or distraction for those with attention deficits
 *
 * Values for the [property@Bobgui.Settings:bobgui-interface-reduced-motion]
 * and [property@Bobgui.CssProvider:prefers-reduced-motion] properties
 * that indicates the preferred level of motion animations.
 *
 * This information can be used inside CSS via media queries.
 *
 * Since: 4.22
 */
typedef enum {
  BOBGUI_REDUCED_MOTION_NO_PREFERENCE,
  BOBGUI_REDUCED_MOTION_REDUCE,
} BobguiReducedMotion;

G_END_DECLS
