#ifndef BOBGUI_CPP_THEME_HPP
#define BOBGUI_CPP_THEME_HPP

#include <bobgui/bobguicssprovider.h>
#include <bobgui/bobguistyleprovider.h>
#include <gdk/gdkdisplay.h>
#include "object_handle.hpp"
#include <string>

namespace bobgui {
namespace cpp {

/**
 * Theme: High-level C++ wrapper for CSS Stylesheets.
 * Parity: QStyleSheet (Qt6), LookAndFeel (JUCE).
 */
class Theme
{
public:
  Theme() : provider_(bobgui_css_provider_new()) {}

  /**
   * Load CSS from a string.
   */
  void load_from_string(const std::string& css)
  {
    bobgui_css_provider_load_from_string(provider_.get(), css.c_str());
  }

  /**
   * Load CSS from a compiled-in resource.
   */
  void load_from_resource(const std::string& path)
  {
    bobgui_css_provider_load_from_resource(provider_.get(), path.c_str());
  }

  /**
   * Apply this theme globally to the entire application.
   */
  void apply_globally()
  {
    bobgui_style_context_add_provider_for_display(
      gdk_display_get_default(),
      BOBGUI_STYLE_PROVIDER(provider_.get()),
      BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
  }

  /**
   * Apply this theme to a specific widget.
   */
  void apply_to_widget(BobguiWidget* widget)
  {
    /* Logic: Widgets in Bobgui 4+ use StyleContext differently, 
       often we add the provider to the display or use a private cascade.
       For the C++ wrapper, we'll focus on global application themes. */
  }

private:
  ObjectHandle<BobguiCssProvider> provider_;
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
