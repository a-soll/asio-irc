#pragma once

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
    const std::string &nick;
    const std::string &token;
    const std::string &channel;
};

using message_handler = std::function<void(std::string_view)>;

class client {
  public:
    // callback from calling class
    using completion_handler = std::function<void(std::string_view)>;
    client(
        asio::io_context &ctx,
        const irc::settings &settings,
        completion_handler c_handler
    );
    void join(std::string_view channel);
    void say(std::string_view receiver, std::string_view message);
    void send_line(const std::string &data);
    void register_handler(std::string &&name, message_handler handler);
    void register_on_connect(std::function<void()> handler);

    inline auto const &get_settings() const {
        return _settings;
    }

  private:
    // @TODO: get rid of this copy
    std::string _parse_type(const std::string &line);
    completion_handler _completion_handler;
    using tcp = asio::ip::tcp;
    void _connect();
    void _identify();
    void _on_hostname_resolved(
        boost::system::error_code const &error,
        tcp::resolver::results_type results
    );
    void _on_connected(boost::system::error_code const &error);
    void _await_new_line();
    void _on_new_line(std::string const &line);
    void _handle_message(std::string const &type, std::string_view message);
    void _send_raw();
    void _handle_write(boost::system::error_code const &error, std::size_t bytes_read);
    asio::io_context &_ctx;
    irc::settings const &_settings;
    std::string _host;
    int _port;
    tcp::socket _socket;
    asio::streambuf _in_buf;
    std::unordered_map<std::string, std::vector<message_handler>> _handlers;
    std::vector<std::function<void()>> _on_connect_handlers;
    std::deque<std::string> _to_write;
};

} // namespace irc
} // namespace chat
