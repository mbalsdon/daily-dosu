#include "Bot.h"
#include "DosuConfig.h"
#include "DailyJob.h"
#include "ScrapeRankings.h"
#include "Util.h"
#include "Logger.h"
#include "OsuWrapper.h"
#include "RankingsDatabaseManager.h"
#include "BotConfigDatabaseManager.h"

#include <functional>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <condition_variable>

namespace
{
    std::atomic<bool> g_bShutdown(false);
    std::condition_variable g_shutdownCV;
    std::mutex g_shutdownMtx;

    /**
     * Handle basic signals.
     */
    void signalHandler(int signum)
    {
        if (signum == SIGINT || signum == SIGTERM)
        {
            g_bShutdown = true;
            g_shutdownCV.notify_all();
        }
    }

    /**
     * Set up program behaviour for signals.
     */
    void setupSignalHandling()
    {
        struct sigaction sa;
        sa.sa_handler = signalHandler; // Route signals
        sigemptyset(&sa.sa_mask); // Don't block any signals
        sa.sa_flags = 0; // Default behaviour when signals are received

        sigaction(SIGINT, &sa, nullptr); // Register SIGINT (e.g. CTRL+C)
        sigaction(SIGTERM, &sa, nullptr); // Register SIGTERM

        signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE
    }
}

/**
 * Entrypoint.
 */
int main()
{
    try
    {
        // Setup signal handling
        setupSignalHandling();

        // Create necessary files/directories
        if (!std::filesystem::exists(k_dataDir))
        {
            std::filesystem::create_directory(k_dataDir);
        }

        // Load environment variables
        if (!std::filesystem::exists(k_dosuConfigFilePath))
        {
            LOG_WARN("Cannot find config file! Running setup tool...");
            DosuConfig::setupConfig(k_dosuConfigFilePath);
            return 0;
        }
        DosuConfig::load(k_dosuConfigFilePath);

        // Initialize services
        Logger::getInstance().setLogLevel(DosuConfig::logLevel);

        RankingsDatabaseManager& rankingsDbm = RankingsDatabaseManager::getInstance();
        rankingsDbm.initialize(DosuConfig::rankingsDatabaseFilePath);

        BotConfigDatabaseManager& botConfigDbm = BotConfigDatabaseManager::getInstance();
        botConfigDbm.initialize(DosuConfig::botConfigDatabaseFilePath);

        // Start bot
        Bot bot(DosuConfig::discordBotToken, rankingsDbm, botConfigDbm);
        bot.start();

        // Start jobs
        DailyJob scrapeRankingsJob(
            DosuConfig::scrapeRankingsRunHour,
            "scrapeRankings",
            [&rankingsDbm] { scrapeRankings(rankingsDbm); },
            [&bot]() { bot.scrapeRankingsCallback(); }
        );
        scrapeRankingsJob.start();

        // Wait for shutdown signal
        {
            std::unique_lock<std::mutex> lock(g_shutdownMtx);
            g_shutdownCV.wait(lock, [] { return g_bShutdown.load(); });
        }

        // Clean everything up
        LOG_INFO("Cleaning up resources and connections - PLEASE DON'T SHUT DOWN...");
        scrapeRankingsJob.stop();
        bot.stop();
        rankingsDbm.cleanup();
        botConfigDbm.cleanup();

        LOG_INFO("Cleanup complete! Exiting...");
        Logger::getInstance().setLogLevel(Logger::Level::ERROR); // Quiet destructor logs; we already cleaned
        return 0;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Fatal error: ", e.what());
        return 1;
    }
    catch (...)
    {
        LOG_ERROR("Unknown fatal error occurrred!");
        return 1;
    }
}
