#include "OsutrackWrapper.h"
#include "Logger.h"

#include <thread>
#include <string>

/**
 * OsutrackWrapper constructor.
 */
OsutrackWrapper::OsutrackWrapper(int const& apiCooldownMs)
{
    LOG_DEBUG("Constructing OsutrackWrapper...");

    m_apiCooldownMs = apiCooldownMs;

    m_curlHandle = curl_easy_init();
    if (!m_curlHandle)
    {
        std::string reason = "Failed to initialize CURL handle in OsutrackWrapper!";
        throw std::runtime_error(reason);
    }

    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &m_responseData);
    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, OsutrackWrapper::curlWriteCallback);
}

/**
 * OsutrackWrapper destructor.
 */
OsutrackWrapper::~OsutrackWrapper()
{
    LOG_DEBUG("Destructing OsutrackWrapper...");

    if (m_curlHandle)
    {
        curl_easy_cleanup(m_curlHandle);
    }
}

/**
 * Get top plays from osu!track.
 *
 * fromDate and toDate must be of form YYYY-MM-DD.
 */
bool OsutrackWrapper::getBestPlays(Gamemode const& mode, std::string const& fromDate, std::string const& toDate, std::size_t const& maxNumPlays, nlohmann::json& bestPlays /* out */)
{
    LOG_DEBUG("Requesting data from osu!track API for best ", maxNumPlays, " ", mode.toString(), " plays from ", fromDate, " to ", toDate, "...");

    if (maxNumPlays < 1)
    {
        std::string reason = std::string("OsutrackWrapper::getBestPlays - maxNumPlays must be greater than zero! maxNumPlays=") + std::to_string(maxNumPlays);
        throw std::invalid_argument(reason);
    }

    std::string requestURL =
        "https://osutrack-api.ameo.dev/bestplays?mode=" + std::to_string(mode.toOsutrackInt()) +
        "&from=" + fromDate +
        "&to=" + toDate +
        "&limit=" + std::to_string(maxNumPlays);
    bool bSuccess = makeRequest(requestURL, CURLOPT_HTTPGET);
    if (!bSuccess)
    {
        return false;
    }

    nlohmann::json responseDataJson = nlohmann::json::parse(m_responseData);
    m_responseData.clear();
    if (!responseDataJson.is_array())
    {
        std::string reason = std::string("OsutrackWrapper::getBestPlays - responseDataJson is not an array! responseDataJson=").append(responseDataJson.dump());
        throw std::runtime_error(reason);
    }

    bestPlays = responseDataJson;
    return true;
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
bool OsutrackWrapper::makeRequest(std::string const& requestURL, CURLoption const& method)
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
        requestHeaders = curl_slist_append(requestHeaders, "Accept: application/json");
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
        else if (std::to_string(httpCode)[0] == '4') // 4XX Client Error
        {
            LOG_ERROR("Got ", httpCode, " response from ", requestURL);
            return false;
        }
        else if (std::to_string(httpCode)[0] == '5') // 5XX Internal Server Error - increase wait time
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
std::size_t OsutrackWrapper::curlWriteCallback(void* contents, std::size_t size, std::size_t nmemb, std::string* response)
{
    std::size_t totalSize = size * nmemb;
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}
