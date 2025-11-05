#include "DosuConfig.h"
#include "Logger.h"
#include "Util.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <thread>

int DosuConfig::logLevel;
bool DosuConfig::logAnsiColors;
std::string DosuConfig::discordBotToken;
std::string DosuConfig::osuClientID;
std::string DosuConfig::osuClientSecret;
int DosuConfig::scrapeRankingsRunHour;
int DosuConfig::topPlaysRunHour;
int DosuConfig::threadCount;
std::filesystem::path DosuConfig::rankingsDatabaseFilePath;
std::filesystem::path DosuConfig::topPlaysDatabaseFilePath;
std::filesystem::path DosuConfig::botConfigDatabaseFilePath;
std::map<std::string, std::string> DosuConfig::discordBotStrings;

namespace
{
/**
 * Convert given hour from UTC to system timezone.
 * guy on SO 14 years ago -> claude -> this
 */
[[nodiscard]] int utcToLocal(int const& utcHour) noexcept
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
} /* namespace */

/**
 * Load JSON configuration.
 */
void DosuConfig::load(std::filesystem::path const& filePath)
{
    LOG_DEBUG("Loading config from ", filePath);

    // Read file
    nlohmann::json configDataJson;
    std::ifstream inputFile(filePath);
    LOG_ERROR_THROW(
        inputFile.is_open(),
        "Failed to open ", filePath.string()
    );
    inputFile >> configDataJson;
    inputFile.close();

    // Store in memory
    DosuConfig::logLevel = configDataJson.at(k_logLevelKey);
    if (DosuConfig::logLevel < 0 || DosuConfig::logLevel > 3)
    {
        LOG_WARN("Configured ", k_logLevelKey, " is out of bounds! Setting to 1");
        DosuConfig::logLevel = 1;
    }
    DosuConfig::logAnsiColors = configDataJson.at(k_logAnsiColorsKey);
    DosuConfig::discordBotToken = configDataJson.at(k_discordBotTokenKey);
    DosuConfig::discordBotStrings = configDataJson.at(k_discordBotStringsKey).get<std::map<std::string, std::string>>();
    DosuConfig::osuClientID = configDataJson.at(k_osuClientIdKey);
    DosuConfig::osuClientSecret = configDataJson.at(k_osuClientSecretKey);
    DosuConfig::scrapeRankingsRunHour = configDataJson.at(k_scrapeRankingsRunHourKey);
    if (DosuConfig::scrapeRankingsRunHour < 0 || DosuConfig::scrapeRankingsRunHour > 23)
    {
        DosuConfig::scrapeRankingsRunHour = DosuConfig::scrapeRankingsRunHour % 24;
        LOG_WARN("Configured ", k_scrapeRankingsRunHourKey, " is out of bounds! Normalizing to ", DosuConfig::scrapeRankingsRunHour);
    }
    DosuConfig::topPlaysRunHour = configDataJson.at(k_topPlaysRunHourKey);
    if (DosuConfig::topPlaysRunHour < 0 || DosuConfig::topPlaysRunHour > 23)
    {
        DosuConfig::topPlaysRunHour = DosuConfig::topPlaysRunHour % 24;
        LOG_WARN("Configured ", k_topPlaysRunHourKey, " is out of bounds! Normalizing to ", DosuConfig::topPlaysRunHour);
    }
    DosuConfig::threadCount = configDataJson.at(k_threadCountKey);
    if (DosuConfig::threadCount < 1)
    {
        DosuConfig::threadCount = static_cast<int>(std::thread::hardware_concurrency());
        LOG_WARN("Configured ", k_threadCountKey, " is out of bounds! Setting to ", DosuConfig::threadCount);
    }
    DosuConfig::rankingsDatabaseFilePath = std::filesystem::path(configDataJson.at(k_rankingsDbFilePathKey));
    DosuConfig::topPlaysDatabaseFilePath = std::filesystem::path(configDataJson.at(k_topPlaysDbFilePathKey));
    DosuConfig::botConfigDatabaseFilePath = std::filesystem::path(configDataJson.at(k_botConfigDbFilePathKey));
}

/**
 * Create config from user input.
 */
void DosuConfig::setupConfig(std::filesystem::path const& filePath, std::atomic<bool> const& bShutdown)
{
    LOG_DEBUG("Running config setup tool");

    nlohmann::json newConfigJson;
    newConfigJson[k_logLevelKey] = 1;
    newConfigJson[k_logAnsiColorsKey] = false;
    newConfigJson[k_scrapeRankingsRunHourKey] = utcToLocal(3);
    newConfigJson[k_topPlaysRunHourKey] = utcToLocal(1);

    std::string botToken;
    std::string clientID;
    std::string clientSecret;

    std::cout << "      _       _ _                 _                 " << std::endl;
    std::cout << "   __| | __ _(_) |_   _        __| | ___  ___ _   _ " << std::endl;
    std::cout << "  / _` |/ _` | | | | | |_____ / _` |/ _ \\/ __| | | |" << std::endl;
    std::cout << " | (_| | (_| | | | |_| |_____| (_| | (_) \\__ \\ |_| |" << std::endl;
    std::cout << "  \\__,_|\\__,_|_|_|\\__, |      \\__,_|\\___/|___/\\__,_|" << std::endl;
    std::cout << "                  |___/                             " << std::endl;

    std::cout << "In order to run this bot, you will need to register a Discord application and an osu!API client." << std::endl;

    std::cout << "Enter Discord bot token: ";
    std::getline(std::cin, botToken);
    if (bShutdown.load()) return;

    std::cout << "Enter osu!API client ID: ";
    std::getline(std::cin, clientID);
    if (bShutdown.load()) return;

    std::cout << "Enter osu!API client secret: ";
    std::getline(std::cin, clientSecret);
    if (bShutdown.load()) return;

    newConfigJson[k_discordBotTokenKey] = botToken;
    newConfigJson[k_osuClientIdKey] = clientID;
    newConfigJson[k_osuClientSecretKey] = clientSecret;
    newConfigJson[k_rankingsDbFilePathKey] = k_dataDir / "rankings.db";
    newConfigJson[k_topPlaysDbFilePathKey] = k_dataDir / "top_plays.db";
    newConfigJson[k_botConfigDbFilePathKey] = k_dataDir / "bot_config.db";
    newConfigJson[k_threadCountKey] = static_cast<int>(std::thread::hardware_concurrency());

    nlohmann::json defaultDiscordBotStrings;
    defaultDiscordBotStrings[k_letterRankXKey]  = "X";
    defaultDiscordBotStrings[k_letterRankXHKey] = "XH";
    defaultDiscordBotStrings[k_letterRankSKey]  = "S";
    defaultDiscordBotStrings[k_letterRankSHKey] = "SH";
    defaultDiscordBotStrings[k_letterRankDKey]  = "D";
    defaultDiscordBotStrings[k_letterRankCKey]  = "C";
    defaultDiscordBotStrings[k_letterRankBKey]  = "B";
    defaultDiscordBotStrings[k_letterRankAKey]  = "A";
    defaultDiscordBotStrings[k_modMRKey] = "MR";
    defaultDiscordBotStrings[k_modTPKey] = "TP";
    defaultDiscordBotStrings[k_modSDKey] = "SD";
    defaultDiscordBotStrings[k_modSOKey] = "SO";
    defaultDiscordBotStrings[k_modAPKey] = "AP";
    defaultDiscordBotStrings[k_modRXKey] = "RX";
    defaultDiscordBotStrings[k_modRDKey] = "RD";
    defaultDiscordBotStrings[k_modPFKey] = "PF";
    defaultDiscordBotStrings[k_modNFKey] = "NF";
    defaultDiscordBotStrings[k_modNCKey] = "NC";
    defaultDiscordBotStrings[k_modCPKey] = "CP";
    defaultDiscordBotStrings[k_mod9KKey] = "9K";
    defaultDiscordBotStrings[k_mod8KKey] = "8K";
    defaultDiscordBotStrings[k_mod7KKey] = "7K";
    defaultDiscordBotStrings[k_mod6KKey] = "6K";
    defaultDiscordBotStrings[k_mod5KKey] = "5K";
    defaultDiscordBotStrings[k_mod4KKey] = "4K";
    defaultDiscordBotStrings[k_mod3KKey] = "3K";
    defaultDiscordBotStrings[k_mod2KKey] = "2K";
    defaultDiscordBotStrings[k_mod1KKey] = "1K";
    defaultDiscordBotStrings[k_modHDKey] = "HD";
    defaultDiscordBotStrings[k_modHRKey] = "HR";
    defaultDiscordBotStrings[k_modHTKey] = "HT";
    defaultDiscordBotStrings[k_modFLKey] = "FL";
    defaultDiscordBotStrings[k_modFIKey] = "FI";
    defaultDiscordBotStrings[k_modEZKey] = "EZ";
    defaultDiscordBotStrings[k_modDTKey] = "DT";
    defaultDiscordBotStrings[k_modCMKey] = "CM";
    defaultDiscordBotStrings[k_modATKey] = "AT";
    newConfigJson[k_discordBotStringsKey] = defaultDiscordBotStrings;

    std::ofstream outputFile(filePath);
    outputFile << newConfigJson.dump(4) << std::endl;

    std::cout << "Success! Some values have been set by default. Config can be found at " << filePath << std::endl;
}
