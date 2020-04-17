#pragma once
#include "root_certificates.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <iostream>
#include <memory>
#include <string>
#include "fmt/format.h"
#include <nlohmann/json.hpp>
#include <tabulate/table.hpp>
#include <robin-hood-hashing/robin_hood.h>
#include "yaml-cpp/yaml.h"

class Client {
private:
	boost::asio::ip::tcp::resolver resolver_;
	boost::beast::ssl_stream<boost::beast::tcp_stream> stream_;
	const std::string host_{"api.trello.com"};
	const unsigned short port_{ 443 };
	const unsigned short version_{ 11 };
	boost::beast::http::request<boost::beast::http::empty_body> req_;
	boost::beast::flat_buffer buffer_;
	std::string secrect_;
	robin_hood::unordered_map<std::string, std::string> boards_map;

private:
	void make_secrect();

public:
	Client(boost::asio::executor ex, ssl::context& ctx);
	~Client();

	// Copy constructor
	Client(const Client& other) = delete;
	// Copy assignment
	Client& operator=(const Client& other) = delete;
	// Move constructor
	Client(Client&& other) noexcept;
	// Move assignment
	Client& operator=(Client&& other) noexcept;

	void init();
	void make_request(const std::string& target);
	void get_board();
};