#ifndef BOBGUI_CPP_RESOURCE_HPP
#define BOBGUI_CPP_RESOURCE_HPP

#include <gio/gio.h>
#include <string>
#include <vector>

namespace bobgui {
namespace cpp {

/**
 * Resource: Access compiled-in application assets.
 * Parity: QResource (Qt6), juce::File::createMemoryInputStream.
 */
class Resource
{
public:
  static std::vector<char> load(const std::string& path)
  {
    GError* error = nullptr;
    GBytes* bytes = g_resources_lookup_data(path.c_str(), G_RESOURCE_LOOKUP_FLAGS_NONE, &error);
    
    if (error)
      {
        g_error_free(error);
        return {};
      }

    gsize size;
    const char* data = (const char*)g_bytes_get_data(bytes, &size);
    std::vector<char> result(data, data + size);
    
    g_bytes_unref(bytes);
    return result;
  }

  static bool exists(const std::string& path)
  {
    return g_resources_get_info(path.c_str(), G_RESOURCE_LOOKUP_FLAGS_NONE, nullptr, nullptr, nullptr);
  }
};

} /* namespace cpp */
} /* namespace bobgui */

#endif
