#include <chat/chat.h>
#include <fstream>
#include <iostream>
#include <string>

/**
 * [0] = nick
 * [1] = token
 */
static std::vector<std::string> get_token() {
    std::ifstream f;
    std::string buf;
    std::vector<std::string> ret(2);
    f.open("config");
    while (std::getline(f, buf)) {
        if (ret[0].empty()) {
            ret[0] = buf;
        } else if (ret[1].empty()) {
            ret[1] = buf;
        } else {
            ret[2] = buf;
            break;
        }
    }
    return ret;
}

int main() {
    auto config          = get_token();
    std::string &nick    = config[0];
    std::string &token   = config[1];
    std::string &channel = config[2];

    std::cout << "NICK: " << nick << '\n';
    std::cout << "TOKEN: " << token << '\n';
    std::cout << "CHANNEL: " << channel << '\n';

    chat::irc::settings settings{nick, token, channel};
    chat::Chat e(settings);

    e.async_read_chat();
}
