#include "TokenManager.h"
#include "Logger.h"

#include <dpp/nlohmann/json.hpp>

/**
 * Initialize TokenManager instance.
 */
void TokenManager::initialize(const std::string clientID, const std::string clientSecret)
{
    std::lock_guard<std::mutex> lock(m_initMtx);
    LOG_INFO("Initializing TokenManager (clientID=", clientID, ", clientSecret.length()=", clientSecret.length(), ")");

    if (m_bIsInitialized)
    {
        LOG_WARN("TokenManager is already initialized!");
        return;
    }

    m_clientID = clientID;
    m_clientSecret = clientSecret;

    m_curlHandle = curl_easy_init();
    if (!m_curlHandle)
    {
        std::string reason = "Failed to initialize CURL handle in TokenManager!";
        throw std::runtime_error(reason);
    }

    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &m_responseData);
    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, TokenManager::curlWriteCallback);

    m_bIsInitialized = true;
}

/**
 * Clean up resources.
 */
void TokenManager::cleanup()
{
    std::lock_guard<std::mutex> lock(m_initMtx);
    LOG_INFO("Cleaning up TokenManager...");

    if (!m_bIsInitialized || !m_curlHandle)
    {
        LOG_WARN("TokenManager is not initialized!");
        return;
    }

    curl_easy_cleanup(m_curlHandle);
    m_bIsInitialized = false;
}

/**
 * Retrieve token.
 * If it is being updated, wait for it.
 */
std::string TokenManager::getAccessToken()
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
    LOG_DEBUG("Attempting to update access token...");

    if (updateLock.owns_lock())
    {
        std::unique_lock<std::shared_mutex> tokenLock(m_tokenMtx);
        LOG_INFO("Updating access token...");

        curl_easy_setopt(m_curlHandle, CURLOPT_URL, "https://osu.ppy.sh/oauth/token");
        curl_easy_setopt(m_curlHandle, CURLOPT_POST, 1L);
        nlohmann::json requestBodyJson = {
            { "client_id", m_clientID },
            { "client_secret", m_clientSecret },
            { "grant_type", "client_credentials" },
            { "scope", "public" }
        };
        std::string requestBody = requestBodyJson.dump();
        curl_easy_setopt(m_curlHandle, CURLOPT_POSTFIELDS, requestBody.c_str());

        while (true)
        {
            m_responseData.clear();

            struct curl_slist* requestHeaders = nullptr;
            requestHeaders = curl_slist_append(requestHeaders, "Content-Type: application/json");
            requestHeaders = curl_slist_append(requestHeaders, "Accept: application/json");
            curl_easy_setopt(m_curlHandle, CURLOPT_HTTPHEADER, requestHeaders);

            long httpCode = 0;
            CURLcode response = curl_easy_perform(m_curlHandle);
            curl_slist_free_all(requestHeaders);

            if (response == CURLE_OK)
            {
                curl_easy_getinfo(m_curlHandle, CURLINFO_RESPONSE_CODE, &httpCode);
            }
            else
            {
                LOG_WARN("Request failed; got CURLcode ", std::to_string(response), ". Retrying in ", k_curlTokenWaitMs, "ms...");
                std::this_thread::sleep_for(std::chrono::milliseconds(k_curlTokenWaitMs));
                continue;
            }

            if (httpCode == 200)
            {
                break;
            }
            else if (std::to_string(httpCode)[0] == '5')
            {
                LOG_WARN("Request failed; got HTTP code ", std::to_string(httpCode), ". Retrying in ", k_curlTokenWaitMs, "ms...");
                std::this_thread::sleep_for(std::chrono::milliseconds(k_curlTokenWaitMs));
                continue;
            }
            else
            {
                std::string reason = std::string("Request failed; got HTTP code ").append(std::to_string(httpCode));
                throw std::runtime_error(reason);
            }
        }

        nlohmann::json responseDataJson = nlohmann::json::parse(m_responseData);
        m_responseData.clear();
        if (!responseDataJson.is_object())
        {
            std::string reason = std::string("Response data is not an object! responseDataJson=").append(responseDataJson.dump());
            throw std::runtime_error(reason);
        }

        try
        {
            m_accessToken = responseDataJson.at("access_token").get<std::string>();
        }
        catch(const std::exception& e)
        {
            std::string reason = std::string("Tried to retrieve access_token from JSON response but failed; ").append(e.what());
            throw std::runtime_error(reason);
        }

        return;
    }
    else
    {
        LOG_DEBUG("Someone is already updating the token!");
        std::shared_lock<std::shared_mutex> tokenLock(m_tokenMtx);
        return;
    }

}

/**
 * TokenManager destructor.
 */
TokenManager::~TokenManager()
{
    LOG_DEBUG("Destructing TokenManager instance...");
    cleanup();
}

/**
 * Write callback function for CURL requests.
 */
std::size_t TokenManager::curlWriteCallback(void *contents, std::size_t size, std::size_t nmemb, std::string *response)
{
    std::size_t totalSize = size * nmemb;
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}
