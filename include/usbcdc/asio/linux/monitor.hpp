#ifndef USBCDC_ASIO_LINUX_MONITOR_HPP
#define USBCDC_ASIO_LINUX_MONITOR_HPP

#include <util/asio/asynccompletion.hpp>
#include <util/asio/operation.hpp>
#include <util/asio/transparentservice.hpp>

#include <boost/process.hpp>
#include <boost/process/mitigate.hpp>

#include <boost/iostreams/device/file_descriptor.hpp>

#include <boost/asio/completion_condition.hpp>
#include <boost/asio/buffer.hpp>

#include <usbcdc/devices.hpp>

#include <boost/asio/yield.hpp>

#include <memory>
#include <vector>

namespace usbcdc { namespace asio {

class MonitorImpl {
public:
    explicit MonitorImpl (boost::asio::io_service& context);

    void close (boost::system::error_code& ec);

    template <class CompletionToken>
    auto asyncDevices (CompletionToken&& token);

    template <class CompletionToken>
    auto asyncReceiveDeviceEvent (CompletionToken&& token);

private:
    boost::asio::io_service& mContext;

    boost::process::pipe mPipe;
    boost::process::pipe_end mChildStdout;
    boost::process::child mChildProcess;
};

template <class Args>
static inline auto executeProcess (boost::asio::io_service& context,
        decltype(boost::process::pipe::sink) fd, Args&& args) {
    using namespace boost::process::initializers;
    boost::iostreams::file_descriptor_sink sink { fd, boost::iostreams::close_handle };
    return boost::process::execute(
        set_args(std::forward<Args>(args)),
        bind_stdout(sink),
        close_stdin(),
        close_stderr(),
        throw_on_error(),
        notify_io_service(context)
    );
}

static inline auto executeUdevadmMonitor (boost::asio::io_service& context,
        decltype(boost::process::pipe::sink) fd) {
    return executeProcess(context, fd, std::vector<std::string>{
        boost::process::search_path("stdbuf"),
        "-oL",
        // When run in a terminal, `udevadm` has line-buffered output. That is, as soon
        // as `udevadm` emits a newline, its output buffer is flushed. When run in a pipeline, // `udevadm` has block-buffered output, so its output buffer is flushed much less
        // frequently. Since `udevadm` demarcates its event records with newlines, we must
        // force its stdout pipe to be line-buffered to have any hope of receiving events in
        // real-time. `stdout -oL <cmd...>` is an easy way of doing this.
        boost::process::search_path("udevadm"),
        "monitor",
        "--udev",  // Only receive post-processed udev events
        "--property",  // Dump some metadata that includes the encoded product string
        "--subsystem-match=tty"
    });
}

static inline auto executeUdevadmInfo (boost::asio::io_service& context,
        decltype(boost::process::pipe::sink) fd) {
    return executeProcess(context, fd, std::vector<std::string>{
        boost::process::search_path("sh"),
        "-c",
        "find /sys/class/tty -print0 | xargs -0 -L1 udevadm info | cut -d' ' -f2"
    });
}

inline MonitorImpl::MonitorImpl (boost::asio::io_service& context)
    : mContext(context)
    , mPipe(boost::process::create_pipe())
    , mChildStdout(mContext, mPipe.source)
    , mChildProcess(executeUdevadmMonitor(context, mPipe.sink))
{}

inline void MonitorImpl::close (boost::system::error_code& ec) {
    boost::process::terminate(mChildProcess);
    // Calling `terminate()` is not recommended by Boost.Process docs, but they also document that
    // it is implemented on Linux using SIGTERM, which is what we would probably do anyway.

    mChildStdout.close(ec);
}

bool parseUdevadm (boost::asio::streambuf& buf, size_t n, Device& event);
// Parse the output of `udevadm info`, returning false on parse failure.

bool parseUdevadm (boost::asio::streambuf& buf, size_t n, DeviceEvent& event);
// Parse the output of `udevadm monitor --property`, returning false on parse failure.

template <class CompletionToken>
inline auto MonitorImpl::asyncDevices (CompletionToken&& token) {
    auto p = boost::process::create_pipe();

    auto coroutine =
    [ this
    , childStdout = boost::process::pipe_end(mContext, p.source)
    , childProcess = executeUdevadmInfo(mContext, p.sink)
    , buf = std::make_unique<boost::asio::streambuf>()
    , device = Device{}
    , devices = DeviceSet{}
    ](auto&& op, boost::system::error_code ec = {}, size_t n = 0) mutable {
        reenter (op) {
            yield boost::asio::async_read_until(childStdout, *buf, "\n\n", std::move(op));
            while (n && !ec) {
                if (parseUdevadm(*buf, n, device)) {
                    devices.insert(device);
                }
                buf->consume(n);
                yield boost::asio::async_read_until(childStdout, *buf, "\n\n", std::move(op));
            }

            if (!n) { ec = {}; }  // 0-length read means we got an EOF, which means we're done
            op.complete(ec, devices);
        }
    };

    return util::asio::asyncDispatch(
        mContext,
        std::make_tuple(make_error_code(boost::asio::error::operation_aborted), DeviceSet{}),
        std::move(coroutine),
        std::forward<CompletionToken>(token)
    );
}
template <class CompletionToken>
inline auto MonitorImpl::asyncReceiveDeviceEvent (CompletionToken&& token) {
    auto coroutine =
    [ this
    , buf = std::make_unique<boost::asio::streambuf>()
    , event = DeviceEvent{}
    , paragraph = std::string{}
    ](auto&& op, boost::system::error_code ec = {}, size_t n = 0) mutable {
        reenter (op) {
            do {
                buf->consume(n);
                yield boost::asio::async_read_until(mChildStdout, *buf, "\n\n", std::move(op));
            } while (!ec && !parseUdevadm(*buf, n, event));
            op.complete(ec, event);
        }
    };

    return util::asio::asyncDispatch(
        mContext,
        std::make_tuple(make_error_code(boost::asio::error::operation_aborted), DeviceEvent{}),
        std::move(coroutine),
        std::forward<CompletionToken>(token)
    );
}

class Monitor : public util::asio::TransparentIoObject<MonitorImpl> {
public:
    explicit Monitor (boost::asio::io_service& context)
        : util::asio::TransparentIoObject<MonitorImpl>(context)
    {}

    UTIL_ASIO_DECL_ASYNC_METHOD(asyncDevices)
    UTIL_ASIO_DECL_ASYNC_METHOD(asyncReceiveDeviceEvent)
};

}} // usbcdc::asio

#include <boost/asio/unyield.hpp>

#endif
