#ifndef BOBGUI_CPP_SIGNAL_HPP
#define BOBGUI_CPP_SIGNAL_HPP

#include <functional>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>

namespace bobgui {
namespace cpp {

/**
 * Signal: A type-safe multi-cast event system.
 * Parity: QSignal (Qt6), juce::ListenerList / std::function based events.
 */
template <typename... Args>
class Signal
{
public:
  typedef std::function<void(Args...)> Slot;
  typedef size_t ConnectionId;

  Signal() : next_id_(0) {}

  /**
   * Connect a function or lambda to this signal.
   * Returns a ConnectionId that can be used to disconnect.
   */
  ConnectionId connect(Slot slot)
  {
    ConnectionId id = ++next_id_;
    slots_[id] = std::move(slot);
    return id;
  }

  /**
   * Disconnect a slot using its ID.
   */
  void disconnect(ConnectionId id)
  {
    slots_.erase(id);
  }

  /**
   * Emit the signal, calling all connected slots.
   */
  void emit(Args... args)
  {
    for (auto const& [id, slot] : slots_)
      {
        if (slot)
          slot(args...);
      }
  }

  /**
   * Syntactic sugar for emit.
   */
  void operator()(Args... args)
  {
    emit(args...);
  }

private:
  std::map<ConnectionId, Slot> slots_;
  ConnectionId next_id_;
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
