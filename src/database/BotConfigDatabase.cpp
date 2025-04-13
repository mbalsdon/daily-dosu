#include "BotConfigDatabase.h"
#include "Logger.h"

#include <cstdint>

/**
 * BotConfigDatabase constructor.
 */
BotConfigDatabase::BotConfigDatabase(std::filesystem::path const& dbFilePath)
: m_dbFilePath(dbFilePath)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Opening database connection for bot config; dbFilePath=", dbFilePath.string());
    m_pDatabase = std::make_unique<SQLite::Database>(m_dbFilePath.string(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    createTables_();
}

/**
 * BotConfigDatabase destructor.
 */
BotConfigDatabase::~BotConfigDatabase()
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Closing database connection for bot config");
    if (m_pDatabase->getTotalChanges() > 0)
    {
        try
        {
            m_pDatabase->exec("COMMIT");
        }
        catch(SQLite::Exception const& e)
        {}
    }

    m_pDatabase.reset();
}

/**
 * Get channelIDs.
 */
std::vector<dpp::snowflake> BotConfigDatabase::getSubscribedChannels(std::string const& newsletterPage)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Retrieving IDs of channels subscribed to ", newsletterPage);

    std::vector<dpp::snowflake> channelIDs;
    std::string table = k_newsletterToTableMap.at(newsletterPage);
    SQLite::Statement query(*m_pDatabase, "SELECT channelID FROM BotConfig WHERE " + table + " = 1");

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
void BotConfigDatabase::addSubscription(dpp::snowflake const& channelID, std::string const& newsletterPage)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Adding subscription for channel with ID ", channelID.operator uint64_t(), " to ", newsletterPage);

    if (channelExists_(channelID))
    {
        std::string table = k_newsletterToTableMap.at(newsletterPage);

        SQLite::Statement updateQuery(*m_pDatabase, "UPDATE BotConfig SET " + table + " = 1 WHERE channelID = ?");
        updateQuery.bind(1, static_cast<int64_t>(channelID.operator uint64_t()));

        updateQuery.exec();
    }
    else
    {
        std::string scrapeRankingsTable = k_newsletterToTableMap.at(k_newsletterPageOptionScrapeRankings.second);
        std::string getTopPlaysTable = k_newsletterToTableMap.at(k_newsletterPageOptionTopPlays.second);

        SQLite::Statement insertQuery(*m_pDatabase,
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
void BotConfigDatabase::removeSubscription(dpp::snowflake const& channelID, std::string const& newsletterPage)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Removing channel with ID ", channelID.operator uint64_t(), "'s subscription from ", newsletterPage);

    std::string table = k_newsletterToTableMap.at(newsletterPage);
    SQLite::Statement query(*m_pDatabase, "UPDATE BotConfig SET " + table + " = 0 WHERE channelID = ?");
    query.bind(1, static_cast<int64_t>(channelID.operator uint64_t()));
    query.exec();
}

/**
 * Check if channel exists.
 */
[[nodiscard]] bool BotConfigDatabase::isChannelSubscribed(dpp::snowflake const& channelID, std::string const& newsletterPage)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Checking if channel with ID ", channelID.operator uint64_t(), " is subscribed to ", newsletterPage);

    std::string table = k_newsletterToTableMap.at(newsletterPage);
    SQLite::Statement query(*m_pDatabase,
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
[[nodiscard]] bool BotConfigDatabase::channelExists_(dpp::snowflake const& channelID)
{
    LOG_DEBUG("Checking if channel exists with ID ", channelID.operator uint64_t());

    SQLite::Statement query(*m_pDatabase, "SELECT COUNT(*) FROM BotConfig WHERE channelID = ?");
    query.bind(1, static_cast<int64_t>(channelID.operator uint64_t()));
    return (query.executeStep() && query.getColumn(0).getInt64() > 0);
}

/**
 * Create database tables if they don't exist.
 */
void BotConfigDatabase::createTables_()
{
    LOG_DEBUG("Creating tables");

    /* Discord IDs (snowflakes) are uint64 but SQLiteCpp's largest native type is int64.
    So this would not be a good idea, however the first 41 bits of a (Discord) snowflake
    represent milliseconds since 01 Jan 2015). Thus a few decades will have to pass before
    this becomes problematic. */
    m_pDatabase->exec(
        "CREATE TABLE IF NOT EXISTS BotConfig ("
        "channelID INTEGER PRIMARY KEY, " +
        k_newsletterToTableMap.at(k_newsletterPageOptionScrapeRankings.second) + " INTEGER NOT NULL, " +
        k_newsletterToTableMap.at(k_newsletterPageOptionTopPlays.second) + " INTEGER NOT NULL"
        ")"
    );
}
