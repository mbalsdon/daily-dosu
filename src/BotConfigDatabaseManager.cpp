#include "BotConfigDatabaseManager.h"
#include "Logger.h"

#include <cstdint>

/**
 * Initialize BotConfigDatabaseManager instance.
 */
void BotConfigDatabaseManager::initialize(const std::filesystem::path dbFilePath)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_INFO("Initializing BotConfigDatabaseManager (dbFilePath=", dbFilePath.string(), ")...");

    if (m_bIsInitialized)
    {
        LOG_WARN("BotConfigDatabaseManager is already initialized!");
        return;
    }

    m_database = std::make_unique<SQLite::Database>(dbFilePath.string(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

    createTables();
    m_bIsInitialized = true;
}

/**
 * Clean up database resources.
 */
void BotConfigDatabaseManager::cleanup()
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_INFO("Cleaning up BotConfigDatabaseManager...");

    if (!m_bIsInitialized || !m_database)
    {
        LOG_WARN("BotConfigDatabaseManager is not initialized!");
        return;
    }

    if (m_database->getTotalChanges() > 0)
    {
        m_database->exec("COMMIT");
    }

    m_database.reset();
    m_bIsInitialized = false;
}

/**
 * Get channelIDs.
 */
std::vector<dpp::snowflake> BotConfigDatabaseManager::getChannelIDs()
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Retrieving channel IDs...");

    std::vector<dpp::snowflake> channelIDs;
    SQLite::Statement query(*m_database, "SELECT channelID FROM BotConfig");
    while (query.executeStep())
    {
        int64_t channelID = query.getColumn(0).getInt64();
        if (channelID < 0)
        {
            std::string reason = std::string("Invalid negative channel ID in database = ").append(std::to_string(channelID));
            throw std::runtime_error(reason);
        }

        channelIDs.push_back(static_cast<dpp::snowflake>(static_cast<uint64_t>(channelID))); 
    }

    return channelIDs;
}

/**
 * Add channel.
 */
void BotConfigDatabaseManager::addChannel(dpp::snowflake channelID)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Adding channel with ID=", channelID.operator uint64_t());

    SQLite::Statement query(*m_database, "INSERT OR REPLACE INTO BotConfig (channelID) VALUES (?)");
    query.bind(1, static_cast<int64_t>(channelID.operator uint64_t()));
    query.exec();
}

/**
 * Remove channel.
 */
void BotConfigDatabaseManager::removeChannel(dpp::snowflake channelID)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Removing channel with ID=", channelID.operator uint64_t());

    SQLite::Statement query(*m_database, "DELETE FROM BotConfig WHERE channelID = ?");
    query.bind(1, static_cast<int64_t>(channelID.operator uint64_t()));
    query.exec();
}

/**
 * BotConfigDatabaseManager destructor.
 */
BotConfigDatabaseManager::~BotConfigDatabaseManager()
{
    LOG_DEBUG("Destructing BotConfigDatabaseManager instance...");
    cleanup();
}

/**
 * Create database tables if they don't exist.
 */
void BotConfigDatabaseManager::createTables()
{
    LOG_DEBUG("Creating tables...");

    /* Discord IDs (snowflakes) are uint64 but SQLiteCpp's largest native type is int64.
    So this would not be a good idea, however the first 41 bits of a (Discord) snowflake 
    represent milliseconds since 01 Jan 2015). Thus a few decades will have to pass before
    this becomes problematic. */
    m_database->exec(R"(
        CREATE TABLE IF NOT EXISTS BotConfig (
            channelID INTEGER PRIMARY KEY
        )
    )");
}
