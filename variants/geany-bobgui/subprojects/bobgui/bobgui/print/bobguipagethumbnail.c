#include "config.h"

#include "bobguipagethumbnailprivate.h"

enum
{
  PROP_PAGE_NUM = 1,
  NUM_PROPERTIES,
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

struct _BobguiPageThumbnail
{
  BobguiWidget parent_instance;

  BobguiWidget *label;
  int page_num;
};

struct _BobguiPageThumbnailClass
{
  BobguiWidgetClass parent_class;
};

G_DEFINE_TYPE (BobguiPageThumbnail, bobgui_page_thumbnail, BOBGUI_TYPE_WIDGET)

static void
bobgui_page_thumbnail_init (BobguiPageThumbnail *self)
{
  self->label = bobgui_inscription_new ("0");
  bobgui_widget_set_parent (self->label, BOBGUI_WIDGET (self));
  bobgui_inscription_set_min_chars (BOBGUI_INSCRIPTION (self->label), 1);
  bobgui_inscription_set_nat_chars (BOBGUI_INSCRIPTION (self->label), 1);
}

static void
bobgui_page_thumbnail_dispose (GObject *object)
{
  BobguiPageThumbnail *self = BOBGUI_PAGE_THUMBNAIL (object);

  g_clear_pointer (&self->label, bobgui_widget_unparent);

  G_OBJECT_CLASS (bobgui_page_thumbnail_parent_class)->dispose (object);
}

static void
bobgui_page_thumbnail_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  BobguiPageThumbnail *self = BOBGUI_PAGE_THUMBNAIL (object);

  switch (prop_id)
    {
    case PROP_PAGE_NUM:
      bobgui_page_thumbnail_set_page_num (self, g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_page_thumbnail_get_property (GObject     *object,
                                 guint        prop_id,
                                 GValue      *value,
                                 GParamSpec  *pspec)
{
  BobguiPageThumbnail *self = BOBGUI_PAGE_THUMBNAIL (object);

  switch (prop_id)
    {
    case PROP_PAGE_NUM:
      g_value_set_int (value, self->page_num);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bobgui_page_thumbnail_size_allocate (BobguiWidget *widget,
                                  int        width,
                                  int        height,
                                  int        baseline)
{
  BobguiPageThumbnail *self = BOBGUI_PAGE_THUMBNAIL (widget);
  BobguiRequisition nat;
  BobguiAllocation alloc;

  bobgui_widget_get_preferred_size (self->label, NULL, &nat);
  alloc.x = width - nat.width;
  alloc.y = height - nat.height;
  alloc.width = nat.width;
  alloc.height = nat.height;
  bobgui_widget_size_allocate (self->label, &alloc, -1);
}

static void
bobgui_page_thumbnail_class_init (BobguiPageThumbnailClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (class);

  object_class->dispose = bobgui_page_thumbnail_dispose;
  object_class->set_property = bobgui_page_thumbnail_set_property;
  object_class->get_property = bobgui_page_thumbnail_get_property;

  widget_class->size_allocate = bobgui_page_thumbnail_size_allocate;

  properties[PROP_PAGE_NUM] =
      g_param_spec_int ("page-num", NULL, NULL,
                        0, G_MAXINT, 0, G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

  bobgui_widget_class_set_css_name (widget_class, "page-thumbnail");
}

BobguiPageThumbnail *
bobgui_page_thumbnail_new (void)
{
  return g_object_new (BOBGUI_TYPE_PAGE_THUMBNAIL, NULL);
}

void
bobgui_page_thumbnail_set_page_num (BobguiPageThumbnail *self,
                                 int               page_num)
{
  g_return_if_fail (BOBGUI_IS_PAGE_THUMBNAIL (self));
  g_return_if_fail (page_num >= 0);
  char text[64];

  if (self->page_num == page_num)
    return;

  self->page_num = page_num;

  g_snprintf (text, sizeof (text), "%d", self->page_num);

  bobgui_inscription_set_text (BOBGUI_INSCRIPTION (self->label), text);
  bobgui_widget_queue_draw (BOBGUI_WIDGET (self));
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PAGE_NUM]);
}

int
bobgui_page_thumbnail_get_page_num (BobguiPageThumbnail *self)
{
  g_return_val_if_fail (BOBGUI_IS_PAGE_THUMBNAIL (self), 0);

  return self->page_num;
}
