/* Constraints/Builder
 *
 * BobguiConstraintLayouts can be created in .ui files, and constraints can
 * be set up at that time as well, as this example demonstrates. It shows
 * various ways to do spacing and sizing with constraints.
 *
 * Make the window wider to see the rows react differently
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>

G_DECLARE_FINAL_TYPE (ConstraintsGrid, constraints_grid, CONSTRAINTS, GRID, BobguiWidget)

struct _ConstraintsGrid
{
  BobguiWidget parent_instance;
};

G_DEFINE_TYPE (ConstraintsGrid, constraints_grid, BOBGUI_TYPE_WIDGET)

static void
constraints_grid_init (ConstraintsGrid *grid)
{
}

static void
constraints_grid_dispose (GObject *object)
{
  BobguiWidget *widget = BOBGUI_WIDGET (object);
  BobguiWidget *child;

  while ((child = bobgui_widget_get_first_child (widget)))
    bobgui_widget_unparent (child);

  G_OBJECT_CLASS (constraints_grid_parent_class)->dispose (object);
}

static void
constraints_grid_class_init (ConstraintsGridClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = constraints_grid_dispose;
}

BobguiWidget *
do_constraints_builder (BobguiWidget *do_widget)
{
 static BobguiWidget *window;

 if (!window)
   {
     BobguiBuilder *builder;

     g_type_ensure (constraints_grid_get_type ());

     builder = bobgui_builder_new_from_resource ("/constraints_builder/constraints_builder.ui");

     window = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "window1"));
     bobgui_window_set_display (BOBGUI_WINDOW (window),
                             bobgui_widget_get_display (do_widget));
     g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

     g_object_unref (builder);
   }

 if (!bobgui_widget_get_visible (window))
   bobgui_widget_set_visible (window, TRUE);
 else
   bobgui_window_destroy (BOBGUI_WINDOW (window));

 return window;
}
