#include "Bot.h"
#include "DosuConfig.h"
#include "DailyJob.h"
#include "ScrapeRankings.h"
#include "GetTopPlays.h"
#include "Util.h"
#include "Logger.h"
#include "RankingsDatabase.h"
#include "TopPlaysDatabase.h"
#include "BotConfigDatabase.h"
#include "TokenManager.h"

#include <curl/curl.h>

#include <functional>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <memory>
#include <condition_variable>

namespace
{
    std::atomic<bool> g_bShutdown(false);
    std::condition_variable g_shutdownCV;
    std::mutex g_shutdownMtx;

    /**
     * Handle basic signals.
     */
    void signalHandler(int signum) noexcept
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
    void setupSignalHandling() noexcept
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
int main() noexcept
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

        // Initialize libcurl
        curl_global_init(CURL_GLOBAL_ALL);

        // Initialize logger
        Logger::getInstance().setLogLevel(DosuConfig::logLevel);

        // Initialize token manager
        std::shared_ptr<TokenManager> pTokenManager = std::make_shared<TokenManager>(DosuConfig::osuClientID, DosuConfig::osuClientSecret);

        // Connect to databases
        std::shared_ptr<RankingsDatabase> pRankingsDatabase = std::make_shared<RankingsDatabase>(DosuConfig::rankingsDatabaseFilePath);
        std::shared_ptr<TopPlaysDatabase> pTopPlaysDatabase = std::make_shared<TopPlaysDatabase>(DosuConfig::topPlaysDatabaseFilePath);
        std::shared_ptr<BotConfigDatabase> pBotConfigDatabase = std::make_shared<BotConfigDatabase>(DosuConfig::botConfigDatabaseFilePath);

        // Initialize and start bot
        std::shared_ptr<Bot> pBot = std::make_shared<Bot>(DosuConfig::discordBotToken, pRankingsDatabase, pTopPlaysDatabase, pBotConfigDatabase);
        pBot->start();

        // Initialize jobs
        std::unique_ptr<DailyJob> scrapeRankingsJob = std::make_unique<DailyJob>(
            DosuConfig::scrapeRankingsRunHour,
            "scrapeRankings",
            [&pTokenManager, &pRankingsDatabase] { scrapeRankings(pTokenManager, pRankingsDatabase); },
            [&pBot]() { pBot->scrapeRankingsCallback(); }
        );
        std::unique_ptr<DailyJob> topPlaysJob = std::make_unique<DailyJob>(
            DosuConfig::topPlaysRunHour,
            "getTopPlays",
            [&pTokenManager, &pTopPlaysDatabase] { getTopPlays(pTokenManager, pTopPlaysDatabase); },
            [&pBot]() { pBot->topPlaysCallback(); }
        );

        // Start jobs
        scrapeRankingsJob->start();
        topPlaysJob->start();

        // Wait for shutdown signal
        {
            std::unique_lock<std::mutex> lock(g_shutdownMtx);
            g_shutdownCV.wait(lock, [] { return g_bShutdown.load(); });
        }

        // Clean everything up
        LOG_INFO("Cleaning up resources and connections - PLEASE DON'T SHUT DOWN...");
        scrapeRankingsJob->stop();
        topPlaysJob->stop();
        pBot->stop();
        curl_global_cleanup();

        return 0;
    }
    catch (std::exception const& e)
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
