#ifndef __TOP_PLAYS_DATABASE_MANAGER_H__
#define __TOP_PLAYS_DATABASE_MANAGER_H__

#include "Util.h"

#include <SQLiteCpp/SQLiteCpp.h>

#include <filesystem>
#include <mutex>
#include <atomic>
#include <unordered_map>

const std::unordered_map<Gamemode, std::string> k_modeToTopPlaysTable = {
    { Gamemode::Osu, "OsuTopPlays" },
    { Gamemode::Taiko, "TaikoTopPlays" },
    { Gamemode::Mania, "ManiaTopPlays" },
    { Gamemode::Catch, "CatchTopPlays" }
};

/**
 * SQLiteCpp wrapper for top plays tables.
 */
class TopPlaysDatabaseManager
{
public:
    static TopPlaysDatabaseManager& getInstance()
    {
        static TopPlaysDatabaseManager instance;
        return instance;
    }

    void initialize(const std::filesystem::path dbFilePath);
    void cleanup();

    std::filesystem::file_time_type lastWriteTime();
    void wipeTables();
    bool hasEmptyTable();
    void insertTopPlays(Gamemode const& mode, std::vector<TopPlay> const& topPlays);
    std::vector<TopPlay> getTopPlays(std::string const& countryCode, std::size_t const& numTopPlays, Gamemode const& mode);

private:
    TopPlaysDatabaseManager()
    : m_database(nullptr)
    , m_bIsInitialized(false)
    {}

    ~TopPlaysDatabaseManager();

    TopPlaysDatabaseManager(const TopPlaysDatabaseManager&) = delete;
    TopPlaysDatabaseManager(TopPlaysDatabaseManager&&) = delete;
    TopPlaysDatabaseManager& operator=(const TopPlaysDatabaseManager&) = delete;
    TopPlaysDatabaseManager& operator=(TopPlaysDatabaseManager&&) = delete;

    void createTables();

    std::unique_ptr<SQLite::Database> m_database;
    std::filesystem::path m_dbFilePath;
    std::mutex m_dbMtx;
    std::atomic<bool> m_bIsInitialized;
};

#endif /* __TOP_PLAYS_DATABASE_MANAGER_H__ */
