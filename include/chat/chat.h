#include <boost/asio.hpp>
#include <chat/irc_client.h>
#include <chat/message_container.h>
#include <string>
#include <vector>

namespace asio = boost::asio;

namespace chat {

class Chat {
  public:
    Chat(irc::settings &settings);
    std::vector<MessageContainer> messages;
    void async_read_chat();

  private:
    asio::io_context _ioc;
    irc::client _irc;
    std::string _nick;
    std::string _token;
    std::string _channel;
    irc::settings settings;
};

} // namespace chat
