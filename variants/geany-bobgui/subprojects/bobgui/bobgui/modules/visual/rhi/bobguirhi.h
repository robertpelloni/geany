/* bobgui/modules/visual/rhi/bobguirhi.h */
#ifndef BOBGUI_RHI_H
#define BOBGUI_RHI_H

#include <glib-object.h>

G_BEGIN_DECLS

/* Rendering Hardware Interface (Grand Unified API) */
typedef enum {
    BOBGUI_RHI_BACKEND_VULKAN,
    BOBGUI_RHI_BACKEND_METAL,
    BOBGUI_RHI_BACKEND_D3D12,
    BOBGUI_RHI_BACKEND_OPENGL
} BobguiRhiBackend;

#define BOBGUI_TYPE_RHI_DEVICE (bobgui_rhi_device_get_type ())
G_DECLARE_FINAL_TYPE (BobguiRhiDevice, bobgui_rhi_device, BOBGUI, RHI_DEVICE, GObject)

BobguiRhiDevice * bobgui_rhi_device_new (BobguiRhiBackend backend);

/* Low-level Resource Management (Zero overhead abstraction) */
typedef struct _BobguiRhiBuffer BobguiRhiBuffer;
typedef struct _BobguiRhiTexture BobguiRhiTexture;

BobguiRhiBuffer * bobgui_rhi_create_buffer (BobguiRhiDevice *self, size_t size, uint32_t usage);
void               bobgui_rhi_submit_commands (BobguiRhiDevice *self, gpointer cmd_list);

/* Cross-Platform Shader Translation (SPIR-V to MSL/HLSL) */
void bobgui_rhi_translate_shader (const char *spirv_path, BobguiRhiBackend target);

G_END_DECLS

#endif /* BOBGUI_RHI_H */
