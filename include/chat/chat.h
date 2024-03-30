#pragma once

#include <chat/irc_client.h>
#include <string>
#include <vector>

#include <boost/asio.hpp>

namespace asio = boost::asio;

namespace chat {

class Chat {
  public:
    Chat(irc::settings &settings, irc::message_handler handler);
    void async_read_chat();

  private:
    asio::io_context _ioc;
    irc::client _irc;
    irc::settings _settings;
    irc::message_handler _handler;
};

} // namespace chat
