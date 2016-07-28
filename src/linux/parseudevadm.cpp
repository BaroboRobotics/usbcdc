#include <usbcdc/asio/linux/monitor.hpp>

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
    qi::rule<Iter> headline;
    qi::rule<Iter, std::pair<std::string, std::string>()> property;
    qi::rule<Iter, std::string()> key;
    qi::rule<Iter, std::string()> value;

    UdevadmGrammar () : UdevadmGrammar::base_type(start, "udevadm") {
        using qi::_1;
        using qi::_val;
        using boost::phoenix::at_c;
        using boost::phoenix::insert;

        start.name("start");
        start = headline
            >> *(property[insert(_val, _1)])
            >> qi::eol
            > qi::eoi;

        headline.name("headline");
        headline = +(qi::char_ - qi::eol) >> qi::eol;

        property.name("property");
        property %= key >> '=' >> value >> qi::eol;

        key.name("key");
        key %= +(qi::char_ - '=');

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
        qi::on_error<qi::fail>(headline, logError);
        qi::on_error<qi::fail>(property, logError);
        qi::on_error<qi::fail>(key, logError);
        qi::on_error<qi::fail>(value, logError);
    }
};

} // <anonymous>

bool parseUdevadm (boost::asio::streambuf& buf, size_t n, DeviceEvent& event) {
    util::log::Logger lg;

    auto begin = boost::asio::buffers_begin(buf.data());
    auto end = begin + n;

    auto properties = std::map<std::string, std::string>{};
    UdevadmGrammar<decltype(begin)> grammar;
    auto success = qi::parse(begin, end, grammar, properties);

    if (!success || !properties.size() || properties["ID_USB_DRIVER"] != "cdc_acm") {
        return false;
    }

    if (properties["ACTION"] == "add") {
        event.type = DeviceEvent::ADD;
    }
    else if (properties["ACTION"] == "remove") {
        event.type = DeviceEvent::REMOVE;
    }
    else {
        BOOST_LOG(lg) << "Unrecognized ACTION: " << properties["ACTION"];
        return false;
    }

    event.device.path(properties["DEVNAME"]);

    // udev encodes some characters, such as spaces, to an escaped character sequence of the form
    // `\xhh` where `h` is a hexadecimal digit. Decode all such instances. This is a bit ugly.
    auto productString = properties["ID_MODEL_ENC"];
    boost::algorithm::find_format_all(
        productString,
        boost::algorithm::regex_finder(boost::regex(R"(\\x([a-fA-F0-9][a-fA-F0-9]))")),
        [](const auto& result) {
            char c;
            assert(std::distance(result.begin(), result.end()) == 4);
            boost::algorithm::unhex(result.begin() + 2, result.end(), &c);
            return std::string{c};
        }
    );
    event.device.productString(productString);

    return true;
}

}} // usbcdc::asio
