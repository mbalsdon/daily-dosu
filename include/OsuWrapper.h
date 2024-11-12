#ifndef __OSU_WRAPPER_H__
#define __OSU_WRAPPER_H__

#include "Util.h"

#include <curl/curl.h>

#include <string>
#include <cstddef>
#include <vector>

const int k_curlRetryWaitMs = 30000;

class OsuWrapper 
{
public:
    OsuWrapper(const std::string& clientID, const std::string& clientSecret, const int apiCooldownMs);
    ~OsuWrapper();

    bool getRankingIDs(Page page, std::vector<UserID>& userIDs, std::size_t numIDs);
    bool getUser(UserID userID, DosuUser& user);

private:
    void updateToken();
    bool makeRequest();
    static std::size_t curlWriteCallback(void *contents, std::size_t size, std::size_t nmemb, std::string *response);

    CURL* m_curlHandle;
    std::string m_clientID;
    std::string m_clientSecret;
    int m_apiCooldownMs;
    OAuthToken m_accessToken;
};

#endif /* __OSU_WRAPPER_H__ */
