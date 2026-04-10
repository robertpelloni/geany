#ifndef BOBGUI_CPP_SETTINGS_HPP
#define BOBGUI_CPP_SETTINGS_HPP

#include <glib.h>
#include <string>
#include <memory>

namespace bobgui {
namespace cpp {

/**
 * Settings: A high-level C++ wrapper for persistent application state.
 * Parity: QSettings (Qt6), PropertiesFile (JUCE).
 * Backend: GKeyFile.
 */
class Settings
{
public:
  explicit Settings (const std::string &group_name = "General")
  : group_ (group_name),
    file_path_ (g_build_filename (g_get_user_config_dir (), "bobgui", "settings.ini", NULL)),
    key_file_ (g_key_file_new ())
  {
    g_mkdir_with_parents (g_path_get_dirname (file_path_), 0755);
    g_key_file_load_from_file (key_file_, file_path_, G_KEY_FILE_NONE, NULL);
  }

  ~Settings ()
  {
    save ();
    g_key_file_free (key_file_);
    g_free (file_path_);
  }

  void set_string (const std::string &key, const std::string &value)
  {
    g_key_file_set_string (key_file_, group_.c_str (), key.c_str (), value.c_str ());
  }

  std::string get_string (const std::string &key, const std::string &default_value = "") const
  {
    g_autofree char *val = g_key_file_get_string (key_file_, group_.c_str (), key.c_str (), NULL);
    return val ? std::string (val) : default_value;
  }

  void set_int (const std::string &key, int value)
  {
    g_key_file_set_integer (key_file_, group_.c_str (), key.c_str (), value);
  }

  int get_int (const std::string &key, int default_value = 0) const
  {
    GError *error = NULL;
    int val = g_key_file_get_integer (key_file_, group_.c_str (), key.c_str (), &error);
    if (error)
      {
        g_error_free (error);
        return default_value;
      }
    return val;
  }

  void set_bool (const std::string &key, bool value)
  {
    g_key_file_set_boolean (key_file_, group_.c_str (), key.c_str (), value);
  }

  bool get_bool (const std::string &key, bool default_value = false) const
  {
    GError *error = NULL;
    bool val = g_key_file_get_boolean (key_file_, group_.c_str (), key.c_str (), &error);
    if (error)
      {
        g_error_free (error);
        return default_value;
      }
    return val;
  }

  void save ()
  {
    g_autofree char *data = g_key_file_to_data (key_file_, NULL, NULL);
    if (data)
      g_file_set_contents (file_path_, data, -1, NULL);
  }

private:
  std::string group_;
  char *file_path_;
  GKeyFile *key_file_;
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
