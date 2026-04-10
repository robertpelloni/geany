/* List Box/Complex
 *
 * BobguiListBox allows lists with complicated layouts, using
 * regular widgets supporting sorting and filtering.
 */

#include <bobgui/bobgui.h>
#include <stdlib.h>
#include <string.h>

static GdkTexture *avatar_texture_other;
static BobguiWidget *window = NULL;

#define BOBGUI_TYPE_MESSAGE		  (bobgui_message_get_type ())
#define BOBGUI_MESSAGE(message)		  (G_TYPE_CHECK_INSTANCE_CAST ((message), BOBGUI_TYPE_MESSAGE, BobguiMessage))
#define BOBGUI_MESSAGE_CLASS(klass)		  (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_MESSAGE, BobguiMessageClass))
#define BOBGUI_IS_MESSAGE(message)		  (G_TYPE_CHECK_INSTANCE_TYPE ((message), BOBGUI_TYPE_MESSAGE))
#define BOBGUI_IS_MESSAGE_CLASS(klass)	  (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_MESSAGE))
#define BOBGUI_MESSAGE_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_MESSAGE, BobguiMessageClass))

#define BOBGUI_TYPE_MESSAGE_ROW		  (bobgui_message_row_get_type ())
#define BOBGUI_MESSAGE_ROW(message_row)		  (G_TYPE_CHECK_INSTANCE_CAST ((message_row), BOBGUI_TYPE_MESSAGE_ROW, BobguiMessageRow))
#define BOBGUI_MESSAGE_ROW_CLASS(klass)		  (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_MESSAGE_ROW, BobguiMessageRowClass))
#define BOBGUI_IS_MESSAGE_ROW(message_row)		  (G_TYPE_CHECK_INSTANCE_TYPE ((message_row), BOBGUI_TYPE_MESSAGE_ROW))
#define BOBGUI_IS_MESSAGE_ROW_CLASS(klass)	  (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_MESSAGE_ROW))
#define BOBGUI_MESSAGE_ROW_GET_CLASS(obj)         (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_MESSAGE_ROW, BobguiMessageRowClass))

typedef struct _BobguiMessage   BobguiMessage;
typedef struct _BobguiMessageClass  BobguiMessageClass;
typedef struct _BobguiMessageRow   BobguiMessageRow;
typedef struct _BobguiMessageRowClass  BobguiMessageRowClass;
typedef struct _BobguiMessageRowPrivate  BobguiMessageRowPrivate;


struct _BobguiMessage
{
  GObject parent;

  guint id;
  char *sender_name;
  char *sender_nick;
  char *message;
  gint64 time;
  guint reply_to;
  char *resent_by;
  int n_favorites;
  int n_reshares;
};

struct _BobguiMessageClass
{
  GObjectClass parent_class;
};

struct _BobguiMessageRow
{
  BobguiListBoxRow parent;

  BobguiMessageRowPrivate *priv;
};

struct _BobguiMessageRowClass
{
  BobguiListBoxRowClass parent_class;
};

struct _BobguiMessageRowPrivate
{
  BobguiMessage *message;
  BobguiRevealer *details_revealer;
  BobguiImage *avatar_image;
  BobguiWidget *extra_buttons_box;
  BobguiLabel *content_label;
  BobguiLabel *source_name;
  BobguiLabel *source_nick;
  BobguiLabel *short_time_label;
  BobguiLabel *detailed_time_label;
  BobguiBox *resent_box;
  BobguiLinkButton *resent_by_button;
  BobguiLabel *n_favorites_label;
  BobguiLabel *n_reshares_label;
  BobguiButton *expand_button;
};

GType      bobgui_message_get_type  (void) G_GNUC_CONST;
GType      bobgui_message_row_get_type  (void) G_GNUC_CONST;

G_DEFINE_TYPE (BobguiMessage, bobgui_message, G_TYPE_OBJECT);

static void
bobgui_message_finalize (GObject *obj)
{
  BobguiMessage *msg = BOBGUI_MESSAGE (obj);

  g_free (msg->sender_name);
  g_free (msg->sender_nick);
  g_free (msg->message);
  g_free (msg->resent_by);

  G_OBJECT_CLASS (bobgui_message_parent_class)->finalize (obj);
}
static void
bobgui_message_class_init (BobguiMessageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = bobgui_message_finalize;
}

static void
bobgui_message_init (BobguiMessage *msg)
{
}

static void
bobgui_message_parse (BobguiMessage *msg, const char *str)
{
  char **strv;
  int i;

  strv = g_strsplit (str, "|", 0);

  i = 0;
  msg->id = strtol (strv[i++], NULL, 10);
  msg->sender_name = g_strdup (strv[i++]);
  msg->sender_nick = g_strdup (strv[i++]);
  msg->message = g_strdup (strv[i++]);
  msg->time = strtol (strv[i++], NULL, 10);
  if (strv[i])
    {
      msg->reply_to = strtol (strv[i++], NULL, 10);
      if (strv[i])
        {
          if (*strv[i])
            msg->resent_by = g_strdup (strv[i]);
          i++;
          if (strv[i])
            {
              msg->n_favorites = strtol (strv[i++], NULL, 10);
              if (strv[i])
                {
                  msg->n_reshares = strtol (strv[i++], NULL, 10);
                }

            }
        }
    }

  g_strfreev (strv);
}

static BobguiMessage *
bobgui_message_new (const char *str)
{
  BobguiMessage *msg;
  msg = g_object_new (bobgui_message_get_type (), NULL);
  bobgui_message_parse (msg, str);
  return msg;
}

G_DEFINE_TYPE_WITH_PRIVATE (BobguiMessageRow, bobgui_message_row, BOBGUI_TYPE_LIST_BOX_ROW);


static void
bobgui_message_row_update (BobguiMessageRow *row)
{
  BobguiMessageRowPrivate *priv = row->priv;
  GDateTime *t;
  char *s;

  bobgui_label_set_text (priv->source_name, priv->message->sender_name);
  bobgui_label_set_text (priv->source_nick, priv->message->sender_nick);
  bobgui_label_set_text (priv->content_label, priv->message->message);
  t = g_date_time_new_from_unix_utc (priv->message->time);
  s = g_date_time_format (t, "%e %b %y");
  bobgui_label_set_text (priv->short_time_label, s);
  g_free (s);
  s = g_date_time_format (t, "%X - %e %b %Y");
  bobgui_label_set_text (priv->detailed_time_label, s);
  g_free (s);
  g_date_time_unref (t);

  bobgui_widget_set_visible (BOBGUI_WIDGET(priv->n_favorites_label),
                          priv->message->n_favorites != 0);
  s = g_strdup_printf ("<b>%d</b>\nFavorites", priv->message->n_favorites);
  bobgui_label_set_markup (priv->n_favorites_label, s);
  g_free (s);

  bobgui_widget_set_visible (BOBGUI_WIDGET(priv->n_reshares_label),
                          priv->message->n_reshares != 0);
  s = g_strdup_printf ("<b>%d</b>\nReshares", priv->message->n_reshares);
  bobgui_label_set_markup (priv->n_reshares_label, s);
  g_free (s);

  bobgui_widget_set_visible (BOBGUI_WIDGET (priv->resent_box), priv->message->resent_by != NULL);
  if (priv->message->resent_by)
    bobgui_button_set_label (BOBGUI_BUTTON (priv->resent_by_button), priv->message->resent_by);

  if (strcmp (priv->message->sender_nick, "BOBGUItoolkit") == 0)
    bobgui_image_set_from_icon_name (priv->avatar_image, "org.bobgui.Demo4");
  else
    bobgui_image_set_from_paintable (priv->avatar_image, GDK_PAINTABLE (avatar_texture_other));

}

static void
bobgui_message_row_expand (BobguiMessageRow *row)
{
  BobguiMessageRowPrivate *priv = row->priv;
  gboolean expand;

  expand = !bobgui_revealer_get_reveal_child (priv->details_revealer);

  bobgui_revealer_set_reveal_child (priv->details_revealer, expand);
  if (expand)
    bobgui_button_set_label (priv->expand_button, "Hide");
  else
    bobgui_button_set_label (priv->expand_button, "Expand");
}

static void
expand_clicked (BobguiMessageRow *row,
                BobguiButton *button)
{
  bobgui_message_row_expand (row);
}

static void
reshare_clicked (BobguiMessageRow *row,
                 BobguiButton *button)
{
  BobguiMessageRowPrivate *priv = row->priv;

  priv->message->n_reshares++;
  bobgui_message_row_update (row);
}

static void
favorite_clicked (BobguiMessageRow *row,
                  BobguiButton *button)
{
  BobguiMessageRowPrivate *priv = row->priv;

  priv->message->n_favorites++;
  bobgui_message_row_update (row);
}

static void
bobgui_message_row_state_flags_changed (BobguiWidget    *widget,
                                     BobguiStateFlags previous_state_flags)
{
  BobguiMessageRowPrivate *priv = BOBGUI_MESSAGE_ROW (widget)->priv;
  BobguiStateFlags flags;
  gboolean visible;

  flags = bobgui_widget_get_state_flags (widget);

  visible = flags & (BOBGUI_STATE_FLAG_PRELIGHT | BOBGUI_STATE_FLAG_SELECTED) ? TRUE : FALSE;
  bobgui_widget_set_visible (priv->extra_buttons_box, visible);

  BOBGUI_WIDGET_CLASS (bobgui_message_row_parent_class)->state_flags_changed (widget, previous_state_flags);
}

static void
bobgui_message_row_dispose (GObject *obj)
{
  bobgui_widget_dispose_template (BOBGUI_WIDGET (obj), BOBGUI_TYPE_MESSAGE_ROW);
  G_OBJECT_CLASS (bobgui_message_row_parent_class)->dispose (obj);
}

static void
bobgui_message_row_finalize (GObject *obj)
{
  BobguiMessageRowPrivate *priv = BOBGUI_MESSAGE_ROW (obj)->priv;
  g_object_unref (priv->message);
  G_OBJECT_CLASS (bobgui_message_row_parent_class)->finalize (obj);
}

static void
bobgui_message_row_class_init (BobguiMessageRowClass *klass)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = bobgui_message_row_dispose;
  object_class->finalize = bobgui_message_row_finalize;

  bobgui_widget_class_set_template_from_resource (widget_class, "/listbox/listbox.ui");
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiMessageRow, content_label);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiMessageRow, source_name);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiMessageRow, source_nick);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiMessageRow, short_time_label);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiMessageRow, detailed_time_label);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiMessageRow, extra_buttons_box);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiMessageRow, details_revealer);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiMessageRow, avatar_image);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiMessageRow, resent_box);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiMessageRow, resent_by_button);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiMessageRow, n_reshares_label);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiMessageRow, n_favorites_label);
  bobgui_widget_class_bind_template_child_private (widget_class, BobguiMessageRow, expand_button);
  bobgui_widget_class_bind_template_callback (widget_class, expand_clicked);
  bobgui_widget_class_bind_template_callback (widget_class, reshare_clicked);
  bobgui_widget_class_bind_template_callback (widget_class, favorite_clicked);

  widget_class->state_flags_changed = bobgui_message_row_state_flags_changed;
}

static void
bobgui_message_row_init (BobguiMessageRow *row)
{
  row->priv = bobgui_message_row_get_instance_private (row);

  bobgui_widget_init_template (BOBGUI_WIDGET (row));
}

static BobguiMessageRow *
bobgui_message_row_new (BobguiMessage *message)
{
  BobguiMessageRow *row;

  row = g_object_new (bobgui_message_row_get_type (), NULL);
  row->priv->message = message;
  bobgui_message_row_update (row);

  return row;
}

static int
bobgui_message_row_sort (BobguiMessageRow *a, BobguiMessageRow *b, gpointer data)
{
  return b->priv->message->time - a->priv->message->time;
}

static void
row_activated (BobguiListBox *listbox, BobguiListBoxRow *row)
{
  bobgui_message_row_expand (BOBGUI_MESSAGE_ROW (row));
}

BobguiWidget *
do_listbox (BobguiWidget *do_widget)
{
  BobguiWidget *scrolled, *listbox, *vbox, *label;
  BobguiMessage *message;
  BobguiMessageRow *row;
  GBytes *data;
  char **lines;
  int i;

  if (!window)
    {
      avatar_texture_other = gdk_texture_new_from_resource ("/listbox/apple-red.png");

      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "List Box — Complex");
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 400, 600);
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 12);
      bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);
      label = bobgui_label_new ("Messages from BOBGUI and friends");
      bobgui_box_append (BOBGUI_BOX (vbox), label);
      scrolled = bobgui_scrolled_window_new ();
      bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolled), BOBGUI_POLICY_NEVER, BOBGUI_POLICY_AUTOMATIC);
      bobgui_widget_set_vexpand (scrolled, TRUE);
      bobgui_box_append (BOBGUI_BOX (vbox), scrolled);
      listbox = bobgui_list_box_new ();
      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled), listbox);

      bobgui_list_box_set_sort_func (BOBGUI_LIST_BOX (listbox), (BobguiListBoxSortFunc)bobgui_message_row_sort, listbox, NULL);
      bobgui_list_box_set_activate_on_single_click (BOBGUI_LIST_BOX (listbox), FALSE);
      g_signal_connect (listbox, "row-activated", G_CALLBACK (row_activated), NULL);

      data = g_resources_lookup_data ("/listbox/messages.txt", 0, NULL);
      lines = g_strsplit (g_bytes_get_data (data, NULL), "\n", 0);

      for (i = 0; lines[i] != NULL && *lines[i]; i++)
        {
          message = bobgui_message_new (lines[i]);
          row = bobgui_message_row_new (message);
          bobgui_list_box_insert (BOBGUI_LIST_BOX (listbox), BOBGUI_WIDGET (row), -1);
        }

      g_strfreev (lines);
      g_bytes_unref (data);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
