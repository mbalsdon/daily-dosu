#ifndef __BOT_CONFIG_DATABASE_MANAGER__
#define __BOT_CONFIG_DATABASE_MANAGER__

#include <Util.h>

#include <SQLiteCpp/SQLiteCpp.h>
#include <dpp/dpp.h>

#include <filesystem>
#include <optional>
#include <mutex>
#include <vector>
#include <atomic>
#include <unordered_map>

const std::unordered_map<std::string, std::string> k_newsletterToTableMap = {
    { k_newsletterPageOptionScrapeRankings.second, "scrapeRankingsSub" },
    { k_newsletterPageOptionTopPlays.second, "getTopPlaysSub" }
};

/**
 * SQLiteCpp wrapper for bot config tables.
 */
class BotConfigDatabaseManager
{
public:
    static BotConfigDatabaseManager& getInstance()
    {
        static BotConfigDatabaseManager instance;
        return instance;
    }

    void initialize(const std::filesystem::path dbFilePath);
    void cleanup();

    std::vector<dpp::snowflake> getSubscribedChannels(std::string const& newsletterPage);
    void addSubscription(dpp::snowflake const& channelID, std::string const& newsletterPage);
    void removeSubscription(dpp::snowflake const& channelID, std::string const& newsletterPage);
    bool isChannelSubscribed(dpp::snowflake const& channelID, std::string const& newsletterPage);

private:
    BotConfigDatabaseManager()
    : m_database(nullptr)
    , m_bIsInitialized(false)
    {}

    ~BotConfigDatabaseManager();

    BotConfigDatabaseManager(const BotConfigDatabaseManager&) = delete;
    BotConfigDatabaseManager(BotConfigDatabaseManager&&) = delete;
    BotConfigDatabaseManager& operator=(const BotConfigDatabaseManager&) = delete;
    BotConfigDatabaseManager& operator=(BotConfigDatabaseManager&&) = delete;

    void createTables();
    bool channelExists_(dpp::snowflake const& channelID);

    std::unique_ptr<SQLite::Database> m_database;
    std::mutex m_dbMtx;
    std::atomic<bool> m_bIsInitialized;
};

#endif /* __BOT_CONFIG_DATABASE_MANAGER__ */
