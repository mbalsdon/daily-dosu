#include "DosuConfig.h"

#include <nlohmann/json.hpp>

#include <fstream>

int DosuConfig::logLevel;
std::string DosuConfig::discordBotToken;
std::string DosuConfig::osuClientID;
std::string DosuConfig::osuClientSecret;
int DosuConfig::osuApiCooldownMs;

/**
 * Load JSON configuration.
 */
void DosuConfig::load(const std::filesystem::path& filePath)
{
    nlohmann::json configDataJson;

    std::ifstream inputFile(filePath);
    inputFile >> configDataJson;
    inputFile.close();

    DosuConfig::logLevel = configDataJson["LOG_LEVEL"];
    DosuConfig::discordBotToken = configDataJson["DISCORD_BOT_TOKEN"];
    DosuConfig::osuClientID = configDataJson["OSU_CLIENT_ID"];
    DosuConfig::osuClientSecret = configDataJson["OSU_CLIENT_SECRET"];
    DosuConfig::osuApiCooldownMs = configDataJson["OSU_API_COOLDOWN_MS"];
}
