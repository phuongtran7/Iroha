#pragma once
#include "root_certificates.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <boost/algorithm/string.hpp> 
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include "fmt/format.h"
#include <nlohmann/json.hpp>
#include <tabulate/table.hpp>
#include <robin-hood-hashing/robin_hood.h>
#include "yaml-cpp/yaml.h"

class Client {
private:
	//Represent each item in Trello
	// Either a board, a list or a card
	struct Item {
		std::string trello_id;
		std::string name;
	};
	boost::asio::ip::tcp::resolver resolver_;
	boost::beast::ssl_stream<boost::beast::tcp_stream> stream_;
	const std::string host_{"api.trello.com"};
	const unsigned short port_{ 443 };
	const unsigned short version_{ 11 };
	boost::beast::http::request<boost::beast::http::empty_body> req_;
	boost::beast::flat_buffer buffer_;
	std::string secrect_;
	robin_hood::unordered_map<std::string, Item> boards_map_;
	robin_hood::unordered_map<std::string, Item> lists_map_;
	robin_hood::unordered_map<std::string, Item> cards_map_;

private:
	void make_secrect();
	void init();
	void make_request(boost::beast::http::verb type, const std::string& target);
	std::string trim_to_new_line(const std::string& input);

public:
	Client(boost::asio::io_context& ioc, ssl::context& ctx);
	~Client();

	// Copy constructor
	Client(const Client& other) = delete;
	// Copy assignment
	Client& operator=(const Client& other) = delete;
	// Move constructor
	Client(Client&& other) noexcept;
	// Move assignment
	Client& operator=(Client&& other) noexcept;

	void view_board(); // View all available board
	void view_list(const std::string& board_id); // View lists in a particualar board
	void view_card(const std::string& list_id); // View cards in list
	bool create_board(std::string& name);
	bool create_list(const std::string& board_id, std::string& name);
	bool create_card(const std::string& list_id, std::string& name);
	bool update_board(const std::string& board_id, std::string& new_name);
	bool update_list(const std::string& list_id, std::string& new_name);
};