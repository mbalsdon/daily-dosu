#ifndef __OSUTRACK_WRAPPER_H__
#define __OSUTRACK_WRAPPER_H__

#include "Util.h"
#include "HttpRequester.h"

#include <nlohmann/json.hpp>

#include <string>
#include <memory>
#include <vector>
#include <cstddef>

/**
 * osu!track wrapper. Only implements endpoints necessary for this project.
 */
class OsutrackWrapper
{
public:
    OsutrackWrapper(int const& apiCooldownMs) noexcept;
    ~OsutrackWrapper() = default;
    OsutrackWrapper(OsutrackWrapper const&) = delete;
    OsutrackWrapper& operator=(OsutrackWrapper const&) = delete;
    OsutrackWrapper(OsutrackWrapper&&) = default;
    OsutrackWrapper& operator=(OsutrackWrapper&&) = default;

    bool getBestPlays(Gamemode const& mode, std::string const& fromDate, std::string const& toDate, std::size_t const& maxNumPlays, nlohmann::json& bestPlays /* out */);

private:
    [[nodiscard]] bool apiRequest_(std::string const& url, std::string const& method, std::vector<std::string> headers, std::string const& body, nlohmann::json& responseDataJson /* out */);

    std::unique_ptr<HttpRequester> m_pHttpRequester = std::make_unique<HttpRequester>();
    int m_apiCooldownMs;
};

#endif /* __OSUTRACK_WRAPPER_H__ */
