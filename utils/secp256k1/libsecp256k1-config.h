/* src/libsecp256k1-config.h.  Generated from libsecp256k1-config.h.in by configure.  */
/* src/libsecp256k1-config.h.in.  Generated from configure.ac by autoheader.  */

#ifndef LIBSECP256K1_CONFIG_H

#define LIBSECP256K1_CONFIG_H

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define this symbol if OpenSSL EC functions are available */
/* #undef ENABLE_OPENSSL_TESTS */

/* Define this symbol if __builtin_expect is available */
#if !defined(_MSC_VER)
#define HAVE_BUILTIN_EXPECT 1
#endif

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if the system has the type `__int128'. */
#if !defined(_MSC_VER) && (defined(__x86_64__) || defined(_M_X64))
#define HAVE___INT128 1
#endif

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "libsecp256k1"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "libsecp256k1"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libsecp256k1 0.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libsecp256k1"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "0.1"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define this symbol to enable x86_64 assembly optimizations */
/* #undef USE_ASM_X86_64 */

/* Define this symbol to use endomorphism optimization */
/* #undef USE_ENDOMORPHISM */

/* Select implementation for num */
#if defined(_MSC_VER)
#define USE_NUM_NONE 1
#else
#define USE_NUM_GMP 1
#endif

/* Select implementation for field */
#if HAVE___INT128
#define USE_FIELD_5X52 1
#else
#define USE_FIELD_10X26 1
#endif

/* Select field inverse implementation */
#if USE_NUM_GMP
#define USE_FIELD_INV_NUM 1
#else
#define USE_FIELD_INV_BUILTIN 1
#endif

/* Select implementation for scalar */
#if HAVE___INT128
#define USE_SCALAR_4X64 1
#else
#define USE_SCALAR_8X32 1
#endif

/* Define this symbol to use the native scalar inverse implementation */
/* Select scalar inverse implementation */
#if USE_NUM_GMP
#define USE_SCALAR_INV_NUM 1
#else
#define USE_SCALAR_INV_BUILTIN 1
#endif

/* Version number of package */
#define VERSION "0.1"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

#endif /*LIBSECP256K1_CONFIG_H*/
