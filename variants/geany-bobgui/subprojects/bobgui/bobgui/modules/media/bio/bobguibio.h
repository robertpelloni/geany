/* bobgui/modules/media/bio/bobguibio.h */
#ifndef BOBGUI_BIO_H
#define BOBGUI_BIO_H

#include <glib-object.h>
#include <bobgui/bobgui.h>

G_BEGIN_DECLS

/* Biological Data Engine (Better than standard health SDKs) */
#define BOBGUI_TYPE_BIO_MANAGER (bobgui_bio_manager_get_type ())
G_DECLARE_FINAL_TYPE (BobguiBioManager, bobgui_bio_manager, BOBGUI, BIO_MANAGER, GObject)

BobguiBioManager * bobgui_bio_manager_get_default (void);

/* Health Sensor Integration (Heart Rate, EEG, SPO2) */
void bobgui_bio_start_sensor_stream (BobguiBioManager *self, const char *sensor_id);
void bobgui_bio_on_vital_sign_changed (BobguiBioManager *self, GCallback callback, gpointer user_data);

/* Medical Imaging (DICOM/MRI hardware-accelerated viewing) */
#define BOBGUI_TYPE_BIO_VIEWER (bobgui_bio_viewer_get_type ())
G_DECLARE_FINAL_TYPE (BobguiBioViewer, bobgui_bio_viewer, BOBGUI, BIO_VIEWER, BobguiWidget)

BobguiBioViewer * bobgui_bio_viewer_new (void);
void              bobgui_bio_viewer_load_dicom (BobguiBioViewer *self, const char *path);

/* Adaptive UI (UI changes based on user stress/heart-rate - Exclusive Feature) */
void bobgui_bio_enable_biometric_adaptation (BobguiBioManager *self, BobguiWidget *ui_root);

G_END_DECLS

#endif /* BOBGUI_BIO_H */
