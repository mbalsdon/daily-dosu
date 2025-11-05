#include "TokenManager.h"
#include "Logger.h"

#include <nlohmann/json.hpp>

#include <vector>

/**
 * TokenManager constructor.
 */
TokenManager::TokenManager(std::string const& clientID, std::string const& clientSecret) noexcept
: m_clientID(clientID)
, m_clientSecret(clientSecret)
{}

/**
 * Retrieve token.
 * If it is being updated, wait for it.
 */
[[nodiscard]] std::string TokenManager::getAccessToken() noexcept
{
    std::shared_lock<std::shared_mutex> lock(m_tokenMtx);
    return m_accessToken;
}

/**
 * Update token.
 * If it is already being updated, wait for it.
 */
void TokenManager::updateAccessToken()
{
    std::unique_lock<std::mutex> updateLock(m_updateMtx, std::try_to_lock);
    LOG_DEBUG("Attempting to update access token");

    if (updateLock.owns_lock())
    {
        std::unique_lock<std::shared_mutex> tokenLock(m_tokenMtx);
        LOG_INFO("Updating access token");

        std::string url = "https://osu.ppy.sh/oauth/token";
        std::string method = "POST";
        std::vector<std::string> headers = {
            "Content-Type: application/json",
            "Accept: application/json"
        };
        nlohmann::json requestBodyJson = {
            { "client_id", m_clientID },
            { "client_secret", m_clientSecret },
            { "grant_type", "client_credentials" },
            { "scope", "public" }
        };

        while (true)
        {
            long httpCode = 0;
            std::string responseData = "";
            if (!m_pHttpRequester->makeRequest(url, method, headers, requestBodyJson.dump(), httpCode, responseData))
            {
                LOG_WARN("Request failed, retrying in ", k_tokenWaitMs, "ms");
                std::this_thread::sleep_for(std::chrono::milliseconds(k_tokenWaitMs));
                continue;
            }

            // 200 OK -> parse and return
            if (httpCode == 200)
            {
                nlohmann::json responseDataJson = nlohmann::json::parse(responseData);
                LOG_ERROR_THROW(
                    responseDataJson.is_object(),
                    "responseDataJson is not an object! responseDataJson=", responseDataJson.dump());

                m_accessToken = responseDataJson.at("access_token").get<std::string>();
                return;
            }
            // 429 Too Many Requests / 5XX Internal Server Error -> wait, then retry
            else if ((httpCode == 429) || (std::to_string(httpCode)[0] == '5'))
            {
                LOG_WARN("Request failed (", httpCode, "); retrying in ", k_tokenWaitMs, "ms");
                std::this_thread::sleep_for(std::chrono::milliseconds(k_tokenWaitMs));
                continue;
            }

            LOG_ERROR_THROW(false, "Request failed; got unhandled HTTP code ", httpCode);
        }
    }
    else /* we don't own the lock */
    {
        // Wait by trying to grab the lock (once we get it, we know the updater finished)
        LOG_DEBUG("Somebody is already updating the token!");
        std::shared_lock<std::shared_mutex> tokenLock(m_tokenMtx);
        return;
    }
}