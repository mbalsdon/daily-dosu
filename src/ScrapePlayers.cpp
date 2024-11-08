#include "ScrapePlayers.h"
#include "OsuWrapper.h"
#include "DosuConfig.h"

#include <dpp/nlohmann/json.hpp>

#include <iostream>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace Detail
{
RankChangeRatio getRankChangeRatio(DosuUser user)
{
    int yesterdayRank = static_cast<int>(user.yesterdayRank);
    int currentRank = static_cast<int>(user.currentRank);

    if (yesterdayRank < currentRank) // Rank decreased (e.g. #5 -> #10)
    {
        return 0. - (static_cast<RankChangeRatio>(currentRank - yesterdayRank) / currentRank);
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

    // Store data (full dataset)
    std::filesystem::path rootDir = std::filesystem::path(__FILE__).parent_path().parent_path();

    std::filesystem::path dataDir = rootDir / k_dataDirName;
    if (!std::filesystem::exists(dataDir))
    {
        std::filesystem::create_directory(dataDir);
    }

    std::filesystem::path usersFullFilePath = dataDir / k_usersFullFileName;
    std::ofstream usersFullJsonFile(usersFullFilePath, std::ios::trunc);
    if (!usersFullJsonFile.is_open())
    {
        std::string reason = std::string("scrapePlayers - failed to open ").append(usersFullFilePath.string());
        throw std::runtime_error(reason);
    }

    usersFullJsonFile << jsonUsers.dump(2) << std::endl;
    usersFullJsonFile.close();
    std::cout << "scrapePlayers - finished writing user data to " << usersFullFilePath.string() << "!" << std::endl;

    // Get top users from each rank range
    nlohmann::json topUsersFirstRangeJson = nlohmann::json::array();
    nlohmann::json topUsersSecondRangeJson = nlohmann::json::array();
    nlohmann::json topUsersThirdRangeJson = nlohmann::json::array();
    for (const auto& jsonUser : jsonUsers)
    {
        Rank userRank = jsonUser.at(k_currentRankKey).get<Rank>();

        if ((topUsersFirstRangeJson.size() < k_numDisplayUsers) &&
            (userRank < k_firstRangeMax))
        {
            topUsersFirstRangeJson.push_back(jsonUser);
        }
        else if ((topUsersSecondRangeJson.size() < k_numDisplayUsers) &&
                 (userRank >= k_firstRangeMax) &&
                 (userRank < k_secondRangeMax))
        {
            topUsersSecondRangeJson.push_back(jsonUser);
        }
        else if ((topUsersThirdRangeJson.size() < k_numDisplayUsers) &&
                 (userRank >= k_secondRangeMax) &&
                 (userRank < k_thirdRangeMax))
        {
            topUsersThirdRangeJson.push_back(jsonUser);
        }
    }

    // Get bottom users from each rank range
    nlohmann::json bottomUsersFirstRangeJson = nlohmann::json::array();
    nlohmann::json bottomUsersSecondRangeJson = nlohmann::json::array();
    nlohmann::json bottomUsersThirdRangeJson = nlohmann::json::array();
    for (auto it = jsonUsers.rbegin(); it != jsonUsers.rend(); ++it)
    {
        const auto& jsonUser = *it;
        Rank userRank = jsonUser.at(k_currentRankKey).get<Rank>();

        if ((bottomUsersFirstRangeJson.size() < k_numDisplayUsers) &&
            (userRank < k_firstRangeMax))
        {
            bottomUsersFirstRangeJson.push_back(jsonUser);
        }
        else if ((bottomUsersSecondRangeJson.size() < k_numDisplayUsers) &&
                 (userRank >= k_firstRangeMax) &&
                 (userRank < k_secondRangeMax))
        {
            bottomUsersSecondRangeJson.push_back(jsonUser);
        }
        else if ((bottomUsersThirdRangeJson.size() < k_numDisplayUsers) &&
                 (userRank >= k_secondRangeMax) &&
                 (userRank < k_thirdRangeMax))
        {
            bottomUsersThirdRangeJson.push_back(jsonUser);
        }
    }

    // Build JSON object (compact dataset)
    nlohmann::json compactUsersJson = {
        {k_firstRangeKey, {
            {k_topUsersKey, topUsersFirstRangeJson},
            {k_bottomUsersKey, bottomUsersFirstRangeJson}
        }},
        {k_secondRangeKey, {
            {k_topUsersKey, topUsersSecondRangeJson},
            {k_bottomUsersKey, bottomUsersSecondRangeJson}
        }},
        {k_thirdRangeKey, {
            {k_topUsersKey, topUsersThirdRangeJson},
            {k_bottomUsersKey, bottomUsersThirdRangeJson}
        }}
    };

    // Store data (compact dataset)
    std::filesystem::path usersCompactFilePath = dataDir / k_usersCompactFileName;
    std::ofstream usersCompactJsonFile(usersCompactFilePath, std::ios::trunc);
    if (!usersCompactJsonFile.is_open())
    {
        std::string reason = std::string("scrapePlayers - failed to open ").append(usersCompactFilePath.string());
        throw std::runtime_error(reason);
    }

    usersCompactJsonFile << compactUsersJson.dump(2) << std::endl;
    usersCompactJsonFile.close();
    std::cout << "scrapePlayers - finished writing user data to " << usersCompactFilePath.string() << "!" << std::endl;
}
