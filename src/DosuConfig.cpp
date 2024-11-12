#include "DosuConfig.h"
#include "Logger.h"
#include "Util.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <filesystem>
#include <iostream>

int DosuConfig::logLevel;
std::string DosuConfig::discordBotToken;
std::string DosuConfig::osuClientID;
std::string DosuConfig::osuClientSecret;
int DosuConfig::osuApiCooldownMs;
int DosuConfig::scrapePlayersRunHour;
int DosuConfig::backupServerConfigRunHour;

/**
 * Load JSON configuration.
 */
void DosuConfig::load()
{
    LOG_DEBUG("Loading config from ", k_dosuConfigFilePath);

    nlohmann::json configDataJson;
    std::ifstream inputFile(k_dosuConfigFilePath);
    if (!inputFile.is_open())
    {
        std::string reason = std::string("DosuConfig::load - failed to open ").append(k_dosuConfigFilePath.string());
        throw std::runtime_error(reason);
    }
    inputFile >> configDataJson;
    inputFile.close();

    DosuConfig::logLevel = configDataJson[k_logLevelKey];
    if (DosuConfig::logLevel < 0 || DosuConfig::logLevel > 3)
    {
        LOG_WARN("Configured ", k_logLevelKey, " is out of bounds! Setting to 1...");
        DosuConfig::logLevel = 1;
    }
    DosuConfig::discordBotToken = configDataJson[k_discordBotTokenKey];
    DosuConfig::osuClientID = configDataJson[k_osuClientIdKey];
    DosuConfig::osuClientSecret = configDataJson[k_osuClientSecretKey];
    DosuConfig::osuApiCooldownMs = configDataJson[k_osuApiCooldownMsKey];
    if (DosuConfig::osuApiCooldownMs < 0)
    {
        LOG_WARN("Configured ", k_osuApiCooldownMsKey, " is out of bounds! Setting to 1000...");
        DosuConfig::osuApiCooldownMs = 1000;
    }
    DosuConfig::scrapePlayersRunHour = configDataJson[k_scrapePlayersRunHourKey];
    if (DosuConfig::scrapePlayersRunHour < 0 || DosuConfig::scrapePlayersRunHour > 23)
    {
        DosuConfig::scrapePlayersRunHour = DosuConfig::scrapePlayersRunHour % 24;
        LOG_WARN("Configured ", k_scrapePlayersRunHourKey, " is out of bounds! Normalizing to ", DosuConfig::scrapePlayersRunHour, "...");
    }
    DosuConfig::backupServerConfigRunHour = configDataJson[k_backupServerConfigRunHourKey];
    if (DosuConfig::backupServerConfigRunHour < 0 || DosuConfig::backupServerConfigRunHour > 23)
    {
        DosuConfig::backupServerConfigRunHour = DosuConfig::backupServerConfigRunHour % 24;
        LOG_WARN("Configured ", k_serverConfigChannelsKey, " is out of bounds! Normalizing to ", DosuConfig::backupServerConfigRunHour, "...");
    }
}

/**
 * Check if config file exists.
 */
bool DosuConfig::configExists()
{
    return std::filesystem::exists(k_dosuConfigFilePath);
}

/**
 * Create config from user input.
 */
void DosuConfig::setupConfig()
{
    LOG_DEBUG("Running config setup tool...");

    nlohmann::json newConfigJson;
    newConfigJson[k_logLevelKey] = 1;
    newConfigJson[k_osuApiCooldownMsKey] = 1000;
    newConfigJson[k_scrapePlayersRunHourKey] = 0;
    newConfigJson[k_backupServerConfigRunHourKey] = 23;

    std::string botToken;
    std::string clientID;
    std::string clientSecret;

    std::cout << "In order to run this bot, you will need to register a Discord application and an osu!API client." << std::endl;
    std::cout << "Enter Discord bot token: ";
    std::getline(std::cin, botToken);
    std::cout << "Enter osu!API client ID: ";
    std::getline(std::cin, clientID);
    std::cout << "Enter osu!API client secret: ";
    std::getline(std::cin, clientSecret);

    newConfigJson[k_discordBotTokenKey] = botToken;
    newConfigJson[k_osuClientIdKey] = clientID;
    newConfigJson[k_osuClientSecretKey] = clientSecret;

    std::ofstream outputFile(k_dosuConfigFilePath);
    outputFile << newConfigJson.dump(4) << std::endl;

    std::cout << "Success! Some values have been set by default. Config can be found at " << k_dosuConfigFilePath << std::endl;
}
