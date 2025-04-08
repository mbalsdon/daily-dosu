#ifndef __TOP_PLAYS_DATABASE_MANAGER_H__
#define __TOP_PLAYS_DATABASE_MANAGER_H__

#include "Util.h"

#include <SQLiteCpp/SQLiteCpp.h>

#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <cstddef>
#include <vector>

const std::unordered_map<Gamemode, std::string> k_modeToTopPlaysTable = {
    { Gamemode::Osu, "OsuTopPlays" },
    { Gamemode::Taiko, "TaikoTopPlays" },
    { Gamemode::Mania, "ManiaTopPlays" },
    { Gamemode::Catch, "CatchTopPlays" }
};

/**
 * SQLiteCpp wrapper for top plays tables.
 */
class TopPlaysDatabase
{
public:
    TopPlaysDatabase(std::filesystem::path const& dbFilePath);
    ~TopPlaysDatabase();

    [[nodiscard]] std::filesystem::file_time_type lastWriteTime();
    void wipeTables();
    [[nodiscard]] bool hasEmptyTable();
    void insertTopPlays(Gamemode const& mode, std::vector<TopPlay> const& topPlays);
    [[nodiscard]] std::vector<TopPlay> getTopPlays(std::string const& countryCode, std::size_t const& numTopPlays, Gamemode const& mode);

private:
    void createTables_();

    std::unique_ptr<SQLite::Database> m_pDatabase;
    std::filesystem::path m_dbFilePath;
    std::mutex m_dbMtx;
};

#endif /* __TOP_PLAYS_DATABASE_MANAGER_H__ */
