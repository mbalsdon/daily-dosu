#ifndef __RANKINGS_DATABASE_MANAGER_H__
#define __RANKINGS_DATABASE_MANAGER_H__

#include "Util.h"

#include <SQLiteCpp/SQLiteCpp.h>

#include <filesystem>
#include <vector>
#include <mutex>
#include <cstdint>
#include <cstddef>
#include <unordered_map>

const std::unordered_map<Gamemode, std::string> k_modeToRankingsTable = {
    { Gamemode::Osu, "OsuRankings" },
    { Gamemode::Taiko, "TaikoRankings" },
    { Gamemode::Mania, "ManiaRankings" },
    { Gamemode::Catch, "CatchRankings" }
};

/**
 * SQLiteCpp wrapper for rankings tables.
 */
class RankingsDatabase
{
public:
    RankingsDatabase(std::filesystem::path const& dbFilePath);
    ~RankingsDatabase();

    [[nodiscard]] std::filesystem::file_time_type lastWriteTime();
    void wipeTables();
    void shiftRanks(Gamemode const& mode);
    void insertRankingsUsers(std::vector<RankingsUser> const& rankingsUsers, Gamemode const& mode);
    void deleteUsersWithNullCurrentRank(Gamemode const& mode);
    [[nodiscard]] std::vector<UserID> getUserIDsWithNullYesterdayRank(Gamemode const& mode);
    void updateYesterdayRanks(std::vector<std::pair<UserID, Rank>> const& userYesterdayRanks, Gamemode const& mode);
    [[nodiscard]] bool hasEmptyTable();
    [[nodiscard]] std::vector<RankImprovement> getTopRankImprovements(
        std::string const& countryCode,
        int64_t const& minRank,
        int64_t const& maxRank,
        std::size_t const& numUsers,
        Gamemode const& mode);
    [[nodiscard]] std::vector<RankImprovement> getBottomRankImprovements(
        std::string const& countryCode,
        int64_t const& minRank,
        int64_t const& maxRank,
        std::size_t const& numUsers,
        Gamemode const& mode);

private:
    void createTables_();

    std::unique_ptr<SQLite::Database> m_pDatabase;
    std::filesystem::path m_dbFilePath;
    std::mutex m_dbMtx;
};

#endif /* __RANKINGS_DATABASE_MANAGER_H__ */
