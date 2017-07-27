#ifndef USBCDC_GENERIC_MONITOR_HPP
#define USBCDC_GENERIC_MONITOR_HPP

#include <usbcdc/devices.hpp>

#include <util/log.hpp>
#include <util/producerconsumerqueue.hpp>

#include <composed/op.hpp>

#include <beast/core/handler_alloc.hpp>

#include <boost/asio/steady_timer.hpp>

#include <exception>
#include <future>
#include <memory>

#include <boost/asio/yield.hpp>

namespace usbcdc {

class Monitor {
public:
    explicit Monitor(boost::asio::io_service& c)
        : timer(c)
    {}

    ~Monitor() {
        boost::system::error_code ec;
        close(ec);
    }

    boost::asio::io_service& get_io_service();

    void close(boost::system::error_code& ec);

private:
    template <class Handler = void(boost::system::error_code, DeviceSet)>
    struct DevicesOp;

    template <class Handler = void(boost::system::error_code, DeviceEvent)>
    struct ReceiveDeviceEventOp;

public:
    template <class Token>
    auto asyncDevices(Token&& token) {
        // Get the initial set of devices present in the system. Should only be called once, before
        // any calls to `asyncReceiveDeviceEvent()`.
        return composed::operation<DevicesOp<>>{}(*this, std::forward<Token>(token));
    }

    template <class Token>
    auto asyncReceiveDeviceEvent(Token&& token) {
        // Wait for the monitor to detect the arrival or removal of a device in the system. Should
        // only be called after `asyncDevices()`.
        return composed::operation<ReceiveDeviceEventOp<>>{}(*this, std::forward<Token>(token));
    }

private:
    DeviceSet lastDevices;
    std::chrono::time_point<std::chrono::steady_clock> lastDevicesTime;
    boost::asio::steady_timer timer;
    std::queue<DeviceEvent> eventQueue;

    static constexpr const std::chrono::milliseconds kPollInterval{500};
};

inline boost::asio::io_service& Monitor::get_io_service() { return timer.get_io_service(); }
inline void Monitor::close(boost::system::error_code& ec) {
    timer.expires_at(decltype(timer)::clock_type::time_point::max(), ec);
}

// =======================================================================================
// Devices operation

template <class Handler>
struct Monitor::DevicesOp: boost::asio::coroutine {
    using handler_type = Handler;
    using allocator_type = beast::handler_alloc<char, handler_type>;

    Monitor& self;

    composed::associated_logger_t<handler_type> lg;
    boost::system::error_code ec;

    DevicesOp(handler_type& h, Monitor& m)
        : self(m)
        , lg(composed::get_associated_logger(h))
    {}

    void operator()(composed::op<DevicesOp>&);
};

template <class Handler>
void Monitor::DevicesOp<Handler>::operator()(composed::op<DevicesOp>& op) {
    if (!ec) reenter(this) {
        yield return self.timer.get_io_service().post(op());

        try {
            self.lastDevices = devices();
            self.lastDevicesTime = std::chrono::steady_clock::now();
            // FIXME FIXME FIXME :(
            // We should really generate events and put them on the event queue here. The way it's
            // written, you must call `asyncDevices()` before `asyncReceiveDeviceEvent()`.
        }
        catch (const boost::system::system_error& e) {
            BOOST_LOG(lg) << "devices() threw: " << e.what();
            ec = e.code();
        }
        catch (const std::exception& e) {
            BOOST_LOG(lg) << "devices() threw: " << e.what();
            ec = boost::asio::error::network_down;
        }
    }
    op.complete(ec, self.lastDevices);
};

// =======================================================================================
// ReceiveDeviceEvent operation

template <class Handler>
struct Monitor::ReceiveDeviceEventOp: boost::asio::coroutine {
    using handler_type = Handler;
    using allocator_type = beast::handler_alloc<char, handler_type>;

    Monitor& self;

    DeviceEvent event;
    // With some refactoring we could avoid storing this, using self.eventQueue.front() as the op's
    // result, requiring the op to pop the queue on entry. Sounds annoying.

    composed::associated_logger_t<handler_type> lg;
    boost::system::error_code ec;

    ReceiveDeviceEventOp(handler_type& h, Monitor& m)
        : self(m)
        , lg(composed::get_associated_logger(h))
    {}

    void operator()(composed::op<ReceiveDeviceEventOp>&);
};

template <class Handler>
void Monitor::ReceiveDeviceEventOp<Handler>::operator()(composed::op<ReceiveDeviceEventOp>& op) {
    if (!ec) reenter(this) {
        while (true) {
            if (self.eventQueue.size()) {
                event = self.eventQueue.front();
                self.eventQueue.pop();
                yield return self.timer.get_io_service().post(op());
                yield break;
            }

            if (self.timer.expires_at() == decltype(self.timer)::clock_type::time_point::min()) {
                ec = boost::asio::error::operation_aborted;
                yield return self.timer.get_io_service().post(op());
            }
            self.timer.expires_at(self.lastDevicesTime + self.kPollInterval);
            yield return self.timer.async_wait(op(ec));

            {
                auto newDevices = devices();
                auto diff = deviceSetDifferences(self.lastDevices, newDevices);
                for (auto d: diff.added) {
                    self.eventQueue.push({DeviceEvent::ADD, d});
                }
                for (auto d: diff.removed) {
                    self.eventQueue.push({DeviceEvent::REMOVE, d});
                }

                self.lastDevices = newDevices;
                self.lastDevicesTime = std::chrono::steady_clock::now();
            }
        }
    }
    op.complete(ec, event);
};

} // usbcdc

#include <boost/asio/unyield.hpp>

#endif
