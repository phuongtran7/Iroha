#include "Iroha.h"

int main()
{
    fmt::print("Welcome to Iroha.\n");

    // The io_context is required for all I/O
    boost::asio::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx(ssl::context::tlsv12_client);

    // This holds the root certificate used for verification
    load_root_certificates(ctx);

    // Verify the remote server's certificate
    ctx.set_verify_mode(ssl::verify_peer);

    Client client{ioc, ctx};

    auto running = true;
    while (running) {
        running = client.get_user_input();
    }

	return 0;
}
