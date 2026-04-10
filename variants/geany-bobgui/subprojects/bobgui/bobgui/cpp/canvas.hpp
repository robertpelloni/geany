#ifndef BOBGUI_CPP_CANVAS_HPP
#define BOBGUI_CPP_CANVAS_HPP

#include <bobgui/bobguidrawingarea.h>
#include "widget.hpp"
#include "graphics.hpp"
#include <functional>

namespace bobgui {
namespace cpp {

/**
 * Canvas: A custom component for 2D painting.
 * Parity: juce::Component (paint method), QWidget (paintEvent).
 */
class Canvas : public Widget
{
public:
  typedef std::function<void(Graphics&)> PaintHandler;

  Canvas()
  : Widget(bobgui_drawing_area_new())
  {
    bobgui_drawing_area_set_draw_func(BOBGUI_DRAWING_AREA(widget_), 
                                      &Canvas::draw_trampoline, 
                                      this, nullptr);
  }

  /**
   * Set a C++ lambda to handle the painting.
   */
  void on_paint(PaintHandler handler)
  {
    on_paint_ = std::move(handler);
  }

  void set_content_size(int w, int h)
  {
    bobgui_drawing_area_set_content_width(BOBGUI_DRAWING_AREA(widget_), w);
    bobgui_drawing_area_set_content_height(BOBGUI_DRAWING_AREA(widget_), h);
  }

private:
  static void draw_trampoline(BobguiDrawingArea* drawing_area,
                              cairo_t*           cr,
                              int                width,
                              int                height,
                              gpointer           user_data)
  {
    Canvas* self = static_cast<Canvas*>(user_data);
    if (self->on_paint_)
      {
        Graphics g(cr);
        self->on_paint_(g);
      }
  }

  PaintHandler on_paint_;
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
