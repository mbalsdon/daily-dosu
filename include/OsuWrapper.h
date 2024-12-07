#ifndef __OSU_WRAPPER_H__
#define __OSU_WRAPPER_H__

#include "Util.h"

#include <dpp/nlohmann/json.hpp>
#include <curl/curl.h>

#include <string>
#include <cstddef>
#include <vector>
#include <mutex>

const int k_curlRetryWaitMs = 30000;

/**
 * osu!API v2 wrapper. Only implements endpoints necessary for this project,
 * but can be easily extended.
 */
class OsuWrapper 
{
public:
    OsuWrapper(const std::string& clientID, const std::string& clientSecret, const int apiCooldownMs);
    ~OsuWrapper();

    bool getRankings(Page page, Gamemode mode, nlohmann::json& rankings);
    bool getUser(UserID userID, Gamemode mode, nlohmann::json& user);

private:
    void updateToken();
    bool makeRequest();
    static std::size_t curlWriteCallback(void *contents, std::size_t size, std::size_t nmemb, std::string *response);

    CURL* m_curlHandle;
    std::string m_responseData;

    std::string m_clientID;
    std::string m_clientSecret;
    int m_apiCooldownMs;
    OAuthToken m_accessToken;
};

#endif /* __OSU_WRAPPER_H__ */
