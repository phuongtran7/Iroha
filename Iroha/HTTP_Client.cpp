#include "HTTP_Client.h"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

void Client::make_secrect()
{
	YAML::Node config = YAML::LoadFile("Config.yaml");
	auto key = config["Key"].as<std::string>();
	auto token = config["Token"].as<std::string>();

	// Prepend "?" if secrect is the only thing that need to append to URL else prepend "&"
	secrect_ = fmt::format("key={}&token={}", key, token);
}

Client::Client(boost::asio::executor ex, ssl::context& ctx) :
	resolver_{ ex },
	stream_{ex, ctx}
{
	make_secrect();
	init();
}

Client::~Client()
{
}

void Client::init()
{
	// Set SNI Hostname (many hosts need this to handshake successfully)
	if (!SSL_set_tlsext_host_name(stream_.native_handle(), host_.c_str()))
	{
		beast::error_code ec{ static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
		fmt::print("{}\n", ec.message());
		return;
	}

	// Look up the domain name
	auto const results = resolver_.resolve(host_, std::to_string(port_));

	// Make the connection on the IP address we get from a lookup
	beast::get_lowest_layer(stream_).connect(results);

	// Perform the SSL handshake
	stream_.handshake(ssl::stream_base::client);
}

void Client::make_request(const std::string& target)
{
	// Set up an HTTP GET request message
	req_.version(version_);
	req_.method(http::verb::get);
	req_.target(target);
	req_.set(http::field::host, host_);
	req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
}

void Client::get_board()
{
	http::response<http::string_body> res;

	auto const target = fmt::format("/1/members/me/boards?fields=name&filter=open&{}", secrect_);
	make_request(target);

	// Receive the HTTP response
	http::read(stream_, buffer_, res);

	// Write the message to standard out
	//std::cout << res << std::endl;

	tabulate::Table boards;
	boards.add_row({ "ID", "Name" });

	nlohmann::json body = nlohmann::json::parse(res.body());
	for (auto i = 0; i < body.size(); ++i) {
		boards.add_row({ std::to_string(i), body[i].find("name").value() });
		boards_map.emplace(std::pair<std::string, std::string>(std::to_string(i), body[i].find("name").value()));
	}

	boards[0][0].format()
		.font_color(tabulate::Color::yellow)
		.font_align(tabulate::FontAlign::center)
		.font_style({ tabulate::FontStyle::bold });

	boards[0][1].format()
		.font_color(tabulate::Color::yellow)
		.font_align(tabulate::FontAlign::center)
		.font_style({ tabulate::FontStyle::bold });

	std::cout << boards << std::endl;
}
