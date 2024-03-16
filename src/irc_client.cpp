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
client::client(asio::io_context &ctx, irc::settings const &settings)
    : ctx{ctx}
    , settings{settings}
    , socket{ctx} {
    this->host = "irc.chat.twitch.tv";
    this->port = 6667;

    register_handler("PING", [this](std::string_view message) {
        std::stringstream pong;
        pong << "PONG :" << message;
        send_line(pong.str());
    });

    register_handler("001", [this](std::string_view message) {
        this->join(this->settings.channel);
    });

    connect();
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

void client::send_line(std::string data) {
    std::cout << "Sending: " << data << std::endl;
    data += "\r\n";
    to_write.push_back(std::move(data));
    // if size == 1 then there is only
    // the line we've just added.
    // Nothing is being sent at this
    // moment so we can safely proceed
    // to send the new message.
    if (to_write.size() == 1)
        send_raw();
}

void client::register_handler(std::string name, message_handler handler) {
    handlers[std::move(name)].push_back(handler);
}

void client::register_on_connect(std::function<void()> handler) {
    on_connect_handlers.push_back(handler);
}

void client::connect() {
    socket.close();
    tcp::resolver resolver(ctx);

    auto handler = [this](auto &&...params) {
        on_hostname_resolved(std::forward<decltype(params)>(params)...);
    };

    resolver.async_resolve(this->host, std::to_string(this->port), handler);
}

/**
 * the type is between the first and second space in the string_view
 */
std::string client::parse_type(const std::string &line) {
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

void client::identify() {
    std::stringstream msg;
    msg << "PASS oauth:" + this->settings.token;
    this->send_line(msg.str());

    msg.str("");
    msg << "NICK " << this->settings.nick;
    this->send_line(msg.str());
    this->send_line("CAP REQ :twitch.tv/tags");
}

void client::on_hostname_resolved(
    boost::system::error_code const &error,
    tcp::resolver::results_type results
) {
    if (error) {
        connect();
        return;
    }
    if (!results.size()) {
        std::stringstream msg;
        msg << "Failed to resolve '" << this->host << "'";
        throw std::runtime_error(msg.str());
    }
    auto handler = [this](auto const &error) { this->on_connected(error); };
    socket.async_connect(*results, handler);
}

void client::on_connected(boost::system::error_code const &error) {
    if (error) {
        connect();
        return;
    }
    identify();
    for (auto &handler : on_connect_handlers) {
        handler();
    }
    await_new_line();
}

void client::await_new_line() {
    auto handler = [this](auto const &error, std::size_t s) {
        if (error) {
            connect();
            return;
        }

        std::istream i{&this->in_buf};
        std::string line;
        std::getline(i, line);

        on_new_line(line);
        await_new_line();
    };
    asio::async_read_until(this->socket, this->in_buf, "\r\n", handler);
}

void client::on_new_line(std::string const &message) {
    std::string type;
    type = this->parse_type(message);

    handle_message(type, message);
}

void client::handle_message(std::string const &type, std::string_view message) {
    std::cout << message << '\n';
    for (auto const &h : handlers[type]) {
        h(message);
    }
}

void client::send_raw() {
    if (to_write.empty()) {
        return;
    }

    socket.async_send(
        asio::buffer(to_write.front().data(), to_write.front().size()),
        [this](auto &&...params) { handle_write(params...); }
    );
}

void client::handle_write(
    boost::system::error_code const &error,
    std::size_t bytes_read
) {
    if (error) {
        std::cerr << "Error: " << error << std::endl;
        return;
    }
    auto to_erase = std::min(bytes_read, to_write.front().size());
    auto &buf     = to_write.front();
    buf.erase(buf.begin(), buf.begin() + to_erase);

    if (buf.empty()) {
        to_write.erase(to_write.begin());
    }

    if (!to_write.empty()) {
        send_raw();
    }
}

} // namespace irc
} // namespace chat
