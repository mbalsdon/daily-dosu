#ifndef __DOSU_CONFIG_H__
#define __DOSU_CONFIG_H__

#include <dpp/nlohmann/json.hpp>

class DosuConfig 
{
public:
    static void load(const std::string& filePath);

    static std::string discordBotToken;
    static std::string osuClientID;
    static std::string osuClientSecret;
    static std::string osuClientRedirectURI;
    static int osuApiCooldownMs;
};

#endif /* __DOSU_CONFIG_H__ */