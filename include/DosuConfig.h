#ifndef __DOSU_CONFIG_H__
#define __DOSU_CONFIG_H__

#include <dpp/nlohmann/json.hpp>

#include <filesystem>

const std::string k_configFileName = "dosu_config.json";

class DosuConfig 
{
public:
    static void load(const std::filesystem::path& filePath);

    static std::string discordBotToken;
    static std::string osuClientID;
    static std::string osuClientSecret;
    static std::string osuClientRedirectURI;
    static int osuApiCooldownMs;
};

#endif /* __DOSU_CONFIG_H__ */
