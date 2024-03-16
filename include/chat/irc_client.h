#ifndef IRC_CLIENT_HPP
#define IRC_CLIENT_HPP

#include <deque>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <boost/asio.hpp>

namespace chat {
namespace irc {

namespace asio = boost::asio;

struct settings {
    std::string nick;
    std::string token;
    std::string channel;
};

class client {
  public:
    using message_handler = std::function<void(std::string_view)>;
    client(asio::io_context &ctx, irc::settings const &settings);
    void join(std::string_view channel);
    void say(std::string_view receiver, std::string_view message);
    void send_line(std::string data);
    void register_handler(std::string name, message_handler handler);
    void register_on_connect(std::function<void()> handler);

    auto const &get_settings() const {
        return settings;
    }

  private:
    std::string parse_type(const std::string &line);
    using tcp = asio::ip::tcp;
    void connect();
    void identify();
    void on_hostname_resolved(
        boost::system::error_code const &error,
        tcp::resolver::results_type results
    );
    void on_connected(boost::system::error_code const &error);
    void await_new_line();
    void on_new_line(std::string const &line);
    void handle_message(std::string const &type, std::string_view message);
    void send_raw();
    void handle_write(boost::system::error_code const &error, std::size_t bytes_read);
    asio::io_context &ctx;
    irc::settings settings;
    std::string host;
    int port;
    tcp::socket socket;
    asio::streambuf in_buf;
    std::unordered_map<std::string, std::vector<message_handler>> handlers;
    std::vector<std::function<void()>> on_connect_handlers;
    std::deque<std::string> to_write;
};

} // namespace irc
} // namespace chat
#endif // IRC_CLIENT_HPP
