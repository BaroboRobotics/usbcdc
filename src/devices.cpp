#include <usbcdc/devices.hpp>

#include <algorithm>
#include <iterator>

namespace usbcdc {

DeviceSetDifferences deviceSetDifferences(const DeviceSet& a, const DeviceSet& b) {
    auto devicesRemoved = decltype(b){};
    std::set_difference(a.cbegin(), a.cend(),
        b.cbegin(), b.cend(),
        std::inserter(devicesRemoved, devicesRemoved.end()));
    // Compute `a - b`, devices which are in `a` but not `b`.

    auto devicesAdded = decltype(b){};
    std::set_difference(b.cbegin(), b.cend(),
        a.cbegin(), a.cend(),
        std::inserter(devicesAdded, devicesAdded.end()));
    // Compute `b - a`, devices which are in `b` but not `a`.

    return {std::move(devicesAdded), std::move(devicesRemoved)};
}

}  // usbcdc
