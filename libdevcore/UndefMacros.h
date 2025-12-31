// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2015-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

/// @file
/// This header should be used to #undef some really evil macros defined by
/// windows.h which result in conflict with our libsolidity/Token.h
#pragma once

#if defined(_MSC_VER) || defined(__MINGW32__)

#undef DELETE
#undef IN
#undef VOID
#undef THIS
#undef CONST

// Conflicting define on MinGW in windows.h
// windows.h(19): #define interface struct
#ifdef interface
#undef interface
#endif

#elif defined(DELETE) || defined(IN) || defined(VOID) || defined(THIS) || defined(CONST) || defined(interface)

#error "The preceding macros in this header file are reserved for V8's "\
"TOKEN_LIST. Please add a platform specific define above to undefine "\
"overlapping macros."

#endif
