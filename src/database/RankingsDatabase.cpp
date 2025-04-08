#include "RankingsDatabase.h"
#include "Logger.h"

/**
 * RankingsDatabase constructor.
 */
RankingsDatabase::RankingsDatabase(std::filesystem::path const& dbFilePath)
: m_dbFilePath(dbFilePath)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Opening database connection for rankings; dbFilePath=", dbFilePath.string());
    m_pDatabase = std::make_unique<SQLite::Database>(dbFilePath.string(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    createTables_();
}

/**
 * RankingsDatabase destructor.
 */
RankingsDatabase::~RankingsDatabase()
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Closing database connection for rankings");
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
 * Return time of last write to database.
 * NOTE: The whole database, not a specific table!
 */
[[nodiscard]] std::filesystem::file_time_type RankingsDatabase::lastWriteTime()
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Getting time of last write to database...");

    return std::filesystem::last_write_time(m_dbFilePath);
}

/**
 * Delete all entries in tables.
 */
void RankingsDatabase::wipeTables()
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Wiping tables...");

    try
    {
        SQLite::Transaction txn(*m_pDatabase);

        for (auto const& [_, table] : k_modeToRankingsTable)
        {
            m_pDatabase->exec("DELETE FROM " + table);
        }

        txn.commit();
    }
    catch (std::exception const& e)
    {
        LOG_ERROR("Failed to wipe tables; ", e.what());
        throw;
    }
}

/**
 * Clear yesterday's ranks and move current ranks into their place.
 */
void RankingsDatabase::shiftRanks(Gamemode const& mode)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Shifting ranks for ", mode.toString(), "...");

    const std::string table = k_modeToRankingsTable.at(mode);
    const std::string query =
        "UPDATE " + table + " "
        "SET yesterdayRank = currentRank, "
        "currentRank = NULL;";

    m_pDatabase->exec(query);
}

/**
 * Perform batch insert of users.
 */
void RankingsDatabase::insertRankingsUsers(std::vector<RankingsUser> const& rankingsUsers, Gamemode const& mode)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Inserting ", rankingsUsers.size(), " rankings users into " + mode.toString() + "...");

    const std::string table = k_modeToRankingsTable.at(mode);

    SQLite::Transaction txn(*m_pDatabase);
    SQLite::Statement query(*m_pDatabase,
        "INSERT OR REPLACE INTO " + table + " "
        "(userID, username, countryCode, pfpLink, performancePoints, accuracy, hoursPlayed, currentRank, yesterdayRank) "
        "SELECT ?, ?, ?, ?, ?, ?, ?, ?, "
        "   (SELECT yesterdayRank FROM " + table + " WHERE userID = ?)"
    );

    for (auto const& rankingsUser : rankingsUsers)
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
void RankingsDatabase::deleteUsersWithNullCurrentRank(Gamemode const& mode)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Removing users with NULL current rank from " + mode.toString() + "...");

    const std::string table = k_modeToRankingsTable.at(mode);

    m_pDatabase->exec("DELETE FROM " + table + " WHERE currentRank IS NULL");
}

/**
 * Get userIDs with NULL yesterdayRank.
 */
[[nodiscard]] std::vector<UserID> RankingsDatabase::getUserIDsWithNullYesterdayRank(Gamemode const& mode)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Finding users with NULL yesterday rank from " + mode.toString() + "...");

    const std::string table = k_modeToRankingsTable.at(mode);

    std::vector<UserID> userIDs;
    SQLite::Statement query(*m_pDatabase, "SELECT userID FROM " + table + " WHERE yesterdayRank IS NULL");
    while (query.executeStep())
    {
        userIDs.push_back(query.getColumn(0).getInt64());
    }

    return userIDs;
}

/**
 * Update yesterdayRank values.
 */
void RankingsDatabase::updateYesterdayRanks(std::vector<std::pair<UserID, Rank>> const& userYesterdayRanks, Gamemode const& mode)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Updating yesterday ranks of ", userYesterdayRanks.size(), " users from " + mode.toString() + "...");

    const std::string table = k_modeToRankingsTable.at(mode);

    SQLite::Transaction txn(*m_pDatabase);
    SQLite::Statement query(*m_pDatabase,
        "UPDATE " + table + " "
        "SET yesterdayRank = ? "
        "WHERE userID = ?"
    );

    for (auto const& userYesterdayRank : userYesterdayRanks)
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
[[nodiscard]] bool RankingsDatabase::hasEmptyTable()
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Checking if database has empty tables...");

    for (auto const& [_, table] : k_modeToRankingsTable)
    {
        SQLite::Statement query(*m_pDatabase, "SELECT NOT EXISTS (SELECT 1 FROM " + table + " LIMIT 1)");

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
[[nodiscard]] std::vector<RankImprovement> RankingsDatabase::getTopRankImprovements(
    std::string const& countryCode,
    int64_t const& minRank,
    int64_t const& maxRank,
    std::size_t const& numUsers,
    Gamemode const& mode)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Retrieving top users by rank improvement from " + mode.toString() + "...");

    std::string table = k_modeToRankingsTable.at(mode);

    std::vector<RankImprovement> results;
    SQLite::Statement query(*m_pDatabase,
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
[[nodiscard]] std::vector<RankImprovement> RankingsDatabase::getBottomRankImprovements(
    std::string const& countryCode,
    int64_t const& minRank,
    int64_t const& maxRank,
    std::size_t const& numUsers,
    Gamemode const& mode)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Retrieving bottom users by rank improvement from " + mode.toString() + "...");

    std::string table = k_modeToRankingsTable.at(mode);

    std::vector<RankImprovement> results;
    SQLite::Statement query(*m_pDatabase,
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
 * Create database tables if they don't exist.
 * Does not use a mutex.
 */
void RankingsDatabase::createTables_()
{
    LOG_DEBUG("Creating tables...");

    try
    {
        SQLite::Transaction txn(*m_pDatabase);

        for (auto const& [_, table] : k_modeToRankingsTable)
        {
            m_pDatabase->exec(
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
    catch (std::exception const& e)
    {
        LOG_ERROR("Failed to create tables; ", e.what());
        throw;
    }
}
