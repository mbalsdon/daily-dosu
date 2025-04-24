#include "ScrapeRankings.h"
#include "OsuWrapper.h"
#include "Util.h"
#include "Logger.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <vector>
#include <string>
#include <cstdint>
#include <cstddef>
#include <future>
#include <utility>

namespace
{
/**
 * Get rankings user for given page and mode.
 */
std::vector<RankingsUser> getRankingsUsersChunk(
    std::shared_ptr<TokenManager> pTokenManager,
    Page const& page,
    Gamemode const& mode)
{
    OsuWrapper osu(pTokenManager, 0);
    nlohmann::json rankingsObj;
    LOG_ERROR_THROW(
        osu.getRankings(page, mode, rankingsObj),
        "Failed to get ranking IDs! page=", page, ", mode=", mode.toString()
    );

    std::vector<RankingsUser> rankingsUsersChunk;
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

    return rankingsUsersChunk;
}

/**
 * Get the rank that a user was yesterday, for given mode.
 */
std::pair<UserID, Rank> getUserYesterdayRank(
    std::shared_ptr<TokenManager> pTokenManager,
    UserID const& userID,
    Gamemode const& mode)
{
    OsuWrapper osu(pTokenManager, 0);
    nlohmann::json userObj;
    LOG_ERROR_THROW(
        osu.getUser(userID, mode, userObj),
        "Failed to get user! userID=", userID, ", mode=", mode.toString()
    );

    auto userYesterdayRank = std::make_pair(userID, userObj.at("rank_history").at("data")[88].get<Rank>());
    return userYesterdayRank;
}

/**
 * Get data for current top 10000 players for given mode.
 */
void scrapeRankingsMode(
    std::shared_ptr<TokenManager> pTokenManager,
    std::shared_ptr<RankingsDatabase> pRankingsDb,
    Gamemode const& mode)
{
    // Shift current rank to yesterday for any existing players
    pRankingsDb->shiftRanks(mode);

    // Get current top 10,000 players and update database with them
    std::vector<RankingsUser> rankingsUsers;
    rankingsUsers.reserve(k_getRankingIDMaxPage * k_getRankingIDMaxNumIDs);

    std::vector<std::future<std::vector<RankingsUser>>> rankingsUsersFutures;
    rankingsUsersFutures.reserve(k_getRankingIDMaxPage);

    for (Page i = 0; i < k_getRankingIDMaxPage; ++i)
    {
        auto futureRankingsUsersChunk = std::async(std::launch::async, getRankingsUsersChunk, pTokenManager, i, mode);
        rankingsUsersFutures.push_back(std::move(futureRankingsUsersChunk));
    }

    for (auto& futureRankingsUsersChunk : rankingsUsersFutures)
    {
        std::vector<RankingsUser> rankingsUsersChunk = futureRankingsUsersChunk.get();
        rankingsUsers.insert(rankingsUsers.end(), std::make_move_iterator(rankingsUsersChunk.begin()), std::make_move_iterator(rankingsUsersChunk.end()));
    }

    pRankingsDb->insertRankingsUsers(rankingsUsers, mode);

    // Remove entries w/ null currentRank (=> they dropped out of top 10k)
    pRankingsDb->deleteUsersWithNullCurrentRank(mode);

    // Fill in yesterdayRank for entries where it's null (=> they entered top 10k)
    std::vector<UserID> remainingUserIDs = pRankingsDb->getUserIDsWithNullYesterdayRank(mode);

    std::vector<std::pair<UserID, Rank>> remainingUserYesterdayRanks;
    remainingUserYesterdayRanks.reserve(remainingUserIDs.size());

    std::vector<std::future<std::pair<UserID, Rank>>> remainingUserYesterdayRankFutures;
    remainingUserYesterdayRankFutures.reserve(remainingUserIDs.size());

    for (const auto& remainingUserID : remainingUserIDs)
    {
        auto futureRemainingUserYesterdayRank = std::async(std::launch::async, getUserYesterdayRank, pTokenManager, remainingUserID, mode);
        remainingUserYesterdayRankFutures.push_back(std::move(futureRemainingUserYesterdayRank));
    }

    for (auto& futureRemainingUserYesterdayRank : remainingUserYesterdayRankFutures)
    {
        std::pair<UserID, Rank> remainingUserYesterdayRank = futureRemainingUserYesterdayRank.get();
        remainingUserYesterdayRanks.push_back(std::move(remainingUserYesterdayRank));
    }

    pRankingsDb->updateYesterdayRanks(remainingUserYesterdayRanks, mode);
}
} /* namespace */

/**
 * WARNING: This script runs fast! If your system is powerful enough, you might get
 * ratelimited (but the API wrapper should deal with that).
 *
 * Get data for current top 10000 players in each mode. If the last run was (roughly) a day ago, this
 * script will make a bit over ~800 osu!API calls. Otherwise, it will make 40,800 calls.
 */
void scrapeRankings(std::shared_ptr<TokenManager> pTokenManager, std::shared_ptr<RankingsDatabase> pRankingsDb)
{
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
