#pragma once

#include <string>

#if __linux
	#define UTILS_OS_LINUX 1
#else
	#define UTILS_OS_LINUX 0
#endif

#if __APPLE__
	#define UTILS_OS_MAC 1
#else
	#define UTILS_OS_MAC 0
#endif

#if UTILS_OS_LINUX || UTILS_OS_MAC
	#define UTILS_OS_POSIX 1
#else
	#define UTILS_OS_POSIX 0
#endif

#if _WIN32
	#define UTILS_OS_WINDOWS 1
#else
	#define UTILS_OS_WINDOWS 0
#endif

namespace dev
{
namespace path
{
	std::string user_home_directory();
	std::string user_cache_directory();
}
}
