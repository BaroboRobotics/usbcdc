#include <util/doctest.h>

#include <usbcdc/monitor.hpp>

namespace {

// =======================================================================================
// Test cases

TEST_CASE("can immediately destroy a Monitor") {
    boost::asio::io_service context;
    {
        usbcdc::Monitor m{context};
        // At one time, this hung on Windows due to a race in the monitor thread.
    }
    CHECK(true);
}

TEST_CASE("can get the current list of devices") {
    boost::asio::io_service context;
    usbcdc::Monitor m{context};

    m.asyncDevices([](const boost::system::error_code& ec, const usbcdc::DeviceSet& devices) {
        util::log::Logger lg;
        BOOST_LOG(lg) << "The following devices are plugged in:";
        for (auto& d: devices) {
            BOOST_LOG(lg) << d;
        }
    });

    context.run();
}

TEST_CASE("can respond to events") {
    boost::asio::io_service context;
    usbcdc::Monitor m{context};

    m.asyncDevices([&m](const boost::system::error_code& ec, const usbcdc::DeviceSet& devices) {
        util::log::Logger lg;
        BOOST_LOG(lg) << "Waiting for device event";
        m.asyncReceiveDeviceEvent([](const boost::system::error_code& ec, const usbcdc::DeviceEvent& event) {
            util::log::Logger lg;
            BOOST_LOG(lg) << "DeviceEvent received: " << event;
        });
    });

    context.run();
}

}  // <anonymous>
