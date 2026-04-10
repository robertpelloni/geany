/* bobgui/modules/core/layout/bobguilayout.h */
#ifndef BOBGUI_LAYOUT_H
#define BOBGUI_LAYOUT_H

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

/* Flexbox and Grid Engine (Better than standard toolkits) */
typedef enum {
  BOBGUI_LAYOUT_TYPE_FLEX,
  BOBGUI_LAYOUT_TYPE_GRID,
  BOBGUI_LAYOUT_TYPE_ABSOLUTE
} BobguiLayoutType;

#define BOBGUI_TYPE_LAYOUT_MANAGER (bobgui_layout_manager_get_type ())
G_DECLARE_FINAL_TYPE (BobguiLayoutManager, bobgui_layout_manager, BOBGUI, LAYOUT_MANAGER, BobguiLayoutManager)

BobguiLayoutManager * bobgui_layout_manager_new (BobguiLayoutType type);

/* Flexbox Properties (Superior Parity with Web/JavaFX) */
void bobgui_layout_set_flex_direction (BobguiLayoutManager *self, const char *direction);
void bobgui_layout_set_justify_content (BobguiLayoutManager *self, const char *justify);
void bobgui_layout_set_align_items (BobguiLayoutManager *self, const char *align);

/* Grid Properties (Better than manual coordinate management) */
void bobgui_layout_set_grid_template_columns (BobguiLayoutManager *self, const char *template);
void bobgui_layout_set_grid_template_rows    (BobguiLayoutManager *self, const char *template);

/* Application to Widget (Any container can use modern layout) */
void bobgui_container_set_layout_manager (BobguiWidget *container, BobguiLayoutManager *mgr);

G_END_DECLS

#endif /* BOBGUI_LAYOUT_H */
