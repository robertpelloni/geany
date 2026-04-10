/* bobguiaccessiblehypertext.c: Interface for accessible objects with links
 *
 * SPDX-FileCopyrightText: 2025 Red Hat, Inc.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h"

#include "bobguiaccessiblehypertextprivate.h"

#include "bobguiatcontextprivate.h"

G_DEFINE_INTERFACE (BobguiAccessibleHypertext, bobgui_accessible_hypertext, BOBGUI_TYPE_ACCESSIBLE)

static unsigned int
bobgui_accessible_hypertext_default_get_n_links (BobguiAccessibleHypertext *self)
{
  g_warning ("BobguiAccessibleHypertext::get_n_links not implemented for %s",
             G_OBJECT_TYPE_NAME (self));

  return 0;
}

static BobguiAccessibleHyperlink *
bobgui_accessible_hypertext_default_get_link (BobguiAccessibleHypertext *self,
                                           unsigned int            index)
{
  g_warning ("BobguiAccessibleHypertext::get_link not implemented for %s",
             G_OBJECT_TYPE_NAME (self));

  return NULL;
}

static unsigned int
bobgui_accessible_hypertext_default_get_link_at (BobguiAccessibleHypertext *self,
                                              unsigned int            offset)
{
  return G_MAXUINT;
}

static void
bobgui_accessible_hypertext_default_init (BobguiAccessibleHypertextInterface *iface)
{
  iface->get_n_links = bobgui_accessible_hypertext_default_get_n_links;
  iface->get_link = bobgui_accessible_hypertext_default_get_link;
  iface->get_link_at = bobgui_accessible_hypertext_default_get_link_at;
}

/*< private >
 * bobgui_accessible_hypertext_get_n_links:
 * @self: the accessible object
 *
 * Retrieve the number of links in the object.
 *
 * Returns: the number of links
 *
 * Since: 4.22
 */
unsigned int
bobgui_accessible_hypertext_get_n_links (BobguiAccessibleHypertext *self)
{
  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE_HYPERTEXT (self), 0);

  return BOBGUI_ACCESSIBLE_HYPERTEXT_GET_IFACE (self)->get_n_links (self);
}

/*< private >
 * bobgui_accessible_hypertext_get_link:
 * @self: the accessible object
 * @index: the index of the link to retrieve
 *
 * Retrieve the n-th link of the object.
 *
 * Returns: the link
 *
 * Since: 4.22
 */
BobguiAccessibleHyperlink *
bobgui_accessible_hypertext_get_link (BobguiAccessibleHypertext *self,
                                   unsigned int            index)
{
  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE_HYPERTEXT (self), NULL);

  return BOBGUI_ACCESSIBLE_HYPERTEXT_GET_IFACE (self)->get_link (self, index);
}

/*< private >
 * bobgui_accessible_hypertext_get_link_at:
 * @self: the accessible object
 * @offset: the character offset
 *
 * Retrieve the index of the ink at the given character offset.
 *
 * Returns: the index, or `G_MAXUINT`
 *
 * Since: 4.22
 */
unsigned int
bobgui_accessible_hypertext_get_link_at (BobguiAccessibleHypertext *self,
                                      unsigned int            offset)
{
  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE_HYPERTEXT (self), G_MAXUINT);

  return BOBGUI_ACCESSIBLE_HYPERTEXT_GET_IFACE (self)->get_link_at (self, offset);
}

/* {{{ BobguiAccessibleHyperlink */

enum {
  PROP_ACCESSIBLE_ROLE = 1,
};

/**
 * BobguiAccessibleHyperlink:
 *
 * Represents a link (i.e. a uri).
 *
 * A widget that contains one or more links should implement
 * the [iface@Bobgui.AccessibleHypertext] interface and return
 * `BobguiAccessibleHyperlink` objects for each of the links.
 *
 * Since: 4.22
 */
struct _BobguiAccessibleHyperlink
{
  GObject parent_instance;

  BobguiATContext *at_context;

  BobguiAccessibleHypertext *parent;
  unsigned int index;

  char *uri;
  gsize start;
  gsize length;

  unsigned int platform_state;
};

static BobguiATContext *
bobgui_accessible_hyperlink_get_at_context (BobguiAccessible *accessible)
{
  BobguiAccessibleHyperlink *self = BOBGUI_ACCESSIBLE_HYPERLINK (accessible);

  if (self->at_context == NULL)
    {
      BobguiAccessibleRole role = BOBGUI_ACCESSIBLE_ROLE_LINK;
      GdkDisplay *display;

      display = gdk_display_get_default ();

      self->at_context = bobgui_at_context_create (role, accessible, display);
      if (self->at_context == NULL)
        return NULL;
    }

  return g_object_ref (self->at_context);
}

static gboolean
bobgui_accessible_hyperlink_get_platform_state (BobguiAccessible              *accessible,
                                             BobguiAccessiblePlatformState  state)
{
  BobguiAccessibleHyperlink *self = BOBGUI_ACCESSIBLE_HYPERLINK (accessible);

  return (self->platform_state & (1 << state)) != 0;
}

static BobguiAccessible *
bobgui_accessible_hyperlink_get_accessible_parent (BobguiAccessible *accessible)
{
  BobguiAccessibleHyperlink *self = BOBGUI_ACCESSIBLE_HYPERLINK (accessible);

  if (self->parent == NULL)
    return NULL;

  return g_object_ref (BOBGUI_ACCESSIBLE (self->parent));
}

static BobguiAccessible *
bobgui_accessible_hyperlink_get_first_accessible_child (BobguiAccessible *accessible)
{
  return NULL;
}

static BobguiAccessible *
bobgui_accessible_hyperlink_get_next_accessible_sibling (BobguiAccessible *accessible)
{
  BobguiAccessibleHyperlink *self = BOBGUI_ACCESSIBLE_HYPERLINK (accessible);

  if (self->index + 1 < bobgui_accessible_hypertext_get_n_links (self->parent))
    return g_object_ref (BOBGUI_ACCESSIBLE (bobgui_accessible_hypertext_get_link (self->parent, self->index + 1)));

  return NULL;
}

static gboolean
bobgui_accessible_hyperlink_get_bounds (BobguiAccessible *accessible,
                                     int           *x,
                                     int           *y,
                                     int           *width,
                                     int           *height)
{
  return FALSE;
}

static void
bobgui_accessible_hyperlink_accessible_init (BobguiAccessibleInterface *iface)
{
  iface->get_at_context = bobgui_accessible_hyperlink_get_at_context;
  iface->get_platform_state = bobgui_accessible_hyperlink_get_platform_state;
  iface->get_accessible_parent = bobgui_accessible_hyperlink_get_accessible_parent;
  iface->get_first_accessible_child = bobgui_accessible_hyperlink_get_first_accessible_child;
  iface->get_next_accessible_sibling = bobgui_accessible_hyperlink_get_next_accessible_sibling;
  iface->get_bounds = bobgui_accessible_hyperlink_get_bounds;
}

G_DEFINE_TYPE_WITH_CODE (BobguiAccessibleHyperlink, bobgui_accessible_hyperlink, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACCESSIBLE,
                                                bobgui_accessible_hyperlink_accessible_init))

static void
bobgui_accessible_hyperlink_init (BobguiAccessibleHyperlink *self)
{
}

static void
bobgui_accessible_hyperlink_dispose (GObject *object)
{
  BobguiAccessibleHyperlink *self = BOBGUI_ACCESSIBLE_HYPERLINK (object);

  g_clear_object (&self->at_context);

  G_OBJECT_CLASS (bobgui_accessible_hyperlink_parent_class)->dispose (object);
}

static void
bobgui_accessible_hyperlink_finalize (GObject *object)
{
  BobguiAccessibleHyperlink *self = BOBGUI_ACCESSIBLE_HYPERLINK (object);

  g_free (self->uri);

  G_OBJECT_CLASS (bobgui_accessible_hyperlink_parent_class)->finalize (object);
}

static void
bobgui_accessible_hyperlink_get_property (GObject      *object,
                                       unsigned int  prop_id,
                                       GValue       *value,
                                       GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_ACCESSIBLE_ROLE:
      g_value_set_enum (value, BOBGUI_ACCESSIBLE_ROLE_LINK);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_accessible_hyperlink_set_property (GObject      *object,
                                       unsigned int  prop_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
}

static void
bobgui_accessible_hyperlink_class_init (BobguiAccessibleHyperlinkClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->dispose = bobgui_accessible_hyperlink_dispose;
  object_class->finalize = bobgui_accessible_hyperlink_finalize;
  object_class->get_property = bobgui_accessible_hyperlink_get_property;
  object_class->set_property = bobgui_accessible_hyperlink_set_property;

  g_object_class_override_property (object_class, PROP_ACCESSIBLE_ROLE, "accessible-role");
}

/**
 * bobgui_accessible_hyperlink_new:
 * @parent: the parent
 * @index: the index of this link in the parent
 * @uri: the uri
 * @bounds: the text range that the link occupies (or 0, 0)
 *
 * Creates an accessible object that represents a hyperlink.
 *
 * This is meant to be used with an implementation of the
 * [iface@Bobgui.AccessibleHypertext] interface.
 *
 * Since: 4.22
 */
BobguiAccessibleHyperlink *
bobgui_accessible_hyperlink_new (BobguiAccessibleHypertext *parent,
                              unsigned int            index,
                              const char             *uri,
                              BobguiAccessibleTextRange *bounds)
{
  BobguiAccessibleHyperlink *self;

  self = g_object_new (BOBGUI_ACCESSIBLE_HYPERLINK_TYPE, NULL);

  self->parent = parent;
  self->index = index;

  self->uri = g_strdup (uri);
  self->start = bounds->start;
  self->length = bounds->length;

  return self;
}

/**
 * bobgui_accessible_hyperlink_set_platform_state:
 * @self: the accessible
 * @state: the platform state to change
 * @enabled: the new value for the platform state
 *
 * Sets a platform state on the accessible.
 *
 * Since: 4.22
 */
void
bobgui_accessible_hyperlink_set_platform_state (BobguiAccessibleHyperlink     *self,
                                             BobguiAccessiblePlatformState  state,
                                             gboolean                    enabled)
{
  if (enabled)
    self->platform_state |= 1 << state;
  else
    self->platform_state ^= 1 << state;

  bobgui_accessible_update_platform_state (BOBGUI_ACCESSIBLE (self), state);
}

unsigned int
bobgui_accessible_hyperlink_get_index (BobguiAccessibleHyperlink *self)
{
  return self->index;
}

const char *
bobgui_accessible_hyperlink_get_uri (BobguiAccessibleHyperlink *self)
{
  return self->uri;
}

void
bobgui_accessible_hyperlink_get_extents (BobguiAccessibleHyperlink *self,
                                      BobguiAccessibleTextRange *bounds)
{
  bounds->start = self->start;
  bounds->length = self->length;
}

/* }}} */

/* vim:set foldmethod=marker: */
