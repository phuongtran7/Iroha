#include "Iroha.h"

int main()
{
    // The io_context is required for all I/O
    boost::asio::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx(ssl::context::tlsv12_client);

    // This holds the root certificate used for verification
    load_root_certificates(ctx);

    // Verify the remote server's certificate
    ctx.set_verify_mode(ssl::verify_peer);

    Client client{ioc, ctx};

    client.view_board();
    client.view_list("2");
    //client.view_card("2-0");

    //std::string new_board{ "Iroha" };
    //client.create_board(new_board);

    //std::string new_list{ "Test list" };
    //client.create_list("2", new_list);

    //std::string new_card{ " Never accept the world as it appears to be, dare to see it for what it could be" };
    //client.create_card("2-0", new_card);

    //std::string new_name{ "Isshiki Iroha" };
    //client.update_board("2", new_name);

    //std::string new_name{ "Test List New" };
    //client.update_list("2-0", new_name);

    std::getchar();
	return 0;
}
