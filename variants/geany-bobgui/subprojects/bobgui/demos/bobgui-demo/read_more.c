/* Read More
 * #Keywords: BobguiInscription
 *
 * A simple implementation of a widget that can either
 * display a lot of text or just the first few lines with a
 * "Read More" button.
 */

#include <bobgui/bobgui.h>

#define READ_TYPE_MORE (read_more_get_type ())
G_DECLARE_FINAL_TYPE(ReadMore, read_more, READ, MORE, BobguiWidget)

struct _ReadMore {
  BobguiWidget parent_instance;
 
  BobguiWidget *label;
  BobguiWidget *inscription;
  BobguiWidget *box;
  gboolean show_more;
};

G_DEFINE_TYPE (ReadMore, read_more, BOBGUI_TYPE_WIDGET)

static BobguiSizeRequestMode
read_more_get_request_mode (BobguiWidget *widget)
{
  return BOBGUI_SIZE_REQUEST_HEIGHT_FOR_WIDTH;
}

static void
read_more_measure (BobguiWidget      *widget,
                   BobguiOrientation  orientation,
                   int             for_size,
                   int            *minimum,
                   int            *natural,
                   int            *minimum_baseline,
                   int            *natural_baseline)
{
  ReadMore *self = READ_MORE (widget);
  int label_min, label_nat, label_min_baseline, label_nat_baseline;
  int box_min, box_nat, box_min_baseline, box_nat_baseline;
  int min_check;

  if (self->show_more)
    min_check = G_MAXINT;
  else if (for_size >= 0)
    bobgui_widget_measure (self->box, 1 - orientation, -1, &min_check, NULL, NULL, NULL);
  else
    min_check = -1;

  if (min_check > for_size)
    {
      bobgui_widget_measure (self->label,
                          orientation,
                          for_size,
                          minimum, natural,
                          minimum_baseline, natural_baseline);
      return;
    }

  else if (for_size >= 0)
    bobgui_widget_measure (self->label, 1 - orientation, -1, &min_check, NULL, NULL, NULL);
  else
    min_check = -1;

  if (min_check > for_size)
    {
      bobgui_widget_measure (self->box,
                          orientation,
                          for_size,
                          minimum, natural,
                          minimum_baseline, natural_baseline);
      return;
    }

  bobgui_widget_measure (self->label,
                      orientation,
                      for_size,
                      &label_min, &label_nat,
                      &label_min_baseline, &label_nat_baseline);
  bobgui_widget_measure (self->box,
                      orientation,
                      for_size,
                      &box_min, &box_nat,
                      &box_min_baseline, &box_nat_baseline);

  *minimum = MIN (label_min, box_min);
  *natural = MIN (label_nat, box_nat);

  /* FIXME: Figure out baselines! */
}

static void
read_more_allocate (BobguiWidget *widget,
                    int        width,
                    int        height,
                    int        baseline)
{
  ReadMore *self = READ_MORE (widget);
  gboolean show_more;

  if (self->show_more)
    {
      show_more = TRUE;
    }
  else
    {
      int needed;

      /* check to see if we have enough space to show all text */
      bobgui_widget_measure (self->label,
                          BOBGUI_ORIENTATION_VERTICAL,
                          width,
                          &needed, NULL, NULL, NULL);

      show_more = needed <= height;
    }

  bobgui_widget_set_child_visible (self->label, show_more);
  bobgui_widget_set_child_visible (self->box, !show_more);

  if (show_more)
    bobgui_widget_size_allocate (self->label, &(BobguiAllocation) { 0, 0, width, height }, baseline);
  else
    bobgui_widget_size_allocate (self->box, &(BobguiAllocation) { 0, 0, width, height }, baseline);
}

static void
read_more_dispose (GObject *object)
{
  ReadMore *self = READ_MORE (object);

  g_clear_pointer (&self->label, bobgui_widget_unparent);
  g_clear_pointer (&self->box, bobgui_widget_unparent);

  G_OBJECT_CLASS (read_more_parent_class)->dispose (object);
}

static void
read_more_class_init (ReadMoreClass *klass)
{
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  widget_class->get_request_mode = read_more_get_request_mode;
  widget_class->measure = read_more_measure;
  widget_class->size_allocate = read_more_allocate;

  object_class->dispose = read_more_dispose;
}

static void
read_more_clicked (BobguiButton *button,
                   ReadMore  *self)
{
  self->show_more = TRUE;
  bobgui_widget_queue_resize (BOBGUI_WIDGET (self));
}

static void
read_more_init (ReadMore *self)
{
  BobguiWidget *button;

  self->label = bobgui_label_new (NULL);
  bobgui_label_set_xalign (BOBGUI_LABEL (self->label), 0.0);
  bobgui_label_set_yalign (BOBGUI_LABEL (self->label), 0.0);
  bobgui_label_set_wrap (BOBGUI_LABEL (self->label), TRUE);
  bobgui_label_set_width_chars (BOBGUI_LABEL (self->label), 3);
  bobgui_label_set_max_width_chars (BOBGUI_LABEL (self->label), 30);
  bobgui_widget_set_parent (self->label, BOBGUI_WIDGET (self));

  self->box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_widget_set_vexpand (self->box, FALSE);
  bobgui_widget_set_parent (self->box, BOBGUI_WIDGET (self));

  self->inscription = bobgui_inscription_new (NULL);
  bobgui_inscription_set_xalign (BOBGUI_INSCRIPTION (self->inscription), 0.0);
  bobgui_inscription_set_yalign (BOBGUI_INSCRIPTION (self->inscription), 0.0);
  bobgui_inscription_set_min_lines (BOBGUI_INSCRIPTION (self->inscription), 3);
  bobgui_inscription_set_nat_chars (BOBGUI_INSCRIPTION (self->inscription), 30);
  bobgui_widget_set_vexpand (self->inscription, TRUE);
  bobgui_box_append (BOBGUI_BOX (self->box), self->inscription);
  
  button = bobgui_button_new_with_label ("Read More");
  g_signal_connect (button, "clicked", G_CALLBACK (read_more_clicked), self);
  bobgui_box_append (BOBGUI_BOX (self->box), button);
}

static void
read_more_set_text (ReadMore   *self,
                    const char *text)
{
  bobgui_label_set_label (BOBGUI_LABEL (self->label), text);
  bobgui_inscription_set_text (BOBGUI_INSCRIPTION (self->inscription), text);
}

static BobguiWidget *
read_more_new (const char *text)
{
  ReadMore *self = g_object_new (READ_TYPE_MORE, NULL);

  read_more_set_text (self, text);

  return BOBGUI_WIDGET (self);
}

BobguiWidget *
do_read_more (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *readmore;

      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Read More");
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      readmore = read_more_new (
"I'd just like to interject for a moment. What you're referring to as Linux, is in fact, GNU/Linux, or as I've recently taken to calling it, GNU plus Linux. Linux is not an operating system unto itself, but rather another free component of a fully functioning GNU system made useful by the GNU corelibs, shell utilities and vital system components comprising a full OS as defined by POSIX.\n"
"\n"
"Many computer users run a modified version of the GNU system every day, without realizing it. Through a peculiar turn of events, the version of GNU which is widely used today is often called \"Linux\", and many of its users are not aware that it is basically the GNU system, developed by the GNU Project.\n"
"\n"
"There really is a Linux, and these people are using it, but it is just a part of the system they use. Linux is the kernel: the program in the system that allocates the machine's resources to the other programs that you run. The kernel is an essential part of an operating system, but useless by itself; it can only function in the context of a complete operating system. Linux is normally used in combination with the GNU operating system: the whole system is basically GNU with Linux added, or GNU/Linux. All the so-called \"Linux\" distributions are really distributions of GNU/Linux.");
      bobgui_window_set_child (BOBGUI_WINDOW (window), readmore);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
