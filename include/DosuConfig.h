#ifndef __DOSU_CONFIG_H__
#define __DOSU_CONFIG_H__

#include <filesystem>
#include <string>

class DosuConfig 
{
public:
    static void load(const std::filesystem::path& filePath);

    static int logLevel;
    static std::string discordBotToken;
    static std::string osuClientID;
    static std::string osuClientSecret;
    static std::string osuClientRedirectURI;
    static int osuApiCooldownMs;
};

#endif /* __DOSU_CONFIG_H__ */
