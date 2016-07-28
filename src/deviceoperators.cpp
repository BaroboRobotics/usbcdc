#include <usbcdc/devices.hpp>

namespace usbcdc {

std::ostream& operator<< (std::ostream& os, const Device& d) {
    return os << '\'' << d.productString() << "'@" << d.path();
}

bool operator< (const Device& a, const Device& b) {
    if (a.path() < b.path()) {
        return true;
    }
    if (a.path() == b.path() && a.productString() < b.productString()) {
        return true;
    }
    return false;
}

} // usbcdc
