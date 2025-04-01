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
        try
        {
            m_database->exec("COMMIT");
        }
        catch(const SQLite::Exception& e)
        {}
    }

    m_database.reset();
    m_bIsInitialized = false;
}

/**
 * Get channelIDs.
 */
std::vector<dpp::snowflake> BotConfigDatabaseManager::getSubscribedChannels(std::string const& newsletterPage)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Retrieving IDs of channels subscribed to ", newsletterPage);

    std::vector<dpp::snowflake> channelIDs;
    std::string table = k_newsletterToTableMap.at(newsletterPage);
    SQLite::Statement query(*m_database, "SELECT channelID FROM BotConfig WHERE " + table + " = 1");

    while (query.executeStep())
    {
        int64_t channelID = query.getColumn(0).getInt64();
        LOG_ERROR_THROW(
            (channelID >= 0),
            "Invalid negative channel ID in database = ", channelID
        );
        channelIDs.push_back(static_cast<dpp::snowflake>(static_cast<uint64_t>(channelID)));
    }

    return channelIDs;
}

/**
 * Add channel subscription.
 */
void BotConfigDatabaseManager::addSubscription(dpp::snowflake const& channelID, std::string const& newsletterPage)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Adding subscription for channel with ID ", channelID.operator uint64_t(), " to ", newsletterPage);

    if (channelExists_(channelID))
    {
        std::string table = k_newsletterToTableMap.at(newsletterPage);

        SQLite::Statement updateQuery(*m_database, "UPDATE BotConfig SET " + table + " = 1 WHERE channelID = ?");
        updateQuery.bind(1, static_cast<int64_t>(channelID.operator uint64_t()));

        updateQuery.exec();
    }
    else
    {
        std::string scrapeRankingsTable = k_newsletterToTableMap.at(k_newsletterPageOptionScrapeRankings.second);
        std::string getTopPlaysTable = k_newsletterToTableMap.at(k_newsletterPageOptionTopPlays.second);

        SQLite::Statement insertQuery(*m_database,
            "INSERT INTO BotConfig "
            "(channelID, " + scrapeRankingsTable + ", " + getTopPlaysTable + ") "
            "VALUES (?, ?, ?)"
        );
        insertQuery.bind(1, static_cast<int64_t>(channelID.operator uint64_t()));
        insertQuery.bind(2, (newsletterPage == k_newsletterPageOptionScrapeRankings.second ? 1 : 0));
        insertQuery.bind(3, (newsletterPage == k_newsletterPageOptionTopPlays.second ? 1 : 0));

        insertQuery.exec();
    }
}

/**
 * Remove channel subscription.
 */
void BotConfigDatabaseManager::removeSubscription(dpp::snowflake const& channelID, std::string const& newsletterPage)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Removing channel with ID ", channelID.operator uint64_t(), "'s subscription from ", newsletterPage);

    std::string table = k_newsletterToTableMap.at(newsletterPage);
    SQLite::Statement query(*m_database, "UPDATE BotConfig SET " + table + " = 0 WHERE channelID = ?");
    query.bind(1, static_cast<int64_t>(channelID.operator uint64_t()));
    query.exec();
}

/**
 * Check if channel exists.
 */
bool BotConfigDatabaseManager::isChannelSubscribed(dpp::snowflake const& channelID, std::string const& newsletterPage)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Checking if channel with ID ", channelID.operator uint64_t(), " is subscribed to ", newsletterPage);

    std::string table = k_newsletterToTableMap.at(newsletterPage);
    SQLite::Statement query(*m_database,
        "SELECT COUNT(*) "
        "FROM BotConfig "
        "WHERE channelID = ? AND " + table + " = 1"
    );
    query.bind(1, static_cast<int64_t>(channelID.operator uint64_t()));
    return (query.executeStep() && query.getColumn(0).getInt64() > 0);
}

/**
 * WARNING: This function is not thread-safe!
 * Check if channel exists.
 */
bool BotConfigDatabaseManager::channelExists_(dpp::snowflake const& channelID)
{
    LOG_DEBUG("Checking if channel exists with ID ", channelID.operator uint64_t());

    SQLite::Statement query(*m_database, "SELECT COUNT(*) FROM BotConfig WHERE channelID = ?");
    query.bind(1, static_cast<int64_t>(channelID.operator uint64_t()));
    return (query.executeStep() && query.getColumn(0).getInt64() > 0);
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
    m_database->exec(
        "CREATE TABLE IF NOT EXISTS BotConfig ("
        "channelID INTEGER PRIMARY KEY, " +
        k_newsletterToTableMap.at(k_newsletterPageOptionScrapeRankings.second) + " INTEGER NOT NULL, " +
        k_newsletterToTableMap.at(k_newsletterPageOptionTopPlays.second) + " INTEGER NOT NULL"
        ")"
    );
}
