#ifndef BOBGUI_CPP_APPLICATION_HPP
#define BOBGUI_CPP_APPLICATION_HPP

#include <bobgui/bobgui.h>

#include "object_handle.hpp"

#include <functional>
#include <utility>

namespace bobgui {
namespace cpp {

class Application
{
public:
  typedef std::function<void(Application &)> LifecycleHandler;
  typedef std::function<void(Application &, GFile **, int, const char *)> OpenHandler;
  typedef std::function<int(Application &, GApplicationCommandLine *)> CommandLineHandler;

  explicit Application (const char *application_id, GApplicationFlags flags = G_APPLICATION_DEFAULT_FLAGS)
  : app_ (BOBGUI_APPLICATION (g_object_new (BOBGUI_TYPE_APPLICATION,
                                            "application-id", application_id,
                                            "flags", flags,
                                            NULL)))
  {
    g_signal_connect (app_.get (),
                      "startup",
                      G_CALLBACK (&Application::startup_trampoline),
                      this);

    g_signal_connect (app_.get (),
                      "activate",
                      G_CALLBACK (&Application::activate_trampoline),
                      this);

    g_signal_connect (app_.get (),
                      "shutdown",
                      G_CALLBACK (&Application::shutdown_trampoline),
                      this);

    g_signal_connect (app_.get (),
                      "open",
                      G_CALLBACK (&Application::open_trampoline),
                      this);

    g_signal_connect (app_.get (),
                      "command-line",
                      G_CALLBACK (&Application::command_line_trampoline),
                      this);
  }

  BobguiApplication *native () const
  {
    return app_.get ();
  }

  void on_startup (LifecycleHandler handler)
  {
    on_startup_ = std::move (handler);
  }

  void on_activate (LifecycleHandler handler)
  {
    on_activate_ = std::move (handler);
  }

  void on_shutdown (LifecycleHandler handler)
  {
    on_shutdown_ = std::move (handler);
  }

  void on_open (OpenHandler handler)
  {
    on_open_ = std::move (handler);
  }

  void on_command_line (CommandLineHandler handler)
  {
    on_command_line_ = std::move (handler);
  }

  int run (int argc, char **argv)
  {
    return g_application_run (G_APPLICATION (app_.get ()), argc, argv);
  }

private:
  static void startup_trampoline (BobguiApplication *, gpointer user_data)
  {
    Application *self = static_cast<Application *> (user_data);

    if (self->on_startup_)
      self->on_startup_ (*self);
  }

  static void activate_trampoline (BobguiApplication *, gpointer user_data)
  {
    Application *self = static_cast<Application *> (user_data);

    if (self->on_activate_)
      self->on_activate_ (*self);
  }

  static void shutdown_trampoline (BobguiApplication *, gpointer user_data)
  {
    Application *self = static_cast<Application *> (user_data);

    if (self->on_shutdown_)
      self->on_shutdown_ (*self);
  }

  static void open_trampoline (BobguiApplication *, gpointer files, gint n_files, const gchar *hint, gpointer user_data)
  {
    Application *self = static_cast<Application *> (user_data);

    if (self->on_open_)
      self->on_open_ (*self, static_cast<GFile **> (files), n_files, hint);
  }

  static int command_line_trampoline (BobguiApplication *, GApplicationCommandLine *cmdline, gpointer user_data)
  {
    Application *self = static_cast<Application *> (user_data);

    if (self->on_command_line_)
      return self->on_command_line_ (*self, cmdline);

    return 0;
  }

  ObjectHandle<BobguiApplication> app_;
  LifecycleHandler on_startup_;
  LifecycleHandler on_activate_;
  LifecycleHandler on_shutdown_;
  OpenHandler on_open_;
  CommandLineHandler on_command_line_;
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
