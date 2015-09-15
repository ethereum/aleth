#include "Path.h"
#include <cstdlib>

namespace dev
{
namespace path
{
  std::string user_home_directory()
  {
    #if UTILS_OS_POSIX
      return std::getenv("HOME");
    #endif
    return {};
  }

  std::string user_cache_directory()
  {
    #if UTILS_OS_LINUX
      auto xds_cache_home = std::getenv("XDS_CACHE_HOME");
      if (xds_cache_home && *xds_cache_home != '\0')
        return xds_cache_home;

      auto home = user_home_directory();
      if (!home.empty())
        return home + "/.cache";
    #elif UTILS_OS_MAC
      auto home = user_home_directory();
      if (!home.empty())
        return home + "/Library/Caches";
    #endif

    return {};
  }
}
}
