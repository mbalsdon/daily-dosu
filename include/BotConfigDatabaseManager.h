#ifndef __BOT_CONFIG_DATABASE_MANAGER__
#define __BOT_CONFIG_DATABASE_MANAGER__

#include <SQLiteCpp/SQLiteCpp.h>
#include <dpp/dpp.h>

#include <filesystem>
#include <optional>
#include <mutex>
#include <vector>
#include <atomic>

/**
 * SQLiteCpp wrapper for the BotConfig table.
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

    std::vector<dpp::snowflake> getChannelIDs();
    void addChannel(dpp::snowflake channelID);
    void removeChannel(dpp::snowflake channelID);
    bool channelExists(dpp::snowflake channelID);

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

    std::unique_ptr<SQLite::Database> m_database;
    std::mutex m_dbMtx;
    std::atomic<bool> m_bIsInitialized;
};

#endif /* __BOT_CONFIG_DATABASE_MANAGER__ */
