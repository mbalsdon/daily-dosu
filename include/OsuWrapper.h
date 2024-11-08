#ifndef __OSU_WRAPPER_H__
#define __OSU_WRAPPER_H__

#include <curl/curl.h>
#include <dpp/nlohmann/json.hpp>

#include <string>
#include <cstdint>
#include <vector>

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

const int k_curlRetryWaitMs = 30000;

constexpr size_t k_getRankingIDMaxNumIDs = 50;
constexpr size_t k_getRankingIDMaxPage = 200;

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

class OsuWrapper 
{
public:
    OsuWrapper(const std::string& clientID, const std::string& clientSecret, const int apiCooldownMs);
    ~OsuWrapper();

    bool getRankingIDs(Page page, std::vector<UserID>& userIDs, size_t numIDs);
    bool getUser(UserID userID, DosuUser& user);

private:
    void updateToken();
    bool makeRequest();
    static size_t curlWriteCallback(void *contents, size_t size, size_t nmemb, std::string *response);

    CURL* m_curlHandle;
    std::string m_clientID;
    std::string m_clientSecret;
    std::string m_clientRedirectURI;
    int m_apiCooldownMs;
    OAuthToken m_accessToken;
};

#endif /* __OSU_WRAPPER_H__ */
