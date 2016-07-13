// A dumb, generic implementation of a USB-CDC device monitor using usbcdc::devices(). This is not
// efficient. Possible improvements include:
//   - implement the polling logic (devices()) in a service rather than the object implementation
//   - use udevadm on linux to monitor
//   - use libudev on linux to monitor
//   - Use SetupAPI and IORegistry on Windows and Mac to monitor

#ifndef USBCDC_ASIO_MONITOR_HPP
#define USBCDC_ASIO_MONITOR_HPP

#include <usbcdc/devices.hpp>

#include <util/asio/asynccompletion.hpp>
#include <util/asio/transparentservice.hpp>
#include <util/asio/iothread.hpp>

#include <util/producerconsumerqueue.hpp>
#include <util/log.hpp>

#include <algorithm>
#include <atomic>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include <boost/asio/yield.hpp>

namespace usbcdc { namespace asio {

class MonitorImpl : public std::enable_shared_from_this<MonitorImpl> {
public:
    explicit MonitorImpl (boost::asio::io_service& context);
    //void init ();

    void close (boost::system::error_code& ec);

#if 0
    template <class CompletionToken>
    auto asyncReceiveDevice (CompletionToken&& token);
    // Receive one hotplug-add notice in the form of a `usbcdc::Device`. This is not quite as
    // useful as it sounds without hotplug-removal notices in yet.
#endif

    template <class CompletionToken>
    auto asyncDevices (CompletionToken&& token);
    // Fetch the current known device set in the form of `usbcdc::DeviceSet`.

private:
    boost::asio::io_service& mContext;

    util::asio::IoThread mPollingThread;
    //std::atomic<bool> mTerminatePollingThread { false };
    //DeviceSet mCurrentDeviceSet;

    //util::ProducerConsumerQueue<boost::system::error_code, Device> mDeviceQueue;

    //mutable util::log::Logger mLog;
};

inline MonitorImpl::MonitorImpl (boost::asio::io_service& context)
    : mContext(context)
{
}

#if 0
inline void MonitorImpl::init () {
    mCurrentDeviceSet = devices();
    auto self = this->shared_from_this();
    util::asio::asyncDispatch(mPollingThread.context(),
        std::make_tuple(),
        [ this
        , self
        //, newDeviceSet = mCurrentDeviceSet
        //, previousDeviceSet = mCurrentDeviceSet
        ]
        (auto&& op) mutable {
            reenter (op) {
                while (!mTerminatePollingThread) {
#if 0
                    if (newDeviceSet.size()) {
                        mContext.post([this, self, newDeviceSet]() mutable {
                            for (auto&& d : newDeviceSet) {
                                mDeviceQueue.produce(boost::system::error_code(), d);
                            }
                        });
                    }
#endif

                    std::this_thread::sleep_for(std::chrono::milliseconds(500));

                    mCurrentDeviceSet = devices();
#if 0
                    newDeviceSet.clear();

                    std::set_difference(mCurrentDeviceSet.begin(), mCurrentDeviceSet.end(),
                        previousDeviceSet.begin(), previousDeviceSet.end(),
                        std::inserter(newDeviceSet, newDeviceSet.end()));
                    // newDeviceSet = mCurrentDeviceSet - previousDeviceSet

                    previousDeviceSet = mCurrentDeviceSet;
#endif
                }
                op.complete();
            }
        },
        [this, self]() mutable {
            BOOST_LOG(mLog) << "USB-CDC device monitor polling task completed";
#if 0
            mContext.post([this, self]() mutable {
                while (mDeviceQueue.depth() < 0) {
                    mDeviceQueue.produce(boost::asio::error::operation_aborted, Device{});
                }
                while (mDeviceQueue.depth() > 0) {
                    mDeviceQueue.consume([this](boost::system::error_code ec, Device d) {
                        BOOST_LOG(mLog) << "USB-CDC monitor discarding device "
                            << d << ':' << ec.message();
                    });
                }
            });
#endif
        }
    );
}
#endif

inline void MonitorImpl::close (boost::system::error_code& ec) {
    //mTerminatePollingThread = true;
    ec = {};
}

#if 0
template <class CompletionToken>
inline auto MonitorImpl::asyncReceiveDevice (CompletionToken&& token) {
    util::asio::AsyncCompletion<
        CompletionToken, void(boost::system::error_code, Device)
    > init { std::forward<CompletionToken>(token) };

    mDeviceQueue.consume(std::move(init.handler));

    return init.result.get();
}
#endif

template <class CompletionToken>
inline auto MonitorImpl::asyncDevices (CompletionToken&& token) {
    util::asio::AsyncCompletion<
        CompletionToken, void(boost::system::error_code, DeviceSet)
    > init { std::forward<CompletionToken>(token) };

    mPollingThread.context().dispatch([this, handler = std::move(init.handler)]() mutable {
        mContext.dispatch(std::bind(handler, boost::system::error_code(), usbcdc::devices()));
    });

    return init.result.get();
}

class Monitor : public util::asio::TransparentIoObject<MonitorImpl> {
public:
    explicit Monitor (boost::asio::io_service& context)
        : util::asio::TransparentIoObject<MonitorImpl>(context)
    {
        //this->get_implementation()->init();
    }

    //UTIL_ASIO_DECL_ASYNC_METHOD(asyncReceiveDevice)
    UTIL_ASIO_DECL_ASYNC_METHOD(asyncDevices)
};

}} // usbcdc::asio

#include <boost/asio/unyield.hpp>

#endif
