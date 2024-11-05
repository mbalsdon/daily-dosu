#include "ScrapePlayers.h"
#include "OsuWrapper.h"
#include "DosuConfig.h"

#include <dpp/nlohmann/json.hpp>

#include <iostream>
#include <vector>
#include <filesystem>
#include <fstream>

namespace Detail
{
RankChangeRatio getRankChangeRatio(DosuUser user)
{
    int yesterdayRank = static_cast<int>(user.yesterdayRank);
    int currentRank = static_cast<int>(user.currentRank);

    if (yesterdayRank < currentRank) // Rank decreased (e.g. #5 -> #10)
    {
        return 0.f - (static_cast<RankChangeRatio>(currentRank - yesterdayRank) / currentRank);
    }
    else // Rank increased (e.g. #10 -> #5)
    {
        return static_cast<RankChangeRatio>(yesterdayRank - currentRank) / currentRank;
    }
}
} /* namespace Detail */

/**
 * Retrieves player data from the osu!API and stores it.
 * Sorts by rank gain ratio, the main metric we're interested in.
 * For example, going from rank #1,000 to #500 means a 100% increase, and going from rank #5 to #10 means a 100% decrease.
 * The file store is overwritten each time this function completes.
 */
void scrapePlayers()
{
    OsuWrapper osu(DosuConfig::osuClientID, DosuConfig::osuClientSecret, DosuConfig::osuApiCooldownMs);
    size_t numIDs = k_getRankingIDMaxPage * k_getRankingIDMaxNumIDs;

    // Grab user IDs
    std::vector<UserID> userIDs;
    userIDs.reserve(numIDs);
    for (Page page = 0; page < k_getRankingIDMaxPage; ++page)
    {
        std::vector<UserID> pageIDs;
        pageIDs.reserve(k_getRankingIDMaxNumIDs);
        if (!osu.getRankingIDs(page, pageIDs, k_getRankingIDMaxNumIDs))
        {
            std::string reason = "scrapePlayers - failed to get ranking IDs!";
            throw std::runtime_error(reason);
        }

        userIDs.insert(userIDs.end(), pageIDs.begin(), pageIDs.end());
    }

    // Grab user data and calculate associated rank change ratio
    nlohmann::json jsonUsers = nlohmann::json::array();
    for (const UserID& userID : userIDs)
    {
        DosuUser user;
        if (!osu.getUser(userID, user))
        {
            std::cout << "scrapePlayers - failed to get user " << static_cast<int>(userID) << "! Skipping..." << std::endl;
            continue;
        }

        jsonUsers.push_back({
            {k_userIDKey, user.userID},
            {k_usernameKey, user.username},
            {k_countryCodeKey, user.countryCode},
            {k_pfpLinkKey, user.pfpLink},
            {k_hoursPlayedKey, user.hoursPlayed},
            {k_currentRankKey, user.currentRank},
            {k_yesterdayRankKey, user.yesterdayRank},
            {k_performancePointsKey, user.performancePoints},
            {k_accuracyKey, user.accuracy},
            {k_rankChangeRatioKey, Detail::getRankChangeRatio(user)},
        });
    }

    // Sort by rank change ratio (highest to lowest)
    std::sort(
        jsonUsers.begin(),
        jsonUsers.end(),
        [](const nlohmann::json& a, const nlohmann::json& b) { return a[k_rankChangeRatioKey] > b[k_rankChangeRatioKey]; }
    );

    // Store data
    std::filesystem::path rootDir = std::filesystem::path(__FILE__).parent_path().parent_path();

    std::filesystem::path dataDir = rootDir / k_dataDirName;
    if (!std::filesystem::exists(dataDir))
    {
        std::filesystem::create_directory(dataDir);
    }

    std::filesystem::path dataFilePath = dataDir / k_jsonFileName;
    std::ofstream jsonFile(dataFilePath, std::ios::trunc);
    if (!jsonFile.is_open())
    {
        std::string reason = std::string("scrapePlayers - failed to open ").append(dataFilePath.string());
        throw std::runtime_error(reason);
    }

    jsonFile << jsonUsers.dump(2) << std::endl;
    jsonFile.close();
    std::cout << "scrapePlayers - finished writing user data to " << dataFilePath.string() << "!" << std::endl;
}
