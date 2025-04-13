#ifndef __OSU_WRAPPER_H__
#define __OSU_WRAPPER_H__

#include "Util.h"
#include "TokenManager.h"
#include "HttpRequester.h"

#include <nlohmann/json.hpp>

#include <string>
#include <cstddef>
#include <memory>
#include <vector>

/**
 * osu!API v2 wrapper. Only implements endpoints necessary for this project.
 */
class OsuWrapper
{
public:
    OsuWrapper(std::shared_ptr<TokenManager> tokenManager, int const& apiCooldownMs) noexcept;
    ~OsuWrapper() = default;
    OsuWrapper(OsuWrapper const&) = delete;
    OsuWrapper& operator=(OsuWrapper const&) = delete;
    OsuWrapper(OsuWrapper&&) = default;
    OsuWrapper& operator=(OsuWrapper&&) = default;

    bool getRankings(Page page, Gamemode const& mode, nlohmann::json& rankings /* out */);
    bool getUser(UserID const& userID, Gamemode const& mode, nlohmann::json& user /* out */);
    bool getUserBeatmapScores(Gamemode const& mode, UserID const& userID, BeatmapID const& beatmapID, nlohmann::json& userBeatmapScores /* out */);
    bool getBeatmap(BeatmapID const& beatmapID, nlohmann::json& beatmap /* out */);

private:
    [[nodiscard]] bool apiRequest_(std::string const& url, std::string const& method, std::vector<std::string> headers, std::string const& body, nlohmann::json& responseDataJson /* out */);

    std::unique_ptr<HttpRequester> m_pHttpRequester = std::make_unique<HttpRequester>();
    std::shared_ptr<TokenManager> m_pTokenManager;
    int m_apiCooldownMs;
};

#endif /* __OSU_WRAPPER_H__ */
