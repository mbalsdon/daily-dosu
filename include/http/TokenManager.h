#ifndef __TOKEN_MANAGER_H__
#define __TOKEN_MANAGER_H__

#include "HttpRequester.h"

#include <string>
#include <memory>
#include <shared_mutex>
#include <mutex>

constexpr int k_tokenWaitMs = 10000;

/**
 * Thread-safe class for osu!API v2 OAuth token management.
 */
class TokenManager
{
public:
    TokenManager(std::string const& clientID, std::string const& clientSecret) noexcept;
    ~TokenManager() = default;

    [[nodiscard]] std::string getAccessToken() noexcept;
    void updateAccessToken();

private:
    std::unique_ptr<HttpRequester> m_pHttpRequester = std::make_unique<HttpRequester>();
    std::string m_clientID;
    std::string m_clientSecret;

    std::string m_accessToken;
    std::shared_mutex m_tokenMtx;
    std::mutex m_updateMtx;
};

#endif /* __TOKEN_MANAGER_H__ */
