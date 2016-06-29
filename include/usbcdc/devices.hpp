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

} // namespace usbcdc

#endif
