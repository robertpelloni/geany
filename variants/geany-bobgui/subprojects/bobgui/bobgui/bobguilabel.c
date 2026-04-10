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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.Free
 */

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#include "config.h"

#include "bobguilabelprivate.h"

#include "bobguiaccessibletextprivate.h"
#include "bobguiaccessiblehypertextprivate.h"
#include "bobguibuildable.h"
#include "bobguibuilderprivate.h"
#include "bobguicsscolorvalueprivate.h"
#include "bobguidragsourceprivate.h"
#include "bobguidragicon.h"
#include "bobguieventcontrollermotion.h"
#include "bobguieventcontrollerfocus.h"
#include "bobguifilelauncher.h"
#include "bobguigesturedrag.h"
#include "bobguigestureclick.h"
#include "bobguigesturesingle.h"
#include "bobguijoinedmenuprivate.h"
#include "bobguimarshalers.h"
#include "bobguinative.h"
#include "bobguinotebook.h"
#include "bobguipangoprivate.h"
#include "bobguipopovermenu.h"
#include "bobguiprivate.h"
#include "bobguirenderbackgroundprivate.h"
#include "bobguirenderborderprivate.h"
#include "bobguirenderlayoutprivate.h"
#include "bobguishortcut.h"
#include "bobguishortcutcontroller.h"
#include "bobguishortcuttrigger.h"
#include "bobguisnapshot.h"
#include "bobguitextutilprivate.h"
#include "bobguitooltip.h"
#include "bobguitypebuiltins.h"
#include "bobguiurilauncher.h"
#include "bobguiwidgetprivate.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gi18n-lib.h>

/**
 * BobguiLabel:
 *
 * Displays a small amount of text.
 *
 * Most labels are used to label another widget (such as an [class@Entry]).
 *
 * <picture>
 *   <source srcset="label-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiLabel" src="label.png">
 * </picture>
 *
 * ## Shortcuts and Gestures
 *
 * `BobguiLabel` supports the following keyboard shortcuts, when the cursor is
 * visible:
 *
 * - <kbd>Shift</kbd>+<kbd>F10</kbd> or <kbd>Menu</kbd> opens the context menu.
 * - <kbd>Ctrl</kbd>+<kbd>A</kbd> or <kbd>Ctrl</kbd>+<kbd>&sol;</kbd>
 *   selects all.
 * - <kbd>Ctrl</kbd>+<kbd>Shift</kbd>+<kbd>A</kbd> or
 *   <kbd>Ctrl</kbd>+<kbd>&bsol;</kbd> unselects all.
 *
 * Additionally, the following signals have default keybindings:
 *
 * - [signal@Bobgui.Label::activate-current-link]
 * - [signal@Bobgui.Label::copy-clipboard]
 * - [signal@Bobgui.Label::move-cursor]
 *
 * ## Actions
 *
 * `BobguiLabel` defines a set of built-in actions:
 *
 * - `clipboard.copy` copies the text to the clipboard.
 * - `clipboard.cut` doesn't do anything, since text in labels can't be deleted.
 * - `clipboard.paste` doesn't do anything, since text in labels can't be
 *   edited.
 * - `link.open` opens the link, when activated on a link inside the label.
 * - `link.copy` copies the link to the clipboard, when activated on a link
 *   inside the label.
 * - `menu.popup` opens the context menu.
 * - `selection.delete` doesn't do anything, since text in labels can't be
 *   deleted.
 * - `selection.select-all` selects all of the text, if the label allows
 *   selection.
 *
 * ## CSS nodes
 *
 * ```
 * label
 * ├── [selection]
 * ├── [link]
 * ┊
 * ╰── [link]
 * ```
 *
 * `BobguiLabel` has a single CSS node with the name label. A wide variety
 * of style classes may be applied to labels, such as .title, .subtitle,
 * .dim-label, etc. In the `BobguiShortcutsWindow`, labels are used with the
 * .keycap style class.
 *
 * If the label has a selection, it gets a subnode with name selection.
 *
 * If the label has links, there is one subnode per link. These subnodes
 * carry the link or visited state depending on whether they have been
 * visited. In this case, label node also gets a .link style class.
 *
 * ## BobguiLabel as BobguiBuildable
 *
 * The BobguiLabel implementation of the BobguiBuildable interface supports a
 * custom `<attributes>` element, which supports any number of `<attribute>`
 * elements. The `<attribute>` element has attributes named “name“, “value“,
 * “start“ and “end“ and allows you to specify [struct@Pango.Attribute]
 * values for this label.
 *
 * An example of a UI definition fragment specifying Pango attributes:
 *
 * ```xml
 * <object class="BobguiLabel">
 *   <attributes>
 *     <attribute name="weight" value="PANGO_WEIGHT_BOLD"/>
 *     <attribute name="background" value="red" start="5" end="10"/>
 *   </attributes>
 * </object>
 * ```
 *
 * The start and end attributes specify the range of characters to which the
 * Pango attribute applies. If start and end are not specified, the attribute is
 * applied to the whole text. Note that specifying ranges does not make much
 * sense with translatable attributes. Use markup embedded in the translatable
 * content instead.
 *
 * ## Accessibility
 *
 * `BobguiLabel` uses the [enum@Bobgui.AccessibleRole.label] role.
 *
 * ## Mnemonics
 *
 * Labels may contain “mnemonics”. Mnemonics are underlined characters in the
 * label, used for keyboard navigation. Mnemonics are created by providing a
 * string with an underscore before the mnemonic character, such as `"_File"`,
 * to the functions [ctor@Bobgui.Label.new_with_mnemonic] or
 * [method@Bobgui.Label.set_text_with_mnemonic].
 *
 * Mnemonics automatically activate any activatable widget the label is
 * inside, such as a [class@Bobgui.Button]; if the label is not inside the
 * mnemonic’s target widget, you have to tell the label about the target
 * using [method@Bobgui.Label.set_mnemonic_widget].
 *
 * Here’s a simple example where the label is inside a button:
 *
 * ```c
 * // Pressing Alt+H will activate this button
 * BobguiWidget *button = bobgui_button_new ();
 * BobguiWidget *label = bobgui_label_new_with_mnemonic ("_Hello");
 * bobgui_button_set_child (BOBGUI_BUTTON (button), label);
 * ```
 *
 * There’s a convenience function to create buttons with a mnemonic label
 * already inside:
 *
 * ```c
 * // Pressing Alt+H will activate this button
 * BobguiWidget *button = bobgui_button_new_with_mnemonic ("_Hello");
 * ```
 *
 * To create a mnemonic for a widget alongside the label, such as a
 * [class@Bobgui.Entry], you have to point the label at the entry with
 * [method@Bobgui.Label.set_mnemonic_widget]:
 *
 * ```c
 * // Pressing Alt+H will focus the entry
 * BobguiWidget *entry = bobgui_entry_new ();
 * BobguiWidget *label = bobgui_label_new_with_mnemonic ("_Hello");
 * bobgui_label_set_mnemonic_widget (BOBGUI_LABEL (label), entry);
 * ```
 *
 * ## Markup (styled text)
 *
 * To make it easy to format text in a label (changing colors, fonts, etc.),
 * label text can be provided in a simple markup format:
 *
 * Here’s how to create a label with a small font:
 * ```c
 * BobguiWidget *label = bobgui_label_new (NULL);
 * bobgui_label_set_markup (BOBGUI_LABEL (label), "<small>Small text</small>");
 * ```
 *
 * (See the Pango manual for complete documentation] of available
 * tags, [func@Pango.parse_markup])
 *
 * The markup passed to [method@Bobgui.Label.set_markup] must be valid XML; for example,
 * literal `<`, `>` and `&` characters must be escaped as `&lt;`, `&gt;`, and `&amp;`.
 * If you pass text obtained from the user, file, or a network to
 * [method@Bobgui.Label.set_markup], you’ll want to escape it with
 * [func@GLib.markup_escape_text] or [func@GLib.markup_printf_escaped].
 *
 * Markup strings are just a convenient way to set the [struct@Pango.AttrList]
 * on a label; [method@Bobgui.Label.set_attributes] may be a simpler way to set
 * attributes in some cases. Be careful though; [struct@Pango.AttrList] tends
 * to cause internationalization problems, unless you’re applying attributes
 * to the entire string (i.e. unless you set the range of each attribute
 * to [0, `G_MAXINT`)). The reason is that specifying the `start_index` and
 * `end_index` for a [struct@Pango.Attribute] requires knowledge of the exact
 * string being displayed, so translations will cause problems.
 *
 * ## Selectable labels
 *
 * Labels can be made selectable with [method@Bobgui.Label.set_selectable].
 * Selectable labels allow the user to copy the label contents to the
 * clipboard. Only labels that contain useful-to-copy information — such
 * as error messages — should be made selectable.
 *
 * ## Text layout
 *
 * A label can contain any number of paragraphs, but will have
 * performance problems if it contains more than a small number.
 * Paragraphs are separated by newlines or other paragraph separators
 * understood by Pango.
 *
 * Labels can automatically wrap text if you call [method@Bobgui.Label.set_wrap].
 *
 * [method@Bobgui.Label.set_justify] sets how the lines in a label align
 * with one another. If you want to set how the label as a whole aligns
 * in its available space, see the [property@Bobgui.Widget:halign] and
 * [property@Bobgui.Widget:valign] properties.
 *
 * The [property@Bobgui.Label:width-chars] and [property@Bobgui.Label:max-width-chars]
 * properties can be used to control the size allocation of ellipsized or
 * wrapped labels. For ellipsizing labels, if either is specified (and less
 * than the actual text size), it is used as the minimum width, and the actual
 * text size is used as the natural width of the label. For wrapping labels,
 * width-chars is used as the minimum width, if specified, and max-width-chars
 * is used as the natural width. Even if max-width-chars specified, wrapping
 * labels will be rewrapped to use all of the available width.
 *
 * ## Links
 *
 * BOBGUI supports markup for clickable hyperlinks in addition to regular Pango
 * markup. The markup for links is borrowed from HTML, using the `<a>` tag
 * with “href“, “title“ and “class“ attributes. BOBGUI renders links similar to
 * the way they appear in web browsers, with colored, underlined text. The
 * “title“ attribute is displayed as a tooltip on the link. The “class“
 * attribute is used as style class on the CSS node for the link.
 *
 * An example of inline links looks like this:
 *
 * ```c
 * const char *text =
 * "Go to the "
 * "<a href=\"https://www.bobgui.org\" title=\"&lt;i&gt;Our&lt;/i&gt; website\">"
 * "BOBGUI website</a> for more...";
 * BobguiWidget *label = bobgui_label_new (NULL);
 * bobgui_label_set_markup (BOBGUI_LABEL (label), text);
 * ```
 *
 * It is possible to implement custom handling for links and their tooltips
 * with the [signal@Bobgui.Label::activate-link] signal and the
 * [method@Bobgui.Label.get_current_uri] function.
 */

typedef struct _BobguiLabelClass         BobguiLabelClass;
typedef struct _BobguiLabelSelectionInfo BobguiLabelSelectionInfo;

struct _BobguiLabel
{
  BobguiWidget parent_instance;

  BobguiLabelSelectionInfo *select_info;
  BobguiWidget *mnemonic_widget;
  BobguiEventController *mnemonic_controller;

  PangoAttrList *attrs;
  PangoAttrList *markup_attrs;
  PangoLayout   *layout;
  PangoTabArray *tabs;

  BobguiWidget *popup_menu;
  GMenuModel *extra_menu;

  char    *label;
  char    *text;

  float    xalign;
  float    yalign;

  guint    mnemonics_visible  : 1;
  guint    jtype              : 2;
  guint    wrap               : 1;
  guint    use_underline      : 1;
  guint    ellipsize          : 3;
  guint    use_markup         : 1;
  guint    wrap_mode          : 3;
  guint    natural_wrap_mode  : 3;
  guint    single_line_mode   : 1;
  guint    in_click           : 1;
  guint    track_links        : 1;

  guint    mnemonic_keyval;

  int      width_chars;
  int      max_width_chars;
  int      lines;
};

struct _BobguiLabelClass
{
  BobguiWidgetClass parent_class;

  void (* move_cursor)     (BobguiLabel       *self,
                            BobguiMovementStep step,
                            int             count,
                            gboolean        extend_selection);
  void (* copy_clipboard)  (BobguiLabel       *self);

  gboolean (*activate_link) (BobguiLabel       *self,
                             const char     *uri);
};

/* Notes about the handling of links:
 *
 * Links share the BobguiLabelSelectionInfo struct with selectable labels.
 * There are some new fields for links. The links field contains the list
 * of BobguiLabelLink structs that describe the links which are embedded in
 * the label. The active_link field points to the link under the mouse
 * pointer. For keyboard navigation, the “focus” link is determined by
 * finding the link which contains the selection_anchor position.
 * The link_clicked field is used with button press and release events
 * to ensure that pressing inside a link and releasing outside of it
 * does not activate the link.
 *
 * Links are rendered with the BOBGUI_STATE_FLAG_LINK/BOBGUI_STATE_FLAG_VISITED
 * state flags. When the mouse pointer is over a link, the pointer is changed
 * to indicate the link.
 *
 * Labels with links accept keyboard focus, and it is possible to move
 * the focus between the embedded links using Tab/Shift-Tab. The focus
 * is indicated by a focus rectangle that is drawn around the link text.
 * Pressing Enter activates the focused link, and there is a suitable
 * context menu for links that can be opened with the Menu key. Pressing
 * Control-C copies the link URI to the clipboard.
 *
 * In selectable labels with links, link functionality is only available
 * when the selection is empty.
 */
typedef struct
{
  char *uri;
  char *title;     /* the title attribute, used as tooltip */

  BobguiCssNode *cssnode;
  BobguiAccessibleHyperlink *accessible;

  gboolean visited; /* get set when the link is activated; this flag
                     * gets preserved over later set_markup() calls
                     */
  int start;       /* position of the link in the PangoLayout */
  int end;
} BobguiLabelLink;

struct _BobguiLabelSelectionInfo
{
  int selection_anchor;
  int selection_end;
  BobguiCssNode *selection_node;
  GdkContentProvider *provider;

  BobguiLabelLink *links;
  guint n_links;
  BobguiLabelLink *active_link;
  BobguiLabelLink *context_link;

  BobguiGesture *drag_gesture;
  BobguiGesture *click_gesture;
  BobguiEventController *motion_controller;
  BobguiEventController *focus_controller;

  int drag_start_x;
  int drag_start_y;

  guint in_drag      : 1;
  guint select_words : 1;
  guint selectable   : 1;
  guint link_clicked : 1;
};

enum {
  MOVE_CURSOR,
  COPY_CLIPBOARD,
  ACTIVATE_LINK,
  ACTIVATE_CURRENT_LINK,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_LABEL,
  PROP_ATTRIBUTES,
  PROP_USE_MARKUP,
  PROP_USE_UNDERLINE,
  PROP_JUSTIFY,
  PROP_WRAP,
  PROP_WRAP_MODE,
  PROP_NATURAL_WRAP_MODE,
  PROP_SELECTABLE,
  PROP_MNEMONIC_KEYVAL,
  PROP_MNEMONIC_WIDGET,
  PROP_ELLIPSIZE,
  PROP_WIDTH_CHARS,
  PROP_SINGLE_LINE_MODE,
  PROP_MAX_WIDTH_CHARS,
  PROP_LINES,
  PROP_XALIGN,
  PROP_YALIGN,
  PROP_EXTRA_MENU,
  PROP_TABS,
  NUM_PROPERTIES
};

static GParamSpec *label_props[NUM_PROPERTIES] = { NULL, };

static guint signals[LAST_SIGNAL] = { 0 };

static GQuark quark_mnemonics_visible_connected;

static void bobgui_label_set_markup_internal        (BobguiLabel      *self,
                                                  const char    *str,
                                                  gboolean       with_uline);
static void bobgui_label_recalculate                (BobguiLabel      *self);
static void bobgui_label_do_popup                   (BobguiLabel      *self,
                                                  double         x,
                                                  double         y);
static void bobgui_label_ensure_select_info  (BobguiLabel *self);
static void bobgui_label_clear_select_info   (BobguiLabel *self);
static void bobgui_label_clear_provider_info (BobguiLabel *self);
static void bobgui_label_clear_layout        (BobguiLabel *self);
static void bobgui_label_ensure_layout       (BobguiLabel *self);
static void bobgui_label_select_region_index (BobguiLabel *self,
                                           int       anchor_index,
                                           int       end_index);
static void bobgui_label_update_active_link  (BobguiWidget *widget,
                                           double     x,
                                           double     y);
static void     bobgui_label_setup_mnemonic    (BobguiLabel          *self);

/* For selectable labels: */
static void bobgui_label_move_cursor        (BobguiLabel        *self,
                                          BobguiMovementStep  step,
                                          int              count,
                                          gboolean         extend_selection);

static void     bobgui_label_buildable_interface_init   (BobguiBuildableIface  *iface);
static BobguiBuildableIface *buildable_parent_iface = NULL;

static void     bobgui_label_accessible_init (BobguiAccessibleInterface *iface);
static void     bobgui_label_accessible_text_init (BobguiAccessibleTextInterface *iface);
static void     bobgui_label_accessible_hypertext_init (BobguiAccessibleHypertextInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiLabel, bobgui_label, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_label_buildable_interface_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACCESSIBLE,
                                                bobgui_label_accessible_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACCESSIBLE_TEXT,
                                                bobgui_label_accessible_text_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACCESSIBLE_HYPERTEXT,
                                                bobgui_label_accessible_hypertext_init))

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
bobgui_label_set_property (GObject      *object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  BobguiLabel *self = BOBGUI_LABEL (object);

  switch (prop_id)
    {
    case PROP_LABEL:
      bobgui_label_set_label (self, g_value_get_string (value));
      break;
    case PROP_ATTRIBUTES:
      bobgui_label_set_attributes (self, g_value_get_boxed (value));
      break;
    case PROP_USE_MARKUP:
      bobgui_label_set_use_markup (self, g_value_get_boolean (value));
      break;
    case PROP_USE_UNDERLINE:
      bobgui_label_set_use_underline (self, g_value_get_boolean (value));
      break;
    case PROP_JUSTIFY:
      bobgui_label_set_justify (self, g_value_get_enum (value));
      break;
    case PROP_WRAP:
      bobgui_label_set_wrap (self, g_value_get_boolean (value));
      break;
    case PROP_WRAP_MODE:
      bobgui_label_set_wrap_mode (self, g_value_get_enum (value));
      break;
    case PROP_NATURAL_WRAP_MODE:
      bobgui_label_set_natural_wrap_mode (self, g_value_get_enum (value));
      break;
    case PROP_SELECTABLE:
      bobgui_label_set_selectable (self, g_value_get_boolean (value));
      break;
    case PROP_MNEMONIC_WIDGET:
      bobgui_label_set_mnemonic_widget (self, (BobguiWidget*) g_value_get_object (value));
      break;
    case PROP_ELLIPSIZE:
      bobgui_label_set_ellipsize (self, g_value_get_enum (value));
      break;
    case PROP_WIDTH_CHARS:
      bobgui_label_set_width_chars (self, g_value_get_int (value));
      break;
    case PROP_SINGLE_LINE_MODE:
      bobgui_label_set_single_line_mode (self, g_value_get_boolean (value));
      break;
    case PROP_MAX_WIDTH_CHARS:
      bobgui_label_set_max_width_chars (self, g_value_get_int (value));
      break;
    case PROP_LINES:
      bobgui_label_set_lines (self, g_value_get_int (value));
      break;
    case PROP_XALIGN:
      bobgui_label_set_xalign (self, g_value_get_float (value));
      break;
    case PROP_YALIGN:
      bobgui_label_set_yalign (self, g_value_get_float (value));
      break;
    case PROP_EXTRA_MENU:
      bobgui_label_set_extra_menu (self, g_value_get_object (value));
      break;
    case PROP_TABS:
      bobgui_label_set_tabs (self, g_value_get_boxed (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_label_get_property (GObject     *object,
                        guint        prop_id,
                        GValue      *value,
                        GParamSpec  *pspec)
{
  BobguiLabel *self = BOBGUI_LABEL (object);

  switch (prop_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, self->label);
      break;
    case PROP_ATTRIBUTES:
      g_value_set_boxed (value, self->attrs);
      break;
    case PROP_USE_MARKUP:
      g_value_set_boolean (value, self->use_markup);
      break;
    case PROP_USE_UNDERLINE:
      g_value_set_boolean (value, self->use_underline);
      break;
    case PROP_JUSTIFY:
      g_value_set_enum (value, self->jtype);
      break;
    case PROP_WRAP:
      g_value_set_boolean (value, self->wrap);
      break;
    case PROP_WRAP_MODE:
      g_value_set_enum (value, self->wrap_mode);
      break;
    case PROP_NATURAL_WRAP_MODE:
      g_value_set_enum (value, self->natural_wrap_mode);
      break;
    case PROP_SELECTABLE:
      g_value_set_boolean (value, bobgui_label_get_selectable (self));
      break;
    case PROP_MNEMONIC_KEYVAL:
      g_value_set_uint (value, self->mnemonic_keyval);
      break;
    case PROP_MNEMONIC_WIDGET:
      g_value_set_object (value, (GObject*) self->mnemonic_widget);
      break;
    case PROP_ELLIPSIZE:
      g_value_set_enum (value, self->ellipsize);
      break;
    case PROP_WIDTH_CHARS:
      g_value_set_int (value, bobgui_label_get_width_chars (self));
      break;
    case PROP_SINGLE_LINE_MODE:
      g_value_set_boolean (value, bobgui_label_get_single_line_mode (self));
      break;
    case PROP_MAX_WIDTH_CHARS:
      g_value_set_int (value, bobgui_label_get_max_width_chars (self));
      break;
    case PROP_LINES:
      g_value_set_int (value, bobgui_label_get_lines (self));
      break;
    case PROP_XALIGN:
      g_value_set_float (value, bobgui_label_get_xalign (self));
      break;
    case PROP_YALIGN:
      g_value_set_float (value, bobgui_label_get_yalign (self));
      break;
    case PROP_EXTRA_MENU:
      g_value_set_object (value, bobgui_label_get_extra_menu (self));
      break;
    case PROP_TABS:
      g_value_set_boxed (value, self->tabs);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_label_init (BobguiLabel *self)
{
  self->width_chars = -1;
  self->max_width_chars = -1;
  self->label = g_strdup ("");
  self->lines = -1;

  self->xalign = 0.5;
  self->yalign = 0.5;

  self->jtype = BOBGUI_JUSTIFY_LEFT;
  self->wrap = FALSE;
  self->wrap_mode = PANGO_WRAP_WORD;
  self->natural_wrap_mode = BOBGUI_NATURAL_WRAP_INHERIT;
  self->ellipsize = PANGO_ELLIPSIZE_NONE;

  self->use_underline = FALSE;
  self->use_markup = FALSE;

  self->mnemonic_keyval = GDK_KEY_VoidSymbol;
  self->layout = NULL;
  self->text = g_strdup ("");
  self->attrs = NULL;
  self->tabs = NULL;

  self->mnemonic_widget = NULL;

  self->mnemonics_visible = FALSE;
}

static const BobguiBuildableParser pango_parser =
{
  bobgui_pango_attribute_start_element,
};

static gboolean
bobgui_label_buildable_custom_tag_start (BobguiBuildable       *buildable,
                                      BobguiBuilder         *builder,
                                      GObject            *child,
                                      const char         *tagname,
                                      BobguiBuildableParser *parser,
                                      gpointer           *data)
{
  if (buildable_parent_iface->custom_tag_start (buildable, builder, child,
                                                tagname, parser, data))
    return TRUE;

  if (strcmp (tagname, "attributes") == 0)
    {
      BobguiPangoAttributeParserData *parser_data;

      bobgui_buildable_tag_deprecation_warning (buildable, builder, "attributes", "attributes");

      parser_data = g_new0 (BobguiPangoAttributeParserData, 1);
      parser_data->builder = g_object_ref (builder);
      parser_data->object = (GObject *) g_object_ref (buildable);
      *parser = pango_parser;
      *data = parser_data;
      return TRUE;
    }
  return FALSE;
}

static void
bobgui_label_buildable_custom_finished (BobguiBuildable *buildable,
                                     BobguiBuilder   *builder,
                                     GObject      *child,
                                     const char   *tagname,
                                     gpointer      user_data)
{
  BobguiPangoAttributeParserData *data = user_data;

  buildable_parent_iface->custom_finished (buildable, builder, child,
                                           tagname, user_data);

  if (strcmp (tagname, "attributes") == 0)
    {
      if (data->attrs)
        {
          bobgui_label_set_attributes (BOBGUI_LABEL (buildable), data->attrs);
          pango_attr_list_unref (data->attrs);
        }

      g_object_unref (data->object);
      g_object_unref (data->builder);
      g_free (data);
    }
}

static void
bobgui_label_buildable_interface_init (BobguiBuildableIface *iface)
{
  buildable_parent_iface = g_type_interface_peek_parent (iface);

  iface->custom_tag_start = bobgui_label_buildable_custom_tag_start;
  iface->custom_finished = bobgui_label_buildable_custom_finished;
}

static void
update_link_state (BobguiLabel *self)
{
  BobguiStateFlags state;
  guint i;

  if (!self->select_info)
    return;

  for (i = 0; i < self->select_info->n_links; i++)
    {
      const BobguiLabelLink *link = &self->select_info->links[i];

      state = bobgui_widget_get_state_flags (BOBGUI_WIDGET (self));
      if (link->visited)
        state |= BOBGUI_STATE_FLAG_VISITED;
      else
        state |= BOBGUI_STATE_FLAG_LINK;
      if (link == self->select_info->active_link)
        {
          if (self->select_info->link_clicked)
            state |= BOBGUI_STATE_FLAG_ACTIVE;
          else
            state |= BOBGUI_STATE_FLAG_PRELIGHT;
        }
      bobgui_css_node_set_state (link->cssnode, state);
    }
}

static void
bobgui_label_update_cursor (BobguiLabel *self)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);

  if (!self->select_info)
    return;

  if (bobgui_widget_is_sensitive (widget))
    {
      if (self->select_info->active_link)
        bobgui_widget_set_cursor_from_name (widget, "pointer");
      else if (self->select_info->selectable)
        bobgui_widget_set_cursor_from_name (widget, "text");
      else
        bobgui_widget_set_cursor (widget, NULL);
    }
  else
    bobgui_widget_set_cursor (widget, NULL);
}

static void
bobgui_label_state_flags_changed (BobguiWidget     *widget,
                               BobguiStateFlags  prev_state)
{
  BobguiLabel *self = BOBGUI_LABEL (widget);

  if (self->select_info)
    {
      BobguiStateFlags state;

      if (!bobgui_widget_is_sensitive (widget))
        bobgui_label_select_region (self, 0, 0);

      bobgui_label_update_cursor (self);
      update_link_state (self);

      state = bobgui_widget_get_state_flags (widget) & ~BOBGUI_STATE_FLAG_DROP_ACTIVE;

      if (self->select_info->selection_node)
        {
          bobgui_css_node_set_state (self->select_info->selection_node, state);

          bobgui_widget_queue_draw (widget);
        }
    }

  if (BOBGUI_WIDGET_CLASS (bobgui_label_parent_class)->state_flags_changed)
    BOBGUI_WIDGET_CLASS (bobgui_label_parent_class)->state_flags_changed (widget, prev_state);
}

static void
bobgui_label_update_layout_attributes (BobguiLabel      *self,
                                    PangoAttrList *style_attrs)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  BobguiCssStyle *style;
  PangoAttrList *attrs;

  if (self->layout == NULL)
    {
      pango_attr_list_unref (style_attrs);
      return;
    }

  if (self->select_info && self->select_info->links)
    {
      guint i;

      attrs = pango_attr_list_new ();

      for (i = 0; i < self->select_info->n_links; i++)
        {
          const BobguiLabelLink *link = &self->select_info->links[i];
          const GdkRGBA *link_color;
          PangoAttrList *link_attrs;
          PangoAttribute *attr;

          style = bobgui_css_node_get_style (link->cssnode);
          link_attrs = bobgui_css_style_get_pango_attributes (style);
          if (link_attrs)
            {
              GSList *attributes = pango_attr_list_get_attributes (link_attrs);
              GSList *l;
              for (l = attributes; l; l = l->next)
                {
                  attr = l->data;

                  attr->start_index = link->start;
                  attr->end_index = link->end;
                  pango_attr_list_insert (attrs, attr);
                }
              g_slist_free (attributes);
            }

          link_color = bobgui_css_color_value_get_rgba (style->used->color);

          attr = pango_attr_foreground_new (CLAMP (link_color->red * 65535. + 0.5, 0, 65535),
                                            CLAMP (link_color->green * 65535. + 0.5, 0, 65535),
                                            CLAMP (link_color->blue * 65535. + 0.5, 0, 65535));

          attr->start_index = link->start;
          attr->end_index = link->end;
          pango_attr_list_insert (attrs, attr);

          if (link_color->alpha < 0.999)
            {
              attr = pango_attr_foreground_alpha_new (CLAMP (link_color->alpha * 65535. + 0.5, 0, 65535));

              attr->start_index = link->start;
              attr->end_index = link->end;
              pango_attr_list_insert (attrs, attr);
            }

          pango_attr_list_unref (link_attrs);
        }
    }
  else
    attrs = NULL;

  style = bobgui_css_node_get_style (bobgui_widget_get_css_node (widget));
  if (!style_attrs)
    style_attrs = bobgui_css_style_get_pango_attributes (style);

  if (style_attrs)
    {
      attrs = _bobgui_pango_attr_list_merge (attrs, style_attrs);
      pango_attr_list_unref (style_attrs);
    }

  attrs = _bobgui_pango_attr_list_merge (attrs, self->markup_attrs);
  attrs = _bobgui_pango_attr_list_merge (attrs, self->attrs);

  pango_layout_set_attributes (self->layout, attrs);

  pango_attr_list_unref (attrs);
}

static void
bobgui_label_css_changed (BobguiWidget         *widget,
                       BobguiCssStyleChange *change)
{
  BobguiLabel *self = BOBGUI_LABEL (widget);
  gboolean attrs_affected;
  PangoAttrList *new_attrs = NULL;

  BOBGUI_WIDGET_CLASS (bobgui_label_parent_class)->css_changed (widget, change);

  if (bobgui_css_style_change_affects (change, BOBGUI_CSS_AFFECTS_TEXT_ATTRS))
    {
      new_attrs = bobgui_css_style_get_pango_attributes (bobgui_css_style_change_get_new_style (change));
      attrs_affected = (self->layout && pango_layout_get_attributes (self->layout)) ||
                       new_attrs;
    }
  else
    attrs_affected = FALSE;

  if (change == NULL || attrs_affected  || (self->select_info && self->select_info->links))
    {
      bobgui_label_update_layout_attributes (self, new_attrs);

      if (attrs_affected)
        bobgui_widget_queue_draw (widget);
    }
}

static PangoDirection
get_cursor_direction (BobguiLabel *self)
{
  GSList *l;

  g_assert (self->select_info);

  bobgui_label_ensure_layout (self);

  for (l = pango_layout_get_lines_readonly (self->layout); l; l = l->next)
    {
      PangoLayoutLine *line = l->data;

      /* If self->select_info->selection_end is at the very end of
       * the line, we don't know if the cursor is on this line or
       * the next without looking ahead at the next line. (End
       * of paragraph is different from line break.) But it's
       * definitely in this paragraph, which is good enough
       * to figure out the resolved direction.
       */
       if (pango_layout_line_get_start_index (line) + pango_layout_line_get_length (line) >= self->select_info->selection_end)
        return pango_layout_line_get_resolved_direction (line);
    }

  return PANGO_DIRECTION_LTR;
}

static int
_bobgui_label_get_link_at (BobguiLabel *self,
                        int       pos)
{
  if (self->select_info)
    {
      guint i;

      for (i = 0; i < self->select_info->n_links; i++)
        {
          const BobguiLabelLink *link = &self->select_info->links[i];

          if (link->start <= pos && pos < link->end)
            return i;
        }
    }

  return -1;
}

static BobguiLabelLink *
bobgui_label_get_focus_link (BobguiLabel *self,
                          int      *out_index)
{
  BobguiLabelSelectionInfo *info = self->select_info;
  int link_index;

  if (!info ||
      info->selection_anchor != info->selection_end)
    goto nope;

  link_index = _bobgui_label_get_link_at (self, info->selection_anchor);

  if (link_index != -1)
    {
      if (out_index)
        *out_index = link_index;

      return &info->links[link_index];
    }

nope:
  if (out_index)
    *out_index = -1;
  return NULL;
}

/**
 * bobgui_label_get_measuring_layout:
 * @self: the label
 * @existing_layout: an existing layout already in use
 * @width: the width to measure with, in Pango units, or -1 for infinite
 *
 * Gets a layout that can be used for measuring sizes.
 *
 * The returned layout will be identical to the label’s layout except
 * for the layout’s width, which will be set to @width. Do not modify
 * the returned layout.
 *
 * Returns: a Pango layout
 */
static PangoLayout *
bobgui_label_get_measuring_layout (BobguiLabel    *self,
                                PangoLayout *existing_layout,
                                int          width)
{
  PangoLayout *copy;

  if (existing_layout != NULL)
    {
      if (existing_layout != self->layout)
        {
          pango_layout_set_width (existing_layout, width);
          return existing_layout;
        }

      g_object_unref (existing_layout);
    }

  bobgui_label_ensure_layout (self);

  if (pango_layout_get_width (self->layout) == width)
    {
      g_object_ref (self->layout);
      return self->layout;
    }

  /* We can use the label's own layout if we're not allocated a size yet,
   * because we don't need it to be properly setup at that point.
   * This way we can make use of caching upon the label's creation.
   */
  if (bobgui_widget_get_width (BOBGUI_WIDGET (self)) <= 1)
    {
      g_object_ref (self->layout);
      pango_layout_set_width (self->layout, width);
      return self->layout;
    }

  /* oftentimes we want to measure a width that is far wider than the current width,
   * even though the layout would not change if we made it wider. In that case, we
   * can just return the current layout, because for measuring purposes, it will be
   * identical.
   */
  if (!pango_layout_is_wrapped (self->layout) &&
      !pango_layout_is_ellipsized (self->layout))
    {
      PangoRectangle rect;

      if (width == -1)
        return g_object_ref (self->layout);

      pango_layout_get_extents (self->layout, NULL, &rect);
      if (rect.width <= width)
        return g_object_ref (self->layout);
    }

  copy = pango_layout_copy (self->layout);
  pango_layout_set_width (copy, width);
  return copy;
}

static int
get_char_pixels (PangoLayout *layout)
{
  PangoContext *context;
  PangoFontMetrics *metrics;
  int char_width, digit_width;

  context = pango_layout_get_context (layout);
  metrics = pango_context_get_metrics (context, NULL, NULL);
  char_width = pango_font_metrics_get_approximate_char_width (metrics);
  digit_width = pango_font_metrics_get_approximate_digit_width (metrics);
  pango_font_metrics_unref (metrics);

  return MAX (char_width, digit_width);
}

static void
get_default_widths (BobguiLabel    *self,
                    int         *minimum,
                    int         *natural)
{
  int char_pixels;

  if (self->width_chars < 0 && self->max_width_chars < 0)
    {
      if (minimum)
        *minimum = -1;
      if (natural)
        *natural = -1;
      return;
    }

  bobgui_label_ensure_layout (self);
  char_pixels = get_char_pixels (self->layout);

  if (minimum)
    {
      if (self->width_chars < 0)
        *minimum = -1;
      else
        *minimum = char_pixels * self->width_chars;
    }

  if (natural)
    {
      if (self->max_width_chars < 0)
        *natural = -1;
      else
        *natural = char_pixels * MAX (self->width_chars, self->max_width_chars);
    }
}

static void
get_static_size (BobguiLabel       *self,
                 BobguiOrientation  orientation,
                 int            *minimum,
                 int            *natural,
                 int            *minimum_baseline,
                 int            *natural_baseline)
{
  int minimum_default, natural_default;
  PangoLayout *layout;

  get_default_widths (self, &minimum_default, &natural_default);

  layout = bobgui_label_get_measuring_layout (self, NULL, self->ellipsize ? natural_default : -1);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      pango_layout_get_size (layout, natural, NULL);
      if (self->ellipsize)
        {
          layout = bobgui_label_get_measuring_layout (self, layout, 0);
          pango_layout_get_size (layout, minimum, NULL);
          /* yes, Pango ellipsizes even when that needs more space */
          *minimum = MIN (*minimum, *natural);
        }
      else
        *minimum = *natural;

      if (minimum_default > *minimum)
        *minimum = minimum_default;
      *natural = MAX (*minimum, *natural);
    }
  else
    {
      pango_layout_get_size (layout, NULL, minimum);
      *minimum_baseline = pango_layout_get_baseline (layout);

      *natural = *minimum;
      *natural_baseline = *minimum_baseline;
    }

  g_object_unref (layout);
}

static void
get_height_for_width (BobguiLabel *self,
                      int       width,
                      int      *minimum_height,
                      int      *natural_height,
                      int      *minimum_baseline,
                      int      *natural_baseline)
{
  PangoLayout *layout;
  int natural_width, text_height, baseline;

  if (width < 0)
    {
      /* Minimum height is assuming infinite width */
      layout = bobgui_label_get_measuring_layout (self, NULL, -1);
      pango_layout_get_size (layout, NULL, minimum_height);
      baseline = pango_layout_get_baseline (layout);
      *minimum_baseline = baseline;

      /* Natural height is assuming natural width */
      get_default_widths (self, NULL, &natural_width);

      layout = bobgui_label_get_measuring_layout (self, layout, natural_width);
      pango_layout_get_size (layout, NULL, natural_height);
      baseline = pango_layout_get_baseline (layout);
      *natural_baseline = baseline;
    }
  else
    {
      /* minimum = natural for any given width */
      layout = bobgui_label_get_measuring_layout (self, NULL, width);

      pango_layout_get_size (layout, NULL, &text_height);

      *minimum_height = text_height;
      *natural_height = text_height;

      baseline = pango_layout_get_baseline (layout);
      *minimum_baseline = baseline;
      *natural_baseline = baseline;
    }

  g_object_unref (layout);
}

static int
my_pango_layout_get_width_for_height (PangoLayout *layout,
                                      int          for_height,
                                      int          min,
                                      int          max)
{
  int mid, text_width, text_height;

  min = PANGO_PIXELS_CEIL (min);
  max = PANGO_PIXELS_CEIL (max);

  while (min < max)
    {
      mid = (min + max) / 2;
      pango_layout_set_width (layout, mid * PANGO_SCALE);
      pango_layout_get_size (layout, &text_width, &text_height);
      text_width = PANGO_PIXELS_CEIL (text_width);
      if (text_width > mid)
        min = text_width;
      else if (text_height > for_height)
        min = mid + 1;
      else
        max = text_width;
    }

  return min * PANGO_SCALE;
}

static void
get_width_for_height (BobguiLabel *self,
                      int       height,
                      int      *minimum_width,
                      int      *natural_width)
{
  PangoLayout *layout;
  int minimum_default, natural_default;

  get_default_widths (self, &minimum_default, &natural_default);

  if (height < 0)
    {
      /* Minimum width is as many line breaks as possible */
      layout = bobgui_label_get_measuring_layout (self, NULL, MAX (minimum_default, 0));
      pango_layout_get_size (layout, minimum_width, NULL);
      *minimum_width = MAX (*minimum_width, minimum_default);

      /* Natural width is natural width - or as wide as possible */
      layout = bobgui_label_get_measuring_layout (self, layout, natural_default);
      pango_layout_get_size (layout, natural_width, NULL);
      *natural_width = MAX (*natural_width, *minimum_width);
    }
  else
    {
      int min, max;

      /* Can't use a measuring layout here, because we need to force
       * ellipsizing mode */
      bobgui_label_ensure_layout (self);
      layout = pango_layout_copy (self->layout);
      pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_NONE);

      /* binary search for the smallest width where the height doesn't
       * eclipse the given height */
      min = MAX (minimum_default, 0);

      pango_layout_set_width (layout, -1);
      pango_layout_get_size (layout, &max, NULL);

      /* first, do natural width */
      if (self->natural_wrap_mode == BOBGUI_NATURAL_WRAP_NONE)
        {
          *natural_width = max;
        }
      else
        {
          if (self->natural_wrap_mode == BOBGUI_NATURAL_WRAP_WORD)
            pango_layout_set_wrap (layout, PANGO_WRAP_WORD);
          *natural_width = my_pango_layout_get_width_for_height (layout, height, min, max);
        }

      /* then, do minimum width */
      if (self->natural_wrap_mode == BOBGUI_NATURAL_WRAP_INHERIT && self->ellipsize == PANGO_ELLIPSIZE_NONE)
        {
          *minimum_width = *natural_width;
        }
      else
        {
          pango_layout_set_wrap (layout, self->wrap_mode);
          pango_layout_set_ellipsize (layout, self->ellipsize);
          *minimum_width = my_pango_layout_get_width_for_height (layout, height, min, *natural_width);
        }
    }

  g_object_unref (layout);
}

static void
bobgui_label_measure (BobguiWidget      *widget,
                   BobguiOrientation  orientation,
                   int             for_size,
                   int            *minimum,
                   int            *natural,
                   int            *minimum_baseline,
                   int            *natural_baseline)
{
  BobguiLabel *self = BOBGUI_LABEL (widget);

  if (for_size > 0)
    for_size *= PANGO_SCALE;

  if (!self->wrap)
    get_static_size (self, orientation, minimum, natural, minimum_baseline, natural_baseline);
  else if (orientation == BOBGUI_ORIENTATION_VERTICAL)
    get_height_for_width (self, for_size, minimum, natural, minimum_baseline, natural_baseline);
  else
    get_width_for_height (self, for_size, minimum, natural);

  *minimum = PANGO_PIXELS_CEIL (*minimum);
  *natural = PANGO_PIXELS_CEIL (*natural);
  if (*minimum_baseline > 0)
    *minimum_baseline = PANGO_PIXELS_CEIL (*minimum_baseline);
  if (*natural_baseline > 0)
    *natural_baseline = PANGO_PIXELS_CEIL (*natural_baseline);
}

void
bobgui_label_get_layout_location (BobguiLabel  *self,
                               float     *xp,
                               float     *yp)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  const int widget_width = bobgui_widget_get_width (widget);
  const int widget_height = bobgui_widget_get_height (widget);
  PangoRectangle logical;
  float xalign;
  int baseline;
  float x, y;

  g_assert (xp);
  g_assert (yp);

  bobgui_label_ensure_layout (self);

  xalign = self->xalign;

  if (_bobgui_widget_get_direction (widget) != BOBGUI_TEXT_DIR_LTR)
    xalign = 1.0 - xalign;

  pango_layout_get_pixel_extents (self->layout, NULL, &logical);
  x = floor ((xalign * (widget_width - logical.width)) - logical.x);

  baseline = bobgui_widget_get_baseline (widget);
  if (baseline != -1)
    {
      int layout_baseline = pango_layout_get_baseline (self->layout) / PANGO_SCALE;
      /* yalign is 0 because we can't support yalign while baseline aligning */
      y = baseline - layout_baseline;
    }
  else
    {
      y = floor ((widget_height - logical.height) * self->yalign);
    }

  *xp = x;
  *yp = y;
}

static void
bobgui_label_size_allocate (BobguiWidget *widget,
                         int        width,
                         int        height,
                         int        baseline)
{
  BobguiLabel *self = BOBGUI_LABEL (widget);

  if (self->layout)
    {
      if (self->ellipsize || self->wrap)
        pango_layout_set_width (self->layout, width * PANGO_SCALE);
      else
        pango_layout_set_width (self->layout, -1);
    }

  if (self->popup_menu)
    bobgui_popover_present (BOBGUI_POPOVER (self->popup_menu));
}



#define GRAPHENE_RECT_FROM_RECT(_r) (GRAPHENE_RECT_INIT ((_r)->x, (_r)->y, (_r)->width, (_r)->height))

static void
bobgui_label_snapshot (BobguiWidget   *widget,
                    BobguiSnapshot *snapshot)
{
  BobguiLabel *self = BOBGUI_LABEL (widget);
  BobguiLabelSelectionInfo *info;
  BobguiCssStyle *style;
  float lx, ly;
  int width, height;
  BobguiCssBoxes boxes;

  if (!self->text || (*self->text == '\0'))
    return;

  bobgui_label_ensure_layout (self);

  bobgui_label_get_layout_location (self, &lx, &ly);

  bobgui_css_boxes_init (&boxes, widget);
  bobgui_css_style_snapshot_layout (&boxes, snapshot, lx, ly, self->layout);

  info = self->select_info;
  if (!info)
    return;

  width = bobgui_widget_get_width (widget);
  height = bobgui_widget_get_height (widget);

  if (info->selection_anchor != info->selection_end)
    {
      int range[2];
      cairo_region_t *range_clip;
      cairo_rectangle_int_t clip_rect;
      int i;

      range[0] = MIN (info->selection_anchor, info->selection_end);
      range[1] = MAX (info->selection_anchor, info->selection_end);

      style = bobgui_css_node_get_style (info->selection_node);
      bobgui_css_boxes_init_border_box (&boxes, style, 0, 0, width, height);

      range_clip = gdk_pango_layout_get_clip_region (self->layout, lx, ly, range, 1);
      for (i = 0; i < cairo_region_num_rectangles (range_clip); i++)
        {
          cairo_region_get_rectangle (range_clip, i, &clip_rect);

          bobgui_snapshot_push_clip (snapshot, &GRAPHENE_RECT_FROM_RECT (&clip_rect));
          bobgui_css_style_snapshot_background (&boxes, snapshot);
          bobgui_css_style_snapshot_layout (&boxes, snapshot, lx, ly, self->layout);
          bobgui_snapshot_pop (snapshot);
        }

      cairo_region_destroy (range_clip);
    }
  else
    {
      BobguiLabelLink *focus_link;
      BobguiLabelLink *active_link;
      int range[2];
      cairo_region_t *range_clip;
      cairo_rectangle_int_t clip_rect;
      int i;
      GdkRectangle rect;

      if (info->selectable &&
          bobgui_widget_has_focus (widget) &&
          bobgui_widget_is_drawable (widget))
        {
          PangoDirection cursor_direction;

          cursor_direction = get_cursor_direction (self);
          bobgui_css_style_snapshot_caret (&boxes, bobgui_widget_get_display (widget),
                                        snapshot,
                                        lx, ly,
                                        self->layout,
                                        self->select_info->selection_end,
                                        cursor_direction);
        }

      focus_link = bobgui_label_get_focus_link (self, NULL);
      active_link = info->active_link;

      if (active_link)
        {
          range[0] = active_link->start;
          range[1] = active_link->end;

          style = bobgui_css_node_get_style (active_link->cssnode);
          bobgui_css_boxes_init_border_box (&boxes, style, 0, 0, width, height);

          range_clip = gdk_pango_layout_get_clip_region (self->layout, lx, ly, range, 1);
          for (i = 0; i < cairo_region_num_rectangles (range_clip); i++)
            {
              cairo_region_get_rectangle (range_clip, i, &clip_rect);

              bobgui_snapshot_push_clip (snapshot, &GRAPHENE_RECT_FROM_RECT (&clip_rect));
              bobgui_css_style_snapshot_background (&boxes, snapshot);
              bobgui_css_style_snapshot_layout (&boxes, snapshot, lx, ly, self->layout);
              bobgui_snapshot_pop (snapshot);
            }

          cairo_region_destroy (range_clip);
        }

      if (focus_link && bobgui_widget_has_visible_focus (widget))
        {
          range[0] = focus_link->start;
          range[1] = focus_link->end;

          style = bobgui_css_node_get_style (focus_link->cssnode);

          range_clip = gdk_pango_layout_get_clip_region (self->layout, lx, ly, range, 1);
          cairo_region_get_extents (range_clip, &rect);

          bobgui_css_boxes_init_border_box (&boxes, style, rect.x, rect.y, rect.width, rect.height);
          bobgui_css_style_snapshot_outline (&boxes, snapshot);

          cairo_region_destroy (range_clip);
        }
    }
}

static BobguiSizeRequestMode
bobgui_label_get_request_mode (BobguiWidget *widget)
{
  BobguiLabel *self = BOBGUI_LABEL (widget);

  if (self->wrap)
    return BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH;

  return BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;
}

static void
bobgui_label_dispose (GObject *object)
{
  BobguiLabel *self = BOBGUI_LABEL (object);

  bobgui_label_set_mnemonic_widget (self, NULL);
  bobgui_label_clear_select_info (self);
  bobgui_label_clear_provider_info (self);

  G_OBJECT_CLASS (bobgui_label_parent_class)->dispose (object);
}

static void
bobgui_label_clear_links (BobguiLabel *self)
{
  guint i;

  if (!self->select_info)
    return;

  for (i = 0; i < self->select_info->n_links; i++)
    {
      BobguiLabelLink *link = &self->select_info->links[i];
      bobgui_css_node_set_parent (link->cssnode, NULL);
      g_clear_object (&link->accessible);
      g_free (link->uri);
      g_free (link->title);
    }
  g_free (self->select_info->links);
  self->select_info->links = NULL;
  self->select_info->n_links = 0;
  self->select_info->active_link = NULL;
  bobgui_widget_remove_css_class (BOBGUI_WIDGET (self), "link");
}

static void
bobgui_label_finalize (GObject *object)
{
  BobguiLabel *self = BOBGUI_LABEL (object);

  g_free (self->label);
  g_free (self->text);

  g_clear_object (&self->layout);
  g_clear_pointer (&self->attrs, pango_attr_list_unref);
  g_clear_pointer (&self->markup_attrs, pango_attr_list_unref);

  if (self->select_info && self->select_info->provider)
    g_object_unref (self->select_info->provider);

  bobgui_label_clear_links (self);
  g_free (self->select_info);

  g_clear_pointer (&self->popup_menu, bobgui_widget_unparent);
  g_clear_object (&self->extra_menu);

  g_clear_pointer (&self->tabs, pango_tab_array_free);

  G_OBJECT_CLASS (bobgui_label_parent_class)->finalize (object);
}

static void
bobgui_label_unrealize (BobguiWidget *widget)
{
  BobguiLabel *self = BOBGUI_LABEL (widget);

  if (self->select_info &&
      self->select_info->provider)
    {
      GdkClipboard *clipboard = bobgui_widget_get_primary_clipboard (widget);

      if (gdk_clipboard_get_content (clipboard) == self->select_info->provider)
        gdk_clipboard_set_content (clipboard, NULL);
    }

  BOBGUI_WIDGET_CLASS (bobgui_label_parent_class)->unrealize (widget);
}

static gboolean
range_is_in_ellipsis_full (BobguiLabel *self,
                           int       range_start,
                           int       range_end,
                           int      *ellipsis_start,
                           int      *ellipsis_end)
{
  PangoLayoutIter *iter;
  gboolean in_ellipsis;

  if (!self->ellipsize)
    return FALSE;

  bobgui_label_ensure_layout (self);

  if (!pango_layout_is_ellipsized (self->layout))
    return FALSE;

  iter = pango_layout_get_iter (self->layout);

  in_ellipsis = FALSE;

  do {
    PangoLayoutRun *run;

    run = pango_layout_iter_get_run_readonly (iter);
    if (run)
      {
        PangoItem *item;

        item = ((PangoGlyphItem*)run)->item;

        if (item->offset <= range_start && range_end <= item->offset + item->length)
          {
            if (item->analysis.flags & PANGO_ANALYSIS_FLAG_IS_ELLIPSIS)
              {
                if (ellipsis_start)
                  *ellipsis_start = item->offset;
                if (ellipsis_end)
                  *ellipsis_end = item->offset + item->length;
                in_ellipsis = TRUE;
              }
            break;
          }
        else if (item->offset + item->length >= range_end)
          break;
      }
  } while (pango_layout_iter_next_run (iter));

  pango_layout_iter_free (iter);

  return in_ellipsis;
}

static gboolean
range_is_in_ellipsis (BobguiLabel *self,
                      int       range_start,
                      int       range_end)
{
  return range_is_in_ellipsis_full (self, range_start, range_end, NULL, NULL);
}

static gboolean
bobgui_label_grab_focus (BobguiWidget *widget)
{
  BobguiLabel *self = BOBGUI_LABEL (widget);
  gboolean select_on_focus;
  BobguiWidget *prev_focus;

  if (self->select_info == NULL)
    return FALSE;

  prev_focus = bobgui_root_get_focus (bobgui_widget_get_root (widget));

  if (!BOBGUI_WIDGET_CLASS (bobgui_label_parent_class)->grab_focus (widget))
    return FALSE;

  if (self->select_info->selectable)
    {
      g_object_get (bobgui_widget_get_settings (widget),
                    "bobgui-label-select-on-focus",
                    &select_on_focus,
                    NULL);

      if (select_on_focus && !self->in_click &&
          !(prev_focus && bobgui_widget_is_ancestor (prev_focus, widget)))
        bobgui_label_select_region (self, 0, -1);
    }
  else
    {
      if (self->select_info->links && !self->in_click &&
          !(prev_focus && bobgui_widget_is_ancestor (prev_focus, widget)))
        {
          guint i;

          for (i = 0; i < self->select_info->n_links; i++)
            {
              const BobguiLabelLink *link = &self->select_info->links[i];

              if (!range_is_in_ellipsis (self, link->start, link->end))
                {
                  self->select_info->selection_anchor = link->start;
                  self->select_info->selection_end = link->start;
                  break;
                }
            }
        }
    }

  return TRUE;
}

static gboolean
get_layout_index (BobguiLabel *self,
                  int       x,
                  int       y,
                  int      *index)
{
  int trailing = 0;
  const char *cluster;
  const char *cluster_end;
  gboolean inside;
  float lx, ly;

  *index = 0;

  bobgui_label_ensure_layout (self);
  bobgui_label_get_layout_location (self, &lx, &ly);

  /* Translate x/y to layout position */
  x -= lx;
  y -= ly;

  x *= PANGO_SCALE;
  y *= PANGO_SCALE;

  inside = pango_layout_xy_to_index (self->layout,
                                     x, y,
                                     index, &trailing);

  cluster = self->text + *index;
  cluster_end = cluster;
  while (trailing)
    {
      cluster_end = g_utf8_next_char (cluster_end);
      --trailing;
    }

  *index += (cluster_end - cluster);

  return inside;
}

static gboolean
bobgui_label_query_tooltip (BobguiWidget  *widget,
                         int         x,
                         int         y,
                         gboolean    keyboard_tip,
                         BobguiTooltip *tooltip)
{
  BobguiLabel *self = BOBGUI_LABEL (widget);
  BobguiLabelSelectionInfo *info = self->select_info;
  int index = -1;

  if (info && info->links)
    {
      if (keyboard_tip)
        {
          if (info->selection_anchor == info->selection_end)
            index = info->selection_anchor;
        }
      else
        {
          if (!get_layout_index (self, x, y, &index))
            index = -1;
        }

      if (index != -1)
        {
          const int link_index = _bobgui_label_get_link_at (self, index);

          if (link_index != -1)
            {
              const BobguiLabelLink *link = &info->links[link_index];

              if (link->title)
                {
                  bobgui_tooltip_set_markup (tooltip, link->title);
                  return TRUE;
                }
            }
        }
    }

  return BOBGUI_WIDGET_CLASS (bobgui_label_parent_class)->query_tooltip (widget,
                                                                   x, y,
                                                                   keyboard_tip,
                                                                   tooltip);
}

static void
set_link_focus (BobguiLabelLink *link,
                gboolean      focused)
{
  if (link->accessible)
    bobgui_accessible_hyperlink_set_platform_state (link->accessible,
                                                 BOBGUI_ACCESSIBLE_PLATFORM_STATE_FOCUSED,
                                                 focused);
}

static gboolean
bobgui_label_focus (BobguiWidget        *widget,
                 BobguiDirectionType  direction)
{
  BobguiLabel *self = BOBGUI_LABEL (widget);
  BobguiLabelSelectionInfo *info = self->select_info;
  BobguiWidget *focus_child = bobgui_widget_get_focus_child (widget);
  BobguiLabelLink *focus_link;
  gboolean focus_in_child = FALSE;

  if (focus_child)
    {
      focus_in_child = bobgui_widget_child_focus (focus_child, direction);
      if (focus_in_child)
        return TRUE;
    }

  if (!bobgui_widget_is_focus (widget))
    {
      bobgui_widget_grab_focus (widget);
      if (info)
        {
          focus_link = bobgui_label_get_focus_link (self, NULL);
          if (focus_link && direction == BOBGUI_DIR_TAB_BACKWARD)
            {
              int i;
              set_link_focus (focus_link, TRUE);
              for (i = info->n_links - 1; i >= 0; i--)
                {
                  focus_link = &info->links[i];
                  if (!range_is_in_ellipsis (self, focus_link->start, focus_link->end))
                    {
                      info->selection_anchor = focus_link->start;
                      info->selection_end = focus_link->start;
                      break;
                    }
                }
            }

          return TRUE;
        }

      return FALSE;
    }

  if (!info)
    return FALSE;

  if (info->selectable)
    {
      int index;

      if (info->selection_anchor != info->selection_end)
        goto out;

      focus_link = bobgui_label_get_focus_link (self, NULL);
      if (focus_link)
        set_link_focus (focus_link, FALSE);

      index = info->selection_anchor;

      if (direction == BOBGUI_DIR_TAB_FORWARD)
        {
          guint i;
          for (i = 0; i < info->n_links; i++)
            {
              BobguiLabelLink *link = &info->links[i];

              if (link->start > index)
                {
                  set_link_focus (link, TRUE);
                  if (!range_is_in_ellipsis (self, link->start, link->end))
                    {
                      bobgui_label_select_region_index (self, link->start, link->start);
                      return TRUE;
                    }
                }
            }
        }
      else if (direction == BOBGUI_DIR_TAB_BACKWARD)
        {
          int i;
          for (i = info->n_links - 1; i >= 0; i--)
            {
              BobguiLabelLink *link = &info->links[i];

              if (link->end < index)
                {
                  set_link_focus (link, TRUE);
                  if (!range_is_in_ellipsis (self, link->start, link->end))
                    {
                      bobgui_label_select_region_index (self, link->start, link->start);
                      return TRUE;
                    }
                }
            }
        }

      goto out;
    }
  else
    {
      int focus_link_index;
      int new_index = -1;

      if (info->n_links == 0)
        goto out;

      focus_link = bobgui_label_get_focus_link (self, &focus_link_index);

      if (!focus_link)
        goto out;

      set_link_focus (focus_link, FALSE);

      switch (direction)
        {
        case BOBGUI_DIR_TAB_FORWARD:
          if (focus_link)
            new_index = focus_link_index + 1;
          else
            new_index = 0;

          if (new_index >= info->n_links)
            goto out;

          while (new_index < info->n_links)
            {
              const BobguiLabelLink *link = &info->links[new_index];
              if (!range_is_in_ellipsis (self, link->start, link->end))
                break;

              new_index++;
            }
          break;

        case BOBGUI_DIR_TAB_BACKWARD:
          if (focus_link)
            new_index = focus_link_index - 1;
          else
            new_index = info->n_links - 1;

          if (new_index < 0)
            goto out;

          while (new_index >= 0)
            {
              const BobguiLabelLink *link = &info->links[new_index];
              if (!range_is_in_ellipsis (self, link->start, link->end))
                break;

              new_index--;
            }
          break;

        default:
        case BOBGUI_DIR_UP:
        case BOBGUI_DIR_DOWN:
        case BOBGUI_DIR_LEFT:
        case BOBGUI_DIR_RIGHT:
          goto out;
        }

      if (new_index != -1 && new_index < info->n_links)
        {
          focus_link = &info->links[new_index];
          set_link_focus (focus_link, TRUE);
          info->selection_anchor = focus_link->start;
          info->selection_end = focus_link->start;
          bobgui_widget_queue_draw (widget);

          return TRUE;
        }
    }

out:

  return FALSE;
}

static void
emit_activate_link (BobguiLabel     *self,
                    BobguiLabelLink *link)
{
  gboolean handled;

  g_signal_emit (self, signals[ACTIVATE_LINK], 0, link->uri, &handled);

  /* signal handler might have invalidated the layout */
  if (!self->layout)
    return;

  if (handled && !link->visited &&
      self->select_info && self->select_info->links)
    {
      link->visited = TRUE;
      update_link_state (self);
    }
}

static void
bobgui_label_activate_link_open (BobguiWidget  *widget,
                              const char *name,
                              GVariant   *parameter)
{
  BobguiLabel *self = BOBGUI_LABEL (widget);
  if (self->select_info)
    {
      BobguiLabelLink *link = self->select_info->context_link;

      if (link)
        emit_activate_link (self, link);
    }
}

static void
bobgui_label_activate_link_copy (BobguiWidget  *widget,
                              const char *name,
                              GVariant   *parameter)
{
  BobguiLabel *self = BOBGUI_LABEL (widget);
  if (self->select_info)
    {
      BobguiLabelLink *link = self->select_info->context_link;

      if (link)
        {
          GdkClipboard *clipboard;

          clipboard = bobgui_widget_get_clipboard (widget);
          gdk_clipboard_set_text (clipboard, link->uri);
        }
    }
}

static void
bobgui_label_activate_clipboard_copy (BobguiWidget  *widget,
                                   const char *name,
                                   GVariant   *parameter)
{
  g_signal_emit_by_name (widget, "copy-clipboard");
}

static gboolean
bobgui_label_select_all (BobguiLabel *self)
{
  bobgui_label_select_region_index (self, 0, strlen (self->text));
  return bobgui_label_get_selectable (self);
}

static void
bobgui_label_activate_selection_select_all (BobguiWidget  *widget,
                                         const char *name,
                                         GVariant   *parameter)
{
  bobgui_label_select_all (BOBGUI_LABEL (widget));
}

static void
bobgui_label_nop (BobguiWidget  *widget,
               const char *name,
               GVariant   *parameter)
{
}

static gboolean
bobgui_label_mnemonic_activate (BobguiWidget *widget,
                             gboolean   group_cycling)
{
  BobguiLabel *self = BOBGUI_LABEL (widget);
  BobguiWidget *parent;

  if (self->mnemonic_widget)
    return bobgui_widget_mnemonic_activate (self->mnemonic_widget, group_cycling);

  /* Not a label for something else, but is selectable, so set focus into
   * the label itself.
  */
  if (bobgui_label_get_selectable (self) && bobgui_widget_get_focusable (widget))
    return bobgui_label_grab_focus (widget);

  /* Try to find the widget to activate by traversing the
   * widget's ancestry.
   */
  parent = bobgui_widget_get_parent (widget);

  if (BOBGUI_IS_NOTEBOOK (parent))
    return FALSE;

  while (parent)
    {
      if (bobgui_widget_get_focusable (parent) ||
          (!group_cycling && bobgui_widget_can_activate (parent)) ||
          BOBGUI_IS_NOTEBOOK (bobgui_widget_get_parent (parent)))
        return bobgui_widget_mnemonic_activate (parent, group_cycling);
      parent = bobgui_widget_get_parent (parent);
    }

  /* barf if there was nothing to activate */
  g_warning ("Couldn't find a target for a mnemonic activation.");
  bobgui_widget_error_bell (widget);

  return FALSE;
}

static void
bobgui_label_popup_menu (BobguiWidget  *widget,
                      const char *action_name,
                      GVariant   *parameters)
{
  BobguiLabel *self = BOBGUI_LABEL (widget);

  bobgui_label_do_popup (self, -1, -1);
}

static void
bobgui_label_root (BobguiWidget *widget)
{
  BobguiLabel *self = BOBGUI_LABEL (widget);

  BOBGUI_WIDGET_CLASS (bobgui_label_parent_class)->root (widget);

  bobgui_label_setup_mnemonic (self);

  /* The PangoContext is replaced when the display changes, so clear the layouts */
  bobgui_label_clear_layout (BOBGUI_LABEL (widget));
}

static void
bobgui_label_unroot (BobguiWidget *widget)
{
  BobguiLabel *self = BOBGUI_LABEL (widget);

  bobgui_label_setup_mnemonic (self);

  BOBGUI_WIDGET_CLASS (bobgui_label_parent_class)->unroot (widget);
}

static void
launch_done (GObject      *source,
             GAsyncResult *result,
             gpointer      data)
{
  GError *error = NULL;
  gboolean success;

  if (BOBGUI_IS_FILE_LAUNCHER (source))
    success = bobgui_file_launcher_launch_finish (BOBGUI_FILE_LAUNCHER (source), result, &error);
  else if (BOBGUI_IS_URI_LAUNCHER (source))
    success = bobgui_uri_launcher_launch_finish (BOBGUI_URI_LAUNCHER (source), result, &error);
  else
    g_assert_not_reached ();

  if (!success)
    {
      g_warning ("Failed to launch handler: %s", error->message);
      g_error_free (error);
    }
}

static gboolean
bobgui_label_activate_link (BobguiLabel    *self,
                         const char *uri)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  BobguiWidget *toplevel = BOBGUI_WIDGET (bobgui_widget_get_root (widget));
  const char *uri_scheme;

  if (!BOBGUI_IS_WINDOW (toplevel))
    return FALSE;

  uri_scheme = g_uri_peek_scheme (uri);
  if (g_strcmp0 (uri_scheme, "file") == 0)
    {
      GFile *file;
      BobguiFileLauncher *launcher;

      file = g_file_new_for_uri (uri);
      launcher = bobgui_file_launcher_new (file);
      bobgui_file_launcher_launch (launcher, BOBGUI_WINDOW (toplevel), NULL, launch_done, NULL);
      g_object_unref (launcher);
      g_object_unref (file);
    }
  else
    {
      BobguiUriLauncher *launcher;

      launcher = bobgui_uri_launcher_new (uri);
      bobgui_uri_launcher_launch (launcher, BOBGUI_WINDOW (toplevel), NULL, launch_done, NULL);
      g_object_unref (launcher);
    }

  return TRUE;
}

static void
bobgui_label_activate_current_link (BobguiLabel *self)
{
  BobguiLabelLink *link;
  BobguiWidget *widget = BOBGUI_WIDGET (self);

  link = bobgui_label_get_focus_link (self, NULL);

  if (link)
    emit_activate_link (self, link);
  else
    bobgui_widget_activate_default (widget);
}

static void
bobgui_label_copy_clipboard (BobguiLabel *self)
{
  if (self->text && self->select_info)
    {
      int start, end;
      int len;
      GdkClipboard *clipboard;

      start = MIN (self->select_info->selection_anchor,
                   self->select_info->selection_end);
      end = MAX (self->select_info->selection_anchor,
                 self->select_info->selection_end);

      len = strlen (self->text);

      if (end > len)
        end = len;

      if (start > len)
        start = len;

      clipboard = bobgui_widget_get_clipboard (BOBGUI_WIDGET (self));

      if (start != end)
        {
          char *str = g_strndup (self->text + start, end - start);
          gdk_clipboard_set_text (clipboard, str);
          g_free (str);
        }
      else
        {
          BobguiLabelLink *link;

          link = bobgui_label_get_focus_link (self, NULL);
          if (link)
            gdk_clipboard_set_text (clipboard, link->uri);
        }
    }
}

static void
bobgui_label_direction_changed (BobguiWidget        *widget,
                             BobguiTextDirection  previous_direction)
{
  bobgui_label_clear_layout (BOBGUI_LABEL (widget));
}

static void
bobgui_label_class_init (BobguiLabelClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  gobject_class->set_property = bobgui_label_set_property;
  gobject_class->get_property = bobgui_label_get_property;
  gobject_class->finalize = bobgui_label_finalize;
  gobject_class->dispose = bobgui_label_dispose;

  widget_class->size_allocate = bobgui_label_size_allocate;
  widget_class->state_flags_changed = bobgui_label_state_flags_changed;
  widget_class->css_changed = bobgui_label_css_changed;
  widget_class->query_tooltip = bobgui_label_query_tooltip;
  widget_class->snapshot = bobgui_label_snapshot;
  widget_class->unrealize = bobgui_label_unrealize;
  widget_class->root = bobgui_label_root;
  widget_class->unroot = bobgui_label_unroot;
  widget_class->mnemonic_activate = bobgui_label_mnemonic_activate;
  widget_class->grab_focus = bobgui_label_grab_focus;
  widget_class->focus = bobgui_label_focus;
  widget_class->get_request_mode = bobgui_label_get_request_mode;
  widget_class->measure = bobgui_label_measure;
  widget_class->direction_changed = bobgui_label_direction_changed;

  class->move_cursor = bobgui_label_move_cursor;
  class->copy_clipboard = bobgui_label_copy_clipboard;
  class->activate_link = bobgui_label_activate_link;

  /**
   * BobguiLabel::move-cursor:
   * @entry: the label which emitted the signal
   * @step: the granularity of the move, as a `BobguiMovementStep`
   * @count: the number of @step units to move
   * @extend_selection: true if the move should extend the selection
   *
   * Gets emitted when the user initiates a cursor movement.
   *
   * The `::move-cursor` signal is a [keybinding signal](class.SignalAction.html).
   * If the cursor is not visible in @entry, this signal causes the viewport to
   * be moved instead.
   *
   * Applications should not connect to it, but may emit it with
   * [func@GObject.signal_emit_by_name] if they need to control
   * the cursor programmatically.
   *
   * The default bindings for this signal come in two variants, the
   * variant with the <kbd>Shift</kbd> modifier extends the selection,
   * the variant without the <kbd>Shift</kbd> modifier does not.
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
                  G_STRUCT_OFFSET (BobguiLabelClass, move_cursor),
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
   * BobguiLabel::copy-clipboard:
   * @self: the label which emitted the signal
   *
   * Gets emitted to copy the selection to the clipboard.
   *
   * The `::copy-clipboard` signal is a [keybinding signal](class.SignalAction.html).
   *
   * The default binding for this signal is <kbd>Ctrl</kbd>+<kbd>c</kbd>.
   */
  signals[COPY_CLIPBOARD] =
    g_signal_new (I_("copy-clipboard"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiLabelClass, copy_clipboard),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiLabel::activate-current-link:
   * @self: the label which emitted the signal
   *
   * Gets emitted when the user activates a link in the label.
   *
   * The `::activate-current-link` is a [keybinding signal](class.SignalAction.html).
   *
   * Applications may also emit the signal with g_signal_emit_by_name()
   * if they need to control activation of URIs programmatically.
   *
   * The default bindings for this signal are all forms of the <kbd>Enter</kbd> key.
   */
  signals[ACTIVATE_CURRENT_LINK] =
    g_signal_new_class_handler (I_("activate-current-link"),
                                G_TYPE_FROM_CLASS (gobject_class),
                                G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                                G_CALLBACK (bobgui_label_activate_current_link),
                                NULL, NULL,
                                NULL,
                                G_TYPE_NONE, 0);

  /**
   * BobguiLabel::activate-link:
   * @self: the label which emitted the signal
   * @uri: the URI that is activated
   *
   * Gets emitted to activate a URI.
   *
   * Applications may connect to it to override the default behaviour,
   * which is to call [method@Bobgui.FileLauncher.launch].
   *
   * Returns: true if the link has been activated
   */
  signals[ACTIVATE_LINK] =
    g_signal_new (I_("activate-link"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiLabelClass, activate_link),
                  _bobgui_boolean_handled_accumulator, NULL,
                  _bobgui_marshal_BOOLEAN__STRING,
                  G_TYPE_BOOLEAN, 1, G_TYPE_STRING);
  g_signal_set_va_marshaller (signals[ACTIVATE_LINK],
                              G_TYPE_FROM_CLASS (gobject_class),
                              _bobgui_marshal_BOOLEAN__STRINGv);

  /**
   * BobguiLabel:label:
   *
   * The contents of the label.
   *
   * If the string contains Pango markup (see [func@Pango.parse_markup]),
   * you will have to set the [property@Bobgui.Label:use-markup] property to
   * true in order for the label to display the markup attributes. See also
   * [method@Bobgui.Label.set_markup] for a convenience function that sets both
   * this property and the [property@Bobgui.Label:use-markup] property at the
   * same time.
   *
   * If the string contains underlines acting as mnemonics, you will have to
   * set the [property@Bobgui.Label:use-underline] property to true in order
   * for the label to display them.
   */
  label_props[PROP_LABEL] =
      g_param_spec_string ("label", NULL, NULL,
                           "",
                           BOBGUI_PARAM_READWRITE);

  /**
   * BobguiLabel:attributes:
   *
   * A list of style attributes to apply to the text of the label.
   */
  label_props[PROP_ATTRIBUTES] =
      g_param_spec_boxed ("attributes", NULL, NULL,
                          PANGO_TYPE_ATTR_LIST,
                          BOBGUI_PARAM_READWRITE);

  /**
   * BobguiLabel:use-markup:
   *
   * True if the text of the label includes Pango markup.
   *
   * See [func@Pango.parse_markup].
   */
  label_props[PROP_USE_MARKUP] =
      g_param_spec_boolean ("use-markup", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiLabel:use-underline:
   *
   * True if the text of the label indicates a mnemonic with an `_`
   * before the mnemonic character.
   */
  label_props[PROP_USE_UNDERLINE] =
      g_param_spec_boolean ("use-underline", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiLabel:justify:
   *
   * The alignment of the lines in the text of the label, relative to each other.
   *
   * This does *not* affect the alignment of the label within its allocation.
   * See [property@Bobgui.Label:xalign] for that.
   */
  label_props[PROP_JUSTIFY] =
      g_param_spec_enum ("justify", NULL, NULL,
                         BOBGUI_TYPE_JUSTIFICATION,
                         BOBGUI_JUSTIFY_LEFT,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiLabel:xalign:
   *
   * The horizontal alignment of the label text inside its size allocation.
   *
   * Compare this to [property@Bobgui.Widget:halign], which determines how the
   * labels size allocation is positioned in the space available for the label.
   */
  label_props[PROP_XALIGN] =
      g_param_spec_float ("xalign", NULL, NULL,
                          0.0, 1.0,
                          0.5,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiLabel:yalign:
   *
   * The vertical alignment of the label text inside its size allocation.
   *
   * Compare this to [property@Bobgui.Widget:valign], which determines how the
   * labels size allocation is positioned in the space available for the label.
   */
  label_props[PROP_YALIGN] =
      g_param_spec_float ("yalign", NULL, NULL,
                          0.0, 1.0,
                          0.5,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiLabel:wrap:
   *
   * True if the label text will wrap if it gets too wide.
   */
  label_props[PROP_WRAP] =
      g_param_spec_boolean ("wrap", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiLabel:wrap-mode:
   *
   * Controls how the line wrapping is done.
   *
   * This only affects the formatting if line wrapping is on (see the
   * [property@Bobgui.Label:wrap] property). The default is [enum@Pango.WrapMode.word],
   * which means wrap on word boundaries.
   *
   * For sizing behavior, also consider the [property@Bobgui.Label:natural-wrap-mode]
   * property.
   */
  label_props[PROP_WRAP_MODE] =
      g_param_spec_enum ("wrap-mode", NULL, NULL,
                         PANGO_TYPE_WRAP_MODE,
                         PANGO_WRAP_WORD,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiLabel:natural-wrap-mode:
   *
   * Select the line wrapping for the natural size request.
   *
   * This only affects the natural size requested. For the actual wrapping
   * used, see the [property@Bobgui.Label:wrap-mode] property.
   *
   * The default is [enum@Bobgui.NaturalWrapMode.inherit], which inherits
   * the behavior of the [property@Bobgui.Label:wrap-mode] property.
   *
   * Since: 4.6
   */
  label_props[PROP_NATURAL_WRAP_MODE] =
      g_param_spec_enum ("natural-wrap-mode", NULL, NULL,
                         BOBGUI_TYPE_NATURAL_WRAP_MODE,
                         BOBGUI_NATURAL_WRAP_INHERIT,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiLabel:selectable:
   *
   * Whether the label text can be selected with the mouse.
   */
  label_props[PROP_SELECTABLE] =
      g_param_spec_boolean ("selectable", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiLabel:mnemonic-keyval:
   *
   * The mnemonic accelerator key for the label.
   */
  label_props[PROP_MNEMONIC_KEYVAL] =
      g_param_spec_uint ("mnemonic-keyval", NULL, NULL,
                         0, G_MAXUINT,
                         GDK_KEY_VoidSymbol,
                         BOBGUI_PARAM_READABLE);

  /**
   * BobguiLabel:mnemonic-widget:
   *
   * The widget to be activated when the labels mnemonic key is pressed.
   */
  label_props[PROP_MNEMONIC_WIDGET] =
      g_param_spec_object ("mnemonic-widget", NULL, NULL,
                           BOBGUI_TYPE_WIDGET,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiLabel:ellipsize:
   *
   * The preferred place to ellipsize the string, if the label does
   * not have enough room to display the entire string.
   *
   * Note that setting this property to a value other than
   * [enum.Pango.EllipsizeMode.none] has the side-effect that the label requests
   * only enough space to display the ellipsis "...". In particular, this
   * means that ellipsizing labels do not work well in notebook tabs, unless
   * the [property@Bobgui.NotebookPage:tab-expand] child property is set to true.
   *
   * Other ways to set a label's width are [method@Bobgui.Widget.set_size_request]
   * and [method@Bobgui.Label.set_width_chars].
   */
  label_props[PROP_ELLIPSIZE] =
      g_param_spec_enum ("ellipsize", NULL, NULL,
                         PANGO_TYPE_ELLIPSIZE_MODE,
                         PANGO_ELLIPSIZE_NONE,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiLabel:width-chars:
   *
   * The desired width of the label, in characters.
   *
   * If this property is set to -1, the width will be calculated automatically.
   *
   * See the section on [text layout](class.Label.html#text-layout) for details
   * of how [property@Bobgui.Label:width-chars] and [property@Bobgui.Label:max-width-chars]
   * determine the width of ellipsized and wrapped labels.
   */
  label_props[PROP_WIDTH_CHARS] =
      g_param_spec_int ("width-chars", NULL, NULL,
                        -1, G_MAXINT,
                        -1,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiLabel:single-line-mode:
   *
   * Whether the label is in single line mode.
   *
   * In single line mode, the height of the label does not depend on the
   * actual text, it is always set to ascent + descent of the font. This
   * can be an advantage in situations where resizing the label because
   * of text changes would be distracting, e.g. in a statusbar.
   */
  label_props[PROP_SINGLE_LINE_MODE] =
      g_param_spec_boolean ("single-line-mode", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiLabel:max-width-chars:
   *
   * The desired maximum width of the label, in characters.
   *
   * If this property is set to -1, the width will be calculated automatically.
   *
   * See the section on [text layout](class.Label.html#text-layout) for details
   * of how [property@Bobgui.Label:width-chars] and [property@Bobgui.Label:max-width-chars]
   * determine the width of ellipsized and wrapped labels.
   */
  label_props[PROP_MAX_WIDTH_CHARS] =
      g_param_spec_int ("max-width-chars", NULL, NULL,
                        -1, G_MAXINT,
                        -1,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiLabel:lines:
   *
   * The number of lines to which an ellipsized, wrapping label
   * should display before it gets ellipsized. This both prevents the label
   * from ellipsizing before this many lines are displayed, and limits the
   * height request of the label to this many lines.
   *
   * ::: warning
   *     Setting this property has unintuitive and unfortunate consequences
   *     for the minimum _width_ of the label. Specifically, if the height
   *     of the label is such that it fits a smaller number of lines than
   *     the value of this property, the label can not be ellipsized at all,
   *     which means it must be wide enough to fit all the text fully.
   *
   * This property has no effect if the label is not wrapping or ellipsized.
   *
   * Set this property to -1 if you don't want to limit the number of lines.
   */
  label_props[PROP_LINES] =
      g_param_spec_int ("lines", NULL, NULL,
                        -1, G_MAXINT,
                        -1,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiLabel:extra-menu:
   *
   * A menu model whose contents will be appended to the context menu.
   */
  label_props[PROP_EXTRA_MENU] =
      g_param_spec_object ("extra-menu", NULL, NULL,
                          G_TYPE_MENU_MODEL,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiLabel:tabs:
   *
   * Custom tabs for this label.
   *
   * Since: 4.8
   */
  label_props[PROP_TABS] =
      g_param_spec_boxed ("tabs", NULL, NULL,
                          PANGO_TYPE_TAB_ARRAY,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, label_props);

  /**
   * BobguiLabel|menu.popup:
   *
   * Opens the context menu.
   */
  bobgui_widget_class_install_action (widget_class, "menu.popup", NULL, bobgui_label_popup_menu);

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

  /* Moving the insertion point */
  add_move_binding (widget_class, GDK_KEY_Right, 0,
                    BOBGUI_MOVEMENT_VISUAL_POSITIONS, 1);

  add_move_binding (widget_class, GDK_KEY_Left, 0,
                    BOBGUI_MOVEMENT_VISUAL_POSITIONS, -1);

  add_move_binding (widget_class, GDK_KEY_KP_Right, 0,
                    BOBGUI_MOVEMENT_VISUAL_POSITIONS, 1);

  add_move_binding (widget_class, GDK_KEY_KP_Left, 0,
                      BOBGUI_MOVEMENT_VISUAL_POSITIONS, -1);

  add_move_binding (widget_class, GDK_KEY_f, GDK_CONTROL_MASK,
                    BOBGUI_MOVEMENT_LOGICAL_POSITIONS, 1);

  add_move_binding (widget_class, GDK_KEY_b, GDK_CONTROL_MASK,
                    BOBGUI_MOVEMENT_LOGICAL_POSITIONS, -1);

  add_move_binding (widget_class, GDK_KEY_Right, GDK_CONTROL_MASK,
                      BOBGUI_MOVEMENT_WORDS, 1);

  add_move_binding (widget_class, GDK_KEY_Left, GDK_CONTROL_MASK,
                    BOBGUI_MOVEMENT_WORDS, -1);

  add_move_binding (widget_class, GDK_KEY_KP_Right, GDK_CONTROL_MASK,
                    BOBGUI_MOVEMENT_WORDS, 1);

  add_move_binding (widget_class, GDK_KEY_KP_Left, GDK_CONTROL_MASK,
                    BOBGUI_MOVEMENT_WORDS, -1);

  /* select all */
#ifdef __APPLE__
  bobgui_widget_class_add_binding (widget_class,
                                GDK_KEY_a, GDK_META_MASK,
                                (BobguiShortcutFunc) bobgui_label_select_all,
                                NULL);
#else
  bobgui_widget_class_add_binding (widget_class,
                                GDK_KEY_a, GDK_CONTROL_MASK,
                                (BobguiShortcutFunc) bobgui_label_select_all,
                                NULL);
  bobgui_widget_class_add_binding (widget_class,
                                GDK_KEY_slash, GDK_CONTROL_MASK,
                                (BobguiShortcutFunc) bobgui_label_select_all,
                                NULL);
#endif

  /* unselect all */
#ifdef __APPLE__
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_a, GDK_SHIFT_MASK | GDK_META_MASK,
                                       "move-cursor",
                                       "(iib)", BOBGUI_MOVEMENT_PARAGRAPH_ENDS, 0, FALSE);
#else
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_a, GDK_SHIFT_MASK | GDK_CONTROL_MASK,
                                       "move-cursor",
                                       "(iib)", BOBGUI_MOVEMENT_PARAGRAPH_ENDS, 0, FALSE);

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_backslash, GDK_CONTROL_MASK,
                                       "move-cursor",
                                       "(iib)", BOBGUI_MOVEMENT_PARAGRAPH_ENDS, 0, FALSE);
#endif

  add_move_binding (widget_class, GDK_KEY_f, GDK_ALT_MASK,
                    BOBGUI_MOVEMENT_WORDS, 1);

  add_move_binding (widget_class, GDK_KEY_b, GDK_ALT_MASK,
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

  /* copy */
#ifdef __APPLE__
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_c, GDK_META_MASK,
                                       "copy-clipboard",
                                       NULL);
#else
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_c, GDK_CONTROL_MASK,
                                       "copy-clipboard",
                                       NULL);
#endif

  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_Return, 0,
                                       "activate-current-link",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_ISO_Enter, 0,
                                       "activate-current-link",
                                       NULL);
  bobgui_widget_class_add_binding_signal (widget_class,
                                       GDK_KEY_KP_Enter, 0,
                                       "activate-current-link",
                                       NULL);

  bobgui_widget_class_set_css_name (widget_class, I_("label"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_LABEL);

  quark_mnemonics_visible_connected = g_quark_from_static_string ("bobgui-label-mnemonics-visible-connected");

  /**
   * BobguiLabel|clipboard.cut:
   *
   * Doesn't do anything, since text in labels can't be deleted.
   */
  bobgui_widget_class_install_action (widget_class, "clipboard.cut", NULL,
                                   bobgui_label_nop);

  /**
   * BobguiLabel|clipboard.copy:
   *
   * Copies the text to the clipboard.
   */
  bobgui_widget_class_install_action (widget_class, "clipboard.copy", NULL,
                                   bobgui_label_activate_clipboard_copy);

  /**
   * BobguiLabel|clipboard.paste:
   *
   * Doesn't do anything, since text in labels can't be edited.
   */
  bobgui_widget_class_install_action (widget_class, "clipboard.paste", NULL,
                                   bobgui_label_nop);

  /**
   * BobguiLabel|selection.delete:
   *
   * Doesn't do anything, since text in labels can't be deleted.
   */
  bobgui_widget_class_install_action (widget_class, "selection.delete", NULL,
                                   bobgui_label_nop);

  /**
   * BobguiLabel|selection.select-all:
   *
   * Selects all of the text, if the label allows selection.
   */
  bobgui_widget_class_install_action (widget_class, "selection.select-all", NULL,
                                   bobgui_label_activate_selection_select_all);

  /**
   * BobguiLabel|link.open:
   *
   * Opens the link, when activated on a link inside the label.
   */
  bobgui_widget_class_install_action (widget_class, "link.open", NULL,
                                   bobgui_label_activate_link_open);

  /**
   * BobguiLabel|link.copy:
   *
   * Copies the link to the clipboard, when activated on a link
   * inside the label.
   */
  bobgui_widget_class_install_action (widget_class, "link.copy", NULL,
                                   bobgui_label_activate_link_copy);
}

/**
 * bobgui_label_new:
 * @str: (nullable): the text of the label
 *
 * Creates a new label with the given text inside it.
 *
 * You can pass `NULL` to get an empty label widget.
 *
 * Returns: the new label
 **/
BobguiWidget*
bobgui_label_new (const char *str)
{
  BobguiLabel *self;

  self = g_object_new (BOBGUI_TYPE_LABEL, NULL);

  if (str && *str)
    bobgui_label_set_text (self, str);

  return BOBGUI_WIDGET (self);
}

/**
 * bobgui_label_new_with_mnemonic:
 * @str: (nullable): the text of the label, with an underscore in front of the
 *   mnemonic character
 *
 * Creates a new label with the given text inside it, and a mnemonic.
 *
 * If characters in @str are preceded by an underscore, they are
 * underlined. If you need a literal underscore character in a label, use
 * '__' (two underscores). The first underlined character represents a
 * keyboard accelerator called a mnemonic. The mnemonic key can be used
 * to activate another widget, chosen automatically, or explicitly using
 * [method@Bobgui.Label.set_mnemonic_widget].
 *
 * If [method@Bobgui.Label.set_mnemonic_widget] is not called, then the first
 * activatable ancestor of the label will be chosen as the mnemonic
 * widget. For instance, if the label is inside a button or menu item,
 * the button or menu item will automatically become the mnemonic widget
 * and be activated by the mnemonic.
 *
 * Returns: the new label
 **/
BobguiWidget*
bobgui_label_new_with_mnemonic (const char *str)
{
  BobguiLabel *self;

  self = g_object_new (BOBGUI_TYPE_LABEL, NULL);

  if (str && *str)
    bobgui_label_set_text_with_mnemonic (self, str);

  return BOBGUI_WIDGET (self);
}

static void
_bobgui_label_mnemonics_visible_apply_recursively (BobguiWidget *widget,
                                                gboolean   visible)
{
  if (BOBGUI_IS_LABEL (widget))
    {
      BobguiLabel *self = BOBGUI_LABEL (widget);

      if (self->mnemonics_visible != visible)
        {
          self->mnemonics_visible = visible;
          bobgui_label_recalculate (self);
        }
    }
  else
    {
      BobguiWidget *child;

      for (child = bobgui_widget_get_first_child (widget);
           child;
           child = bobgui_widget_get_next_sibling (child))
        {
          if (BOBGUI_IS_NATIVE (child))
            continue;

          _bobgui_label_mnemonics_visible_apply_recursively (child, visible);
        }
    }
}

static void
label_mnemonics_visible_changed (BobguiWidget  *widget,
                                 GParamSpec *pspec,
                                 gpointer    data)
{
  gboolean visible;

  g_object_get (widget, "mnemonics-visible", &visible, NULL);
  _bobgui_label_mnemonics_visible_apply_recursively (widget, visible);
}

static void
bobgui_label_setup_mnemonic (BobguiLabel *self)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  BobguiShortcut *shortcut;
  BobguiNative *native;
  gboolean connected;
  gboolean mnemonics_visible;

  if (self->mnemonic_keyval == GDK_KEY_VoidSymbol)
    {
      if (self->mnemonic_controller)
        {
          bobgui_widget_remove_controller (widget, self->mnemonic_controller);
          self->mnemonic_controller = NULL;
        }
      return;
    }

  if (self->mnemonic_controller == NULL)
    {
      self->mnemonic_controller = bobgui_shortcut_controller_new ();
      bobgui_event_controller_set_propagation_phase (self->mnemonic_controller, BOBGUI_PHASE_CAPTURE);
      bobgui_shortcut_controller_set_scope (BOBGUI_SHORTCUT_CONTROLLER (self->mnemonic_controller), BOBGUI_SHORTCUT_SCOPE_MANAGED);
      shortcut = bobgui_shortcut_new (bobgui_mnemonic_trigger_new (self->mnemonic_keyval),
                                   g_object_ref (bobgui_mnemonic_action_get ()));
      bobgui_shortcut_controller_add_shortcut (BOBGUI_SHORTCUT_CONTROLLER (self->mnemonic_controller), shortcut);
      bobgui_widget_add_controller (BOBGUI_WIDGET (self), self->mnemonic_controller);
    }
  else
    {
      shortcut = g_list_model_get_item (G_LIST_MODEL (self->mnemonic_controller), 0);
      bobgui_shortcut_set_trigger (shortcut, bobgui_mnemonic_trigger_new (self->mnemonic_keyval));
      g_object_unref (shortcut);
    }

  /* Connect to notify::mnemonics-visible of the root */
  native = bobgui_widget_get_native (BOBGUI_WIDGET (self));
  if (!BOBGUI_IS_WINDOW (native) && !BOBGUI_IS_POPOVER (native))
    return;

  /* always set up this widgets initial value */
  g_object_get (native, "mnemonics-visible", &mnemonics_visible, NULL);
  if (self->mnemonics_visible != mnemonics_visible)
    {
      self->mnemonics_visible = mnemonics_visible;
      bobgui_label_recalculate (self);
    }

  connected = GPOINTER_TO_INT (g_object_get_qdata (G_OBJECT (native),
                                                   quark_mnemonics_visible_connected));

  if (!connected)
    {
      g_signal_connect (native,
                        "notify::mnemonics-visible",
                        G_CALLBACK (label_mnemonics_visible_changed),
                        self);
      g_object_set_qdata (G_OBJECT (native),
                          quark_mnemonics_visible_connected,
                          GINT_TO_POINTER (1));
    }
}

static void
label_mnemonic_widget_weak_notify (gpointer      data,
                                   GObject      *where_the_object_was)
{
  BobguiLabel *self = data;

  self->mnemonic_widget = NULL;
  g_object_notify_by_pspec (G_OBJECT (self), label_props[PROP_MNEMONIC_WIDGET]);
}

/**
 * bobgui_label_set_mnemonic_widget:
 * @self: a label
 * @widget: (nullable): the target widget
 *
 * Associate the label with its mnemonic target.
 *
 * If the label has been set so that it has a mnemonic key (using
 * i.e. [method@Bobgui.Label.set_markup_with_mnemonic],
 * [method@Bobgui.Label.set_text_with_mnemonic],
 * [ctor@Bobgui.Label.new_with_mnemonic]
 * or the [property@Bobgui.Label:use_underline] property) the label can
 * be associated with a widget that is the target of the mnemonic.
 * When the label is inside a widget (like a [class@Bobgui.Button] or a
 * [class@Bobgui.Notebook] tab) it is automatically associated with the
 * correct widget, but sometimes (i.e. when the target is a [class@Bobgui.Entry]
 * next to the label) you need to set it explicitly using this function.
 *
 * The target widget will be accelerated by emitting the
 * [signal@Bobgui.Widget::mnemonic-activate] signal on it. The default handler
 * for this signal will activate the widget if there are no mnemonic
 * collisions and toggle focus between the colliding widgets otherwise.
 */
void
bobgui_label_set_mnemonic_widget (BobguiLabel  *self,
                               BobguiWidget *widget)
{
  g_return_if_fail (BOBGUI_IS_LABEL (self));
  g_return_if_fail (widget == NULL || BOBGUI_IS_WIDGET (widget));

  if (self->mnemonic_widget == widget)
    return;

  if (self->mnemonic_widget)
    {
      bobgui_widget_remove_mnemonic_label (self->mnemonic_widget, BOBGUI_WIDGET (self));
      g_object_weak_unref (G_OBJECT (self->mnemonic_widget),
                           label_mnemonic_widget_weak_notify,
                           self);
    }
  self->mnemonic_widget = widget;
  if (self->mnemonic_widget)
    {
      g_object_weak_ref (G_OBJECT (self->mnemonic_widget),
                         label_mnemonic_widget_weak_notify,
                         self);
      bobgui_widget_add_mnemonic_label (self->mnemonic_widget, BOBGUI_WIDGET (self));
    }

  g_object_notify_by_pspec (G_OBJECT (self), label_props[PROP_MNEMONIC_WIDGET]);
}

/**
 * bobgui_label_get_mnemonic_widget:
 * @self: a label
 *
 * Retrieves the mnemonic target of this label.
 *
 * See [method@Bobgui.Label.set_mnemonic_widget].
 *
 * Returns: (nullable) (transfer none): the target of the label’s mnemonic,
 *   or `NULL` if none has been set and the default algorithm will be used.
 **/
BobguiWidget *
bobgui_label_get_mnemonic_widget (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), NULL);

  return self->mnemonic_widget;
}

/**
 * bobgui_label_get_mnemonic_keyval:
 * @self: a label
 *
 * Return the mnemonic accelerator.
 *
 * If the label has been set so that it has a mnemonic key this function
 * returns the keyval used for the mnemonic accelerator. If there is no
 * mnemonic set up it returns `GDK_KEY_VoidSymbol`.
 *
 * Returns: GDK keyval usable for accelerators, or `GDK_KEY_VoidSymbol`
 **/
guint
bobgui_label_get_mnemonic_keyval (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), GDK_KEY_VoidSymbol);

  return self->mnemonic_keyval;
}

static void
bobgui_label_set_text_internal (BobguiLabel *self,
                             char     *str)
{
  BobguiAccessibleRole role;

  if (g_strcmp0 (self->text, str) == 0)
    {
      g_free (str);
      return;
    }

  g_free (self->text);
  self->text = str;

  role = bobgui_accessible_get_accessible_role (BOBGUI_ACCESSIBLE (self));

  if (bobgui_accessible_role_get_naming (role) != BOBGUI_ACCESSIBLE_NAME_PROHIBITED)
    {
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                      BOBGUI_ACCESSIBLE_PROPERTY_LABEL,
                                      self->text,
                                      -1);
    }

  bobgui_label_select_region_index (self, 0, 0);
}

static gboolean
bobgui_label_set_label_internal (BobguiLabel   *self,
                              const char *str)
{
  if (g_strcmp0 (str, self->label) == 0)
    return FALSE;

  g_free (self->label);
  self->label = g_strdup (str ? str : "");

  g_object_notify_by_pspec (G_OBJECT (self), label_props[PROP_LABEL]);

  return TRUE;
}

static gboolean
bobgui_label_set_use_markup_internal (BobguiLabel *self,
                                   gboolean  val)
{
  if (self->use_markup != val)
    {
      self->use_markup = val;

      g_object_notify_by_pspec (G_OBJECT (self), label_props[PROP_USE_MARKUP]);

      return TRUE;
    }

  return FALSE;
}

static gboolean
bobgui_label_set_use_underline_internal (BobguiLabel *self,
                                      gboolean  val)
{
  if (self->use_underline != val)
    {
      self->use_underline = val;

      g_object_notify_by_pspec (G_OBJECT (self), label_props[PROP_USE_UNDERLINE]);

      return TRUE;
    }

  return FALSE;
}

/* Calculates text, attrs and mnemonic_keyval from
 * label, use_underline and use_markup
 */
static void
bobgui_label_recalculate (BobguiLabel *self)
{
  guint keyval = self->mnemonic_keyval;

  bobgui_label_clear_links (self);
  bobgui_label_clear_layout (self);
  bobgui_label_clear_select_info (self);

  if (self->use_markup)
    {
      bobgui_label_set_markup_internal (self, self->label, self->use_underline);
    }
  else if (self->use_underline)
    {
      char *text;

      text = g_markup_escape_text (self->label, -1);
      bobgui_label_set_markup_internal (self, text, TRUE);
      g_free (text);
    }
  else
    {
      g_clear_pointer (&self->markup_attrs, pango_attr_list_unref);

      bobgui_label_set_text_internal (self, g_strdup (self->label));
    }

  if (!self->use_underline)
    self->mnemonic_keyval = GDK_KEY_VoidSymbol;

  if (keyval != self->mnemonic_keyval)
    {
      bobgui_label_setup_mnemonic (self);
      g_object_notify_by_pspec (G_OBJECT (self), label_props[PROP_MNEMONIC_KEYVAL]);
    }

  bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
}

/**
 * bobgui_label_set_text:
 * @self: a label
 * @str: the text to show in @self
 *
 * Sets the text for the label.
 *
 * It overwrites any text that was there before and clears any
 * previously set mnemonic accelerators, and sets the
 * [property@Bobgui.Label:use-underline] and
 * [property@Bobgui.Label:use-markup] properties to false.
 *
 * Also see [method@Bobgui.Label.set_markup].
 */
void
bobgui_label_set_text (BobguiLabel    *self,
                    const char *str)
{
  gboolean changed;

  g_return_if_fail (BOBGUI_IS_LABEL (self));

  g_object_freeze_notify (G_OBJECT (self));

  changed = bobgui_label_set_label_internal (self, str);
  changed = bobgui_label_set_use_markup_internal (self, FALSE) || changed;
  changed = bobgui_label_set_use_underline_internal (self, FALSE) || changed;

  if (changed)
    bobgui_label_recalculate (self);

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_label_set_attributes:
 * @self: a label
 * @attrs: (nullable): a list of style attributes
 *
 * Apply attributes to the label text.
 *
 * The attributes set with this function will be applied and merged with
 * any other attributes previously effected by way of the
 * [property@Bobgui.Label:use-underline] or [property@Bobgui.Label:use-markup]
 * properties
 *
 * While it is not recommended to mix markup strings with manually set
 * attributes, if you must; know that the attributes will be applied
 * to the label after the markup string is parsed.
 */
void
bobgui_label_set_attributes (BobguiLabel         *self,
                          PangoAttrList    *attrs)
{
  g_return_if_fail (BOBGUI_IS_LABEL (self));

  if (!attrs && !self->attrs)
    return;

  if (attrs)
    pango_attr_list_ref (attrs);

  if (self->attrs)
    pango_attr_list_unref (self->attrs);
  self->attrs = attrs;

  g_object_notify_by_pspec (G_OBJECT (self), label_props[PROP_ATTRIBUTES]);

  bobgui_label_clear_layout (self);
  bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
}

/**
 * bobgui_label_get_attributes:
 * @self: a label
 *
 * Gets the label's attribute list.
 *
 * This is the [struct@Pango.AttrList] that was set on the label using
 * [method@Bobgui.Label.set_attributes], if any. This function does not
 * reflect attributes that come from the label's markup (see
 * [method@Bobgui.Label.set_markup]). If you want to get the effective
 * attributes for the label, use
 * `pango_layout_get_attributes (bobgui_label_get_layout (self))`.
 *
 * Returns: (nullable) (transfer none): the attribute list
 */
PangoAttrList *
bobgui_label_get_attributes (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), NULL);

  return self->attrs;
}

/**
 * bobgui_label_set_label:
 * @self: a label
 * @str: the new text to set for the label
 *
 * Sets the text of the label.
 *
 * The label is interpreted as including embedded underlines and/or Pango
 * markup depending on the values of the [property@Bobgui.Label:use-underline]
 * and [property@Bobgui.Label:use-markup] properties.
 */
void
bobgui_label_set_label (BobguiLabel    *self,
                     const char *str)
{
  g_return_if_fail (BOBGUI_IS_LABEL (self));

  g_object_freeze_notify (G_OBJECT (self));

  if (bobgui_label_set_label_internal (self, str))
    bobgui_label_recalculate (self);

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_label_get_label:
 * @self: a label
 *
 * Fetches the text from a label.
 *
 * The returned text includes any embedded underlines indicating
 * mnemonics and Pango markup. (See [method@Bobgui.Label.get_text]).
 *
 * Returns: the text of the label widget
 */
const char *
bobgui_label_get_label (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), NULL);

  return self->label;
}

typedef struct
{
  BobguiLabel *label;
  GArray *links;
  GString *new_str;
  gsize text_len;
  gboolean strip_ulines;
  GString *text_data;
  gunichar accel_key;
} UriParserData;

static char *
strip_ulines (const char *text,
              guint      *accel_key)
{
  char *new_text;
  const char *p;
  char *q;
  gboolean after_uline = FALSE;

  new_text = g_malloc (strlen (text) + 1);

  q = new_text;
  for (p = text; *p; p++)
    {
      if (*p == '_' && !after_uline)
        {
          after_uline = TRUE;
          continue;
        }

      *q = *p;
      if (after_uline && *p != '_' && *accel_key == 0)
        *accel_key = g_utf8_get_char (p);

      q++;
      after_uline = FALSE;
    }

  if (after_uline)
    {
      *q = '_';
      q++;
    }

  *q = '\0';

  return new_text;
}

static void
finish_text (UriParserData *pdata)
{
  if (pdata->text_data->len > 0)
    {
      char *text;
      gsize text_len;
      char *newtext;

      if (pdata->strip_ulines && strchr (pdata->text_data->str, '_'))
        {
          text = strip_ulines (pdata->text_data->str, &pdata->accel_key);
          text_len = strlen (text);
        }
      else
        {
          text = pdata->text_data->str;
          text_len = pdata->text_data->len;
        }

      newtext = g_markup_escape_text (text, text_len);
      g_string_append (pdata->new_str, newtext);
      pdata->text_len += text_len;
      g_free (newtext);

      if (text != pdata->text_data->str)
        g_free (text);

      g_string_set_size (pdata->text_data, 0);
    }
}

static void
link_style_changed_cb (BobguiCssNode        *node,
                       BobguiCssStyleChange *change,
                       BobguiLabel          *self)
{
  if (bobgui_css_style_change_affects (change,
                                    BOBGUI_CSS_AFFECTS_CONTENT |
                                    BOBGUI_CSS_AFFECTS_TEXT_ATTRS))
    {
      bobgui_label_ensure_layout (self);
      bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
    }
}

static void
selection_style_changed_cb (BobguiCssNode        *node,
                            BobguiCssStyleChange *change,
                            BobguiLabel          *self)
{
  if (bobgui_css_style_change_affects (change, BOBGUI_CSS_AFFECTS_REDRAW))
    bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
}

static void
clear_label_link (gpointer data)
{
  BobguiLabelLink *link = data;

  g_clear_object (&link->accessible);
}

static void
start_element_handler (GMarkupParseContext  *context,
                       const char           *element_name,
                       const char          **attribute_names,
                       const char          **attribute_values,
                       gpointer              user_data,
                       GError              **error)
{
  UriParserData *pdata = user_data;
  BobguiLabel *self = pdata->label;

  finish_text (pdata);

  if (strcmp (element_name, "a") == 0)
    {
      BobguiLabelLink link;
      const char *uri = NULL;
      const char *title = NULL;
      const char *class = NULL;
      gboolean visited = FALSE;
      int line_number;
      int char_number;
      int i;
      BobguiCssNode *widget_node;
      BobguiStateFlags state;

      g_markup_parse_context_get_position (context, &line_number, &char_number);

      for (i = 0; attribute_names[i] != NULL; i++)
        {
          const char *attr = attribute_names[i];

          if (strcmp (attr, "href") == 0)
            uri = attribute_values[i];
          else if (strcmp (attr, "title") == 0)
            title = attribute_values[i];
          else if (strcmp (attr, "class") == 0)
            class = attribute_values[i];
          else
            {
              g_set_error (error,
                           G_MARKUP_ERROR,
                           G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
                           "Attribute '%s' is not allowed on the <a> tag "
                           "on line %d char %d",
                            attr, line_number, char_number);
              return;
            }
        }

      if (uri == NULL)
        {
          g_set_error (error,
                       G_MARKUP_ERROR,
                       G_MARKUP_ERROR_INVALID_CONTENT,
                       "Attribute 'href' was missing on the <a> tag "
                       "on line %d char %d",
                       line_number, char_number);
          return;
        }

      visited = FALSE;
      if (self->select_info)
        {
          for (i = 0; i < self->select_info->n_links; i++)
            {
              const BobguiLabelLink *l = &self->select_info->links[i];

              if (strcmp (uri, l->uri) == 0)
                {
                  visited = l->visited;
                  break;
                }
            }
        }

      if (!pdata->links)
        {
          pdata->links = g_array_new (FALSE, TRUE, sizeof (BobguiLabelLink));
          g_array_set_clear_func (pdata->links, clear_label_link);
        }

      link.uri = g_strdup (uri);
      link.title = g_strdup (title);
      link.accessible = NULL;

      widget_node = bobgui_widget_get_css_node (BOBGUI_WIDGET (pdata->label));
      link.cssnode = bobgui_css_node_new ();

      bobgui_css_node_set_name (link.cssnode, g_quark_from_static_string ("link"));
      bobgui_css_node_set_parent (link.cssnode, widget_node);
      if (class)
        bobgui_css_node_add_class (link.cssnode, g_quark_from_string (class));
      g_signal_connect (link.cssnode, "style-changed", G_CALLBACK (link_style_changed_cb), self);

      state = bobgui_css_node_get_state (widget_node);
      if (visited)
        state |= BOBGUI_STATE_FLAG_VISITED;
      else
        state |= BOBGUI_STATE_FLAG_LINK;
      bobgui_css_node_set_state (link.cssnode, state);
      g_object_unref (link.cssnode);

      link.visited = visited;
      link.start = pdata->text_len;
      g_array_append_val (pdata->links, link);
    }
  else
    {
      int i;

      g_string_append_c (pdata->new_str, '<');
      g_string_append (pdata->new_str, element_name);

      for (i = 0; attribute_names[i] != NULL; i++)
        {
          const char *attr  = attribute_names[i];
          const char *value = attribute_values[i];
          char *newvalue;

          newvalue = g_markup_escape_text (value, -1);

          g_string_append_c (pdata->new_str, ' ');
          g_string_append (pdata->new_str, attr);
          g_string_append (pdata->new_str, "=\"");
          g_string_append (pdata->new_str, newvalue);
          g_string_append_c (pdata->new_str, '\"');

          g_free (newvalue);
        }
      g_string_append_c (pdata->new_str, '>');
    }
}

static void
end_element_handler (GMarkupParseContext  *context,
                     const char           *element_name,
                     gpointer              user_data,
                     GError              **error)
{
  UriParserData *pdata = user_data;

  finish_text (pdata);

  if (!strcmp (element_name, "a"))
    {
      BobguiLabelLink *link = &g_array_index (pdata->links, BobguiLabelLink, pdata->links->len - 1);

      link->end = pdata->text_len;
    }
  else
    {
      g_string_append (pdata->new_str, "</");
      g_string_append (pdata->new_str, element_name);
      g_string_append_c (pdata->new_str, '>');
    }
}

static void
text_handler (GMarkupParseContext  *context,
              const char           *text,
              gsize                 text_len,
              gpointer              user_data,
              GError              **error)
{
  UriParserData *pdata = user_data;

  g_string_append_len (pdata->text_data, text, text_len);
}

static const GMarkupParser markup_parser =
{
  start_element_handler,
  end_element_handler,
  text_handler,
  NULL,
  NULL
};

static gboolean
xml_isspace (char c)
{
  return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

static gboolean
parse_uri_markup (BobguiLabel      *self,
                  const char    *str,
                  gboolean       strip_ulines,
                  gunichar      *accel_key,
                  char         **new_str,
                  BobguiLabelLink **links,
                  guint         *out_n_links,
                  GError       **error)
{
  GMarkupParseContext *context;
  const char *p, *end;
  gsize length;
  UriParserData pdata;

  length = strlen (str);
  p = str;
  end = str + length;

  pdata.label = self;
  pdata.links = NULL;
  pdata.new_str = g_string_sized_new (length);
  pdata.text_len = 0;
  pdata.strip_ulines = strip_ulines;
  pdata.text_data = g_string_new ("");
  pdata.accel_key = 0;

  while (p != end && xml_isspace (*p))
    p++;

  context = g_markup_parse_context_new (&markup_parser, 0, &pdata, NULL);

  if (end - p >= 8 && strncmp (p, "<markup>", 8) == 0)
    {
      if (!g_markup_parse_context_parse (context, str, length, error))
        goto failed;
    }
  else
    {
      if (!g_markup_parse_context_parse (context, "<markup>", 8, error))
        goto failed;

      if (!g_markup_parse_context_parse (context, str, length, error))
        goto failed;

      if (!g_markup_parse_context_parse (context, "</markup>", 9, error))
        goto failed;
    }

  if (!g_markup_parse_context_end_parse (context, error))
    goto failed;

  g_markup_parse_context_free (context);

  g_string_free (pdata.text_data, TRUE);

  *new_str = g_string_free (pdata.new_str, FALSE);

  if (pdata.links)
    {
      *out_n_links = pdata.links->len;
      *links = (BobguiLabelLink *)g_array_free (pdata.links, FALSE);
    }
  else
    {
      *links = NULL;
    }

  if (accel_key)
    *accel_key = pdata.accel_key;

  return TRUE;

failed:
  g_markup_parse_context_free (context);
  g_string_free (pdata.new_str, TRUE);

  if (pdata.links)
    g_array_free (pdata.links, TRUE);

  return FALSE;
}

static void
bobgui_label_ensure_has_tooltip (BobguiLabel *self)
{
  guint i;
  gboolean has_tooltip = bobgui_widget_get_has_tooltip(BOBGUI_WIDGET(self));

  if (has_tooltip) {
    return;
  }

  for (i = 0; i < self->select_info->n_links; i++)
    {
      const BobguiLabelLink *link = &self->select_info->links[i];

      if (link->title)
        {
          has_tooltip = TRUE;
          break;
        }
    }

  bobgui_widget_set_has_tooltip (BOBGUI_WIDGET (self), has_tooltip);
}

static void
bobgui_label_set_markup_internal (BobguiLabel   *self,
                               const char *str,
                               gboolean    with_uline)
{
  char *text = NULL;
  GError *error = NULL;
  PangoAttrList *attrs = NULL;
  char *str_for_display = NULL;
  BobguiLabelLink *links = NULL;
  guint n_links = 0;
  gunichar accel_keyval = 0;
  gboolean do_mnemonics;

  do_mnemonics = self->mnemonics_visible &&
                 bobgui_widget_is_sensitive (BOBGUI_WIDGET (self)) &&
                 (!self->mnemonic_widget || bobgui_widget_is_sensitive (self->mnemonic_widget));

  if (!parse_uri_markup (self, str,
                         with_uline && !do_mnemonics,
                         &accel_keyval,
                         &str_for_display,
                         &links, &n_links,
                         &error))
    goto error_set;

  if (links)
    {
      bobgui_label_ensure_select_info (self);
      self->select_info->links = g_steal_pointer (&links);
      self->select_info->n_links = n_links;
      bobgui_label_ensure_has_tooltip (self);
      bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "link");
    }

  if (!pango_parse_markup (str_for_display, -1,
                           with_uline && do_mnemonics ? '_' : 0,
                           &attrs, &text,
                           with_uline && do_mnemonics ? &accel_keyval : NULL,
                           &error))
    goto error_set;

  g_free (str_for_display);

  if (text)
    bobgui_label_set_text_internal (self, text);

  g_clear_pointer (&self->markup_attrs, pango_attr_list_unref);
  self->markup_attrs = attrs;

  self->mnemonic_keyval = accel_keyval ? accel_keyval : GDK_KEY_VoidSymbol;

  return;

error_set:
  g_warning ("Failed to set text '%s' from markup due to error parsing markup: %s",
             str, error->message);
  g_error_free (error);

}

/**
 * bobgui_label_set_markup:
 * @self: a label
 * @str: the markup string
 *
 * Sets the labels text and attributes from markup.
 *
 * The string must be marked up with Pango markup
 * (see [func@Pango.parse_markup]).
 *
 * If @str is external data, you may need to escape it
 * with [func@GLib.markup_escape_text] or [func@GLib.markup_printf_escaped]:
 *
 * ```c
 * BobguiWidget *self = bobgui_label_new (NULL);
 * const char *str = "...";
 * const char *format = "<span style=\"italic\">\%s</span>";
 * char *markup;
 *
 * markup = g_markup_printf_escaped (format, str);
 * bobgui_label_set_markup (BOBGUI_LABEL (self), markup);
 * g_free (markup);
 * ```
 *
 * This function sets the [property@Bobgui.Label:use-markup] property
 * to true.
 *
 * Also see [method@Bobgui.Label.set_text].
 */
void
bobgui_label_set_markup (BobguiLabel    *self,
                      const char *str)
{
  gboolean changed;

  g_return_if_fail (BOBGUI_IS_LABEL (self));

  g_object_freeze_notify (G_OBJECT (self));

  changed = bobgui_label_set_label_internal (self, str);
  changed = bobgui_label_set_use_markup_internal (self, TRUE) || changed;
  changed = bobgui_label_set_use_underline_internal (self, FALSE) || changed;

  if (changed)
    bobgui_label_recalculate (self);

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_label_set_markup_with_mnemonic:
 * @self: a label
 * @str: the markup string
 *
 * Sets the labels text, attributes and mnemonic from markup.
 *
 * Parses @str which is marked up with Pango markup (see [func@Pango.parse_markup]),
 * setting the label’s text and attribute list based on the parse results.
 * If characters in @str are preceded by an underscore, they are underlined
 * indicating that they represent a keyboard accelerator called a mnemonic.
 *
 * The mnemonic key can be used to activate another widget, chosen
 * automatically, or explicitly using [method@Bobgui.Label.set_mnemonic_widget].
 */
void
bobgui_label_set_markup_with_mnemonic (BobguiLabel    *self,
                                    const char *str)
{
  gboolean changed;

  g_return_if_fail (BOBGUI_IS_LABEL (self));

  g_object_freeze_notify (G_OBJECT (self));

  changed = bobgui_label_set_label_internal (self, str);
  changed = bobgui_label_set_use_markup_internal (self, TRUE) || changed;
  changed = bobgui_label_set_use_underline_internal (self, TRUE) || changed;

  if (changed)
    bobgui_label_recalculate (self);

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_label_get_text:
 * @self: a label
 *
 * Gets the text of the label.
 *
 * The returned text is as it appears on screen. This does not include
 * any embedded underlines indicating mnemonics or Pango markup. (See
 * [method@Bobgui.Label.get_label])
 *
 * Returns: the text in the label widget
 **/
const char *
bobgui_label_get_text (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), NULL);

  return self->text;
}

/**
 * bobgui_label_set_justify:
 * @self: a label
 * @jtype: the new justification
 *
 * Sets the alignment of lines in the label relative to each other.
 *
 * This function has no effect on labels containing only a single line.
 *
 * [enum@Bobgui.Justification.left] is the default value when the widget
 * is first created with [ctor@Bobgui.Label.new].
 *
 * If you instead want to set the alignment of the label as a whole,
 * use [method@Bobgui.Widget.set_halign] instead.
 */
void
bobgui_label_set_justify (BobguiLabel        *self,
                       BobguiJustification jtype)
{
  g_return_if_fail (BOBGUI_IS_LABEL (self));
  g_return_if_fail (jtype >= BOBGUI_JUSTIFY_LEFT && jtype <= BOBGUI_JUSTIFY_FILL);

  if ((BobguiJustification) self->jtype != jtype)
    {
      self->jtype = jtype;

      /* No real need to be this drastic, but easier than duplicating the code */
      bobgui_label_clear_layout (self);

      g_object_notify_by_pspec (G_OBJECT (self), label_props[PROP_JUSTIFY]);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
    }
}

/**
 * bobgui_label_get_justify:
 * @self: a label
 *
 * Returns the justification of the label.
 *
 * See [method@Bobgui.Label.set_justify].
 *
 * Returns: the justification value
 **/
BobguiJustification
bobgui_label_get_justify (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), 0);

  return self->jtype;
}

/**
 * bobgui_label_set_ellipsize:
 * @self: a label
 * @mode: the ellipsization mode
 *
 * Sets the mode used to ellipsize the text.
 *
 * The text will be ellipsized if there is not
 * enough space to render the entire string.
 */
void
bobgui_label_set_ellipsize (BobguiLabel          *self,
                         PangoEllipsizeMode mode)
{
  g_return_if_fail (BOBGUI_IS_LABEL (self));
  g_return_if_fail (mode >= PANGO_ELLIPSIZE_NONE && mode <= PANGO_ELLIPSIZE_END);

  if ((PangoEllipsizeMode) self->ellipsize != mode)
    {
      self->ellipsize = mode;

      /* No real need to be this drastic, but easier than duplicating the code */
      bobgui_label_clear_layout (self);

      g_object_notify_by_pspec (G_OBJECT (self), label_props[PROP_ELLIPSIZE]);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
    }
}

/**
 * bobgui_label_get_ellipsize:
 * @self: a label
 *
 * Returns the ellipsization mode of the label.
 *
 * See [method@Bobgui.Label.set_ellipsize].
 *
 * Returns: the ellipsization mode
 **/
PangoEllipsizeMode
bobgui_label_get_ellipsize (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), PANGO_ELLIPSIZE_NONE);

  return self->ellipsize;
}

/**
 * bobgui_label_set_width_chars:
 * @self: a label
 * @n_chars: the new desired width, in characters.
 *
 * Sets the desired width in characters of the label.
 */
void
bobgui_label_set_width_chars (BobguiLabel *self,
                           int       n_chars)
{
  g_return_if_fail (BOBGUI_IS_LABEL (self));

  if (self->width_chars != n_chars)
    {
      self->width_chars = n_chars;
      g_object_notify_by_pspec (G_OBJECT (self), label_props[PROP_WIDTH_CHARS]);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
    }
}

/**
 * bobgui_label_get_width_chars:
 * @self: a label
 *
 * Retrieves the desired width of the label in characters.
 *
 * See [method@Bobgui.Label.set_width_chars].
 *
 * Returns: the desired width of the label, in characters
 */
int
bobgui_label_get_width_chars (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), -1);

  return self->width_chars;
}

/**
 * bobgui_label_set_max_width_chars:
 * @self: a label
 * @n_chars: the new maximum width, in characters.
 *
 * Sets the maximum width of the label in characters.
 */
void
bobgui_label_set_max_width_chars (BobguiLabel *self,
                               int       n_chars)
{
  g_return_if_fail (BOBGUI_IS_LABEL (self));

  if (self->max_width_chars != n_chars)
    {
      self->max_width_chars = n_chars;

      g_object_notify_by_pspec (G_OBJECT (self), label_props[PROP_MAX_WIDTH_CHARS]);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
    }
}

/**
 * bobgui_label_get_max_width_chars:
 * @self: a label
 *
 * Retrieves the maximum width of the label in characters.
 *
 * See [method@Bobgui.Label.set_width_chars].
 *
 * Returns: the maximum width of the label, in characters
 **/
int
bobgui_label_get_max_width_chars (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), -1);

  return self->max_width_chars;
}

/**
 * bobgui_label_set_wrap:
 * @self: a label
 * @wrap: whether to wrap lines
 *
 * Toggles line wrapping within the label.
 *
 * True makes it break lines if text exceeds the widget’s size.
 * false lets the text get cut off by the edge of the widget if
 * it exceeds the widget size.
 *
 * Note that setting line wrapping to true does not make the label
 * wrap at its parent widget’s width, because BOBGUI widgets conceptually
 * can’t make their requisition depend on the parent  widget’s size.
 * For a label that wraps at a specific position, set the label’s width
 * using [method@Bobgui.Widget.set_size_request].
 */
void
bobgui_label_set_wrap (BobguiLabel *self,
                    gboolean  wrap)
{
  g_return_if_fail (BOBGUI_IS_LABEL (self));

  wrap = wrap != FALSE;

  if (self->wrap != wrap)
    {
      self->wrap = wrap;

      bobgui_label_clear_layout (self);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
      g_object_notify_by_pspec (G_OBJECT (self), label_props[PROP_WRAP]);
    }
}

/**
 * bobgui_label_get_wrap:
 * @self: a label
 *
 * Returns whether lines in the label are automatically wrapped.
 *
 * See [method@Bobgui.Label.set_wrap].
 *
 * Returns: true if the lines of the label are automatically wrapped
 */
gboolean
bobgui_label_get_wrap (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), FALSE);

  return self->wrap;
}

/**
 * bobgui_label_set_wrap_mode:
 * @self: a label
 * @wrap_mode: the line wrapping mode
 *
 * Controls how line wrapping is done.
 *
 * This only affects the label if line wrapping is on. (See
 * [method@Bobgui.Label.set_wrap])
 *
 * The default is [enum@Pango.WrapMode.word], which means
 * wrap on word boundaries.
 *
 * For sizing behavior, also consider the
 * [property@Bobgui.Label:natural-wrap-mode] property.
 */
void
bobgui_label_set_wrap_mode (BobguiLabel *self,
                         PangoWrapMode wrap_mode)
{
  g_return_if_fail (BOBGUI_IS_LABEL (self));

  if (self->wrap_mode != wrap_mode)
    {
      self->wrap_mode = wrap_mode;
      g_object_notify_by_pspec (G_OBJECT (self), label_props[PROP_WRAP_MODE]);

      bobgui_label_clear_layout (self);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
    }
}

/**
 * bobgui_label_get_wrap_mode:
 * @self: a label
 *
 * Returns line wrap mode used by the label.
 *
 * See [method@Bobgui.Label.set_wrap_mode].
 *
 * Returns: the line wrap mode
 */
PangoWrapMode
bobgui_label_get_wrap_mode (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), PANGO_WRAP_WORD);

  return self->wrap_mode;
}

/**
 * bobgui_label_set_natural_wrap_mode:
 * @self: a label
 * @wrap_mode: the line wrapping mode
 *
 * Selects the line wrapping for the natural size request.
 *
 * This only affects the natural size requested, for the actual wrapping used,
 * see the [property@Bobgui.Label:wrap-mode] property.
 *
 * Since: 4.6
 */
void
bobgui_label_set_natural_wrap_mode (BobguiLabel           *self,
                                 BobguiNaturalWrapMode  wrap_mode)
{
  g_return_if_fail (BOBGUI_IS_LABEL (self));

  if (self->natural_wrap_mode != wrap_mode)
    {
      self->natural_wrap_mode = wrap_mode;
      g_object_notify_by_pspec (G_OBJECT (self), label_props[PROP_NATURAL_WRAP_MODE]);

      bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
    }
}

/**
 * bobgui_label_get_natural_wrap_mode:
 * @self: a label
 *
 * Returns natural line wrap mode used by the label.
 *
 * See [method@Bobgui.Label.set_natural_wrap_mode].
 *
 * Returns: the natural line wrap mode
 *
 * Since: 4.6
 */
BobguiNaturalWrapMode
bobgui_label_get_natural_wrap_mode (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), BOBGUI_NATURAL_WRAP_INHERIT);

  return self->natural_wrap_mode;
}

static void
bobgui_label_clear_layout (BobguiLabel *self)
{
  g_clear_object (&self->layout);
}

static void
bobgui_label_ensure_layout (BobguiLabel *self)
{
  PangoAlignment align;
  gboolean rtl;

  if (self->layout)
    return;

  rtl = _bobgui_widget_get_direction (BOBGUI_WIDGET (self)) == BOBGUI_TEXT_DIR_RTL;
  self->layout = bobgui_widget_create_pango_layout (BOBGUI_WIDGET (self), self->text);

  bobgui_label_update_layout_attributes (self, NULL);

  switch (self->jtype)
    {
    case BOBGUI_JUSTIFY_LEFT:
      align = rtl ? PANGO_ALIGN_RIGHT : PANGO_ALIGN_LEFT;
      break;
    case BOBGUI_JUSTIFY_RIGHT:
      align = rtl ? PANGO_ALIGN_LEFT : PANGO_ALIGN_RIGHT;
      break;
    case BOBGUI_JUSTIFY_CENTER:
      align = PANGO_ALIGN_CENTER;
      break;
    case BOBGUI_JUSTIFY_FILL:
      align = rtl ? PANGO_ALIGN_RIGHT : PANGO_ALIGN_LEFT;
      pango_layout_set_justify (self->layout, TRUE);
      break;
    default:
      g_assert_not_reached();
    }

  pango_layout_set_alignment (self->layout, align);
  pango_layout_set_ellipsize (self->layout, self->ellipsize);
  pango_layout_set_wrap (self->layout, self->wrap_mode);
  pango_layout_set_single_paragraph_mode (self->layout, self->single_line_mode);
  if (self->lines > 0)
    pango_layout_set_height (self->layout, - self->lines);

  if (self->ellipsize || self->wrap)
    pango_layout_set_width (self->layout, bobgui_widget_get_width (BOBGUI_WIDGET (self)) * PANGO_SCALE);

  pango_layout_set_tabs (self->layout, self->tabs);
}

/**
 * bobgui_label_set_text_with_mnemonic:
 * @self: a label
 * @str: the text
 *
 * Sets the text for the label, with mnemonics.
 *
 * If characters in @str are preceded by an underscore, they are underlined
 * indicating that they represent a keyboard accelerator called a mnemonic.
 * The mnemonic key can be used to activate another widget, chosen
 * automatically, or explicitly using [method@Bobgui.Label.set_mnemonic_widget].
 */
void
bobgui_label_set_text_with_mnemonic (BobguiLabel    *self,
                                  const char *str)
{
  gboolean changed;

  g_return_if_fail (BOBGUI_IS_LABEL (self));
  g_return_if_fail (str != NULL);

  g_object_freeze_notify (G_OBJECT (self));

  changed = bobgui_label_set_label_internal (self, str);
  changed = bobgui_label_set_use_markup_internal (self, FALSE) || changed;
  changed = bobgui_label_set_use_underline_internal (self, TRUE) || changed;

  if (changed)
    bobgui_label_recalculate (self);

  g_object_thaw_notify (G_OBJECT (self));
}

static int
bobgui_label_move_forward_word (BobguiLabel *self,
                             int       start)
{
  int new_pos = g_utf8_pointer_to_offset (self->text, self->text + start);
  int length;

  length = g_utf8_strlen (self->text, -1);
  if (new_pos < length)
    {
      const PangoLogAttr *log_attrs;
      int n_attrs;

      bobgui_label_ensure_layout (self);

      log_attrs = pango_layout_get_log_attrs_readonly (self->layout, &n_attrs);

      /* Find the next word end */
      new_pos++;
      while (new_pos < n_attrs && !log_attrs[new_pos].is_word_end)
        new_pos++;
    }

  return g_utf8_offset_to_pointer (self->text, new_pos) - self->text;
}

static int
bobgui_label_move_backward_word (BobguiLabel *self,
                              int       start)
{
  int new_pos = g_utf8_pointer_to_offset (self->text, self->text + start);

  if (new_pos > 0)
    {
      const PangoLogAttr *log_attrs;
      int n_attrs;

      bobgui_label_ensure_layout (self);

      log_attrs = pango_layout_get_log_attrs_readonly (self->layout, &n_attrs);

      new_pos -= 1;

      /* Find the previous word beginning */
      while (new_pos > 0 && !log_attrs[new_pos].is_word_start)
        new_pos--;
    }

  return g_utf8_offset_to_pointer (self->text, new_pos) - self->text;
}

static void
bobgui_label_select_word (BobguiLabel *self)
{
  int min, max;

  int start_index = bobgui_label_move_backward_word (self, self->select_info->selection_end);
  int end_index = bobgui_label_move_forward_word (self, self->select_info->selection_end);

  min = MIN (self->select_info->selection_anchor,
             self->select_info->selection_end);
  max = MAX (self->select_info->selection_anchor,
             self->select_info->selection_end);

  min = MIN (min, start_index);
  max = MAX (max, end_index);

  bobgui_label_select_region_index (self, min, max);
}

static void
bobgui_label_click_gesture_pressed (BobguiGestureClick *gesture,
                                 int              n_press,
                                 double           widget_x,
                                 double           widget_y,
                                 BobguiLabel        *self)
{
  BobguiLabelSelectionInfo *info = self->select_info;
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  GdkEventSequence *sequence;
  GdkEvent *event;
  guint button;

  button = bobgui_gesture_single_get_current_button (BOBGUI_GESTURE_SINGLE (gesture));
  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  event = bobgui_gesture_get_last_event (BOBGUI_GESTURE (gesture), sequence);
  bobgui_label_update_active_link (widget, widget_x, widget_y);

  bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);

  if (info->active_link)
    {
      if (gdk_event_triggers_context_menu (event))
        {
          info->link_clicked = TRUE;
          update_link_state (self);
          bobgui_label_do_popup (self, widget_x, widget_y);
          return;
        }
      else if (button == GDK_BUTTON_PRIMARY)
        {
          info->link_clicked = TRUE;
          update_link_state (self);
          bobgui_widget_queue_draw (widget);
          if (!info->selectable)
            return;
        }
    }

  if (!info->selectable)
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_DENIED);
      return;
    }

  info->in_drag = FALSE;
  info->select_words = FALSE;

  if (gdk_event_triggers_context_menu (event))
    bobgui_label_do_popup (self, widget_x, widget_y);
  else if (button == GDK_BUTTON_PRIMARY)
    {
      if (!bobgui_widget_has_focus (widget))
        {
          self->in_click = TRUE;
          bobgui_widget_grab_focus (widget);
          self->in_click = FALSE;
        }

      if (n_press == 3)
        bobgui_label_select_region_index (self, 0, strlen (self->text));
      else if (n_press == 2)
        {
          info->select_words = TRUE;
          bobgui_label_select_word (self);
        }
    }
  else
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_DENIED);
      return;
    }

  if (n_press >= 3)
    bobgui_event_controller_reset (BOBGUI_EVENT_CONTROLLER (gesture));
}

static void
bobgui_label_click_gesture_released (BobguiGestureClick *gesture,
                                  int              n_press,
                                  double           x,
                                  double           y,
                                  BobguiLabel        *self)
{
  BobguiLabelSelectionInfo *info = self->select_info;
  GdkEventSequence *sequence;
  int index;

  if (info == NULL)
    return;

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));

  if (!bobgui_gesture_handles_sequence (BOBGUI_GESTURE (gesture), sequence))
    return;

  if (n_press != 1)
    return;

  if (info->in_drag)
    {
      info->in_drag = FALSE;
      get_layout_index (self, x, y, &index);
      bobgui_label_select_region_index (self, index, index);
    }
  else if (info->active_link &&
           info->selection_anchor == info->selection_end &&
           info->link_clicked)
    {
      emit_activate_link (self, info->active_link);
      info->link_clicked = FALSE;
    }
}

static GdkPaintable *
get_selection_paintable (BobguiLabel *self)
{
  if ((self->select_info->selection_anchor !=
       self->select_info->selection_end) &&
      self->text)
    {
      int start, end;
      int len;

      start = MIN (self->select_info->selection_anchor,
                   self->select_info->selection_end);
      end = MAX (self->select_info->selection_anchor,
                 self->select_info->selection_end);

      len = strlen (self->text);

      if (end > len)
        end = len;

      if (start > len)
        start = len;

      return bobgui_text_util_create_drag_icon (BOBGUI_WIDGET (self), self->text + start, end - start);
    }

  return NULL;
}

static void
bobgui_label_drag_gesture_begin (BobguiGestureDrag *gesture,
                              double          start_x,
                              double          start_y,
                              BobguiLabel       *self)
{
  BobguiLabelSelectionInfo *info = self->select_info;
  GdkModifierType state_mask;
  GdkEventSequence *sequence;
  GdkEvent *event;
  int min, max, index;

  if (!info || !info->selectable)
    {
      bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_DENIED);
      return;
    }

  get_layout_index (self, start_x, start_y, &index);
  min = MIN (info->selection_anchor, info->selection_end);
  max = MAX (info->selection_anchor, info->selection_end);

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  event = bobgui_gesture_get_last_event (BOBGUI_GESTURE (gesture), sequence);
  state_mask = gdk_event_get_modifier_state (event);

  if ((info->selection_anchor != info->selection_end) &&
      ((state_mask & GDK_SHIFT_MASK) != 0))
    {
      if (index > min && index < max)
        {
          /* truncate selection, but keep it as big as possible */
          if (index - min > max - index)
            max = index;
          else
            min = index;
        }
      else
        {
          /* extend (same as motion) */
          min = MIN (min, index);
          max = MAX (max, index);
        }

      /* ensure the anchor is opposite index */
      if (index == min)
        {
          int tmp = min;
          min = max;
          max = tmp;
        }

      bobgui_label_select_region_index (self, min, max);
    }
  else
    {
      if (min < max && min <= index && index <= max)
        {
          if (!info->select_words)
            info->in_drag = TRUE;
          info->drag_start_x = start_x;
          info->drag_start_y = start_y;
        }
      else
        /* start a replacement */
        bobgui_label_select_region_index (self, index, index);
    }
}

static void
bobgui_label_drag_gesture_update (BobguiGestureDrag *gesture,
                               double          offset_x,
                               double          offset_y,
                               BobguiLabel       *self)
{
  BobguiLabelSelectionInfo *info = self->select_info;
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  GdkEventSequence *sequence;
  double x, y;
  int index;

  if (info == NULL || !info->selectable)
    return;

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  bobgui_gesture_get_point (BOBGUI_GESTURE (gesture), sequence, &x, &y);

  if (info->in_drag)
    {
      if (bobgui_drag_check_threshold_double (widget, info->drag_start_x, info->drag_start_y, x, y))
        {
          GdkDrag *drag;
          GdkSurface *surface;
          GdkDevice *device;

          surface = bobgui_native_get_surface (bobgui_widget_get_native (widget));
          device = bobgui_gesture_get_device (BOBGUI_GESTURE (gesture));

          drag = gdk_drag_begin (surface,
                                 device,
                                 info->provider,
                                 GDK_ACTION_COPY,
                                 info->drag_start_x,
                                 info->drag_start_y);

          bobgui_drag_icon_set_from_paintable (drag, get_selection_paintable (self), 0, 0);

          g_object_unref (drag);
          info->in_drag = FALSE;
        }
    }
  else
    {
      get_layout_index (self, x, y, &index);

      if (index != info->selection_anchor)
        bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);

      if (info->select_words)
        {
          int min, max;
          int old_min, old_max;
          int anchor, end;

          min = bobgui_label_move_backward_word (self, index);
          max = bobgui_label_move_forward_word (self, index);

          anchor = info->selection_anchor;
          end = info->selection_end;

          old_min = MIN (anchor, end);
          old_max = MAX (anchor, end);

          if (min < old_min)
            {
              anchor = min;
              end = old_max;
            }
          else if (old_max < max)
            {
              anchor = max;
              end = old_min;
            }
          else if (anchor == old_min)
            {
              if (anchor != min)
                anchor = max;
            }
          else
            {
              if (anchor != max)
                anchor = min;
            }

          bobgui_label_select_region_index (self, anchor, end);
        }
      else
        bobgui_label_select_region_index (self, info->selection_anchor, index);
    }
}

static void
bobgui_label_update_actions (BobguiLabel *self)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  gboolean has_selection;
  BobguiLabelLink *link;

  if (self->select_info)
    {
      has_selection = self->select_info->selection_anchor != self->select_info->selection_end;
      link = self->select_info->active_link;
    }
  else
    {
      has_selection = FALSE;
      link = bobgui_label_get_focus_link (self, NULL);
    }

  bobgui_widget_action_set_enabled (widget, "clipboard.cut", FALSE);
  bobgui_widget_action_set_enabled (widget, "clipboard.copy", has_selection);
  bobgui_widget_action_set_enabled (widget, "clipboard.paste", FALSE);
  bobgui_widget_action_set_enabled (widget, "selection.select-all",
                                 bobgui_label_get_selectable (self));
  bobgui_widget_action_set_enabled (widget, "selection.delete", FALSE);
  bobgui_widget_action_set_enabled (widget, "link.open", !has_selection && link);
  bobgui_widget_action_set_enabled (widget, "link.copy", !has_selection && link);
}

static void
bobgui_label_update_active_link (BobguiWidget *widget,
                              double     x,
                              double     y)
{
  BobguiLabel *self = BOBGUI_LABEL (widget);
  BobguiLabelSelectionInfo *info = self->select_info;
  int index;

  if (info == NULL)
    return;

  if (info->links && !info->in_drag)
    {
      BobguiLabelLink *link = NULL;
      gboolean found = FALSE;

      if (info->selection_anchor == info->selection_end)
        {
          if (get_layout_index (self, x, y, &index))
            {
              const int link_index = _bobgui_label_get_link_at (self, index);

              if (link_index != -1)
                {
                  link = &info->links[link_index];

                  if (!range_is_in_ellipsis (self, link->start, link->end))
                    found = TRUE;
                }
            }
        }

      if (found)
        {
          if (info->active_link != link)
            {
              info->link_clicked = FALSE;
              info->active_link = link;
              update_link_state (self);
              bobgui_label_update_cursor (self);
              bobgui_widget_queue_draw (widget);
            }
        }
      else
        {
          if (info->active_link != NULL)
            {
              info->link_clicked = FALSE;
              info->active_link = NULL;
              update_link_state (self);
              bobgui_label_update_cursor (self);
              bobgui_widget_queue_draw (widget);
            }
        }

      bobgui_label_update_actions (self);
    }
}

static void
bobgui_label_motion (BobguiEventControllerMotion *controller,
                  double                    x,
                  double                    y,
                  gpointer                  data)
{
  bobgui_label_update_active_link (BOBGUI_WIDGET (data), x, y);
}

static void
bobgui_label_leave (BobguiEventControllerMotion *controller,
                 gpointer                  data)
{
  BobguiLabel *self = BOBGUI_LABEL (data);

  if (self->select_info)
    {
      self->select_info->active_link = NULL;
      bobgui_label_update_cursor (self);
      bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
    }
}

#define BOBGUI_TYPE_LABEL_CONTENT            (bobgui_label_content_get_type ())
#define BOBGUI_LABEL_CONTENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_LABEL_CONTENT, BobguiLabelContent))
#define BOBGUI_IS_LABEL_CONTENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_LABEL_CONTENT))
#define BOBGUI_LABEL_CONTENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_LABEL_CONTENT, BobguiLabelContentClass))
#define BOBGUI_IS_LABEL_CONTENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_LABEL_CONTENT))
#define BOBGUI_LABEL_CONTENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_LABEL_CONTENT, BobguiLabelContentClass))

typedef struct _BobguiLabelContent BobguiLabelContent;
typedef struct _BobguiLabelContentClass BobguiLabelContentClass;

struct _BobguiLabelContent
{
  GdkContentProvider parent;

  BobguiLabel *label;
};

struct _BobguiLabelContentClass
{
  GdkContentProviderClass parent_class;
};

GType bobgui_label_content_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE (BobguiLabelContent, bobgui_label_content, GDK_TYPE_CONTENT_PROVIDER)

static GdkContentFormats *
bobgui_label_content_ref_formats (GdkContentProvider *provider)
{
  BobguiLabelContent *content = BOBGUI_LABEL_CONTENT (provider);

  if (content->label)
    return gdk_content_formats_new_for_gtype (G_TYPE_STRING);
  else
    return gdk_content_formats_new (NULL, 0);
}

static gboolean
bobgui_label_content_get_value (GdkContentProvider  *provider,
                             GValue              *value,
                             GError             **error)
{
  BobguiLabelContent *content = BOBGUI_LABEL_CONTENT (provider);

  if (G_VALUE_HOLDS (value, G_TYPE_STRING) &&
      content->label != NULL)
    {
      BobguiLabel *self = content->label;

      if (self->select_info &&
          (self->select_info->selection_anchor !=
           self->select_info->selection_end) &&
          self->text)
        {
          int start, end;
          int len;
          char *str;

          start = MIN (self->select_info->selection_anchor,
                       self->select_info->selection_end);
          end = MAX (self->select_info->selection_anchor,
                     self->select_info->selection_end);

          len = strlen (self->text);

          if (end > len)
            end = len;

          if (start > len)
            start = len;

          str = g_strndup (self->text + start, end - start);
          g_value_take_string (value, str);
          return TRUE;
        }
    }

  return GDK_CONTENT_PROVIDER_CLASS (bobgui_label_content_parent_class)->get_value (provider, value, error);
}

static void
bobgui_label_content_detach (GdkContentProvider *provider,
                          GdkClipboard       *clipboard)
{
  BobguiLabelContent *content = BOBGUI_LABEL_CONTENT (provider);
  BobguiLabel *self = content->label;

  if (self == NULL || self->select_info == NULL)
    return;

  self->select_info->selection_anchor = self->select_info->selection_end;

  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
}

static void
bobgui_label_content_class_init (BobguiLabelContentClass *class)
{
  GdkContentProviderClass *provider_class = GDK_CONTENT_PROVIDER_CLASS (class);

  provider_class->ref_formats = bobgui_label_content_ref_formats;
  provider_class->get_value = bobgui_label_content_get_value;
  provider_class->detach_clipboard = bobgui_label_content_detach;
}

static void
bobgui_label_content_init (BobguiLabelContent *content)
{
}

static void
focus_change (BobguiEventControllerFocus *controller,
              BobguiLabel                *self)
{
  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
}

static void
bobgui_label_ensure_select_info (BobguiLabel *self)
{
  if (self->select_info == NULL)
    {
      self->select_info = g_new0 (BobguiLabelSelectionInfo, 1);

      bobgui_widget_set_focusable (BOBGUI_WIDGET (self), TRUE);

      self->select_info->drag_gesture = bobgui_gesture_drag_new ();
      g_signal_connect (self->select_info->drag_gesture, "drag-begin",
                        G_CALLBACK (bobgui_label_drag_gesture_begin), self);
      g_signal_connect (self->select_info->drag_gesture, "drag-update",
                        G_CALLBACK (bobgui_label_drag_gesture_update), self);
      bobgui_gesture_single_set_exclusive (BOBGUI_GESTURE_SINGLE (self->select_info->drag_gesture), TRUE);
      bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (self->select_info->drag_gesture));

      self->select_info->click_gesture = bobgui_gesture_click_new ();
      g_signal_connect (self->select_info->click_gesture, "pressed",
                        G_CALLBACK (bobgui_label_click_gesture_pressed), self);
      g_signal_connect (self->select_info->click_gesture, "released",
                        G_CALLBACK (bobgui_label_click_gesture_released), self);
      bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (self->select_info->click_gesture), 0);
      bobgui_gesture_single_set_exclusive (BOBGUI_GESTURE_SINGLE (self->select_info->click_gesture), TRUE);
      bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (self->select_info->click_gesture));

      self->select_info->motion_controller = bobgui_event_controller_motion_new ();
      g_signal_connect (self->select_info->motion_controller, "motion",
                        G_CALLBACK (bobgui_label_motion), self);
      g_signal_connect (self->select_info->motion_controller, "leave",
                        G_CALLBACK (bobgui_label_leave), self);
      bobgui_widget_add_controller (BOBGUI_WIDGET (self), self->select_info->motion_controller);

      self->select_info->focus_controller = bobgui_event_controller_focus_new ();
      g_signal_connect (self->select_info->focus_controller, "enter",
                        G_CALLBACK (focus_change), self);
      g_signal_connect (self->select_info->focus_controller, "leave",
                        G_CALLBACK (focus_change), self);
      bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (self->select_info->focus_controller));

      self->select_info->provider = g_object_new (BOBGUI_TYPE_LABEL_CONTENT, NULL);
      BOBGUI_LABEL_CONTENT (self->select_info->provider)->label = self;

      bobgui_label_update_cursor (self);
    }
}

static void
bobgui_label_clear_select_info (BobguiLabel *self)
{
  if (self->select_info == NULL)
    return;

  if (!self->select_info->selectable && !self->select_info->links)
    {
      bobgui_widget_remove_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (self->select_info->drag_gesture));
      bobgui_widget_remove_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (self->select_info->click_gesture));
      bobgui_widget_remove_controller (BOBGUI_WIDGET (self), self->select_info->motion_controller);
      bobgui_widget_remove_controller (BOBGUI_WIDGET (self), self->select_info->focus_controller);
      BOBGUI_LABEL_CONTENT (self->select_info->provider)->label = NULL;
      g_clear_object (&self->select_info->provider);

      g_free (self->select_info);
      self->select_info = NULL;

      bobgui_widget_set_cursor (BOBGUI_WIDGET (self), NULL);

      bobgui_widget_set_focusable (BOBGUI_WIDGET (self), FALSE);
    }
}

static void
bobgui_label_clear_provider_info (BobguiLabel *self)
{
  if (self->select_info == NULL)
    return;

  BOBGUI_LABEL_CONTENT (self->select_info->provider)->label = NULL;
}

/**
 * bobgui_label_set_selectable:
 * @self: a label
 * @setting: true to allow selecting text in the label
 *
 * Makes text in the label selectable.
 *
 * Selectable labels allow the user to select text from the label,
 * for copy-and-paste.
 */
void
bobgui_label_set_selectable (BobguiLabel *self,
                          gboolean  setting)
{
  gboolean old_setting;

  g_return_if_fail (BOBGUI_IS_LABEL (self));

  setting = setting != FALSE;
  old_setting = self->select_info && self->select_info->selectable;

  if (setting)
    {
      bobgui_label_ensure_select_info (self);
      self->select_info->selectable = TRUE;
      bobgui_label_update_cursor (self);

      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                      BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP, TRUE,
                                      -1);
    }
  else
    {
      if (old_setting)
        {
          /* unselect, to give up the selection */
          bobgui_label_select_region (self, 0, 0);

          self->select_info->selectable = FALSE;
          bobgui_label_clear_select_info (self);
        }

      bobgui_accessible_reset_property (BOBGUI_ACCESSIBLE (self), BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP);
    }

  if (setting != old_setting)
    {
      g_object_freeze_notify (G_OBJECT (self));
      g_object_notify_by_pspec (G_OBJECT (self), label_props[PROP_SELECTABLE]);
      g_object_thaw_notify (G_OBJECT (self));
      bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
    }
}

/**
 * bobgui_label_get_selectable:
 * @self: a label
 *
 * Returns whether the label is selectable.
 *
 * Returns: true if the user can copy text from the label
 */
gboolean
bobgui_label_get_selectable (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), FALSE);

  return self->select_info && self->select_info->selectable;
}

static void
bobgui_label_select_region_index (BobguiLabel *self,
                               int       anchor_index,
                               int       end_index)
{
  g_return_if_fail (BOBGUI_IS_LABEL (self));

  if (self->select_info && self->select_info->selectable)
    {
      GdkClipboard *clipboard;
      int s, e;

      /* Ensure that we treat an ellipsized region like a single
       * character with respect to selection.
       */
      if (anchor_index < end_index)
        {
          if (range_is_in_ellipsis_full (self, anchor_index, anchor_index + 1, &s, &e))
            {
              if (self->select_info->selection_anchor == s)
                anchor_index = e;
              else
                anchor_index = s;
            }
          if (range_is_in_ellipsis_full (self, end_index - 1, end_index, &s, &e))
            {
              if (self->select_info->selection_end == e)
                end_index = s;
              else
                end_index = e;
            }
        }
      else if (end_index < anchor_index)
        {
          if (range_is_in_ellipsis_full (self, end_index, end_index + 1, &s, &e))
            {
              if (self->select_info->selection_end == s)
                end_index = e;
              else
                end_index = s;
            }
          if (range_is_in_ellipsis_full (self, anchor_index - 1, anchor_index, &s, &e))
            {
              if (self->select_info->selection_anchor == e)
                anchor_index = s;
              else
                anchor_index = e;
            }
        }
      else
        {
          if (range_is_in_ellipsis_full (self, anchor_index, anchor_index, &s, &e))
            {
              if (self->select_info->selection_anchor == s)
                anchor_index = e;
              else if (self->select_info->selection_anchor == e)
                anchor_index = s;
              else if (anchor_index - s < e - anchor_index)
                anchor_index = s;
              else
                anchor_index = e;
              end_index = anchor_index;
            }
        }

      if (self->select_info->selection_anchor == anchor_index &&
          self->select_info->selection_end == end_index)
        return;

      g_object_freeze_notify (G_OBJECT (self));

      self->select_info->selection_anchor = anchor_index;
      self->select_info->selection_end = end_index;

      clipboard = bobgui_widget_get_primary_clipboard (BOBGUI_WIDGET (self));

      if (anchor_index != end_index)
        {
          gdk_content_provider_content_changed (self->select_info->provider);
          gdk_clipboard_set_content (clipboard, self->select_info->provider);

          if (!self->select_info->selection_node)
            {
              BobguiCssNode *widget_node;

              widget_node = bobgui_widget_get_css_node (BOBGUI_WIDGET (self));
              self->select_info->selection_node = bobgui_css_node_new ();
              bobgui_css_node_set_name (self->select_info->selection_node, g_quark_from_static_string ("selection"));
              bobgui_css_node_set_parent (self->select_info->selection_node, widget_node);
              bobgui_css_node_set_state (self->select_info->selection_node, bobgui_css_node_get_state (widget_node));
              g_signal_connect (self->select_info->selection_node, "style-changed",
                                G_CALLBACK (selection_style_changed_cb), self);
              g_object_unref (self->select_info->selection_node);
            }
        }
      else
        {
          if (gdk_clipboard_get_content (clipboard) == self->select_info->provider)
            gdk_clipboard_set_content (clipboard, NULL);

          if (self->select_info->selection_node)
            {
              bobgui_css_node_set_parent (self->select_info->selection_node, NULL);
              self->select_info->selection_node = NULL;
            }
        }


      bobgui_label_update_actions (self);

      bobgui_accessible_text_update_caret_position (BOBGUI_ACCESSIBLE_TEXT (self));
      bobgui_accessible_text_update_selection_bound (BOBGUI_ACCESSIBLE_TEXT (self));

      bobgui_widget_queue_draw (BOBGUI_WIDGET (self));

      g_object_thaw_notify (G_OBJECT (self));
    }
}

/**
 * bobgui_label_select_region:
 * @self: a label
 * @start_offset: start offset, in characters
 * @end_offset: end offset, in characters
 *
 * Selects a range of characters in the label, if the label is selectable.
 *
 * See [method@Bobgui.Label.set_selectable]. If the label is not selectable,
 * this function has no effect. If @start_offset or
 * @end_offset are -1, then the end of the label will be substituted.
 */
void
bobgui_label_select_region  (BobguiLabel *self,
                          int       start_offset,
                          int       end_offset)
{
  g_return_if_fail (BOBGUI_IS_LABEL (self));

  if (self->text && self->select_info)
    {
      if (start_offset < 0)
        start_offset = g_utf8_strlen (self->text, -1);

      if (end_offset < 0)
        end_offset = g_utf8_strlen (self->text, -1);

      bobgui_label_select_region_index (self,
                                     g_utf8_offset_to_pointer (self->text, start_offset) - self->text,
                                     g_utf8_offset_to_pointer (self->text, end_offset) - self->text);
    }
}

/**
 * bobgui_label_get_selection_bounds:
 * @self: a label
 * @start: (out) (optional): return location for start of selection
 * @end: (out) (optional): return location for end of selection
 *
 * Gets the selected range of characters in the label.
 *
 * The returned @start and @end positions are in characters.
 *
 * Returns: true if selection is non-empty
 **/
gboolean
bobgui_label_get_selection_bounds (BobguiLabel  *self,
                                int       *start,
                                int       *end)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), FALSE);

  if (self->select_info == NULL)
    {
      /* not a selectable label */
      if (start)
        *start = 0;
      if (end)
        *end = 0;

      return FALSE;
    }
  else
    {
      int start_index, end_index;
      int start_offset, end_offset;
      int len;

      start_index = MIN (self->select_info->selection_anchor,
                   self->select_info->selection_end);
      end_index = MAX (self->select_info->selection_anchor,
                 self->select_info->selection_end);

      len = strlen (self->text);

      if (end_index > len)
        end_index = len;

      if (start_index > len)
        start_index = len;

      start_offset = g_utf8_strlen (self->text, start_index);
      end_offset = g_utf8_strlen (self->text, end_index);

      if (start_offset > end_offset)
        {
          int tmp = start_offset;
          start_offset = end_offset;
          end_offset = tmp;
        }

      if (start)
        *start = start_offset;

      if (end)
        *end = end_offset;

      return start_offset != end_offset;
    }
}


/**
 * bobgui_label_get_layout:
 * @self: a label
 *
 * Gets the Pango layout used to display the label.
 *
 * The layout is useful to e.g. convert text positions to pixel
 * positions, in combination with [method@Bobgui.Label.get_layout_offsets].
 * The returned layout is owned by the @label so need not be
 * freed by the caller. The @label is free to recreate its layout
 * at any time, so it should be considered read-only.
 *
 * Returns: (transfer none): the [class@Pango.Layout] for this label
 */
PangoLayout*
bobgui_label_get_layout (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), NULL);

  bobgui_label_ensure_layout (self);

  return self->layout;
}

/**
 * bobgui_label_get_layout_offsets:
 * @self: a label
 * @x: (out) (optional): location to store X offset of layout
 * @y: (out) (optional): location to store Y offset of layout
 *
 * Obtains the coordinates where the label will draw its Pango layout.
 *
 * The coordinates are useful to convert mouse events into coordinates
 * inside the [class@Pango.Layout], e.g. to take some action if some part
 * of the label is clicked. Remember when using the [class@Pango.Layout]
 * functions you need to convert to and from pixels using `PANGO_PIXELS()`
 * or [const@Pango.SCALE].
 */
void
bobgui_label_get_layout_offsets (BobguiLabel *self,
                              int      *x,
                              int      *y)
{
  float local_x, local_y;
  g_return_if_fail (BOBGUI_IS_LABEL (self));

  bobgui_label_get_layout_location (self, &local_x, &local_y);

  if (x)
    *x = (int) local_x;

  if (y)
    *y = (int) local_y;
}

/**
 * bobgui_label_set_use_markup:
 * @self: a label
 * @setting: true if the label’s text should be parsed for markup.
 *
 * Sets whether the text of the label contains markup.
 *
 * See [method@Bobgui.Label.set_markup].
 */
void
bobgui_label_set_use_markup (BobguiLabel *self,
                          gboolean  setting)
{
  g_return_if_fail (BOBGUI_IS_LABEL (self));

  g_object_freeze_notify (G_OBJECT (self));

  if (bobgui_label_set_use_markup_internal (self, !!setting))
    bobgui_label_recalculate (self);

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_label_get_use_markup:
 * @self: a label
 *
 * Returns whether the label’s text is interpreted as Pango markup.
 *
 * See [method@Bobgui.Label.set_use_markup].
 *
 * Returns: true if the label’s text will be parsed for markup
 */
gboolean
bobgui_label_get_use_markup (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), FALSE);

  return self->use_markup;
}

/**
 * bobgui_label_set_use_underline:
 * @self: a label
 * @setting: true if underlines in the text indicate mnemonics
 *
 * Sets whether underlines in the text indicate mnemonics.
 */
void
bobgui_label_set_use_underline (BobguiLabel *self,
                             gboolean  setting)
{
  g_return_if_fail (BOBGUI_IS_LABEL (self));

  g_object_freeze_notify (G_OBJECT (self));

  if (bobgui_label_set_use_underline_internal (self, !!setting))
    bobgui_label_recalculate (self);

  g_object_thaw_notify (G_OBJECT (self));
}

/**
 * bobgui_label_get_use_underline:
 * @self: a label
 *
 * Returns whether underlines in the label indicate mnemonics.
 *
 * See [method@Bobgui.Label.set_use_underline].
 *
 * Returns: true if underlines in the label indicate mnemonics
 */
gboolean
bobgui_label_get_use_underline (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), FALSE);

  return self->use_underline;
}

/**
 * bobgui_label_set_single_line_mode:
 * @self: a label
 * @single_line_mode: true to enable single line mode
 *
 * Sets whether the label is in single line mode.
 */
void
bobgui_label_set_single_line_mode (BobguiLabel *self,
                                gboolean single_line_mode)
{
  g_return_if_fail (BOBGUI_IS_LABEL (self));

  single_line_mode = single_line_mode != FALSE;

  if (self->single_line_mode != single_line_mode)
    {
      self->single_line_mode = single_line_mode;

      bobgui_label_clear_layout (self);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (self));

      g_object_notify_by_pspec (G_OBJECT (self), label_props[PROP_SINGLE_LINE_MODE]);
    }
}

/**
 * bobgui_label_get_single_line_mode:
 * @self: a label
 *
 * Returns whether the label is in single line mode.
 *
 * Returns: true if the label is in single line mode
 **/
gboolean
bobgui_label_get_single_line_mode  (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), FALSE);

  return self->single_line_mode;
}

/* Compute the X position for an offset that corresponds to the more important
 * cursor position for that offset. We use this when trying to guess to which
 * end of the selection we should go to when the user hits the left or
 * right arrow key.
 */
static void
get_better_cursor (BobguiLabel *self,
                   int      index,
                   int      *x,
                   int      *y)
{
  GdkSeat *seat;
  GdkDevice *keyboard;
  PangoDirection keymap_direction;
  PangoDirection cursor_direction;
  gboolean split_cursor;
  PangoRectangle strong_pos, weak_pos;

  seat = gdk_display_get_default_seat (bobgui_widget_get_display (BOBGUI_WIDGET (self)));
  if (seat)
    keyboard = gdk_seat_get_keyboard (seat);
  else
    keyboard = NULL;
  if (keyboard)
    keymap_direction = gdk_device_get_direction (keyboard);
  else
    keymap_direction = PANGO_DIRECTION_LTR;

  cursor_direction = get_cursor_direction (self);

  g_object_get (bobgui_widget_get_settings (BOBGUI_WIDGET (self)),
                "bobgui-split-cursor", &split_cursor,
                NULL);

  bobgui_label_ensure_layout (self);

  pango_layout_get_cursor_pos (self->layout, index,
                               &strong_pos, &weak_pos);

  if (split_cursor)
    {
      *x = strong_pos.x / PANGO_SCALE;
      *y = strong_pos.y / PANGO_SCALE;
    }
  else
    {
      if (keymap_direction == cursor_direction)
        {
          *x = strong_pos.x / PANGO_SCALE;
          *y = strong_pos.y / PANGO_SCALE;
        }
      else
        {
          *x = weak_pos.x / PANGO_SCALE;
          *y = weak_pos.y / PANGO_SCALE;
        }
    }
}


static int
bobgui_label_move_logically (BobguiLabel *self,
                          int       start,
                          int       count)
{
  int offset = g_utf8_pointer_to_offset (self->text, self->text + start);

  if (self->text)
    {
      const PangoLogAttr *log_attrs;
      int n_attrs;
      int length;

      bobgui_label_ensure_layout (self);

      length = g_utf8_strlen (self->text, -1);

      log_attrs = pango_layout_get_log_attrs_readonly (self->layout, &n_attrs);

      while (count > 0 && offset < length)
        {
          do
            offset++;
          while (offset < length && !log_attrs[offset].is_cursor_position);

          count--;
        }
      while (count < 0 && offset > 0)
        {
          do
            offset--;
          while (offset > 0 && !log_attrs[offset].is_cursor_position);

          count++;
        }
    }

  return g_utf8_offset_to_pointer (self->text, offset) - self->text;
}

static int
bobgui_label_move_visually (BobguiLabel *self,
                         int       start,
                         int       count)
{
  int index;

  index = start;

  while (count != 0)
    {
      int new_index, new_trailing;
      gboolean split_cursor;
      gboolean strong;

      bobgui_label_ensure_layout (self);

      g_object_get (bobgui_widget_get_settings (BOBGUI_WIDGET (self)),
                    "bobgui-split-cursor", &split_cursor,
                    NULL);

      if (split_cursor)
        strong = TRUE;
      else
        {
          GdkSeat *seat;
          GdkDevice *keyboard;
          PangoDirection keymap_direction;

          seat = gdk_display_get_default_seat (bobgui_widget_get_display (BOBGUI_WIDGET (self)));
          if (seat)
            keyboard = gdk_seat_get_keyboard (seat);
          else
            keyboard = NULL;
          if (keyboard)
            keymap_direction = gdk_device_get_direction (keyboard);
          else
            keymap_direction = PANGO_DIRECTION_LTR;

          strong = keymap_direction == get_cursor_direction (self);
        }

      if (count > 0)
        {
          pango_layout_move_cursor_visually (self->layout, strong, index, 0, 1, &new_index, &new_trailing);
          count--;
        }
      else
        {
          pango_layout_move_cursor_visually (self->layout, strong, index, 0, -1, &new_index, &new_trailing);
          count++;
        }

      if (new_index < 0 || new_index == G_MAXINT)
        break;

      index = new_index;

      while (new_trailing--)
        index = g_utf8_next_char (self->text + new_index) - self->text;
    }

  return index;
}

static void
bobgui_label_move_cursor (BobguiLabel       *self,
                       BobguiMovementStep step,
                       int             count,
                       gboolean        extend_selection)
{
  int old_pos;
  int new_pos;

  if (self->select_info == NULL)
    return;

  old_pos = new_pos = self->select_info->selection_end;

  if (self->select_info->selection_end != self->select_info->selection_anchor &&
      !extend_selection)
    {
      /* If we have a current selection and aren't extending it, move to the
       * start/or end of the selection as appropriate
       */
      switch (step)
        {
        case BOBGUI_MOVEMENT_VISUAL_POSITIONS:
          {
            int end_x, end_y;
            int anchor_x, anchor_y;
            gboolean end_is_left;

            get_better_cursor (self, self->select_info->selection_end, &end_x, &end_y);
            get_better_cursor (self, self->select_info->selection_anchor, &anchor_x, &anchor_y);

            end_is_left = (end_y < anchor_y) || (end_y == anchor_y && end_x < anchor_x);

            if (count < 0)
              new_pos = end_is_left ? self->select_info->selection_end : self->select_info->selection_anchor;
            else
              new_pos = !end_is_left ? self->select_info->selection_end : self->select_info->selection_anchor;
            break;
          }
        case BOBGUI_MOVEMENT_LOGICAL_POSITIONS:
        case BOBGUI_MOVEMENT_WORDS:
          if (count < 0)
            new_pos = MIN (self->select_info->selection_end, self->select_info->selection_anchor);
          else
            new_pos = MAX (self->select_info->selection_end, self->select_info->selection_anchor);
          break;
        case BOBGUI_MOVEMENT_DISPLAY_LINE_ENDS:
        case BOBGUI_MOVEMENT_PARAGRAPH_ENDS:
        case BOBGUI_MOVEMENT_BUFFER_ENDS:
          /* FIXME: Can do better here */
          new_pos = count < 0 ? 0 : strlen (self->text);
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
          new_pos = bobgui_label_move_logically (self, new_pos, count);
          break;
        case BOBGUI_MOVEMENT_VISUAL_POSITIONS:
          new_pos = bobgui_label_move_visually (self, new_pos, count);
          if (new_pos == old_pos)
            {
              if (!extend_selection)
                {
                  if (!bobgui_widget_keynav_failed (BOBGUI_WIDGET (self),
                                                 count > 0 ?
                                                 BOBGUI_DIR_RIGHT : BOBGUI_DIR_LEFT))
                    {
                      BobguiRoot *root = bobgui_widget_get_root (BOBGUI_WIDGET (self));

                      if (root)
                        bobgui_widget_child_focus (BOBGUI_WIDGET (root), count > 0 ? BOBGUI_DIR_RIGHT : BOBGUI_DIR_LEFT);
                    }
                }
              else
                {
                  bobgui_widget_error_bell (BOBGUI_WIDGET (self));
                }
            }
          break;
        case BOBGUI_MOVEMENT_WORDS:
          while (count > 0)
            {
              new_pos = bobgui_label_move_forward_word (self, new_pos);
              count--;
            }
          while (count < 0)
            {
              new_pos = bobgui_label_move_backward_word (self, new_pos);
              count++;
            }
          if (new_pos == old_pos)
            bobgui_widget_error_bell (BOBGUI_WIDGET (self));
          break;
        case BOBGUI_MOVEMENT_DISPLAY_LINE_ENDS:
        case BOBGUI_MOVEMENT_PARAGRAPH_ENDS:
        case BOBGUI_MOVEMENT_BUFFER_ENDS:
          /* FIXME: Can do better here */
          new_pos = count < 0 ? 0 : strlen (self->text);
          if (new_pos == old_pos)
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
    bobgui_label_select_region_index (self,
                                   self->select_info->selection_anchor,
                                   new_pos);
  else
    bobgui_label_select_region_index (self, new_pos, new_pos);
}

static GMenuModel *
bobgui_label_get_menu_model (BobguiLabel *self)
{
  BobguiJoinedMenu *joined;
  GMenu *menu, *section;
  GMenuItem *item;

  joined = bobgui_joined_menu_new ();
  menu = g_menu_new ();

  section = g_menu_new ();
  g_menu_append (section, _("Cu_t"), "clipboard.cut");
  g_menu_append (section, _("_Copy"), "clipboard.copy");
  g_menu_append (section, _("_Paste"), "clipboard.paste");
  g_menu_append (section, _("_Delete"), "selection.delete");
  g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
  g_object_unref (section);

  section = g_menu_new ();
  g_menu_append (section, _("Select _All"), "selection.select-all");
  g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
  g_object_unref (section);

  section = g_menu_new ();
  item = g_menu_item_new (_("_Open Link"), "link.open");
  g_menu_item_set_attribute (item, "hidden-when", "s", "action-disabled");
  g_menu_append_item (section, item);
  g_object_unref (item);
  item = g_menu_item_new (_("Copy _Link Address"), "link.copy");
  g_menu_item_set_attribute (item, "hidden-when", "s", "action-disabled");
  g_menu_append_item (section, item);
  g_object_unref (item);
  g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
  g_object_unref (section);

  bobgui_joined_menu_append_menu (joined, G_MENU_MODEL (menu));
  g_object_unref (menu);

  if (self->extra_menu)
    bobgui_joined_menu_append_menu (joined, self->extra_menu);

  return G_MENU_MODEL (joined);
}

static void
bobgui_label_do_popup (BobguiLabel *self,
                    double    x,
                    double    y)
{
  if (!self->select_info)
    return;

  if (self->select_info->link_clicked)
    self->select_info->context_link = self->select_info->active_link;
  else
    self->select_info->context_link = bobgui_label_get_focus_link (self, NULL);

  bobgui_label_update_actions (self);

  if (!self->popup_menu)
    {
      GMenuModel *model;

      model = bobgui_label_get_menu_model (self);
      self->popup_menu = bobgui_popover_menu_new_from_model (model);
      bobgui_widget_set_parent (self->popup_menu, BOBGUI_WIDGET (self));
      bobgui_popover_set_position (BOBGUI_POPOVER (self->popup_menu), BOBGUI_POS_BOTTOM);

      bobgui_popover_set_has_arrow (BOBGUI_POPOVER (self->popup_menu), FALSE);
      bobgui_widget_set_halign (self->popup_menu, BOBGUI_ALIGN_START);

      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self->popup_menu),
                                      BOBGUI_ACCESSIBLE_PROPERTY_LABEL, _("Context menu"),
                                      -1);

      g_object_unref (model);
    }

  if (x != -1 && y != -1)
    {
      GdkRectangle rect = { x, y, 1, 1 };
      bobgui_popover_set_pointing_to (BOBGUI_POPOVER (self->popup_menu), &rect);
    }
  else
    bobgui_popover_set_pointing_to (BOBGUI_POPOVER (self->popup_menu), NULL);

  bobgui_popover_popup (BOBGUI_POPOVER (self->popup_menu));
}

/**
 * bobgui_label_get_current_uri:
 * @self: a label
 *
 * Returns the URI for the active link in the label.
 *
 * The active link is the one under the mouse pointer or, in a
 * selectable label, the link in which the text cursor is currently
 * positioned.
 *
 * This function is intended for use in a [signal@Bobgui.Label::activate-link]
 * handler or for use in a [signal@Bobgui.Widget::query-tooltip] handler.
 *
 * Returns: (nullable): the active URI
 */
const char *
bobgui_label_get_current_uri (BobguiLabel *self)
{
  const BobguiLabelLink *link;

  g_return_val_if_fail (BOBGUI_IS_LABEL (self), NULL);

  if (!self->select_info)
    return NULL;

  if (!self->select_info->link_clicked && self->select_info->selectable)
    link = bobgui_label_get_focus_link (self, NULL);
  else
    link = self->select_info->active_link;

  if (link)
    return link->uri;

  return NULL;
}

int
_bobgui_label_get_cursor_position (BobguiLabel *self)
{
  if (self->select_info && self->select_info->selectable)
    return g_utf8_pointer_to_offset (self->text,
                                     self->text + self->select_info->selection_end);

  return 0;
}

int
_bobgui_label_get_selection_bound (BobguiLabel *self)
{
  if (self->select_info && self->select_info->selectable)
    return g_utf8_pointer_to_offset (self->text,
                                     self->text + self->select_info->selection_anchor);

  return 0;
}

/**
 * bobgui_label_set_lines:
 * @self: a label
 * @lines: the desired number of lines, or -1
 *
 * Sets the number of lines to which an ellipsized, wrapping label
 * should be limited.
 *
 * This has no effect if the label is not wrapping or ellipsized.
 * Set this to -1 if you don’t want to limit the number of lines.
 */
void
bobgui_label_set_lines (BobguiLabel *self,
                     int       lines)
{
  g_return_if_fail (BOBGUI_IS_LABEL (self));

  if (self->lines != lines)
    {
      self->lines = lines;
      bobgui_label_clear_layout (self);
      g_object_notify_by_pspec (G_OBJECT (self), label_props[PROP_LINES]);
      bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
    }
}

/**
 * bobgui_label_get_lines:
 * @self: a label
 *
 * Gets the number of lines to which an ellipsized, wrapping
 * label should be limited.
 *
 * See [method@Bobgui.Label.set_lines].
 *
 * Returns: the number of lines
 */
int
bobgui_label_get_lines (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), -1);

  return self->lines;
}

/**
 * bobgui_label_set_xalign:
 * @self: a label
 * @xalign: the new xalign value, between 0 and 1
 *
 * Sets the `xalign` of the label.
 *
 * See the [property@Bobgui.Label:xalign] property.
 */
void
bobgui_label_set_xalign (BobguiLabel *self,
                      float     xalign)
{
  g_return_if_fail (BOBGUI_IS_LABEL (self));

  xalign = CLAMP (xalign, 0.0, 1.0);

  if (self->xalign == xalign)
    return;

  self->xalign = xalign;

  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), label_props[PROP_XALIGN]);
}

/**
 * bobgui_label_get_xalign:
 * @self: a label
 *
 * Gets the `xalign` of the label.
 *
 * See the [property@Bobgui.Label:xalign] property.
 *
 * Returns: the xalign value
 */
float
bobgui_label_get_xalign (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), 0.5);

  return self->xalign;
}

/**
 * bobgui_label_set_yalign:
 * @self: a label
 * @yalign: the new yalign value, between 0 and 1
 *
 * Sets the `yalign` of the label.
 *
 * See the [property@Bobgui.Label:yalign] property.
 */
void
bobgui_label_set_yalign (BobguiLabel *self,
                      float     yalign)
{
  g_return_if_fail (BOBGUI_IS_LABEL (self));

  yalign = CLAMP (yalign, 0.0, 1.0);

  if (self->yalign == yalign)
    return;

  self->yalign = yalign;

  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), label_props[PROP_YALIGN]);
}

/**
 * bobgui_label_get_yalign:
 * @self: a label
 *
 * Gets the `yalign` of the label.
 *
 * See the [property@Bobgui.Label:yalign] property.
 *
 * Returns: the yalign value
 */
float
bobgui_label_get_yalign (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), 0.5);

  return self->yalign;
}

/**
 * bobgui_label_set_extra_menu:
 * @self: a label
 * @model: (nullable): a menu model
 *
 * Sets a menu model to add to the context menu of the label.
 */
void
bobgui_label_set_extra_menu (BobguiLabel   *self,
                          GMenuModel *model)
{
  g_return_if_fail (BOBGUI_IS_LABEL (self));

  if (g_set_object (&self->extra_menu, model))
    {
      g_clear_pointer (&self->popup_menu, bobgui_widget_unparent);
      g_object_notify_by_pspec (G_OBJECT (self), label_props[PROP_EXTRA_MENU]);
    }
}

/**
 * bobgui_label_get_extra_menu:
 * @self: a label
 *
 * Gets the extra menu model of the label.
 *
 * See [method@Bobgui.Label.set_extra_menu].
 *
 * Returns: (transfer none) (nullable): the menu model
 */
GMenuModel *
bobgui_label_get_extra_menu (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), NULL);

  return self->extra_menu;
}

/**
 * bobgui_label_set_tabs:
 * @self: a label
 * @tabs: (nullable): tab stops
 *
 * Sets tab stops for the label.
 *
 * Since: 4.8
 */
void
bobgui_label_set_tabs (BobguiLabel      *self,
                    PangoTabArray *tabs)
{
  g_return_if_fail (BOBGUI_IS_LABEL (self));

  if (self->tabs == tabs)
    return;

  if (self->tabs)
    pango_tab_array_free (self->tabs);
  self->tabs = pango_tab_array_copy (tabs);

  bobgui_label_clear_layout (self);
  g_object_notify_by_pspec (G_OBJECT (self), label_props[PROP_TABS]);
  bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
}

/**
 * bobgui_label_get_tabs:
 * @self: a label
 *
 * Gets the tab stops for the label.
 *
 * The returned array will be `NULL` if “standard” (8-space) tabs are used.
 *
 * Returns: (nullable) (transfer full): copy of default tab array,
 *   or `NULL` if standard tabs are used
 *
 * Since: 4.8
 */
PangoTabArray *
bobgui_label_get_tabs (BobguiLabel *self)
{
  g_return_val_if_fail (BOBGUI_IS_LABEL (self), NULL);

  return self->tabs ? pango_tab_array_copy (self->tabs) : NULL;
}

/* {{{ BobguiAccessible implementation */

static void
bobgui_label_link_ensure_accessible (BobguiLabel     *self,
                                  BobguiLabelLink *link,
                                  gsize         index)
{
  if (!link->accessible)
    {
      BobguiAccessibleTextRange range;
      BobguiLabelLink *next = NULL;

      range.start = link->start;
      range.length = link->end - link->start;
      link->accessible = bobgui_accessible_hyperlink_new (BOBGUI_ACCESSIBLE_HYPERTEXT (self), index, link->uri, &range);

      if (index + 1 < self->select_info->n_links)
        {
          next = &self->select_info->links[index + 1];
          bobgui_label_link_ensure_accessible (self, next, index + 1);
        }

      bobgui_accessible_set_accessible_parent (BOBGUI_ACCESSIBLE (link->accessible),
                                            BOBGUI_ACCESSIBLE (self),
                                            next ? BOBGUI_ACCESSIBLE (next->accessible) : NULL);

      if (link->title)
        bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (link->accessible),
                                        BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION,
                                        link->title,
                                        -1);

      bobgui_accessible_hyperlink_set_platform_state (link->accessible,
                                                   BOBGUI_ACCESSIBLE_PLATFORM_STATE_FOCUSABLE,
                                                   TRUE);

      if (link == bobgui_label_get_focus_link (self, NULL))
        bobgui_accessible_hyperlink_set_platform_state (link->accessible,
                                                     BOBGUI_ACCESSIBLE_PLATFORM_STATE_FOCUSED,
                                                     TRUE);
    }
}

static BobguiAccessible *
bobgui_label_accessible_get_first_accessible_child (BobguiAccessible *self)
{
  BobguiLabel *label = BOBGUI_LABEL (self);

  if (label->select_info && label->select_info->links &&
      0 < label->select_info->n_links)
    {
      BobguiLabelLink *link;

      link = &label->select_info->links[0];

      bobgui_label_link_ensure_accessible (label, link, 0);

      return BOBGUI_ACCESSIBLE (g_object_ref (link->accessible));
    }

  return NULL;
}

static void
bobgui_label_accessible_init (BobguiAccessibleInterface *iface)
{
  BobguiAccessibleInterface *parent = g_type_interface_peek_parent (iface);

  *iface = *parent;
  iface->get_first_accessible_child = bobgui_label_accessible_get_first_accessible_child;
}

/* }}} */
/* {{{ BobguiAccessibleText implementation */

static GBytes *
bobgui_label_accessible_text_get_contents (BobguiAccessibleText *self,
                                        unsigned int       start,
                                        unsigned int       end)
{
  const char *text;
  int len;
  char *string;
  gsize size;

  text = bobgui_label_get_text (BOBGUI_LABEL (self));
  len = g_utf8_strlen (text, -1);

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

  return g_bytes_new_take (string, size);
}

static GBytes *
bobgui_label_accessible_text_get_contents_at (BobguiAccessibleText            *self,
                                           unsigned int                  offset,
                                           BobguiAccessibleTextGranularity  granularity,
                                           unsigned int                 *start,
                                           unsigned int                 *end)
{
  PangoLayout *layout = bobgui_label_get_layout (BOBGUI_LABEL (self));
  char *string = bobgui_pango_get_string_at (layout, offset, granularity, start, end);

  return g_bytes_new_take (string, strlen (string));
}

static unsigned
bobgui_label_accessible_text_get_caret_position (BobguiAccessibleText *self)
{
  return _bobgui_label_get_cursor_position (BOBGUI_LABEL (self));
}

static gboolean
bobgui_label_accessible_text_get_selection (BobguiAccessibleText       *self,
                                         gsize                   *n_ranges,
                                         BobguiAccessibleTextRange **ranges)
{
  int start, end;

  if (!bobgui_label_get_selection_bounds (BOBGUI_LABEL (self), &start, &end))
    return FALSE;

  *n_ranges = 1;
  *ranges = g_new (BobguiAccessibleTextRange, 1);
  (*ranges)[0].start = start;
  (*ranges)[0].length = end - start;

  return TRUE;
}

static void
bobgui_label_accessible_text_get_default_attributes (BobguiAccessibleText   *self,
                                                  char              ***attribute_names,
                                                  char              ***attribute_values)
{
  PangoLayout *layout = bobgui_label_get_layout (BOBGUI_LABEL (self));
  char **names, **values;

  bobgui_pango_get_default_attributes (layout, &names, &values);

  *attribute_names = names;
  *attribute_values = values;
}

static gboolean
bobgui_label_accessible_text_get_attributes (BobguiAccessibleText        *self,
                                          unsigned int              offset,
                                          gsize                    *n_ranges,
                                          BobguiAccessibleTextRange  **ranges,
                                          char                   ***attribute_names,
                                          char                   ***attribute_values)
{
  PangoLayout *layout = bobgui_label_get_layout (BOBGUI_LABEL (self));
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

static gboolean
bobgui_label_accessible_text_get_extents (BobguiAccessibleText *self,
                                       unsigned int       start,
                                       unsigned int       end,
                                       graphene_rect_t   *extents)
{
  BobguiLabel *label = BOBGUI_LABEL (self);
  PangoLayout *layout;
  const char *text;
  float lx, ly;
  cairo_region_t *range_clip;
  cairo_rectangle_int_t clip_rect;
  int range[2];

  layout = label->layout;
  text = label->text;
  bobgui_label_get_layout_location (label, &lx, &ly);

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
bobgui_label_accessible_text_get_offset (BobguiAccessibleText      *self,
                                      const graphene_point_t *point,
                                      unsigned int           *offset)
{
  BobguiLabel *label = BOBGUI_LABEL (self);
  int index;

  if (!get_layout_index (label, roundf (point->x), roundf (point->y), &index))
    return FALSE;

  *offset = (unsigned int) g_utf8_pointer_to_offset (label->text, label->text + index);

  return TRUE;
}

static gboolean
bobgui_label_accessible_text_set_caret_position (BobguiAccessibleText *self,
                                              unsigned int       offset)
{
  BobguiLabel *label = BOBGUI_LABEL (self);
  int index;

  if (!bobgui_label_get_selectable (label))
    return FALSE;

  index = g_utf8_offset_to_pointer (label->text, offset) - label->text;

  bobgui_label_select_region_index (label, index, index);
  return TRUE;
}

static gboolean
bobgui_label_accessible_text_set_selection (BobguiAccessibleText      *self,
                                         gsize                   i,
                                         BobguiAccessibleTextRange *range)
{
  BobguiLabel *label = BOBGUI_LABEL (self);
  int index1, index2;

  if (!bobgui_label_get_selectable (label))
    return FALSE;

  if (i != 0)
    return FALSE;

  index1 = g_utf8_offset_to_pointer (label->text, range->start) - label->text;
  index2 = g_utf8_offset_to_pointer (label->text, range->start + range->length) - label->text;

  bobgui_label_select_region_index (label, index1, index2);
  return TRUE;
}

static void
bobgui_label_accessible_text_init (BobguiAccessibleTextInterface *iface)
{
  iface->get_contents = bobgui_label_accessible_text_get_contents;
  iface->get_contents_at = bobgui_label_accessible_text_get_contents_at;
  iface->get_caret_position = bobgui_label_accessible_text_get_caret_position;
  iface->get_selection = bobgui_label_accessible_text_get_selection;
  iface->get_attributes = bobgui_label_accessible_text_get_attributes;
  iface->get_default_attributes = bobgui_label_accessible_text_get_default_attributes;
  iface->get_extents = bobgui_label_accessible_text_get_extents;
  iface->get_offset = bobgui_label_accessible_text_get_offset;
  iface->set_caret_position = bobgui_label_accessible_text_set_caret_position;
  iface->set_selection = bobgui_label_accessible_text_set_selection;
}

 /* }}} */
/*  {{{ BobguiAccessibleHypertext implementation */

static unsigned int
bobgui_label_accessible_hypertext_get_n_links (BobguiAccessibleHypertext *self)
{
  BobguiLabel *label = BOBGUI_LABEL (self);

  if (label->select_info && label->select_info->links)
    return label->select_info->n_links;

  return 0;
}

static BobguiAccessibleHyperlink *
bobgui_label_accessible_hypertext_get_link (BobguiAccessibleHypertext *self,
                                         unsigned int            index)
{
  BobguiLabel *label = BOBGUI_LABEL (self);

  if (label->select_info && label->select_info->links &&
      index < label->select_info->n_links)
    {
      BobguiLabelLink *link;

      link = &label->select_info->links[index];

      bobgui_label_link_ensure_accessible (label, link, index);

      return BOBGUI_ACCESSIBLE_HYPERLINK (link->accessible);
    }

  return NULL;
}

static unsigned int
bobgui_label_accessible_hypertext_get_link_at (BobguiAccessibleHypertext *self,
                                            unsigned int            offset)
{
  BobguiLabel *label = BOBGUI_LABEL (self);
  int pos;
  int link_index;

  pos = g_utf8_offset_to_pointer (label->text, offset) - label->text;
  link_index = _bobgui_label_get_link_at (label, pos);

  if (link_index != -1)
    return (unsigned int) link_index;

  return G_MAXUINT;
}

static void
bobgui_label_accessible_hypertext_init (BobguiAccessibleHypertextInterface *iface)
{
  iface->get_n_links = bobgui_label_accessible_hypertext_get_n_links;
  iface->get_link = bobgui_label_accessible_hypertext_get_link;
  iface->get_link_at = bobgui_label_accessible_hypertext_get_link_at;
}

/* }}} */

/* vim:set foldmethod=marker: */

