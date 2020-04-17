#include "Iroha.h"

int main()
{
	YAML::Node config = YAML::LoadFile("Config.yaml");

	auto key = config["Key"].as<std::string>();
	auto token = config["Token"].as<std::string>();

    auto secrect = fmt::format("&key={}&token={}", key, token);

    auto const host = "api.trello.com";
    auto const port = "443";
    auto const target = fmt::format("/1/members/me/boards?fields=name&filter=open{}", secrect);
    int version = 11;

    // The io_context is required for all I/O
    boost::asio::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ ssl::context::tlsv12_client };

    // This holds the root certificate used for verification
    load_root_certificates(ctx);

    // Verify the remote server's certificate
    ctx.set_verify_mode(ssl::verify_peer);

    // Launch the asynchronous operation
    // The session is constructed with a strand to
    // ensure that handlers do not execute concurrently.
    std::make_shared<session>(boost::asio::make_strand(ioc), ctx, secrect)->run(host, port, target.c_str(), version);

    // Run the I/O service. The call will return when
    // the get operation is complete.
    ioc.run();

    std::getchar();
	return 0;
}
