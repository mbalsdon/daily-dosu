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
    OsuWrapper(const int apiCooldownMs);
    ~OsuWrapper();

    bool getRankings(Page page, Gamemode mode, nlohmann::json& rankings);
    bool getUser(UserID userID, Gamemode mode, nlohmann::json& user);

private:
    bool makeRequest(const std::string requestURL, const CURLoption method);
    static std::size_t curlWriteCallback(void *contents, std::size_t size, std::size_t nmemb, std::string *response);

    CURL* m_curlHandle;
    std::string m_responseData;

    int m_apiCooldownMs;
};

#endif /* __OSU_WRAPPER_H__ */
