#include "TopPlaysDatabase.h"
#include "Logger.h"

/**
 * TopPlaysDatabase constructor.
 */
TopPlaysDatabase::TopPlaysDatabase(std::filesystem::path const& dbFilePath)
: m_dbFilePath(dbFilePath)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Opening database connection for top plays; dbFilePath=", dbFilePath.string());
    m_pDatabase = std::make_unique<SQLite::Database>(dbFilePath.string(), SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
    createTables_();
}

/**
 * TopPlaysDatabase destructor.
 */
TopPlaysDatabase::~TopPlaysDatabase()
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_INFO("Closing database connection for top plays");
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
[[nodiscard]] std::filesystem::file_time_type TopPlaysDatabase::lastWriteTime()
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Getting time of last write to database...");

    return std::filesystem::last_write_time(m_dbFilePath);
}

/**
 * Delete all entries in tables.
 */
void TopPlaysDatabase::wipeTables()
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Wiping tables...");

    try
    {
        SQLite::Transaction txn(*m_pDatabase);

        for (auto const& [_, table] : k_modeToTopPlaysTable)
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
 * Return true if database has an empty table, false otherwise.
 */
[[nodiscard]] bool TopPlaysDatabase::hasEmptyTable()
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Checking if database has empty tables...");

    for (auto const& [_, table] : k_modeToTopPlaysTable)
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
 * Perform batch insert of top plays.
 */
void TopPlaysDatabase::insertTopPlays(Gamemode const& mode, std::vector<TopPlay> const& topPlays)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Inserting ", topPlays.size(), " top plays into " + mode.toString() + "...");

    const std::string table = k_modeToTopPlaysTable.at(mode);

    SQLite::Transaction txn(*m_pDatabase);
    SQLite::Statement query(*m_pDatabase,
        "INSERT INTO " + table + " "
        "("
        "rank, "
        "scoreID, mods, performancePoints, accuracy, totalScore, createdAt, combo, letterRank, count300, count100, count50, countMiss, "
        "beatmapID, beatmapStarRating, beatmapDifficultyName, beatmapArtist, beatmapTitle, mapsetCreator, beatmapMaxCombo, "
        "userID, username, userCountryCode, userPfpLink, userPerformancePoints, userAccuracy, userHoursPlayed, userCurrentRank"
        ")"
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
    );

    for (auto const& topPlay : topPlays)
    {
        if (topPlay.isValid())
        {
            query.reset();

            query.bind(1, topPlay.rank);

            query.bind(2, topPlay.score.scoreID);
            query.bind(3, topPlay.score.mods.toString());
            query.bind(4, topPlay.score.performancePoints);
            query.bind(5, topPlay.score.accuracy);
            query.bind(6, topPlay.score.totalScore);
            query.bind(7, topPlay.score.createdAt.toString());
            query.bind(8, topPlay.score.combo);
            query.bind(9, topPlay.score.letterRank);
            query.bind(10, topPlay.score.count300);
            query.bind(11, topPlay.score.count100);
            query.bind(12, topPlay.score.count50);
            query.bind(13, topPlay.score.countMiss);

            query.bind(14, topPlay.score.beatmap.beatmapID);
            query.bind(15, topPlay.score.beatmap.starRating);
            query.bind(16, topPlay.score.beatmap.difficultyName);
            query.bind(17, topPlay.score.beatmap.artist);
            query.bind(18, topPlay.score.beatmap.title);
            query.bind(19, topPlay.score.beatmap.mapsetCreator);
            query.bind(20, topPlay.score.beatmap.maxCombo);

            query.bind(21, topPlay.score.user.userID);
            query.bind(22, topPlay.score.user.username);
            query.bind(23, topPlay.score.user.countryCode);
            query.bind(24, topPlay.score.user.pfpLink);
            query.bind(25, topPlay.score.user.performancePoints);
            query.bind(26, topPlay.score.user.accuracy);
            query.bind(27, topPlay.score.user.hoursPlayed);
            query.bind(28, topPlay.score.user.currentRank);

            query.exec();
        }
    }

    txn.commit();
}

[[nodiscard]] std::vector<TopPlay> TopPlaysDatabase::getTopPlays(std::string const& countryCode, std::size_t const& numTopPlays, Gamemode const& mode)
{
    std::lock_guard<std::mutex> lock(m_dbMtx);
    LOG_DEBUG("Retrieving top plays for mode ", mode.toString(), "...");

    std::string table = k_modeToTopPlaysTable.at(mode);

    std::vector<TopPlay> results;
    SQLite::Statement query(*m_pDatabase,
        "SELECT "
        "   rank, "
        "   scoreID, mods, performancePoints, accuracy, totalScore, createdAt, combo, letterRank, count300, count100, count50, countMiss, "
        "   beatmapID, beatmapStarRating, beatmapDifficultyName, beatmapArtist, beatmapTitle, mapsetCreator, beatmapMaxCombo, "
        "   userID, username, userCountryCode, userPfpLink, userPerformancePoints, userAccuracy, userHoursPlayed, userCurrentRank "
        "FROM " + table + " "
        "WHERE (userCountryCode = ? OR ? = '" + k_global + "') "
        "ORDER BY rank ASC "
        "LIMIT ?"
    );

    query.bind(1, countryCode);
    query.bind(2, countryCode);
    query.bind(3, static_cast<int64_t>(numTopPlays));

    while (query.executeStep())
    {
        TopPlay topPlay;

        topPlay.rank = query.getColumn(0).getInt64();

        topPlay.score.scoreID           = query.getColumn(1).getInt64();
        topPlay.score.mods              = OsuMods(query.getColumn(2).getString());
        topPlay.score.performancePoints = query.getColumn(3).getDouble();
        topPlay.score.accuracy          = query.getColumn(4).getDouble();
        topPlay.score.totalScore        = query.getColumn(5).getInt64();
        topPlay.score.createdAt         = ISO8601DateTimeUTC(query.getColumn(6).getString());
        topPlay.score.combo             = query.getColumn(7).getInt64();
        topPlay.score.letterRank        = query.getColumn(8).getString();
        topPlay.score.count300          = query.getColumn(9).getInt64();
        topPlay.score.count100          = query.getColumn(10).getInt64();
        topPlay.score.count50           = query.getColumn(11).getInt64();
        topPlay.score.countMiss         = query.getColumn(12).getInt64();

        topPlay.score.beatmap.beatmapID      = query.getColumn(13).getInt64();
        topPlay.score.beatmap.starRating     = query.getColumn(14).getDouble();
        topPlay.score.beatmap.difficultyName = query.getColumn(15).getString();
        topPlay.score.beatmap.artist         = query.getColumn(16).getString();
        topPlay.score.beatmap.title          = query.getColumn(17).getString();
        topPlay.score.beatmap.mapsetCreator  = query.getColumn(18).getString();
        topPlay.score.beatmap.maxCombo       = query.getColumn(19).getInt64();

        topPlay.score.user.userID            = query.getColumn(20).getInt64();
        topPlay.score.user.username          = query.getColumn(21).getString();
        topPlay.score.user.countryCode       = query.getColumn(22).getString();
        topPlay.score.user.pfpLink           = query.getColumn(23).getString();
        topPlay.score.user.performancePoints = query.getColumn(24).getDouble();
        topPlay.score.user.accuracy          = query.getColumn(25).getDouble();
        topPlay.score.user.hoursPlayed       = query.getColumn(26).getInt64();
        topPlay.score.user.currentRank       = query.getColumn(27).getInt64();

        results.push_back(topPlay);
    }

    return results;
}

/**
 * Create database tables if they don't exist.
 * Does not use a mutex.
 */
void TopPlaysDatabase::createTables_()
{
    LOG_DEBUG("Creating tables...");

    try
    {
        SQLite::Transaction txn(*m_pDatabase);

        for (auto const& [_, table] : k_modeToTopPlaysTable)
        {
            m_pDatabase->exec(
                "CREATE TABLE IF NOT EXISTS " + table + " ("
                "   rank              INTEGER  PRIMARY KEY, "
                "   scoreID           INTEGER  UNIQUE,      "
                "   mods              TEXT,                 "
                "   performancePoints REAL     NOT NULL,    "
                "   accuracy          REAL,                 "
                "   totalScore        INTEGER  NOT NULL,    "
                "   createdAt         TEXT     NOT NULL,    "
                "   combo             INTEGER,              "
                "   letterRank        TEXT     NOT NULL,    "
                "   count300          INTEGER,              "
                "   count100          INTEGER,              "
                "   count50           INTEGER,              "
                "   countMiss         INTEGER,              "

                "   beatmapID             INTEGER  NOT NULL, "
                "   beatmapStarRating     REAL,              "
                "   beatmapDifficultyName TEXT,              "
                "   beatmapArtist         TEXT,              "
                "   beatmapTitle          TEXT,              "
                "   mapsetCreator         TEXT,              "
                "   beatmapMaxCombo       INTEGER,           "

                "   userID                INTEGER  NOT NULL, "
                "   username              TEXT,              "
                "   userCountryCode       TEXT,              "
                "   userPfpLink           TEXT,              "
                "   userPerformancePoints REAL,              "
                "   userAccuracy          REAL,              "
                "   userHoursPlayed       INTEGER,           "
                "   userCurrentRank       INTEGER            "
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
