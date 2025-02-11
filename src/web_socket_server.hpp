#pragma once
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <set>
#include "log_watcher.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <thread>

class WebSocketServer
{
public:
    WebSocketServer(uint16_t ws_port, uint16_t http_port, const std::string &log_file);
    ~WebSocketServer();

    void run();
    void stop();

private:
    using server = websocketpp::server<websocketpp::config::asio>;
    using connection_hdl = websocketpp::connection_hdl;

    void onOpen(connection_hdl hdl);
    void onClose(connection_hdl hdl);
    void onLogUpdate(const std::string &line);
    void runHttpServer();
    std::string loadFile(const std::string &path);

    server server_;
    std::set<connection_hdl, std::owner_less<connection_hdl>> connections_;
    LogWatcher log_watcher_;
    std::mutex mutex_;

    boost::asio::io_service http_io_service_;
    boost::asio::ip::tcp::acceptor http_acceptor_;
    std::thread http_thread_;
    bool running_;
    uint16_t http_port_;
    uint16_t ws_port_;
};