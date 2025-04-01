#ifndef __OSU_WRAPPER_H__
#define __OSU_WRAPPER_H__

#include "Util.h"

#include <dpp/nlohmann/json.hpp>
#include <curl/curl.h>

#include <string>
#include <cstddef>

/**
 * osu!API v2 wrapper. Only implements endpoints necessary for this project.
 */
class OsuWrapper
{
public:
    OsuWrapper(const int apiCooldownMs);
    ~OsuWrapper();

    bool getRankings(Page page, Gamemode mode, nlohmann::json& rankings);
    bool getUser(UserID userID, Gamemode mode, nlohmann::json& user);
    bool getUserBeatmapScores(Gamemode const& mode, UserID const& userID, BeatmapID const& beatmapID, nlohmann::json& userBeatmapScores /* out */);
    bool getBeatmap(BeatmapID const& beatmapID, nlohmann::json& beatmap /* out */);
    bool getBeatmapAttributes(BeatmapID const& beatmapID, Gamemode const& mode, OsuMods const& mods, nlohmann::json& attributes /* out */);

private:
    bool makeRequest(const std::string requestURL, const CURLoption method);
    static std::size_t curlWriteCallback(void *contents, std::size_t size, std::size_t nmemb, std::string *response);

    CURL* m_curlHandle;
    std::string m_responseData;

    int m_apiCooldownMs;
};

#endif /* __OSU_WRAPPER_H__ */
