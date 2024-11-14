#ifndef __UTIL_H__
#define __UTIL_H__

#include <string>
#include <cstdint>
#include <cstddef>
#include <filesystem>
#include <chrono>

/* - - - - - - - - Types - - - - - - - */

typedef std::string OAuthToken;
typedef uint16_t Page;

typedef uint32_t UserID;
typedef std::string Username;
typedef std::string CountryCode;
typedef std::string ProfilePicture;
typedef uint32_t HoursPlayed;
typedef uint32_t Rank;
typedef uint32_t PerformancePoints;
typedef double Accuracy;

typedef double RankChangeRatio;

struct DosuUser
{
    UserID userID = 0;
    Username username = "";
    CountryCode countryCode = "";
    ProfilePicture pfpLink = "";
    HoursPlayed hoursPlayed = 0;
    Rank currentRank = 0;
    Rank yesterdayRank = 0;
    PerformancePoints performancePoints = 0;
    Accuracy accuracy = 0.;

    bool isEmpty() const { return userID == 0; }
    void clear() { *this = DosuUser{}; }
};

/* - - - - - - Constants - - - - - - - */

constexpr std::size_t k_getRankingIDMaxNumIDs = 50;
constexpr std::size_t k_getRankingIDMaxPage = 200;

const std::string k_userIDKey = "user_id";
const std::string k_usernameKey = "username";
const std::string k_countryCodeKey = "country_code";
const std::string k_pfpLinkKey = "pfp_link";
const std::string k_hoursPlayedKey = "hours_played";
const std::string k_currentRankKey = "current_rank";
const std::string k_yesterdayRankKey = "yesterday_rank";
const std::string k_performancePointsKey = "performance_points";
const std::string k_accuracyKey = "accuracy";
const std::string k_rankChangeRatioKey = "rank_change_ratio";

constexpr std::size_t k_firstRangeMax = 100;
constexpr std::size_t k_secondRangeMax = 1000;
constexpr std::size_t k_thirdRangeMax = 10000;

constexpr std::size_t k_numDisplayUsersTop = 15;
constexpr std::size_t k_numDisplayUsersBottom = 5;

const std::string k_firstRangeKey = "first_range";
const std::string k_secondRangeKey = "second_range";
const std::string k_thirdRangeKey = "third_range";

const std::string k_topUsersKey = "top";
const std::string k_bottomUsersKey = "bottom";

const std::string k_serverConfigChannelsKey = "channels";

const std::filesystem::path k_rootDir = std::filesystem::path(__FILE__).parent_path().parent_path();
const std::filesystem::path k_dosuConfigFilePath = k_rootDir / "dosu_config.json";

const std::filesystem::path k_dataDir = k_rootDir / "data";
const std::filesystem::path k_usersFullFilePath = k_dataDir / "users_full.json";
const std::filesystem::path k_usersCompactFilePath = k_dataDir / "users_compact.json";
const std::filesystem::path k_serverConfigFilePath = k_dataDir / "server_config.json";

const std::string k_serverConfigBackupSuffix = "__server_config_backup.json";
constexpr const char* k_serverConfigBackupTimeFormat = "%Y-%m-%d_%H:%M:%S";
const std::string k_serverConfigTimeRegex = "\\d{4}-\\d{2}-\\d{2}_\\d{2}:\\d{2}:\\d{2}";
const std::chrono::hours k_serverConfigBackupExpiry = std::chrono::hours(24 * 14);

#endif /* __UTIL_H__ */
