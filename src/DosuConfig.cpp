#include "DosuConfig.h"
#include "Logger.h"
#include "Util.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>

int DosuConfig::logLevel;
std::string DosuConfig::discordBotToken;
std::string DosuConfig::osuClientID;
std::string DosuConfig::osuClientSecret;
int DosuConfig::scrapeRankingsRunHour;
std::filesystem::path DosuConfig::rankingsDatabaseFilePath;
std::filesystem::path DosuConfig::botConfigDatabaseFilePath;

namespace Detail
{
/**
 * Convert given hour from UTC to system timezone.
 * Thanks to the dev who put this on SO 14 years ago and thanks to Claude for feeding it to me!
 */
int utcToLocal(int utcHour)
{
    auto now = std::chrono::system_clock::now();
    auto nowTt = std::chrono::system_clock::to_time_t(now);

    std::tm localTm = *std::localtime(&nowTt);
    std::tm utcTm = *std::gmtime(&nowTt);

    int offset = localTm.tm_hour - utcTm.tm_hour;
    if (offset > 12)
    {
        offset -= 24;
    }
    else if (offset < -12)
    {
        offset += 24;
    }

    int localHour = utcHour + offset;
    if (localHour >= 24)
    {
        localHour -= 24;
    }
    else if (localHour < 0)
    {
        localHour += 24;
    }

    return localHour;
}
}

/**
 * Load JSON configuration.
 */
void DosuConfig::load(std::filesystem::path filePath)
{
    LOG_DEBUG("Loading config from ", filePath);

    // Read file
    nlohmann::json configDataJson;
    std::ifstream inputFile(filePath);
    if (!inputFile.is_open())
    {
        std::string reason = std::string("DosuConfig::load - failed to open ").append(filePath.string());
        throw std::runtime_error(reason);
    }
    inputFile >> configDataJson;
    inputFile.close();

    // Store in memory
    DosuConfig::logLevel = configDataJson.at(k_logLevelKey);
    if (DosuConfig::logLevel < 0 || DosuConfig::logLevel > 3)
    {
        LOG_WARN("Configured ", k_logLevelKey, " is out of bounds! Setting to 1...");
        DosuConfig::logLevel = 1;
    }
    DosuConfig::discordBotToken = configDataJson.at(k_discordBotTokenKey);
    DosuConfig::osuClientID = configDataJson.at(k_osuClientIdKey);
    DosuConfig::osuClientSecret = configDataJson.at(k_osuClientSecretKey);
    DosuConfig::scrapeRankingsRunHour = configDataJson.at(k_scrapeRankingsRunHourKey);
    if (DosuConfig::scrapeRankingsRunHour < 0 || DosuConfig::scrapeRankingsRunHour > 23)
    {
        DosuConfig::scrapeRankingsRunHour = DosuConfig::scrapeRankingsRunHour % 24;
        LOG_WARN("Configured ", k_scrapeRankingsRunHourKey, " is out of bounds! Normalizing to ", DosuConfig::scrapeRankingsRunHour, "...");
    }
    DosuConfig::rankingsDatabaseFilePath = std::filesystem::path(configDataJson.at(k_rankingsDbFilePathKey));
    DosuConfig::botConfigDatabaseFilePath = std::filesystem::path(configDataJson.at(k_botConfigDbFilePathKey));
}

/**
 * Create config from user input.
 */
void DosuConfig::setupConfig(std::filesystem::path filePath)
{
    LOG_DEBUG("Running config setup tool...");

    nlohmann::json newConfigJson;
    newConfigJson[k_logLevelKey] = 1;
    newConfigJson[k_scrapeRankingsRunHourKey] = Detail::utcToLocal(5);

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
    newConfigJson[k_rankingsDbFilePathKey] = k_dataDir / "rankings.db";
    newConfigJson[k_botConfigDbFilePathKey] = k_dataDir / "bot_config.db";

    std::ofstream outputFile(filePath);
    outputFile << newConfigJson.dump(4) << std::endl;

    std::cout << "Success! Some values have been set by default. Config can be found at " << filePath << std::endl;
}
