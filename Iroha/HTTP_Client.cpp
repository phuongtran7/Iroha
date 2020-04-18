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

	// Set up an HTTP GET request message
	req_.version(version_);
	req_.set(http::field::host, host_);
	req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
}

void Client::make_request(http::verb type, const std::string& target)
{
	req_.method(type);
	req_.target(target);
}

std::string Client::trim_to_new_line(const std::string& input)
{
	if (input.empty()) {
		return std::string{};
	}
	std::size_t pos = input.find("\n");
	return fmt::format("{}...", input.substr(0, pos));
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
	if (boards_map_.size() != 0) {
		boards_map_.clear();
	}

	http::response<http::string_body> res;

	auto const target = fmt::format("/1/members/me/boards?fields=name&filter=open&{}", secrect_);
	make_request(http::verb::get, target);

	// Send the HTTP request to the remote host
	http::write(stream_, req_);

	// Receive the HTTP response
	http::read(stream_, buffer_, res);

	// Write the message to standard out
	//std::cout << res << std::endl;

	if (res.body().empty()) {
		fmt::print("Wow, such empty.\n");
		return;
	}

	tabulate::Table header;
	header.add_row({ "Boards" });
	header[0][0].format()
		.font_color(tabulate::Color::green)
		.font_align(tabulate::FontAlign::center)
		.font_style({ tabulate::FontStyle::bold });


	tabulate::Table boards;
	boards.add_row({ "ID", "Name" });

	nlohmann::json body = nlohmann::json::parse(res.body());
	for (auto i = 0; i < body.size(); ++i) {
		boards.add_row({ std::to_string(i), body[i].find("name").value() });
		Item board{ body[i].find("id").value() , body[i].find("name").value() };
		boards_map_.emplace(std::to_string(i), board);
	}

	for (auto i = 0; i < 2; i++) {
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
	if (lists_map_.size() != 0) {
		lists_map_.clear();
	}

	// Search for trello ID using user-friendly board ID
	auto board = boards_map_.find(board_id);

	if (board == boards_map_.end()) {
		fmt::print("View list failed. Cannot find board with ID: {}\n", board_id);
		return;
	}

	http::response<http::string_body> res;
	auto const target = fmt::format("/1/boards/{}/lists?{}", board->second.trello_id, secrect_);
	make_request(http::verb::get, target);

	// Send the HTTP request to the remote host
	http::write(stream_, req_);

	// Receive the HTTP response
	http::read(stream_, buffer_, res);

	// Write the message to standard out
	//std::cout << res << std::endl;

	if (res.body().empty()) {
		fmt::print("Wow, such empty.\n");
		return;
	}

	tabulate::Table header;
	header.add_row({ "Lists" });
	header[0][0].format()
		.font_color(tabulate::Color::green)
		.font_align(tabulate::FontAlign::center)
		.font_style({ tabulate::FontStyle::bold });


	tabulate::Table lists;
	lists.add_row({ "ID", "Name" });

	nlohmann::json body = nlohmann::json::parse(res.body());
	for (auto i = 0; i < body.size(); ++i) {
		auto list_id = fmt::format("{}-{}", board_id, i); // Prepend the user-friendly board ID
		lists.add_row({ list_id, body[i].find("name").value() });
		Item list{ body[i].find("id").value() , body[i].find("name").value() };
		lists_map_.emplace(list_id, list);
	}

	for (auto i = 0; i < 2; i++) {
		// Center all the collumns of the first row
		lists[0][i].format()
			.font_align(tabulate::FontAlign::center)
			.font_style({ tabulate::FontStyle::bold });
	}

	header.add_row({ lists });
	header[1].format().hide_border_top();

	std::cout << header << std::endl;
}

void Client::view_card(const std::string& list_id)
{
	if (cards_map_.size() != 0) {
		cards_map_.clear();
	}

	// Search for trello ID using user-friendly board ID
	auto list = lists_map_.find(list_id);

	if (list == lists_map_.end()) {
		fmt::print("View card failed. Cannot find list with ID: {}\n", list_id);
		return;
	}


	http::response<http::string_body> res;

	// Currently only need to get id, name and desciption of a card
	auto const target = fmt::format("/1/lists/{}/cards?fields=name,desc,id&{}", list->second.trello_id, secrect_);
	make_request(http::verb::get, target);

	// Send the HTTP request to the remote host
	http::write(stream_, req_);

	// Receive the HTTP response
	http::read(stream_, buffer_, res);

	// Write the message to standard out
	//std::cout << res << std::endl;

	if (res.body().empty()) {
		fmt::print("Wow, such empty.\n");
		return;
	}

	tabulate::Table header;
	header.add_row({ "Cards" });
	header[0][0].format()
		.font_color(tabulate::Color::green)
		.font_align(tabulate::FontAlign::center)
		.font_style({ tabulate::FontStyle::bold });


	tabulate::Table cards;
	cards.add_row({ "ID", "Name", "Description" });

	nlohmann::json body = nlohmann::json::parse(res.body());
	for (auto i = 0; i < body.size(); ++i) {
		auto card_id = fmt::format("{}-{}", list_id, i); // Prepend the user-friendly list ID
		auto desc = body[i].find("desc").value();
		cards.add_row({ card_id, body[i].find("name").value(), trim_to_new_line(desc) });
		Item card{ body[i].find("id").value() , body[i].find("name").value() };
		cards_map_.emplace(card_id, card);
	}

	for (auto i = 0; i < body.size(); i++) {
		// Force fixed size on name and desc column
		cards[i][1].format().width(25);
		cards[i][2].format().width(50);
	}

	for (auto i = 0; i < 3; i++) {
		// Center all the collumns of the first row
		cards[0][i].format()
			.font_align(tabulate::FontAlign::center)
			.font_style({ tabulate::FontStyle::bold });
	}

	header.add_row({ cards });
	header[1].format().hide_border_top();

	std::cout << header << std::endl;
}

bool Client::create_board(std::string& name)
{
	// Trello allows duplicated names in Board, List and Card.

	http::response<http::string_body> res;

	// Replace all space in name with HTML code
	boost::replace_all(name, " ", "+");

	auto const target = fmt::format("/1/boards/?name={}&defaultLists=false&{}", name, secrect_);
	make_request(http::verb::post, target);

	// Send the HTTP request to the remote host
	http::write(stream_, req_);

	// Receive the HTTP response
	http::read(stream_, buffer_, res);

	if (res.result() != http::status::ok) {
		fmt::print("Create board failed: {}: {}\n", res.result_int(), res.reason().to_string());
		return false;
	}

	// Write the message to standard out
	//std::cout << res << std::endl;

	// Due to the response sorts the board name, so we have to make another request to get the correct ID
	view_board();

	return true;
}

bool Client::create_list(const std::string& board_id, std::string& name)
{
	// Find the Trello ID that the list is in
	auto list = boards_map_.find(board_id);

	if (list == boards_map_.end()) {
		fmt::print("Create list failed. Cannot find board with ID: {}\n", board_id);
		return false;
	}

	http::response<http::string_body> res;

	// Replace all space in name with HTML code
	boost::replace_all(name, " ", "+");

	auto const target = fmt::format("/1/lists?name={}&idBoard={}&{}", name, list->second.trello_id, secrect_);
	make_request(http::verb::post, target);

	// Send the HTTP request to the remote host
	http::write(stream_, req_);

	// Receive the HTTP response
	http::read(stream_, buffer_, res);

	if (res.result() != http::status::ok) {
		fmt::print("Create list failed: {}: {}\n", res.result_int(), res.reason().to_string());
		return false;
	}

	// Write the message to standard out
	//std::cout << res << std::endl;

	view_list(board_id);

	return true;
}

bool Client::create_card(const std::string& list_id, std::string& name)
{
	// Find the Trello ID of the list that the card is in
	auto card = lists_map_.find(list_id);

	if (card == lists_map_.end()) {
		fmt::print("Create card failed. Cannot find list with ID: {}\n", list_id);
		return false;
	}

	http::response<http::string_body> res;

	// Replace all space in name with HTML code
	boost::replace_all(name, " ", "+");

	auto const target = fmt::format("/1/cards?name={}&idList={}&{}", name, card->second.trello_id, secrect_);
	make_request(http::verb::post, target);

	// Send the HTTP request to the remote host
	http::write(stream_, req_);

	// Receive the HTTP response
	http::read(stream_, buffer_, res);

	if (res.result() != http::status::ok) {
		fmt::print("Create list failed: {}: {}\n", res.result_int(), res.reason().to_string());
		return false;
	}

	// Write the message to standard out
	//std::cout << res << std::endl;

	view_card(list_id);

	return true;
}

bool Client::update_board(const std::string& board_id, std::string& new_name)
{
	// Find the Trello ID of the board
	auto board = boards_map_.find(board_id);

	if (board == boards_map_.end()) {
		fmt::print("Update board failed. Cannot find board with ID: {}\n", board_id);
		return false;
	}

	http::response<http::string_body> res;

	// Replace all space in name with HTML code
	boost::replace_all(new_name, " ", "+");

	auto const target = fmt::format("/1/boards/{}?name={}&{}", board->second.trello_id, new_name, secrect_);
	make_request(http::verb::put, target);

	// Send the HTTP request to the remote host
	http::write(stream_, req_);

	// Receive the HTTP response
	http::read(stream_, buffer_, res);

	if (res.result() != http::status::ok) {
		fmt::print("Update board failed: {}: {}\n", res.result_int(), res.reason().to_string());
		return false;
	}

	// Write the message to standard out
	//std::cout << res << std::endl;

	view_board();

	return true;
}

bool Client::update_list(const std::string& list_id, std::string& new_name)
{
	// Find the Trello ID of the list
	auto list = lists_map_.find(list_id);

	if (list == lists_map_.end()) {
		fmt::print("Update list failed. Cannot find list with ID: {}\n", list_id);
		return false;
	}

	http::response<http::string_body> res;

	// Replace all space in name with HTML code
	boost::replace_all(new_name, " ", "+");

	auto const target = fmt::format("/1/lists/{}?name={}&{}", list->second.trello_id, new_name, secrect_);
	make_request(http::verb::put, target);

	// Send the HTTP request to the remote host
	http::write(stream_, req_);

	// Receive the HTTP response
	http::read(stream_, buffer_, res);

	if (res.result() != http::status::ok) {
		fmt::print("Update list failed: {}: {}\n", res.result_int(), res.reason().to_string());
		return false;
	}

	// Write the message to standard out
	//std::cout << res << std::endl;

	view_list(list_id.substr(0, 1));

	return true;
}

bool Client::update_card(const std::string& card_id, std::string& new_name, std::string& new_desc)
{
	// Find the Trello ID of the card
	auto card = cards_map_.find(card_id);

	if (card == cards_map_.end()) {
		fmt::print("Update card failed. Cannot find board with ID: {}\n", card_id);
		return false;
	}

	http::response<http::string_body> res;

	// Replace all space in name with HTML code
	boost::replace_all(new_name, " ", "+");
	boost::replace_all(new_desc, " ", "+");

	auto const target = fmt::format("/1/cards/{}?name={}&desc={}&{}", card->second.trello_id, new_name, new_desc, secrect_);
	make_request(http::verb::put, target);

	// Send the HTTP request to the remote host
	http::write(stream_, req_);

	// Receive the HTTP response
	http::read(stream_, buffer_, res);

	if (res.result() != http::status::ok) {
		fmt::print("Update card failed: {}: {}\n", res.result_int(), res.reason().to_string());
		return false;
	}

	// Write the message to standard out
	//std::cout << res << std::endl;

	view_card(card_id.substr(0, 3));

	return true;
}
