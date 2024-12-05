#ifndef __RANKINGS_DATABASE_MANAGER_H__
#define __RANKINGS_DATABASE_MANAGER_H__

#include "Util.h"

#include <SQLiteCpp/SQLiteCpp.h>

#include <filesystem>
#include <optional>
#include <vector>
#include <mutex>
#include <cstdint>
#include <cstddef>
#include <atomic>

/**
 * SQLiteCpp wrapper for the Rankings table.
 */
class RankingsDatabaseManager
{
public:
    static RankingsDatabaseManager& getInstance()
    {
        static RankingsDatabaseManager instance;
        return instance;
    }

    void initialize(const std::filesystem::path dbFilePath);
    void cleanup();

    std::filesystem::file_time_type lastWriteTime();
    void wipeTables();
    void shiftRanks();
    void insertRankingsUsers(std::vector<RankingsUser>& rankingsUsers);
    void deleteUsersWithNullCurrentRank();
    std::vector<UserID> getUserIDsWithNullYesterdayRank();
    void updateYesterdayRanks(std::vector<std::pair<UserID, Rank>>& userYesterdayRanks);
    bool isRankingsEmpty();
    std::vector<RankImprovement> getTopRankImprovements(std::string countryCode, int64_t minRank, int64_t maxRank, std::size_t numUsers);
    std::vector<RankImprovement> getBottomRankImprovements(std::string countryCode, int64_t minRank, int64_t maxRank, std::size_t numUsers);

private:
    RankingsDatabaseManager()
    : m_database(nullptr)
    , m_bIsInitialized(false)
    {}

    ~RankingsDatabaseManager();
    RankingsDatabaseManager(const RankingsDatabaseManager&) = delete;
    RankingsDatabaseManager& operator=(const RankingsDatabaseManager&) = delete;

    void createTables();

    std::unique_ptr<SQLite::Database> m_database;
    std::filesystem::path m_dbFilePath;
    std::mutex m_dbMtx;
    std::atomic<bool> m_bIsInitialized;
};

#endif /* __RANKINGS_DATABASE_MANAGER_H__ */
