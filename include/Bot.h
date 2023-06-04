#ifndef __BOT_H__
#define __BOT_H__

#include <dpp/dpp.h>

class Bot 
{
public:
    Bot(const std::string& botToken);
    void start();

private:
    void cmdPing(const dpp::slashcommand_t& event);

    dpp::cluster m_bot;
};

#endif /* __BOT_H__ */