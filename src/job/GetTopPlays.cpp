#include "GetTopPlays.h"
#include "Logger.h"
#include "Util.h"
#include "OsutrackWrapper.h"
#include "OsuWrapper.h"

#include <string>
#include <vector>
#include <cstdint>
#include <future>
#include <unordered_map>
#include <utility>

#include <nlohmann/json.hpp>

namespace
{
/**
 * Vector to string.
 */
template<typename T>
std::string printVector(std::vector<T> v)
{
    std::string ret = "[";
    for (auto it = v.begin(); it != v.end(); ++it)
    {
        ret += std::to_string(*it);
        if (std::next(it) != v.end()) ret += ", ";
    }
    ret += "]";
    return ret;
}

/**
 * Cross-reference osutrack best play with osu!API.
 * Return true if it was found, as well as the play with osutrack data + osu!API score data filled in.
 */
std::pair<bool, TopPlay> findTopPlay(
    std::shared_ptr<TokenManager> pTokenManager,
    int64_t const& i,
    nlohmann::json const& bestPlayObj,
    Gamemode const& mode)
{
    OsuWrapper osu(pTokenManager, 0);

    // Fill in data that osutrack gave us
    TopPlay tp = {
        .rank = i,
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

    // Attempt to find osu!API data for the score by retrieving all of the user's scores on the beatmap and matching the date
    nlohmann::json userBeatmapScoresObj;
    LOG_ERROR_THROW(
        osu.getUserBeatmapScores(mode, tp.score.user.userID, tp.score.beatmap.beatmapID, userBeatmapScoresObj),
        "Failed to get user beatmap scores! mode=", mode.toString(), ", userID=", tp.score.user.userID, ", beatmapID=", tp.score.beatmap.beatmapID
    );

    bool bFoundScore = false;
    for (auto const& userBeatmapScore : userBeatmapScoresObj["scores"])
    {
        // If score time matches this is the play
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
        }
    }

    return std::make_pair(bFoundScore, tp);
}

/**
 * Fill in remaining fields for each play in given chunk.
 */
std::vector<TopPlay> fillInTopPlaysChunk(
    std::shared_ptr<TokenManager> pTokenManager,
    std::vector<TopPlay> topPlaysChunk,
    Gamemode const& mode)
{
    OsuWrapper osu(pTokenManager, 0);

    // Collect userIDs and beatmapIDs so that we can batch request
    std::vector<UserID> userIDs;
    userIDs.reserve(topPlaysChunk.size());
    std::vector<BeatmapID> beatmapIDs;
    beatmapIDs.reserve(topPlaysChunk.size());
    for (auto const& topPlay : topPlaysChunk)
    {
        userIDs.push_back(topPlay.score.user.userID);
        beatmapIDs.push_back(topPlay.score.beatmap.beatmapID);
    }

    // Do batch requests
    nlohmann::json usersArr;
    nlohmann::json beatmapsArr;
    LOG_ERROR_THROW(
        osu.getUsers(userIDs, mode, usersArr),
        "Failed to get users! userIDs=", printVector(userIDs), ", mode=", mode.toString()
    );
    LOG_ERROR_THROW(
        osu.getBeatmaps(beatmapIDs, mode, beatmapsArr),
        "Failed to get beatmaps! beatmapIDs=", printVector(beatmapIDs), ", mode=", mode.toString()
    );

    // Fill in missing data
    // We can't rely on ordering since osu!API batch requests return sets (no duplicates)
    std::unordered_map<UserID, nlohmann::json> userMap;
    std::unordered_map<BeatmapID, nlohmann::json> beatmapMap;
    for (auto const& userObj : usersArr.at("users"))
    {
        userMap[userObj.at("id")] = userObj;
    }
    for (auto const& beatmapObj : beatmapsArr.at("beatmaps"))
    {
        beatmapMap[beatmapObj.at("id")] = beatmapObj;
    }

    std::vector<TopPlay> completedTopPlays;
    completedTopPlays.reserve(topPlaysChunk.size());
    for (auto const& topPlay : topPlaysChunk)
    {
        auto userIt = userMap.find(topPlay.score.user.userID);
        LOG_ERROR_THROW(
            userIt != userMap.end(),
            "Failed to find userID ", topPlay.score.user.userID, " in userMap!"
        );
        auto beatmapIt = beatmapMap.find(topPlay.score.beatmap.beatmapID);
        LOG_ERROR_THROW(
            beatmapIt != beatmapMap.end(),
            "Failed to find beatmapID ", topPlay.score.beatmap.beatmapID, " in beatmapMap!"
        );

        nlohmann::json userObj = userIt->second;
        nlohmann::json beatmapObj = beatmapIt->second;
        TopPlay completedTopPlay = topPlay;

        completedTopPlay.score.user.username = userObj.at("username").get<Username>();
        completedTopPlay.score.user.countryCode = userObj.at("country_code").get<CountryCode>();
        completedTopPlay.score.user.pfpLink = userObj.at("avatar_url").get<ProfilePicture>();
        completedTopPlay.score.user.performancePoints = userObj.at("statistics_rulesets").at(mode.toString()).at("pp").get<PerformancePoints>();
        completedTopPlay.score.user.accuracy = userObj.at("statistics_rulesets").at(mode.toString()).at("hit_accuracy").get<Accuracy>();
        completedTopPlay.score.user.hoursPlayed = static_cast<HoursPlayed>(userObj.at("statistics_rulesets").at(mode.toString()).at("play_time").get<uint64_t>() / 3600); // FIXME: round instead of trunc
        completedTopPlay.score.user.currentRank = userObj.at("statistics_rulesets").at(mode.toString()).at("global_rank").get<Rank>();

        completedTopPlay.score.beatmap.maxCombo = beatmapObj.at("max_combo").get<Combo>();
        completedTopPlay.score.beatmap.difficultyName = beatmapObj.at("version").get<DifficultyName>();
        completedTopPlay.score.beatmap.artist = beatmapObj.at("beatmapset").at("artist").get<BeatmapArtist>();
        completedTopPlay.score.beatmap.title = beatmapObj.at("beatmapset").at("title").get<BeatmapTitle>();
        completedTopPlay.score.beatmap.mapsetCreator = beatmapObj.at("beatmapset").at("creator").get<Username>();
        completedTopPlay.score.beatmap.starRating = beatmapObj.at("difficulty_rating").get<StarRating>();

        completedTopPlays.push_back(completedTopPlay);
    }

    return completedTopPlays;
}

/**
 * Get today's top plays for given mode.
 */
void getTopPlaysMode(
    OsutrackWrapper& osutrack,
    std::shared_ptr<TokenManager> pTokenManager,
    std::shared_ptr<TopPlaysDatabase> pTopPlaysDb,
    ISO8601DateTimeUTC const& now,
    Gamemode const& mode)
{
    // Grab the top plays
    nlohmann::json bestPlaysArr;
    ISO8601DateTimeUTC yesterday = now;
    yesterday.addDays(-1);
    LOG_ERROR_THROW(
        osutrack.getBestPlays(mode, yesterday.toDateString(), now.toDateString(), k_numTopPlays, bestPlaysArr),
        "Failed to get best plays! mode=", mode.toString(), ", from=", yesterday.toString(), ", to=", now.toString()
    );
    LOG_ERROR_THROW(
        bestPlaysArr.size() <= k_numTopPlays,
        "Expected at most ", k_numTopPlays, " plays from osu!track but got ", bestPlaysArr.size()
    );

    // Try to find each top play in the osu!API
    int64_t i = 1;
    std::vector<TopPlay> topPlays;
    topPlays.reserve(k_numTopPlays);

    std::vector<std::future<std::pair<bool, TopPlay>>> topPlayFutures;
    topPlayFutures.reserve(bestPlaysArr.size());
    for (auto const& bestPlayObj : bestPlaysArr)
    {
        auto futureTopPlay = std::async(std::launch::async, findTopPlay, pTokenManager, i++, bestPlayObj, mode);
        topPlayFutures.push_back(std::move(futureTopPlay));
    }

    for (auto& futureTopPlay : topPlayFutures)
    {
        auto [bFoundScore, tp] = futureTopPlay.get();
        if (!bFoundScore)
        {
            LOG_WARN("Failed to find ", mode.toString(), " score set by user ", tp.score.user.userID, " on beatmap ", tp.score.beatmap.beatmapID, " - score was skipped");
            continue;
        }
        topPlays.push_back(tp);
    }

    // Fill in any missing information using osu!API
    std::vector<TopPlay> completeTopPlays;
    completeTopPlays.reserve(topPlays.size());

    std::vector<std::future<std::vector<TopPlay>>> completeTopPlaysChunkFutures;
    std::size_t numChunks = (topPlays.size() + k_batchMaxIDs - 1) / k_batchMaxIDs;
    completeTopPlaysChunkFutures.reserve(numChunks);
    for (std::size_t j = 0; j < topPlays.size(); j += k_batchMaxIDs)
    {
        std::size_t endIdx = std::min(j + k_batchMaxIDs, topPlays.size());
        auto beginIt = topPlays.begin();
        std::advance(beginIt, j);
        auto endIt = topPlays.begin();
        std::advance(endIt, endIdx);
        std::vector<TopPlay> topPlaysChunk(beginIt, endIt);
        auto futureCompleteTopPlaysChunk = std::async(std::launch::async, fillInTopPlaysChunk, pTokenManager, topPlaysChunk, mode);
        completeTopPlaysChunkFutures.push_back(std::move(futureCompleteTopPlaysChunk));
    }

    for (auto& futureCompleteTopPlaysChunk : completeTopPlaysChunkFutures)
    {
        auto completeTopPlaysChunk = futureCompleteTopPlaysChunk.get();
        completeTopPlays.insert(completeTopPlays.end(), std::make_move_iterator(completeTopPlaysChunk.begin()), std::make_move_iterator(completeTopPlaysChunk.end()));
    }

    pTopPlaysDb->insertTopPlays(mode, completeTopPlays);
}
} /* namespace */

/**
 * Get daily top plays for each mode.
 * Makes 4 osutrack API calls and up to [4 * k_numTopPlays + 8 * ceil(k_numTopPlays / 50)] osu!API calls.
 */
void getTopPlays(std::shared_ptr<TokenManager> pTokenManager, std::shared_ptr<TopPlaysDatabase> pTopPlaysDb)
{
    LOG_INFO("Grabbing top plays of the day");

    OsutrackWrapper osutrack(0);
    ISO8601DateTimeUTC now;

    pTopPlaysDb->wipeTables();

    // Do work for each mode
    getTopPlaysMode(osutrack, pTokenManager, pTopPlaysDb, now, Gamemode::Osu);
    getTopPlaysMode(osutrack, pTokenManager, pTopPlaysDb, now, Gamemode::Taiko);
    getTopPlaysMode(osutrack, pTokenManager, pTopPlaysDb, now, Gamemode::Mania);
    getTopPlaysMode(osutrack, pTokenManager, pTopPlaysDb, now, Gamemode::Catch);
}
