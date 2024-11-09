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

// TODO: clean up storage file consts ("data", "x.json", etc)
// // // commonly used paths (SEE SERVERCONFIG PATH BUILDING) can be saved as obj state
// // clean up consts in general (including .h just for consts is cringe.. all in one file idk)
// // rank range enum to key, in bot?
// // ^^^^^^^^ new commits

// TODO: server config backup (dailyjob) [concurrency issues...?]

// TODO: settings (country, only rankup/rankdown, ...)
// // setting options returns sample display
// // scrapePlayers prebuilds files (CA.json, EE.json, etc)
// // server config needs more complex structure; channel id -> cfg
// // ^^^^^^^^ new commit

// TODO: help command

// TODO: improve logging

// TODO: dosu config setup if doesnt exist (see FUTURE)

// TODO: pls shilling (kofi?)

// TODO: release prep
// // go thru TODOs (readme, comments, etc)
// // any cleanups?
// // ...
// // ^^^^^^^^ new commits
// // run for a few days; get bot into 2 servers (2 diff discord accts)

// TODO: release

// TODO: email peppy ratelimit increase

// TODO: new job: highest play today
// // https://github.com/Ameobea/osutrack-api
// // "Get the best plays by pp for all users in a given mode"

// TODO: move to sqlite3 you chud
