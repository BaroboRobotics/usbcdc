#ifndef USBCDC_DEVICES_HPP
#define USBCDC_DEVICES_HPP

#include <algorithm>
#include <iostream>
#include <set>
#include <string>

namespace usbcdc {

class Device {
public:
    Device () = default;
    Device (const std::string& path, const std::string& productString)
        : mPath(path), mProductString(productString)
    {}
    void path (const std::string& p) { mPath = p; }
    void productString (const std::string& ps) { mProductString = ps; }
    const std::string& path () const { return mPath; }
    const std::string& productString () const { return mProductString; }
private:
    std::string mPath;
    std::string mProductString;
};

std::ostream& operator<< (std::ostream& os, const Device& d);
bool operator< (const Device& a, const Device& b);

using DeviceSet = std::set<Device>;
DeviceSet devices ();

struct DeviceSetDifferences {
    DeviceSet added;
    DeviceSet removed;
};

DeviceSetDifferences deviceSetDifferences(const DeviceSet& a, const DeviceSet& b);
// Returns the two sets `added` (devices present in `b` but not `a`), and `removed (devices present
// in `a` but not `b`).

} // namespace usbcdc

#endif
