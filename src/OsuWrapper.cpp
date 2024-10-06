#include "OsuWrapper.h"

#include <dpp/nlohmann/json.hpp>

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cmath>

/**
 * OsuWrapper constructor.
*/
OsuWrapper::OsuWrapper(const std::string& clientID, const std::string& clientSecret, const int apiCooldownMs)
{
    m_clientID = clientID;
    m_clientSecret = clientSecret;
    m_apiCooldownMs = apiCooldownMs;

    curl_global_init(CURL_GLOBAL_ALL);
    m_curlHandle = curl_easy_init();

    updateToken();
}

/**
 * OsuWrapper destructor.
*/
OsuWrapper::~OsuWrapper() 
{
    curl_easy_cleanup(m_curlHandle);
    curl_global_cleanup();
}

/**
 * Update OsuWrapper token field with new OAauth Client Credentials token.
*/
void OsuWrapper::updateToken()
{
    std::cout << "OsuWrapper::updateToken - requesting OAuth token from osu!API..." << std::endl;

    CURL * tokenCurlHandle = curl_easy_init();
    if (!tokenCurlHandle) {
        std::string reason = "OsuWrapper::updateToken - tokenCurlHandle does not exist!";
        throw std::runtime_error(reason);
    }

    std::string requestURL = "https://osu.ppy.sh/oauth/token";
    curl_easy_setopt(tokenCurlHandle, CURLOPT_URL, requestURL.c_str());

    struct curl_slist* requestHeaders = nullptr;
    requestHeaders = curl_slist_append(requestHeaders, "Content-Type: application/json");
    requestHeaders = curl_slist_append(requestHeaders, "Accept: application/json");
    curl_easy_setopt(tokenCurlHandle, CURLOPT_HTTPHEADER, requestHeaders);

    nlohmann::json requestBodyJson = {
        { "client_id", m_clientID },
        { "client_secret", m_clientSecret },
        { "grant_type", "client_credentials" },
        { "scope", "public" }
    };
    std::string requestBody = requestBodyJson.dump();
    curl_easy_setopt(tokenCurlHandle, CURLOPT_POSTFIELDS, requestBody.c_str());

    std::string responseData;
    curl_easy_setopt(tokenCurlHandle, CURLOPT_WRITEDATA, &responseData);
    curl_easy_setopt(tokenCurlHandle, CURLOPT_WRITEFUNCTION, OsuWrapper::curlWriteCallback);

    CURLcode response = curl_easy_perform(tokenCurlHandle);
    if (response != CURLE_OK) {
        curl_easy_cleanup(tokenCurlHandle);
        std::string reason = std::string("OsuWrapper::updateToken - request failed! response=").append(std::to_string(response));
        throw std::runtime_error(reason);
    }

    long httpCode = 0;
    curl_easy_getinfo(tokenCurlHandle, CURLINFO_RESPONSE_CODE, &httpCode);
    if (httpCode != 200) {
        curl_easy_cleanup(tokenCurlHandle);
        std::string reason = std::string("OsuWrapper::updateToken - request failed! httpCode=").append(std::to_string(httpCode));
        throw std::runtime_error(reason);
    }

    curl_slist_free_all(requestHeaders);

    nlohmann::json responseDataJson = nlohmann::json::parse(responseData);
    if (!responseDataJson.is_object()) {
        curl_easy_cleanup(tokenCurlHandle);
        std::string reason = std::string("OsuWrapper::updateToken - responseDataJson is not an object! responseData=").append(responseData);
        throw std::runtime_error(reason);
    }

    try {
        m_accessToken = responseDataJson.at("access_token").get<std::string>();
    } catch (const std::exception& e) {
        curl_easy_cleanup(tokenCurlHandle);
        std::string reason = std::string("OsuWrapper::updateToken - could not access responseDataJson fields! responseData=").append(responseData);
        throw std::out_of_range(reason);
    }
}

/**
 * Get osu! ranking user IDs.
 * Page=0 returns users ranked #1-#50, page=1 returns users ranked #51-#100, etc.
 * Data is returned in userIDs.
 * Returns true if request succeeds, false if otherwise.
*/
bool OsuWrapper::getRankingIDs(Page page, UserID userIDs[], size_t numIDs) 
{
    std::cout << "OsuWrapper::getRankingIDs - requesting ranking data from osu!API..." << std::endl;

    if (page > 200) {
        std::string reason = std::string("OsuWrapper::getRankingIDs - page cannot be greater than 200! page=").append(std::to_string(page));
        throw std::invalid_argument(reason);
    }
    if (numIDs > 50) {
        std::string reason = std::string("OsuWrapper::getRankingIDs - numIDs cannot be greater than 50! numIDs=").append(std::to_string(numIDs));
        throw std::invalid_argument(reason);
    }
    if (!m_curlHandle) {
        std::string reason = "OsuWrapper::getRankingIDs - m_curlHandle does not exist!";
        throw std::runtime_error(reason);
    }

    curl_easy_reset(m_curlHandle);

    std::string requestURL = std::string("https://osu.ppy.sh/api/v2/rankings/osu/performance?page=").append(std::to_string(static_cast<int>(page)));
    curl_easy_setopt(m_curlHandle, CURLOPT_URL, requestURL.c_str());

    struct curl_slist* requestHeaders = nullptr;
    std::string authString = "Authorization: Bearer ";
    authString = authString.append(m_accessToken);
    requestHeaders = curl_slist_append(requestHeaders, "Content-Type: application/json");
    requestHeaders = curl_slist_append(requestHeaders, "Accept: application/json");
    requestHeaders = curl_slist_append(requestHeaders, authString.c_str());
    curl_easy_setopt(m_curlHandle, CURLOPT_HTTPHEADER, requestHeaders);

    std::string responseData;
    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &responseData);
    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, OsuWrapper::curlWriteCallback);

    curl_easy_setopt(m_curlHandle, CURLOPT_HTTPGET, 1L);

    if (!makeRequest()) {
        return false;
    }
    
    curl_slist_free_all(requestHeaders);

    nlohmann::json responseDataJson = nlohmann::json::parse(responseData);
    if (!responseDataJson.is_object()) {
        std::string reason = std::string("OsuWrapper::getRankingIDs - responseDataJson is not an object! responseData=").append(responseData);
        throw std::runtime_error(reason);
    }

    for (size_t i = 0; i < numIDs; ++i) {
        try {
            UserID userID = responseDataJson.at("ranking")[i]
                                            .at("user")
                                            .at("id")
                                            .get<uint32_t>();
            userIDs[i] = userID;
        } catch (const std::exception& e) {
            std::string reason = std::string("OsuWrapper::getRankingIDs - could not access responseDataJson fields! responseData=").append(responseData);
            throw std::out_of_range(reason);
        }
    }
    
    return true;
}

/**
 * Get osu! user given their ID.
*/
bool OsuWrapper::getUser(UserID userID, DosuUser& user) 
{
    std::cout << "OsuWrapper::getUser - requesting data from osu!API for user with ID " << userID << "..." << std::endl;
    
    if (!m_curlHandle) {
        std::string reason = "OsuWrapper::getUser - m_curlHandle does not exist!";
        throw std::runtime_error(reason);
    }

    curl_easy_reset(m_curlHandle);

    std::string requestURL = std::string("https://osu.ppy.sh/api/v2/users/").append(std::to_string(static_cast<int>(userID))).append("/osu?key=id");
    curl_easy_setopt(m_curlHandle, CURLOPT_URL, requestURL.c_str());

    struct curl_slist* requestHeaders = nullptr;
    std::string authString = "Authorization: Bearer ";
    authString = authString.append(m_accessToken);
    requestHeaders = curl_slist_append(requestHeaders, "Content-Type: application/json");
    requestHeaders = curl_slist_append(requestHeaders, "Accept: application/json");
    requestHeaders = curl_slist_append(requestHeaders, authString.c_str());
    curl_easy_setopt(m_curlHandle, CURLOPT_HTTPHEADER, requestHeaders);

    std::string responseData;
    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &responseData);
    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, OsuWrapper::curlWriteCallback);

    curl_easy_setopt(m_curlHandle, CURLOPT_HTTPGET, 1L);

    if (!makeRequest()) {
        return false;
    }

    curl_slist_free_all(requestHeaders);

    nlohmann::json responseDataJson = nlohmann::json::parse(responseData);
    if (!responseDataJson.is_object()) {
        std::string reason = std::string("OsuWrapper::getUser - responseDataJson is not an object! responseData=").append(responseData);
        throw std::runtime_error(reason);
    }
    
    try {
        user.userID = userID;
        user.username = responseDataJson.at("username").get<std::string>();
        std::string countryCodeStr = responseDataJson.at("country_code").get<std::string>();
        std::strncpy(user.countryCode, countryCodeStr.c_str(), sizeof(user.countryCode) - 1);
        user.countryCode[sizeof(user.countryCode) - 1] = '\0';
        user.pfpLink = responseDataJson.at("avatar_url").get<std::string>();
        user.currentRank = responseDataJson.at("rank_history")
                                        .at("data")[0]
                                        .get<uint32_t>();
        user.yesterdayRank = responseDataJson.at("rank_history")
                                        .at("data")[1]
                                        .get<uint32_t>();
        
    } catch (const std::exception& e) {
        std::string reason = std::string("OsuWrapper::getUser - could not access responseDataJson fields! responseData=").append(responseData);
        throw std::out_of_range(reason);
    }

    return true;
}

/**
 * Make CURL request. 
 * If request gets ratelimited or a server error occurs, waits according to [exponential backoff](https://cloud.google.com/iot/docs/how-tos/exponential-backoff).
 * Status code logic is implemented according to [osu-web](https://github.com/ppy/osu-web/blob/master/resources/lang/en/layout.php).
 * Return true if request succeeds, false if not.
 */
bool OsuWrapper::makeRequest()
{
    if (!m_curlHandle) {
        std::string reason = "OsuWrapper::makeRequest - m_curlHandle does not exist!";
        throw std::runtime_error(reason);
    }

    size_t retries = 0;

    while (true) {
        long httpCode = 0;

        int delayMs = m_apiCooldownMs;
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        CURLcode response = curl_easy_perform(m_curlHandle);

        if (response == CURLE_OK) {
            curl_easy_getinfo(m_curlHandle, CURLINFO_RESPONSE_CODE, &httpCode);
        } else {
            std::cout << "OsuWrapper::makeRequest - request failed; got CURLcode " << std::to_string(response) << std::endl;
            return false;
        }

        // 200 OK
        if (httpCode == 200) {
            return true;
        // 401 Unauthorized - refresh token
        } else if (httpCode == 401) {
            updateToken();
        // 404 Not Found
        } else if (httpCode == 404) {
            return false;
        // 429 Too Many Requests / 5XX Internal Server Error - increase wait time
        } else if ((httpCode == 429) || (std::to_string(httpCode)[0] == '5')) {
            if (delayMs >= 64000) {
                double offset = ((double)rand() / RAND_MAX) * 1000;
                delayMs = 64000 + std::round(offset);
            } else {
                double offset = (double)rand() / RAND_MAX;
                delayMs = (std::pow(2, retries) + offset) * 1000;
            }

            std::cout << "OsuWrapper::makeRequest - request failed; retrying in " << std::to_string(delayMs) << "ms..." << std::endl;
            ++retries;
        } else {
            std::cout << "OsuWrapper::makeRequest - received unhandled response code " << std::to_string(httpCode) << std::endl;
            return false;
        }
    }
}

/**
 * Write callback function for CURL requests.
*/
size_t OsuWrapper::curlWriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) 
{
    size_t totalSize = size * nmemb;
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}
