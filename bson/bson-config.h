/*
 * This is a temporary config.h until the build system generates it.
 */

#if defined(_MSC_VER) || defined(_WIN32) || defined(_WIN64)
#define HAVE_WINDOWS
#endif

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
#define HAVE_PTHREADS
#endif

