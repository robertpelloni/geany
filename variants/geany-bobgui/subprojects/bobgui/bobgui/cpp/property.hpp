#ifndef BOBGUI_CPP_PROPERTY_HPP
#define BOBGUI_CPP_PROPERTY_HPP

#include "signal.hpp"

namespace bobgui {
namespace cpp {

/**
 * Property: A value wrapper with automatic change notification.
 * Parity: QProperty (Qt6), Observable (U++).
 */
template <typename T>
class Property
{
public:
  Property() = default;
  explicit Property(const T& value) : value_(value) {}

  /**
   * Set the value. If the value has changed, the on_changed signal is emitted.
   */
  void set(const T& value)
  {
    if (value_ != value)
      {
        value_ = value;
        on_changed(value_);
      }
  }

  /**
   * Get the current value.
   */
  const T& get() const
  {
    return value_;
  }

  /**
   * Implicit conversion to T for easy reading.
   */
  operator const T&() const
  {
    return get();
  }

  /**
   * Assignment operator for easy writing.
   */
  Property& operator=(const T& value)
  {
    set(value);
    return *this;
  }

  /**
   * Signal emitted whenever the value changes.
   */
  Signal<const T&> on_changed;

private:
  T value_;
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
