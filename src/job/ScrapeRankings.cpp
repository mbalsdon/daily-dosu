#include "ScrapeRankings.h"
#include "OsuWrapper.h"
#include "Util.h"
#include "Logger.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <thread>
#include <vector>
#include <string>
#include <utility>
#include <mutex>
#include <cstdint>
#include <cstddef>
#include <functional>

namespace
{
/**
 * Get rankings user for given range and mode, pushing them to the shared vector.
 */
void processRankingsUsers(
    std::vector<RankingsUser>& rankingsUsers /* out */,
    std::mutex& rankingsMtx,
    std::shared_ptr<TokenManager> pTokenManager,
    std::size_t const& start,
    std::size_t const& end,
    Gamemode const& mode)
{
    LOG_DEBUG("Executing thread ", std::this_thread::get_id(), " for pages ", start, "-", end - 1, " for mode ", mode.toString());
    std::vector<RankingsUser> rankingsUsersChunk;
    OsuWrapper osu(pTokenManager, 0);
    for (Page page = start; page < end; ++page)
    {
        nlohmann::json rankingsObj;
        LOG_ERROR_THROW(
            osu.getRankings(page, mode, rankingsObj),
            "Failed to get ranking IDs! page=", page, ", mode=", mode.toString()
        );

        nlohmann::json rankings = rankingsObj["ranking"];
        for (auto const& userStatistics : rankings)
        {
            RankingsUser rankingsUser = {
                .userID = userStatistics.at("user").at("id").get<UserID>(),
                .username = userStatistics.at("user").at("username").get<Username>(),
                .countryCode = userStatistics.at("user").at("country_code").get<CountryCode>(),
                .pfpLink = userStatistics.at("user").at("avatar_url").get<ProfilePicture>(),
                .performancePoints = userStatistics.at("pp").get<PerformancePoints>(),
                .accuracy = userStatistics.at("hit_accuracy").get<Accuracy>(),
                .hoursPlayed = static_cast<HoursPlayed>(userStatistics.at("play_time").get<uint64_t>() / 3600), // FIXME: round instead of trunc
                .currentRank = userStatistics.at("global_rank").get<Rank>()
            };
            rankingsUsersChunk.push_back(rankingsUser);
        }
    }
    {
        std::lock_guard<std::mutex> lock(rankingsMtx);
        rankingsUsers.insert(rankingsUsers.end(), rankingsUsersChunk.begin(), rankingsUsersChunk.end());
    }
}

/**
 * Get users for given range and push them to the shared vector.
 */
void processUsers(
    std::vector<std::pair<UserID, Rank>>& remainingUserRanks /* out */,
    std::vector<UserID> const& remainingUserIDs,
    std::mutex& usersMtx,
    std::shared_ptr<TokenManager> pTokenManager,
    std::size_t const& start,
    std::size_t const& end,
    Gamemode const& mode)
{
    LOG_DEBUG("Executing thread ", std::this_thread::get_id(), " for ", end - start, " users, for mode ", mode.toString());
    std::vector<std::pair<UserID, Rank>> remainingUserRanksChunk;
    OsuWrapper osu(pTokenManager, 0);
    for (std::size_t j = start; j < end; ++j)
    {
        nlohmann::json userObj;
        UserID userID = remainingUserIDs.at(j);
        LOG_ERROR_THROW(
            osu.getUser(userID, mode, userObj),
            "Failed to get user! userID=", userID, ", mode=", mode.toString()
        );
        auto remainingUserRank = std::make_pair(userID, userObj.at("rank_history").at("data")[88].get<Rank>());
        remainingUserRanksChunk.push_back(remainingUserRank);
    }
    {
        std::lock_guard<std::mutex> lock(usersMtx);
        remainingUserRanks.insert(remainingUserRanks.end(), remainingUserRanksChunk.begin(), remainingUserRanksChunk.end());
    }
}

/**
 * Get data for current top 10000 players for given mode.
 */
void scrapeRankingsMode(std::shared_ptr<TokenManager> pTokenManager, std::shared_ptr<RankingsDatabase> pRankingsDb, Gamemode const& mode)
{
    // Shift current rank to yesterday for any existing players
    LOG_DEBUG("Shifting ", mode.toString(), " player current rank values to yesterday");
    pRankingsDb->shiftRanks(mode);

    // Get current top 10,000 players and update database with them
    std::vector<RankingsUser> rankingsUsers;
    rankingsUsers.reserve(k_getRankingIDMaxPage * k_getRankingIDMaxNumIDs);

    std::size_t numCores = static_cast<std::size_t>(std::thread::hardware_concurrency());
    std::size_t numCallsPerThread = k_getRankingIDMaxPage / numCores;
    std::size_t numLeftoverCalls = k_getRankingIDMaxPage % numCores;

    LOG_DEBUG("Hardware concurrency is ", numCores);

    std::mutex rankingsMtx;
    std::vector<std::thread> getRankingThreads;
    getRankingThreads.reserve(numCores);

    // Distribute work as evenly as possible
    for (std::size_t i = 0; i < numCores; ++i)
    {
        bool bDoExtraCall = (i < numLeftoverCalls);
        std::size_t startIdx = i * numCallsPerThread;
        std::size_t endIdx = startIdx + numCallsPerThread;
        if (bDoExtraCall)
        {
            startIdx += i;
            endIdx += i + 1;
        }
        else
        {
            startIdx += numLeftoverCalls;
            endIdx += numLeftoverCalls;
        }

        // Fire thread (each populates the vector with its portion)
        getRankingThreads.emplace_back(
            processRankingsUsers,
            std::ref(rankingsUsers),
            std::ref(rankingsMtx),
            std::ref(pTokenManager),
            startIdx,
            endIdx,
            mode);
    }

    // Wait for threads to complete
    for (auto& getRankingThread : getRankingThreads)
    {
        getRankingThread.join();
    }

    // Dump to database
    LOG_DEBUG("Dumping ", mode.toString(), " player data to database");
    pRankingsDb->insertRankingsUsers(rankingsUsers, mode);

    // Remove any entry with null currentRank (means they dropped out of top 10k)
    LOG_DEBUG("Removing ", mode.toString(), " players that dropped out of top 10k");
    pRankingsDb->deleteUsersWithNullCurrentRank(mode);

    // Fill in yesterdayRank for rows where it is NULL (means they entered top 10k)
    LOG_DEBUG("Filling in data for ", mode.toString(), " players that entered top 10k");
    std::vector<UserID> remainingUserIDs = pRankingsDb->getUserIDsWithNullYesterdayRank(mode);
    std::size_t numRemainingUsers = remainingUserIDs.size();

    std::vector<std::pair<UserID, Rank>> remainingUserRanks;
    remainingUserRanks.reserve(numRemainingUsers);

    numCallsPerThread = numRemainingUsers / numCores;
    numLeftoverCalls = numRemainingUsers % numCores;

    std::mutex usersMtx;
    std::vector<std::thread> getUserThreads;
    getUserThreads.reserve(numCores);

    // Distribute work as evenly as possible
    for (std::size_t i = 0; i < numCores; ++i)
    {
        bool bDoExtraCall = (i < numLeftoverCalls);
        std::size_t startIdx = i * numCallsPerThread;
        std::size_t endIdx = startIdx + numCallsPerThread;
        if (bDoExtraCall)
        {
            startIdx += i;
            endIdx += i + 1;
        }
        else
        {
            startIdx += numLeftoverCalls;
            endIdx += numLeftoverCalls;
        }

        // Fire thread (each populates the vector with its portion)
        getUserThreads.emplace_back(
            processUsers,
            std::ref(remainingUserRanks),
            std::ref(remainingUserIDs),
            std::ref(usersMtx),
            std::ref(pTokenManager),
            startIdx,
            endIdx,
            mode);
    }

    // Wait for threads to complete
    for (auto& getUserThread : getUserThreads)
    {
        getUserThread.join();
    }

    // Dump to database
    LOG_DEBUG("Updating ", mode.toString(), " database with new player yesterday rank values");
    pRankingsDb->updateYesterdayRanks(remainingUserRanks, mode);
}
} /* namespace */

/**
 * WARNING: This script runs fast! If your system is powerful enough, you might get
 * ratelimited (but the API wrapper should deal with that).
 *
 * Get data for current top 10000 players in each mode. If the last run was (roughly) a day ago, this
 * script will make a bit over ~800 osu!API calls. Otherwise, it will make 40,800 calls.
 */
void scrapeRankings(std::shared_ptr<TokenManager> pTokenManager, std::shared_ptr<RankingsDatabase> pRankingsDb, bool bParallel)
{
    // FUTURE: implement
    LOG_ERROR_THROW(bParallel, "Sequential version of scrapeRankings is not implemented!");

    LOG_INFO("Scraping osu! rankings");

    // If last run was not roughly a day ago, wipe everything
    auto lastWriteTime = pRankingsDb->lastWriteTime();
    auto now = std::filesystem::file_time_type::clock::now();
    std::chrono::hours ageHours = std::chrono::duration_cast<std::chrono::hours>(now - lastWriteTime);
    if ((ageHours < k_minValidScrapeRankingsHour) || (ageHours > k_maxValidScrapeRankingsHour))
    {
        LOG_WARN("Database is out of sync with current time of running; starting from scratch");
        pRankingsDb->wipeTables();
    }

    // Do work for each mode
    scrapeRankingsMode(pTokenManager, pRankingsDb, Gamemode::Osu);
    scrapeRankingsMode(pTokenManager, pRankingsDb, Gamemode::Taiko);
    scrapeRankingsMode(pTokenManager, pRankingsDb, Gamemode::Mania);
    scrapeRankingsMode(pTokenManager, pRankingsDb, Gamemode::Catch);
}
