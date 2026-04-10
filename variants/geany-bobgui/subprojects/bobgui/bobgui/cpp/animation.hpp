#ifndef BOBGUI_CPP_ANIMATION_HPP
#define BOBGUI_CPP_ANIMATION_HPP

#include "property.hpp"
#include "timer.hpp"
#include <cmath>

namespace bobgui {
namespace cpp {

/**
 * PropertyAnimation: Animates a Property<float> over time.
 * Parity: QPropertyAnimation (Qt6).
 */
class PropertyAnimation
{
public:
  explicit PropertyAnimation(Property<float>& target)
  : target_(target), start_value_(0), end_value_(0), duration_ms_(1000), elapsed_ms_(0)
  {
  }

  void set_range(float start, float end, unsigned int duration_ms)
  {
    start_value_ = start;
    end_value_ = end;
    duration_ms_ = duration_ms;
  }

  void start()
  {
    elapsed_ms_ = 0;
    timer_.start(16, [this]() {
      elapsed_ms_ += 16;
      float progress = (float)elapsed_ms_ / duration_ms_;
      if (progress >= 1.0f)
        {
          target_ = end_value_;
          return false; // stop
        }
      
      // Linear interpolation (Parity: Linear easing)
      float current = start_value_ + (end_value_ - start_value_) * progress;
      target_ = current;
      return true; // repeat
    });
  }

  void stop() { timer_.stop(); }

private:
  Property<float>& target_;
  float start_value_;
  float end_value_;
  unsigned int duration_ms_;
  unsigned int elapsed_ms_;
  Timer timer_;
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
