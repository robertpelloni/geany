#ifndef BOBGUI_CPP_FILE_SYSTEM_HPP
#define BOBGUI_CPP_FILE_SYSTEM_HPP

#include <gio/gio.h>
#include "object_handle.hpp"
#include "signal.hpp"
#include <string>

namespace bobgui {
namespace cpp {

/**
 * FileSystemWatcher: Monitor files and directories for changes.
 * Parity: QFileSystemWatcher (Qt6).
 */
class FileSystemWatcher
{
public:
  FileSystemWatcher() = default;

  /**
   * Add a path to monitor.
   */
  void add_path(const std::string& path)
  {
    GFile* file = g_file_new_for_path(path.c_str());
    GFileMonitor* monitor = g_file_monitor(file, G_FILE_MONITOR_NONE, nullptr, nullptr);
    
    g_signal_connect(monitor, "changed", G_CALLBACK(changed_trampoline), this);
    
    monitors_.push_back(ObjectHandle<GFileMonitor>(monitor));
    g_object_unref(file);
  }

  /**
   * Signal emitted when a file changes.
   */
  Signal<std::string, GFileMonitorEvent> on_changed;

private:
  static void changed_trampoline(GFileMonitor* monitor, 
                                 GFile* file, 
                                 GFile* other_file, 
                                 GFileMonitorEvent event_type, 
                                 gpointer user_data)
  {
    FileSystemWatcher* self = static_cast<FileSystemWatcher*>(user_data);
    g_autofree char* path = g_file_get_path(file);
    if (path)
      self->on_changed(std::string(path), event_type);
  }

  std::vector<ObjectHandle<GFileMonitor>> monitors_;
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
