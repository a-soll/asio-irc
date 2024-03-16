#include <chat/chat.h>

namespace chat {

Chat::Chat(irc::settings &settings)
    : settings{std::move(settings)}
    , _irc{this->_ioc, settings} {}

void Chat::async_read_chat() {
    this->_ioc.run();
}

} // namespace chat
