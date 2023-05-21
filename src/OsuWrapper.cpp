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
OsuWrapper::OsuWrapper(const std::string& clientID, const std::string& clientSecret) 
{
    curl_global_init(CURL_GLOBAL_ALL);
    m_curlHandle = curl_easy_init();
    bool tokenUpdated = updateToken(clientID, clientSecret);
    if (!tokenUpdated) {
        std::string reason = "OsuWrapper::OsuWrapper - failed to update token!";
        throw std::runtime_error(reason);
    }
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
 * Return true if request succeeds, false if not.
*/
bool OsuWrapper::updateToken(const std::string& clientID, const std::string& clientSecret) 
{
    std::cout << "OsuWrapper::updateToken - requesting OAuth token from osu!API..." << std::endl;

    if (!m_curlHandle) {
        std::string reason = "OsuWrapper::updateToken - m_curlHandle does not exist!";
        throw std::runtime_error(reason);
    }

    curl_easy_reset(m_curlHandle);

    std::string requestURL = "https://osu.ppy.sh/oauth/token";
    curl_easy_setopt(m_curlHandle, CURLOPT_URL, requestURL.c_str());
    
    struct curl_slist* requestHeaders = nullptr;
    requestHeaders = curl_slist_append(requestHeaders, "Content-Type: application/json");
    requestHeaders = curl_slist_append(requestHeaders, "Accept: application/json");
    curl_easy_setopt(m_curlHandle, CURLOPT_HTTPHEADER, requestHeaders);

    nlohmann::json requestBodyJson = {
        { "client_id", clientID },
        { "client_secret", clientSecret },
        { "grant_type", "client_credentials" },
        { "scope", "public" }
    };
    std::string requestBody = requestBodyJson.dump();
    curl_easy_setopt(m_curlHandle, CURLOPT_POSTFIELDS, requestBody.c_str());

    std::string responseData;
    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &responseData);
    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, OsuWrapper::curlWriteCallback);

    if (!makeRequest(4)) {
        return false;
    }

    curl_slist_free_all(requestHeaders);

    nlohmann::json responseDataJson = nlohmann::json::parse(responseData);
    if (!responseDataJson.is_object()) {
        std::string reason = std::string("OsuWrapper::getToken - responseDataJson is not an object! responseData=").append(responseData);
        throw std::runtime_error(reason);
    }

    try {
        m_accessToken = responseDataJson.at("access_token").get<std::string>();
    } catch (const std::exception& e) {
        std::string reason = std::string("OsuWrapper::updateToken - could not access responseDataJson fields! responseData=").append(responseData);
        throw std::out_of_range(reason);
    }

    return true;
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

    if (!makeRequest(4)) {
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

    if (!makeRequest(4)) {
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
 * If request fails, attempt again up to specified number of tries.
 * Uses exponential backoff to calculate time between requests with a base of 3 seconds.
 * Return true if request succeeds, false if not.
*/
bool OsuWrapper::makeRequest(size_t maxRetries) 
{
    if (!m_curlHandle) {
        std::string reason = "OsuWrapper::makeRequest - m_curlHandle does not exist!";
        throw std::runtime_error(reason);
    }

    size_t retries = 0;
    uint8_t delaySec = 1;

    while (retries < maxRetries) {
        CURLcode response = curl_easy_perform(m_curlHandle);
        std::this_thread::sleep_for(std::chrono::seconds(delaySec));
        if (response == CURLE_OK) {
            return true;
        } else {
            ++retries;
            delaySec = std::pow(3, retries);
            if (retries < maxRetries) {
                std::cout << "OsuWrapper::makeRequest - request failed; retrying in " << std::to_string(delaySec) << "s..." << std::endl;
            }
        }
    }

    std::cout << "OsuWrapper::makeRequest - exceeded the maximum number of retries. Aborting..." << std::endl;
    return false;
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
