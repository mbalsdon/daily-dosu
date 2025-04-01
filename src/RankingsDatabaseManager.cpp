#include "RankingsDatabaseManager.h"
#include "Logger.h"

/**
 * Initialize RankingsDatabaseManager instance.
 */
void RankingsDatabaseManager::initialize(const std::filesystem::path dbFilePath)
{

    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_INFO("Initializing RankingsDatabaseManager (dbFilePath=", dbFilePath.string(), ")...");

    if (m_bIsInitialized)
    {
        LOG_WARN("RankingsDatabaseManager is already initialized!");
        return;
    }

    m_dbFilePath = dbFilePath;
    m_database = std::make_unique<SQLite::Database>(dbFilePath.string(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

    createTables();
    m_bIsInitialized = true;
}

/**
 * Clean up database resources.
 */
void RankingsDatabaseManager::cleanup()
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_INFO("Cleaning up RankingsDatabaseManager (m_dbFilePath=", m_dbFilePath.string(), ")...");

    if (!m_bIsInitialized || !m_database)
    {
        LOG_WARN("RankingsDatabaseManager is not initialized!");
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
 * Return time of last write to database.
 * NOTE: The whole database, not a specific table!
 */
std::filesystem::file_time_type RankingsDatabaseManager::lastWriteTime()
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Getting time of last write to database...");

    return std::filesystem::last_write_time(m_dbFilePath);
}

/**
 * Delete all entries in tables.
 */
void RankingsDatabaseManager::wipeTables()
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Wiping tables...");

    try
    {
        SQLite::Transaction txn(*m_database);

        for (const auto& [_, table] : k_modeToRankingsTable)
        {
            m_database->exec("DELETE FROM " + table);
        }

        txn.commit();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Failed to wipe tables; ", e.what());
        throw;
    }
}

/**
 * Clear yesterday's ranks and move current ranks into their place.
 */
void RankingsDatabaseManager::shiftRanks(Gamemode mode)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Shifting ranks for ", mode.toString(), "...");

    const std::string table = k_modeToRankingsTable.at(mode);
    const std::string query =
        "UPDATE " + table + " "
        "SET yesterdayRank = currentRank, "
        "currentRank = NULL;";

    m_database->exec(query);
}

/**
 * Perform batch insert of users.
 */
void RankingsDatabaseManager::insertRankingsUsers(std::vector<RankingsUser>& rankingsUsers, Gamemode mode)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Inserting ", rankingsUsers.size(), " rankings users into " + mode.toString() + "...");

    const std::string table = k_modeToRankingsTable.at(mode);

    SQLite::Transaction txn(*m_database);
    SQLite::Statement query(*m_database,
        "INSERT OR REPLACE INTO " + table + " "
        "(userID, username, countryCode, pfpLink, performancePoints, accuracy, hoursPlayed, currentRank, yesterdayRank) "
        "SELECT ?, ?, ?, ?, ?, ?, ?, ?, "
        "   (SELECT yesterdayRank FROM " + table + " WHERE userID = ?)"
    );

    for (const auto& rankingsUser : rankingsUsers)
    {
        if (rankingsUser.isValid())
        {
            query.reset();
            query.bind(1, rankingsUser.userID);
            query.bind(2, rankingsUser.username);
            query.bind(3, rankingsUser.countryCode);
            query.bind(4, rankingsUser.pfpLink);
            query.bind(5, rankingsUser.performancePoints);
            query.bind(6, rankingsUser.accuracy);
            query.bind(7, rankingsUser.hoursPlayed);
            query.bind(8, rankingsUser.currentRank);
            query.bind(9, rankingsUser.userID);
            query.exec();
        }
    }

    txn.commit();
}

/**
 * Remove users with NULL currentRank.
 */
void RankingsDatabaseManager::deleteUsersWithNullCurrentRank(Gamemode mode)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Removing users with NULL current rank from " + mode.toString() + "...");

    const std::string table = k_modeToRankingsTable.at(mode);

    m_database->exec("DELETE FROM " + table + " WHERE currentRank IS NULL");
}

/**
 * Get userIDs with NULL yesterdayRank.
 */
std::vector<UserID> RankingsDatabaseManager::getUserIDsWithNullYesterdayRank(Gamemode mode)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Finding users with NULL yesterday rank from " + mode.toString() + "...");

    const std::string table = k_modeToRankingsTable.at(mode);

    std::vector<UserID> userIDs;
    SQLite::Statement query(*m_database, "SELECT userID FROM " + table + " WHERE yesterdayRank IS NULL");
    while (query.executeStep())
    {
        userIDs.push_back(query.getColumn(0).getInt64());
    }

    return userIDs;
}

/**
 * Update yesterdayRank values.
 */
void RankingsDatabaseManager::updateYesterdayRanks(std::vector<std::pair<UserID, Rank>>& userYesterdayRanks, Gamemode mode)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Updating yesterday ranks of ", userYesterdayRanks.size(), " users from " + mode.toString() + "...");

    const std::string table = k_modeToRankingsTable.at(mode);

    SQLite::Transaction txn(*m_database);
    SQLite::Statement query(*m_database,
        "UPDATE " + table + " "
        "SET yesterdayRank = ? "
        "WHERE userID = ?"
    );

    for (const auto& userYesterdayRank : userYesterdayRanks)
    {
        query.reset();
        query.bind(1, userYesterdayRank.second);
        query.bind(2, userYesterdayRank.first);
        query.exec();
    }

    txn.commit();
}

/**
 * Return true if database has an empty table, false otherwise.
 */
bool RankingsDatabaseManager::hasEmptyTable()
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Checking if database has empty tables...");

    for (const auto& [_, table] : k_modeToRankingsTable)
    {
        SQLite::Statement query(*m_database, "SELECT NOT EXISTS (SELECT 1 FROM " + table + " LIMIT 1)");

        if (query.executeStep() && query.getColumn(0).getInt64())
        {
            return true;
        }
    }

    return false;
}

/**
 * Get top users sorted by relative rank improvement.
 * Entering "GLOBAL" for countryCode means no filter.
 */
std::vector<RankImprovement> RankingsDatabaseManager::getTopRankImprovements(std::string countryCode, int64_t minRank, int64_t maxRank, std::size_t numUsers, Gamemode mode)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Retrieving top users by rank improvement from " + mode.toString() + "...");

    std::string table = k_modeToRankingsTable.at(mode);

    std::vector<RankImprovement> results;
    SQLite::Statement query(*m_database,
        "SELECT "
        "   userID, "
        "   username, "
        "   countryCode, "
        "   pfpLink, "
        "   performancePoints, "
        "   accuracy, "
        "   hoursPlayed, "
        "   yesterdayRank, "
        "   currentRank, "
        "   CAST(yesterdayRank - currentRank AS FLOAT) / currentRank AS relative_improvement "
        "FROM " + table + " "
        "WHERE "
        "   currentRank IS NOT NULL "
        "   AND yesterdayRank IS NOT NULL "
        "   AND currentRank != 0 "
        "   AND currentRank >= ? "
        "   AND currentRank <= ? "
        "   AND yesterdayRank > currentRank "
        "   AND (countryCode = ? OR ? = '" + k_global + "') "
        "ORDER BY (CAST(yesterdayRank - currentRank AS FLOAT) / currentRank) DESC "
        "LIMIT ?"
    );

    query.bind(1, minRank);
    query.bind(2, maxRank);
    query.bind(3, countryCode);
    query.bind(4, countryCode);
    query.bind(5, static_cast<int64_t>(numUsers));

    while (query.executeStep())
    {
        RankImprovement rankImprovement;
        rankImprovement.user.userID = query.getColumn(0).getInt64();
        rankImprovement.user.username = query.getColumn(1).getString();
        rankImprovement.user.countryCode = query.getColumn(2).getString();
        rankImprovement.user.pfpLink = query.getColumn(3).getString();
        rankImprovement.user.performancePoints = query.getColumn(4).getDouble();
        rankImprovement.user.accuracy = query.getColumn(5).getDouble();
        rankImprovement.user.hoursPlayed = query.getColumn(6).getInt64();
        rankImprovement.user.yesterdayRank = query.getColumn(7).getInt64();
        rankImprovement.user.currentRank = query.getColumn(8).getInt64();
        rankImprovement.relativeImprovement = query.getColumn(9).getDouble();

        results.push_back(rankImprovement);
    }

    return results;
}

/**
 * Get bottom users sorted by relative rank improvement.
 * Entering "GLOBAL" for countryCode means no filter.
 */
std::vector<RankImprovement> RankingsDatabaseManager::getBottomRankImprovements(std::string countryCode, int64_t minRank, int64_t maxRank, std::size_t numUsers, Gamemode mode)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Retrieving bottom users by rank improvement from " + mode.toString() + "...");

    std::string table = k_modeToRankingsTable.at(mode);

    std::vector<RankImprovement> results;
    SQLite::Statement query(*m_database,
        "SELECT "
        "   userID, "
        "   username, "
        "   countryCode, "
        "   pfpLink, "
        "   performancePoints, "
        "   accuracy, "
        "   hoursPlayed, "
        "   yesterdayRank, "
        "   currentRank, "
        "   CAST(currentRank - yesterdayRank AS FLOAT) / currentRank AS relative_improvement "
        "FROM " + table + " "
        "WHERE "
        "   currentRank IS NOT NULL "
        "   AND yesterdayRank IS NOT NULL "
        "   AND currentRank != 0 "
        "   AND currentRank >= ? "
        "   AND currentRank <= ? "
        "   AND yesterdayRank < currentRank "
        "   AND (countryCode = ? OR ? = '" + k_global + "') "
        "ORDER BY (CAST(currentRank - yesterdayRank AS FLOAT) / currentRank) DESC "
        "LIMIT ?"
    );

    query.bind(1, minRank);
    query.bind(2, maxRank);
    query.bind(3, countryCode);
    query.bind(4, countryCode);
    query.bind(5, static_cast<int64_t>(numUsers));

    while (query.executeStep())
    {
        RankImprovement rankImprovement;
        rankImprovement.user.userID = query.getColumn(0).getInt64();
        rankImprovement.user.username = query.getColumn(1).getString();
        rankImprovement.user.countryCode = query.getColumn(2).getString();
        rankImprovement.user.pfpLink = query.getColumn(3).getString();
        rankImprovement.user.performancePoints = query.getColumn(4).getDouble();
        rankImprovement.user.accuracy = query.getColumn(5).getDouble();
        rankImprovement.user.hoursPlayed = query.getColumn(6).getInt64();
        rankImprovement.user.yesterdayRank = query.getColumn(7).getInt64();
        rankImprovement.user.currentRank = query.getColumn(8).getInt64();
        rankImprovement.relativeImprovement = query.getColumn(9).getDouble();

        results.push_back(rankImprovement);
    }

    return results;
}

/**
 * RankingsDatabaseManager destructor.
 */
RankingsDatabaseManager::~RankingsDatabaseManager()
{
    LOG_DEBUG("Destructing RankingsDatabaseManager instance...");
    cleanup();
}

/**
 * Create database tables if they don't exist.
 * Does not use a mutex.
 */
void RankingsDatabaseManager::createTables()
{
    LOG_DEBUG("Creating tables...");

    try
    {
        SQLite::Transaction txn(*m_database);

        for (const auto& [_, table] : k_modeToRankingsTable)
        {
            m_database->exec(
                "CREATE TABLE IF NOT EXISTS " + table + " ("
                "   userID            INTEGER  PRIMARY KEY,     "
                "   username          TEXT     NOT NULL UNIQUE, "
                "   countryCode       TEXT     NOT NULL,        "
                "   pfpLink           TEXT     NOT NULL,        "
                "   performancePoints REAL     NOT NULL,        "
                "   accuracy          REAL     NOT NULL,        "
                "   hoursPlayed       INTEGER  NOT NULL,        "
                "   yesterdayRank     INTEGER,                  "
                "   currentRank       INTEGER                   "
                ")"
            );
        }

        txn.commit();
    }
    catch(const std::exception& e)
    {
        LOG_ERROR("Failed to create tables; ", e.what());
        throw;
    }
}
