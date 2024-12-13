#ifndef __TOKEN_MANAGER_H__
#define __TOKEN_MANAGER_H__

#include <curl/curl.h>

#include <string>
#include <cstddef>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>

const int k_curlTokenWaitMs = 10000;

/**
 * Osu!API v2 helper class for OAuth token management. Enables us to parallelize network requests
 * while sharing the same token.
 */
class TokenManager
{
public:
    static TokenManager& getInstance()
    {
        static TokenManager instance;
        return instance;
    }

    void initialize(const std::string clientID, const std::string clientSecret);
    void cleanup();

    std::string getAccessToken();
    void updateAccessToken();

private:
    TokenManager()
    : m_bIsInitialized(false)
    {}

    ~TokenManager();

    TokenManager(const TokenManager&) = delete;
    TokenManager(TokenManager&&) = delete;
    TokenManager& operator=(const TokenManager&) = delete;
    TokenManager& operator=(TokenManager&&) = delete;

    static std::size_t curlWriteCallback(void *contents, std::size_t size, std::size_t nmemb, std::string *response);

    std::string m_clientID;
    std::string m_clientSecret;
    std::atomic<bool> m_bIsInitialized;

    struct curl_slist* m_requestHeaders;
    CURL* m_curlHandle;
    std::string m_responseData;

    std::mutex m_initMtx;

    std::string m_accessToken;
    std::shared_mutex m_tokenMtx;
    std::mutex m_updateMtx;
};

#endif /* __TOKEN_MANAGER_H__ */
