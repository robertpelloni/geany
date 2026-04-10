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

/*
 * Modified by the BOBGUI Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the BOBGUI Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * BOBGUI at ftp://ftp.bobgui.org/pub/bobgui/.
 */

#include "config.h"

#include "bobguientryprivate.h"

#include "bobguiaccessibleprivate.h"
#include "bobguiadjustment.h"
#include "deprecated/bobguicelleditable.h"
#include "bobguidebug.h"
#include "bobguieditable.h"
#include "bobguiemojichooser.h"
#include "bobguiemojicompletion.h"
#include "bobguientrybuffer.h"
#include "bobguigesturedrag.h"
#include <glib/gi18n-lib.h>
#include "bobguijoinedmenuprivate.h"
#include "bobguilabel.h"
#include "bobguimain.h"
#include "bobguimarshalers.h"
#include "bobguipangoprivate.h"
#include "bobguiprivate.h"
#include "bobguiprogressbar.h"
#include "bobguisnapshot.h"
#include "bobguitextprivate.h"
#include "bobguitextutilprivate.h"
#include "bobguitooltip.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"
#include "bobguinative.h"
#include "bobguigestureclick.h"
#include "bobguidragsourceprivate.h"
#include "bobguidragicon.h"
#include "bobguiwidgetpaintable.h"
#include "bobguibuilderprivate.h"

#include <cairo-gobject.h>
#include <string.h>

/**
 * BobguiEntry:
 *
 * A single-line text entry widget.
 *
 * <picture>
 *   <source srcset="entry-dark.png" media="(prefers-color-scheme: dark)">
 *   <img alt="An example BobguiEntry" src="entry.png">
 * </picture>
 *
 * A fairly large set of key bindings are supported by default. If the
 * entered text is longer than the allocation of the widget, the widget
 * will scroll so that the cursor position is visible.
 *
 * When using an entry for passwords and other sensitive information, it
 * can be put into “password mode” using [method@Bobgui.Entry.set_visibility].
 * In this mode, entered text is displayed using a “invisible” character.
 * By default, BOBGUI picks the best invisible character that is available
 * in the current font, but it can be changed with
 * [method@Bobgui.Entry.set_invisible_char].
 *
 * `BobguiEntry` has the ability to display progress or activity
 * information behind the text. To make an entry display such information,
 * use [method@Bobgui.Entry.set_progress_fraction] or
 * [method@Bobgui.Entry.set_progress_pulse_step].
 *
 * Additionally, `BobguiEntry` can show icons at either side of the entry.
 * These icons can be activatable by clicking, can be set up as drag source
 * and can have tooltips. To add an icon, use
 * [method@Bobgui.Entry.set_icon_from_gicon] or one of the various other functions
 * that set an icon from an icon name or a paintable. To trigger an action when
 * the user clicks an icon, connect to the [signal@Bobgui.Entry::icon-press] signal.
 * To allow DND operations from an icon, use
 * [method@Bobgui.Entry.set_icon_drag_source]. To set a tooltip on an icon, use
 * [method@Bobgui.Entry.set_icon_tooltip_text] or the corresponding function
 * for markup.
 *
 * Note that functionality or information that is only available by clicking
 * on an icon in an entry may not be accessible at all to users which are not
 * able to use a mouse or other pointing device. It is therefore recommended
 * that any such functionality should also be available by other means, e.g.
 * via the context menu of the entry.
 *
 * # CSS nodes
 *
 * ```
 * entry[.flat][.warning][.error]
 * ├── text[.readonly]
 * ├── image.left
 * ├── image.right
 * ╰── [progress[.pulse]]
 * ```
 *
 * `BobguiEntry` has a main node with the name entry. Depending on the properties
 * of the entry, the style classes .read-only and .flat may appear. The style
 * classes .warning and .error may also be used with entries.
 *
 * When the entry shows icons, it adds subnodes with the name image and the
 * style class .left or .right, depending on where the icon appears.
 *
 * When the entry shows progress, it adds a subnode with the name progress.
 * The node has the style class .pulse when the shown progress is pulsing.
 *
 * For all the subnodes added to the text node in various situations,
 * see [class@Bobgui.Text].
 *
 * # BobguiEntry as BobguiBuildable
 *
 * The `BobguiEntry` implementation of the `BobguiBuildable` interface supports a
 * custom `<attributes>` element, which supports any number of `<attribute>`
 * elements. The `<attribute>` element has attributes named “name“, “value“,
 * “start“ and “end“ and allows you to specify `PangoAttribute` values for
 * this label.
 *
 * An example of a UI definition fragment specifying Pango attributes:
 * ```xml
 * <object class="BobguiEntry">
 *   <attributes>
 *     <attribute name="weight" value="PANGO_WEIGHT_BOLD"/>
 *     <attribute name="background" value="red" start="5" end="10"/>
 *   </attributes>
 * </object>
 * ```
 *
 * The start and end attributes specify the range of characters to which the
 * Pango attribute applies. If start and end are not specified, the attribute
 * is applied to the whole text. Note that specifying ranges does not make much
 * sense with translatable attributes. Use markup embedded in the translatable
 * content instead.
 *
 * # Accessibility
 *
 * `BobguiEntry` uses the [enum@Bobgui.AccessibleRole.text_box] role.
 */

#define MAX_ICONS 2

#define IS_VALID_ICON_POSITION(pos)               \
  ((pos) == BOBGUI_ENTRY_ICON_PRIMARY ||                   \
   (pos) == BOBGUI_ENTRY_ICON_SECONDARY)

static GQuark          quark_entry_completion = 0;

typedef struct _EntryIconInfo EntryIconInfo;

typedef struct _BobguiEntryPrivate       BobguiEntryPrivate;
struct _BobguiEntryPrivate
{
  EntryIconInfo *icons[MAX_ICONS];

  BobguiWidget     *text;
  BobguiWidget     *progress_widget;
  GMenuModel    *extra_menu;
  gchar         *menu_entry_icon_primary_text;
  gchar         *menu_entry_icon_secondary_text;

  guint         show_emoji_icon         : 1;
  guint         editing_canceled        : 1; /* Only used by BobguiCellRendererText */
};

struct _EntryIconInfo
{
  BobguiWidget *widget;
  char *tooltip;
  guint nonactivatable : 1;
  guint in_drag        : 1;

  GdkDragAction actions;
  GdkContentProvider *content;
};

enum {
  ACTIVATE,
  ICON_PRESS,
  ICON_RELEASE,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_BUFFER,
  PROP_MAX_LENGTH,
  PROP_VISIBILITY,
  PROP_HAS_FRAME,
  PROP_INVISIBLE_CHAR,
  PROP_ACTIVATES_DEFAULT,
  PROP_SCROLL_OFFSET,
  PROP_TRUNCATE_MULTILINE,
  PROP_OVERWRITE_MODE,
  PROP_TEXT_LENGTH,
  PROP_INVISIBLE_CHAR_SET,
  PROP_PROGRESS_FRACTION,
  PROP_PROGRESS_PULSE_STEP,
  PROP_PAINTABLE_PRIMARY,
  PROP_PAINTABLE_SECONDARY,
  PROP_ICON_NAME_PRIMARY,
  PROP_ICON_NAME_SECONDARY,
  PROP_GICON_PRIMARY,
  PROP_GICON_SECONDARY,
  PROP_STORAGE_TYPE_PRIMARY,
  PROP_STORAGE_TYPE_SECONDARY,
  PROP_ACTIVATABLE_PRIMARY,
  PROP_ACTIVATABLE_SECONDARY,
  PROP_SENSITIVE_PRIMARY,
  PROP_SENSITIVE_SECONDARY,
  PROP_TOOLTIP_TEXT_PRIMARY,
  PROP_TOOLTIP_TEXT_SECONDARY,
  PROP_TOOLTIP_MARKUP_PRIMARY,
  PROP_TOOLTIP_MARKUP_SECONDARY,
  PROP_IM_MODULE,
  PROP_PLACEHOLDER_TEXT,
  PROP_COMPLETION,
  PROP_INPUT_PURPOSE,
  PROP_INPUT_HINTS,
  PROP_ATTRIBUTES,
  PROP_TABS,
  PROP_EXTRA_MENU,
  PROP_SHOW_EMOJI_ICON,
  PROP_ENABLE_EMOJI_COMPLETION,
  PROP_MENU_ENTRY_ICON_PRIMARY_TEXT,
  PROP_MENU_ENTRY_ICON_SECONDARY_TEXT,
  PROP_EDITING_CANCELED,
  NUM_PROPERTIES = PROP_EDITING_CANCELED,
};

static GParamSpec *entry_props[NUM_PROPERTIES] = { NULL, };

static guint signals[LAST_SIGNAL] = { 0 };

typedef enum {
  CURSOR_STANDARD,
  CURSOR_DND
} CursorType;

typedef enum
{
  DISPLAY_NORMAL,       /* The entry text is being shown */
  DISPLAY_INVISIBLE,    /* In invisible mode, text replaced by (eg) bullets */
  DISPLAY_BLANK         /* In invisible mode, nothing shown at all */
} DisplayMode;

/* GObject methods
 */
static void   bobgui_entry_editable_init        (BobguiEditableInterface *iface);
static void   bobgui_entry_cell_editable_init   (BobguiCellEditableIface *iface);
static void   bobgui_entry_set_property         (GObject          *object,
                                              guint             prop_id,
                                              const GValue     *value,
                                              GParamSpec       *pspec);
static void   bobgui_entry_get_property         (GObject          *object,
                                              guint             prop_id,
                                              GValue           *value,
                                              GParamSpec       *pspec);
static void   bobgui_entry_finalize             (GObject          *object);
static void   bobgui_entry_dispose              (GObject          *object);

/* BobguiWidget methods
 */
static void   bobgui_entry_size_allocate        (BobguiWidget        *widget,
                                              int               width,
                                              int               height,
                                                int               baseline);
static void   bobgui_entry_snapshot             (BobguiWidget        *widget,
                                              BobguiSnapshot      *snapshot);
static gboolean bobgui_entry_query_tooltip      (BobguiWidget        *widget,
                                              int               x,
                                              int               y,
                                              gboolean          keyboard_tip,
                                              BobguiTooltip       *tooltip);
static void   bobgui_entry_direction_changed    (BobguiWidget        *widget,
					      BobguiTextDirection  previous_dir);


/* BobguiCellEditable method implementations
 */
static void bobgui_entry_start_editing (BobguiCellEditable *cell_editable,
				     GdkEvent        *event);

static void update_extra_menu (BobguiEntry *entry);

/* Default signal handlers
 */
static BobguiEntryBuffer *get_buffer                      (BobguiEntry       *entry);
static void         set_show_emoji_icon                (BobguiEntry       *entry,
                                                        gboolean        value);

static void     bobgui_entry_measure (BobguiWidget           *widget,
                                   BobguiOrientation       orientation,
                                   int                  for_size,
                                   int                 *minimum,
                                   int                 *natural,
                                   int                 *minimum_baseline,
                                   int                 *natural_baseline);

static BobguiBuildableIface *buildable_parent_iface = NULL;

static void     bobgui_entry_buildable_interface_init (BobguiBuildableIface *iface);
static void     bobgui_entry_accessible_interface_init (BobguiAccessibleInterface *iface);

G_DEFINE_TYPE_WITH_CODE (BobguiEntry, bobgui_entry, BOBGUI_TYPE_WIDGET,
                         G_ADD_PRIVATE (BobguiEntry)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACCESSIBLE,
                                                bobgui_entry_accessible_interface_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_BUILDABLE,
                                                bobgui_entry_buildable_interface_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_EDITABLE,
                                                bobgui_entry_editable_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_CELL_EDITABLE,
                                                bobgui_entry_cell_editable_init))

/* Implement the BobguiAccessible interface, in order to obtain focus
 * state from the BobguiText widget that we are wrapping. The BobguiText
 * widget is ignored for accessibility purposes (it has role NONE),
 * and any a11y text functionality is implemented for BobguiEntry and
 * similar wrappers (BobguiPasswordEntry, BobguiSpinButton, etc).
 */
static gboolean
bobgui_entry_accessible_get_platform_state (BobguiAccessible              *self,
                                         BobguiAccessiblePlatformState  state)
{
  return bobgui_editable_delegate_get_accessible_platform_state (BOBGUI_EDITABLE (self), state);
}

static void
bobgui_entry_accessible_interface_init (BobguiAccessibleInterface *iface)
{
  BobguiAccessibleInterface *parent_iface = g_type_interface_peek_parent (iface);
  iface->get_at_context = parent_iface->get_at_context;
  iface->get_platform_state = bobgui_entry_accessible_get_platform_state;
}

static const BobguiBuildableParser pango_parser =
{
  bobgui_pango_attribute_start_element,
};

static gboolean
bobgui_entry_buildable_custom_tag_start (BobguiBuildable       *buildable,
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
bobgui_entry_buildable_custom_finished (BobguiBuildable *buildable,
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
          bobgui_entry_set_attributes (BOBGUI_ENTRY (buildable), data->attrs);
          pango_attr_list_unref (data->attrs);
        }

      g_object_unref (data->object);
      g_object_unref (data->builder);
      g_free (data);
    }
}

static void
bobgui_entry_buildable_interface_init (BobguiBuildableIface *iface)
{
  buildable_parent_iface = g_type_interface_peek_parent (iface);

  iface->custom_tag_start = bobgui_entry_buildable_custom_tag_start;
  iface->custom_finished = bobgui_entry_buildable_custom_finished;
}

static gboolean
bobgui_entry_grab_focus (BobguiWidget *widget)
{
  BobguiEntry *entry = BOBGUI_ENTRY (widget);
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  return bobgui_widget_grab_focus (priv->text);
}

static gboolean
bobgui_entry_mnemonic_activate (BobguiWidget *widget,
                             gboolean   group_cycling)
{
  BobguiEntry *entry = BOBGUI_ENTRY (widget);
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  bobgui_widget_grab_focus (priv->text);

  return TRUE;
}

static void
bobgui_entry_activate_misc_icon (BobguiWidget  *widget,
                              const char *action_name,
                              GVariant   *parameter)
{
  BobguiEntry *self = BOBGUI_ENTRY (widget);
  bobgui_entry_activate_icon (self,
                           g_str_equal (action_name, "misc.menu_entry_icon_primary")
                           ? BOBGUI_ENTRY_ICON_PRIMARY
                           : BOBGUI_ENTRY_ICON_SECONDARY);
}

static void
bobgui_entry_class_init (BobguiEntryClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class;

  widget_class = (BobguiWidgetClass*) class;

  gobject_class->dispose = bobgui_entry_dispose;
  gobject_class->finalize = bobgui_entry_finalize;
  gobject_class->set_property = bobgui_entry_set_property;
  gobject_class->get_property = bobgui_entry_get_property;

  widget_class->measure = bobgui_entry_measure;
  widget_class->size_allocate = bobgui_entry_size_allocate;
  widget_class->snapshot = bobgui_entry_snapshot;
  widget_class->query_tooltip = bobgui_entry_query_tooltip;
  widget_class->direction_changed = bobgui_entry_direction_changed;
  widget_class->grab_focus = bobgui_entry_grab_focus;
  widget_class->focus = bobgui_widget_focus_child;
  widget_class->mnemonic_activate = bobgui_entry_mnemonic_activate;

  quark_entry_completion = g_quark_from_static_string ("bobgui-entry-completion-key");

  /**
   * BobguiEntry:buffer:
   *
   * The buffer object which actually stores the text.
   */
  entry_props[PROP_BUFFER] =
      g_param_spec_object ("buffer", NULL, NULL,
                           BOBGUI_TYPE_ENTRY_BUFFER,
                           BOBGUI_PARAM_READWRITE|G_PARAM_CONSTRUCT|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:max-length:
   *
   * Maximum number of characters for this entry.
   */
  entry_props[PROP_MAX_LENGTH] =
      g_param_spec_int ("max-length", NULL, NULL,
                        0, BOBGUI_ENTRY_BUFFER_MAX_SIZE,
                        0,
                        BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:visibility:
   *
   * Whether the entry should show the “invisible char” instead of the
   * actual text (“password mode”).
   */
  entry_props[PROP_VISIBILITY] =
      g_param_spec_boolean ("visibility", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:has-frame:
   *
   * Whether the entry should draw a frame.
   */
  entry_props[PROP_HAS_FRAME] =
      g_param_spec_boolean ("has-frame", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:invisible-char:
   *
   * The character to use when masking entry contents (“password mode”).
   */
  entry_props[PROP_INVISIBLE_CHAR] =
      g_param_spec_unichar ("invisible-char", NULL, NULL,
                            '*',
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:activates-default:
   *
   * Whether to activate the default widget when Enter is pressed.
   */
  entry_props[PROP_ACTIVATES_DEFAULT] =
      g_param_spec_boolean ("activates-default", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:scroll-offset:
   *
   * Number of pixels of the entry scrolled off the screen to the left.
   */
  entry_props[PROP_SCROLL_OFFSET] =
      g_param_spec_int ("scroll-offset", NULL, NULL,
                        0, G_MAXINT,
                        0,
                        BOBGUI_PARAM_READABLE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:truncate-multiline:
   *
   * When %TRUE, pasted multi-line text is truncated to the first line.
   */
  entry_props[PROP_TRUNCATE_MULTILINE] =
      g_param_spec_boolean ("truncate-multiline", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:overwrite-mode:
   *
   * If text is overwritten when typing in the `BobguiEntry`.
   */
  entry_props[PROP_OVERWRITE_MODE] =
      g_param_spec_boolean ("overwrite-mode", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:text-length:
   *
   * The length of the text in the `BobguiEntry`.
   */
  entry_props[PROP_TEXT_LENGTH] =
      g_param_spec_uint ("text-length", NULL, NULL,
                         0, G_MAXUINT16,
                         0,
                         BOBGUI_PARAM_READABLE);

  /**
   * BobguiEntry:invisible-char-set:
   *
   * Whether the invisible char has been set for the `BobguiEntry`.
   */
  entry_props[PROP_INVISIBLE_CHAR_SET] =
      g_param_spec_boolean ("invisible-char-set", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE);

  /**
   * BobguiEntry:progress-fraction:
   *
   * The current fraction of the task that's been completed.
   */
  entry_props[PROP_PROGRESS_FRACTION] =
      g_param_spec_double ("progress-fraction", NULL, NULL,
                           0.0, 1.0,
                           0.0,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:progress-pulse-step:
   *
   * The fraction of total entry width to move the progress
   * bouncing block for each pulse.
   *
   * See [method@Bobgui.Entry.progress_pulse].
   */
  entry_props[PROP_PROGRESS_PULSE_STEP] =
      g_param_spec_double ("progress-pulse-step", NULL, NULL,
                           0.0, 1.0,
                           0.0,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
  * BobguiEntry:placeholder-text:
  *
  * The text that will be displayed in the `BobguiEntry` when it is empty
  * and unfocused.
  */
  entry_props[PROP_PLACEHOLDER_TEXT] =
      g_param_spec_string ("placeholder-text", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

   /**
   * BobguiEntry:primary-icon-paintable:
   *
   * A `GdkPaintable` to use as the primary icon for the entry.
   */
  entry_props[PROP_PAINTABLE_PRIMARY] =
      g_param_spec_object ("primary-icon-paintable", NULL, NULL,
                           GDK_TYPE_PAINTABLE,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:secondary-icon-paintable:
   *
   * A `GdkPaintable` to use as the secondary icon for the entry.
   */
  entry_props[PROP_PAINTABLE_SECONDARY] =
      g_param_spec_object ("secondary-icon-paintable", NULL, NULL,
                           GDK_TYPE_PAINTABLE,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:primary-icon-name:
   *
   * The icon name to use for the primary icon for the entry.
   */
  entry_props[PROP_ICON_NAME_PRIMARY] =
      g_param_spec_string ("primary-icon-name", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:secondary-icon-name:
   *
   * The icon name to use for the secondary icon for the entry.
   */
  entry_props[PROP_ICON_NAME_SECONDARY] =
      g_param_spec_string ("secondary-icon-name", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:primary-icon-gicon:
   *
   * The `GIcon` to use for the primary icon for the entry.
   */
  entry_props[PROP_GICON_PRIMARY] =
      g_param_spec_object ("primary-icon-gicon", NULL, NULL,
                           G_TYPE_ICON,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:secondary-icon-gicon:
   *
   * The `GIcon` to use for the secondary icon for the entry.
   */
  entry_props[PROP_GICON_SECONDARY] =
      g_param_spec_object ("secondary-icon-gicon", NULL, NULL,
                           G_TYPE_ICON,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:primary-icon-storage-type:
   *
   * The representation which is used for the primary icon of the entry.
   */
  entry_props[PROP_STORAGE_TYPE_PRIMARY] =
      g_param_spec_enum ("primary-icon-storage-type", NULL, NULL,
                         BOBGUI_TYPE_IMAGE_TYPE,
                         BOBGUI_IMAGE_EMPTY,
                         BOBGUI_PARAM_READABLE);

  /**
   * BobguiEntry:secondary-icon-storage-type:
   *
   * The representation which is used for the secondary icon of the entry.
   */
  entry_props[PROP_STORAGE_TYPE_SECONDARY] =
      g_param_spec_enum ("secondary-icon-storage-type", NULL, NULL,
                         BOBGUI_TYPE_IMAGE_TYPE,
                         BOBGUI_IMAGE_EMPTY,
                         BOBGUI_PARAM_READABLE);

  /**
   * BobguiEntry:primary-icon-activatable:
   *
   * Whether the primary icon is activatable.
   *
   * BOBGUI emits the [signal@Bobgui.Entry::icon-press] and
   * [signal@Bobgui.Entry::icon-release] signals only on sensitive,
   * activatable icons.
   *
   * Sensitive, but non-activatable icons can be used for purely
   * informational purposes.
   */
  entry_props[PROP_ACTIVATABLE_PRIMARY] =
      g_param_spec_boolean ("primary-icon-activatable", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:secondary-icon-activatable:
   *
   * Whether the secondary icon is activatable.
   *
   * BOBGUI emits the [signal@Bobgui.Entry::icon-press] and
   * [signal@Bobgui.Entry::icon-release] signals only on sensitive,
   * activatable icons.
   *
   * Sensitive, but non-activatable icons can be used for purely
   * informational purposes.
   */
  entry_props[PROP_ACTIVATABLE_SECONDARY] =
      g_param_spec_boolean ("secondary-icon-activatable", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:primary-icon-sensitive:
   *
   * Whether the primary icon is sensitive.
   *
   * An insensitive icon appears grayed out. BOBGUI does not emit the
   * [signal@Bobgui.Entry::icon-press] and [signal@Bobgui.Entry::icon-release]
   * signals and does not allow DND from insensitive icons.
   *
   * An icon should be set insensitive if the action that would trigger
   * when clicked is currently not available.
   */
  entry_props[PROP_SENSITIVE_PRIMARY] =
      g_param_spec_boolean ("primary-icon-sensitive", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:secondary-icon-sensitive:
   *
   * Whether the secondary icon is sensitive.
   *
   * An insensitive icon appears grayed out. BOBGUI does not emit the
   * [signal@Bobgui.Entry::icon-press[ and [signal@Bobgui.Entry::icon-release]
   * signals and does not allow DND from insensitive icons.
   *
   * An icon should be set insensitive if the action that would trigger
   * when clicked is currently not available.
   */
  entry_props[PROP_SENSITIVE_SECONDARY] =
      g_param_spec_boolean ("secondary-icon-sensitive", NULL, NULL,
                            TRUE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:primary-icon-tooltip-text:
   *
   * The contents of the tooltip on the primary icon.
   *
   * Also see [method@Bobgui.Entry.set_icon_tooltip_text].
   */
  entry_props[PROP_TOOLTIP_TEXT_PRIMARY] =
      g_param_spec_string ("primary-icon-tooltip-text", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:secondary-icon-tooltip-text:
   *
   * The contents of the tooltip on the secondary icon.
   *
   * Also see [method@Bobgui.Entry.set_icon_tooltip_text].
   */
  entry_props[PROP_TOOLTIP_TEXT_SECONDARY] =
      g_param_spec_string ("secondary-icon-tooltip-text", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:primary-icon-tooltip-markup:
   *
   * The contents of the tooltip on the primary icon, with markup.
   *
   * Also see [method@Bobgui.Entry.set_icon_tooltip_markup].
   */
  entry_props[PROP_TOOLTIP_MARKUP_PRIMARY] =
      g_param_spec_string ("primary-icon-tooltip-markup", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:secondary-icon-tooltip-markup:
   *
   * The contents of the tooltip on the secondary icon, with markup.
   *
   * Also see [method@Bobgui.Entry.set_icon_tooltip_markup].
   */
  entry_props[PROP_TOOLTIP_MARKUP_SECONDARY] =
      g_param_spec_string ("secondary-icon-tooltip-markup", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:im-module:
   *
   * Which IM (input method) module should be used for this entry.
   *
   * See [class@Bobgui.IMContext].
   *
   * Setting this to a non-%NULL value overrides the system-wide IM
   * module setting. See the BobguiSettings [property@Bobgui.Settings:bobgui-im-module]
   * property.
   */
  entry_props[PROP_IM_MODULE] =
      g_param_spec_string ("im-module", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:completion:
   *
   * The auxiliary completion object to use with the entry.
   *
   * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
   */
  entry_props[PROP_COMPLETION] =
      g_param_spec_object ("completion", NULL, NULL,
                           BOBGUI_TYPE_ENTRY_COMPLETION,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY|G_PARAM_DEPRECATED);

  /**
   * BobguiEntry:input-purpose:
   *
   * The purpose of this text field.
   *
   * This property can be used by on-screen keyboards and other input
   * methods to adjust their behaviour.
   *
   * Note that setting the purpose to %BOBGUI_INPUT_PURPOSE_PASSWORD or
   * %BOBGUI_INPUT_PURPOSE_PIN is independent from setting
   * [property@Bobgui.Entry:visibility].
   */
  entry_props[PROP_INPUT_PURPOSE] =
      g_param_spec_enum ("input-purpose", NULL, NULL,
                         BOBGUI_TYPE_INPUT_PURPOSE,
                         BOBGUI_INPUT_PURPOSE_FREE_FORM,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:input-hints:
   *
   * Additional hints that allow input methods to fine-tune their behavior.
   *
   * Also see [property@Bobgui.Entry:input-purpose]
   */
  entry_props[PROP_INPUT_HINTS] =
      g_param_spec_flags ("input-hints", NULL, NULL,
                          BOBGUI_TYPE_INPUT_HINTS,
                          BOBGUI_INPUT_HINT_NONE,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:attributes:
   *
   * A list of Pango attributes to apply to the text of the entry.
   *
   * This is mainly useful to change the size or weight of the text.
   *
   * The `PangoAttribute`'s @start_index and @end_index must refer to the
   * [class@Bobgui.EntryBuffer] text, i.e. without the preedit string.
   */
  entry_props[PROP_ATTRIBUTES] =
      g_param_spec_boxed ("attributes", NULL, NULL,
                          PANGO_TYPE_ATTR_LIST,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:tabs:
   *
   * A list of tabstops to apply to the text of the entry.
   */
  entry_props[PROP_TABS] =
      g_param_spec_boxed ("tabs", NULL, NULL,
                          PANGO_TYPE_TAB_ARRAY,
                          BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:show-emoji-icon:
   *
   * Whether the entry will show an Emoji icon in the secondary icon position
   * to open the Emoji chooser.
   */
  entry_props[PROP_SHOW_EMOJI_ICON] =
      g_param_spec_boolean ("show-emoji-icon", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:extra-menu:
   *
   * A menu model whose contents will be appended to the context menu.
   */
  entry_props[PROP_EXTRA_MENU] =
      g_param_spec_object ("extra-menu", NULL, NULL,
                           G_TYPE_MENU_MODEL,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:enable-emoji-completion:
   *
   * Whether to suggest Emoji replacements for :-delimited names
   * like `:heart:`.
   */
  entry_props[PROP_ENABLE_EMOJI_COMPLETION] =
      g_param_spec_boolean ("enable-emoji-completion", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:menu-entry-icon-primary-text:
   *
   * Text for an item in the context menu to activate the primary icon action.
   *
   * When the primary icon is activatable and this property has been set, a new entry
   * in the context menu of this BobguiEntry will appear with this text. Selecting that
   * menu entry will result in the primary icon being activated, exactly in the same way
   * as it would be activated from a mouse click.
   *
   * This simplifies adding accessibility support to applications using activatable
   * icons. The activatable icons aren't focusable when navigating the interface with
   * the keyboard This is why Bobgui recommends to also add those actions in the context
   * menu. This set of methods greatly simplifies this, by adding a menu item that, when
   * enabled, calls the same callback than clicking on the icon.
   *
   * Since: 4.20
   */
  entry_props[PROP_MENU_ENTRY_ICON_PRIMARY_TEXT] =
      g_param_spec_string ("menu-entry-icon-primary-text", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiEntry:menu-entry-icon-secondary-text:
   *
   * Text for an item in the context menu to activate the secondary icon action.
   *
   * When the primary icon is activatable and this property has been set, a new entry
   * in the context menu of this BobguiEntry will appear with this text. Selecting that
   * menu entry will result in the primary icon being activated, exactly in the same way
   * as it would be activated from a mouse click.
   *
   * This simplifies adding accessibility support to applications using activatable
   * icons. The activatable icons aren't focusable when navigating the interface with
   * the keyboard This is why Bobgui recommends to also add those actions in the context
   * menu. This set of methods greatly simplifies this, by adding a menu item that, when
   * enabled, calls the same callback than clicking on the icon.
   *
   * Since: 4.20
   */
  entry_props[PROP_MENU_ENTRY_ICON_SECONDARY_TEXT] =
      g_param_spec_string ("menu-entry-icon-secondary-text", NULL, NULL,
                           NULL,
                           BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, entry_props);
  g_object_class_override_property (gobject_class, PROP_EDITING_CANCELED, "editing-canceled");
  bobgui_editable_install_properties (gobject_class, PROP_EDITING_CANCELED + 1);

  /**
   * BobguiEntry::activate:
   * @self: The widget on which the signal is emitted
   *
   * Emitted when the entry is activated.
   *
   * The keybindings for this signal are all forms of the Enter key.
   */
  signals[ACTIVATE] =
    g_signal_new (I_("activate"),
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                  G_STRUCT_OFFSET (BobguiEntryClass, activate),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiEntry::icon-press:
   * @entry: The entry on which the signal is emitted
   * @icon_pos: The position of the clicked icon
   *
   * Emitted when an activatable icon is clicked.
   */
  signals[ICON_PRESS] =
    g_signal_new (I_("icon-press"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  BOBGUI_TYPE_ENTRY_ICON_POSITION);

  /**
   * BobguiEntry::icon-release:
   * @entry: The entry on which the signal is emitted
   * @icon_pos: The position of the clicked icon
   *
   * Emitted on the button release from a mouse click
   * over an activatable icon.
   */
  signals[ICON_RELEASE] =
    g_signal_new (I_("icon-release"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1,
                  BOBGUI_TYPE_ENTRY_ICON_POSITION);

  bobgui_widget_class_set_css_name (widget_class, I_("entry"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_TEXT_BOX);

  bobgui_widget_class_install_action (widget_class, "misc.menu_entry_icon_primary", NULL,
                                   bobgui_entry_activate_misc_icon);
  bobgui_widget_class_install_action (widget_class, "misc.menu_entry_icon_secondary", NULL,
                                   bobgui_entry_activate_misc_icon);
}

static BobguiEditable *
bobgui_entry_get_delegate (BobguiEditable *editable)
{
  BobguiEntry *entry = BOBGUI_ENTRY (editable);
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  return BOBGUI_EDITABLE (priv->text);
}

static void
bobgui_entry_editable_init (BobguiEditableInterface *iface)
{
  iface->get_delegate = bobgui_entry_get_delegate;
}

static void
bobgui_entry_cell_editable_init (BobguiCellEditableIface *iface)
{
  iface->start_editing = bobgui_entry_start_editing;
}

static void
bobgui_entry_set_property (GObject         *object,
                        guint            prop_id,
                        const GValue    *value,
                        GParamSpec      *pspec)
{
  BobguiEntry *entry = BOBGUI_ENTRY (object);
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  if (bobgui_editable_delegate_set_property (object, prop_id, value, pspec))
    {
      if (prop_id == PROP_EDITING_CANCELED + 1 + BOBGUI_EDITABLE_PROP_EDITABLE)
        {
          bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (entry),
                                          BOBGUI_ACCESSIBLE_PROPERTY_READ_ONLY, !g_value_get_boolean (value),
                                          -1);
        }

      return;
    }

  switch (prop_id)
    {
    case PROP_BUFFER:
    case PROP_MAX_LENGTH:
    case PROP_VISIBILITY:
    case PROP_INVISIBLE_CHAR:
    case PROP_INVISIBLE_CHAR_SET:
    case PROP_ACTIVATES_DEFAULT:
    case PROP_TRUNCATE_MULTILINE:
    case PROP_OVERWRITE_MODE:
    case PROP_IM_MODULE:
    case PROP_INPUT_PURPOSE:
    case PROP_INPUT_HINTS:
    case PROP_ATTRIBUTES:
    case PROP_TABS:
    case PROP_ENABLE_EMOJI_COMPLETION:
      g_object_set_property (G_OBJECT (priv->text), pspec->name, value);
      break;

    case PROP_PLACEHOLDER_TEXT:
      bobgui_entry_set_placeholder_text (entry, g_value_get_string (value));
      break;

    case PROP_HAS_FRAME:
      bobgui_entry_set_has_frame (entry, g_value_get_boolean (value));
      break;

    case PROP_PROGRESS_FRACTION:
      bobgui_entry_set_progress_fraction (entry, g_value_get_double (value));
      break;

    case PROP_PROGRESS_PULSE_STEP:
      bobgui_entry_set_progress_pulse_step (entry, g_value_get_double (value));
      break;

    case PROP_PAINTABLE_PRIMARY:
      bobgui_entry_set_icon_from_paintable (entry,
                                         BOBGUI_ENTRY_ICON_PRIMARY,
                                         g_value_get_object (value));
      break;

    case PROP_PAINTABLE_SECONDARY:
      bobgui_entry_set_icon_from_paintable (entry,
                                         BOBGUI_ENTRY_ICON_SECONDARY,
                                         g_value_get_object (value));
      break;

    case PROP_ICON_NAME_PRIMARY:
      bobgui_entry_set_icon_from_icon_name (entry,
                                         BOBGUI_ENTRY_ICON_PRIMARY,
                                         g_value_get_string (value));
      break;

    case PROP_ICON_NAME_SECONDARY:
      bobgui_entry_set_icon_from_icon_name (entry,
                                         BOBGUI_ENTRY_ICON_SECONDARY,
                                         g_value_get_string (value));
      break;

    case PROP_GICON_PRIMARY:
      bobgui_entry_set_icon_from_gicon (entry,
                                     BOBGUI_ENTRY_ICON_PRIMARY,
                                     g_value_get_object (value));
      break;

    case PROP_GICON_SECONDARY:
      bobgui_entry_set_icon_from_gicon (entry,
                                     BOBGUI_ENTRY_ICON_SECONDARY,
                                     g_value_get_object (value));
      break;

    case PROP_ACTIVATABLE_PRIMARY:
      bobgui_entry_set_icon_activatable (entry,
                                      BOBGUI_ENTRY_ICON_PRIMARY,
                                      g_value_get_boolean (value));
      break;

    case PROP_ACTIVATABLE_SECONDARY:
      bobgui_entry_set_icon_activatable (entry,
                                      BOBGUI_ENTRY_ICON_SECONDARY,
                                      g_value_get_boolean (value));
      break;

    case PROP_SENSITIVE_PRIMARY:
      bobgui_entry_set_icon_sensitive (entry,
                                    BOBGUI_ENTRY_ICON_PRIMARY,
                                    g_value_get_boolean (value));
      break;

    case PROP_SENSITIVE_SECONDARY:
      bobgui_entry_set_icon_sensitive (entry,
                                    BOBGUI_ENTRY_ICON_SECONDARY,
                                    g_value_get_boolean (value));
      break;

    case PROP_TOOLTIP_TEXT_PRIMARY:
      bobgui_entry_set_icon_tooltip_text (entry,
                                       BOBGUI_ENTRY_ICON_PRIMARY,
                                       g_value_get_string (value));
      break;

    case PROP_TOOLTIP_TEXT_SECONDARY:
      bobgui_entry_set_icon_tooltip_text (entry,
                                       BOBGUI_ENTRY_ICON_SECONDARY,
                                       g_value_get_string (value));
      break;

    case PROP_TOOLTIP_MARKUP_PRIMARY:
      bobgui_entry_set_icon_tooltip_markup (entry,
                                         BOBGUI_ENTRY_ICON_PRIMARY,
                                         g_value_get_string (value));
      break;

    case PROP_TOOLTIP_MARKUP_SECONDARY:
      bobgui_entry_set_icon_tooltip_markup (entry,
                                         BOBGUI_ENTRY_ICON_SECONDARY,
                                         g_value_get_string (value));
      break;

    case PROP_EDITING_CANCELED:
      if (priv->editing_canceled != g_value_get_boolean (value))
        {
          priv->editing_canceled = g_value_get_boolean (value);
          g_object_notify (object, "editing-canceled");
        }
      break;

    case PROP_COMPLETION:
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      bobgui_entry_set_completion (entry, BOBGUI_ENTRY_COMPLETION (g_value_get_object (value)));
G_GNUC_END_IGNORE_DEPRECATIONS
      break;

    case PROP_SHOW_EMOJI_ICON:
      set_show_emoji_icon (entry, g_value_get_boolean (value));
      break;

    case PROP_EXTRA_MENU:
      bobgui_entry_set_extra_menu (entry, g_value_get_object (value));
      break;

    case PROP_MENU_ENTRY_ICON_PRIMARY_TEXT:
      bobgui_entry_set_menu_entry_icon_text (entry,
                                          BOBGUI_ENTRY_ICON_PRIMARY,
                                          g_value_get_string (value));
      break;

    case PROP_MENU_ENTRY_ICON_SECONDARY_TEXT:
      bobgui_entry_set_menu_entry_icon_text (entry,
                                          BOBGUI_ENTRY_ICON_SECONDARY,
                                          g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_entry_get_property (GObject         *object,
                        guint            prop_id,
                        GValue          *value,
                        GParamSpec      *pspec)
{
  BobguiEntry *entry = BOBGUI_ENTRY (object);
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  if (bobgui_editable_delegate_get_property (object, prop_id, value, pspec))
    return;

  switch (prop_id)
    {
    case PROP_BUFFER:
    case PROP_IM_MODULE:
    case PROP_MAX_LENGTH:
    case PROP_VISIBILITY:
    case PROP_INVISIBLE_CHAR:
    case PROP_INVISIBLE_CHAR_SET:
    case PROP_ACTIVATES_DEFAULT:
    case PROP_SCROLL_OFFSET:
    case PROP_TRUNCATE_MULTILINE:
    case PROP_OVERWRITE_MODE:
    case PROP_PLACEHOLDER_TEXT:
    case PROP_INPUT_PURPOSE:
    case PROP_INPUT_HINTS:
    case PROP_ATTRIBUTES:
    case PROP_TABS:
    case PROP_ENABLE_EMOJI_COMPLETION:
      g_object_get_property (G_OBJECT (priv->text), pspec->name, value);
      break;

    case PROP_HAS_FRAME:
      g_value_set_boolean (value, bobgui_entry_get_has_frame (entry));
      break;

    case PROP_TEXT_LENGTH:
      g_value_set_uint (value, bobgui_entry_get_text_length (entry));
      break;

    case PROP_PROGRESS_FRACTION:
      g_value_set_double (value, bobgui_entry_get_progress_fraction (entry));
      break;

    case PROP_PROGRESS_PULSE_STEP:
      g_value_set_double (value, bobgui_entry_get_progress_pulse_step (entry));
      break;

    case PROP_PAINTABLE_PRIMARY:
      g_value_set_object (value,
                          bobgui_entry_get_icon_paintable (entry,
                                                        BOBGUI_ENTRY_ICON_PRIMARY));
      break;

    case PROP_PAINTABLE_SECONDARY:
      g_value_set_object (value,
                          bobgui_entry_get_icon_paintable (entry,
                                                        BOBGUI_ENTRY_ICON_SECONDARY));
      break;

    case PROP_ICON_NAME_PRIMARY:
      g_value_set_string (value,
                          bobgui_entry_get_icon_name (entry,
                                                   BOBGUI_ENTRY_ICON_PRIMARY));
      break;

    case PROP_ICON_NAME_SECONDARY:
      g_value_set_string (value,
                          bobgui_entry_get_icon_name (entry,
                                                   BOBGUI_ENTRY_ICON_SECONDARY));
      break;

    case PROP_GICON_PRIMARY:
      g_value_set_object (value,
                          bobgui_entry_get_icon_gicon (entry,
                                                    BOBGUI_ENTRY_ICON_PRIMARY));
      break;

    case PROP_GICON_SECONDARY:
      g_value_set_object (value,
                          bobgui_entry_get_icon_gicon (entry,
                                                    BOBGUI_ENTRY_ICON_SECONDARY));
      break;

    case PROP_STORAGE_TYPE_PRIMARY:
      g_value_set_enum (value,
                        bobgui_entry_get_icon_storage_type (entry, 
                                                         BOBGUI_ENTRY_ICON_PRIMARY));
      break;

    case PROP_STORAGE_TYPE_SECONDARY:
      g_value_set_enum (value,
                        bobgui_entry_get_icon_storage_type (entry, 
                                                         BOBGUI_ENTRY_ICON_SECONDARY));
      break;

    case PROP_ACTIVATABLE_PRIMARY:
      g_value_set_boolean (value,
                           bobgui_entry_get_icon_activatable (entry, BOBGUI_ENTRY_ICON_PRIMARY));
      break;

    case PROP_ACTIVATABLE_SECONDARY:
      g_value_set_boolean (value,
                           bobgui_entry_get_icon_activatable (entry, BOBGUI_ENTRY_ICON_SECONDARY));
      break;

    case PROP_SENSITIVE_PRIMARY:
      g_value_set_boolean (value,
                           bobgui_entry_get_icon_sensitive (entry, BOBGUI_ENTRY_ICON_PRIMARY));
      break;

    case PROP_SENSITIVE_SECONDARY:
      g_value_set_boolean (value,
                           bobgui_entry_get_icon_sensitive (entry, BOBGUI_ENTRY_ICON_SECONDARY));
      break;

    case PROP_TOOLTIP_TEXT_PRIMARY:
      g_value_take_string (value,
                           bobgui_entry_get_icon_tooltip_text (entry, BOBGUI_ENTRY_ICON_PRIMARY));
      break;

    case PROP_TOOLTIP_TEXT_SECONDARY:
      g_value_take_string (value,
                           bobgui_entry_get_icon_tooltip_text (entry, BOBGUI_ENTRY_ICON_SECONDARY));
      break;

    case PROP_TOOLTIP_MARKUP_PRIMARY:
      g_value_take_string (value,
                           bobgui_entry_get_icon_tooltip_markup (entry, BOBGUI_ENTRY_ICON_PRIMARY));
      break;

    case PROP_TOOLTIP_MARKUP_SECONDARY:
      g_value_take_string (value,
                           bobgui_entry_get_icon_tooltip_markup (entry, BOBGUI_ENTRY_ICON_SECONDARY));
      break;

    case PROP_EDITING_CANCELED:
      g_value_set_boolean (value,
                           priv->editing_canceled);
      break;

    case PROP_COMPLETION:
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      g_value_set_object (value, G_OBJECT (bobgui_entry_get_completion (entry)));
G_GNUC_END_IGNORE_DEPRECATIONS
      break;

    case PROP_SHOW_EMOJI_ICON:
      g_value_set_boolean (value, priv->show_emoji_icon);
      break;

    case PROP_EXTRA_MENU:
      g_value_set_object (value, bobgui_entry_get_extra_menu (entry));
      break;

    case PROP_MENU_ENTRY_ICON_PRIMARY_TEXT:
      g_value_set_string (value,
                          bobgui_entry_get_menu_entry_icon_text (entry,
                                                              BOBGUI_ENTRY_ICON_PRIMARY));
      break;

    case PROP_MENU_ENTRY_ICON_SECONDARY_TEXT:
      g_value_set_string (value,
                          bobgui_entry_get_menu_entry_icon_text (entry,
                                                              BOBGUI_ENTRY_ICON_SECONDARY));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
activate_cb (BobguiText *text, BobguiEntry  *entry)
{
  g_signal_emit (entry, signals[ACTIVATE], 0);
}

static void
notify_cb (GObject    *object,
           GParamSpec *pspec,
           gpointer    data)
{
  gpointer iface;
  gpointer class;

  /* The editable interface properties are already forwarded by the editable delegate setup */
  iface = g_type_interface_peek (g_type_class_peek (G_OBJECT_TYPE (object)), bobgui_editable_get_type ());
  class = g_type_class_peek (BOBGUI_TYPE_ENTRY);
  if (!g_object_interface_find_property (iface, pspec->name) &&
      g_object_class_find_property (class, pspec->name))
    g_object_notify (data, pspec->name);
}

static void
connect_text_signals (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_signal_connect (priv->text, "activate", G_CALLBACK (activate_cb), entry);
  g_signal_connect (priv->text, "notify", G_CALLBACK (notify_cb), entry);
}

static void
disconnect_text_signals (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_signal_handlers_disconnect_by_func (priv->text, activate_cb, entry);
  g_signal_handlers_disconnect_by_func (priv->text, notify_cb, entry);
}

static void
catchall_click_press (BobguiGestureClick *gesture,
                      int              n_press,
                      double           x,
                      double           y,
                      gpointer         user_data)
{
  bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
}

static void
bobgui_entry_init (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  BobguiGesture *catchall;

  priv->text = bobgui_text_new ();
  bobgui_widget_set_parent (priv->text, BOBGUI_WIDGET (entry));
  bobgui_editable_init_delegate (BOBGUI_EDITABLE (entry));
  connect_text_signals (entry);

  catchall = bobgui_gesture_click_new ();
  g_signal_connect (catchall, "pressed",
                    G_CALLBACK (catchall_click_press), entry);
  bobgui_widget_add_controller (BOBGUI_WIDGET (entry),
                             BOBGUI_EVENT_CONTROLLER (catchall));

  priv->editing_canceled = FALSE;

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (entry),
                                  BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP, TRUE,
                                  -1);
}

static void
bobgui_entry_dispose (GObject *object)
{
  BobguiEntry *entry = BOBGUI_ENTRY (object);
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  bobgui_entry_set_icon_from_paintable (entry, BOBGUI_ENTRY_ICON_PRIMARY, NULL);
  bobgui_entry_set_icon_tooltip_markup (entry, BOBGUI_ENTRY_ICON_PRIMARY, NULL);
  bobgui_entry_set_icon_from_paintable (entry, BOBGUI_ENTRY_ICON_SECONDARY, NULL);
  bobgui_entry_set_icon_tooltip_markup (entry, BOBGUI_ENTRY_ICON_SECONDARY, NULL);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  bobgui_entry_set_completion (entry, NULL);
G_GNUC_END_IGNORE_DEPRECATIONS

  if (priv->text)
    {
      disconnect_text_signals (entry);
      bobgui_editable_finish_delegate (BOBGUI_EDITABLE (entry));
    }
  g_clear_pointer (&priv->text, bobgui_widget_unparent);
  g_clear_object (&priv->extra_menu);
  g_clear_pointer (&priv->menu_entry_icon_primary_text, g_free);
  g_clear_pointer (&priv->menu_entry_icon_secondary_text, g_free);

  G_OBJECT_CLASS (bobgui_entry_parent_class)->dispose (object);
}

static void
bobgui_entry_finalize (GObject *object)
{
  BobguiEntry *entry = BOBGUI_ENTRY (object);
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info = NULL;
  int i;

  for (i = 0; i < MAX_ICONS; i++)
    {
      icon_info = priv->icons[i];
      if (icon_info == NULL)
        continue;

      g_clear_object (&icon_info->content);

      bobgui_widget_unparent (icon_info->widget);

      g_free (icon_info);
    }

  g_clear_pointer (&priv->progress_widget, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_entry_parent_class)->finalize (object);
}

static void
update_icon_style (BobguiWidget            *widget,
                   BobguiEntryIconPosition  icon_pos)
{
  BobguiEntry *entry = BOBGUI_ENTRY (widget);
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info = priv->icons[icon_pos];
  const char *sides[2] = { "left", "right" };

  if (icon_info == NULL)
    return;

  if (bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL)
    icon_pos = 1 - icon_pos;

  bobgui_widget_add_css_class (icon_info->widget, sides[icon_pos]);
  bobgui_widget_remove_css_class (icon_info->widget, sides[1 - icon_pos]);
}

static void
update_node_ordering (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info;
  BobguiEntryIconPosition first_icon_pos, second_icon_pos;

  if (priv->progress_widget)
    bobgui_widget_insert_before (priv->progress_widget, BOBGUI_WIDGET (entry), NULL);

  if (bobgui_widget_get_direction (BOBGUI_WIDGET (entry)) == BOBGUI_TEXT_DIR_RTL)
    {
      first_icon_pos = BOBGUI_ENTRY_ICON_SECONDARY;
      second_icon_pos = BOBGUI_ENTRY_ICON_PRIMARY;
    }
  else
    {
      first_icon_pos = BOBGUI_ENTRY_ICON_PRIMARY;
      second_icon_pos = BOBGUI_ENTRY_ICON_SECONDARY;
    }

  icon_info = priv->icons[first_icon_pos];
  if (icon_info)
    bobgui_widget_insert_after (icon_info->widget, BOBGUI_WIDGET (entry), NULL);

  icon_info = priv->icons[second_icon_pos];
  if (icon_info)
    bobgui_widget_insert_before (icon_info->widget, BOBGUI_WIDGET (entry), NULL);
}

static BobguiEntryIconPosition
get_icon_position_from_controller (BobguiEntry           *entry,
                                   BobguiEventController *controller)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  BobguiWidget *widget = bobgui_event_controller_get_widget (controller);

  if (priv->icons[BOBGUI_ENTRY_ICON_PRIMARY] &&
      priv->icons[BOBGUI_ENTRY_ICON_PRIMARY]->widget == widget)
    return BOBGUI_ENTRY_ICON_PRIMARY;
  else if (priv->icons[BOBGUI_ENTRY_ICON_SECONDARY] &&
           priv->icons[BOBGUI_ENTRY_ICON_SECONDARY]->widget == widget)
    return BOBGUI_ENTRY_ICON_SECONDARY;

  g_assert_not_reached ();
  return -1;
}

static void
icon_pressed_cb (BobguiGestureClick *gesture,
                 int                   n_press,
                 double                x,
                 double                y,
                 BobguiEntry             *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  BobguiEntryIconPosition pos;
  EntryIconInfo *icon_info;

  pos = get_icon_position_from_controller (entry, BOBGUI_EVENT_CONTROLLER (gesture));
  icon_info = priv->icons[pos];

  if (!icon_info->nonactivatable)
    g_signal_emit (entry, signals[ICON_PRESS], 0, pos);

  bobgui_gesture_set_state (BOBGUI_GESTURE (gesture), BOBGUI_EVENT_SEQUENCE_CLAIMED);
}

static void
icon_released_cb (BobguiGestureClick *gesture,
                  int              n_press,
                  double           x,
                  double           y,
                  BobguiEntry        *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  BobguiEntryIconPosition pos;
  EntryIconInfo *icon_info;

  pos = get_icon_position_from_controller (entry, BOBGUI_EVENT_CONTROLLER (gesture));
  icon_info = priv->icons[pos];

  if (!icon_info->nonactivatable)
    g_signal_emit (entry, signals[ICON_RELEASE], 0, pos);
}

static void
icon_drag_update_cb (BobguiGestureDrag *gesture,
                     double          offset_x,
                     double          offset_y,
                     BobguiEntry       *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  BobguiEntryIconPosition pos;
  EntryIconInfo *icon_info;

  pos = get_icon_position_from_controller (entry, BOBGUI_EVENT_CONTROLLER (gesture));
  icon_info = priv->icons[pos];

  if (icon_info->content != NULL &&
      bobgui_drag_check_threshold_double (icon_info->widget, 0, 0, offset_x, offset_y))
    {
      GdkPaintable *paintable;
      GdkSurface *surface;
      GdkDevice *device;
      GdkDrag *drag;
      double start_x, start_y;

      icon_info->in_drag = TRUE;

      surface = bobgui_native_get_surface (bobgui_widget_get_native (BOBGUI_WIDGET (entry)));
      device = bobgui_gesture_get_device (BOBGUI_GESTURE (gesture));

      bobgui_gesture_drag_get_start_point (gesture, &start_x, &start_y);

      drag = gdk_drag_begin (surface, device, icon_info->content, icon_info->actions, start_x, start_y);
      paintable = bobgui_widget_paintable_new (icon_info->widget);
      bobgui_drag_icon_set_from_paintable (drag, paintable, -2, -2);
      g_object_unref (paintable);

      g_object_unref (drag);
    }
}

static EntryIconInfo*
construct_icon_info (BobguiWidget            *widget,
                     BobguiEntryIconPosition  icon_pos)
{
  BobguiEntry *entry = BOBGUI_ENTRY (widget);
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info;
  BobguiGesture *drag, *press;

  g_return_val_if_fail (priv->icons[icon_pos] == NULL, NULL);

  icon_info = g_new0 (EntryIconInfo, 1);
  priv->icons[icon_pos] = icon_info;

  icon_info->widget = bobgui_image_new ();
  bobgui_widget_set_cursor_from_name (icon_info->widget, "default");
  if (icon_pos == BOBGUI_ENTRY_ICON_PRIMARY)
    bobgui_widget_insert_before (icon_info->widget, widget, priv->text);
  else
    bobgui_widget_insert_after (icon_info->widget, widget, priv->text);

  update_icon_style (widget, icon_pos);
  update_node_ordering (entry);

  press = bobgui_gesture_click_new ();
  g_signal_connect (press, "pressed", G_CALLBACK (icon_pressed_cb), entry);
  g_signal_connect (press, "released", G_CALLBACK (icon_released_cb), entry);
  bobgui_widget_add_controller (icon_info->widget, BOBGUI_EVENT_CONTROLLER (press));

  drag = bobgui_gesture_drag_new ();
  g_signal_connect (drag, "drag-update",
                    G_CALLBACK (icon_drag_update_cb), entry);
  bobgui_widget_add_controller (icon_info->widget, BOBGUI_EVENT_CONTROLLER (drag));

  bobgui_gesture_group (press, drag);

  return icon_info;
}

static void
bobgui_entry_measure (BobguiWidget      *widget,
                   BobguiOrientation  orientation,
                   int             for_size,
                   int             *minimum,
                   int             *natural,
                   int             *minimum_baseline,
                   int             *natural_baseline)
{
  BobguiEntry *entry = BOBGUI_ENTRY (widget);
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  int text_min, text_nat;
  int i;

  bobgui_widget_measure (priv->text,
                      orientation,
                      for_size,
                      &text_min, &text_nat,
                      minimum_baseline, natural_baseline);

  *minimum = text_min;
  *natural = text_nat;

  for (i = 0; i < MAX_ICONS; i++)
    {
      EntryIconInfo *icon_info = priv->icons[i];
      int icon_min, icon_nat;

      if (!icon_info)
        continue;

      bobgui_widget_measure (icon_info->widget,
                          BOBGUI_ORIENTATION_HORIZONTAL,
                          -1, &icon_min, &icon_nat, NULL, NULL);

      if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
        {
          *minimum += icon_min;
          *natural += icon_nat;
        }
      else
        {
          *minimum = MAX (*minimum, icon_min);
          *natural = MAX (*natural, icon_nat);
        }
    }

  if (priv->progress_widget && bobgui_widget_get_visible (priv->progress_widget))
    {
      int prog_min, prog_nat;

      bobgui_widget_measure (priv->progress_widget,
                          orientation,
                          for_size,
                          &prog_min, &prog_nat,
                          NULL, NULL);

      *minimum = MAX (*minimum, prog_min);
      *natural = MAX (*natural, prog_nat);
    }

  if (orientation == BOBGUI_ORIENTATION_VERTICAL)
    {
      if (G_LIKELY (*minimum_baseline >= 0))
        *minimum_baseline += (*minimum - text_min) / 2;
      if (G_LIKELY (*natural_baseline >= 0))
        *natural_baseline += (*natural - text_nat) / 2;
    }
}

static void
bobgui_entry_size_allocate (BobguiWidget *widget,
                         int        width,
                         int        height,
                         int        baseline)
{
  const gboolean is_rtl = bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL;
  BobguiEntry *entry = BOBGUI_ENTRY (widget);
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  int i;
  BobguiAllocation text_alloc;

  text_alloc.x = 0;
  text_alloc.y = 0;
  text_alloc.width = width;
  text_alloc.height = height;

  if (bobgui_widget_get_valign (widget) != BOBGUI_ALIGN_BASELINE_FILL &&
      bobgui_widget_get_valign (widget) != BOBGUI_ALIGN_BASELINE_CENTER)
    baseline = -1;

  for (i = 0; i < MAX_ICONS; i++)
    {
      EntryIconInfo *icon_info = priv->icons[i];
      BobguiAllocation icon_alloc;
      int icon_width;

      if (!icon_info)
        continue;

      bobgui_widget_measure (icon_info->widget,
                          BOBGUI_ORIENTATION_HORIZONTAL,
                          -1,
                          NULL, &icon_width,
                          NULL, NULL);

      if ((is_rtl  && i == BOBGUI_ENTRY_ICON_PRIMARY) ||
          (!is_rtl && i == BOBGUI_ENTRY_ICON_SECONDARY))
        icon_alloc.x = width - icon_width;
      else
        icon_alloc.x = 0;
      icon_alloc.y = 0;
      icon_alloc.width = icon_width;
      icon_alloc.height = height;

      bobgui_widget_size_allocate (icon_info->widget, &icon_alloc, baseline);

      text_alloc.width -= icon_width;

      if ((!is_rtl  && i == BOBGUI_ENTRY_ICON_PRIMARY) ||
          (is_rtl && i == BOBGUI_ENTRY_ICON_SECONDARY))
        text_alloc.x += icon_width;
    }

  bobgui_widget_size_allocate (priv->text, &text_alloc, baseline);

  if (priv->progress_widget && bobgui_widget_get_visible (priv->progress_widget))
    {
      BobguiAllocation progress_alloc;
      int min, nat;

      bobgui_widget_measure (priv->progress_widget,
                          BOBGUI_ORIENTATION_VERTICAL,
                          -1,
                          &min, &nat,
                          NULL, NULL);
      progress_alloc.x = 0;
      progress_alloc.y = height - nat;
      progress_alloc.width = width;
      progress_alloc.height = nat;

      bobgui_widget_size_allocate (priv->progress_widget, &progress_alloc, -1);
    }

  /* Do this here instead of bobgui_entry_size_allocate() so it works
   * inside spinbuttons, which don't chain up.
   */
  if (bobgui_widget_get_realized (widget))
    {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      BobguiEntryCompletion *completion;

      completion = bobgui_entry_get_completion (entry);
      if (completion)
        _bobgui_entry_completion_resize_popup (completion);
G_GNUC_END_IGNORE_DEPRECATIONS
    }
}

static void
bobgui_entry_snapshot (BobguiWidget   *widget,
                    BobguiSnapshot *snapshot)
{
  BobguiEntry *entry = BOBGUI_ENTRY (widget);
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  int i;

  /* Draw progress */
  if (priv->progress_widget && bobgui_widget_get_visible (priv->progress_widget))
    bobgui_widget_snapshot_child (widget, priv->progress_widget, snapshot);

  bobgui_widget_snapshot_child (widget, priv->text, snapshot);

  /* Draw icons */
  for (i = 0; i < MAX_ICONS; i++)
    {
      EntryIconInfo *icon_info = priv->icons[i];

      if (icon_info != NULL)
        bobgui_widget_snapshot_child (widget, icon_info->widget, snapshot);
    }
}

/**
 * bobgui_entry_grab_focus_without_selecting:
 * @entry: a `BobguiEntry`
 *
 * Causes @entry to have keyboard focus.
 *
 * It behaves like [method@Bobgui.Widget.grab_focus], except that it doesn't
 * select the contents of the entry. You only want to call this on some
 * special entries which the user usually doesn't want to replace all text
 * in, such as search-as-you-type entries.
 *
 * Returns: %TRUE if focus is now inside @self
 */
gboolean
bobgui_entry_grab_focus_without_selecting (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), FALSE);

  return bobgui_text_grab_focus_without_selecting (BOBGUI_TEXT (priv->text));
}

static void
bobgui_entry_direction_changed (BobguiWidget        *widget,
                             BobguiTextDirection  previous_dir)
{
  BobguiEntry *entry = BOBGUI_ENTRY (widget);

  update_icon_style (widget, BOBGUI_ENTRY_ICON_PRIMARY);
  update_icon_style (widget, BOBGUI_ENTRY_ICON_SECONDARY);

  update_node_ordering (entry);

  BOBGUI_WIDGET_CLASS (bobgui_entry_parent_class)->direction_changed (widget, previous_dir);
}

/* BobguiCellEditable method implementations
 */

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
bobgui_cell_editable_entry_activated (BobguiEntry *entry, gpointer data)
{
  bobgui_cell_editable_editing_done (BOBGUI_CELL_EDITABLE (entry));
  bobgui_cell_editable_remove_widget (BOBGUI_CELL_EDITABLE (entry));
}

static gboolean
bobgui_cell_editable_entry_key_pressed (BobguiEventControllerKey *key,
                                     guint                  keyval,
                                     guint                  keycode,
                                     GdkModifierType        modifiers,
                                     BobguiEntry              *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  if (keyval == GDK_KEY_Escape)
    {
      priv->editing_canceled = TRUE;
      bobgui_cell_editable_editing_done (BOBGUI_CELL_EDITABLE (entry));
      bobgui_cell_editable_remove_widget (BOBGUI_CELL_EDITABLE (entry));

      return GDK_EVENT_STOP;
    }

  /* override focus */
  if (keyval == GDK_KEY_Up || keyval == GDK_KEY_Down)
    {
      bobgui_cell_editable_editing_done (BOBGUI_CELL_EDITABLE (entry));
      bobgui_cell_editable_remove_widget (BOBGUI_CELL_EDITABLE (entry));

      return GDK_EVENT_STOP;
    }

  return GDK_EVENT_PROPAGATE;
}

static void
bobgui_entry_start_editing (BobguiCellEditable *cell_editable,
			 GdkEvent        *event)
{
  g_signal_connect (cell_editable, "activate",
		    G_CALLBACK (bobgui_cell_editable_entry_activated), NULL);
  g_signal_connect (bobgui_entry_get_key_controller (BOBGUI_ENTRY (cell_editable)),
                    "key-pressed",
                    G_CALLBACK (bobgui_cell_editable_entry_key_pressed),
                    cell_editable);
}

G_GNUC_END_IGNORE_DEPRECATIONS

/* Internal functions
 */

/**
 * bobgui_entry_reset_im_context:
 * @entry: a `BobguiEntry`
 *
 * Reset the input method context of the entry if needed.
 *
 * This can be necessary in the case where modifying the buffer
 * would confuse on-going input method behavior.
 */
void
bobgui_entry_reset_im_context (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));

  bobgui_text_reset_im_context (BOBGUI_TEXT (priv->text));
}

static void
bobgui_entry_clear_icon (BobguiEntry             *entry,
                      BobguiEntryIconPosition  icon_pos)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info = priv->icons[icon_pos];
  BobguiImageType storage_type;

  if (icon_info == NULL)
    return;

  storage_type = bobgui_image_get_storage_type (BOBGUI_IMAGE (icon_info->widget));

  if (storage_type == BOBGUI_IMAGE_EMPTY)
    return;

  g_object_freeze_notify (G_OBJECT (entry));

  switch (storage_type)
    {
    case BOBGUI_IMAGE_PAINTABLE:
      g_object_notify_by_pspec (G_OBJECT (entry),
                                entry_props[icon_pos == BOBGUI_ENTRY_ICON_PRIMARY
                                            ? PROP_PAINTABLE_PRIMARY
                                            : PROP_PAINTABLE_SECONDARY]);
      break;

    case BOBGUI_IMAGE_ICON_NAME:
      g_object_notify_by_pspec (G_OBJECT (entry),
                                entry_props[icon_pos == BOBGUI_ENTRY_ICON_PRIMARY
                                            ? PROP_ICON_NAME_PRIMARY
                                            : PROP_ICON_NAME_SECONDARY]);
      break;

    case BOBGUI_IMAGE_GICON:
      g_object_notify_by_pspec (G_OBJECT (entry),
                                entry_props[icon_pos == BOBGUI_ENTRY_ICON_PRIMARY
                                            ? PROP_GICON_PRIMARY
                                            : PROP_GICON_SECONDARY]);
      break;

    case BOBGUI_IMAGE_EMPTY:
    default:
      g_assert_not_reached ();
      break;
    }

  bobgui_image_clear (BOBGUI_IMAGE (icon_info->widget));

  g_object_notify_by_pspec (G_OBJECT (entry),
                            entry_props[icon_pos == BOBGUI_ENTRY_ICON_PRIMARY
                            ? PROP_STORAGE_TYPE_PRIMARY
                            : PROP_STORAGE_TYPE_SECONDARY]);

  g_object_thaw_notify (G_OBJECT (entry));
}

/* Public API
 */

/**
 * bobgui_entry_new:
 *
 * Creates a new entry.
 *
 * Returns: a new `BobguiEntry`.
 */
BobguiWidget*
bobgui_entry_new (void)
{
  return g_object_new (BOBGUI_TYPE_ENTRY, NULL);
}

/**
 * bobgui_entry_new_with_buffer:
 * @buffer: The buffer to use for the new `BobguiEntry`.
 *
 * Creates a new entry with the specified text buffer.
 *
 * Returns: a new `BobguiEntry`
 */
BobguiWidget*
bobgui_entry_new_with_buffer (BobguiEntryBuffer *buffer)
{
  g_return_val_if_fail (BOBGUI_IS_ENTRY_BUFFER (buffer), NULL);

  return g_object_new (BOBGUI_TYPE_ENTRY, "buffer", buffer, NULL);
}

static BobguiEntryBuffer*
get_buffer (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  return bobgui_text_get_buffer (BOBGUI_TEXT (priv->text));
}

/**
 * bobgui_entry_get_buffer:
 * @entry: a `BobguiEntry`
 *
 * Get the `BobguiEntryBuffer` object which holds the text for
 * this widget.
 *
 * Returns: (transfer none): A `BobguiEntryBuffer` object.
 */
BobguiEntryBuffer*
bobgui_entry_get_buffer (BobguiEntry *entry)
{
  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), NULL);

  return get_buffer (entry);
}

/**
 * bobgui_entry_set_buffer:
 * @entry: a `BobguiEntry`
 * @buffer: a `BobguiEntryBuffer`
 *
 * Set the `BobguiEntryBuffer` object which holds the text for
 * this widget.
 */
void
bobgui_entry_set_buffer (BobguiEntry       *entry,
                      BobguiEntryBuffer *buffer)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));

  bobgui_text_set_buffer (BOBGUI_TEXT (priv->text), buffer);
}

/**
 * bobgui_entry_set_visibility:
 * @entry: a `BobguiEntry`
 * @visible: %TRUE if the contents of the entry are displayed as plaintext
 *
 * Sets whether the contents of the entry are visible or not.
 *
 * When visibility is set to %FALSE, characters are displayed
 * as the invisible char, and will also appear that way when
 * the text in the entry widget is copied elsewhere.
 *
 * By default, BOBGUI picks the best invisible character available
 * in the current font, but it can be changed with
 * [method@Bobgui.Entry.set_invisible_char].
 *
 * Note that you probably want to set [property@Bobgui.Entry:input-purpose]
 * to %BOBGUI_INPUT_PURPOSE_PASSWORD or %BOBGUI_INPUT_PURPOSE_PIN to
 * inform input methods about the purpose of this entry,
 * in addition to setting visibility to %FALSE.
 */
void
bobgui_entry_set_visibility (BobguiEntry *entry,
			  gboolean  visible)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));

  bobgui_text_set_visibility (BOBGUI_TEXT (priv->text), visible);
}

/**
 * bobgui_entry_get_visibility:
 * @entry: a `BobguiEntry`
 *
 * Retrieves whether the text in @entry is visible.
 *
 * See [method@Bobgui.Entry.set_visibility].
 *
 * Returns: %TRUE if the text is currently visible
 */
gboolean
bobgui_entry_get_visibility (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), FALSE);

  return bobgui_text_get_visibility (BOBGUI_TEXT (priv->text));
}

/**
 * bobgui_entry_set_invisible_char:
 * @entry: a `BobguiEntry`
 * @ch: a Unicode character
 *
 * Sets the character to use in place of the actual text
 * in “password mode”.
 *
 * See [method@Bobgui.Entry.set_visibility] for how to enable
 * “password mode”.
 *
 * By default, BOBGUI picks the best invisible char available in
 * the current font. If you set the invisible char to 0, then
 * the user will get no feedback at all; there will be no text
 * on the screen as they type.
 */
void
bobgui_entry_set_invisible_char (BobguiEntry *entry,
                              gunichar  ch)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));

  bobgui_text_set_invisible_char (BOBGUI_TEXT (priv->text), ch);
}

/**
 * bobgui_entry_get_invisible_char:
 * @entry: a `BobguiEntry`
 *
 * Retrieves the character displayed in place of the actual text
 * in “password mode”.
 *
 * Returns: the current invisible char, or 0, if the entry does not
 *   show invisible text at all.
 */
gunichar
bobgui_entry_get_invisible_char (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), 0);

  return bobgui_text_get_invisible_char (BOBGUI_TEXT (priv->text));
}

/**
 * bobgui_entry_unset_invisible_char:
 * @entry: a `BobguiEntry`
 *
 * Unsets the invisible char, so that the default invisible char
 * is used again. See [method@Bobgui.Entry.set_invisible_char].
 */
void
bobgui_entry_unset_invisible_char (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));

  bobgui_text_unset_invisible_char (BOBGUI_TEXT (priv->text));
}

/**
 * bobgui_entry_set_overwrite_mode:
 * @entry: a `BobguiEntry`
 * @overwrite: new value
 *
 * Sets whether the text is overwritten when typing in the `BobguiEntry`.
 */
void
bobgui_entry_set_overwrite_mode (BobguiEntry *entry,
                              gboolean  overwrite)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));

  bobgui_text_set_overwrite_mode (BOBGUI_TEXT (priv->text), overwrite);
}

/**
 * bobgui_entry_get_overwrite_mode:
 * @entry: a `BobguiEntry`
 *
 * Gets whether the `BobguiEntry` is in overwrite mode.
 *
 * Returns: whether the text is overwritten when typing.
 */
gboolean
bobgui_entry_get_overwrite_mode (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), FALSE);

  return bobgui_text_get_overwrite_mode (BOBGUI_TEXT (priv->text));

}

/**
 * bobgui_entry_set_max_length:
 * @entry: a `BobguiEntry`
 * @max: the maximum length of the entry, or 0 for no maximum.
 *   (other than the maximum length of entries.) The value passed in will
 *   be clamped to the range 0-65536.
 *
 * Sets the maximum allowed length of the contents of the widget.
 *
 * If the current contents are longer than the given length, then
 * they will be truncated to fit. The length is in characters.
 *
 * This is equivalent to getting @entry's `BobguiEntryBuffer` and
 * calling [method@Bobgui.EntryBuffer.set_max_length] on it.
 */
void
bobgui_entry_set_max_length (BobguiEntry     *entry,
                          int           max)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));

  bobgui_text_set_max_length (BOBGUI_TEXT (priv->text), max);
}

/**
 * bobgui_entry_get_max_length:
 * @entry: a `BobguiEntry`
 *
 * Retrieves the maximum allowed length of the text in @entry.
 *
 * See [method@Bobgui.Entry.set_max_length].
 *
 * Returns: the maximum allowed number of characters
 *   in `BobguiEntry`, or 0 if there is no maximum.
 */
int
bobgui_entry_get_max_length (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), 0);

  return bobgui_text_get_max_length (BOBGUI_TEXT (priv->text));
}

/**
 * bobgui_entry_get_text_length:
 * @entry: a `BobguiEntry`
 *
 * Retrieves the current length of the text in @entry.
 *
 * This is equivalent to getting @entry's `BobguiEntryBuffer`
 * and calling [method@Bobgui.EntryBuffer.get_length] on it.
 *
 * Returns: the current number of characters
 *   in `BobguiEntry`, or 0 if there are none.
 */
guint16
bobgui_entry_get_text_length (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), 0);

  return bobgui_text_get_text_length (BOBGUI_TEXT (priv->text));
}

/**
 * bobgui_entry_set_activates_default:
 * @entry: a `BobguiEntry`
 * @setting: %TRUE to activate window’s default widget on Enter keypress
 *
 * Sets whether pressing Enter in the @entry will activate the default
 * widget for the window containing the entry.
 *
 * This usually means that the dialog containing the entry will be closed,
 * since the default widget is usually one of the dialog buttons.
 */
void
bobgui_entry_set_activates_default (BobguiEntry *entry,
                                 gboolean  setting)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));

  bobgui_text_set_activates_default (BOBGUI_TEXT (priv->text), setting);
}

/**
 * bobgui_entry_get_activates_default:
 * @entry: a `BobguiEntry`
 *
 * Retrieves the value set by bobgui_entry_set_activates_default().
 *
 * Returns: %TRUE if the entry will activate the default widget
 */
gboolean
bobgui_entry_get_activates_default (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), FALSE);

  return bobgui_text_get_activates_default (BOBGUI_TEXT (priv->text));
}

/**
 * bobgui_entry_set_has_frame:
 * @entry: a `BobguiEntry`
 * @setting: new value
 *
 * Sets whether the entry has a beveled frame around it.
 */
void
bobgui_entry_set_has_frame (BobguiEntry *entry,
                         gboolean  setting)
{
  g_return_if_fail (BOBGUI_IS_ENTRY (entry));

  setting = (setting != FALSE);

  if (setting == bobgui_entry_get_has_frame (entry))
    return;

  if (setting)
    bobgui_widget_remove_css_class (BOBGUI_WIDGET (entry), "flat");
  else
    bobgui_widget_add_css_class (BOBGUI_WIDGET (entry), "flat");

  g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_HAS_FRAME]);
}

/**
 * bobgui_entry_get_has_frame:
 * @entry: a `BobguiEntry`
 *
 * Gets the value set by bobgui_entry_set_has_frame().
 *
 * Returns: whether the entry has a beveled frame
 */
gboolean
bobgui_entry_get_has_frame (BobguiEntry *entry)
{
  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), FALSE);

  return !bobgui_widget_has_css_class (BOBGUI_WIDGET (entry), "flat");
}

/**
 * bobgui_entry_set_alignment:
 * @entry: a `BobguiEntry`
 * @xalign: The horizontal alignment, from 0 (left) to 1 (right).
 *   Reversed for RTL layouts
 *
 * Sets the alignment for the contents of the entry.
 *
 * This controls the horizontal positioning of the contents when
 * the displayed text is shorter than the width of the entry.
 *
 * See also: [property@Bobgui.Editable:xalign]
 */
void
bobgui_entry_set_alignment (BobguiEntry *entry,
                         float     xalign)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));

  bobgui_editable_set_alignment (BOBGUI_EDITABLE (priv->text), xalign);
}

/**
 * bobgui_entry_get_alignment:
 * @entry: a `BobguiEntry`
 *
 * Gets the value set by bobgui_entry_set_alignment().
 *
 * See also: [property@Bobgui.Editable:xalign]
 *
 * Returns: the alignment
 */
float
bobgui_entry_get_alignment (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), 0.0);

  return bobgui_editable_get_alignment (BOBGUI_EDITABLE (priv->text));
}

/**
 * bobgui_entry_set_icon_from_paintable:
 * @entry: a `BobguiEntry`
 * @icon_pos: Icon position
 * @paintable: (nullable): A `GdkPaintable`
 *
 * Sets the icon shown in the specified position using a `GdkPaintable`.
 *
 * If @paintable is %NULL, no icon will be shown in the specified position.
 */
void
bobgui_entry_set_icon_from_paintable (BobguiEntry             *entry,
                                 BobguiEntryIconPosition  icon_pos,
                                 GdkPaintable           *paintable)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  g_object_freeze_notify (G_OBJECT (entry));

  if (paintable)
    {
      EntryIconInfo *icon_info;

      if ((icon_info = priv->icons[icon_pos]) == NULL)
        icon_info = construct_icon_info (BOBGUI_WIDGET (entry), icon_pos);

      g_object_ref (paintable);

      bobgui_image_set_from_paintable (BOBGUI_IMAGE (icon_info->widget), paintable);

      if (icon_pos == BOBGUI_ENTRY_ICON_PRIMARY)
        {
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_PAINTABLE_PRIMARY]);
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_STORAGE_TYPE_PRIMARY]);
        }
      else
        {
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_PAINTABLE_SECONDARY]);
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_STORAGE_TYPE_SECONDARY]);
        }

      g_object_unref (paintable);
    }
  else
    bobgui_entry_clear_icon (entry, icon_pos);

  if (bobgui_widget_get_visible (BOBGUI_WIDGET (entry)))
    bobgui_widget_queue_resize (BOBGUI_WIDGET (entry));

  g_object_thaw_notify (G_OBJECT (entry));
}

/**
 * bobgui_entry_set_icon_from_icon_name:
 * @entry: A `BobguiEntry`
 * @icon_pos: The position at which to set the icon
 * @icon_name: (nullable): An icon name
 *
 * Sets the icon shown in the entry at the specified position
 * from the current icon theme.
 *
 * If the icon name isn’t known, a “broken image” icon will be
 * displayed instead.
 *
 * If @icon_name is %NULL, no icon will be shown in the
 * specified position.
 */
void
bobgui_entry_set_icon_from_icon_name (BobguiEntry             *entry,
                                   BobguiEntryIconPosition  icon_pos,
                                   const char           *icon_name)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info;

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (BOBGUI_WIDGET (entry), icon_pos);

  g_object_freeze_notify (G_OBJECT (entry));


  if (icon_name != NULL)
    {
      bobgui_image_set_from_icon_name (BOBGUI_IMAGE (icon_info->widget), icon_name);

      if (icon_pos == BOBGUI_ENTRY_ICON_PRIMARY)
        {
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_ICON_NAME_PRIMARY]);
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_STORAGE_TYPE_PRIMARY]);
        }
      else
        {
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_ICON_NAME_SECONDARY]);
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_STORAGE_TYPE_SECONDARY]);
        }
    }
  else
    bobgui_entry_clear_icon (entry, icon_pos);

  if (bobgui_widget_get_visible (BOBGUI_WIDGET (entry)))
    bobgui_widget_queue_resize (BOBGUI_WIDGET (entry));

  g_object_thaw_notify (G_OBJECT (entry));
}

/**
 * bobgui_entry_set_icon_from_gicon:
 * @entry: A `BobguiEntry`
 * @icon_pos: The position at which to set the icon
 * @icon: (nullable): The icon to set
 *
 * Sets the icon shown in the entry at the specified position
 * from the current icon theme.
 *
 * If the icon isn’t known, a “broken image” icon will be
 * displayed instead.
 *
 * If @icon is %NULL, no icon will be shown in the
 * specified position.
 */
void
bobgui_entry_set_icon_from_gicon (BobguiEntry             *entry,
                               BobguiEntryIconPosition  icon_pos,
                               GIcon                *icon)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info;

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (BOBGUI_WIDGET (entry), icon_pos);

  g_object_freeze_notify (G_OBJECT (entry));

  if (icon)
    {
      bobgui_image_set_from_gicon (BOBGUI_IMAGE (icon_info->widget), icon);

      if (icon_pos == BOBGUI_ENTRY_ICON_PRIMARY)
        {
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_GICON_PRIMARY]);
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_STORAGE_TYPE_PRIMARY]);
        }
      else
        {
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_GICON_SECONDARY]);
          g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_STORAGE_TYPE_SECONDARY]);
        }
    }
  else
    bobgui_entry_clear_icon (entry, icon_pos);

  if (bobgui_widget_get_visible (BOBGUI_WIDGET (entry)))
    bobgui_widget_queue_resize (BOBGUI_WIDGET (entry));

  g_object_thaw_notify (G_OBJECT (entry));
}

/**
 * bobgui_entry_set_icon_activatable:
 * @entry: A `BobguiEntry`
 * @icon_pos: Icon position
 * @activatable: %TRUE if the icon should be activatable
 *
 * Sets whether the icon is activatable.
 */
void
bobgui_entry_set_icon_activatable (BobguiEntry             *entry,
                                BobguiEntryIconPosition  icon_pos,
                                gboolean              activatable)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info;

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (BOBGUI_WIDGET (entry), icon_pos);

  activatable = activatable != FALSE;

  if (icon_info->nonactivatable != !activatable)
    {
      icon_info->nonactivatable = !activatable;

      g_object_notify_by_pspec (G_OBJECT (entry),
                                entry_props[icon_pos == BOBGUI_ENTRY_ICON_PRIMARY
                                            ? PROP_ACTIVATABLE_PRIMARY
                                            : PROP_ACTIVATABLE_SECONDARY]);
    }
  update_extra_menu (entry);
}

/**
 * bobgui_entry_get_icon_activatable:
 * @entry: a `BobguiEntry`
 * @icon_pos: Icon position
 *
 * Returns whether the icon is activatable.
 *
 * Returns: %TRUE if the icon is activatable.
 */
gboolean
bobgui_entry_get_icon_activatable (BobguiEntry             *entry,
                                BobguiEntryIconPosition  icon_pos)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info;

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), FALSE);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), FALSE);

  icon_info = priv->icons[icon_pos];

  return (!icon_info || !icon_info->nonactivatable);
}

/**
 * bobgui_entry_get_icon_paintable:
 * @entry: A `BobguiEntry`
 * @icon_pos: Icon position
 *
 * Retrieves the `GdkPaintable` used for the icon.
 *
 * If no `GdkPaintable` was used for the icon, %NULL is returned.
 *
 * Returns: (transfer none) (nullable): A `GdkPaintable`
 *   if no icon is set for this position or the icon set is not
 *   a `GdkPaintable`.
 */
GdkPaintable *
bobgui_entry_get_icon_paintable (BobguiEntry             *entry,
                              BobguiEntryIconPosition  icon_pos)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info;

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), NULL);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), NULL);

  icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return NULL;

  return bobgui_image_get_paintable (BOBGUI_IMAGE (icon_info->widget));
}

/**
 * bobgui_entry_get_icon_gicon:
 * @entry: A `BobguiEntry`
 * @icon_pos: Icon position
 *
 * Retrieves the `GIcon` used for the icon.
 *
 * %NULL will be returned if there is no icon or if the icon was
 * set by some other method (e.g., by `GdkPaintable` or icon name).
 *
 * Returns: (transfer none) (nullable): A `GIcon`
 */
GIcon *
bobgui_entry_get_icon_gicon (BobguiEntry             *entry,
                          BobguiEntryIconPosition  icon_pos)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info;

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), NULL);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), NULL);

  icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return NULL;

  return bobgui_image_get_gicon (BOBGUI_IMAGE (icon_info->widget));
}

/**
 * bobgui_entry_get_icon_name:
 * @entry: A `BobguiEntry`
 * @icon_pos: Icon position
 *
 * Retrieves the icon name used for the icon.
 *
 * %NULL is returned if there is no icon or if the icon was set
 * by some other method (e.g., by `GdkPaintable` or gicon).
 *
 * Returns: (nullable): An icon name
 */
const char *
bobgui_entry_get_icon_name (BobguiEntry             *entry,
                         BobguiEntryIconPosition  icon_pos)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info;

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), NULL);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), NULL);

  icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return NULL;

  return bobgui_image_get_icon_name (BOBGUI_IMAGE (icon_info->widget));
}

/**
 * bobgui_entry_set_icon_sensitive:
 * @entry: A `BobguiEntry`
 * @icon_pos: Icon position
 * @sensitive: Specifies whether the icon should appear
 *   sensitive or insensitive
 *
 * Sets the sensitivity for the specified icon.
 */
void
bobgui_entry_set_icon_sensitive (BobguiEntry             *entry,
                              BobguiEntryIconPosition  icon_pos,
                              gboolean              sensitive)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info;

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (BOBGUI_WIDGET (entry), icon_pos);

  if (bobgui_widget_get_sensitive (icon_info->widget) != sensitive)
    {
      bobgui_widget_set_sensitive (icon_info->widget, sensitive);

      g_object_notify_by_pspec (G_OBJECT (entry),
                                entry_props[icon_pos == BOBGUI_ENTRY_ICON_PRIMARY
                                            ? PROP_SENSITIVE_PRIMARY
                                            : PROP_SENSITIVE_SECONDARY]);
    }
  update_extra_menu (entry);
}

/**
 * bobgui_entry_get_icon_sensitive:
 * @entry: a `BobguiEntry`
 * @icon_pos: Icon position
 *
 * Returns whether the icon appears sensitive or insensitive.
 *
 * Returns: %TRUE if the icon is sensitive.
 */
gboolean
bobgui_entry_get_icon_sensitive (BobguiEntry             *entry,
                              BobguiEntryIconPosition  icon_pos)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info;

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), TRUE);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), TRUE);

  icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return TRUE; /* Default of the property */

  return bobgui_widget_get_sensitive (icon_info->widget);
}

/**
 * bobgui_entry_get_icon_storage_type:
 * @entry: a `BobguiEntry`
 * @icon_pos: Icon position
 *
 * Gets the type of representation being used by the icon
 * to store image data.
 *
 * If the icon has no image data, the return value will
 * be %BOBGUI_IMAGE_EMPTY.
 *
 * Returns: image representation being used
 */
BobguiImageType
bobgui_entry_get_icon_storage_type (BobguiEntry             *entry,
                                 BobguiEntryIconPosition  icon_pos)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info;

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), BOBGUI_IMAGE_EMPTY);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), BOBGUI_IMAGE_EMPTY);

  icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return BOBGUI_IMAGE_EMPTY;

  return bobgui_image_get_storage_type (BOBGUI_IMAGE (icon_info->widget));
}

/**
 * bobgui_entry_get_icon_at_pos:
 * @entry: a `BobguiEntry`
 * @x: the x coordinate of the position to find, relative to @entry
 * @y: the y coordinate of the position to find, relative to @entry
 *
 * Finds the icon at the given position and return its index.
 *
 * The position’s coordinates are relative to the @entry’s
 * top left corner. If @x, @y doesn’t lie inside an icon,
 * -1 is returned. This function is intended for use in a
 * [signal@Bobgui.Widget::query-tooltip] signal handler.
 *
 * Returns: the index of the icon at the given position, or -1
 */
int
bobgui_entry_get_icon_at_pos (BobguiEntry *entry,
                           int       x,
                           int       y)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  guint i;

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), -1);

  for (i = 0; i < MAX_ICONS; i++)
    {
      EntryIconInfo *icon_info = priv->icons[i];
      graphene_point_t p;

      if (icon_info == NULL)
        continue;

      if (!bobgui_widget_compute_point (BOBGUI_WIDGET (entry), icon_info->widget,
                                     &GRAPHENE_POINT_INIT (x, y), &p))
        continue;

      if (bobgui_widget_contains (icon_info->widget, p.x, p.y))
        return i;
    }

  return -1;
}

/**
 * bobgui_entry_set_icon_drag_source:
 * @entry: a `BobguiEntry`
 * @icon_pos: icon position
 * @provider: a `GdkContentProvider`
 * @actions: a bitmask of the allowed drag actions
 *
 * Sets up the icon at the given position as drag source.
 *
 * This makes it so that BOBGUI will start a drag
 * operation when the user clicks and drags the icon.
 */
void
bobgui_entry_set_icon_drag_source (BobguiEntry             *entry,
                                BobguiEntryIconPosition  icon_pos,
                                GdkContentProvider   *provider,
                                GdkDragAction         actions)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info;

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (BOBGUI_WIDGET (entry), icon_pos);

  g_set_object (&icon_info->content, provider);
  icon_info->actions = actions;
}

/**
 * bobgui_entry_get_current_icon_drag_source:
 * @entry: a `BobguiEntry`
 *
 * Returns the index of the icon which is the source of the
 * current  DND operation, or -1.
 *
 * Returns: index of the icon which is the source of the
 *   current DND operation, or -1.
 */
int
bobgui_entry_get_current_icon_drag_source (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info = NULL;
  int i;

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), -1);

  for (i = 0; i < MAX_ICONS; i++)
    {
      if ((icon_info = priv->icons[i]))
        {
          if (icon_info->in_drag)
            return i;
        }
    }

  return -1;
}

/**
 * bobgui_entry_get_icon_area:
 * @entry: A `BobguiEntry`
 * @icon_pos: Icon position
 * @icon_area: (out): Return location for the icon’s area
 *
 * Gets the area where entry’s icon at @icon_pos is drawn.
 *
 * This function is useful when drawing something to the
 * entry in a draw callback.
 *
 * If the entry is not realized or has no icon at the given
 * position, @icon_area is filled with zeros. Otherwise,
 * @icon_area will be filled with the icon's allocation,
 * relative to @entry's allocation.
 */
void
bobgui_entry_get_icon_area (BobguiEntry             *entry,
                         BobguiEntryIconPosition  icon_pos,
                         GdkRectangle         *icon_area)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info;
  graphene_rect_t r;

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));
  g_return_if_fail (icon_area != NULL);

  icon_info = priv->icons[icon_pos];

  if (icon_info &&
      bobgui_widget_compute_bounds (icon_info->widget, BOBGUI_WIDGET (entry), &r))
    {
      *icon_area = (GdkRectangle){
        floorf (r.origin.x),
        floorf (r.origin.y),
        ceilf (r.size.width),
        ceilf (r.size.height),
      };
    }
  else
    {
      icon_area->x = 0;
      icon_area->y = 0;
      icon_area->width = 0;
      icon_area->height = 0;
    }
}

static void
ensure_has_tooltip (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  const char *text = bobgui_widget_get_tooltip_text (BOBGUI_WIDGET (entry));
  gboolean has_tooltip = text != NULL;

  if (!has_tooltip)
    {
      int i;

      for (i = 0; i < MAX_ICONS; i++)
        {
          EntryIconInfo *icon_info = priv->icons[i];

          if (icon_info != NULL && icon_info->tooltip != NULL)
            {
              has_tooltip = TRUE;
              break;
            }
        }
    }

  bobgui_widget_set_has_tooltip (BOBGUI_WIDGET (entry), has_tooltip);
}

/**
 * bobgui_entry_get_icon_tooltip_text:
 * @entry: a `BobguiEntry`
 * @icon_pos: the icon position
 *
 * Gets the contents of the tooltip on the icon at the specified
 * position in @entry.
 *
 * Returns: (nullable) (transfer full): the tooltip text
 */
char *
bobgui_entry_get_icon_tooltip_text (BobguiEntry             *entry,
                                 BobguiEntryIconPosition  icon_pos)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info;
  char *text = NULL;

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), NULL);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), NULL);

  icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return NULL;
 
  if (icon_info->tooltip && 
      !pango_parse_markup (icon_info->tooltip, -1, 0, NULL, &text, NULL, NULL))
    g_assert (NULL == text); /* text should still be NULL in case of markup errors */

  return text;
}

/**
 * bobgui_entry_set_icon_tooltip_text:
 * @entry: a `BobguiEntry`
 * @icon_pos: the icon position
 * @tooltip: (nullable): the contents of the tooltip for the icon
 *
 * Sets @tooltip as the contents of the tooltip for the icon
 * at the specified position.
 *
 * Use %NULL for @tooltip to remove an existing tooltip.
 *
 * See also [method@Bobgui.Widget.set_tooltip_text] and
 * [method@Bobgui.Entry.set_icon_tooltip_markup].
 *
 * If you unset the widget tooltip via
 * [method@Bobgui.Widget.set_tooltip_text] or
 * [method@Bobgui.Widget.set_tooltip_markup], this sets
 * [property@Bobgui.Widget:has-tooltip] to %FALSE, which suppresses
 * icon tooltips too. You can resolve this by then calling
 * [method@Bobgui.Widget.set_has_tooltip] to set
 * [property@Bobgui.Widget:has-tooltip] back to %TRUE, or
 * setting at least one non-empty tooltip on any icon
 * achieves the same result.
 */
void
bobgui_entry_set_icon_tooltip_text (BobguiEntry             *entry,
                                 BobguiEntryIconPosition  icon_pos,
                                 const char           *tooltip)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info;

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (BOBGUI_WIDGET (entry), icon_pos);

  g_free (icon_info->tooltip);

  /* Treat an empty string as a NULL string,
   * because an empty string would be useless for a tooltip:
   */
  if (tooltip && tooltip[0] == '\0')
    tooltip = NULL;

  icon_info->tooltip = tooltip ? g_markup_escape_text (tooltip, -1) : NULL;

  ensure_has_tooltip (entry);

  g_object_notify_by_pspec (G_OBJECT (entry),
                            entry_props[icon_pos == BOBGUI_ENTRY_ICON_PRIMARY
                                        ? PROP_TOOLTIP_TEXT_PRIMARY
                                        : PROP_TOOLTIP_TEXT_SECONDARY]);
}

/**
 * bobgui_entry_get_icon_tooltip_markup:
 * @entry: a `BobguiEntry`
 * @icon_pos: the icon position
 *
 * Gets the contents of the tooltip on the icon at the specified
 * position in @entry.
 *
 * Returns: (nullable) (transfer full): the tooltip text
 */
char *
bobgui_entry_get_icon_tooltip_markup (BobguiEntry             *entry,
                                   BobguiEntryIconPosition  icon_pos)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info;

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), NULL);
  g_return_val_if_fail (IS_VALID_ICON_POSITION (icon_pos), NULL);

  icon_info = priv->icons[icon_pos];

  if (!icon_info)
    return NULL;
 
  return g_strdup (icon_info->tooltip);
}

/**
 * bobgui_entry_set_icon_tooltip_markup:
 * @entry: a `BobguiEntry`
 * @icon_pos: the icon position
 * @tooltip: (nullable): the contents of the tooltip for the icon
 *
 * Sets @tooltip as the contents of the tooltip for the icon at
 * the specified position.
 *
 * @tooltip is assumed to be marked up with Pango Markup.
 *
 * Use %NULL for @tooltip to remove an existing tooltip.
 *
 * See also [method@Bobgui.Widget.set_tooltip_markup] and
 * [method@Bobgui.Entry.set_icon_tooltip_text].
 */
void
bobgui_entry_set_icon_tooltip_markup (BobguiEntry             *entry,
                                   BobguiEntryIconPosition  icon_pos,
                                   const char           *tooltip)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info;

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));
  g_return_if_fail (IS_VALID_ICON_POSITION (icon_pos));

  if ((icon_info = priv->icons[icon_pos]) == NULL)
    icon_info = construct_icon_info (BOBGUI_WIDGET (entry), icon_pos);

  g_free (icon_info->tooltip);

  /* Treat an empty string as a NULL string,
   * because an empty string would be useless for a tooltip:
   */
  if (tooltip && tooltip[0] == '\0')
    tooltip = NULL;

  icon_info->tooltip = g_strdup (tooltip);

  ensure_has_tooltip (entry);

  g_object_notify_by_pspec (G_OBJECT (entry),
                            entry_props[icon_pos == BOBGUI_ENTRY_ICON_PRIMARY
                                        ? PROP_TOOLTIP_MARKUP_PRIMARY
                                        : PROP_TOOLTIP_MARKUP_SECONDARY]);
}

static gboolean
bobgui_entry_query_tooltip (BobguiWidget  *widget,
                         int         x,
                         int         y,
                         gboolean    keyboard_tip,
                         BobguiTooltip *tooltip)
{
  BobguiEntry *entry = BOBGUI_ENTRY (widget);
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info;
  int icon_pos;

  if (!keyboard_tip)
    {
      icon_pos = bobgui_entry_get_icon_at_pos (entry, x, y);
      if (icon_pos != -1)
        {
          if ((icon_info = priv->icons[icon_pos]) != NULL)
            {
              if (icon_info->tooltip)
                {
                  bobgui_tooltip_set_markup (tooltip, icon_info->tooltip);
                  return TRUE;
                }

              return FALSE;
            }
        }
    }

  return BOBGUI_WIDGET_CLASS (bobgui_entry_parent_class)->query_tooltip (widget,
                                                                   x, y,
                                                                   keyboard_tip,
                                                                   tooltip);
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * bobgui_entry_set_completion:
 * @entry: A `BobguiEntry`
 * @completion: (nullable): The `BobguiEntryCompletion`
 *
 * Sets @completion to be the auxiliary completion object
 * to use with @entry.
 *
 * All further configuration of the completion mechanism is
 * done on @completion using the `BobguiEntryCompletion` API.
 * Completion is disabled if @completion is set to %NULL.
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
void
bobgui_entry_set_completion (BobguiEntry           *entry,
                          BobguiEntryCompletion *completion)
{
  BobguiEntryCompletion *old;

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));
  g_return_if_fail (!completion || BOBGUI_IS_ENTRY_COMPLETION (completion));

  old = bobgui_entry_get_completion (entry);

  if (old == completion)
    return;
  
  if (old)
    {
      _bobgui_entry_completion_disconnect (old);
      g_object_unref (old);
    }

  if (!completion)
    {
      g_object_set_qdata (G_OBJECT (entry), quark_entry_completion, NULL);
      return;
    }

  /* hook into the entry */
  g_object_ref (completion);

  _bobgui_entry_completion_connect (completion, entry);

  g_object_set_qdata (G_OBJECT (entry), quark_entry_completion, completion);

  g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_COMPLETION]);
}

/**
 * bobgui_entry_get_completion:
 * @entry: A `BobguiEntry`
 *
 * Returns the auxiliary completion object currently
 * in use by @entry.
 *
 * Returns: (nullable) (transfer none): The auxiliary
 *   completion object currently in use by @entry
 *
 * Deprecated: 4.10: BobguiEntryCompletion will be removed in BOBGUI 5.
 */
BobguiEntryCompletion *
bobgui_entry_get_completion (BobguiEntry *entry)
{
  BobguiEntryCompletion *completion;

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), NULL);

  completion = BOBGUI_ENTRY_COMPLETION (g_object_get_qdata (G_OBJECT (entry), quark_entry_completion));

  return completion;
}

G_GNUC_END_IGNORE_DEPRECATIONS

static void
bobgui_entry_ensure_progress_widget (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  if (priv->progress_widget)
    return;

  priv->progress_widget = g_object_new (BOBGUI_TYPE_PROGRESS_BAR,
                                        "css-name", "progress",
                                        NULL);
  bobgui_widget_set_can_target (priv->progress_widget, FALSE);

  bobgui_widget_set_parent (priv->progress_widget, BOBGUI_WIDGET (entry));

  update_node_ordering (entry);
}

/**
 * bobgui_entry_set_progress_fraction:
 * @entry: a `BobguiEntry`
 * @fraction: fraction of the task that’s been completed
 *
 * Causes the entry’s progress indicator to “fill in” the given
 * fraction of the bar.
 *
 * The fraction should be between 0.0 and 1.0, inclusive.
 */
void
bobgui_entry_set_progress_fraction (BobguiEntry *entry,
                                 double    fraction)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  double           old_fraction;

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));

  bobgui_entry_ensure_progress_widget (entry);
  old_fraction = bobgui_progress_bar_get_fraction (BOBGUI_PROGRESS_BAR (priv->progress_widget));
  fraction = CLAMP (fraction, 0.0, 1.0);

  if (fraction != old_fraction)
    {
      bobgui_progress_bar_set_fraction (BOBGUI_PROGRESS_BAR (priv->progress_widget), fraction);
      bobgui_widget_set_visible (priv->progress_widget, fraction > 0);

      g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_PROGRESS_FRACTION]);
    }
}

/**
 * bobgui_entry_get_progress_fraction:
 * @entry: a `BobguiEntry`
 *
 * Returns the current fraction of the task that’s been completed.
 *
 * See [method@Bobgui.Entry.set_progress_fraction].
 *
 * Returns: a fraction from 0.0 to 1.0
 */
double
bobgui_entry_get_progress_fraction (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), 0.0);

  if (priv->progress_widget)
    return bobgui_progress_bar_get_fraction (BOBGUI_PROGRESS_BAR (priv->progress_widget));

  return 0.0;
}

/**
 * bobgui_entry_set_progress_pulse_step:
 * @entry: a `BobguiEntry`
 * @fraction: fraction between 0.0 and 1.0
 *
 * Sets the fraction of total entry width to move the progress
 * bouncing block for each pulse.
 *
 * Use [method@Bobgui.Entry.progress_pulse] to pulse
 * the progress.
 */
void
bobgui_entry_set_progress_pulse_step (BobguiEntry *entry,
                                   double    fraction)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));

  fraction = CLAMP (fraction, 0.0, 1.0);
  bobgui_entry_ensure_progress_widget (entry);


  if (fraction != bobgui_progress_bar_get_pulse_step (BOBGUI_PROGRESS_BAR (priv->progress_widget)))
    {
      bobgui_progress_bar_set_pulse_step (BOBGUI_PROGRESS_BAR (priv->progress_widget), fraction);
      g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_PROGRESS_PULSE_STEP]);
    }
}

/**
 * bobgui_entry_get_progress_pulse_step:
 * @entry: a `BobguiEntry`
 *
 * Retrieves the pulse step set with
 * bobgui_entry_set_progress_pulse_step().
 *
 * Returns: a fraction from 0.0 to 1.0
 */
double
bobgui_entry_get_progress_pulse_step (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), 0.0);

  if (priv->progress_widget)
    return bobgui_progress_bar_get_pulse_step (BOBGUI_PROGRESS_BAR (priv->progress_widget));

  return 0.0;
}

/**
 * bobgui_entry_progress_pulse:
 * @entry: a `BobguiEntry`
 *
 * Indicates that some progress is made, but you don’t
 * know how much.
 *
 * Causes the entry’s progress indicator to enter “activity
 * mode”, where a block bounces back and forth. Each call to
 * bobgui_entry_progress_pulse() causes the block to move by a
 * little bit (the amount of movement per pulse is determined
 * by [method@Bobgui.Entry.set_progress_pulse_step]).
 */
void
bobgui_entry_progress_pulse (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));

  if (priv->progress_widget)
    bobgui_progress_bar_pulse (BOBGUI_PROGRESS_BAR (priv->progress_widget));
}

/**
 * bobgui_entry_set_placeholder_text:
 * @entry: a `BobguiEntry`
 * @text: (nullable): a string to be displayed when @entry is empty and unfocused
 *
 * Sets text to be displayed in @entry when it is empty.
 *
 * This can be used to give a visual hint of the expected
 * contents of the `BobguiEntry`.
 */
void
bobgui_entry_set_placeholder_text (BobguiEntry    *entry,
                                const char *text)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));

  bobgui_text_set_placeholder_text (BOBGUI_TEXT (priv->text), text);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (entry),
                                  BOBGUI_ACCESSIBLE_PROPERTY_PLACEHOLDER, text,
                                  -1);
}

/**
 * bobgui_entry_get_placeholder_text:
 * @entry: a `BobguiEntry`
 *
 * Retrieves the text that will be displayed when @entry
 * is empty and unfocused
 *
 * Returns: (nullable) (transfer none):a pointer to the
 *   placeholder text as a string. This string points to
 *   internally allocated storage in the widget and must
 *   not be freed, modified or stored. If no placeholder
 *   text has been set, %NULL will be returned.
 */
const char *
bobgui_entry_get_placeholder_text (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), NULL);

  return bobgui_text_get_placeholder_text (BOBGUI_TEXT (priv->text));
}

/**
 * bobgui_entry_set_input_purpose:
 * @entry: a `BobguiEntry`
 * @purpose: the purpose
 *
 * Sets the input purpose which can be used by input methods
 * to adjust their behavior.
 */
void
bobgui_entry_set_input_purpose (BobguiEntry        *entry,
                             BobguiInputPurpose  purpose)

{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));

  bobgui_text_set_input_purpose (BOBGUI_TEXT (priv->text), purpose);
}

/**
 * bobgui_entry_get_input_purpose:
 * @entry: a `BobguiEntry`
 *
 * Gets the input purpose of the `BobguiEntry`.
 *
 * Returns: the input purpose
 */
BobguiInputPurpose
bobgui_entry_get_input_purpose (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), BOBGUI_INPUT_PURPOSE_FREE_FORM);

  return bobgui_text_get_input_purpose (BOBGUI_TEXT (priv->text));
}

/**
 * bobgui_entry_set_input_hints:
 * @entry: a `BobguiEntry`
 * @hints: the hints
 *
 * Set additional hints which allow input methods to
 * fine-tune their behavior.
 */
void
bobgui_entry_set_input_hints (BobguiEntry      *entry,
                           BobguiInputHints  hints)

{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));

  bobgui_text_set_input_hints (BOBGUI_TEXT (priv->text), hints);
}

/**
 * bobgui_entry_get_input_hints:
 * @entry: a `BobguiEntry`
 *
 * Gets the input hints of this `BobguiEntry`.
 *
 * Returns: the input hints
 */
BobguiInputHints
bobgui_entry_get_input_hints (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), BOBGUI_INPUT_HINT_NONE);

  return bobgui_text_get_input_hints (BOBGUI_TEXT (priv->text));
}

/**
 * bobgui_entry_set_attributes:
 * @entry: a `BobguiEntry`
 * @attrs: a `PangoAttrList`
 *
 * Sets a `PangoAttrList`.
 *
 * The attributes in the list are applied to the entry text.
 *
 * Since the attributes will be applied to text that changes
 * as the user types, it makes most sense to use attributes
 * with unlimited extent.
 */
void
bobgui_entry_set_attributes (BobguiEntry      *entry,
                          PangoAttrList *attrs)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));

  bobgui_text_set_attributes (BOBGUI_TEXT (priv->text), attrs);
}

/**
 * bobgui_entry_get_attributes:
 * @entry: a `BobguiEntry`
 *
 * Gets the attribute list of the `BobguiEntry`.
 *
 * See [method@Bobgui.Entry.set_attributes].
 *
 * Returns: (transfer none) (nullable): the attribute list
 */
PangoAttrList *
bobgui_entry_get_attributes (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), NULL);

  return bobgui_text_get_attributes (BOBGUI_TEXT (priv->text));
}

/**
 * bobgui_entry_set_tabs:
 * @entry: a `BobguiEntry`
 * @tabs: (nullable): a `PangoTabArray`
 *
 * Sets a `PangoTabArray`.
 *
 * The tabstops in the array are applied to the entry text.
 */

void
bobgui_entry_set_tabs (BobguiEntry      *entry,
                    PangoTabArray *tabs)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));

  bobgui_text_set_tabs (BOBGUI_TEXT (priv->text), tabs);
}

/**
 * bobgui_entry_get_tabs:
 * @entry: a `BobguiEntry`
 *
 * Gets the tabstops of the `BobguiEntry`.
 *
 * See [method@Bobgui.Entry.set_tabs].
 *
 * Returns: (nullable) (transfer none): the tabstops
 */

PangoTabArray *
bobgui_entry_get_tabs (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), NULL);

  return bobgui_text_get_tabs (BOBGUI_TEXT (priv->text));
}

static void
pick_emoji (BobguiEntry *entry,
            int       icon,
            GdkEvent *event,
            gpointer  data)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  if (bobgui_widget_get_ancestor (BOBGUI_WIDGET (entry), BOBGUI_TYPE_EMOJI_CHOOSER) != NULL)
    return;

  if (icon == BOBGUI_ENTRY_ICON_SECONDARY)
    g_signal_emit_by_name (priv->text, "insert-emoji");
}

static void
set_show_emoji_icon (BobguiEntry *entry,
                     gboolean  value)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  if (priv->show_emoji_icon == value)
    return;

  priv->show_emoji_icon = value;

  if (priv->show_emoji_icon)
    {
      bobgui_entry_set_icon_from_icon_name (entry,
                                         BOBGUI_ENTRY_ICON_SECONDARY,
                                         "face-smile-symbolic");

      bobgui_entry_set_icon_sensitive (entry,
                                    BOBGUI_ENTRY_ICON_SECONDARY,
                                    TRUE);

      bobgui_entry_set_icon_activatable (entry,
                                      BOBGUI_ENTRY_ICON_SECONDARY,
                                      TRUE);

      bobgui_entry_set_icon_tooltip_text (entry,
                                       BOBGUI_ENTRY_ICON_SECONDARY,
                                       _("Insert Emoji"));

      g_signal_connect (entry, "icon-press", G_CALLBACK (pick_emoji), NULL);
    }
  else
    {
      g_signal_handlers_disconnect_by_func (entry, pick_emoji, NULL);

      bobgui_entry_set_icon_from_icon_name (entry,
                                         BOBGUI_ENTRY_ICON_SECONDARY,
                                         NULL);

      bobgui_entry_set_icon_tooltip_text (entry,
                                       BOBGUI_ENTRY_ICON_SECONDARY,
                                       NULL);
    }

  g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_SHOW_EMOJI_ICON]);
  bobgui_widget_queue_resize (BOBGUI_WIDGET (entry));
}

BobguiEventController *
bobgui_entry_get_key_controller (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  return bobgui_text_get_key_controller (BOBGUI_TEXT (priv->text));
}

BobguiText *
bobgui_entry_get_text_widget (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  return BOBGUI_TEXT (priv->text);
}

/**
 * bobgui_entry_get_menu_entry_icon_text:
 * @entry: a `BobguiEntry`
 * @icon_pos: either @BOBGUI_ENTRY_ICON_PRIMARY or @BOBGUI_ENTRY_ICON_SECONDARY
 *
 * Gets the text that will be used in the context menu of the `BobguiEntry`
 * when the specified icon is activatable. Selecting this item in the menu
 * results, from all aspects, the same than clicking on the specified icon.
 * This greatly simplifies making accessible applications, because the icons
 * aren't focusable when using keyboard navigation. This is why Bobgui recommends
 * to add the same action to the context menu.
 *
 * Returns: (nullable) (transfer none): the text that will be used in the menu item,
 *   or NULL if no menu item is desired.
 *
 * Since: 4.20
 */
const char *
bobgui_entry_get_menu_entry_icon_text (BobguiEntry             *entry,
                                    BobguiEntryIconPosition  icon_pos)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), NULL);

  switch (icon_pos)
    {
    case BOBGUI_ENTRY_ICON_PRIMARY:
      return priv->menu_entry_icon_primary_text;
    case BOBGUI_ENTRY_ICON_SECONDARY:
      return priv->menu_entry_icon_secondary_text;
    default:
      g_assert_not_reached ();
      return NULL;
    }
}

/**
 * bobgui_entry_set_menu_entry_icon_text:
 * @entry: a `BobguiEntry`
 * @icon_pos: either @BOBGUI_ENTRY_ICON_PRIMARY or @BOBGUI_ENTRY_ICON_SECONDARY
 * @text: the text used for the menu item in the context menu, or NULL to not add a menu item.
 *
 * Sets the text that will be used in the context menu of the `BobguiEntry`
 * when the specified icon is activatable. Selecting this item in the menu
 * results, from all aspects, the same than clicking on the specified icon.
 * This greatly simplifies making accessible applications, because the icons
 * aren't focusable when using keyboard navigation. This is why Bobgui recommends
 * to add the same action to the context menu.
 *
 * Since: 4.20
 */
void
bobgui_entry_set_menu_entry_icon_text (BobguiEntry             *entry,
                                    BobguiEntryIconPosition  icon_pos,
                                    const gchar          *text)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  char **text_p = NULL;
  guint prop_id = 0;

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));

  switch (icon_pos)
    {
    case BOBGUI_ENTRY_ICON_PRIMARY:
      text_p = &priv->menu_entry_icon_primary_text;
      prop_id = PROP_MENU_ENTRY_ICON_PRIMARY_TEXT;
      break;

    case BOBGUI_ENTRY_ICON_SECONDARY:
      text_p = &priv->menu_entry_icon_secondary_text;
      prop_id = PROP_MENU_ENTRY_ICON_SECONDARY_TEXT;
      break;

    default:
      g_assert_not_reached ();
      return;
    }

  if (!g_set_str (text_p, text))
    return;

  update_extra_menu (entry);

  g_object_notify_by_pspec (G_OBJECT (entry), entry_props[prop_id]);
}

static GMenuItem *
create_icon_menu_entry (BobguiEntry             *entry,
                        BobguiEntryIconPosition  icon_pos,
                        const gchar          *menu_entry_text)
{
  GMenuItem *item;
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  const gchar *action_name = (icon_pos == BOBGUI_ENTRY_ICON_PRIMARY)
                             ? "misc.menu_entry_icon_primary"
                             : "misc.menu_entry_icon_secondary";
  EntryIconInfo *icon_info = priv->icons[icon_pos];

  if (icon_info == NULL)
    return NULL;

  if (icon_info->nonactivatable)
    return NULL;

  if (menu_entry_text == NULL)
    return NULL;

  item = g_menu_item_new (menu_entry_text, action_name);
  bobgui_widget_action_set_enabled (BOBGUI_WIDGET (entry), action_name,
                                 bobgui_widget_get_sensitive (icon_info->widget));
  return item;
}

static void
update_extra_menu (BobguiEntry *entry)
{
  BobguiJoinedMenu *joined;
  GMenu *menu;
  GMenu *section;
  GMenuItem *item;
  gboolean has_icon_entries = FALSE;
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  joined = bobgui_joined_menu_new ();
  menu = g_menu_new ();
  section = g_menu_new ();

  item = create_icon_menu_entry (entry,
                                 BOBGUI_ENTRY_ICON_PRIMARY,
                                 priv->menu_entry_icon_primary_text);
  if (item != NULL)
    {
      has_icon_entries = TRUE;
      g_menu_append_item (section, item);
      g_clear_object (&item);
    }

  item = create_icon_menu_entry (entry,
                                 BOBGUI_ENTRY_ICON_SECONDARY,
                                 priv->menu_entry_icon_secondary_text);
  if (item != NULL)
    {
      has_icon_entries = TRUE;
      g_menu_append_item (section, item);
      g_clear_object (&item);
    }

  if (has_icon_entries)
    {
      g_menu_append_section (menu, NULL, G_MENU_MODEL (section));
      bobgui_joined_menu_append_menu (joined, G_MENU_MODEL (menu));
      if (priv->extra_menu != NULL)
        bobgui_joined_menu_append_menu (joined, G_MENU_MODEL (priv->extra_menu));
      bobgui_text_set_extra_menu (BOBGUI_TEXT (priv->text), G_MENU_MODEL (joined));
    }
  else
    {
      bobgui_text_set_extra_menu (BOBGUI_TEXT (priv->text), priv->extra_menu);
    }

  g_object_unref (joined);
  g_object_unref (menu);
  g_object_unref (section);
}

/**
 * bobgui_entry_set_extra_menu:
 * @entry: a `BobguiEntry`
 * @model: (nullable): a `GMenuModel`
 *
 * Sets a menu model to add when constructing
 * the context menu for @entry.
 */
void
bobgui_entry_set_extra_menu (BobguiEntry   *entry,
                          GMenuModel *model)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_if_fail (BOBGUI_IS_ENTRY (entry));

  g_clear_object (&priv->extra_menu);
  priv->extra_menu = g_object_ref (model);

  update_extra_menu (entry);

  g_object_notify_by_pspec (G_OBJECT (entry), entry_props[PROP_EXTRA_MENU]);
}

/**
 * bobgui_entry_get_extra_menu:
 * @entry: a `BobguiEntry`
 *
 * Gets the menu model set with bobgui_entry_set_extra_menu().
 *
 * Returns: (transfer none) (nullable): the menu model
 */
GMenuModel *
bobgui_entry_get_extra_menu (BobguiEntry *entry)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), NULL);

  return priv->extra_menu;
}

/*< private >
 * bobgui_entry_activate_icon:
 * @entry: a `BobguiEntry`
 * @pos: the icon position
 *
 * Activates an icon.
 *
 * This causes the [signal@Bobgui.Entry::icon-press] and
 * [signal@Bobgui.Entry::icon-release] signals to be emitted
 * on the @entry icon at the given @pos, if the icon is
 * set and activatable.
 *
 * Returns: %TRUE if the signal was emitted
 */
gboolean
bobgui_entry_activate_icon (BobguiEntry             *entry,
                         BobguiEntryIconPosition  pos)
{
  BobguiEntryPrivate *priv = bobgui_entry_get_instance_private (entry);
  EntryIconInfo *icon_info;

  g_return_val_if_fail (BOBGUI_IS_ENTRY (entry), FALSE);

  icon_info = priv->icons[pos];

  if (icon_info != NULL &&
      bobgui_image_get_storage_type (BOBGUI_IMAGE (icon_info->widget)) != BOBGUI_IMAGE_EMPTY &&
      !icon_info->nonactivatable)
    {
      g_signal_emit (entry, signals[ICON_PRESS], 0, pos);
      g_signal_emit (entry, signals[ICON_RELEASE], 0, pos);
      return TRUE;
    }

  return FALSE;
}
