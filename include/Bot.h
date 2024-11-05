#ifndef __BOT_H__
#define __BOT_H__

#include <dpp/dpp.h>

const size_t k_numDisplayUsers = 20;
const std::string k_twemojiClockPrefix = "https://cdn.jsdelivr.net/gh/twitter/twemoji@14.0.2/assets/72x72/1f55";
const std::string k_twemojiClockSuffix = ".png";

class Bot 
{
public:
    Bot(const std::string& botToken);

    void start();
    void scrapePlayersCallback(int runHour);

private:
    void cmdPing(const dpp::slashcommand_t& event);

    dpp::cluster m_bot;

    const std::string k_cmdPing = "ping";
};

#endif /* __BOT_H__ */
