#include "GetTopPlays.h"
#include "Logger.h"
#include "Util.h"
#include "OsutrackWrapper.h"
#include "OsuWrapper.h"

#include <nlohmann/json.hpp>

namespace
{
/**
 * Get today's top plays for given mode.
 */
void getTopPlaysMode(OsutrackWrapper& osutrack, OsuWrapper& osu, std::shared_ptr<TopPlaysDatabase> pTopPlaysDb, ISO8601DateTimeUTC const& now, Gamemode const& mode)
{
    // Grab the top plays
    LOG_INFO("Retrieving top ", k_numTopPlays, " ", mode.toString(), " plays from today...");
    nlohmann::json bestPlaysArr;
    ISO8601DateTimeUTC yesterday = now;
    yesterday.addDays(-1);
    LOG_ERROR_THROW(
        osutrack.getBestPlays(mode, yesterday.toDateString(), now.toDateString(), k_numTopPlays, bestPlaysArr),
        "Failed to get best plays! mode=", mode.toString(), ", from=", yesterday.toString(), ", to=", now.toString()
    );
    LOG_ERROR_THROW(
        bestPlaysArr.size() == k_numTopPlays,
        "Expected ", k_numTopPlays, " from osu!track but got ", bestPlaysArr.size()
    );

    // Process each play
    int64_t i = 1;
    std::vector<TopPlay> topPlays;
    topPlays.reserve(k_numTopPlays);
    for (auto const& bestPlayObj : bestPlaysArr)
    {
        // Fill in data that osutrack gave us
        TopPlay topPlay = {
            .rank = i++,
            .score = {
                .performancePoints = bestPlayObj.at("pp").get<PerformancePoints>(),
                .totalScore = bestPlayObj.at("score").get<TotalScore>(),
                .createdAt = ISO8601DateTimeUTC(bestPlayObj.at("score_time").get<std::string>()),
                .letterRank = bestPlayObj.at("rank").get<LetterRank>(),
                .beatmap = {
                    .beatmapID = bestPlayObj.at("beatmap_id").get<BeatmapID>()
                },
                .user = {
                    .userID = bestPlayObj.at("user").get<UserID>(),
                }
            }
        };

        topPlays.push_back(topPlay);
        TopPlay& tp = topPlays.back();

        // Attempt to find osu!API data for the score by retrieving all of the user's scores on the beatmap and matching the date
        nlohmann::json userBeatmapScoresObj;
        LOG_ERROR_THROW(
            osu.getUserBeatmapScores(mode, tp.score.user.userID, tp.score.beatmap.beatmapID, userBeatmapScoresObj),
            "Failed to get user beatmap scores! mode=", mode.toString(), ", userID=", tp.score.user.userID, ", beatmapID=", tp.score.beatmap.beatmapID
        );

        bool bFoundScore = false;
        for (auto const& userBeatmapScore : userBeatmapScoresObj["scores"])
        {
            // If score time matches this is the play unless user is capable of temporal manipulation
            if (tp.score.createdAt == ISO8601DateTimeUTC(userBeatmapScore.at("created_at").get<std::string>()))
            {
                bFoundScore = true;

                // Fill in the score data that we got
                tp.score.scoreID = userBeatmapScore.at("id").get<ScoreID>();
                tp.score.accuracy = userBeatmapScore.at("accuracy").get<Accuracy>();
                tp.score.mods = OsuMods(userBeatmapScore.at("mods").get<std::vector<std::string>>());
                tp.score.combo = userBeatmapScore.at("max_combo").get<Combo>();
                tp.score.count300 = userBeatmapScore.at("statistics").at("count_300").get<HitCount>();
                tp.score.count100 = userBeatmapScore.at("statistics").at("count_100").get<HitCount>();
                if (mode != Gamemode::Taiko)
                {
                    tp.score.count50 = userBeatmapScore.at("statistics").at("count_50").get<HitCount>();
                }
                tp.score.countMiss = userBeatmapScore.at("statistics").at("count_miss").get<HitCount>();

                // Get user data from osu!API and fill it in
                nlohmann::json userObj;
                LOG_ERROR_THROW(
                    osu.getUser(tp.score.user.userID, mode, userObj),
                    "Failed to get user! mode=", mode.toString(), ", userID=", tp.score.user.userID
                );

                tp.score.user.username = userObj.at("username").get<Username>();
                tp.score.user.countryCode = userObj.at("country").at("code").get<CountryCode>();
                tp.score.user.pfpLink = userObj.at("avatar_url").get<ProfilePicture>();
                tp.score.user.performancePoints = userObj.at("statistics").at("pp").get<PerformancePoints>();
                tp.score.user.accuracy = userObj.at("statistics").at("hit_accuracy").get<Accuracy>();
                tp.score.user.hoursPlayed = static_cast<HoursPlayed>(userObj.at("statistics").at("play_time").get<uint64_t>() / 3600); // FIXME: round instead of trunc
                tp.score.user.currentRank = userObj.at("statistics").at("global_rank").get<Rank>();

                // Get beatmap data from osu!API and fill it in
                nlohmann::json beatmapObj;
                LOG_ERROR_THROW(
                    osu.getBeatmap(tp.score.beatmap.beatmapID, beatmapObj),
                    "Failed to get ", mode.toString(), " score ", tp.score.scoreID
                );

                tp.score.beatmap.maxCombo = beatmapObj.at("max_combo").get<Combo>();
                tp.score.beatmap.difficultyName = beatmapObj.at("version").get<DifficultyName>();
                tp.score.beatmap.artist = beatmapObj.at("beatmapset").at("artist").get<BeatmapArtist>();
                tp.score.beatmap.title = beatmapObj.at("beatmapset").at("title").get<BeatmapTitle>();
                tp.score.beatmap.mapsetCreator = beatmapObj.at("beatmapset").at("creator").get<Username>();

                // Get star rating from osu!API and fill it in
                nlohmann::json attributesObj;
                LOG_ERROR_THROW(
                    osu.getBeatmapAttributes(tp.score.beatmap.beatmapID, mode, tp.score.mods, attributesObj),
                    "Failed to get attributes for ", mode.toString(), " beatmap ", tp.score.beatmap.beatmapID, " with mods ", tp.score.mods.toString()
                );

                tp.score.beatmap.starRating = attributesObj.at("attributes").at("star_rating").get<StarRating>();
            }
        }

        LOG_ERROR_THROW(
            bFoundScore,
            "Failed to find ", mode.toString(), " score set by user ", tp.score.user.userID, " on beatmap ", tp.score.beatmap.beatmapID
        );
    }

    LOG_INFO("Populating database...");
    pTopPlaysDb->insertTopPlays(mode, topPlays);
}
} /* namespace */

/**
 * Get daily top plays for each mode.
 * Makes 4 osutrack API calls and up to 4*k_numTopPlays osu!API calls.
 */
void getTopPlays(std::shared_ptr<TokenManager> pTokenManager, std::shared_ptr<TopPlaysDatabase> pTopPlaysDb)
{
    LOG_INFO("Grabbing top plays of the day...");

    OsutrackWrapper osutrack(0);
    OsuWrapper osu(pTokenManager, 0);
    ISO8601DateTimeUTC now;

    LOG_INFO("Wiping tables...");
    pTopPlaysDb->wipeTables();

    // Do work for each mode
    getTopPlaysMode(osutrack, osu, pTopPlaysDb, now, Gamemode::Osu);
    getTopPlaysMode(osutrack, osu, pTopPlaysDb, now, Gamemode::Taiko);
    getTopPlaysMode(osutrack, osu, pTopPlaysDb, now, Gamemode::Mania);
    getTopPlaysMode(osutrack, osu, pTopPlaysDb, now, Gamemode::Catch);
}
