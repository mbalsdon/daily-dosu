#include "OsuWrapper.h"
#include "Logger.h"
#include "TokenManager.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <cmath>

/**
 * OsuWrapper constructor.
 */
OsuWrapper::OsuWrapper(const int apiCooldownMs)
{
    LOG_DEBUG("Constructing OsuWrapper...");

    m_apiCooldownMs = apiCooldownMs;

    m_curlHandle = curl_easy_init();
    if (!m_curlHandle)
    {
        std::string reason = "Failed to initialize CURL handle in OsuWrapper!";
        throw std::runtime_error(reason);
    }

    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &m_responseData);
    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, OsuWrapper::curlWriteCallback);
}

/**
 * OsuWrapper destructor.
 */
OsuWrapper::~OsuWrapper()
{
    LOG_DEBUG("Destructing OsuWrapper...");

    if (m_curlHandle)
    {
        curl_easy_cleanup(m_curlHandle);
    }
}

/**
 * Get osu! rankings.
 * Page=0 returns users ranked #1-#50, page=1 returns users ranked #51-#100, etc.
 * Data is returned in rankings.
 * Returns true if request succeeds, false if otherwise.
 */
bool OsuWrapper::getRankings(Page page, Gamemode mode, nlohmann::json& rankings)
{
    page += 1;

    LOG_DEBUG("Requesting page ", static_cast<int>(page), " ", mode.toString(), " ranking data from osu!API...");

    if (page > k_getRankingIDMaxPage)
    {
        std::string reason = std::string("OsuWrapper::getRankingIDs - page cannot be greater than k_getRankingIDMaxPage! page=").append(std::to_string(page));
        throw std::invalid_argument(reason);
    }

    std::string requestURL = "https://osu.ppy.sh/api/v2/rankings/" + mode.toString() + "/performance?page=" + std::to_string(static_cast<int>(page));
    bool bSuccess = makeRequest(requestURL, CURLOPT_HTTPGET);
    if (!bSuccess)
    {
        return false;
    }

    nlohmann::json responseDataJson = nlohmann::json::parse(m_responseData);
    m_responseData.clear();
    if (!responseDataJson.is_object())
    {
        std::string reason = std::string("OsuWrapper::getRankingIDs - responseDataJson is not an object! responseDataJson=").append(responseDataJson.dump());
        throw std::runtime_error(reason);
    }

    rankings = responseDataJson;
    return true;
}

/**
 * Get osu! user given their ID.
 */
bool OsuWrapper::getUser(UserID userID, Gamemode mode, nlohmann::json& user)
{
    LOG_DEBUG("Requesting data from osu!API for ", mode.toString(), " user with ID ", userID, "...");

    std::string requestURL = "https://osu.ppy.sh/api/v2/users/" + std::to_string(userID) + "/" + mode.toString() + "?key=id";
    bool bSuccess = makeRequest(requestURL, CURLOPT_HTTPGET);
    if (!bSuccess)
    {
        return false;
    }

    nlohmann::json responseDataJson = nlohmann::json::parse(m_responseData);
    m_responseData.clear();
    if (!responseDataJson.is_object())
    {
        std::string reason = std::string("OsuWrapper::getUser - responseDataJson is not an object! responseDataJson=").append(responseDataJson.dump());
        throw std::runtime_error(reason);
    }

    user = responseDataJson;
    return true;
}

/**
 * Get osu! user's scores on a beatmap.
 */
bool OsuWrapper::getUserBeatmapScores(Gamemode const& mode, UserID const& userID, BeatmapID const& beatmapID, nlohmann::json& userBeatmapScores /* out */)
{
    LOG_DEBUG("Requesting ", mode.toString(), " scores from osu!API for user ", userID, " on beatmap ", beatmapID, "...");

    std::string requestURL = "https://osu.ppy.sh/api/v2/beatmaps/" + std::to_string(beatmapID) + "/scores/users/" + std::to_string(userID) + "/all?ruleset=" + mode.toString();
    bool bSuccess = makeRequest(requestURL, CURLOPT_HTTPGET);
    if (!bSuccess)
    {
        return false;
    }

    nlohmann::json responseDataJson = nlohmann::json::parse(m_responseData);
    m_responseData.clear();
    LOG_ERROR_THROW(responseDataJson.is_object(), "ResponseDataJson is not an object! responseDataJson=", responseDataJson.dump());

    userBeatmapScores = responseDataJson;
    return true;
}

/**
 * Get osu! beatmap
 */
bool OsuWrapper::getBeatmap(BeatmapID const& beatmapID, nlohmann::json& beatmap /* out */)
{
    LOG_DEBUG("Requesting beatmap ", beatmapID, " from osu!API...");

    std::string requestURL = "https://osu.ppy.sh/api/v2/beatmaps/" + std::to_string(beatmapID);
    bool bSuccess = makeRequest(requestURL, CURLOPT_HTTPGET);
    if (!bSuccess)
    {
        return false;
    }

    nlohmann::json responseDataJson = nlohmann::json::parse(m_responseData);
    m_responseData.clear();
    LOG_ERROR_THROW(responseDataJson.is_object(), "ResponseDataJson is not an object! responseDataJson=", responseDataJson.dump());

    beatmap = responseDataJson;
    return true;
}


/**
 * Get osu! beatmap attributes after applying mods.
 */
bool OsuWrapper::getBeatmapAttributes(BeatmapID const& beatmapID, Gamemode const& mode, OsuMods const& mods, nlohmann::json& attributes /* out */)
{
    LOG_DEBUG("Requesting attributes for beatmap ", beatmapID, " with mods ", mods.toString(), " from osu!API...");

    std::string requestURL = "https://osu.ppy.sh/api/v2/beatmaps/" + std::to_string(beatmapID) + "/attributes";
    nlohmann::json requestBodyJson = {
        { "mods", mods.get() },
        { "ruleset", mode.toString() }
    };
    std::string requestBody = requestBodyJson.dump();
    curl_easy_setopt(m_curlHandle, CURLOPT_POSTFIELDS, requestBody.c_str());
    bool bSuccess = makeRequest(requestURL, CURLOPT_POST);
    curl_easy_setopt(m_curlHandle, CURLOPT_POSTFIELDS, NULL); // FIXME: Bad practice - when we refactor this class use reset instead
    if (!bSuccess)
    {
        return false;
    }

    nlohmann::json responseDataJson = nlohmann::json::parse(m_responseData);
    m_responseData.clear();
    LOG_ERROR_THROW(responseDataJson.is_object(), "ResponseDataJson is not an object! responseDataJson=", responseDataJson.dump());

    attributes = responseDataJson;
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
bool OsuWrapper::makeRequest(const std::string requestURL, const CURLoption method)
{
    curl_easy_setopt(m_curlHandle, CURLOPT_URL, requestURL.c_str());
    curl_easy_setopt(m_curlHandle, method, 1L);

    std::size_t retries = 0;
    int delayMs = m_apiCooldownMs;
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));

        m_responseData.clear();

        struct curl_slist* requestHeaders = nullptr;
        std::string authString = "Authorization: Bearer ";
        authString = authString.append(TokenManager::getInstance().getAccessToken());
        requestHeaders = curl_slist_append(requestHeaders, "Content-Type: application/json");
        requestHeaders = curl_slist_append(requestHeaders, "Accept: application/json");
        requestHeaders = curl_slist_append(requestHeaders, authString.c_str());
        curl_easy_setopt(m_curlHandle, CURLOPT_HTTPHEADER, requestHeaders);

        long httpCode = 0;
        CURLcode response = curl_easy_perform(m_curlHandle);
        curl_slist_free_all(requestHeaders);

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

            LOG_WARN("Request failed; ", curl_easy_strerror(response), ". Retrying in ", waitMs + delayMs, "ms...");
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
            LOG_WARN("Got 401, refreshing OAuth token...");
            TokenManager::getInstance().updateAccessToken();
        }
        else if (httpCode == 404) // 404 Not Found
        {
            LOG_ERROR("Got 404 response from ", requestURL);
            return false;
        }
        else if ((httpCode == 429) || (std::to_string(httpCode)[0] == '5')) // 429 Too Many Requests / 5XX Internal Server Error - increase wait time
        {
            if (delayMs >= 64000)
            {
                double offset = (static_cast<double>(rand()) / RAND_MAX) * 1000;
                delayMs = static_cast<int>(64000. + std::round(offset));
            }
            else
            {
                double offset = static_cast<double>(rand()) / RAND_MAX;
                delayMs = static_cast<int>((std::pow(2, retries) + offset) * 1000.);
            }

            LOG_WARN("Request failed (", httpCode, "); retrying in ", std::to_string(delayMs), "ms...");
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
