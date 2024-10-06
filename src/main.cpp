#include "OsuWrapper.h"
#include "Bot.h"
#include "DosuConfig.h"

#include <iostream>
#include <string>

/**
 * Entrypoint.
 * TODO: impl
*/
int main(int argc, char const *argv[])
{
    // TODO: Dynamic filepath
    DosuConfig::load("/home/mathew/dev/daily-dosu/dosu_config.json");
    std::string clientID = DosuConfig::osuClientID;
    std::string clientSecret = DosuConfig::osuClientSecret;
    int apiCooldownMs = DosuConfig::osuApiCooldownMs;
    
    OsuWrapper osu(clientID, clientSecret, apiCooldownMs);

    // DosuUser user;
    // UserID id = 6385683;
    // osu.getUser(id, user);
    // std::cout << user.username << std::endl;

    // UserID userIDs[50];
    // bool succ = osu.getRankingIDs(0, userIDs, 50);
    // for (int i = 0; i < 50; ++i) {
    //     std::cout << userIDs[i] << " ";
    // }
    // std::cout << std::endl;
    
    // Bot bot(DosuConfig::discordBotToken);
    // bot.start();

    return 0;
}
