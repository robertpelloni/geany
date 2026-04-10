/* bobgui/modules/core/fin/bobguifin.h */
#ifndef BOBGUI_FIN_H
#define BOBGUI_FIN_H

#include <glib-object.h>
#include <bobgui/bobgui.h>

G_BEGIN_DECLS

/* Financial Market Engine (Better than generic Charting toolkits) */
#define BOBGUI_TYPE_FIN_ENGINE (bobgui_fin_engine_get_type ())
G_DECLARE_FINAL_TYPE (BobguiFinEngine, bobgui_fin_engine, BOBGUI, FIN_ENGINE, GObject)

BobguiFinEngine * bobgui_fin_engine_new (const char *provider_url);

/* Real-time Streaming (Stocks, Crypto, Forex) */
void bobgui_fin_subscribe_ticker (BobguiFinEngine *self, const char *symbol);

/* Technical Analysis Indicators (SMA, RSI, MACD - Built-in) */
void bobgui_fin_calculate_indicator (BobguiFinEngine *self, 
                                    const char *symbol, 
                                    const char *indicator_type, 
                                    GAsyncReadyCallback callback);

/* Financial Chart Widget (Candlestick, Order Book, Depth) */
#define BOBGUI_TYPE_FIN_CHART (bobgui_fin_chart_get_type ())
G_DECLARE_FINAL_TYPE (BobguiFinChart, bobgui_fin_chart, BOBGUI, FIN_CHART, BobguiWidget)

BobguiFinChart * bobgui_fin_chart_new (void);
void             bobgui_fin_chart_bind_engine (BobguiFinChart *self, BobguiFinEngine *engine);

G_END_DECLS

#endif /* BOBGUI_FIN_H */
