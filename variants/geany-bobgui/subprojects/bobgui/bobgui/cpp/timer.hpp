#ifndef BOBGUI_CPP_TIMER_HPP
#define BOBGUI_CPP_TIMER_HPP

#include <glib.h>
#include <functional>
#include <memory>

namespace bobgui {
namespace cpp {

/**
 * Timer: A high-level C++ wrapper for periodic tasks.
 * Parity: QTimer (Qt6), Timer (JUCE/U++).
 */
class Timer
{
public:
  typedef std::function<bool()> Callback;

  Timer ()
  : source_id_ (0)
  {
  }

  ~Timer ()
  {
    stop ();
  }

  /**
   * Start the timer with a given interval in milliseconds.
   * If the callback returns true, the timer repeats.
   */
  void start (unsigned int interval_ms, Callback callback)
  {
    stop ();
    callback_ = std::move (callback);
    source_id_ = g_timeout_add (interval_ms, (GSourceFunc) timer_trampoline, this);
  }

  /**
   * Run the timer once after a delay.
   */
  void single_shot (unsigned int delay_ms, std::function<void()> callback)
  {
    stop ();
    auto wrapper = [callback] () -> bool {
      callback ();
      return false;
    };
    start (delay_ms, wrapper);
  }

  void stop ()
  {
    if (source_id_ > 0)
      {
        g_source_remove (source_id_);
        source_id_ = 0;
      }
  }

  bool is_running () const
  {
    return source_id_ > 0;
  }

private:
  static gboolean timer_trampoline (gpointer user_data)
  {
    Timer *self = static_cast<Timer *> (user_data);
    if (self->callback_)
      {
        if (self->callback_ ())
          return G_SOURCE_CONTINUE;
      }
    
    self->source_id_ = 0;
    return G_SOURCE_REMOVE;
  }

  guint source_id_;
  Callback callback_;
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
