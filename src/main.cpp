#include "Bot.h"
#include "DosuConfig.h"
#include "DailyJob.h"
#include "ScrapePlayers.h"
#include "Util.h"
#include "Logger.h"

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
        LOG_ERROR("Cannot find ", k_dosuConfigFilePath, "! Exiting...");
        return 1;
    }

    DosuConfig::load(k_dosuConfigFilePath);

    // Set log level
    Logger::getInstance().setLogLevel(DosuConfig::logLevel);

    // Start bot and give it some time to start
    Bot bot(DosuConfig::discordBotToken);
    std::thread botThread(&Bot::start, &bot);
    std::this_thread::sleep_for(std::chrono::seconds(7));

    // Start jobs
    int scrapeHour = 0;
    DailyJob scrapePlayersJob(scrapeHour, "scrapePlayers", scrapePlayers, [&bot, scrapeHour]() { bot.scrapePlayersCallback(scrapeHour); });
    std::thread scrapePlayersThread(&DailyJob::start, &scrapePlayersJob);

    int backupHour = 23;
    DailyJob backupServerConfigJob(backupHour, "backupServerConfig", [&bot]() { bot.backupServerConfig(); }, nullptr);
    std::thread backupServerConfigThread(&DailyJob::start, &backupServerConfigJob);

    // Wait on threads
    botThread.join();
    scrapePlayersThread.join();
    backupServerConfigThread.join();

    return 0;
}

// TODO: DosuConfig setup
// // just construct in main
// // constructor checks file is valid; if not, prompt for cmd setup tool (run if Y exit if N)
// // constructor loads file
// // also move runHour vals (main) to dosuconfig

// TODO: touchups
// // 1 prevent bot spam (X commands per user per time, Y commands per channel per time)
// // // how to impl? burst is fine but wanna prevent constant spam
// // 2 other general security features to have?
// // 3 manual run cmd (allowed user id in dosu_config)
// // 4 help command
// // 5 pls shilling (kofi?)
// // 6 go through TODO/FUTURE/FIXME, any other necessary cleanups

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
