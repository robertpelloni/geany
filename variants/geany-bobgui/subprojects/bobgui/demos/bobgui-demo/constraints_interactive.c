/* Constraints/Interactive Constraints
 * #Keywords: BobguiConstraintLayout
 *
 * This example shows how constraints can be updated during user interaction.
 * The vertical edge between the buttons can be dragged with the mouse.
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>

G_DECLARE_FINAL_TYPE (InteractiveGrid, interactive_grid, INTERACTIVE, GRID, BobguiWidget)

struct _InteractiveGrid
{
  BobguiWidget parent_instance;

  BobguiWidget *button1, *button2;
  BobguiWidget *button3;
  BobguiConstraintGuide *guide;
  BobguiConstraint *constraint;
};

G_DEFINE_TYPE (InteractiveGrid, interactive_grid, BOBGUI_TYPE_WIDGET)

static void
interactive_grid_dispose (GObject *object)
{
  InteractiveGrid *self = INTERACTIVE_GRID (object);

  g_clear_pointer (&self->button1, bobgui_widget_unparent);
  g_clear_pointer (&self->button2, bobgui_widget_unparent);
  g_clear_pointer (&self->button3, bobgui_widget_unparent);

  G_OBJECT_CLASS (interactive_grid_parent_class)->dispose (object);
}

static void
interactive_grid_class_init (InteractiveGridClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = interactive_grid_dispose;

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_CONSTRAINT_LAYOUT);
}

static void
build_constraints (InteractiveGrid          *self,
                   BobguiConstraintLayout *manager)
{
  self->guide = g_object_new (BOBGUI_TYPE_CONSTRAINT_GUIDE, NULL);
  bobgui_constraint_layout_add_guide (manager, self->guide);

  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new_constant (BOBGUI_CONSTRAINT_TARGET (self->guide),
                        BOBGUI_CONSTRAINT_ATTRIBUTE_WIDTH,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        0.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));

  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (NULL,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_START,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        BOBGUI_CONSTRAINT_TARGET (self->button1),
                        BOBGUI_CONSTRAINT_ATTRIBUTE_START,
                        1.0,
                        -8.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (BOBGUI_CONSTRAINT_TARGET (self->button1),
                        BOBGUI_CONSTRAINT_ATTRIBUTE_END,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        BOBGUI_CONSTRAINT_TARGET (self->guide),
                        BOBGUI_CONSTRAINT_ATTRIBUTE_START,
                        1.0,
                        0.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (BOBGUI_CONSTRAINT_TARGET (self->button2),
                        BOBGUI_CONSTRAINT_ATTRIBUTE_START,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        BOBGUI_CONSTRAINT_TARGET (self->guide),
                        BOBGUI_CONSTRAINT_ATTRIBUTE_END,
                        1.0,
                        0.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (BOBGUI_CONSTRAINT_TARGET (self->button2),
                        BOBGUI_CONSTRAINT_ATTRIBUTE_END,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        NULL,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_END,
                        1.0,
                        -8.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (NULL,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_START,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        BOBGUI_CONSTRAINT_TARGET (self->button3),
                        BOBGUI_CONSTRAINT_ATTRIBUTE_START,
                        1.0,
                        -8.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));

  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (BOBGUI_CONSTRAINT_TARGET (self->button3),
                        BOBGUI_CONSTRAINT_ATTRIBUTE_END,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        BOBGUI_CONSTRAINT_TARGET (self->guide),
                        BOBGUI_CONSTRAINT_ATTRIBUTE_START,
                        1.0,
                        0.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));

  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (NULL,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_TOP,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        BOBGUI_CONSTRAINT_TARGET (self->button1),
                        BOBGUI_CONSTRAINT_ATTRIBUTE_TOP,
                        1.0,
                        -8.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (BOBGUI_CONSTRAINT_TARGET (self->button2),
                        BOBGUI_CONSTRAINT_ATTRIBUTE_TOP,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        BOBGUI_CONSTRAINT_TARGET (self->button1),
                        BOBGUI_CONSTRAINT_ATTRIBUTE_BOTTOM,
                        1.0,
                        0.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (BOBGUI_CONSTRAINT_TARGET (self->button3),
                        BOBGUI_CONSTRAINT_ATTRIBUTE_TOP,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        BOBGUI_CONSTRAINT_TARGET (self->button2),
                        BOBGUI_CONSTRAINT_ATTRIBUTE_BOTTOM,
                        1.0,
                        0.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (BOBGUI_CONSTRAINT_TARGET (self->button3),
                        BOBGUI_CONSTRAINT_ATTRIBUTE_BOTTOM,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        NULL,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_BOTTOM,
                        1.0,
                        -8.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
}

static void
drag_cb (BobguiGestureDrag  *drag,
         double           offset_x,
         double           offset_y,
         InteractiveGrid *self)
{
  BobguiConstraintLayout *layout = BOBGUI_CONSTRAINT_LAYOUT (bobgui_widget_get_layout_manager (BOBGUI_WIDGET (self)));
  double x, y;

  if (self->constraint)
    {
      bobgui_constraint_layout_remove_constraint (layout, self->constraint);
      g_clear_object (&self->constraint);
    }

  bobgui_gesture_drag_get_start_point (drag, &x, &y);
  self->constraint = bobgui_constraint_new_constant (BOBGUI_CONSTRAINT_TARGET (self->guide),
                                                  BOBGUI_CONSTRAINT_ATTRIBUTE_LEFT,
                                                  BOBGUI_CONSTRAINT_RELATION_EQ,
                                                  x + offset_x,
                                                  BOBGUI_CONSTRAINT_STRENGTH_REQUIRED);
  bobgui_constraint_layout_add_constraint (layout, g_object_ref (self->constraint));
  bobgui_widget_queue_allocate (BOBGUI_WIDGET (self));
}

static void
interactive_grid_init (InteractiveGrid *self)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  BobguiGesture *drag;

  self->button1 = bobgui_button_new_with_label ("Child 1");
  bobgui_widget_set_parent (self->button1, widget);
  bobgui_widget_set_name (self->button1, "button1");

  self->button2 = bobgui_button_new_with_label ("Child 2");
  bobgui_widget_set_parent (self->button2, widget);
  bobgui_widget_set_name (self->button2, "button2");

  self->button3 = bobgui_button_new_with_label ("Child 3");
  bobgui_widget_set_parent (self->button3, widget);
  bobgui_widget_set_name (self->button3, "button3");

  BobguiLayoutManager *manager = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (self));
  build_constraints (self, BOBGUI_CONSTRAINT_LAYOUT (manager));

  drag = bobgui_gesture_drag_new ();
  g_signal_connect (drag, "drag-update", G_CALLBACK (drag_cb), self);
  bobgui_widget_add_controller (BOBGUI_WIDGET (self), BOBGUI_EVENT_CONTROLLER (drag));
}

BobguiWidget *
do_constraints_interactive (BobguiWidget *do_widget)
{
 static BobguiWidget *window;

 if (!window)
   {
     BobguiWidget *box, *grid;

     window = bobgui_window_new ();
     bobgui_window_set_display (BOBGUI_WINDOW (window), bobgui_widget_get_display (do_widget));
     bobgui_window_set_title (BOBGUI_WINDOW (window), "Interactive Constraints");
     bobgui_window_set_default_size (BOBGUI_WINDOW (window), 260, -1);
     g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

     box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 12);
     bobgui_window_set_child (BOBGUI_WINDOW (window), box);

     grid = g_object_new (interactive_grid_get_type (), NULL);
     bobgui_widget_set_hexpand (grid, TRUE);
     bobgui_widget_set_vexpand (grid, TRUE);
     bobgui_box_append (BOBGUI_BOX (box), grid);
   }

 if (!bobgui_widget_get_visible (window))
   bobgui_widget_set_visible (window, TRUE);
 else
   bobgui_window_destroy (BOBGUI_WINDOW (window));

 return window;
}
