/* bobgui/modules/tools/forge/bobguiforge.h */
#ifndef BOBGUI_FORGE_H
#define BOBGUI_FORGE_H

#include <glib-object.h>

G_BEGIN_DECLS

/* Unified Build & Resource Orchestrator (Better than Projucer / CMake wrappers) */
#define BOBGUI_TYPE_FORGE_CONTEXT (bobgui_forge_context_get_type ())
G_DECLARE_FINAL_TYPE (BobguiForgeContext, bobgui_forge_context, BOBGUI, FORGE_CONTEXT, GObject)

BobguiForgeContext * bobgui_forge_context_new (const char *project_dir);

/* Resource Optimization (AI-driven Texture/Shader compression) */
void bobgui_forge_optimize_assets (BobguiForgeContext *self, GAsyncReadyCallback callback);

/* Automated Deployment (Package for Win/Mac/Linux/Android/XR) */
void bobgui_forge_build_target (BobguiForgeContext *self, 
                               const char *target_os, 
                               const char *target_arch);

/* Cross-Compilation of Shaders (Vulkan/Metal/D3D12 unified) */
void bobgui_forge_compile_shaders (BobguiForgeContext *self);

/* Code Generation (Integrated with BobguiMeta) */
void bobgui_forge_generate_boilerplate (BobguiForgeContext *self, const char *schema_path);

G_END_DECLS

#endif /* BOBGUI_FORGE_H */
