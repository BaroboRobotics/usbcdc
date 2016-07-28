#include <usbcdc/asio/monitor.hpp>

#include <util/log.hpp>

#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/spirit/include/qi.hpp>

#include <boost/fusion/adapted.hpp>

#include <boost/asio/streambuf.hpp>

#include <boost/lexical_cast.hpp>

#include <boost/algorithm/string/find_format.hpp>
#include <boost/algorithm/string/regex_find_format.hpp>
#include <boost/algorithm/hex.hpp>

#include <chrono>
#include <iostream>
#include <map>
#include <string>

namespace usbcdc { namespace asio {

namespace {
namespace qi = boost::spirit::qi;

template <class Iter>
struct UdevadmGrammar : qi::grammar<Iter, std::map<std::string, std::string>()> {
    qi::rule<Iter, std::map<std::string, std::string>()> start;
    qi::rule<Iter> nonProperty;
    qi::rule<Iter, std::pair<std::string, std::string>()> property;
    qi::rule<Iter, std::string()> key;
    qi::rule<Iter, std::string()> value;

    UdevadmGrammar () : UdevadmGrammar::base_type(start, "udevadm") {
        using qi::_1;
        using qi::_val;
        using boost::phoenix::at_c;
        using boost::phoenix::insert;

        start.name("start");
        start = *(property[insert(_val, _1)] | nonProperty)
            >> qi::eol
            > qi::eoi;

        nonProperty.name("nonProperty");
        nonProperty = +(qi::char_ - qi::eol) >> qi::eol;

        property.name("property");
        property %= key >> '=' >> value >> qi::eol;

        key.name("key");
        key %= +(qi::char_ - qi::eol - '=');

        value.name("value");
        value %= +(qi::char_ - qi::eol);

        using ErrorHandlerArgs = boost::fusion::vector<
            Iter&, const Iter&, const Iter&, const qi::info&>;

        auto logError = [](ErrorHandlerArgs args, auto&, qi::error_handler_result&) {
            util::log::Logger lg;
            BOOST_LOG(lg) << "Expected '" << at_c<3>(args) << "' here: '"
                << std::string(at_c<2>(args), at_c<1>(args)) << "'\n";
        };

        qi::on_error<qi::fail>(start, logError);
        qi::on_error<qi::fail>(nonProperty, logError);
        qi::on_error<qi::fail>(property, logError);
        qi::on_error<qi::fail>(key, logError);
        qi::on_error<qi::fail>(value, logError);
    }
};

std::string decodeProductString (std::string input) {
    // udev encodes some characters, such as spaces, to an escaped character sequence of the form
    // `\xhh` where `h` is a hexadecimal digit. Decode all such instances. This is a bit ugly.
    boost::algorithm::find_format_all(
        input,
        boost::algorithm::regex_finder(boost::regex(R"(\\x[0-9a-fA-F]{2})")),
        [](const auto& result) {
            char c;
            assert(std::distance(result.begin(), result.end()) == 4);
            // We know that the regex can only match 4-character substrings. If we interpret only
            // the latter two characters as hexadecimal digits, the decoded integer is guaranteed
            // to fit in our `char c`.
            boost::algorithm::unhex(result.begin() + 2, result.end(), &c);
            return std::string{c};
        }
    );
    return input;
}

bool parseUdevadm (boost::asio::streambuf& buf, size_t n,
        std::map<std::string, std::string>& properties) {
    auto begin = boost::asio::buffers_begin(buf.data());
    auto end = begin + n;
    UdevadmGrammar<decltype(begin)> grammar;
    return qi::parse(begin, end, grammar, properties);
}

} // <anonymous>

bool parseUdevadm (boost::asio::streambuf& buf, size_t n, Device& device) {
    auto properties = std::map<std::string, std::string>{};
    auto success = parseUdevadm(buf, n, properties);

    if (!success || !properties.size() || properties["ID_USB_DRIVER"] != "cdc_acm") {
        return false;
    }

    device.path(properties["DEVNAME"]);
    device.productString(decodeProductString(properties["ID_MODEL_ENC"]));

    return true;
}

bool parseUdevadm (boost::asio::streambuf& buf, size_t n, DeviceEvent& event) {
    auto properties = std::map<std::string, std::string>{};
    auto success = parseUdevadm(buf, n, properties);

    if (!success || !properties.size() || properties["ID_USB_DRIVER"] != "cdc_acm") {
        return false;
    }

    event.device.path(properties["DEVNAME"]);
    event.device.productString(decodeProductString(properties["ID_MODEL_ENC"]));

    if (properties["ACTION"] == "add") {
        event.type = DeviceEvent::ADD;
    }
    else if (properties["ACTION"] == "remove") {
        event.type = DeviceEvent::REMOVE;
    }
    else {
        return false;
    }

    return true;
}

}} // usbcdc::asio
