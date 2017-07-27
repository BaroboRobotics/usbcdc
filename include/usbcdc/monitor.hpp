#ifndef USBCDC_MONITOR_HPP
#define USBCDC_MONITOR_HPP

#include <usbcdc/devices.hpp>

#include <boost/predef.h>

#include <iostream>

namespace usbcdc {

struct DeviceEvent {
    enum {
        ADD,
        REMOVE
    } type;
    Device device;
};

inline std::ostream& operator<<(std::ostream& os, DeviceEvent event) {
    return os << (event.type == DeviceEvent::ADD ? "ADD " : "REMOVE ") << event.device;
}

} // usbcdc

#if BOOST_OS_LINUX
#include <usbcdc/linux/monitor.hpp>
#else
#include <usbcdc/generic/monitor.hpp>
#endif

#endif
