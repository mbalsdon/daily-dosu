#include "Bot.h"
#include "DosuConfig.h"
#include "DailyJob.h"
#include "ScrapePlayers.h"
#include "Util.h"

#include <dpp/nlohmann/json.hpp>

#include <iostream>
#include <functional>
#include <thread>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

/**
 * Entrypoint.
 */
int main(int argc, char const *argv[])
{
    // Create necessary files/directories
    if (!std::filesystem::exists(k_dataDir))
    {
        std::filesystem::create_directory(k_dataDir);
    }

    // Load environment variables
    if (!std::filesystem::exists(k_dosuConfigFilePath))
    {
        // TODO: Setup tool to create a config
        std::cout << "Cannot find " << k_dosuConfigFilePath << "! Exiting..." << std::endl;
        return 1;
    }

    DosuConfig::load(k_dosuConfigFilePath);

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

// TODO: server config backup
// // add backup cmd to ServerConfig (pls having thread safety)
// // // overwrite backup if write time was more than 7d ago
// // register as dailyjob

// TODO: settings
// // 1 list (20) or 2 lists (10+10)
// // choose list(s); global/country, top or bottom
// // setting options returns sample display
// // scrapePlayers prebuilds files (CA.json, EE.json, etc)
// // server config needs more complex structure; channel id -> cfg
// // need to move off of precomputed embeds
// // move stuff from bot to embed builder class? does it need state?
// // ^^^^^^^^ new commit

// TODO: improve logging
// // logger class; timestamps + log level (dosuconfig)
// // no need for disk saves
// // better way to throw runtime errors?

// TODO: DosuConfig setup
// // just construct in main
// // constructor checks file is valid; if not, prompt for cmd setup tool (run if Y exit if N)
// // constructor loads file

// TODO: touchups
// // 1 prevent bot spam (X commands per user per time, Y commands per channel per time)
// // // how to impl? burst is fine but wanna prevent constant spam
// // 2 other general security features to have?
// // 3 manual run cmd (allowed user id in dosu_config)
// // 4 only write full users to disk if dosu_config flag is set (only rly need for debug)
// // 5 help command
// // 6 pls shilling (kofi?)
// // 7 go through TODO/FUTURE/FIXME, any other necessary cleanups

// TODO: release prep
// // test e2e alt acct
// // readme (what, why, contributing, self-setup)
// // run for a few days; in a few servers (test+friend)
// // // probably want an eth line instead of wlan
// // email peppy ratelimit increase
// // if all good, le reddit

// FUTURE: new job: highest play today
// // https://github.com/Ameobea/osutrack-api
// // "Get the best plays by pp for all users in a given mode"

// FUTURE: move to sqlite3 you chud
