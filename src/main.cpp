#include "Bot.h"
#include "DosuConfig.h"
#include "DailyJob.h"
#include "ScrapePlayers.h"
#include "Util.h"
#include "Logger.h"

#include <dpp/nlohmann/json.hpp>

#include <functional>
#include <thread>
#include <chrono>

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
    if (!DosuConfig::configExists())
    {
        LOG_WARN("Cannot find config file! Running setup tool...");
        DosuConfig::setupConfig();
        return 0;
    }
    DosuConfig::load();

    // Set log level
    Logger::getInstance().setLogLevel(DosuConfig::logLevel);

    // Start bot and give it some time to start
    Bot bot(DosuConfig::discordBotToken);
    std::thread botThread(&Bot::start, &bot);
    std::this_thread::sleep_for(std::chrono::seconds(7));

    // Start jobs
    DailyJob scrapePlayersJob(DosuConfig::scrapePlayersRunHour, "scrapePlayers", scrapePlayers, [&bot]() { bot.scrapePlayersCallback(); });
    std::thread scrapePlayersThread(&DailyJob::start, &scrapePlayersJob);
    DailyJob backupServerConfigJob(DosuConfig::backupServerConfigRunHour, "backupServerConfig", [&bot]() { bot.backupServerConfig(); }, nullptr);
    std::thread backupServerConfigThread(&DailyJob::start, &backupServerConfigJob);

    // Wait on threads
    botThread.join();
    scrapePlayersThread.join();
    backupServerConfigThread.join();

    return 0;
}

// FUTURE: parallelize scrapeplayers if necessary

// FUTURE: new job: highest play today
// // https://github.com/Ameobea/osutrack-api
// // "Get the best plays by pp for all users in a given mode"

// FUTURE: move to sqlite3 you chud
