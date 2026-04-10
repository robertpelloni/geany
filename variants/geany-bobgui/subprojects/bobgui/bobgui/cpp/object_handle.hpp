#ifndef BOBGUI_CPP_OBJECT_HANDLE_HPP
#define BOBGUI_CPP_OBJECT_HANDLE_HPP

#include <glib-object.h>

namespace bobgui {
namespace cpp {

template <typename T>
class ObjectHandle
{
public:
  explicit ObjectHandle (T *object = nullptr)
  : object_ (object)
  {
  }

  ~ObjectHandle ()
  {
    if (object_ != nullptr)
      g_object_unref (object_);
  }

  ObjectHandle (const ObjectHandle &) = delete;
  ObjectHandle &operator= (const ObjectHandle &) = delete;

  ObjectHandle (ObjectHandle &&other) noexcept
  : object_ (other.object_)
  {
    other.object_ = nullptr;
  }

  ObjectHandle &operator= (ObjectHandle &&other) noexcept
  {
    if (this != &other)
      {
        if (object_ != nullptr)
          g_object_unref (object_);
        object_ = other.object_;
        other.object_ = nullptr;
      }

    return *this;
  }

  T *get () const
  {
    return object_;
  }

  operator T *() const
  {
    return object_;
  }

private:
  T *object_;
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
