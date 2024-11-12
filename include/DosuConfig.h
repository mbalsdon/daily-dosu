#ifndef __DOSU_CONFIG_H__
#define __DOSU_CONFIG_H__

#include <string>

const std::string k_logLevelKey = "LOG_LEVEL";
const std::string k_discordBotTokenKey = "DISCORD_BOT_TOKEN";
const std::string k_osuClientIdKey = "OSU_CLIENT_ID";
const std::string k_osuClientSecretKey = "OSU_CLIENT_SECRET";
const std::string k_osuApiCooldownMsKey = "OSU_API_COOLDOWN_MS";

class DosuConfig 
{
public:
    static void load();
    static bool configExists();
    static void setupConfig();

    static int logLevel;
    static std::string discordBotToken;
    static std::string osuClientID;
    static std::string osuClientSecret;
    static int osuApiCooldownMs;
};

#endif /* __DOSU_CONFIG_H__ */
