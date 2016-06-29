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

namespace usbcdc {

std::ostream& operator<< (std::ostream& os, const Device& d) {
    return os << '\'' << d.productString() << "'@" << d.path();
}

bool operator< (const Device& a, const Device& b) {
    return std::lexicographical_compare(a.path().begin(), a.path().end(),
        b.path().begin(), b.path().end());
}

} // usbcdc
