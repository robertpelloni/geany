/* Constraints/Simple Constraints
 * #Keywords: BobguiLayoutManager
 *
 * BobguiConstraintLayout provides a layout manager that uses relations
 * between widgets (also known as “constraints”) to compute the position
 * and size of each child.
 *
 * In addition to child widgets, the constraints can involve spacer
 * objects (also known as “guides”). This example has a guide between
 * the two buttons in the top row.
 *
 * Try resizing the window to see how the constraints react to update
 * the layout.
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>

G_DECLARE_FINAL_TYPE (SimpleGrid, simple_grid, SIMPLE, GRID, BobguiWidget)

struct _SimpleGrid
{
  BobguiWidget parent_instance;

  BobguiWidget *button1, *button2;
  BobguiWidget *button3;
};

G_DEFINE_TYPE (SimpleGrid, simple_grid, BOBGUI_TYPE_WIDGET)

static void
simple_grid_dispose (GObject *object)
{
  SimpleGrid *self = SIMPLE_GRID (object);

  g_clear_pointer (&self->button1, bobgui_widget_unparent);
  g_clear_pointer (&self->button2, bobgui_widget_unparent);
  g_clear_pointer (&self->button3, bobgui_widget_unparent);

  G_OBJECT_CLASS (simple_grid_parent_class)->dispose (object);
}

static void
simple_grid_class_init (SimpleGridClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);

  object_class->dispose = simple_grid_dispose;

  bobgui_widget_class_set_layout_manager_type (widget_class, BOBGUI_TYPE_CONSTRAINT_LAYOUT);
}

/* Layout:
 *
 *   +-------------------------------------+
 *   | +-----------++-------++-----------+ |
 *   | |  Child 1  || Space ||  Child 2  | |
 *   | +-----------++-------++-----------+ |
 *   | +---------------------------------+ |
 *   | |             Child 3             | |
 *   | +---------------------------------+ |
 *   +-------------------------------------+
 *
 * Constraints:
 *
 *   super.start = child1.start - 8
 *   child1.width = child2.width
 *   child1.end = space.start
 *   space.end = child2.start
 *   child2.end = super.end - 8
 *   super.start = child3.start - 8
 *   child3.end = super.end - 8
 *   super.top = child1.top - 8
 *   super.top = child2.top - 8
 *   child1.bottom = child3.top - 12
 *   child2.bottom = child3.top - 12
 *   child3.height = child1.height
 *   child3.height = child2.height
 *   child3.bottom = super.bottom - 8
 *
 * To add some flexibility, we make the space
 * stretchable:
 *
 *   space.width >= 10
 *   space.width = 100
 *   space.width <= 200
 */
static void
build_constraints (SimpleGrid          *self,
                   BobguiConstraintLayout *manager)
{
  BobguiConstraintGuide *guide;

  guide = bobgui_constraint_guide_new ();
  bobgui_constraint_guide_set_name (guide, "space");
  bobgui_constraint_guide_set_min_size (guide, 10, 10);
  bobgui_constraint_guide_set_nat_size (guide, 100, 10);
  bobgui_constraint_guide_set_max_size (guide, 200, 20);
  bobgui_constraint_guide_set_strength (guide, BOBGUI_CONSTRAINT_STRENGTH_STRONG);
  bobgui_constraint_layout_add_guide (manager, guide);

  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new_constant (BOBGUI_CONSTRAINT_TARGET (self->button1),
                        BOBGUI_CONSTRAINT_ATTRIBUTE_WIDTH,
                        BOBGUI_CONSTRAINT_RELATION_LE,
                        200.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (NULL,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_START,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        self->button1,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_START,
                        1.0,
                        -8.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (self->button1,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_WIDTH,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        self->button2,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_WIDTH,
                        1.0,
                        0.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (self->button1,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_END,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        guide,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_START,
                        1.0,
                        0.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (guide,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_END,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        self->button2,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_START,
                        1.0,
                        0.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (self->button2,
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
                        self->button3,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_START,
                        1.0,
                        -8.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (self->button3,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_END,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        NULL,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_END,
                        1.0,
                        -8.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (NULL,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_TOP,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        self->button1,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_TOP,
                        1.0,
                        -8.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (NULL,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_TOP,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        self->button2,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_TOP,
                        1.0,
                        -8.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (self->button1,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_BOTTOM,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        self->button3,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_TOP,
                        1.0,
                        -12.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (self->button2,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_BOTTOM,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        self->button3,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_TOP,
                        1.0,
                        -12.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (self->button3,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_HEIGHT,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        self->button1,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_HEIGHT,
                        1.0,
                        0.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (self->button3,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_HEIGHT,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        self->button2,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_HEIGHT,
                        1.0,
                        0.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
  bobgui_constraint_layout_add_constraint (manager,
    bobgui_constraint_new (self->button3,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_BOTTOM,
                        BOBGUI_CONSTRAINT_RELATION_EQ,
                        NULL,
                        BOBGUI_CONSTRAINT_ATTRIBUTE_BOTTOM,
                        1.0,
                        -8.0,
                        BOBGUI_CONSTRAINT_STRENGTH_REQUIRED));
}

static void
simple_grid_init (SimpleGrid *self)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);

  self->button1 = bobgui_button_new_with_label ("Child 1");
  bobgui_widget_set_parent (self->button1, widget);

  self->button2 = bobgui_button_new_with_label ("Child 2");
  bobgui_widget_set_parent (self->button2, widget);

  self->button3 = bobgui_button_new_with_label ("Child 3");
  bobgui_widget_set_parent (self->button3, widget);

  BobguiLayoutManager *manager = bobgui_widget_get_layout_manager (BOBGUI_WIDGET (self));
  build_constraints (self, BOBGUI_CONSTRAINT_LAYOUT (manager));
}

BobguiWidget *
do_constraints (BobguiWidget *do_widget)
{
 static BobguiWidget *window;

 if (!window)
   {
     BobguiWidget *box, *grid;

     window = bobgui_window_new ();
     bobgui_window_set_display (BOBGUI_WINDOW (window), bobgui_widget_get_display (do_widget));
     bobgui_window_set_title (BOBGUI_WINDOW (window), "Simple Constraints");
     bobgui_window_set_default_size (BOBGUI_WINDOW (window), 260, -1);
     g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

     box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 12);
     bobgui_window_set_child (BOBGUI_WINDOW (window), box);

     grid = g_object_new (simple_grid_get_type (), NULL);
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
