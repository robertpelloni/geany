/* bobguiatcontext.c: Assistive technology context
 *
 * Copyright 2020  GNOME Foundation
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/**
 * BobguiATContext:
 *
 * Communicates with platform-specific assistive technologies API.
 *
 * Each platform supported by BOBGUI implements a `BobguiATContext` subclass, and
 * is responsible for updating the accessible state in response to state
 * changes in `BobguiAccessible`.
 */

#include "config.h"

#include "bobguiatcontextprivate.h"

#include "bobguiaccessiblevalueprivate.h"
#include "bobguiaccessibleprivate.h"
#include "bobguidebug.h"
#include "bobguiprivate.h"
#include "bobguitestatcontextprivate.h"
#include "bobguitypebuiltins.h"

#include "bobguibutton.h"
#include "bobguitogglebutton.h"
#include "bobguimenubutton.h"
#include "bobguidropdown.h"
#include "bobguicolordialogbutton.h"
#include "bobguifontdialogbutton.h"
#include "bobguiscalebutton.h"
#include "print/bobguiprinteroptionwidgetprivate.h"

#ifdef HAVE_ACCESSKIT
#include "a11y/bobguiaccesskitcontextprivate.h"
#endif
#if defined(GDK_WINDOWING_X11) || defined(GDK_WINDOWING_WAYLAND)
#include "a11y/bobguiatspicontextprivate.h"
#endif

G_DEFINE_ABSTRACT_TYPE (BobguiATContext, bobgui_at_context, G_TYPE_OBJECT)

enum
{
  PROP_ACCESSIBLE_ROLE = 1,
  PROP_ACCESSIBLE,
  PROP_DISPLAY,
  PROP_REALIZED,

  N_PROPS
};

enum
{
  STATE_CHANGE,

  LAST_SIGNAL
};

static GParamSpec *obj_props[N_PROPS];

static guint obj_signals[LAST_SIGNAL];

static char *bobgui_at_context_get_description_internal (BobguiATContext*self, gboolean check_duplicates);
static char *bobgui_at_context_get_name_internal (BobguiATContext*self, gboolean check_duplicates);

static void
bobgui_at_context_finalize (GObject *gobject)
{
  BobguiATContext *self = BOBGUI_AT_CONTEXT (gobject);

  bobgui_accessible_attribute_set_unref (self->properties);
  bobgui_accessible_attribute_set_unref (self->relations);
  bobgui_accessible_attribute_set_unref (self->states);

  G_OBJECT_CLASS (bobgui_at_context_parent_class)->finalize (gobject);
}

static void
bobgui_at_context_dispose (GObject *gobject)
{
  BobguiATContext *self = BOBGUI_AT_CONTEXT (gobject);

  bobgui_at_context_unrealize (self);

  if (self->accessible_parent != NULL)
    {
      g_object_remove_weak_pointer (G_OBJECT (self->accessible_parent),
                                    (gpointer *) &self->accessible_parent);
      self->accessible_parent = NULL;
    }

  if (self->next_accessible_sibling != NULL)
    {
      g_object_remove_weak_pointer (G_OBJECT (self->next_accessible_sibling),
                                    (gpointer *) &self->next_accessible_sibling);
      self->next_accessible_sibling = NULL;
    }

  G_OBJECT_CLASS (bobgui_at_context_parent_class)->dispose (gobject);
}

static void
bobgui_at_context_set_property (GObject      *gobject,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  BobguiATContext *self = BOBGUI_AT_CONTEXT (gobject);

  switch (prop_id)
    {
    case PROP_ACCESSIBLE_ROLE:
      bobgui_at_context_set_accessible_role (self, g_value_get_enum (value));
      break;

    case PROP_ACCESSIBLE:
      self->accessible = g_value_get_object (value);
      break;

    case PROP_DISPLAY:
      bobgui_at_context_set_display (self, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_at_context_get_property (GObject    *gobject,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  BobguiATContext *self = BOBGUI_AT_CONTEXT (gobject);

  switch (prop_id)
    {
    case PROP_ACCESSIBLE_ROLE:
      g_value_set_enum (value, self->accessible_role);
      break;

    case PROP_ACCESSIBLE:
      g_value_set_object (value, self->accessible);
      break;

    case PROP_DISPLAY:
      g_value_set_object (value, self->display);
      break;

    case PROP_REALIZED:
      g_value_set_boolean (value, self->realized);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_at_context_real_state_change (BobguiATContext                *self,
                                  BobguiAccessibleStateChange     changed_states,
                                  BobguiAccessiblePropertyChange  changed_properties,
                                  BobguiAccessibleRelationChange  changed_relations,
                                  BobguiAccessibleAttributeSet   *states,
                                  BobguiAccessibleAttributeSet   *properties,
                                  BobguiAccessibleAttributeSet   *relations)
{
}

static void
bobgui_at_context_real_platform_change (BobguiATContext                *self,
                                     BobguiAccessiblePlatformChange  change)
{
}

static void
bobgui_at_context_real_bounds_change (BobguiATContext *self)
{
}

static void
bobgui_at_context_real_child_change (BobguiATContext             *self,
                                  BobguiAccessibleChildChange  change,
                                  BobguiAccessible            *child)
{
}

static void
bobgui_at_context_real_realize (BobguiATContext *self)
{
}

static void
bobgui_at_context_real_unrealize (BobguiATContext *self)
{
}

static void
bobgui_at_context_real_update_caret_position (BobguiATContext *self)
{
}

static void
bobgui_at_context_real_update_selection_bound (BobguiATContext *self)
{
}

static void
bobgui_at_context_real_update_text_contents (BobguiATContext *self,
                                          BobguiAccessibleTextContentChange change,
                                          unsigned int start,
                                          unsigned int end)
{
}

static void
bobgui_at_context_class_init (BobguiATContextClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = bobgui_at_context_set_property;
  gobject_class->get_property = bobgui_at_context_get_property;
  gobject_class->dispose = bobgui_at_context_dispose;
  gobject_class->finalize = bobgui_at_context_finalize;

  klass->realize = bobgui_at_context_real_realize;
  klass->unrealize = bobgui_at_context_real_unrealize;
  klass->state_change = bobgui_at_context_real_state_change;
  klass->platform_change = bobgui_at_context_real_platform_change;
  klass->bounds_change = bobgui_at_context_real_bounds_change;
  klass->child_change = bobgui_at_context_real_child_change;
  klass->update_caret_position = bobgui_at_context_real_update_caret_position;
  klass->update_selection_bound = bobgui_at_context_real_update_selection_bound;
  klass->update_text_contents = bobgui_at_context_real_update_text_contents;

  /**
   * BobguiATContext:accessible-role:
   *
   * The accessible role used by the AT context.
   *
   * Depending on the given role, different states and properties can be
   * set or retrieved.
   */
  obj_props[PROP_ACCESSIBLE_ROLE] =
    g_param_spec_enum ("accessible-role", NULL, NULL,
                       BOBGUI_TYPE_ACCESSIBLE_ROLE,
                       BOBGUI_ACCESSIBLE_ROLE_NONE,
                       G_PARAM_READWRITE |
                       G_PARAM_CONSTRUCT |
                       G_PARAM_STATIC_STRINGS);

  /**
   * BobguiATContext:accessible:
   *
   * The `BobguiAccessible` that created the `BobguiATContext` instance.
   */
  obj_props[PROP_ACCESSIBLE] =
    g_param_spec_object ("accessible", NULL, NULL,
                         BOBGUI_TYPE_ACCESSIBLE,
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_STATIC_STRINGS);

  /**
   * BobguiATContext:display:
   *
   * The `GdkDisplay` for the `BobguiATContext`.
   */
  obj_props[PROP_DISPLAY] =
    g_param_spec_object ("display", NULL, NULL,
                         GDK_TYPE_DISPLAY,
                         G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS |
                         G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiATContext:realized:
   *
   * Whether the `BobguiATContext` has been realized or not.
   *
   * Since: 4.24
   */
  obj_props[PROP_REALIZED] =
    g_param_spec_boolean ("realized", NULL, NULL,
                          FALSE,
                          G_PARAM_READABLE |
                          G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiATContext::state-change:
   * @self: the `BobguiATContext`
   *
   * Emitted when the attributes of the accessible for the
   * `BobguiATContext` instance change.
   */
  obj_signals[STATE_CHANGE] =
    g_signal_new ("state-change",
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  g_object_class_install_properties (gobject_class, N_PROPS, obj_props);
}

#define N_PROPERTIES    (BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT + 1)
#define N_RELATIONS     (BOBGUI_ACCESSIBLE_RELATION_FLOW_FROM + 1)
#define N_STATES        (BOBGUI_ACCESSIBLE_STATE_SELECTED + 1)

static const char *property_attrs[] = {
  [BOBGUI_ACCESSIBLE_PROPERTY_AUTOCOMPLETE]        = "autocomplete",
  [BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION]         = "description",
  [BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP]           = "haspopup",
  [BOBGUI_ACCESSIBLE_PROPERTY_KEY_SHORTCUTS]       = "keyshortcuts",
  [BOBGUI_ACCESSIBLE_PROPERTY_LABEL]               = "label",
  [BOBGUI_ACCESSIBLE_PROPERTY_LEVEL]               = "level",
  [BOBGUI_ACCESSIBLE_PROPERTY_MODAL]               = "modal",
  [BOBGUI_ACCESSIBLE_PROPERTY_MULTI_LINE]          = "multiline",
  [BOBGUI_ACCESSIBLE_PROPERTY_MULTI_SELECTABLE]    = "multiselectable",
  [BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION]         = "orientation",
  [BOBGUI_ACCESSIBLE_PROPERTY_PLACEHOLDER]         = "placeholder",
  [BOBGUI_ACCESSIBLE_PROPERTY_READ_ONLY]           = "readonly",
  [BOBGUI_ACCESSIBLE_PROPERTY_REQUIRED]            = "required",
  [BOBGUI_ACCESSIBLE_PROPERTY_ROLE_DESCRIPTION]    = "roledescription",
  [BOBGUI_ACCESSIBLE_PROPERTY_SORT]                = "sort",
  [BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX]           = "valuemax",
  [BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN]           = "valuemin",
  [BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW]           = "valuenow",
  [BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT]          = "valuetext",
  [BOBGUI_ACCESSIBLE_PROPERTY_HELP_TEXT]           = "helptext",
};

/*< private >
 * bobgui_accessible_property_get_attribute_name:
 * @property: a `BobguiAccessibleProperty`
 *
 * Retrieves the name of a `BobguiAccessibleProperty`.
 *
 * Returns: (transfer none): the name of the accessible property
 */
const char *
bobgui_accessible_property_get_attribute_name (BobguiAccessibleProperty property)
{
  g_return_val_if_fail (property >= BOBGUI_ACCESSIBLE_PROPERTY_AUTOCOMPLETE &&
                        property <= BOBGUI_ACCESSIBLE_PROPERTY_HELP_TEXT,
                        "<none>");

  return property_attrs[property];
}

static const char *relation_attrs[] = {
  [BOBGUI_ACCESSIBLE_RELATION_ACTIVE_DESCENDANT]   = "activedescendant",
  [BOBGUI_ACCESSIBLE_RELATION_COL_COUNT]           = "colcount",
  [BOBGUI_ACCESSIBLE_RELATION_COL_INDEX]           = "colindex",
  [BOBGUI_ACCESSIBLE_RELATION_COL_INDEX_TEXT]      = "colindextext",
  [BOBGUI_ACCESSIBLE_RELATION_COL_SPAN]            = "colspan",
  [BOBGUI_ACCESSIBLE_RELATION_CONTROLS]            = "controls",
  [BOBGUI_ACCESSIBLE_RELATION_DESCRIBED_BY]        = "describedby",
  [BOBGUI_ACCESSIBLE_RELATION_DETAILS]             = "details",
  [BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE]       = "errormessage",
  [BOBGUI_ACCESSIBLE_RELATION_FLOW_TO]             = "flowto",
  [BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY]         = "labelledby",
  [BOBGUI_ACCESSIBLE_RELATION_OWNS]                = "owns",
  [BOBGUI_ACCESSIBLE_RELATION_POS_IN_SET]          = "posinset",
  [BOBGUI_ACCESSIBLE_RELATION_ROW_COUNT]           = "rowcount",
  [BOBGUI_ACCESSIBLE_RELATION_ROW_INDEX]           = "rowindex",
  [BOBGUI_ACCESSIBLE_RELATION_ROW_INDEX_TEXT]      = "rowindextext",
  [BOBGUI_ACCESSIBLE_RELATION_ROW_SPAN]            = "rowspan",
  [BOBGUI_ACCESSIBLE_RELATION_SET_SIZE]            = "setsize",
  [BOBGUI_ACCESSIBLE_RELATION_LABEL_FOR]           = "labelfor",
  [BOBGUI_ACCESSIBLE_RELATION_DESCRIPTION_FOR]     = "descriptionfor",
  [BOBGUI_ACCESSIBLE_RELATION_CONTROLLED_BY]       = "controlledby",
  [BOBGUI_ACCESSIBLE_RELATION_DETAILS_FOR]         = "detailsfor",
  [BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE_FOR]   = "errormessagefor",
  [BOBGUI_ACCESSIBLE_RELATION_FLOW_FROM]           = "flowfrom",
};

/*< private >
 * bobgui_accessible_relation_get_attribute_name:
 * @relation: a `BobguiAccessibleRelation`
 *
 * Retrieves the name of a `BobguiAccessibleRelation`.
 *
 * Returns: (transfer none): the name of the accessible relation
 */
const char *
bobgui_accessible_relation_get_attribute_name (BobguiAccessibleRelation relation)
{
  g_return_val_if_fail (relation >= BOBGUI_ACCESSIBLE_RELATION_ACTIVE_DESCENDANT &&
                        relation <= BOBGUI_ACCESSIBLE_RELATION_FLOW_FROM,
                        "<none>");

  return relation_attrs[relation];
}

static const char *state_attrs[] = {
  [BOBGUI_ACCESSIBLE_STATE_BUSY]           = "busy",
  [BOBGUI_ACCESSIBLE_STATE_CHECKED]        = "checked",
  [BOBGUI_ACCESSIBLE_STATE_DISABLED]       = "disabled",
  [BOBGUI_ACCESSIBLE_STATE_EXPANDED]       = "expanded",
  [BOBGUI_ACCESSIBLE_STATE_HIDDEN]         = "hidden",
  [BOBGUI_ACCESSIBLE_STATE_INVALID]        = "invalid",
  [BOBGUI_ACCESSIBLE_STATE_PRESSED]        = "pressed",
  [BOBGUI_ACCESSIBLE_STATE_SELECTED]       = "selected",
  [BOBGUI_ACCESSIBLE_STATE_VISITED]        = "visited",
};

/*< private >
 * bobgui_accessible_state_get_attribute_name:
 * @state: a `BobguiAccessibleState`
 *
 * Retrieves the name of a `BobguiAccessibleState`.
 *
 * Returns: (transfer none): the name of the accessible state
 */
const char *
bobgui_accessible_state_get_attribute_name (BobguiAccessibleState state)
{
  g_return_val_if_fail (state >= BOBGUI_ACCESSIBLE_STATE_BUSY &&
                        state <= BOBGUI_ACCESSIBLE_STATE_SELECTED,
                        "<none>");

  return state_attrs[state];
}

static void
bobgui_at_context_init (BobguiATContext *self)
{
  self->accessible_role = BOBGUI_ACCESSIBLE_ROLE_NONE;

  self->properties =
    bobgui_accessible_attribute_set_new (G_N_ELEMENTS (property_attrs),
                                      (BobguiAccessibleAttributeNameFunc) bobgui_accessible_property_get_attribute_name,
                                      (BobguiAccessibleAttributeDefaultFunc) bobgui_accessible_value_get_default_for_property);
  self->relations =
    bobgui_accessible_attribute_set_new (G_N_ELEMENTS (relation_attrs),
                                      (BobguiAccessibleAttributeNameFunc) bobgui_accessible_relation_get_attribute_name,
                                      (BobguiAccessibleAttributeDefaultFunc) bobgui_accessible_value_get_default_for_relation);
  self->states =
    bobgui_accessible_attribute_set_new (G_N_ELEMENTS (state_attrs),
                                      (BobguiAccessibleAttributeNameFunc) bobgui_accessible_state_get_attribute_name,
                                      (BobguiAccessibleAttributeDefaultFunc) bobgui_accessible_value_get_default_for_state);
}

/**
 * bobgui_at_context_get_accessible:
 * @self: a `BobguiATContext`
 *
 * Retrieves the `BobguiAccessible` using this context.
 *
 * Returns: (transfer none): a `BobguiAccessible`
 */
BobguiAccessible *
bobgui_at_context_get_accessible (BobguiATContext *self)
{
  g_return_val_if_fail (BOBGUI_IS_AT_CONTEXT (self), NULL);

  return self->accessible;
}

/*< private >
 * bobgui_at_context_set_accessible_role:
 * @self: a `BobguiATContext`
 * @role: the accessible role for the context
 *
 * Sets the accessible role for the given `BobguiATContext`.
 *
 * This function can only be called if the `BobguiATContext` is unrealized.
 */
void
bobgui_at_context_set_accessible_role (BobguiATContext      *self,
                                    BobguiAccessibleRole  role)
{
  g_return_if_fail (BOBGUI_IS_AT_CONTEXT (self));
  g_return_if_fail (!self->realized);

  if (self->accessible_role == role)
    return;

  self->accessible_role = role;

  g_object_notify_by_pspec (G_OBJECT (self), obj_props[PROP_ACCESSIBLE_ROLE]);
}

/**
 * bobgui_at_context_get_accessible_role:
 * @self: a `BobguiATContext`
 *
 * Retrieves the accessible role of this context.
 *
 * Returns: a `BobguiAccessibleRole`
 */
BobguiAccessibleRole
bobgui_at_context_get_accessible_role (BobguiATContext *self)
{
  g_return_val_if_fail (BOBGUI_IS_AT_CONTEXT (self), BOBGUI_ACCESSIBLE_ROLE_NONE);

  return self->accessible_role;
}

/*< private >
 * bobgui_at_context_get_accessible_parent:
 * @self: a `BobguiAtContext`
 *
 * Retrieves the parent accessible object of the given `BobguiAtContext`.
 *
 * Returns: (nullable) (transfer none): the parent accessible object, or `NULL` if not set.
 */
BobguiAccessible *
bobgui_at_context_get_accessible_parent (BobguiATContext *self)
{
  g_return_val_if_fail (BOBGUI_IS_AT_CONTEXT (self), NULL);

  return self->accessible_parent;
}


static BobguiATContext * get_parent_context (BobguiATContext *self);

static inline void
maybe_realize_context (BobguiATContext *self)
{
  if (BOBGUI_IS_WIDGET (self->accessible))
    {
      BobguiATContext *parent_context = get_parent_context (self);

      if (parent_context && parent_context->realized)
        bobgui_at_context_realize (self);

      g_clear_object (&parent_context);
    }
  else
    {
      BobguiAccessible *accessible_parent;

      bobgui_at_context_realize (self);

      accessible_parent = self->accessible_parent;
      while (accessible_parent && !BOBGUI_IS_WIDGET (accessible_parent))
        {
          BobguiATContext *parent_context = bobgui_accessible_get_at_context (accessible_parent);

          if (!parent_context)
            break;

          bobgui_at_context_realize (parent_context);
          accessible_parent = parent_context->accessible_parent;

          g_clear_object (&parent_context);
        }
    }
}

/*< private >
 * bobgui_at_context_set_accessible_parent:
 * @self: a `BobguiAtContext`
 * @parent: (nullable): the parent `BobguiAccessible` to set
 *
 * Sets the parent accessible object of the given `BobguiAtContext`.
 */
void
bobgui_at_context_set_accessible_parent (BobguiATContext *self,
                                      BobguiAccessible *parent)
{
  g_return_if_fail (BOBGUI_IS_AT_CONTEXT (self));

  if (self->accessible_parent != parent)
    {
      if (self->accessible_parent != NULL)
        g_object_remove_weak_pointer (G_OBJECT (self->accessible_parent),
                                      (gpointer *) &self->accessible_parent);

      self->accessible_parent = parent;
      if (self->accessible_parent != NULL)
        {
          g_object_add_weak_pointer (G_OBJECT (self->accessible_parent),
                                     (gpointer *) &self->accessible_parent);

          maybe_realize_context (self);
        }
    }
}

/*< private >
 * bobgui_at_context_get_next_accessible_sibling:
 * @self: a `BobguiAtContext`
 *
 * Retrieves the next accessible sibling of the given `BobguiAtContext`.
 *
 * Returns: (nullable) (transfer none): the next accessible sibling.
 */
BobguiAccessible *
bobgui_at_context_get_next_accessible_sibling (BobguiATContext *self)
{
  g_return_val_if_fail (BOBGUI_IS_AT_CONTEXT (self), NULL);

  return self->next_accessible_sibling;
}

/*< private >
 * bobgui_at_context_set_next_accessible_sibling:
 * @self: a `BobguiAtContext`
 * @sibling: (nullable): the next accessible sibling
 *
 * Sets the next accessible sibling object of the given `BobguiAtContext`.
 */
void
bobgui_at_context_set_next_accessible_sibling (BobguiATContext *self,
                                            BobguiAccessible *sibling)
{
  g_return_if_fail (BOBGUI_IS_AT_CONTEXT (self));

  if (self->next_accessible_sibling != sibling)
    {
      if (self->next_accessible_sibling != NULL)
        g_object_remove_weak_pointer (G_OBJECT (self->next_accessible_sibling),
                                      (gpointer *) &self->next_accessible_sibling);

      self->next_accessible_sibling = sibling;

      if (self->next_accessible_sibling != NULL)
        g_object_add_weak_pointer (G_OBJECT (self->next_accessible_sibling),
                                   (gpointer *) &self->next_accessible_sibling);
    }
}

/*< private >
 * bobgui_at_context_set_display:
 * @self: a `BobguiATContext`
 * @display: a `GdkDisplay`
 *
 * Sets the `GdkDisplay` used by the `BobguiATContext`.
 *
 * This function can only be called if the `BobguiATContext` is
 * not realized.
 */
void
bobgui_at_context_set_display (BobguiATContext *self,
                            GdkDisplay   *display)
{
  g_return_if_fail (BOBGUI_IS_AT_CONTEXT (self));
  g_return_if_fail (display == NULL || GDK_IS_DISPLAY (display));

  if (self->display == display)
    return;

  if (self->realized)
    return;

  self->display = display;

  g_object_notify_by_pspec (G_OBJECT (self), obj_props[PROP_DISPLAY]);
}

/*< private >
 * bobgui_at_context_get_display:
 * @self: a `BobguiATContext`
 *
 * Retrieves the `GdkDisplay` used to create the context.
 *
 * Returns: (transfer none): a `GdkDisplay`
 */
GdkDisplay *
bobgui_at_context_get_display (BobguiATContext *self)
{
  g_return_val_if_fail (BOBGUI_IS_AT_CONTEXT (self), NULL);

  return self->display;
}

static const struct {
  const char *name;
  const char *env_name;
  BobguiATContext * (* create_context) (BobguiAccessibleRole accessible_role,
                                     BobguiAccessible    *accessible,
                                     GdkDisplay       *display);
} a11y_backends[] = {
#if defined(GDK_WINDOWING_WAYLAND) || defined(GDK_WINDOWING_X11)
  { "AT-SPI", "atspi", bobgui_at_spi_create_context },
#endif
#ifdef HAVE_ACCESSKIT
  { "AccessKit", "accesskit", bobgui_accesskit_create_context },
#endif
  { "Test", "test", bobgui_test_at_context_new },
};

/**
 * bobgui_at_context_create: (constructor)
 * @accessible_role: the accessible role used by the `BobguiATContext`
 * @accessible: the `BobguiAccessible` implementation using the `BobguiATContext`
 * @display: the `GdkDisplay` used by the `BobguiATContext`
 *
 * Creates a new `BobguiATContext` instance for the given accessible role,
 * accessible instance, and display connection.
 *
 * The `BobguiATContext` implementation being instantiated will depend on the
 * platform.
 *
 * Returns: (nullable) (transfer full): the `BobguiATContext`
 */
BobguiATContext *
bobgui_at_context_create (BobguiAccessibleRole  accessible_role,
                       BobguiAccessible     *accessible,
                       GdkDisplay        *display)
{
  static const char *bobgui_a11y_env;
  BobguiATContext *res = NULL;

  if (bobgui_a11y_env == NULL)
    {
      bobgui_a11y_env = g_getenv ("BOBGUI_A11Y");
      if (bobgui_a11y_env == NULL)
        bobgui_a11y_env = "0";

      if (g_ascii_strcasecmp (bobgui_a11y_env, "help") == 0)
        {
          g_print ("Supported arguments for BOBGUI_A11Y environment variable:\n");
#ifdef HAVE_ACCESSKIT
          g_print ("   accesskit - Use the AccessKit accessibility backend\n");
#else
          g_print ("   accesskit - Disabled during BOBGUI build\n");
#endif
#if defined(GDK_WINDOWING_X11) || defined(GDK_WINDOWING_WAYLAND)
          g_print ("       atspi - Use the AT-SPI accessibility backend\n");
#else
          g_print ("       atspi - Not available on this platform\n");
#endif
          g_print ("        test - Use the test accessibility backend\n");
          g_print ("        none - Disable the accessibility backend\n");
          g_print ("        help - Print this help\n\n");
          g_print ("Other arguments will cause a warning and be ignored.\n");

          bobgui_a11y_env = "0";
        }
    }

  /* Short-circuit disabling the accessibility support */
  if (g_ascii_strcasecmp (bobgui_a11y_env, "none") == 0)
    return NULL;

  for (size_t i = 0; i < G_N_ELEMENTS (a11y_backends); i++)
    {
      g_assert (a11y_backends[i].name != NULL);

      if (a11y_backends[i].create_context != NULL &&
          (*bobgui_a11y_env == '0' || g_ascii_strcasecmp (a11y_backends[i].env_name, bobgui_a11y_env) == 0))
        {
          res = a11y_backends[i].create_context (accessible_role, accessible, display);
          if (res != NULL)
            break;
        }
    }

  if (*bobgui_a11y_env != '0' && res == NULL)
    g_warning ("Unrecognized accessibility backend \"%s\". Try BOBGUI_A11Y=help", bobgui_a11y_env);

  /* Fall back to the test context, so we can get debugging data */
  if (res == NULL)
    res = g_object_new (BOBGUI_TYPE_TEST_AT_CONTEXT,
                        "accessible_role", accessible_role,
                        "accessible", accessible,
                        "display", display,
                        NULL);

  return res;
}

/*< private >
 * bobgui_at_context_clone: (constructor)
 * @self: the `BobguiATContext` to clone
 * @role: the accessible role of the clone, or %BOBGUI_ACCESSIBLE_ROLE_NONE to
 *   use the same accessible role of @self
 * @accessible: (nullable): the accessible creating the context, or %NULL to
 *   use the same `BobguiAccessible` of @self
 * @display: (nullable): the display connection, or %NULL to use the same
 *   `GdkDisplay` of @self
 *
 * Clones the state of the given `BobguiATContext`, using @role, @accessible,
 * and @display.
 *
 * If @self is realized, the returned `BobguiATContext` will also be realized.
 *
 * Returns: (transfer full): the newly created `BobguiATContext`
 */
BobguiATContext *
bobgui_at_context_clone (BobguiATContext      *self,
                      BobguiAccessibleRole  role,
                      BobguiAccessible     *accessible,
                      GdkDisplay        *display)
{
  g_return_val_if_fail (self == NULL || BOBGUI_IS_AT_CONTEXT (self), NULL);
  g_return_val_if_fail (accessible == NULL || BOBGUI_IS_ACCESSIBLE (accessible), NULL);
  g_return_val_if_fail (display == NULL || GDK_IS_DISPLAY (display), NULL);

  if (self != NULL && role == BOBGUI_ACCESSIBLE_ROLE_NONE)
    role = self->accessible_role;

  if (self != NULL && accessible == NULL)
    accessible = self->accessible;

  if (self != NULL && display == NULL)
    display = self->display;

  BobguiATContext *res = bobgui_at_context_create (role, accessible, display);

  if (self != NULL)
    {
      g_clear_pointer (&res->states, bobgui_accessible_attribute_set_unref);
      g_clear_pointer (&res->properties, bobgui_accessible_attribute_set_unref);
      g_clear_pointer (&res->relations, bobgui_accessible_attribute_set_unref);

      res->states = bobgui_accessible_attribute_set_ref (self->states);
      res->properties = bobgui_accessible_attribute_set_ref (self->properties);
      res->relations = bobgui_accessible_attribute_set_ref (self->relations);

      if (self->realized)
        bobgui_at_context_realize (res);
    }

  return res;
}

gboolean
bobgui_at_context_is_realized (BobguiATContext *self)
{
  return self->realized;
}

void
bobgui_at_context_realize (BobguiATContext *self)
{
  if (self->realized)
    return;

  BOBGUI_DEBUG (A11Y, "Realizing AT context '%s'", G_OBJECT_TYPE_NAME (self));
  BOBGUI_AT_CONTEXT_GET_CLASS (self)->realize (self);

  self->realized = TRUE;

  g_object_notify_by_pspec (G_OBJECT (self), obj_props[PROP_REALIZED]);
}

void
bobgui_at_context_unrealize (BobguiATContext *self)
{
  if (!self->realized)
    return;

  BOBGUI_DEBUG (A11Y, "Unrealizing AT context '%s'", G_OBJECT_TYPE_NAME (self));
  BOBGUI_AT_CONTEXT_GET_CLASS (self)->unrealize (self);

  self->realized = FALSE;
}

/*< private >
 * bobgui_at_context_update:
 * @self: a `BobguiATContext`
 *
 * Notifies the AT connected to this `BobguiATContext` that the accessible
 * state and its properties have changed.
 */
void
bobgui_at_context_update (BobguiATContext *self)
{
  g_return_if_fail (BOBGUI_IS_AT_CONTEXT (self));

  if (!self->realized)
    return;

  /* There's no point in notifying of state changes if there weren't any */
  if (self->updated_properties == 0 &&
      self->updated_relations == 0 &&
      self->updated_states == 0)
    return;

  BOBGUI_AT_CONTEXT_GET_CLASS (self)->state_change (self,
                                                 self->updated_states, self->updated_properties, self->updated_relations,
                                                 self->states, self->properties, self->relations);
  g_signal_emit (self, obj_signals[STATE_CHANGE], 0);

  self->updated_properties = 0;
  self->updated_relations = 0;
  self->updated_states = 0;
}

/*< private >
 * bobgui_at_context_set_accessible_state:
 * @self: a `BobguiATContext`
 * @state: a `BobguiAccessibleState`
 * @value: (nullable): `BobguiAccessibleValue`
 *
 * Sets the @value for the given @state of a `BobguiATContext`.
 *
 * If @value is %NULL, the state is unset.
 *
 * This function will accumulate state changes until bobgui_at_context_update()
 * is called.
 */
void
bobgui_at_context_set_accessible_state (BobguiATContext       *self,
                                     BobguiAccessibleState  state,
                                     BobguiAccessibleValue *value)
{
  g_return_if_fail (BOBGUI_IS_AT_CONTEXT (self));

  gboolean res = FALSE;

  if (value != NULL)
    res = bobgui_accessible_attribute_set_add (self->states, state, value);
  else
    res = bobgui_accessible_attribute_set_remove (self->states, state);

  if (res)
    self->updated_states |= (1 << state);
}

/*< private >
 * bobgui_at_context_has_accessible_state:
 * @self: a `BobguiATContext`
 * @state: a `BobguiAccessibleState`
 *
 * Checks whether a `BobguiATContext` has the given @state set.
 *
 * Returns: %TRUE, if the accessible state is set
 */
gboolean
bobgui_at_context_has_accessible_state (BobguiATContext       *self,
                                     BobguiAccessibleState  state)
{
  g_return_val_if_fail (BOBGUI_IS_AT_CONTEXT (self), FALSE);

  return bobgui_accessible_attribute_set_contains (self->states, state);
}

/*< private >
 * bobgui_at_context_get_accessible_state:
 * @self: a `BobguiATContext`
 * @state: a `BobguiAccessibleState`
 *
 * Retrieves the value for the accessible state of a `BobguiATContext`.
 *
 * Returns: (transfer none): the value for the given state
 */
BobguiAccessibleValue *
bobgui_at_context_get_accessible_state (BobguiATContext       *self,
                                     BobguiAccessibleState  state)
{
  g_return_val_if_fail (BOBGUI_IS_AT_CONTEXT (self), NULL);

  return bobgui_accessible_attribute_set_get_value (self->states, state);
}

/*< private >
 * bobgui_at_context_set_accessible_property:
 * @self: a `BobguiATContext`
 * @property: a `BobguiAccessibleProperty`
 * @value: (nullable): `BobguiAccessibleValue`
 *
 * Sets the @value for the given @property of a `BobguiATContext`.
 *
 * If @value is %NULL, the property is unset.
 *
 * This function will accumulate property changes until bobgui_at_context_update()
 * is called.
 */
void
bobgui_at_context_set_accessible_property (BobguiATContext          *self,
                                        BobguiAccessibleProperty  property,
                                        BobguiAccessibleValue    *value)
{
  g_return_if_fail (BOBGUI_IS_AT_CONTEXT (self));

  gboolean res = FALSE;

  if (value != NULL)
    res = bobgui_accessible_attribute_set_add (self->properties, property, value);
  else
    res = bobgui_accessible_attribute_set_remove (self->properties, property);

  if (res && self->realized)
    self->updated_properties |= (1 << property);
}

/*< private >
 * bobgui_at_context_has_accessible_property:
 * @self: a `BobguiATContext`
 * @property: a `BobguiAccessibleProperty`
 *
 * Checks whether a `BobguiATContext` has the given @property set.
 *
 * Returns: %TRUE, if the accessible property is set
 */
gboolean
bobgui_at_context_has_accessible_property (BobguiATContext          *self,
                                        BobguiAccessibleProperty  property)
{
  g_return_val_if_fail (BOBGUI_IS_AT_CONTEXT (self), FALSE);

  return bobgui_accessible_attribute_set_contains (self->properties, property);
}

/*< private >
 * bobgui_at_context_get_accessible_property:
 * @self: a `BobguiATContext`
 * @property: a `BobguiAccessibleProperty`
 *
 * Retrieves the value for the accessible property of a `BobguiATContext`.
 *
 * Returns: (transfer none): the value for the given property
 */
BobguiAccessibleValue *
bobgui_at_context_get_accessible_property (BobguiATContext          *self,
                                        BobguiAccessibleProperty  property)
{
  g_return_val_if_fail (BOBGUI_IS_AT_CONTEXT (self), NULL);

  return bobgui_accessible_attribute_set_get_value (self->properties, property);
}

static void
append_to_accessible_relation (BobguiATContext          *self,
                               BobguiAccessibleRelation  relation,
                               BobguiAccessible         *accessible)
{
  g_return_if_fail (BOBGUI_IS_AT_CONTEXT (self));
  BobguiAccessibleValue *target_value;

  if (bobgui_accessible_attribute_set_contains (self->relations, relation))
    {
      target_value = bobgui_accessible_value_ref (bobgui_accessible_attribute_set_get_value (self->relations, relation));
    }
  else
    {
      target_value = bobgui_reference_list_accessible_value_new (NULL);
      bobgui_accessible_attribute_set_add (self->relations, relation, target_value);
    }

  bobgui_reference_list_accessible_value_append (target_value, accessible);

  bobgui_accessible_value_unref (target_value);

  self->updated_relations |= (1 << relation);
}

static void
remove_from_accessible_relation (BobguiATContext          *self,
                                 BobguiAccessibleRelation  relation,
                                 BobguiAccessible         *accessible)
{
  g_return_if_fail (BOBGUI_IS_AT_CONTEXT (self));
  BobguiAccessibleValue * target_value;

  if (!bobgui_accessible_attribute_set_contains (self->relations, relation))
    return;

  target_value = bobgui_accessible_attribute_set_get_value (self->relations, relation);
  bobgui_reference_list_accessible_value_remove (target_value, accessible);

  self->updated_relations |= (1 << relation);
}

static void
update_reverse_relation (BobguiATContext *self, BobguiAccessibleRelation relation, BobguiAccessibleValue *value)
{
  struct {
    BobguiAccessibleRelation rel;
    BobguiAccessibleRelation reverse_rel;
  } reverse_rels_map[] = {
    { BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, BOBGUI_ACCESSIBLE_RELATION_LABEL_FOR },
    { BOBGUI_ACCESSIBLE_RELATION_DESCRIBED_BY, BOBGUI_ACCESSIBLE_RELATION_DESCRIPTION_FOR },
    { BOBGUI_ACCESSIBLE_RELATION_CONTROLS, BOBGUI_ACCESSIBLE_RELATION_CONTROLLED_BY },
    { BOBGUI_ACCESSIBLE_RELATION_DETAILS, BOBGUI_ACCESSIBLE_RELATION_DETAILS_FOR },
    { BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE, BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE_FOR },
    { BOBGUI_ACCESSIBLE_RELATION_FLOW_TO, BOBGUI_ACCESSIBLE_RELATION_FLOW_FROM },
  };

  GList *l;
  BobguiATContext *related_context;
  for (int i = 0; i < G_N_ELEMENTS (reverse_rels_map); i++)
    {
      if (relation == reverse_rels_map[i].rel)
        {
          if (value)
            {
              for (l = bobgui_reference_list_accessible_value_get (value); l; l = l->next)
                {
                  related_context = bobgui_accessible_get_at_context (l->data);
                  append_to_accessible_relation (related_context, reverse_rels_map[i].reverse_rel, self->accessible);
                  g_clear_object (&related_context);
                }
            }
          else
            {
              if (bobgui_accessible_attribute_set_contains (self->relations, relation))
                {
                  BobguiAccessibleValue *val = bobgui_accessible_attribute_set_get_value (self->relations, relation);
                  for (l = bobgui_reference_list_accessible_value_get (val); l; l = l->next)
                    {
                      related_context = bobgui_accessible_get_at_context (l->data);
                      if (!related_context)
                        continue;
                      remove_from_accessible_relation (related_context, reverse_rels_map[i].reverse_rel, self->accessible);
                      g_clear_object (&related_context);
                    }
                }
            }
          break;
        }
    }
}

/*< private >
 * bobgui_at_context_set_accessible_relation:
 * @self: a `BobguiATContext`
 * @relation: a `BobguiAccessibleRelation`
 * @value: (nullable): `BobguiAccessibleValue`
 *
 * Sets the @value for the given @relation of a `BobguiATContext`.
 *
 * If @value is %NULL, the relation is unset.
 *
 * This function will accumulate relation changes until bobgui_at_context_update()
 * is called.
 */
void
bobgui_at_context_set_accessible_relation (BobguiATContext          *self,
                                        BobguiAccessibleRelation  relation,
                                        BobguiAccessibleValue    *value)
{
  g_return_if_fail (BOBGUI_IS_AT_CONTEXT (self));

  gboolean res = FALSE;

  /* We are setting the relation to a new value,
  * so we must get rid of the reverse relations first.
  */
  update_reverse_relation (self, relation, NULL);
  /* Now, we can create the new reverse relations if it makes sense. */
  if (value != NULL)
    update_reverse_relation (self, relation, value);

  if (value != NULL)
    res = bobgui_accessible_attribute_set_add (self->relations, relation, value);
  else
    res = bobgui_accessible_attribute_set_remove (self->relations, relation);

  if (res)
    self->updated_relations |= (1 << relation);

}

/*< private >
 * bobgui_at_context_has_accessible_relation:
 * @self: a `BobguiATContext`
 * @relation: a `BobguiAccessibleRelation`
 *
 * Checks whether a `BobguiATContext` has the given @relation set.
 *
 * Returns: %TRUE, if the accessible relation is set
 */
gboolean
bobgui_at_context_has_accessible_relation (BobguiATContext          *self,
                                        BobguiAccessibleRelation  relation)
{
  g_return_val_if_fail (BOBGUI_IS_AT_CONTEXT (self), FALSE);

  return bobgui_accessible_attribute_set_contains (self->relations, relation);
}

/*< private >
 * bobgui_at_context_get_accessible_relation:
 * @self: a `BobguiATContext`
 * @relation: a `BobguiAccessibleRelation`
 *
 * Retrieves the value for the accessible relation of a `BobguiATContext`.
 *
 * Returns: (transfer none): the value for the given relation
 */
BobguiAccessibleValue *
bobgui_at_context_get_accessible_relation (BobguiATContext          *self,
                                        BobguiAccessibleRelation  relation)
{
  g_return_val_if_fail (BOBGUI_IS_AT_CONTEXT (self), NULL);

  return bobgui_accessible_attribute_set_get_value (self->relations, relation);
}

/* See ARIA 5.2.8.4, 5.2.8.5 and 5.2.8.6 for the prohibited, from author
 * and from content parts, and the table in
 * https://www.w3.org/WAI/ARIA/apg/practices/names-and-descriptions/
 * for the recommended / not recommended parts. We've made a few changes
 * to the recommendations:
 * - We don't recommend against labelling listitems, sincd BobguiListView
 *   will put the focus on listitems sometimes.
 * - We don't recommend tab lists being labelled, since BobguiNotebook does
 *   not have a practical way of doing that.
 */

#define NAME_FROM_AUTHOR  (1 << 6)
#define NAME_FROM_CONTENT (1 << 7)

static guint8 naming[] = {
  [BOBGUI_ACCESSIBLE_ROLE_ALERT] = NAME_FROM_AUTHOR,
  [BOBGUI_ACCESSIBLE_ROLE_ALERT_DIALOG] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_APPLICATION] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_ARTICLE] = NAME_FROM_AUTHOR,
  [BOBGUI_ACCESSIBLE_ROLE_BANNER] = NAME_FROM_AUTHOR,
  [BOBGUI_ACCESSIBLE_ROLE_BLOCK_QUOTE] = NAME_FROM_AUTHOR,
  [BOBGUI_ACCESSIBLE_ROLE_BUTTON] = NAME_FROM_AUTHOR|NAME_FROM_CONTENT|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_CAPTION] = BOBGUI_ACCESSIBLE_NAME_PROHIBITED,
  [BOBGUI_ACCESSIBLE_ROLE_CELL] = NAME_FROM_AUTHOR|NAME_FROM_CONTENT,
  [BOBGUI_ACCESSIBLE_ROLE_CHECKBOX] = NAME_FROM_AUTHOR|NAME_FROM_CONTENT|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_COLUMN_HEADER] = NAME_FROM_AUTHOR|NAME_FROM_CONTENT|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_COMBO_BOX] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_COMMAND] = BOBGUI_ACCESSIBLE_NAME_PROHIBITED,
  [BOBGUI_ACCESSIBLE_ROLE_COMMENT] = NAME_FROM_AUTHOR|NAME_FROM_CONTENT,
  [BOBGUI_ACCESSIBLE_ROLE_COMPOSITE] = BOBGUI_ACCESSIBLE_NAME_PROHIBITED,
  [BOBGUI_ACCESSIBLE_ROLE_DIALOG] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_DOCUMENT] = NAME_FROM_AUTHOR,
  [BOBGUI_ACCESSIBLE_ROLE_FEED] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_RECOMMENDED,
  [BOBGUI_ACCESSIBLE_ROLE_FORM] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_GENERIC] = BOBGUI_ACCESSIBLE_NAME_PROHIBITED,
  [BOBGUI_ACCESSIBLE_ROLE_GRID] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_GRID_CELL] = NAME_FROM_AUTHOR|NAME_FROM_CONTENT,
  [BOBGUI_ACCESSIBLE_ROLE_GROUP] = NAME_FROM_AUTHOR,
  [BOBGUI_ACCESSIBLE_ROLE_HEADING] = NAME_FROM_AUTHOR|NAME_FROM_CONTENT|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_IMG] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_INPUT] = BOBGUI_ACCESSIBLE_NAME_PROHIBITED,
  [BOBGUI_ACCESSIBLE_ROLE_LABEL] = NAME_FROM_AUTHOR|NAME_FROM_CONTENT,
  [BOBGUI_ACCESSIBLE_ROLE_LANDMARK] = BOBGUI_ACCESSIBLE_NAME_PROHIBITED,
  [BOBGUI_ACCESSIBLE_ROLE_LEGEND] = BOBGUI_ACCESSIBLE_NAME_PROHIBITED,
  [BOBGUI_ACCESSIBLE_ROLE_LINK] = NAME_FROM_AUTHOR|NAME_FROM_CONTENT|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_LIST] = NAME_FROM_AUTHOR,
  [BOBGUI_ACCESSIBLE_ROLE_LIST_BOX] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_LIST_ITEM] = NAME_FROM_AUTHOR,
  [BOBGUI_ACCESSIBLE_ROLE_LOG] = NAME_FROM_AUTHOR,
  [BOBGUI_ACCESSIBLE_ROLE_MAIN] = NAME_FROM_AUTHOR,
  [BOBGUI_ACCESSIBLE_ROLE_MARQUEE] = NAME_FROM_AUTHOR,
  [BOBGUI_ACCESSIBLE_ROLE_MATH] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_RECOMMENDED,
  [BOBGUI_ACCESSIBLE_ROLE_METER] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_MENU] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_RECOMMENDED,
  [BOBGUI_ACCESSIBLE_ROLE_MENU_BAR] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_RECOMMENDED,
  [BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM] = NAME_FROM_AUTHOR|NAME_FROM_CONTENT|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_CHECKBOX] = NAME_FROM_AUTHOR|NAME_FROM_CONTENT|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_RADIO] = NAME_FROM_AUTHOR|NAME_FROM_CONTENT|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_NAVIGATION] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_RECOMMENDED,
  [BOBGUI_ACCESSIBLE_ROLE_NONE] = BOBGUI_ACCESSIBLE_NAME_PROHIBITED,
  [BOBGUI_ACCESSIBLE_ROLE_NOTE] = NAME_FROM_AUTHOR,
  [BOBGUI_ACCESSIBLE_ROLE_OPTION] = NAME_FROM_AUTHOR|NAME_FROM_CONTENT|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_PARAGRAPH] = BOBGUI_ACCESSIBLE_NAME_PROHIBITED,
  [BOBGUI_ACCESSIBLE_ROLE_PRESENTATION] = BOBGUI_ACCESSIBLE_NAME_PROHIBITED,
  [BOBGUI_ACCESSIBLE_ROLE_PROGRESS_BAR] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_RADIO] = NAME_FROM_AUTHOR|NAME_FROM_CONTENT|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_RADIO_GROUP] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_RANGE] = BOBGUI_ACCESSIBLE_NAME_PROHIBITED,
  [BOBGUI_ACCESSIBLE_ROLE_REGION] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_ROW] = NAME_FROM_AUTHOR|NAME_FROM_CONTENT,
  [BOBGUI_ACCESSIBLE_ROLE_ROW_GROUP] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_NOT_RECOMMENDED,
  [BOBGUI_ACCESSIBLE_ROLE_ROW_HEADER] = NAME_FROM_AUTHOR|NAME_FROM_CONTENT|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_SCROLLBAR] = NAME_FROM_AUTHOR,
  [BOBGUI_ACCESSIBLE_ROLE_SEARCH] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_RECOMMENDED,
  [BOBGUI_ACCESSIBLE_ROLE_SEARCH_BOX] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_SECTION] = BOBGUI_ACCESSIBLE_NAME_PROHIBITED,
  [BOBGUI_ACCESSIBLE_ROLE_SECTION_HEAD] = BOBGUI_ACCESSIBLE_NAME_PROHIBITED,
  [BOBGUI_ACCESSIBLE_ROLE_SELECT] = BOBGUI_ACCESSIBLE_NAME_PROHIBITED,
  [BOBGUI_ACCESSIBLE_ROLE_SEPARATOR] = NAME_FROM_AUTHOR,
  [BOBGUI_ACCESSIBLE_ROLE_SLIDER] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_SPIN_BUTTON] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_STATUS] = NAME_FROM_AUTHOR,
  [BOBGUI_ACCESSIBLE_ROLE_STRUCTURE] = BOBGUI_ACCESSIBLE_NAME_PROHIBITED,
  [BOBGUI_ACCESSIBLE_ROLE_SWITCH] = NAME_FROM_AUTHOR|NAME_FROM_CONTENT|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_TAB] = NAME_FROM_AUTHOR|NAME_FROM_CONTENT|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_TABLE] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_TAB_LIST] = NAME_FROM_AUTHOR,
  [BOBGUI_ACCESSIBLE_ROLE_TAB_PANEL] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_TERMINAL] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_TEXT_BOX] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_TIME] = NAME_FROM_AUTHOR,
  [BOBGUI_ACCESSIBLE_ROLE_TIMER] = NAME_FROM_AUTHOR,
  [BOBGUI_ACCESSIBLE_ROLE_TOGGLE_BUTTON] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_TOOLBAR] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_RECOMMENDED,
  [BOBGUI_ACCESSIBLE_ROLE_TOOLTIP] = NAME_FROM_AUTHOR|NAME_FROM_CONTENT| BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_TREE] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_TREE_GRID] = NAME_FROM_AUTHOR|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_TREE_ITEM] = NAME_FROM_AUTHOR|NAME_FROM_CONTENT|BOBGUI_ACCESSIBLE_NAME_REQUIRED,
  [BOBGUI_ACCESSIBLE_ROLE_WIDGET] = NAME_FROM_AUTHOR|NAME_FROM_CONTENT,
  [BOBGUI_ACCESSIBLE_ROLE_WINDOW] = NAME_FROM_AUTHOR,
};

/* < private >
 * bobgui_accessible_role_supports_name_from_author:
 * @role: a `BobguiAccessibleRole`
 *
 * Returns whether this role supports setting the label and description
 * properties or the labelled-by and described-by relations.
 *
 * Returns: %TRUE if the role allows labelling
 */
gboolean
bobgui_accessible_role_supports_name_from_author (BobguiAccessibleRole role)
{
  return (naming[role] & NAME_FROM_AUTHOR) != 0;
}

/* < private >
 * bobgui_accessible_role_supports_name_from_content:
 * @role: a `BobguiAccessibleRole`
 *
 * Returns whether this role will use content of child widgets such
 * as labels for its accessible name and description if no explicit
 * labels are provided.
 *
 * Returns: %TRUE if the role content naming
 */
gboolean
bobgui_accessible_role_supports_name_from_content (BobguiAccessibleRole role)
{
  return (naming[role] & NAME_FROM_CONTENT) != 0;
}

/* < private >
 * bobgui_accessible_role_get_nameing:
 * @role: a `BobguiAccessibleRole`
 *
 * Returns naming information for this role.
 *
 * Returns: information about naming requirements for the role
 */
BobguiAccessibleNaming
bobgui_accessible_role_get_naming (BobguiAccessibleRole role)
{
  return (BobguiAccessibleNaming) (naming[role] & ~(NAME_FROM_AUTHOR|NAME_FROM_CONTENT));
}

gboolean
bobgui_at_context_is_nested_button (BobguiATContext *self)
{
  BobguiAccessible *accessible;
  BobguiWidget *widget, *parent;

  accessible = bobgui_at_context_get_accessible (self);

  if (!BOBGUI_IS_WIDGET (accessible))
    return FALSE;

  widget = BOBGUI_WIDGET (accessible);
  parent = bobgui_widget_get_parent (widget);

  if ((BOBGUI_IS_TOGGLE_BUTTON (widget) && BOBGUI_IS_DROP_DOWN (parent)) ||
      (BOBGUI_IS_TOGGLE_BUTTON (widget) && BOBGUI_IS_MENU_BUTTON (parent)) ||
      (BOBGUI_IS_BUTTON (widget) && BOBGUI_IS_COLOR_DIALOG_BUTTON (parent)) ||
      (BOBGUI_IS_BUTTON (widget) && BOBGUI_IS_FONT_DIALOG_BUTTON (parent)) ||
      (BOBGUI_IS_BUTTON (widget) && BOBGUI_IS_SCALE_BUTTON (parent))
#ifdef G_OS_UNIX
      || (BOBGUI_IS_PRINTER_OPTION_WIDGET (parent) &&
          (BOBGUI_IS_CHECK_BUTTON (widget) ||
           BOBGUI_IS_DROP_DOWN (widget) ||
           BOBGUI_IS_ENTRY (widget) ||
           BOBGUI_IS_IMAGE (widget) ||
           BOBGUI_IS_LABEL (widget) ||
           BOBGUI_IS_BUTTON (widget)))
#endif
      )
    return TRUE;

  return FALSE;
}

static BobguiATContext *
get_parent_context (BobguiATContext *self)
{
  BobguiAccessible *accessible, *parent;

  accessible = bobgui_at_context_get_accessible (self);
  parent = bobgui_accessible_get_accessible_parent (accessible);
  if (parent)
    {
      BobguiATContext *context = bobgui_accessible_get_at_context (parent);
      g_object_unref (parent);
      return context;
    }

  return g_object_ref (self);
}

static inline gboolean
not_just_space (const char *text)
{
  for (const char *p = text; *p; p = g_utf8_next_char (p))
    {
      if (!g_unichar_isspace (g_utf8_get_char (p)))
        return TRUE;
    }

  return FALSE;
}

static inline void
append_with_space (GString    *str,
                   const char *text)
{
  if (str->len > 0)
    g_string_append (str, " ");
  g_string_append (str, text);
}

/* See the WAI-ARIA § 4.3, "Accessible Name and Description Computation",
 * and https://www.w3.org/TR/accname-1.2/
 */

static void
bobgui_at_context_get_text_accumulate (BobguiATContext          *self,
                                    GPtrArray             *nodes,
                                    GString               *res,
                                    BobguiAccessibleProperty  property,
                                    BobguiAccessibleRelation  relation,
                                    gboolean               is_ref,
                                    gboolean               is_child,
                                    gboolean               check_duplicates)
{
  BobguiAccessibleValue *value = NULL;

  /* Step 2.A */
  if (!is_ref)
    {
      if (bobgui_accessible_attribute_set_contains (self->states, BOBGUI_ACCESSIBLE_STATE_HIDDEN))
        {
          value = bobgui_accessible_attribute_set_get_value (self->states, BOBGUI_ACCESSIBLE_STATE_HIDDEN);

          if (bobgui_boolean_accessible_value_get (value))
            return;
        }
    }

  if (bobgui_accessible_role_supports_name_from_author (self->accessible_role))
    {
      /* Step 2.B */
      if (!is_ref && bobgui_accessible_attribute_set_contains (self->relations, relation))
        {
          value = bobgui_accessible_attribute_set_get_value (self->relations, relation);

          GList *list = bobgui_reference_list_accessible_value_get (value);

          for (GList *l = list; l != NULL; l = l->next)
            {
              BobguiAccessible *rel = BOBGUI_ACCESSIBLE (l->data);
              if (!g_ptr_array_find (nodes, rel, NULL))
                {
                  BobguiATContext *rel_context = bobgui_accessible_get_at_context (rel);

                  g_ptr_array_add (nodes, rel);
                  bobgui_at_context_get_text_accumulate (rel_context, nodes, res, property, relation, TRUE, FALSE, check_duplicates);

                  g_object_unref (rel_context);
                }
            }

          return;
        }

      /* Step 2.C */
      if (bobgui_accessible_attribute_set_contains (self->properties, property))
        {
          value = bobgui_accessible_attribute_set_get_value (self->properties, property);

          char *str = (char *) bobgui_string_accessible_value_get (value);
          if (str[0] != '\0')
            {
              append_with_space (res, bobgui_string_accessible_value_get (value));
              return;
            }
        }
    }

  /* Step 2.E */
  if ((property == BOBGUI_ACCESSIBLE_PROPERTY_LABEL && is_child) || (relation == BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY && is_ref))
    {
      if (self->accessible_role == BOBGUI_ACCESSIBLE_ROLE_TEXT_BOX)
        {
          if (BOBGUI_IS_EDITABLE (self->accessible))
            {
              const char *text = bobgui_editable_get_text (BOBGUI_EDITABLE (self->accessible));
            if (text && not_just_space (text))
              append_with_space (res, text);
          }
        return;
      }
      else if (bobgui_accessible_role_is_range_subclass (self->accessible_role))
        {
          if (bobgui_accessible_attribute_set_contains (self->properties, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT))
            {
              value = bobgui_accessible_attribute_set_get_value (self->properties, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT);
              append_with_space (res, bobgui_string_accessible_value_get (value));
            }
          else if (bobgui_accessible_attribute_set_contains (self->properties, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW))
            {
              value = bobgui_accessible_attribute_set_get_value (self->properties, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW);
              if (res->len > 0)
                g_string_append (res, " ");
              g_string_append_printf (res, "%g", bobgui_number_accessible_value_get (value));
            }

          return;
        }
      }

  /* Step 2.F */
  if (bobgui_accessible_role_supports_name_from_content (self->accessible_role) || is_ref || is_child)
    {
      if (BOBGUI_IS_WIDGET (self->accessible))
        {
          GString *s = g_string_new ("");

          for (BobguiWidget *child = bobgui_widget_get_first_child (BOBGUI_WIDGET (self->accessible));
               child != NULL;
               child = bobgui_widget_get_next_sibling (child))
            {
              BobguiAccessible *rel = BOBGUI_ACCESSIBLE (child);
              BobguiATContext *rel_context = bobgui_accessible_get_at_context (rel);

              bobgui_at_context_get_text_accumulate (rel_context, nodes, s, property, relation, FALSE, TRUE, check_duplicates);

              g_object_unref (rel_context);
            }

           if (s->len > 0)
             {
               append_with_space (res, s->str);
               g_string_free (s, TRUE);
               return;
             }

           g_string_free (s, TRUE);
        }
    }

  /* Step 2.I */
  if (BOBGUI_IS_WIDGET (self->accessible))
    {
      const char *text = bobgui_widget_get_tooltip_text (BOBGUI_WIDGET (self->accessible));
      if (text && not_just_space (text))
        {
          gboolean append = !check_duplicates;

          if (!append)
            {
              char *description = bobgui_at_context_get_description_internal (self, FALSE);
              char *name = bobgui_at_context_get_name_internal (self, FALSE);

              append =
                (property == BOBGUI_ACCESSIBLE_PROPERTY_LABEL && strcmp (text, description) != 0) ||
                (property == BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION && strcmp (text, name) != 0);

              g_free (description);
              g_free (name);
            }

          if (append)
            append_with_space (res, text);
        }
    }
}

static char *
bobgui_at_context_get_text (BobguiATContext          *self,
                         BobguiAccessibleProperty  property,
                         BobguiAccessibleRelation  relation,
gboolean              check_duplicates)
{
  BobguiATContext *parent = NULL;

  g_return_val_if_fail (BOBGUI_IS_AT_CONTEXT (self), NULL);

  /* Step 1 */
  if (bobgui_accessible_role_get_naming (self->accessible_role) == BOBGUI_ACCESSIBLE_NAME_PROHIBITED)
    return g_strdup ("");

  /* We special case this here since it is a common pattern:
   * We have a 'wrapper' object, like a BobguiDropdown which
   * contains a toggle button. The dropdown appears in the
   * ui file and carries all the a11y attributes, but the
   * focus ends up on the toggle button.
   */
  if (bobgui_at_context_is_nested_button (self))
    {
      parent = get_parent_context (self);
      self = parent;
      if (bobgui_at_context_is_nested_button (self))
        {
          parent = get_parent_context (parent);
          g_object_unref (self);
          self = parent;
        }
    }

  GPtrArray *nodes = g_ptr_array_new ();
  GString *res = g_string_new ("");

  /* Step 2 */
  bobgui_at_context_get_text_accumulate (self, nodes, res, property, relation, FALSE, FALSE, check_duplicates);

  g_ptr_array_unref (nodes);

  g_clear_object (&parent);

  return g_string_free (res, FALSE);
}

static char *
bobgui_at_context_get_name_internal (BobguiATContext *self, gboolean check_duplicates)
{
  return bobgui_at_context_get_text (self, BOBGUI_ACCESSIBLE_PROPERTY_LABEL, BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, check_duplicates);
}

/*< private >
 * bobgui_at_context_get_name:
 * @self: a `BobguiATContext`
 *
 * Retrieves the accessible name of the `BobguiATContext`.
 *
 * This is a convenience function meant to be used by `BobguiATContext` implementations.
 *
 * Returns: (transfer full): the label of the `BobguiATContext`
 */
char *
bobgui_at_context_get_name (BobguiATContext *self)
{
  /*
  * We intentionally don't check for duplicates here, as the name
  * is more important, and we want the tooltip as the name
  * if everything else fails.
  */
  return bobgui_at_context_get_name_internal (self, FALSE);
}

static char *
bobgui_at_context_get_description_internal (BobguiATContext *self, gboolean check_duplicates)
{
  return bobgui_at_context_get_text (self, BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION, BOBGUI_ACCESSIBLE_RELATION_DESCRIBED_BY, check_duplicates);
}

/*< private >
 * bobgui_at_context_get_description:
 * @self: a `BobguiATContext`
 *
 * Retrieves the accessible description of the `BobguiATContext`.
 *
 * This is a convenience function meant to be used by `BobguiATContext` implementations.
 *
 * Returns: (transfer full): the label of the `BobguiATContext`
 */
char *
bobgui_at_context_get_description (BobguiATContext *self)
{
  return bobgui_at_context_get_description_internal (self, TRUE);
}

void
bobgui_at_context_platform_changed (BobguiATContext                *self,
                                 BobguiAccessiblePlatformChange  change)
{
  bobgui_at_context_realize (self);

  BOBGUI_AT_CONTEXT_GET_CLASS (self)->platform_change (self, change);
}

void
bobgui_at_context_bounds_changed (BobguiATContext *self)
{
  if (!self->realized)
    return;

  BOBGUI_AT_CONTEXT_GET_CLASS (self)->bounds_change (self);
}

void
bobgui_at_context_child_changed (BobguiATContext             *self,
                              BobguiAccessibleChildChange  change,
                              BobguiAccessible            *child)
{
  if (!self->realized)
    return;

  BOBGUI_AT_CONTEXT_GET_CLASS (self)->child_change (self, change, child);
}

void
bobgui_at_context_announce (BobguiATContext                      *self,
                         const char                        *message,
                         BobguiAccessibleAnnouncementPriority  priority)
{
  if (!self->realized)
    return;

  BOBGUI_AT_CONTEXT_GET_CLASS (self)->announce (self, message, priority);
}

void
bobgui_at_context_update_caret_position (BobguiATContext *self)
{
  if (!self->realized)
    return;

  BOBGUI_AT_CONTEXT_GET_CLASS (self)->update_caret_position (self);
}

void
bobgui_at_context_update_selection_bound (BobguiATContext *self)
{
  if (!self->realized)
    return;

  BOBGUI_AT_CONTEXT_GET_CLASS (self)->update_selection_bound (self);
}

void
bobgui_at_context_update_text_contents (BobguiATContext *self,
                                     BobguiAccessibleTextContentChange change,
                                     unsigned int start,
                                     unsigned int end)
{
  if (!self->realized)
    return;

  BOBGUI_AT_CONTEXT_GET_CLASS (self)->update_text_contents (self, change, start, end);
}
