#ifndef BOBGUI_CPP_COMMAND_PALETTE_HPP
#define BOBGUI_CPP_COMMAND_PALETTE_HPP

#include <bobgui/modules/visual/commandpalette/bobguicommandpalette.h>

#include "application.hpp"
#include "object_handle.hpp"

namespace bobgui {
namespace cpp {

class CommandPalette
{
public:
  explicit CommandPalette (Application &application)
  : palette_ (bobgui_command_palette_new (application.native ()))
  {
  }

  BobguiCommandPalette *native () const
  {
    return palette_.get ();
  }

  void set_pinned (const char *command_id,
                   bool        pinned)
  {
    bobgui_command_palette_set_pinned (palette_.get (), command_id, pinned);
  }

  void clear_history ()
  {
    bobgui_command_palette_clear_history (palette_.get ());
  }

private:
  ObjectHandle<BobguiCommandPalette> palette_;
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
