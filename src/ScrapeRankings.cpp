#include "ScrapeRankings.h"
#include "OsuWrapper.h"
#include "Util.h"
#include "Logger.h"
#include "DosuConfig.h"

#include <filesystem>
#include <thread>
#include <vector>
#include <string>
#include <utility>
#include <mutex>
#include <cstdint>
#include <cstddef>
#include <functional>

namespace Detail
{
/**
 * Get rankings user for given range and push them to the shared vector.
 */
void processRankingsUsers(
    std::vector<RankingsUser>& rankingsUsers,
    std::mutex& rankingsMtx,
    std::size_t start,
    std::size_t end)
{
    LOG_INFO("Executing thread ", std::this_thread::get_id(), " for pages ", start, "-", end - 1, "...");
    std::vector<RankingsUser> rankingsUsersChunk;
    OsuWrapper osu(DosuConfig::osuClientID, DosuConfig::osuClientSecret, 0);
    for (Page page = start; page < end; ++page)
    {
        nlohmann::json rankingsObj;
        if (!osu.getRankings(page, rankingsObj))
        {
            std::string reason = std::string("scrapeRankings - failed to get ranking IDs! page=").append(std::to_string(page));
            throw std::runtime_error(reason);
        }

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
    std::vector<std::pair<UserID, Rank>>& remainingUserRanks,
    std::vector<UserID>& remainingUserIDs,
    std::mutex& usersMtx,
    std::size_t start,
    std::size_t end)
{
    LOG_INFO("Executing thread ", std::this_thread::get_id(), " for ", end - start, " users...");
    std::vector<std::pair<UserID, Rank>> remainingUserRanksChunk;
    OsuWrapper osu(DosuConfig::osuClientID, DosuConfig::osuClientSecret, 0);
    for (std::size_t j = start; j < end; ++j)
    {
        nlohmann::json userObj;
        UserID userID = remainingUserIDs.at(j);
        if (!osu.getUser(userID, userObj))
        {
            std::string reason = std::string("scrapeRankings - failed to get user! userID=").append(std::to_string(userID));
            throw std::runtime_error(reason);
        }
        auto remainingUserRank = std::make_pair(userID, userObj.at("rank_history").at("data")[88].get<Rank>());
        remainingUserRanksChunk.push_back(remainingUserRank);
    }
    {
        std::lock_guard<std::mutex> lock(usersMtx);
        remainingUserRanks.insert(remainingUserRanks.end(), remainingUserRanksChunk.begin(), remainingUserRanksChunk.end());
    }
}
} /* namespace Detail */

/**
 * WARNING: This script runs fast! If your system is powerful enough, you might get
 * ratelimited (but the API wrapper should deal with that).
 * 
 * Get data for current top 10000 players. If the last run was (roughly) a day ago, this
 * script will make a bit over ~200 osu!API calls. Otherwise, it will make 10,200 calls.
 */
void scrapeRankings(RankingsDatabaseManager& rankingsDbm)
{
    LOG_INFO("Scraping osu! rankings...");

    // If last run was not roughly a day ago, wipe everything
    auto lastWriteTime = rankingsDbm.lastWriteTime();
    auto now = std::filesystem::file_time_type::clock::now();
    std::chrono::hours ageHours = std::chrono::duration_cast<std::chrono::hours>(now - lastWriteTime);
    if ((ageHours < std::chrono::hours(24)) || (ageHours > std::chrono::hours(25)))
    {
        LOG_INFO("Database is out of sync with current time of running; starting from scratch...");
        rankingsDbm.wipeTables();
    }

    // Shift current rank to yesterday for any existing players
    LOG_INFO("Shifting player current rank values to yesterday...");
    rankingsDbm.shiftRanks();

    // Get current top 10,000 players and update database with them
    std::vector<RankingsUser> rankingsUsers;
    rankingsUsers.reserve(k_getRankingIDMaxPage * k_getRankingIDMaxNumIDs);

    std::size_t numCores = static_cast<std::size_t>(std::thread::hardware_concurrency());
    std::size_t numCallsPerThread = k_getRankingIDMaxPage / numCores;
    std::size_t numLeftoverCalls = k_getRankingIDMaxPage % numCores;

    LOG_INFO("Hardware concurrency is ", numCores);

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
            Detail::processRankingsUsers,
            std::ref(rankingsUsers),
            std::ref(rankingsMtx),
            startIdx,
            endIdx);
    }

    // Wait for threads to complete
    for (auto& getRankingThread : getRankingThreads)
    {
        getRankingThread.join();
    }

    // Dump to database
    LOG_INFO("Dumping player data to database...");
    rankingsDbm.insertRankingsUsers(rankingsUsers);

    // Remove any entry with null currentRank (means they dropped out of top 10k)
    LOG_INFO("Removing players that dropped out of top 10k...");
    rankingsDbm.deleteUsersWithNullCurrentRank();

    // Fill in yesterdayRank for rows where it is NULL (means they entered top 10k)
    LOG_INFO("Filling in data for players that entered top 10k...");
    std::vector<UserID> remainingUserIDs = rankingsDbm.getUserIDsWithNullYesterdayRank();
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

        // FIXME: move chungus lambda to a function
        // Fire thread (each populates the vector with its portion)
        getUserThreads.emplace_back(
            Detail::processUsers,
            std::ref(remainingUserRanks),
            std::ref(remainingUserIDs),
            std::ref(usersMtx),
            startIdx,
            endIdx);
    }

    // Wait for threads to complete
    for (auto& getUserThread : getUserThreads)
    {
        getUserThread.join();
    }

    // Dump to database
    LOG_INFO("Updating database with new player yesterday rank values...");
    rankingsDbm.updateYesterdayRanks(remainingUserRanks);
}