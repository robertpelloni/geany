/* bobgui/modules/media/holograph/bobguiholograph.h */
#ifndef BOBGUI_HOLOGRAPH_H
#define BOBGUI_HOLOGRAPH_H

#include <bobgui/bobgui.h>
#include <graphene.h>

G_BEGIN_DECLS

/* Volumetric Rendering Engine (Better than standard 3D toolkits) */
#define BOBGUI_TYPE_HOLOGRAM (bobgui_hologram_get_type ())
G_DECLARE_FINAL_TYPE (BobguiHologram, bobgui_hologram, BOBGUI, HOLOGRAM, GObject)

BobguiHologram * bobgui_hologram_new_from_voxels (gpointer data, int width, int height, int depth);

/* Light-field UI Projection (Superior Parity with futuristic displays) */
void bobgui_hologram_project_to_space (BobguiHologram *self, 
                                      graphene_matrix_t *basis);

/* Depth-Aware Interaction (Better than ray-casting on 2D planes) */
void bobgui_hologram_set_interaction_density (BobguiHologram *self, float threshold);

/* Holograph View Widget */
#define BOBGUI_TYPE_HOLOGRAPH_VIEW (bobgui_holograph_view_get_type ())
G_DECLARE_FINAL_TYPE (BobguiHolographView, bobgui_holograph_view, BOBGUI, HOLOGRAPH_VIEW, BobguiWidget)

BobguiHolographView * bobgui_holograph_view_new (void);
void                  bobgui_holograph_view_display (BobguiHolographView *self, BobguiHologram *hologram);

G_END_DECLS

#endif /* BOBGUI_HOLOGRAPH_H */
