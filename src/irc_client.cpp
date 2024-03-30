#include <chat/irc_client.h>
#include <iostream>
#include <istream>
#include <sstream>
#include <stdexcept>

namespace chat {
namespace irc {

/**
 * other example options:
 * this->_irc.register_handler("PRIVMSG", [this](auto &&...views) {
 *     say_time(this->_irc, views...);
 * });
 *
 * this->_irc.register_handler("JOIN", [this](auto &&...views) {
 *     greet(this->irc, views...);
 * });
 */
client::client(
    asio::io_context &ctx,
    irc::settings const &settings,
    completion_handler c_handler
)
    : _ctx{ctx}
    , _settings{settings}
    , _socket{ctx}
    , _completion_handler(std::move(c_handler)) {

    this->_host = "irc.chat.twitch.tv";
    this->_port = 6667;

    register_handler("PING", [this](std::string_view message) {
        std::stringstream pong;
        pong << "PONG :" << message;
        send_line(pong.str());
    });

    register_handler("001", [this](std::string_view message) {
        this->join(this->_settings.channel);
    });

    _connect();
}

void client::join(std::string_view channel) {
    std::stringstream msg;
    msg << "JOIN #" << channel;
    send_line(msg.str());
}

void client::say(std::string_view receiver, std::string_view message) {
    std::stringstream msg;
    msg << "PRIVMSG " << receiver << " :" << message;
    send_line(msg.str());
}

void client::send_line(const std::string &data) {
    std::cout << "Sending: " << data << std::endl;
    std::string m(data); // gross copy
    m += "\r\n";
    this->_to_write.push_back(std::move(m));
    // if size == 1 then there is only
    // the line we've just added.
    // Nothing is being sent at this
    // moment so we can safely proceed
    // to send the new message.
    if (this->_to_write.size() == 1)
        _send_raw();
}

void client::register_handler(std::string &&name, message_handler handler) {
    _handlers[std::move(name)].push_back(handler);
}

void client::register_on_connect(std::function<void()> handler) {
    _on_connect_handlers.push_back(handler);
}

void client::_connect() {
    _socket.close();
    tcp::resolver resolver(_ctx);

    auto handler = [this](auto &&...params) {
        _on_hostname_resolved(std::forward<decltype(params)>(params)...);
    };

    resolver.async_resolve(this->_host, std::to_string(this->_port), handler);
}

/**
 * the type is between the first and second space in the string
 */
std::string client::_parse_type(const std::string &line) {
    bool first_space = false;
    std::string_view sv(&line[line.find(".tv")]);
    std::string type;
    for (const auto &c : sv) {
        if (c == ' ') {
            if (!first_space) {
                first_space = true;
                continue;
            } else {
                break;
            }
        }
        if (first_space) {
            type += c;
        }
    }
    return type;
}

void client::_identify() {
    std::stringstream msg;
    msg << "PASS oauth:" + this->_settings.token;
    this->send_line(msg.str());

    msg.str("");
    msg << "NICK " << this->_settings.nick;
    this->send_line(msg.str());
    this->send_line("CAP REQ :twitch.tv/tags");
}

void client::_on_hostname_resolved(
    boost::system::error_code const &error,
    tcp::resolver::results_type results
) {
    if (error) {
        _connect();
        return;
    }
    if (!results.size()) {
        std::stringstream msg;
        msg << "Failed to resolve '" << this->_host << "'";
        throw std::runtime_error(msg.str());
    }
    auto handler = [this](auto const &error) { this->_on_connected(error); };
    _socket.async_connect(*results, handler);
}

void client::_on_connected(boost::system::error_code const &error) {
    if (error) {
        _connect();
        return;
    }
    _identify();
    for (auto &handler : _on_connect_handlers) {
        handler();
    }
    _await_new_line();
}

void client::_await_new_line() {
    auto handler = [this](auto const &error, std::size_t s) {
        if (error) {
            _connect();
            return;
        }

        std::istream i{&this->_in_buf};
        std::string line;
        std::getline(i, line);

        _on_new_line(line);
        _await_new_line();
    };
    asio::async_read_until(this->_socket, this->_in_buf, "\r\n", handler);
}

void client::_on_new_line(std::string const &message) {
    std::string type;
    type = this->_parse_type(message);

    _handle_message(type, message);
}

void client::_handle_message(std::string const &type, std::string_view message) {
    for (auto const &h : _handlers[type]) {
        h(message);
    }
    this->_completion_handler(message);
}

void client::_send_raw() {
    if (this->_to_write.empty()) {
        return;
    }

    _socket.async_send(
        asio::buffer(this->_to_write.front().data(), this->_to_write.front().size()),
        [this](auto &&...params) { this->_handle_write(params...); }
    );
}

void client::_handle_write(
    boost::system::error_code const &error,
    std::size_t bytes_read
) {
    if (error) {
        std::cerr << "Error: " << error << std::endl;
        return;
    }
    auto to_erase = std::min(bytes_read, this->_to_write.front().size());
    auto &buf     = this->_to_write.front();
    buf.erase(buf.begin(), buf.begin() + to_erase);

    if (buf.empty()) {
        this->_to_write.erase(this->_to_write.begin());
    }

    if (!this->_to_write.empty()) {
        _send_raw();
    }
}

} // namespace irc
} // namespace chat
