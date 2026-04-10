#include <bobgui/bobgui.h>
#include "bobgui/bobguiaccessibleprivate.h"

/* These tests verify that the BobguiAccessible machinery works, independent
 * of any concrete widget accessible implementations. Therefore, we use
 * a dummy object that implements BobguiAccessible
 */
typedef struct
{
  GObject parent_instance;

  BobguiAccessibleRole role;
  BobguiATContext *at_context;
} TestObject;

typedef struct
{
  GObjectClass parent_class;
} TestObjectClass;

enum {
  PROP_ACCESSIBLE_ROLE = 1,
  NUM_PROPERTIES
};

static BobguiATContext *
test_object_accessible_get_at_context (BobguiAccessible *accessible)
{
  TestObject *self = (TestObject*)accessible;

  if (self->at_context == NULL)
    self->at_context = bobgui_at_context_create (self->role,
                                              accessible,
                                              gdk_display_get_default ());

  return g_object_ref (self->at_context);
}

static void
test_object_accessible_init (BobguiAccessibleInterface *iface)
{
  iface->get_at_context = test_object_accessible_get_at_context;
}

G_DEFINE_TYPE_WITH_CODE (TestObject, test_object, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_ACCESSIBLE,
                                                test_object_accessible_init))

static void
test_object_init (TestObject *self)
{
  self->role = BOBGUI_ACCESSIBLE_ROLE_WIDGET;
}

static void
test_object_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  TestObject *self = (TestObject *)object;

  switch (prop_id)
    {
    case PROP_ACCESSIBLE_ROLE:
      self->role = g_value_get_enum (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
test_object_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  TestObject *self = (TestObject *)object;

  switch (prop_id)
    {
    case PROP_ACCESSIBLE_ROLE:
      g_value_set_enum (value, self->role);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
test_object_class_init (TestObjectClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->set_property = test_object_set_property;
  gobject_class->get_property = test_object_get_property;

  g_object_class_override_property (gobject_class, PROP_ACCESSIBLE_ROLE, "accessible-role");
}

static TestObject *
test_object_new (BobguiAccessibleRole role)
{
  return g_object_new (test_object_get_type (),
                       "accessible-role", role,
                       NULL);
}

/* Tests for states */

static void
test_boolean_state (gconstpointer data)
{
  BobguiAccessibleState state = GPOINTER_TO_UINT (data);
  TestObject *object;

  object = test_object_new (BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_state (object, state, FALSE);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (object),
                               state, TRUE,
                               -1);
  bobgui_test_accessible_assert_state (object, state, TRUE);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (object),
                               state, FALSE,
                               -1);
  bobgui_test_accessible_assert_state (object, state, FALSE);

  g_object_unref (object);
}

static void
test_maybe_boolean_state (gconstpointer data)
{
  BobguiAccessibleState state = GPOINTER_TO_UINT (data);
  TestObject *object;

  object = test_object_new (BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_state (object, state, BOBGUI_ACCESSIBLE_VALUE_UNDEFINED);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (object),
                               state, TRUE,
                               -1);
  bobgui_test_accessible_assert_state (object, state, TRUE);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (object),
                               state, FALSE,
                               -1);
  bobgui_test_accessible_assert_state (object, state, FALSE);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (object),
                               state, BOBGUI_ACCESSIBLE_VALUE_UNDEFINED,
                               -1);
  bobgui_test_accessible_assert_state (object, state, BOBGUI_ACCESSIBLE_VALUE_UNDEFINED);

  g_object_unref (object);
}

static void
test_tristate_state (gconstpointer data)
{
  BobguiAccessibleState state = GPOINTER_TO_UINT (data);
  TestObject *object;

  object = test_object_new (BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_role (object, BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_state (object, state, BOBGUI_ACCESSIBLE_VALUE_UNDEFINED);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (object),
                               state, BOBGUI_ACCESSIBLE_TRISTATE_FALSE,
                               -1);
  bobgui_test_accessible_assert_state (object, state, BOBGUI_ACCESSIBLE_TRISTATE_FALSE);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (object),
                               state, BOBGUI_ACCESSIBLE_TRISTATE_TRUE,
                               -1);
  bobgui_test_accessible_assert_state (object, state, BOBGUI_ACCESSIBLE_TRISTATE_TRUE);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (object),
                               state, BOBGUI_ACCESSIBLE_TRISTATE_MIXED,
                               -1);
  bobgui_test_accessible_assert_state (object, state, BOBGUI_ACCESSIBLE_TRISTATE_MIXED);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (object),
                               state, BOBGUI_ACCESSIBLE_VALUE_UNDEFINED,
                               -1);
  bobgui_test_accessible_assert_state (object, state, BOBGUI_ACCESSIBLE_VALUE_UNDEFINED);

  g_object_unref (object);
}

static void
test_invalid_state (gconstpointer data)
{
  BobguiAccessibleState state = GPOINTER_TO_UINT (data);
  TestObject *object;

  object = test_object_new (BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_role (object, BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_state (object, state, BOBGUI_ACCESSIBLE_INVALID_FALSE);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (object),
                               state, BOBGUI_ACCESSIBLE_INVALID_TRUE,
                               -1);
  bobgui_test_accessible_assert_state (object, state, BOBGUI_ACCESSIBLE_INVALID_TRUE);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (object),
                               state, BOBGUI_ACCESSIBLE_INVALID_GRAMMAR,
                               -1);
  bobgui_test_accessible_assert_state (object, state, BOBGUI_ACCESSIBLE_INVALID_GRAMMAR);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (object),
                               state, BOBGUI_ACCESSIBLE_INVALID_SPELLING,
                               -1);
  bobgui_test_accessible_assert_state (object, state, BOBGUI_ACCESSIBLE_INVALID_SPELLING);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (object),
                               state, BOBGUI_ACCESSIBLE_INVALID_FALSE,
                               -1);
  bobgui_test_accessible_assert_state (object, state, BOBGUI_ACCESSIBLE_INVALID_FALSE);

  g_object_unref (object);
}

static void
test_update_multiple_states (void)
{
  TestObject *object;

  object = test_object_new (BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (object),
                               BOBGUI_ACCESSIBLE_STATE_BUSY, TRUE,
                               BOBGUI_ACCESSIBLE_STATE_CHECKED, BOBGUI_ACCESSIBLE_TRISTATE_MIXED,
                               -1);

  bobgui_test_accessible_assert_state (object, BOBGUI_ACCESSIBLE_STATE_BUSY, TRUE);
  bobgui_test_accessible_assert_state (object, BOBGUI_ACCESSIBLE_STATE_CHECKED, BOBGUI_ACCESSIBLE_TRISTATE_MIXED);

  bobgui_accessible_update_state (BOBGUI_ACCESSIBLE (object),
                               BOBGUI_ACCESSIBLE_STATE_BUSY, FALSE,
                               BOBGUI_ACCESSIBLE_STATE_CHECKED, BOBGUI_ACCESSIBLE_TRISTATE_TRUE,
                               BOBGUI_ACCESSIBLE_STATE_BUSY, TRUE,
                               BOBGUI_ACCESSIBLE_STATE_BUSY, FALSE,
                               -1);

  bobgui_test_accessible_assert_state (object, BOBGUI_ACCESSIBLE_STATE_BUSY, FALSE);
  bobgui_test_accessible_assert_state (object, BOBGUI_ACCESSIBLE_STATE_CHECKED, BOBGUI_ACCESSIBLE_TRISTATE_TRUE);

  g_object_unref (object);
}

static void
test_autocomplete_property (gconstpointer data)
{
  BobguiAccessibleProperty property = GPOINTER_TO_UINT (data);
  TestObject *object;

  object = test_object_new (BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_role (object, BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_property (object, property, BOBGUI_ACCESSIBLE_AUTOCOMPLETE_NONE);
  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, BOBGUI_ACCESSIBLE_AUTOCOMPLETE_INLINE,
                                  -1);
  bobgui_test_accessible_assert_property (object, property, BOBGUI_ACCESSIBLE_AUTOCOMPLETE_INLINE);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, BOBGUI_ACCESSIBLE_AUTOCOMPLETE_LIST,
                                  -1);
  bobgui_test_accessible_assert_property (object, property, BOBGUI_ACCESSIBLE_AUTOCOMPLETE_LIST);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, BOBGUI_ACCESSIBLE_AUTOCOMPLETE_BOTH,
                                  -1);
  bobgui_test_accessible_assert_property (object, property, BOBGUI_ACCESSIBLE_AUTOCOMPLETE_BOTH);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, BOBGUI_ACCESSIBLE_AUTOCOMPLETE_NONE,
                                  -1);
  bobgui_test_accessible_assert_property (object, property, BOBGUI_ACCESSIBLE_AUTOCOMPLETE_NONE);

  g_object_unref (object);
}

static void
test_string_property (gconstpointer data)
{
  BobguiAccessibleProperty property = GPOINTER_TO_UINT (data);
  TestObject *object;

  object = test_object_new (BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_role (object, BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_property (object, property, NULL);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, "some string that holds no particular value",
                                  -1);
  bobgui_test_accessible_assert_property (object, property, "some string that holds no particular value");

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, "see\nif\nnewlines\nwork ?!",
                                  -1);
  bobgui_test_accessible_assert_property (object, property, "see\nif\nnewlines\nwork ?!");

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, "",
                                  -1);
  bobgui_test_accessible_assert_property (object, property, "");

  g_object_unref (object);
}

static void
test_boolean_property (gconstpointer data)
{
  BobguiAccessibleProperty property = GPOINTER_TO_UINT (data);
  TestObject *object;

  object = test_object_new (BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_role (object, BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_property (object, property, FALSE);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, TRUE,
                                  -1);
  bobgui_test_accessible_assert_property (object, property, TRUE);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, FALSE,
                                  -1);
  bobgui_test_accessible_assert_property (object, property, FALSE);

  g_object_unref (object);
}

static void
test_int_property (gconstpointer data)
{
  BobguiAccessibleProperty property = GPOINTER_TO_UINT (data);
  TestObject *object;

  object = test_object_new (BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_role (object, BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_property (object, property, 0);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, 1,
                                  -1);
  bobgui_test_accessible_assert_property (object, property, 1);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, -1,
                                  -1);
  bobgui_test_accessible_assert_property (object, property, -1);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, 100,
                                  -1);
  bobgui_test_accessible_assert_property (object, property, 100);

  g_object_unref (object);
}

static void
test_number_property (gconstpointer data)
{
  BobguiAccessibleProperty property = GPOINTER_TO_UINT (data);
  TestObject *object;

  object = test_object_new (BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_role (object, BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_property (object, property, 0.);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, 1.5,
                                  -1);
  bobgui_test_accessible_assert_property (object, property, 1.5);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, -1.,
                                  -1);
  bobgui_test_accessible_assert_property (object, property, -1.);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, 1e6,
                                  -1);
  bobgui_test_accessible_assert_property (object, property, 1e6);

  g_object_unref (object);
}

static void
test_orientation_property (gconstpointer data)
{
  BobguiAccessibleProperty property = GPOINTER_TO_UINT (data);
  TestObject *object;

  object = test_object_new (BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_role (object, BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_property (object, property, BOBGUI_ACCESSIBLE_VALUE_UNDEFINED);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, BOBGUI_ORIENTATION_HORIZONTAL,
                                  -1);
  bobgui_test_accessible_assert_property (object, property, BOBGUI_ORIENTATION_HORIZONTAL);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, BOBGUI_ORIENTATION_VERTICAL,
                                  -1);
  bobgui_test_accessible_assert_property (object, property, BOBGUI_ORIENTATION_VERTICAL);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, BOBGUI_ACCESSIBLE_VALUE_UNDEFINED,
                                  -1);
  bobgui_test_accessible_assert_property (object, property, BOBGUI_ACCESSIBLE_VALUE_UNDEFINED);

  g_object_unref (object);
}

static void
test_sort_property (gconstpointer data)
{
  BobguiAccessibleProperty property = GPOINTER_TO_UINT (data);
  TestObject *object;

  object = test_object_new (BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_role (object, BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_property (object, property, BOBGUI_ACCESSIBLE_SORT_NONE);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, BOBGUI_ACCESSIBLE_SORT_ASCENDING,
                                  -1);
  bobgui_test_accessible_assert_property (object, property, BOBGUI_ACCESSIBLE_SORT_ASCENDING);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, BOBGUI_ACCESSIBLE_SORT_DESCENDING,
                                  -1);
  bobgui_test_accessible_assert_property (object, property, BOBGUI_ACCESSIBLE_SORT_DESCENDING);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, BOBGUI_ACCESSIBLE_SORT_OTHER,
                                  -1);
  bobgui_test_accessible_assert_property (object, property, BOBGUI_ACCESSIBLE_SORT_OTHER);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  property, BOBGUI_ACCESSIBLE_SORT_NONE,
                                  -1);
  bobgui_test_accessible_assert_property (object, property, BOBGUI_ACCESSIBLE_SORT_NONE);

  g_object_unref (object);
}

static void
test_update_multiple_properties (void)
{
  TestObject *object;

  object = test_object_new (BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, 100.,
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, 10.,
                                  -1);

  bobgui_test_accessible_assert_property (object, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, 100.);
  bobgui_test_accessible_assert_property (object, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, 10.);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (object),
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, 99.,
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, 11.,
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, 98.,
                                  BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, 97.,
                                  -1);

  bobgui_test_accessible_assert_property (object, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, 97.);
  bobgui_test_accessible_assert_property (object, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, 11.);

  g_object_unref (object);
}

static void
test_int_relation (gconstpointer data)
{
  BobguiAccessibleRelation relation = GPOINTER_TO_UINT (data);
  TestObject *object;

  object = test_object_new (BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_role (object, BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_relation (object, relation, 0);

  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (object),
                                  relation, 1,
                                  -1);
  bobgui_test_accessible_assert_relation (object, relation, 1);

  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (object),
                                  relation, -1,
                                  -1);
  bobgui_test_accessible_assert_relation (object, relation, -1);

  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (object),
                                  relation, 100,
                                  -1);
  bobgui_test_accessible_assert_relation (object, relation, 100);

  g_object_unref (object);
}

static void
test_string_relation (gconstpointer data)
{
  BobguiAccessibleRelation relation = GPOINTER_TO_UINT (data);
  TestObject *object;

  object = test_object_new (BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_role (object, BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_relation (object, relation, NULL);

  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (object),
                                  relation, "some string that holds no particular value",
                                  -1);
  bobgui_test_accessible_assert_relation (object, relation, "some string that holds no particular value");

  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (object),
                                  relation, "see\nif\nnewlines\nwork ?!",
                                  -1);
  bobgui_test_accessible_assert_relation (object, relation, "see\nif\nnewlines\nwork ?!");

  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (object),
                                  relation, "",
                                  -1);
  bobgui_test_accessible_assert_relation (object, relation, "");

  g_object_unref (object);
}

static void
test_ref_relation (gconstpointer data)
{
  BobguiAccessibleRelation relation = GPOINTER_TO_UINT (data);
  TestObject *object;
  TestObject *other;

  object = test_object_new (BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);
  other = test_object_new (BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_role (object, BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_relation (object, relation, NULL);

  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (object),
                                  relation, other,
                                  -1);
  bobgui_test_accessible_assert_relation (object, relation, other);

  g_object_unref (object);
  g_object_unref (other);
}

static void
test_reflist_relation (gconstpointer data)
{
  BobguiAccessibleRelation relation = GPOINTER_TO_UINT (data);
  TestObject *object;
  TestObject *other;
  TestObject *third;

  object = test_object_new (BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);
  other = test_object_new (BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);
  third = test_object_new (BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_role (object, BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  bobgui_test_accessible_assert_relation (object, relation, NULL);

  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (object),
                                  relation, other, NULL,
                                  -1);
  bobgui_test_accessible_assert_relation (object, relation, other, NULL);

  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (object),
                                  relation, other, third, NULL,
                                  -1);
  bobgui_test_accessible_assert_relation (object, relation, other, third, NULL);

  g_object_unref (object);
  g_object_unref (other);
  g_object_unref (third);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_data_func ("/a11y/state/busy", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_STATE_BUSY), test_boolean_state);
  g_test_add_data_func ("/a11y/state/checked", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_STATE_CHECKED), test_tristate_state);
  g_test_add_data_func ("/a11y/state/disabled", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_STATE_DISABLED), test_boolean_state);
  g_test_add_data_func ("/a11y/state/expanded", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_STATE_EXPANDED), test_maybe_boolean_state);
  g_test_add_data_func ("/a11y/state/hidden", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_STATE_HIDDEN), test_boolean_state);
  g_test_add_data_func ("/a11y/state/invalid", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_STATE_INVALID), test_invalid_state);
  g_test_add_data_func ("/a11y/state/pressed", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_STATE_PRESSED), test_tristate_state);
  g_test_add_data_func ("/a11y/state/selected", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_STATE_SELECTED), test_maybe_boolean_state);

  g_test_add_func ("/a11y/state/update-multiple", test_update_multiple_states);

  g_test_add_data_func ("/a11y/property/autocomplete", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_PROPERTY_AUTOCOMPLETE), test_autocomplete_property);
  g_test_add_data_func ("/a11y/property/description", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION), test_string_property);
  g_test_add_data_func ("/a11y/property/has-popup", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP), test_boolean_property);
  g_test_add_data_func ("/a11y/property/key-shortcuts", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_PROPERTY_KEY_SHORTCUTS), test_string_property);
  g_test_add_data_func ("/a11y/property/label", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_PROPERTY_LABEL), test_string_property);
  g_test_add_data_func ("/a11y/property/level", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_PROPERTY_LEVEL), test_int_property);
  g_test_add_data_func ("/a11y/property/modal", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_PROPERTY_MODAL), test_boolean_property);
  g_test_add_data_func ("/a11y/property/multi-line", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_PROPERTY_MULTI_LINE), test_boolean_property);
  g_test_add_data_func ("/a11y/property/multi-selectable", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_PROPERTY_MULTI_SELECTABLE), test_boolean_property);
  g_test_add_data_func ("/a11y/property/orientation", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION), test_orientation_property);
  g_test_add_data_func ("/a11y/property/placeholder", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_PROPERTY_PLACEHOLDER), test_string_property);
  g_test_add_data_func ("/a11y/property/read-only", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_PROPERTY_READ_ONLY), test_boolean_property);
  g_test_add_data_func ("/a11y/property/required", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_PROPERTY_REQUIRED), test_boolean_property);
  g_test_add_data_func ("/a11y/property/role-description", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_PROPERTY_ROLE_DESCRIPTION), test_string_property);
  g_test_add_data_func ("/a11y/property/sort", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_PROPERTY_SORT), test_sort_property);
  g_test_add_data_func ("/a11y/property/value-max", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX), test_number_property);
  g_test_add_data_func ("/a11y/property/value-min", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN), test_number_property);
  g_test_add_data_func ("/a11y/property/value-now", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW), test_number_property);
  g_test_add_data_func ("/a11y/property/value-text", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT), test_string_property);
  g_test_add_data_func ("/a11y/property/help-text", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_PROPERTY_HELP_TEXT), test_string_property);

  g_test_add_func ("/a11y/property/update-multiple", test_update_multiple_properties);

  g_test_add_data_func ("/a11y/relation/active-descendant", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_RELATION_ACTIVE_DESCENDANT), test_ref_relation);
  g_test_add_data_func ("/a11y/relation/col-count", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_RELATION_COL_COUNT), test_int_relation);
  g_test_add_data_func ("/a11y/relation/col-index", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_RELATION_COL_INDEX), test_int_relation);
  g_test_add_data_func ("/a11y/relation/col-index-text", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_RELATION_COL_INDEX_TEXT), test_string_relation);
  g_test_add_data_func ("/a11y/relation/col-span", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_RELATION_COL_SPAN), test_int_relation);
  g_test_add_data_func ("/a11y/relation/controls", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_RELATION_CONTROLS), test_reflist_relation);
  g_test_add_data_func ("/a11y/relation/described-by", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_RELATION_DESCRIBED_BY), test_reflist_relation);
  g_test_add_data_func ("/a11y/relation/details", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_RELATION_DETAILS), test_reflist_relation);
  g_test_add_data_func ("/a11y/relation/error-message", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE), test_reflist_relation);
  g_test_add_data_func ("/a11y/relation/flow-to", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_RELATION_FLOW_TO), test_reflist_relation);
  g_test_add_data_func ("/a11y/relation/labelled-by", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY), test_reflist_relation);
  g_test_add_data_func ("/a11y/relation/owns", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_RELATION_OWNS), test_reflist_relation);
  g_test_add_data_func ("/a11y/relation/pos-in-set", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_RELATION_POS_IN_SET), test_int_relation);
  g_test_add_data_func ("/a11y/relation/row-count", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_RELATION_ROW_COUNT), test_int_relation);
  g_test_add_data_func ("/a11y/relation/row-index", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_RELATION_ROW_INDEX), test_int_relation);
  g_test_add_data_func ("/a11y/relation/row-index-text", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_RELATION_ROW_INDEX_TEXT), test_string_relation);
  g_test_add_data_func ("/a11y/relation/row-span", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_RELATION_ROW_SPAN), test_int_relation);
  g_test_add_data_func ("/a11y/relation/set-size", GUINT_TO_POINTER (BOBGUI_ACCESSIBLE_RELATION_SET_SIZE), test_int_relation);

  return g_test_run ();
}
