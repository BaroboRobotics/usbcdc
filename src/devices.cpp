#include <boost/predef.h>

#if BOOST_OS_MACOS
#include "osx.cpp"
#elif BOOST_OS_WINDOWS
#include "windows.cpp"
#elif BOOST_OS_LINUX
#include "linux.cpp"
#else
#error "usbcdc is unimplemented on this OS"
#endif