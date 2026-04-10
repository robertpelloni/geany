/* bobgui/modules/web/bobguiweb.h */
#ifndef BOBGUI_WEB_H
#define BOBGUI_WEB_H

#include <bobgui/bobgui.h>

G_BEGIN_DECLS

/* Embedded Web Engine (Better than QtWebEngine / JavaFX WebView) */
#define BOBGUI_TYPE_WEB_VIEW (bobgui_web_view_get_type ())
G_DECLARE_FINAL_TYPE (BobguiWebView, bobgui_web_view, BOBGUI, WEB_VIEW, BobguiWidget)

BobguiWebView * bobgui_web_view_new (void);

/* Navigation and Control API */
void bobgui_web_view_load_uri (BobguiWebView *self, const char *uri);
void bobgui_web_view_load_html (BobguiWebView *self, const char *html, const char *base_uri);

/* Direct GObject/JS Bridge (Superior Parity with JavaFX) */
void bobgui_web_view_expose_object (BobguiWebView *self, 
                                   GObject *object, 
                                   const char *name);

/* Script Execution (Better than standard async eval) */
void bobgui_web_view_run_javascript (BobguiWebView *self, 
                                    const char *script, 
                                    GAsyncReadyCallback callback, 
                                    gpointer user_data);

G_END_DECLS

#endif /* BOBGUI_WEB_H */
