/* Lists/Weather
 *
 * This demo shows a few of the rarer features of BobguiListView and
 * how they can be used to display weather information.
 *
 * The hourly weather info uses a horizontal listview. This is easy
 * to achieve because BobguiListView implements the BobguiOrientable interface.
 * To make the items in the list stand out more, the listview uses
 * separators.
 *
 * A BobguiNoSelectionModel is used to make sure no item in the list can be
 * selected. All other interactions with the items is still possible.
 *
 * The dataset used here has 70 000 items.
 */

#include <bobgui/bobgui.h>

#define BOBGUI_TYPE_WEATHER_INFO (bobgui_weather_info_get_type ())

G_DECLARE_FINAL_TYPE (BobguiWeatherInfo, bobgui_weather_info, BOBGUI, WEATHER_INFO, GObject)

typedef enum {
  BOBGUI_WEATHER_CLEAR,
  BOBGUI_WEATHER_FEW_CLOUDS,
  BOBGUI_WEATHER_FOG,
  BOBGUI_WEATHER_OVERCAST,
  BOBGUI_WEATHER_SCATTERED_SHOWERS,
  BOBGUI_WEATHER_SHOWERS,
  BOBGUI_WEATHER_SNOW,
  BOBGUI_WEATHER_STORM
} BobguiWeatherType;

struct _BobguiWeatherInfo
{
  GObject parent_instance;

  gint64 timestamp;
  int temperature;
  BobguiWeatherType weather_type;
};

struct _BobguiWeatherInfoClass
{
  GObjectClass parent_class;
};

G_DEFINE_TYPE (BobguiWeatherInfo, bobgui_weather_info, G_TYPE_OBJECT)

static void
bobgui_weather_info_class_init (BobguiWeatherInfoClass *klass)
{
}

static void
bobgui_weather_info_init (BobguiWeatherInfo *self)
{
}

static BobguiWeatherInfo *
bobgui_weather_info_new (GDateTime      *timestamp,
                      BobguiWeatherInfo *copy_from)
{
  BobguiWeatherInfo *result;

  result = g_object_new (BOBGUI_TYPE_WEATHER_INFO, NULL);

  result->timestamp = g_date_time_to_unix (timestamp);
  if (copy_from)
    {
      result->temperature = copy_from->temperature;
      result->weather_type = copy_from->weather_type;
    }

  return result;
}

static GDateTime *
parse_timestamp (const char *string,
                 GTimeZone  *_tz)
{
  char *with_seconds;
  GDateTime *result;

  with_seconds = g_strconcat (string, ":00", NULL);
  result = g_date_time_new_from_iso8601 (with_seconds, _tz);
  g_free (with_seconds);

  return result;
}

static BobguiWeatherType
parse_weather_type (const char     *clouds,
                    const char     *precip,
                    BobguiWeatherType  fallback)
{
  if (strstr (precip, "SN"))
    return BOBGUI_WEATHER_SNOW;

  if (strstr (precip, "TS"))
    return BOBGUI_WEATHER_STORM;

  if (strstr (precip, "DZ"))
    return BOBGUI_WEATHER_SCATTERED_SHOWERS;

  if (strstr (precip, "SH") || strstr (precip, "RA"))
    return BOBGUI_WEATHER_SHOWERS;

  if (strstr (precip, "FG"))
    return BOBGUI_WEATHER_FOG;

  if (g_str_equal (clouds, "M") ||
      g_str_equal (clouds, ""))
    return fallback;

  if (strstr (clouds, "OVC") ||
      strstr (clouds, "BKN"))
    return BOBGUI_WEATHER_OVERCAST;

  if (strstr (clouds, "BKN") ||
      strstr (clouds, "SCT"))
    return BOBGUI_WEATHER_FEW_CLOUDS;

  if (strstr (clouds, "VV"))
    return BOBGUI_WEATHER_FOG;

  return BOBGUI_WEATHER_CLEAR;
}

static double
parse_temperature (const char *s,
                   double      fallback)
{
  char *endptr;
  double d;

  d = g_ascii_strtod (s, &endptr);
  if (*endptr != '\0')
    return fallback;

  return d;
}

static GListModel *
create_weather_model (void)
{
  GListStore *store;
  GTimeZone *utc;
  GDateTime *timestamp;
  BobguiWeatherInfo *info;
  GBytes *data;
  char **lines;
  guint i;

  store = g_list_store_new (BOBGUI_TYPE_WEATHER_INFO);
  data = g_resources_lookup_data ("/listview_weather/listview_weather.txt", 0, NULL);
  lines = g_strsplit (g_bytes_get_data (data, NULL), "\n", 0);

  utc = g_time_zone_new_utc ();
  timestamp = g_date_time_new (utc, 2011, 1, 1, 0, 0, 0);
  info = bobgui_weather_info_new (timestamp, NULL);
  g_list_store_append (store, info);
  g_object_unref (info);

  for (i = 0; lines[i] != NULL && *lines[i]; i++)
    {
      char **fields;
      GDateTime *date;

      fields = g_strsplit (lines[i], ",", 0);
      date = parse_timestamp (fields[0], utc);
      while (g_date_time_difference (date, timestamp) > 30 * G_TIME_SPAN_MINUTE)
        {
          GDateTime *new_timestamp = g_date_time_add_hours (timestamp, 1);
          g_date_time_unref (timestamp);
          timestamp = new_timestamp;
          info = bobgui_weather_info_new (timestamp, info);
          g_list_store_append (store, info);
          g_object_unref (info);
        }

      info->temperature = parse_temperature (fields[1], info->temperature);
      info->weather_type = parse_weather_type (fields[2], fields[3], info->weather_type);
      g_date_time_unref (date);
      g_strfreev (fields);
    }

  g_date_time_unref (timestamp);
  g_strfreev (lines);
  g_bytes_unref (data);
  g_time_zone_unref (utc);

  return G_LIST_MODEL (store);
}

static void
setup_widget (BobguiSignalListItemFactory *factory,
              BobguiListItem              *list_item)
{
  BobguiWidget *box, *child;

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_list_item_set_child (list_item, box);

  child = bobgui_label_new (NULL);
  bobgui_label_set_width_chars (BOBGUI_LABEL (child), 5);
  bobgui_box_append (BOBGUI_BOX (box), child);

  child = bobgui_image_new ();
  bobgui_image_set_icon_size (BOBGUI_IMAGE (child), BOBGUI_ICON_SIZE_LARGE);
  bobgui_box_append (BOBGUI_BOX (box), child);

  child = bobgui_label_new (NULL);
  bobgui_widget_set_vexpand (child, TRUE);
  bobgui_widget_set_valign (child, BOBGUI_ALIGN_END);
  bobgui_label_set_width_chars (BOBGUI_LABEL (child), 4);
  bobgui_box_append (BOBGUI_BOX (box), child);
}

static void
bind_widget (BobguiSignalListItemFactory *factory,
             BobguiListItem              *list_item)
{
  BobguiWidget *box, *child;
  BobguiWeatherInfo *info;
  GDateTime *timestamp;
  char *s;

  box = bobgui_list_item_get_child (list_item);
  info = bobgui_list_item_get_item (list_item);

  child = bobgui_widget_get_first_child (box);
  timestamp = g_date_time_new_from_unix_utc (info->timestamp);
  s = g_date_time_format (timestamp, "%R");
  bobgui_label_set_text (BOBGUI_LABEL (child), s);
  g_free (s);
  g_date_time_unref (timestamp);

  child = bobgui_widget_get_next_sibling (child);
  switch (info->weather_type)
  {
  case BOBGUI_WEATHER_CLEAR:
    bobgui_image_set_from_icon_name (BOBGUI_IMAGE (child), "weather-clear-symbolic");
    break;
  case BOBGUI_WEATHER_FEW_CLOUDS:
    bobgui_image_set_from_icon_name (BOBGUI_IMAGE (child), "weather-few-clouds-symbolic");
    break;
  case BOBGUI_WEATHER_FOG:
    bobgui_image_set_from_icon_name (BOBGUI_IMAGE (child), "weather-fog-symbolic");
    break;
  case BOBGUI_WEATHER_OVERCAST:
    bobgui_image_set_from_icon_name (BOBGUI_IMAGE (child), "weather-overcast-symbolic");
    break;
  case BOBGUI_WEATHER_SCATTERED_SHOWERS:
    bobgui_image_set_from_icon_name (BOBGUI_IMAGE (child), "weather-showers-scattered-symbolic");
    break;
  case BOBGUI_WEATHER_SHOWERS:
    bobgui_image_set_from_icon_name (BOBGUI_IMAGE (child), "weather-showers-symbolic");
    break;
  case BOBGUI_WEATHER_SNOW:
    bobgui_image_set_from_icon_name (BOBGUI_IMAGE (child), "weather-snow-symbolic");
    break;
  case BOBGUI_WEATHER_STORM:
    bobgui_image_set_from_icon_name (BOBGUI_IMAGE (child), "weather-storm-symbolic");
    break;
  default:
    bobgui_image_clear (BOBGUI_IMAGE (child));
    break;
  }


  child = bobgui_widget_get_next_sibling (child);
  s = g_strdup_printf ("%d°", info->temperature);
  bobgui_label_set_text (BOBGUI_LABEL (child), s);
  g_free (s);
}

static BobguiWidget *window = NULL;

BobguiWidget *
create_weather_view (void)
{
  BobguiWidget *listview;
  BobguiSelectionModel *model;
  BobguiListItemFactory *factory;

  factory = bobgui_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_widget), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_widget), NULL);
  model = BOBGUI_SELECTION_MODEL (bobgui_no_selection_new (create_weather_model ()));
  listview = bobgui_list_view_new (model, factory);
  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (listview), BOBGUI_ORIENTATION_HORIZONTAL);
  bobgui_list_view_set_show_separators (BOBGUI_LIST_VIEW (listview), TRUE);

  return listview;
}

BobguiWidget *
do_listview_weather (BobguiWidget *do_widget)
{
  if (window == NULL)
    {
      BobguiWidget *listview, *sw;

      window = bobgui_window_new ();
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 600, 400);
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Weather");
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Weather");
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *) &window);

      sw = bobgui_scrolled_window_new ();
      bobgui_window_set_child (BOBGUI_WINDOW (window), sw);
      listview = create_weather_view ();
      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), listview);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
