#include "OsutrackWrapper.h"
#include "Logger.h"

#include <thread>
#include <string>

/**
 * OsutrackWrapper constructor.
 */
OsutrackWrapper::OsutrackWrapper(int const& apiCooldownMs) noexcept
: m_apiCooldownMs(apiCooldownMs)
{}

/**
 * Get top N osu! plays in [fromDate, toDate) for given mode.
 *
 * fromDate and toDate must be of form YYYY-MM-DD.
 */
bool OsutrackWrapper::getBestPlays(Gamemode const& mode, std::string const& fromDate, std::string const& toDate, std::size_t const& maxNumPlays, nlohmann::json& bestPlays /* out */)
{
    LOG_DEBUG("Requesting best ", maxNumPlays, " ", mode.toString(), " plays from ", fromDate, " to ", toDate);
    LOG_ERROR_THROW(
        maxNumPlays > 0,
        "maxNumPlays must be greater than zero! maxNumPlays=", maxNumPlays
    );
    std::string url =
        "https://osutrack-api.ameo.dev/bestplays?mode=" + std::to_string(mode.toOsutrackInt()) +
        "&from=" + fromDate +
        "&to=" + toDate +
        "&limit=" + std::to_string(maxNumPlays);
    bool bSuccess = apiRequest_(url, "GET", {}, "", bestPlays);
    LOG_ERROR_THROW(
        bestPlays.is_array(),
        "bestPlays is not an array! bestPlays=", bestPlays.dump());
    return bSuccess;
}

/**
 * WARNING: It is possible for this function to retry forever!
 *
 * Make CURL request.
 * If a server error occurs, waits according to exponential backoff.
 * If the request itself fails (e.g. no internet connection), waits for a while and retries.
 * Status code logic is implemented according to the [osutrack webserver implementation](https://github.com/Ameobea/osutrack-api/blob/main/src/webserver.ts).
 * Return true if request succeeds, false if not.
 */
[[nodiscard]] bool OsutrackWrapper::apiRequest_(std::string const& url, std::string const& method, std::vector<std::string> headers, std::string const& body, nlohmann::json& responseDataJson /* out */)
{
    std::size_t retries = 0;
    int delayMs = m_apiCooldownMs;
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));

        headers.clear();
        headers.push_back("Accept: application/json");

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
            return true;
        }
        // 4XX Client Error
        else if (std::to_string(httpCode)[0] == '4')
        {
            LOG_ERROR("Got ", httpCode, " response from ", url);
            return false;
        }
        // 5XX Internal Server Error -> increase wait time, then retry
        else if (std::to_string(httpCode)[0] == '5')
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
