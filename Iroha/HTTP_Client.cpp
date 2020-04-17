#include "HTTP_Client.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

void Client::make_secrect()
{
	YAML::Node config = YAML::LoadFile("Config.yaml");
	auto key = config["Key"].as<std::string>();
	auto token = config["Token"].as<std::string>();

	// Prepend "?" if secrect is the only thing that need to append to URL else prepend "&"
	secrect_ = fmt::format("key={}&token={}", key, token);
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

Client::Client(boost::asio::io_context& ioc, ssl::context& ctx) :
	resolver_{ ioc },
	stream_{ ioc, ctx }
{
	make_secrect();
	init();
}

Client::~Client()
{
}

Client::Client(Client&& other) noexcept :
	resolver_(std::move(other.resolver_)),
	stream_(std::move(other.stream_)),
	req_(std::move(other.req_)),
	buffer_(std::move(other.buffer_)),
	secrect_(std::move(other.secrect_)),
	boards_map_(std::move(other.boards_map_))
{
}

Client& Client::operator=(Client&& other) noexcept
{
	std::swap(resolver_, other.resolver_);
	std::swap(stream_, other.stream_);
	std::swap(req_, other.req_);
	std::swap(buffer_, other.buffer_);
	std::swap(secrect_, other.secrect_);
	std::swap(boards_map_, other.boards_map_);

	return *this;
}

void Client::view_board()
{
	http::response<http::string_body> res;

	auto const target = fmt::format("/1/members/me/boards?fields=name&filter=open&{}", secrect_);
	make_request(target);

	// Send the HTTP request to the remote host
	http::write(stream_, req_);

	// Receive the HTTP response
	http::read(stream_, buffer_, res);

	// Write the message to standard out
	//std::cout << res << std::endl;

	tabulate::Table header;
	header.add_row({ "Boards" });
	header[0][0].format()
		.font_color(tabulate::Color::green)
		.font_align(tabulate::FontAlign::center)
		.font_style({ tabulate::FontStyle::bold });


	tabulate::Table boards;
	boards.add_row({ "ID", "Name", "Trello ID"});

	nlohmann::json body = nlohmann::json::parse(res.body());
	for (auto i = 0; i < body.size(); ++i) {
		boards.add_row({ std::to_string(i), body[i].find("name").value(), body[i].find("id").value() });
		Item board{ body[i].find("id").value() , body[i].find("name").value() };
		boards_map_.emplace(std::to_string(i), board);
	}

	for (auto i = 0; i < 3; i++) {
		// Center all the collumns of the first row
		boards[0][i].format()
			// For some reason if embedded inside another table
			// the header cannot be colored
			//.font_color(tabulate::Color::green)
			.font_align(tabulate::FontAlign::center)
			.font_style({ tabulate::FontStyle::bold });
	}

	header.add_row({ boards });
	header[1].format().hide_border_top();

	std::cout << header << std::endl;
}

void Client::view_list(const std::string& board_id)
{
	// Search for trello ID using user-friendly board ID
	auto board = boards_map_.find(board_id)->second;

	http::response<http::string_body> res;
	auto const target = fmt::format("/1/boards/{}/lists?{}", board.trello_id, secrect_);
	make_request(target);

	// Send the HTTP request to the remote host
	http::write(stream_, req_);

	// Receive the HTTP response
	http::read(stream_, buffer_, res);

	// Write the message to standard out
	//std::cout << res << std::endl;

	tabulate::Table header;
	header.add_row({ "Lists" });
	header[0][0].format()
		.font_color(tabulate::Color::green)
		.font_align(tabulate::FontAlign::center)
		.font_style({ tabulate::FontStyle::bold });


	tabulate::Table boards;
	boards.add_row({ "ID", "Name", "Trello ID" });

	nlohmann::json body = nlohmann::json::parse(res.body());
	for (auto i = 0; i < body.size(); ++i) {
		auto list_id = fmt::format("{}-{}", board_id, i); // Prepend the user-friendly board ID
		boards.add_row({ list_id, body[i].find("name").value(), body[i].find("id").value() });
		Item board{ body[i].find("id").value() , body[i].find("name").value() };
		boards_map_.emplace(std::to_string(i), board);
	}

	for (auto i = 0; i < 3; i++) {
		// Center all the collumns of the first row
		boards[0][i].format()
			.font_align(tabulate::FontAlign::center)
			.font_style({ tabulate::FontStyle::bold });
	}

	header.add_row({ boards });
	header[1].format().hide_border_top();

	std::cout << header << std::endl;
}
