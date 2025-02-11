#include "web_socket_server.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <boost/asio.hpp>

using json = nlohmann::json;
using boost::asio::ip::tcp;

WebSocketServer::WebSocketServer(uint16_t ws_port, uint16_t http_port, const std::string &log_file)
    : log_watcher_(log_file),
      http_acceptor_(http_io_service_, tcp::endpoint(tcp::v4(), http_port)),
      running_(true),
      http_port_(http_port),
      ws_port_(ws_port)
{

    server_.clear_access_channels(websocketpp::log::alevel::all);
    server_.clear_error_channels(websocketpp::log::elevel::all);
    server_.init_asio();
    server_.set_reuse_addr(true);

    server_.set_open_handler(
        std::bind(&WebSocketServer::onOpen, this, std::placeholders::_1));
    server_.set_close_handler(
        std::bind(&WebSocketServer::onClose, this, std::placeholders::_1));

    try
    {
        server_.listen(ws_port);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Failed to listen on WebSocket port " << ws_port << ": " << e.what() << std::endl;
        throw;
    }

    log_watcher_.setCallback(
        std::bind(&WebSocketServer::onLogUpdate, this, std::placeholders::_1));
}

std::string WebSocketServer::loadFile(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return "";
    }
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

void WebSocketServer::runHttpServer()
{
    while (running_)
    {
        try
        {
            tcp::socket socket(http_io_service_);
            http_acceptor_.accept(socket);

            boost::asio::streambuf request;
            boost::asio::read_until(socket, request, "\r\n\r\n");

            std::string request_str(std::istreambuf_iterator<char>(&request), {});

            std::istringstream request_stream(request_str);
            std::string method, path, http_version;
            request_stream >> method >> path >> http_version;

            std::string response;
            std::string content_type;

            if (path == "/" || path == "/index.html")
            {
                path = "web/index.html";
                content_type = "text/html";
            }

            std::string content = loadFile(path);
            if (content.empty())
            {
                response = "HTTP/1.1 404 Not Found\r\n"
                           "Content-Length: 9\r\n"
                           "Connection: close\r\n\r\n"
                           "Not Found";
            }
            else
            {
                response = "HTTP/1.1 200 OK\r\n"
                           "Content-Type: " +
                           content_type + "\r\n"
                                          "Content-Length: " +
                           std::to_string(content.length()) + "\r\n"
                                                              "Connection: close\r\n\r\n" +
                           content;
            }

            boost::asio::write(socket, boost::asio::buffer(response));
        }
        catch (const std::exception &e)
        {
            if (running_)
            {
                std::cerr << "HTTP Server error: " << e.what() << std::endl;
            }
        }
    }
}

void WebSocketServer::run()
{
    log_watcher_.start();

    http_thread_ = std::thread(&WebSocketServer::runHttpServer, this);

    try
    {
        server_.start_accept();
        std::cout << "WebSocket server is running on port " << ws_port_ << std::endl;
        std::cout << "HTTP server is running on port " << http_port_ << std::endl;
        std::cout << "Visit http://localhost:" << http_port_ << " to view the log" << std::endl;
        server_.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Server error: " << e.what() << std::endl;
        throw;
    }
}

void WebSocketServer::stop()
{
    running_ = false;

    try
    {
        log_watcher_.stop();
        server_.stop_listening();

        std::lock_guard<std::mutex> lock(mutex_);
        for (auto &hdl : connections_)
        {
            try
            {
                server_.close(hdl, websocketpp::close::status::going_away, "Server shutdown");
            }
            catch (...)
            {
            }
        }

        server_.stop();

        http_io_service_.stop();
        if (http_thread_.joinable())
        {
            http_thread_.join();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error during shutdown: " << e.what() << std::endl;
    }
}

void WebSocketServer::onOpen(connection_hdl hdl)
{
    try
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_.insert(hdl);

        auto lines = log_watcher_.getLastLines();
        json initial_data = {
            {"type", "initial"},
            {"lines", lines}};

        server_.send(hdl, initial_data.dump(), websocketpp::frame::opcode::text);
        std::cout << "New client connected" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in onOpen: " << e.what() << std::endl;
    }
}

void WebSocketServer::onClose(connection_hdl hdl)
{
    try
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_.erase(hdl);
        std::cout << "Client disconnected" << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in onClose: " << e.what() << std::endl;
    }
}

void WebSocketServer::onLogUpdate(const std::string &line)
{
    try
    {
        std::lock_guard<std::mutex> lock(mutex_);
        json update = {
            {"type", "update"},
            {"line", line}};

        std::string message = update.dump();
        for (auto it = connections_.begin(); it != connections_.end();)
        {
            try
            {
                server_.send(*it, message, websocketpp::frame::opcode::text);
                ++it;
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error sending to client: " << e.what() << std::endl;
                it = connections_.erase(it);
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in onLogUpdate: " << e.what() << std::endl;
    }
}

WebSocketServer::~WebSocketServer()
{
    stop();
}