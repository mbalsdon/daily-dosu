#include "OsuWrapper.h"
#include "Bot.h"
#include "DosuConfig.h"
#include "DailyJob.h"
#include "ScrapePlayers.h"

#include <iostream>
#include <string>
#include <functional>
#include <thread>
#include <chrono>
#include <filesystem>

/**
 * Entrypoint.
 */
int main(int argc, char const *argv[])
{
    // Load environment variables
    std::filesystem::path rootDir = std::filesystem::path(__FILE__).parent_path().parent_path();
    std::filesystem::path configPath = rootDir / k_configFileName;
    if (!std::filesystem::exists(configPath))
    {
        // FUTURE: Setup tool to create a config?
        std::cout << "Cannot find " << configPath << "! Exiting..." << std::endl;
        return 1;
    }

    DosuConfig::load(configPath);

    // Start bot
    Bot bot(DosuConfig::discordBotToken);
    std::thread botThread(&Bot::start, &bot);

    // Give bot some time to start
    std::this_thread::sleep_for(std::chrono::seconds(7));

    // Start jobs
    int hour = 0;
    DailyJob job(hour, "scrapePlayers", scrapePlayers, [&bot, hour]() { bot.scrapePlayersCallback(hour); });
    std::thread jobThread(&DailyJob::start, &job);

    botThread.join();
    jobThread.join();

    return 0;
}

// TODO: per-server behaviour; start/stop -> do/dont execute callback (save server id map???)
// // embed author pfp (3rd field) = server pfp
// // ^^^^^^^^ new commits

// TODO: command that sends raw users.json
// TODO: settings (country, only rankup/rankdown, ...)
// // setting options returns sample display
// // scrapePlayers prebuilds files (CA.json, EE.json, etc)
// // ^^^^^^^^ new commits

// TODO: help command
// TODO: bot button caching? i.e. calc and store all embeds for each rank range

// TODO: release
// // go thru TODOs (readme, comments, etc)
// // any cleanups?
// // rank range enum to key, in bot?
// // ...
// // ^^^^^^^^ new commits


// TODO: email peppy ratelimit increase
// TODO: userids should be saved incase of error; save data every X retrievals and start from there on failure?
// TODO: sussy detector (< X hrs and > Y increase)
// TODO: go through FUTUREs/FIXMEs

// TODO: new job: highest play today
// // https://github.com/Ameobea/osutrack-api
// // "Get the best plays by pp for all users in a given mode"
