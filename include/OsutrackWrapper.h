#ifndef __OSUTRACK_WRAPPER_H__
#define __OSUTRACK_WRAPPER_H__

#include "Util.h"

#include <dpp/nlohmann/json.hpp>
#include <curl/curl.h>

#include <string>
#include <cstddef>

/**
 * osu!track wrapper. Only implements endpoints necessary for this project.
 */
class OsutrackWrapper
{
public:
    OsutrackWrapper(int const& apiCooldownMs);
    ~OsutrackWrapper();

    bool getBestPlays(Gamemode const& mode, std::string const& fromDate, std::string const& toDate, std::size_t const& maxNumPlays, nlohmann::json& bestPlays /* out */);

private:
    bool makeRequest(std::string const& requestURL, CURLoption const& method);
    static std::size_t curlWriteCallback(void *contents, std::size_t size, std::size_t nmemb, std::string *response);

    CURL* m_curlHandle;
    std::string m_responseData;

    int m_apiCooldownMs;
};

#endif /* __OSUTRACK_WRAPPER_H__ */
