#ifndef __OSU_WRAPPER_H__
#define __OSU_WRAPPER_H__

#include <curl/curl.h>

#include <string>
#include <cstdint>

typedef uint8_t Page;
typedef uint32_t UserID;
typedef std::string OAuthToken;
typedef std::string Username;
typedef char CountryCode[3];
typedef std::string ProfilePicture;
typedef uint32_t Rank;

struct DosuUser
{
    UserID userID;
    Username username;
    CountryCode countryCode;
    ProfilePicture pfpLink;
    Rank currentRank;
    Rank yesterdayRank;
};

class OsuWrapper 
{
public:
    OsuWrapper(const std::string& clientID, const std::string& clientSecret, const int apiCooldownMs);
    ~OsuWrapper();

    bool getRankingIDs(Page page, UserID userIDs[], size_t numIDs);
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
