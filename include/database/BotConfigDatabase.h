#ifndef __BOT_CONFIG_DATABASE_MANAGER__
#define __BOT_CONFIG_DATABASE_MANAGER__

#include <Util.h>

#include <SQLiteCpp/SQLiteCpp.h>
#include <dpp/dpp.h>

#include <filesystem>
#include <optional>
#include <mutex>
#include <vector>
#include <unordered_map>

const std::unordered_map<std::string, std::string> k_newsletterToTableMap = {
    { k_newsletterPageOptionScrapeRankings.second, "scrapeRankingsSub" },
    { k_newsletterPageOptionTopPlays.second, "getTopPlaysSub" }
};

/**
 * SQLiteCpp wrapper for bot config tables.
 */
class BotConfigDatabase
{
public:
    BotConfigDatabase(std::filesystem::path const& dbFilePath);
    ~BotConfigDatabase();

    std::vector<dpp::snowflake> getSubscribedChannels(std::string const& newsletterPage);
    void addSubscription(dpp::snowflake const& channelID, std::string const& newsletterPage);
    void removeSubscription(dpp::snowflake const& channelID, std::string const& newsletterPage);
    [[nodiscard]] bool isChannelSubscribed(dpp::snowflake const& channelID, std::string const& newsletterPage);

private:
    [[nodiscard]] bool channelExists_(dpp::snowflake const& channelID);
    void createTables_();

    std::filesystem::path m_dbFilePath;
    std::unique_ptr<SQLite::Database> m_pDatabase;
    std::mutex m_dbMtx;
};

#endif /* __BOT_CONFIG_DATABASE_MANAGER__ */
