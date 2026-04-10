/*
 * Copyright © 2019 Benjamin Otte
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "config.h"

#include "bobguifilterprivate.h"

#include "bobguitypebuiltins.h"
#include "bobguiprivate.h"

/**
 * BobguiFilter:
 *
 * Describes the filtering to be performed by a [class@Bobgui.FilterListModel].
 *
 * The model will use the filter to determine if it should include items
 * or not by calling [method@Bobgui.Filter.match] for each item and only
 * keeping the ones that the function returns true for.
 *
 * Filters may change what items they match through their lifetime. In that
 * case, they will emit the [signal@Bobgui.Filter::changed] signal to notify
 * that previous filter results are no longer valid and that items should
 * be checked again via [method@Bobgui.Filter.match].
 *
 * BOBGUI provides various pre-made filter implementations for common filtering
 * operations. These filters often include properties that can be linked to
 * various widgets to easily allow searches.
 *
 * However, in particular for large lists or complex search methods, it is
 * also possible to subclass `BobguiFilter` and provide one's own filter.
 */

enum {
  CHANGED,
  LAST_SIGNAL
};

G_DEFINE_TYPE_WITH_CODE (BobguiFilter, bobgui_filter, G_TYPE_OBJECT,
                         g_type_add_class_private (g_define_type_id, sizeof (BobguiFilterClassPrivate)))

static guint signals[LAST_SIGNAL] = { 0 };

typedef struct _ExpressionWatchData {
  BobguiExpression *expression;
  gpointer item;

  BobguiExpressionWatch *watch;
  BobguiFilterWatchCallback callback;

  gpointer user_data;
  GDestroyNotify destroy;
} ExpressionWatchData;

static void
expression_watch_cb (gpointer user_data)
{
  ExpressionWatchData *data = (ExpressionWatchData *) user_data;
  data->callback (data->item, data->user_data);
}

static gpointer
bobgui_filter_default_watch (BobguiFilter              *self,
                          gpointer                item,
                          BobguiFilterWatchCallback  callback,
                          gpointer                user_data,
                          GDestroyNotify          destroy)
{
  GParamSpec *pspec;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (self), "expression");
  if (pspec && g_type_is_a (pspec->value_type, BOBGUI_TYPE_EXPRESSION))
    {
      ExpressionWatchData *data;
      BobguiExpression *expression;

      g_object_get (self, "expression", &expression, NULL);

      data = g_new0 (ExpressionWatchData, 1);
      data->item = item;
      data->callback = callback;
      data->user_data = user_data;
      data->destroy = destroy;
      data->watch = bobgui_expression_watch (expression,
                                          item,
                                          expression_watch_cb,
                                          data,
                                          NULL);

      bobgui_expression_unref (expression);

      return g_steal_pointer (&data);
    }

  return NULL;
}

static void
bobgui_filter_default_unwatch (BobguiFilter *filter,
                            gpointer   watch)
{
  ExpressionWatchData *data = (ExpressionWatchData *) watch;
  g_clear_pointer (&data->watch, bobgui_expression_watch_unwatch);
  g_free (data);
}

static gboolean
bobgui_filter_default_match (BobguiFilter *self,
                          gpointer   item)
{
  g_critical ("Filter of type '%s' does not implement BobguiFilter::match", G_OBJECT_TYPE_NAME (self));

  return FALSE;
}

static BobguiFilterMatch
bobgui_filter_default_get_strictness (BobguiFilter *self)
{
  return BOBGUI_FILTER_MATCH_SOME;
}

static void
bobgui_filter_class_init (BobguiFilterClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BobguiFilterClassPrivate *filter_private_class = G_TYPE_CLASS_GET_PRIVATE (class, BOBGUI_TYPE_FILTER, BobguiFilterClassPrivate);

  class->match = bobgui_filter_default_match;
  class->get_strictness = bobgui_filter_default_get_strictness;

  filter_private_class->watch = bobgui_filter_default_watch;
  filter_private_class->unwatch = bobgui_filter_default_unwatch;

  /**
   * BobguiFilter::changed:
   * @self: the filter
   * @change: how the filter changed
   *
   * Emitted whenever the filter changed.
   *
   * Users of the filter should then check items again via
   * [method@Bobgui.Filter.match].
   *
   * `BobguiFilterListModel` handles this signal automatically.
   *
   * Depending on the @change parameter, not all items need
   * to be checked, but only some. Refer to the [enum@Bobgui.FilterChange]
   * documentation for details.
   */
  signals[CHANGED] =
    g_signal_new (I_("changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__ENUM,
                  G_TYPE_NONE, 1,
                  BOBGUI_TYPE_FILTER_CHANGE);
  g_signal_set_va_marshaller (signals[CHANGED],
                              G_TYPE_FROM_CLASS (gobject_class),
                              g_cclosure_marshal_VOID__ENUMv);
}

static void
bobgui_filter_init (BobguiFilter *self)
{
}

/**
 * bobgui_filter_match:
 * @self: a filter
 * @item: (type GObject) (transfer none): The item to check
 *
 * Checks if the given @item is matched by the filter or not.
 *
 * Returns: true if the filter matches the item
 */
gboolean
bobgui_filter_match (BobguiFilter *self,
                  gpointer   item)
{
  g_return_val_if_fail (BOBGUI_IS_FILTER (self), FALSE);
  g_return_val_if_fail (item != NULL, FALSE);

  return BOBGUI_FILTER_GET_CLASS (self)->match (self, item);
}

/**
 * bobgui_filter_get_strictness:
 * @self: a filter
 *
 * Gets the known strictness of a filter.
 *
 * If the strictness is not known, [enum@Bobgui.FilterMatch.some] is returned.
 *
 * This value may change after emission of the [signal@Bobgui.Filter::changed]
 * signal.
 *
 * This function is meant purely for optimization purposes. Filters can
 * choose to omit implementing it, but `BobguiFilterListModel` uses it.
 *
 * Returns: the strictness of @self
 */
BobguiFilterMatch
bobgui_filter_get_strictness (BobguiFilter *self)
{
  g_return_val_if_fail (BOBGUI_IS_FILTER (self), BOBGUI_FILTER_MATCH_SOME);

  return BOBGUI_FILTER_GET_CLASS (self)->get_strictness (self);
}

/**
 * bobgui_filter_changed:
 * @self: a filter
 * @change: how the filter changed
 *
 * Notifies all users of the filter that it has changed.
 *
 * This emits the [signal@Bobgui.Filter::changed] signal. Users
 * of the filter should then check items again via
 * [method@Bobgui.Filter.match].
 *
 * Depending on the @change parameter, not all items need to
 * be changed, but only some. Refer to the [enum@Bobgui.FilterChange]
 * documentation for details.
 *
 * This function is intended for implementers of `BobguiFilter`
 * subclasses and should not be called from other functions.
 */
void
bobgui_filter_changed (BobguiFilter       *self,
                    BobguiFilterChange  change)
{
  g_return_if_fail (BOBGUI_IS_FILTER (self));

  g_signal_emit (self, signals[CHANGED], 0, change);
}

/*<private>
 * bobgui_filter_watch:
 * @self: a filter
 * @item: (type GObject) (transfer none): The item to watch
 *
 * Watches the the given @item for property changes.
 *
 * Callers are responsible to keep this watch as long as both
 * @self and @item are alive. To destroy the watch, use
 * bobgui_filter_unwatch.
 *
 * Returns: (transfer full) (nullable): the expression watch
 *
 * Since: 4.20
 */
gpointer
bobgui_filter_watch (BobguiFilter              *self,
                  gpointer                item,
                  BobguiFilterWatchCallback  callback,
                  gpointer                user_data,
                  GDestroyNotify          destroy)
{
  BobguiFilterClassPrivate *priv;
  BobguiFilterClass *class;

  g_return_val_if_fail (BOBGUI_IS_FILTER (self), NULL);

  class = BOBGUI_FILTER_GET_CLASS (self);
  priv = G_TYPE_CLASS_GET_PRIVATE (class, BOBGUI_TYPE_FILTER, BobguiFilterClassPrivate);

  return priv->watch (self, item, callback, user_data, destroy);
}

/*<private>
 * bobgui_filter_unwatch:
 * @self: a filter
 * @watch: (transfer full): The item to watch
 *
 * Stops @watch. This is only called with what was previously returned
 * by [vfunc@Bobgui.Filter.watch].
 *
 * Since: 4.20
 */
void
bobgui_filter_unwatch (BobguiFilter *self,
                    gpointer   watch)
{
  BobguiFilterClassPrivate *priv;
  BobguiFilterClass *class;

  g_return_if_fail (BOBGUI_IS_FILTER (self));
  g_return_if_fail (watch != NULL);

  class = BOBGUI_FILTER_GET_CLASS (self);
  priv = G_TYPE_CLASS_GET_PRIVATE (class, BOBGUI_TYPE_FILTER, BobguiFilterClassPrivate);

  priv->unwatch (self, watch);
}
