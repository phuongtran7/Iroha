#pragma once
#include "root_certificates.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include "fmt/format.h"

void fail(boost::beast::error_code ec, char const* what);

// Performs an HTTP GET and prints the response
class session : public std::enable_shared_from_this<session>
{
private:
	boost::asio::ip::tcp::resolver resolver_;
	boost::beast::ssl_stream<boost::beast::tcp_stream> stream_;
	boost::beast::flat_buffer buffer_; // (Must persist between reads)
	boost::beast::http::request<boost::beast::http::empty_body> req_;
	boost::beast::http::response<boost::beast::http::string_body> res_;
	std::string secrect_;

private:
	void on_resolve(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type results);
	void on_connect(boost::beast::error_code ec, boost::asio::ip::tcp::resolver::results_type::endpoint_type);
	void on_handshake(boost::beast::error_code ec);
	void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);
	void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
	void on_shutdown(boost::beast::error_code ec);

public:
	explicit session(boost::asio::executor ex, ssl::context& ctx, const std::string& secrect);
	void run(char const* host, char const* port, char const* target, int version);
};