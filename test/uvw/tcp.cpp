#include <gtest/gtest.h>
#include <uvw/tcp.h>

TEST(TCP, Functionalities) {
    auto loop = uvw::loop::get_default();
    auto handle = loop->resource<uvw::tcp_handle>();

    ASSERT_TRUE(handle->no_delay(true));
    ASSERT_TRUE(handle->keep_alive(true, uvw::tcp_handle::time{128}));
    ASSERT_TRUE(handle->simultaneous_accepts());

    handle->close();
    loop->run();
}

TEST(TCP, ReadWrite) {
    const std::string address = std::string{"127.0.0.1"};
    const unsigned int port = 4242;

    auto loop = uvw::loop::get_default();
    auto server = loop->resource<uvw::tcp_handle>();
    auto client = loop->resource<uvw::tcp_handle>();

    server->on<uvw::error_event>([](const auto &, auto &) { FAIL(); });
    client->on<uvw::error_event>([](const auto &, auto &) { FAIL(); });

    server->on<uvw::listen_event>([](const uvw::listen_event &, uvw::tcp_handle &handle) {
        std::shared_ptr<uvw::tcp_handle> socket = handle.parent().resource<uvw::tcp_handle>();

        socket->on<uvw::error_event>([](const uvw::error_event &, uvw::tcp_handle &) { FAIL(); });
        socket->on<uvw::close_event>([&handle](const uvw::close_event &, uvw::tcp_handle &) { handle.close(); });
        socket->on<uvw::end_event>([](const uvw::end_event &, uvw::tcp_handle &sock) { sock.close(); });

        ASSERT_EQ(0, handle.accept(*socket));
        ASSERT_EQ(0, socket->read());
    });

    client->on<uvw::write_event>([](const uvw::write_event &, uvw::tcp_handle &handle) {
        handle.close();
    });

    client->on<uvw::connect_event>([](const uvw::connect_event &, uvw::tcp_handle &handle) {
        ASSERT_TRUE(handle.writable());
        ASSERT_TRUE(handle.readable());

        auto dataTryWrite = std::unique_ptr<char[]>(new char[1]{'a'});

        ASSERT_EQ(1, handle.try_write(std::move(dataTryWrite), 1));

        auto dataWrite = std::unique_ptr<char[]>(new char[2]{'b', 'c'});
        handle.write(std::move(dataWrite), 2);
    });

    ASSERT_EQ(0, (server->bind(address, port)));
    ASSERT_EQ(0, server->listen());
    ASSERT_EQ(0, (client->connect(address, port)));

    loop->run();
}

TEST(TCP, SockPeer) {
    const std::string address = std::string{"127.0.0.1"};
    const unsigned int port = 4242;

    auto loop = uvw::loop::get_default();
    auto server = loop->resource<uvw::tcp_handle>();
    auto client = loop->resource<uvw::tcp_handle>();

    server->on<uvw::error_event>([](const auto &, auto &) { FAIL(); });
    client->on<uvw::error_event>([](const auto &, auto &) { FAIL(); });

    server->on<uvw::listen_event>([&address](const uvw::listen_event &, uvw::tcp_handle &handle) {
        std::shared_ptr<uvw::tcp_handle> socket = handle.parent().resource<uvw::tcp_handle>();

        socket->on<uvw::error_event>([](const uvw::error_event &, uvw::tcp_handle &) { FAIL(); });
        socket->on<uvw::close_event>([&handle](const uvw::close_event &, uvw::tcp_handle &) { handle.close(); });
        socket->on<uvw::end_event>([](const uvw::end_event &, uvw::tcp_handle &sock) { sock.close(); });

        ASSERT_EQ(0, handle.accept(*socket));
        ASSERT_EQ(0, socket->read());

        uvw::socket_address addr = handle.sock();

        ASSERT_EQ(addr.ip, address);
    });

    client->on<uvw::connect_event>([&address](const uvw::connect_event &, uvw::tcp_handle &handle) {
        uvw::socket_address addr = handle.peer();

        ASSERT_EQ(addr.ip, address);

        handle.close();
    });

    ASSERT_EQ(0, (server->bind(uvw::socket_address{address, port})));
    ASSERT_EQ(0, server->listen());
    ASSERT_EQ(0, (client->connect(uvw::socket_address{address, port})));

    loop->run();
}

TEST(TCP, Shutdown) {
    const std::string address = std::string{"127.0.0.1"};
    const unsigned int port = 4242;

    auto loop = uvw::loop::get_default();
    auto server = loop->resource<uvw::tcp_handle>();
    auto client = loop->resource<uvw::tcp_handle>();

    server->on<uvw::error_event>([](const auto &, auto &) { FAIL(); });
    client->on<uvw::error_event>([](const auto &, auto &) { FAIL(); });

    server->on<uvw::listen_event>([](const uvw::listen_event &, uvw::tcp_handle &handle) {
        std::shared_ptr<uvw::tcp_handle> socket = handle.parent().resource<uvw::tcp_handle>();

        socket->on<uvw::error_event>([](const uvw::error_event &, uvw::tcp_handle &) { FAIL(); });
        socket->on<uvw::close_event>([&handle](const uvw::close_event &, uvw::tcp_handle &) { handle.close(); });
        socket->on<uvw::end_event>([](const uvw::end_event &, uvw::tcp_handle &sock) { sock.close(); });

        ASSERT_EQ(0, handle.accept(*socket));
        ASSERT_EQ(0, socket->read());
    });

    client->on<uvw::shutdown_event>([](const uvw::shutdown_event &, uvw::tcp_handle &handle) {
        handle.close();
    });

    client->on<uvw::connect_event>([](const uvw::connect_event &, uvw::tcp_handle &handle) {
        ASSERT_EQ(0, handle.shutdown());
    });

    ASSERT_EQ(0, (server->bind(address, port)));
    ASSERT_EQ(0, server->listen());
    ASSERT_EQ(0, (client->connect(address, port)));

    loop->run();
}

TEST(TCP, WriteError) {
    auto loop = uvw::loop::get_default();
    auto handle = loop->resource<uvw::tcp_handle>();

    handle->close();

    ASSERT_NE(0, (handle->write(std::unique_ptr<char[]>{}, 0)));
    ASSERT_NE(0, (handle->write(nullptr, 0)));

    ASSERT_LT(handle->try_write(std::unique_ptr<char[]>{}, 0), 0);
    ASSERT_LT(handle->try_write(nullptr, 0), 0);
}
