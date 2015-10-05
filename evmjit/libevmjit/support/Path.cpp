#include "Path.h"
#include <cstdlib>

#if UTILS_OS_WINDOWS
#include <ShlObj.h>
#endif

namespace dev
{
namespace path
{
	std::string user_home_directory()
	{
		#if UTILS_OS_POSIX
			return std::getenv("HOME");
		#elif UTILS_OS_WINDOWS
			char pathBuf[MAX_PATH];
			if (SHGetSpecialFolderPathA(nullptr, pathBuf, CSIDL_LOCAL_APPDATA, true))
				return pathBuf;
		#else
			static_assert(false, "Unsupported OS");
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
		#elif UTILS_OS_WINDOWS
			char pathBuf[MAX_PATH];
			if (SHGetSpecialFolderPathA(nullptr, pathBuf, CSIDL_LOCAL_APPDATA, true))
				return pathBuf;
		#else
			static_assert(false, "Unsupported OS");
		#endif
		return {};
	}
}
}
