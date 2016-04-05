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
			// Not needed on Windows
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
			wchar_t* utf16Path = nullptr;
			if (SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &utf16Path) != S_OK)
				return {};

			char utf8Path[4 * MAX_PATH];
			int len = WideCharToMultiByte(CP_UTF8, 0, utf16Path, lstrlenW(utf16Path), utf8Path, sizeof(utf8Path), nullptr, nullptr);
			CoTaskMemFree(utf16Path);
			return {utf8Path, len};
		#else
			static_assert(false, "Unsupported OS");
		#endif
		return {};
	}
}
}
