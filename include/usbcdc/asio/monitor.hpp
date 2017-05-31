#ifndef USBCDC_ASIO_MONITOR_HPP
#define USBCDC_ASIO_MONITOR_HPP

#include <usbcdc/devices.hpp>

#include <boost/predef.h>

namespace usbcdc { namespace asio {

struct DeviceEvent {
    enum {
        ADD,
        REMOVE
    } type;
    Device device;
};

}} // usbcdc::asio

#if BOOST_OS_LINUX
#include <usbcdc/asio/linux/monitor.hpp>
#else
#error "usbcdc::Monitor is not implemented for your platform"
#endif

#endif
