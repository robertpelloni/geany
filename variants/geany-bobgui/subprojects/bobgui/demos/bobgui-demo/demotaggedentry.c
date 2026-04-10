/* BOBGUI - The GIMP Toolkit
 * Copyright (C) 2019 Red Hat, Inc.
 *
 * Authors:
 * - Matthias Clasen <mclasen@redhat.com>
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

#include "demotaggedentry.h"

#include <bobgui/bobgui.h>

struct _DemoTaggedEntry
{
  BobguiWidget parent_instance;

  BobguiWidget *text;
};

struct _DemoTaggedEntryClass
{
  BobguiWidgetClass parent_class;
};

static void demo_tagged_entry_editable_init (BobguiEditableInterface *iface);

G_DEFINE_TYPE_WITH_CODE (DemoTaggedEntry, demo_tagged_entry, BOBGUI_TYPE_WIDGET,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_EDITABLE, demo_tagged_entry_editable_init))

static void
demo_tagged_entry_init (DemoTaggedEntry *entry)
{
  BobguiCssProvider *provider;

  entry->text = bobgui_text_new ();
  bobgui_widget_set_hexpand (entry->text, TRUE);
  bobgui_widget_set_vexpand (entry->text, TRUE);
  bobgui_widget_set_parent (entry->text, BOBGUI_WIDGET (entry));
  bobgui_editable_init_delegate (BOBGUI_EDITABLE (entry));
  bobgui_editable_set_width_chars (BOBGUI_EDITABLE (entry->text), 6);
  bobgui_editable_set_max_width_chars (BOBGUI_EDITABLE (entry->text), 6);
  bobgui_widget_add_css_class (BOBGUI_WIDGET (entry), "tagged");

  provider = bobgui_css_provider_new ();
  bobgui_css_provider_load_from_resource (provider, "/tagged_entry/tagstyle.css");
  bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                              BOBGUI_STYLE_PROVIDER (provider),
                                              800);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (entry),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL, "Tagged Entry",
                                  -1);

  g_object_unref (provider);
}

static void
demo_tagged_entry_dispose (GObject *object)
{
  DemoTaggedEntry *entry = DEMO_TAGGED_ENTRY (object);
  BobguiWidget *child;

  if (entry->text)
    bobgui_editable_finish_delegate (BOBGUI_EDITABLE (entry));

  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (entry))))
    bobgui_widget_unparent (child);

  entry->text = NULL;

  G_OBJECT_CLASS (demo_tagged_entry_parent_class)->dispose (object);
}

static void
demo_tagged_entry_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  if (bobgui_editable_delegate_set_property (object, prop_id, value, pspec))
    return;

  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
demo_tagged_entry_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  if (bobgui_editable_delegate_get_property (object, prop_id, value, pspec))
    return;

  G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static gboolean
demo_tagged_entry_grab_focus (BobguiWidget *widget)
{
  DemoTaggedEntry *entry = DEMO_TAGGED_ENTRY (widget);

  return bobgui_widget_grab_focus (entry->text);
}

static void
demo_tagged_entry_class_init (DemoTaggedEntryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = demo_tagged_entry_dispose;
  object_class->get_property = demo_tagged_entry_get_property;
  object_class->set_property = demo_tagged_entry_set_property;

  widget_class->grab_focus = demo_tagged_entry_grab_focus;

  bobgui_editable_install_properties (object_class, 1);

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_BOX_LAYOUT);
  bobgui_widget_class_set_css_name (widget_class, "entry");
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_TEXT_BOX);
}

static BobguiEditable *
demo_tagged_entry_get_delegate (BobguiEditable *editable)
{
  return BOBGUI_EDITABLE (DEMO_TAGGED_ENTRY (editable)->text);
}

static void
demo_tagged_entry_editable_init (BobguiEditableInterface *iface)
{
  iface->get_delegate = demo_tagged_entry_get_delegate;
}

BobguiWidget *
demo_tagged_entry_new (void)
{
  return BOBGUI_WIDGET (g_object_new (DEMO_TYPE_TAGGED_ENTRY, NULL));
}

void
demo_tagged_entry_add_tag (DemoTaggedEntry *entry,
                           BobguiWidget       *tag)
{
  g_return_if_fail (DEMO_IS_TAGGED_ENTRY (entry));

  bobgui_widget_set_parent (tag, BOBGUI_WIDGET (entry));
}

void
demo_tagged_entry_insert_tag_after (DemoTaggedEntry *entry,
                                    BobguiWidget       *tag,
                                    BobguiWidget       *sibling)
{
  g_return_if_fail (DEMO_IS_TAGGED_ENTRY (entry));

  bobgui_widget_insert_after (tag, BOBGUI_WIDGET (entry), sibling);
}

void
demo_tagged_entry_remove_tag (DemoTaggedEntry *entry,
                              BobguiWidget       *tag)
{
  g_return_if_fail (DEMO_IS_TAGGED_ENTRY (entry));

  bobgui_widget_unparent (tag);
}

struct _DemoTaggedEntryTag
{
  BobguiWidget parent;

  BobguiWidget *box;
  BobguiWidget *label;
  BobguiWidget *button;

  gboolean has_close_button;
  char *style;
};

struct _DemoTaggedEntryTagClass
{
  BobguiWidgetClass parent_class;
};

enum {
  PROP_0,
  PROP_LABEL,
  PROP_HAS_CLOSE_BUTTON,
};

enum {
  SIGNAL_CLICKED,
  SIGNAL_BUTTON_CLICKED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (DemoTaggedEntryTag, demo_tagged_entry_tag, BOBGUI_TYPE_WIDGET)

static void
on_released (BobguiGestureClick    *gesture,
             int                 n_press,
             double              x,
             double              y,
             DemoTaggedEntryTag *tag)
{
  g_signal_emit (tag, signals[SIGNAL_CLICKED], 0);
}

static void
demo_tagged_entry_tag_init (DemoTaggedEntryTag *tag)
{
  BobguiGesture *gesture;

  tag->box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_parent (tag->box, BOBGUI_WIDGET (tag));
  tag->label = bobgui_label_new ("");
  bobgui_box_append (BOBGUI_BOX (tag->box), tag->label);

  gesture = bobgui_gesture_click_new ();
  g_signal_connect (gesture, "released", G_CALLBACK (on_released), tag);
  bobgui_widget_add_controller (BOBGUI_WIDGET (tag), BOBGUI_EVENT_CONTROLLER (gesture));
}

static void
demo_tagged_entry_tag_dispose (GObject *object)
{
  DemoTaggedEntryTag *tag = DEMO_TAGGED_ENTRY_TAG (object);

  g_clear_pointer (&tag->box, bobgui_widget_unparent);

  G_OBJECT_CLASS (demo_tagged_entry_tag_parent_class)->dispose (object);
}

static void
demo_tagged_entry_tag_finalize (GObject *object)
{
  DemoTaggedEntryTag *tag = DEMO_TAGGED_ENTRY_TAG (object);

  g_clear_pointer (&tag->box, bobgui_widget_unparent);
  g_clear_pointer (&tag->style, g_free);

  G_OBJECT_CLASS (demo_tagged_entry_tag_parent_class)->finalize (object);
}

static void
demo_tagged_entry_tag_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  DemoTaggedEntryTag *tag = DEMO_TAGGED_ENTRY_TAG (object);

  switch (prop_id)
    {
    case PROP_LABEL:
      demo_tagged_entry_tag_set_label (tag, g_value_get_string (value));
      break;

    case PROP_HAS_CLOSE_BUTTON:
      demo_tagged_entry_tag_set_has_close_button (tag, g_value_get_boolean (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
demo_tagged_entry_tag_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  DemoTaggedEntryTag *tag = DEMO_TAGGED_ENTRY_TAG (object);

  switch (prop_id)
    {
    case PROP_LABEL:
      g_value_set_string (value, demo_tagged_entry_tag_get_label (tag));
      break;

    case PROP_HAS_CLOSE_BUTTON:
      g_value_set_boolean (value, demo_tagged_entry_tag_get_has_close_button (tag));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
demo_tagged_entry_tag_measure (BobguiWidget      *widget,
                               BobguiOrientation  orientation,
                               int             for_size,
                               int            *minimum,
                               int            *natural,
                               int            *minimum_baseline,
                               int            *natural_baseline)
{
  DemoTaggedEntryTag *tag = DEMO_TAGGED_ENTRY_TAG (widget);

  bobgui_widget_measure (tag->box, orientation, for_size,
                      minimum, natural,
                      minimum_baseline, natural_baseline);
}

static void
demo_tagged_entry_tag_size_allocate (BobguiWidget *widget,
                                     int        width,
                                     int        height,
                                     int        baseline)
{
  DemoTaggedEntryTag *tag = DEMO_TAGGED_ENTRY_TAG (widget);

  bobgui_widget_size_allocate (tag->box,
                            &(BobguiAllocation) { 0, 0, width, height },
                            baseline);
}

static void
demo_tagged_entry_tag_class_init (DemoTaggedEntryTagClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = demo_tagged_entry_tag_dispose;
  object_class->finalize = demo_tagged_entry_tag_finalize;
  object_class->set_property = demo_tagged_entry_tag_set_property;
  object_class->get_property = demo_tagged_entry_tag_get_property;

  widget_class->measure = demo_tagged_entry_tag_measure;
  widget_class->size_allocate = demo_tagged_entry_tag_size_allocate;

  signals[SIGNAL_CLICKED] =
      g_signal_new ("clicked",
                    DEMO_TYPE_TAGGED_ENTRY_TAG,
                    G_SIGNAL_RUN_FIRST,
                    0, NULL, NULL, NULL,
                    G_TYPE_NONE, 0);
  signals[SIGNAL_BUTTON_CLICKED] =
      g_signal_new ("button-clicked",
                    DEMO_TYPE_TAGGED_ENTRY_TAG,
                    G_SIGNAL_RUN_FIRST,
                    0, NULL, NULL, NULL,
                    G_TYPE_NONE, 0);

  g_object_class_install_property (object_class, PROP_LABEL,
      g_param_spec_string ("label", "Label", "Label",
                           NULL, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_HAS_CLOSE_BUTTON,
      g_param_spec_boolean ("has-close-button", "Has close button", "Whether this tag has a close button",
                            FALSE, G_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY));

  bobgui_widget_class_set_css_name (widget_class, "tag");
}

DemoTaggedEntryTag *
demo_tagged_entry_tag_new (const char *label)
{
  return DEMO_TAGGED_ENTRY_TAG (g_object_new (DEMO_TYPE_TAGGED_ENTRY_TAG,
                                              "label", label,
                                              NULL));
}

const char *
demo_tagged_entry_tag_get_label (DemoTaggedEntryTag *tag)
{
  g_return_val_if_fail (DEMO_IS_TAGGED_ENTRY_TAG (tag), NULL);

  return bobgui_label_get_label (BOBGUI_LABEL (tag->label));
}

void
demo_tagged_entry_tag_set_label (DemoTaggedEntryTag *tag,
                                 const char         *label)
{
  g_return_if_fail (DEMO_IS_TAGGED_ENTRY_TAG (tag));

  bobgui_label_set_label (BOBGUI_LABEL (tag->label), label);
}

static void
on_button_clicked (BobguiButton          *button,
                   DemoTaggedEntryTag *tag)
{
  g_signal_emit (tag, signals[SIGNAL_BUTTON_CLICKED], 0);
}

void
demo_tagged_entry_tag_set_has_close_button (DemoTaggedEntryTag *tag,
                                            gboolean            has_close_button)
{
  g_return_if_fail (DEMO_IS_TAGGED_ENTRY_TAG (tag));

  if ((tag->button != NULL) == has_close_button)
    return;

  if (!has_close_button && tag->button)
    {
      bobgui_box_remove (BOBGUI_BOX (tag->box), tag->button);
      tag->button = NULL;
    }
  else if (has_close_button && tag->button == NULL)
    {
      BobguiWidget *image;

      image = bobgui_image_new_from_icon_name ("window-close-symbolic");
      g_object_set (image, "accessible-role", BOBGUI_ACCESSIBLE_ROLE_PRESENTATION, NULL);
      tag->button = bobgui_button_new ();
      bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (tag->button),
                                      BOBGUI_ACCESSIBLE_PROPERTY_LABEL, "Close",
                                      -1);
      bobgui_button_set_child (BOBGUI_BUTTON (tag->button), image);
      bobgui_widget_set_halign (tag->button, BOBGUI_ALIGN_CENTER);
      bobgui_widget_set_valign (tag->button, BOBGUI_ALIGN_CENTER);
      bobgui_button_set_has_frame (BOBGUI_BUTTON (tag->button), FALSE);
      bobgui_box_append (BOBGUI_BOX (tag->box), tag->button);
      g_signal_connect (tag->button, "clicked", G_CALLBACK (on_button_clicked), tag);
    }

  g_object_notify (G_OBJECT (tag), "has-close-button");
}

gboolean
demo_tagged_entry_tag_get_has_close_button (DemoTaggedEntryTag *tag)
{
  g_return_val_if_fail (DEMO_IS_TAGGED_ENTRY_TAG (tag), FALSE);

  return tag->button != NULL;
}
