#include "OsuWrapper.h"
#include "Logger.h"

#include <dpp/nlohmann/json.hpp>

#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>

/**
 * OsuWrapper constructor.
 */
OsuWrapper::OsuWrapper(const std::string& clientID, const std::string& clientSecret, const int apiCooldownMs)
{
    LOG_DEBUG("Constructing OsuWrapper instance...");

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
    LOG_DEBUG("Requesting OAuth token from osu!API...");

    CURL * tokenCurlHandle = curl_easy_init();
    if (!tokenCurlHandle)
    {
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
    if (response != CURLE_OK)
    {
        curl_easy_cleanup(tokenCurlHandle);
        std::string reason = std::string("OsuWrapper::updateToken - request failed; got CURLcode ").append(std::to_string(response));
        throw std::runtime_error(reason);
    }

    long httpCode = 0;
    curl_easy_getinfo(tokenCurlHandle, CURLINFO_RESPONSE_CODE, &httpCode);
    if (httpCode != 200)
    {
        curl_easy_cleanup(tokenCurlHandle);
        std::string reason = std::string("OsuWrapper::updateToken - request failed; got HTTP code ").append(std::to_string(httpCode));
        throw std::runtime_error(reason);
    }

    curl_slist_free_all(requestHeaders);

    nlohmann::json responseDataJson = nlohmann::json::parse(responseData);
    if (!responseDataJson.is_object())
    {
        curl_easy_cleanup(tokenCurlHandle);
        std::string reason = std::string("OsuWrapper::updateToken - responseDataJson is not an object! responseData=").append(responseData);
        throw std::runtime_error(reason);
    }

    try
    {
        m_accessToken = responseDataJson.at("access_token").get<OAuthToken>();
    }
    catch (const std::exception& e)
    {
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
bool OsuWrapper::getRankingIDs(Page page, std::vector<UserID>& userIDs, std::size_t numIDs)
{
    page += 1;

    LOG_DEBUG("Requesting page ", static_cast<int>(page), " ranking data from osu!API...");

    if (page > k_getRankingIDMaxPage)
    {
        std::string reason = std::string("OsuWrapper::getRankingIDs - page cannot be greater than k_getRankingIDMaxPage! page=").append(std::to_string(page));
        throw std::invalid_argument(reason);
    }
    if (numIDs > k_getRankingIDMaxNumIDs)
    {
        std::string reason = std::string("OsuWrapper::getRankingIDs - numIDs cannot be greater than k_getRankingIDMaxNumIDs! numIDs=").append(std::to_string(numIDs));
        throw std::invalid_argument(reason);
    }
    if (!m_curlHandle)
    {
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

    if (!makeRequest())
    {
        return false;
    }
    
    curl_slist_free_all(requestHeaders);

    nlohmann::json responseDataJson = nlohmann::json::parse(responseData);
    if (!responseDataJson.is_object())
    {
        std::string reason = std::string("OsuWrapper::getRankingIDs - responseDataJson is not an object! responseData=").append(responseData);
        throw std::runtime_error(reason);
    }

    for (std::size_t i = 0; i < numIDs; ++i)
    {
        try
        {
            UserID userID = responseDataJson.at("ranking")[i]
                                            .at("user")
                                            .at("id")
                                            .get<UserID>();
            userIDs.push_back(userID);
        }
        catch (const std::exception& e)
        {
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
    LOG_DEBUG("Requesting data from osu!API for user with ID ", userID, "...");
    
    if (!m_curlHandle)
    {
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

    if (!makeRequest())
    {
        return false;
    }

    curl_slist_free_all(requestHeaders);

    nlohmann::json responseDataJson = nlohmann::json::parse(responseData);
    if (!responseDataJson.is_object())
    {
        std::string reason = std::string("OsuWrapper::getUser - responseDataJson is not an object! responseData=").append(responseData);
        throw std::runtime_error(reason);
    }
    
    try
    {
        user.userID = userID;
        user.username = responseDataJson.at("username").get<Username>();
        user.countryCode = responseDataJson.at("country_code").get<CountryCode>();
        user.pfpLink = responseDataJson.at("avatar_url").get<ProfilePicture>();
        user.currentRank = responseDataJson.at("rank_history").at("data")[89].get<Rank>();
        user.yesterdayRank = responseDataJson.at("rank_history").at("data")[88].get<Rank>();
        user.hoursPlayed = responseDataJson.at("statistics").at("play_time").get<uint64_t>() / 3600;
        user.performancePoints = responseDataJson.at("statistics").at("pp").get<PerformancePoints>();
        user.accuracy = responseDataJson.at("statistics").at("hit_accuracy").get<Accuracy>();
    }
    catch (const std::exception& e)
    {
        std::string reason = std::string("OsuWrapper::getUser - could not access responseDataJson fields! responseData=").append(responseData);
        throw std::out_of_range(reason);
    }

    return true;
}

/**
 * WARNING: It is possible for this function to retry forever!
 *
 * Make CURL request.
 * If request gets ratelimited or a server error occurs, waits according to [exponential backoff](https://cloud.google.com/iot/docs/how-tos/exponential-backoff) then retries.
 * If the request itself fails (e.g. no internet connection), waits for a while and retries.
 * Status code logic is implemented according to [osu-web](https://github.com/ppy/osu-web/blob/master/resources/lang/en/layout.php).
 * Return true if request succeeds, false if not.
 */
bool OsuWrapper::makeRequest()
{
    if (!m_curlHandle)
    {
        std::string reason = "OsuWrapper::makeRequest - m_curlHandle does not exist!";
        throw std::runtime_error(reason);
    }

    std::size_t retries = 0;
    int delayMs = m_apiCooldownMs;
    while (true)
    {
        long httpCode = 0;

        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        CURLcode response = curl_easy_perform(m_curlHandle);

        // Handle CURL code
        if (response == CURLE_OK) // Server responded
        {
            curl_easy_getinfo(m_curlHandle, CURLINFO_RESPONSE_CODE, &httpCode);
        }
        else // Something bad happened, ideally connectivity issues
        {
            int waitMs = k_curlRetryWaitMs - delayMs;
            if (waitMs < 0)
            {
                waitMs = 0;
            }

            LOG_WARN("Request failed; got CURLcode ", std::to_string(response), ". Retrying in ", waitMs + delayMs, "ms...");
            std::this_thread::sleep_for(std::chrono::milliseconds(waitMs));
            continue;
        }

        // Handle HTTP code
        if (httpCode == 200) // 200 OK
        {
            return true;
        }
        else if (httpCode == 401) // 401 Unauthorized - refresh token
        {
            updateToken();
        }
        else if (httpCode == 404) // 404 Not Found
        {
            return false;
        }
        else if ((httpCode == 429) || (std::to_string(httpCode)[0] == '5')) // 429 Too Many Requests / 5XX Internal Server Error - increase wait time
        {
            if (delayMs >= 64000)
            {
                double offset = ((double)rand() / RAND_MAX) * 1000;
                delayMs = 64000 + std::round(offset);
            }
            else
            {
                double offset = (double)rand() / RAND_MAX;
                delayMs = (std::pow(2, retries) + offset) * 1000;
            }

            LOG_WARN("Request failed; retrying in ", std::to_string(delayMs), "ms...");
            ++retries;
        }
        else
        {
            LOG_ERROR("Made request and received unhandled HTTP code ", std::to_string(httpCode), "!");
            return false;
        }
    }
}

/**
 * Write callback function for CURL requests.
 */
std::size_t OsuWrapper::curlWriteCallback(void* contents, std::size_t size, std::size_t nmemb, std::string* response) 
{
    std::size_t totalSize = size * nmemb;
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}
