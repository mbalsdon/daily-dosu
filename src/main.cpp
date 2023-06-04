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
    DosuConfig::load("/home/mathew/dev/build/daily-dosu/dosu_config.json");
    
    OsuWrapper osu(DosuConfig::osuClientID, DosuConfig::osuClientSecret);
    
    Bot bot(DosuConfig::discordBotToken);
    bot.start();

    return 0;
}
