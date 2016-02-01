#ifndef USBCDC_DEVICES_HPP
#define USBCDC_DEVICES_HPP

#include <list>
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

using DeviceList = std::list<Device>;
DeviceList devices ();

} // namespace usbcdc

#endif
