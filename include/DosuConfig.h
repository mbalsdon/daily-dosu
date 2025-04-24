#ifndef __DOSU_CONFIG_H__
#define __DOSU_CONFIG_H__

#include <string>
#include <filesystem>
#include <map>

const std::string k_logLevelKey               = "LOG_LEVEL";
const std::string k_logAnsiColorsKey          = "LOG_ANSI_COLORS";
const std::string k_discordBotTokenKey        = "DISCORD_BOT_TOKEN";
const std::string k_osuClientIdKey            = "OSU_CLIENT_ID";
const std::string k_osuClientSecretKey        = "OSU_CLIENT_SECRET";
const std::string k_scrapeRankingsRunHourKey  = "SCRAPE_RANKINGS_RUN_HOUR";
const std::string k_topPlaysRunHourKey        = "TOP_PLAYS_RUN_HOUR";
const std::string k_rankingsDbFilePathKey     = "RANKINGS_DB_FILE_PATH";
const std::string k_topPlaysDbFilePathKey     = "TOP_PLAYS_DB_FILE_PATH";
const std::string k_botConfigDbFilePathKey    = "BOT_CONFIG_DB_FILE_PATH";
const std::string k_discordBotStringsKey      = "DISCORD_BOT_STRINGS";

const std::string k_letterRankXKey  = "LETTER_RANK_X";
const std::string k_letterRankXHKey = "LETTER_RANK_XH";
const std::string k_letterRankSKey  = "LETTER_RANK_S";
const std::string k_letterRankSHKey = "LETTER_RANK_SH";
const std::string k_letterRankDKey  = "LETTER_RANK_D";
const std::string k_letterRankCKey  = "LETTER_RANK_C";
const std::string k_letterRankBKey  = "LETTER_RANK_B";
const std::string k_letterRankAKey  = "LETTER_RANK_A";

const std::string k_modMRKey = "MOD_MR";
const std::string k_modTPKey = "MOD_TP";
const std::string k_modSDKey = "MOD_SD";
const std::string k_modSOKey = "MOD_SO";
const std::string k_modAPKey = "MOD_AP";
const std::string k_modRXKey = "MOD_RX";
const std::string k_modRDKey = "MOD_RD";
const std::string k_modPFKey = "MOD_PF";
const std::string k_modNFKey = "MOD_NF";
const std::string k_modNCKey = "MOD_NC";
const std::string k_modCPKey = "MOD_CP";
const std::string k_mod9KKey = "MOD_9K";
const std::string k_mod8KKey = "MOD_8K";
const std::string k_mod7KKey = "MOD_7K";
const std::string k_mod6KKey = "MOD_6K";
const std::string k_mod5KKey = "MOD_5K";
const std::string k_mod4KKey = "MOD_4K";
const std::string k_mod3KKey = "MOD_3K";
const std::string k_mod2KKey = "MOD_2K";
const std::string k_mod1KKey = "MOD_1K";
const std::string k_modHDKey = "MOD_HD";
const std::string k_modHRKey = "MOD_HR";
const std::string k_modHTKey = "MOD_HT";
const std::string k_modFLKey = "MOD_FL";
const std::string k_modFIKey = "MOD_FI";
const std::string k_modEZKey = "MOD_EZ";
const std::string k_modDTKey = "MOD_DT";
const std::string k_modCMKey = "MOD_CM";
const std::string k_modATKey = "MOD_AT";

/**
 * Environment variable handler. Loads into memory to save a few disk reads.
 */
class DosuConfig
{
public:
    static void load(std::filesystem::path const& filePath);
    static void setupConfig(std::filesystem::path const& filePath);

    static int logLevel;
    static bool logAnsiColors;
    static std::string discordBotToken;
    static std::string osuClientID;
    static std::string osuClientSecret;
    static int scrapeRankingsRunHour;
    static int topPlaysRunHour;
    static std::filesystem::path rankingsDatabaseFilePath;
    static std::filesystem::path topPlaysDatabaseFilePath;
    static std::filesystem::path botConfigDatabaseFilePath;
    static std::map<std::string, std::string> discordBotStrings;
};

#endif /* __DOSU_CONFIG_H__ */
