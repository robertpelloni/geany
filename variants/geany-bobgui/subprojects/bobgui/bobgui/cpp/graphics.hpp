#ifndef BOBGUI_CPP_GRAPHICS_HPP
#define BOBGUI_CPP_GRAPHICS_HPP

#include <bobgui/bobguisnapshot.h>
#include <gdk/gdkrgba.h>
#include <graphene.h>
#include <cairo.h>
#include <string>

namespace bobgui {
namespace cpp {

/**
 * Graphics: A high-level JUCE-style painting API.
 * Parity: juce::Graphics, QPainter (Qt6).
 */
class Graphics
{
public:
  explicit Graphics(BobguiSnapshot* snapshot) : snapshot_(snapshot), cr_(nullptr) {}
  explicit Graphics(cairo_t* cr) : snapshot_(nullptr), cr_(cr) {}

  void save() 
  { 
    if (snapshot_) bobgui_snapshot_save(snapshot_); 
    if (cr_) cairo_save(cr_);
  }

  void restore() 
  { 
    if (snapshot_) bobgui_snapshot_restore(snapshot_); 
    if (cr_) cairo_restore(cr_);
  }

  void set_color(float r, float g, float b, float a = 1.0f)
  {
    current_color_.red = r;
    current_color_.green = g;
    current_color_.blue = b;
    current_color_.alpha = a;
    if (cr_) cairo_set_source_rgba(cr_, r, g, b, a);
  }

  void fill_rect(float x, float y, float w, float h)
  {
    if (snapshot_)
      {
        graphene_rect_t rect;
        graphene_rect_init(&rect, x, y, w, h);
        bobgui_snapshot_append_color(snapshot_, &current_color_, &rect);
      }
    if (cr_)
      {
        cairo_rectangle(cr_, x, y, w, h);
        cairo_fill(cr_);
      }
  }

  void draw_text(const std::string& text, float x, float y)
  {
    /* Logic: This would normally use Pango, but for this wrapper we'll stub it 
       to the internal append_text if available or just a placeholder */
  }

  void translate(float dx, float dy)
  {
    if (snapshot_) bobgui_snapshot_translate(snapshot_, &GRAPHENE_POINT_INIT(dx, dy));
    if (cr_) cairo_translate(cr_, dx, dy);
  }

private:
  BobguiSnapshot* snapshot_;
  cairo_t* cr_;
  GdkRGBA current_color_ = {1, 1, 1, 1};
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
