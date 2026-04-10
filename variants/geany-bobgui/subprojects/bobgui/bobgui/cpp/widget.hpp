#ifndef BOBGUI_CPP_WIDGET_HPP
#define BOBGUI_CPP_WIDGET_HPP

#include <bobgui/bobgui.h>
#include "object_handle.hpp"
#include "layout.hpp"

namespace bobgui {
namespace cpp {

/**
 * Widget: A high-level C++ wrapper for BobguiWidget.
 * Parity: QWidget (Qt6), Component (JUCE), Ctrl (U++).
 */
class Widget
{
public:
  explicit Widget (BobguiWidget *widget = nullptr)
  : widget_ (widget)
  {
  }

  virtual ~Widget () = default;

  BobguiWidget *native () const { return widget_; }

  void set_visible (bool visible)
  {
    if (widget_)
      bobgui_widget_set_visible (widget_, visible);
  }

  void set_size_request (int width, int height)
  {
    if (widget_)
      bobgui_widget_set_size_request (widget_, width, height);
  }

  /**
   * Set a layout manager for this widget.
   */
  void set_layout (const LayoutManager& layout)
  {
    if (widget_)
      bobgui_container_set_layout_manager (widget_, layout.native ());
  }

  /**
   * Ultimate++ style operator for adding children.
   * If this widget is a container (Box), it appends the child.
   */
  Widget& operator<< (const Widget &child)
  {
    if (widget_ && child.native ())
      {
        if (BOBGUI_IS_BOX (widget_))
          bobgui_box_append (BOBGUI_BOX (widget_), child.native ());
        // We can add more container types here (Grid, etc.)
      }
    return *this;
  }

  Widget& operator<< (BobguiWidget *child)
  {
    if (widget_ && child)
      {
        if (BOBGUI_IS_BOX (widget_))
          bobgui_box_append (BOBGUI_BOX (widget_), child);
      }
    return *this;
  }

protected:
  BobguiWidget *widget_;
};

/**
 * Specialization for Box layout (U++ style).
 */
class Box : public Widget
{
public:
  explicit Box (BobguiOrientation orientation = BOBGUI_ORIENTATION_VERTICAL, int spacing = 0)
  : Widget (bobgui_box_new (orientation, spacing))
  {
  }
};

class Label : public Widget
{
public:
  explicit Label (const char *text)
  : Widget (bobgui_label_new (text))
  {
  }
};

class Button : public Widget
{
public:
  explicit Button (const char *label)
  : Widget (bobgui_button_new_with_label (label))
  {
  }
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
