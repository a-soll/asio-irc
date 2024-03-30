#include <chat/chat.h>
#include <iostream>

namespace chat {

Chat::Chat(irc::settings &settings, irc::message_handler handler)
    : _settings{std::move(settings)}
    , _handler(handler)
    , _irc{this->_ioc, settings, handler} {}

void Chat::async_read_chat() {
    this->_ioc.run();
}

} // namespace chat
