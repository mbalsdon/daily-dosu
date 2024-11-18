#ifndef __DOSU_CONFIG_H__
#define __DOSU_CONFIG_H__

#include <string>
#include <filesystem>

const std::string k_logLevelKey = "LOG_LEVEL";
const std::string k_discordBotTokenKey = "DISCORD_BOT_TOKEN";
const std::string k_osuClientIdKey = "OSU_CLIENT_ID";
const std::string k_osuClientSecretKey = "OSU_CLIENT_SECRET";
const std::string k_scrapeRankingsRunHourKey = "SCRAPE_RANKINGS_RUN_HOUR";
const std::string k_rankingsDbFilePathKey = "RANKINGS_DB_FILE_PATH";
const std::string k_botConfigDbFilePathKey = "BOT_CONFIG_DB_FILE_PATH";

/**
 * Environment variable handler. Loads into memory to save a few disk reads.
 */
class DosuConfig 
{
public:
    static void load(std::filesystem::path filePath);
    static void setupConfig(std::filesystem::path filePath);

    static int logLevel;
    static std::string discordBotToken;
    static std::string osuClientID;
    static std::string osuClientSecret;
    static int scrapeRankingsRunHour;
    static std::filesystem::path rankingsDatabaseFilePath;
    static std::filesystem::path botConfigDatabaseFilePath;
};

#endif /* __DOSU_CONFIG_H__ */
