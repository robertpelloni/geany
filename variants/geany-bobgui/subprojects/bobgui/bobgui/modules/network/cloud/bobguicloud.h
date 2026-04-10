/* bobgui/modules/cloud/bobguicloud.h */
#ifndef BOBGUI_CLOUD_H
#define BOBGUI_CLOUD_H

#include <glib-object.h>

G_BEGIN_DECLS

/* Cloud Backend-as-a-Service (Better than Firebase / Supabase) */
#define BOBGUI_TYPE_CLOUD_CONTEXT (bobgui_cloud_context_get_type ())
G_DECLARE_FINAL_TYPE (BobguiCloudContext, bobgui_cloud_context, BOBGUI, CLOUD_CONTEXT, GObject)

BobguiCloudContext * bobgui_cloud_context_new (const char *api_url, const char *api_key);

/* Integrated Identity and Auth (Unmatched Parity & Security) */
void bobgui_cloud_auth_sign_up (BobguiCloudContext *ctx, const char *email, const char *pass, GAsyncReadyCallback callback);
void bobgui_cloud_auth_sign_in (BobguiCloudContext *ctx, const char *email, const char *pass, GAsyncReadyCallback callback);

/* Real-time Cloud Store (Superior to standard REST calls) */
void bobgui_cloud_db_sync (BobguiCloudContext *ctx, 
                          const char *collection, 
                          GCallback on_change, 
                          gpointer user_data);

/* Integrated S3/Storage (Better than standard manual uploads) */
void bobgui_cloud_upload_asset (BobguiCloudContext *ctx, 
                               const char *path, 
                               GAsyncReadyCallback callback);

G_END_DECLS

#endif /* BOBGUI_CLOUD_H */
