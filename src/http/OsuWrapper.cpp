#include "OsuWrapper.h"
#include "Logger.h"

#include <thread>

namespace
{
template<typename T>
void appendBatchParams(std::vector<T> const& IDs, std::string& url /* out */)
{
    url += "?";
    for (auto it = IDs.begin(); it != IDs.end(); ++it)
    {
        url += "ids[]=" + std::to_string(*it);
        if (std::next(it) != IDs.end()) url += "&";
    }
}
} /* namespace */

/**
 * OsuWrapper constructor.
 */
OsuWrapper::OsuWrapper(std::shared_ptr<TokenManager> tokenManager, int const& apiCooldownMs) noexcept
: m_pTokenManager(tokenManager)
, m_apiCooldownMs(apiCooldownMs)
{}

/**
 * Get osu! rankings for given gamemode.
 * Page=0 returns users ranked #1-#50, page=1 returns users ranked #51-#100, etc.
 * Data is returned in rankings.
 * Returns true if request succeeds, false if otherwise.
 */
bool OsuWrapper::getRankings(Page page, Gamemode const& mode, nlohmann::json& rankings /* out */)
{
    page += 1;
    LOG_DEBUG("Requesting page ", page, " ", mode.toString(), " user rankings");
    LOG_ERROR_THROW(
        page <= k_getRankingIDMaxPage,
        "page cannot be greater than ", k_getRankingIDMaxPage, "! page=", page
    );
    std::string url = "https://osu.ppy.sh/api/v2/rankings/" + mode.toString() + "/performance?page=" + std::to_string(static_cast<int>(page));
    return apiRequest_(url, "GET", {}, "", rankings);
}

/**
 * Get osu! user for gamemode using their ID.
 */
bool OsuWrapper::getUser(UserID const& userID, Gamemode const& mode, nlohmann::json& user /* out */)
{
    LOG_DEBUG("Requesting data for ", mode.toString(), " user ", userID);
    std::string url = "https://osu.ppy.sh/api/v2/users/" + std::to_string(userID) + "/" + mode.toString() + "?key=id";
    return apiRequest_(url, "GET", {}, "", user);
}

/**
 * Get osu! users for gamemode using their IDs.
 */
bool OsuWrapper::getUsers(std::vector<UserID> const& userIDs, Gamemode const& mode, nlohmann::json& users /* out */)
{
    LOG_DEBUG("Requesting data for ", userIDs.size(), " ", mode.toString(), " users");
    LOG_ERROR_THROW(
        userIDs.size() <= k_batchMaxIDs,
        "Cannot request more than ", k_batchMaxIDs, " users at once! userIDs.size()=", userIDs.size()
    );
    std::string url = "https://osu.ppy.sh/api/v2/users";
    appendBatchParams(userIDs, url);
    return apiRequest_(url, "GET", {}, "", users);
}

/**
 * Get osu! user's scores on a beatmap for given gamemode.
 */
bool OsuWrapper::getUserBeatmapScores(Gamemode const& mode, UserID const& userID, BeatmapID const& beatmapID, nlohmann::json& userBeatmapScores /* out */)
{
    LOG_DEBUG("Requesting ", mode.toString(), " scores from user ", userID, " on beatmap ", beatmapID);
    std::string url = "https://osu.ppy.sh/api/v2/beatmaps/" + std::to_string(beatmapID) + "/scores/users/" + std::to_string(userID) + "/all?ruleset=" + mode.toString();
    return apiRequest_(url, "GET", {}, "", userBeatmapScores);
}

/**
 * Get osu! beatmap
 */
bool OsuWrapper::getBeatmap(BeatmapID const& beatmapID, nlohmann::json& beatmap /* out */)
{
    LOG_DEBUG("Requesting beatmap ", beatmapID);
    std::string url = "https://osu.ppy.sh/api/v2/beatmaps/" + std::to_string(beatmapID);
    return apiRequest_(url, "GET", {}, "", beatmap);
}

bool OsuWrapper::getBeatmaps(std::vector<BeatmapID> const& beatmapIDs, Gamemode const& mode, nlohmann::json& beatmaps /* out */)
{
    LOG_DEBUG("Requesting data for ", beatmapIDs.size(), " ", mode.toString(), " beatmaps");
    LOG_ERROR_THROW(
        beatmapIDs.size() <= k_batchMaxIDs,
        "Cannot request more than ", k_batchMaxIDs, " beatmaps at once! beatmapIDs.size()=", beatmapIDs.size()
    );
    std::string url = "https://osu.ppy.sh/api/v2/beatmaps";
    appendBatchParams(beatmapIDs, url);
    return apiRequest_(url, "GET", {}, "", beatmaps);
}

/**
 * WARNING: It is possible for this function to retry forever!
 *
 * Send request to osu!API v2.
 * If request gets ratelimited or a server error occurs, waits according to [exponential backoff](https://cloud.google.com/iot/docs/how-tos/exponential-backoff) then retries.
 * If the request itself fails (e.g. no internet connection), waits for a while and retries.
 * Status code logic is implemented according to [osu-web](https://github.com/ppy/osu-web/blob/master/resources/lang/en/layout.php).
 * Return true if request succeeds, false if not.
 */
bool OsuWrapper::apiRequest_(std::string const& url, std::string const& method, std::vector<std::string> headers, std::string const& body, nlohmann::json& responseDataJson /* out */)
{
    std::size_t retries = 0;
    int delayMs = m_apiCooldownMs;
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));

        headers.clear();
        headers.push_back("Content-Type: application/json");
        headers.push_back("Accept: application/json");
        headers.push_back("Authorization: Bearer " + m_pTokenManager->getAccessToken());

        long httpCode = 0;
        std::string responseData = "";
        if (!m_pHttpRequester->makeRequest(url, method, headers, body, httpCode, responseData))
        {
            int waitMs = k_curlRetryWaitMs - delayMs;
            if (waitMs < 0)
            {
                waitMs = 0;
            }

            LOG_WARN("Request failed, retrying in ", waitMs + delayMs, "ms");
            std::this_thread::sleep_for(std::chrono::milliseconds(waitMs));
            continue;
        }

        // 200 OK -> parse and return
        if (httpCode == 200)
        {
            responseDataJson = nlohmann::json::parse(responseData);
            LOG_ERROR_THROW(
                responseDataJson.is_object(),
                "responseDataJson is not an object! responseDataJson=", responseDataJson.dump());
            return true;
        }
        // 401 Unauthorized -> refresh token
        else if (httpCode == 401)
        {
            LOG_DEBUG("Got 401, attempting to refresh OAuth token");
            m_pTokenManager->updateAccessToken();
            continue;
        }
        // 404 Not Found
        else if (httpCode == 404)
        {
            LOG_ERROR("Got 404 response from ", method, " ", url);
            return false;
        }
        // 429 Too Many Requests / 5XX Internal Server Error -> increase wait time, then retry
        else if ((httpCode == 429) || (std::to_string(httpCode)[0] == '5'))
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

            LOG_WARN("Request failed (", httpCode, "); retrying in ", delayMs, "ms");
            ++retries;
            continue;
        }

        LOG_ERROR("Made ", method, " request to ", url, " and got unhandled response ", httpCode);
        return false;
    }
}