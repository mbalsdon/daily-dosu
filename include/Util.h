#ifndef __UTIL_H__
#define __UTIL_H__

#include <string>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <chrono>

/* - - - - - - - - Types - - - - - - - */

typedef std::string OAuthToken;
typedef std::size_t Page;

typedef int64_t UserID;
typedef std::string Username;
typedef std::string CountryCode;
typedef std::string ProfilePicture;
typedef int64_t PerformancePoints;
typedef double Accuracy;
typedef int64_t HoursPlayed;
typedef int64_t Rank;

struct RankingsUser
{
    UserID userID = -1;
    Username username = "";
    CountryCode countryCode = "";
    ProfilePicture pfpLink = "";
    PerformancePoints performancePoints = -1;
    Accuracy accuracy = -1.;
    HoursPlayed hoursPlayed = -1;
    Rank yesterdayRank = -1;
    Rank currentRank = -1;

    bool isValid() const
    {
        return (
            (userID > 0) &&
            !username.empty() &&
            !countryCode.empty() &&
            !pfpLink.empty() &&
            (performancePoints >= 0) &&
            (accuracy >= 0.) &&
            (hoursPlayed >= 0) &&
            (currentRank >= 1)
        );
    }
};

struct RankImprovement
{
    RankingsUser user;
    double relativeImprovement;
};

enum class RankRange
{
    First,
    Second,
    Third
};

/* - - - - - - Constants - - - - - - - */

constexpr std::size_t k_getRankingIDMaxNumIDs = 50;
constexpr std::size_t k_getRankingIDMaxPage = 200;

constexpr std::size_t k_firstRangeMax = 100;
constexpr std::size_t k_secondRangeMax = 1000;
constexpr std::size_t k_thirdRangeMax = 10000;

constexpr std::size_t k_numDisplayUsersTop = 15;
constexpr std::size_t k_numDisplayUsersBottom = 5;

const std::chrono::hours k_minValidScrapeRankingsHour = std::chrono::hours(22);
const std::chrono::hours k_maxValidScrapeRankingsHour = std::chrono::hours(26);

const std::filesystem::path k_rootDir = std::filesystem::path(__FILE__).parent_path().parent_path();
const std::filesystem::path k_dosuConfigFilePath = k_rootDir / "dosu_config.json";
const std::filesystem::path k_dataDir = k_rootDir / "data";

const std::string k_cmdHelp = "help";
const std::string k_cmdPing = "ping";
const std::string k_cmdNewsletter = "newsletter";
const std::string k_cmdSubscribe = "subscribe";
const std::string k_cmdUnsubscribe = "unsubscribe";

const std::string k_cmdHelpDesc = "List the bot's commands.";
const std::string k_cmdPingDesc = "Check if the bot is alive.";
const std::string k_cmdNewsletterDesc = "Display the daily newsletter.";
const std::string k_cmdSubscribeDesc = "Enable daily newsletters for this channel.";
const std::string k_cmdUnsubscribeDesc = "Disable daily newsletters for this channel.";

const std::string k_firstRangeButtonID = "first_range_button";
const std::string k_secondRangeButtonID = "second_range_button";
const std::string k_thirdRangeButtonID = "third_range_button";

#endif /* __UTIL_H__ */
