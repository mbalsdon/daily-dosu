#ifndef __BOT_H__
#define __BOT_H__

#include "RankingsDatabaseManager.h"
#include "BotConfigDatabaseManager.h"
#include "EmbedGenerator.h"
#include "Util.h"

#include <dpp/dpp.h>

#include <string>
#include <cstdint>
#include <unordered_map>
#include <chrono>
#include <atomic>

const std::chrono::seconds k_cmdRateLimitPeriod = std::chrono::seconds(5);
const std::chrono::seconds k_interactionRateLimitPeriod = std::chrono::seconds(3);

/**
 * DPP wrapper for the Discord bot. All event handling logic (and nothing else) should go here.
 * Where possible, the strategy should be to build embeds/components as little as possible, opting
 * instead to save them to memory such that we save compute / disk accesses.
 */
class Bot 
{
public:
    Bot(const std::string& botToken, RankingsDatabaseManager& rankingsDbm, BotConfigDatabaseManager& botConfigDbm);
    ~Bot();

    void start();
    void stop();
    void scrapeRankingsCallback();

private:
    Bot(const Bot&) = delete;
    Bot(Bot&&) = delete;
    Bot& operator=(const Bot&) = delete;
    Bot& operator=(Bot&&) = delete;

    void onLog(const dpp::log_t& event);
    void onReady(const dpp::ready_t& event);
    void onSlashCommand(const dpp::slashcommand_t& event);
    void onButtonClick(const dpp::button_click_t& event);

    void deleteGlobalCommand(std::string cmdName);

    void cmdHelp(const dpp::slashcommand_t& event);
    void cmdPing(const dpp::slashcommand_t& event);
    void cmdNewsletter(const dpp::slashcommand_t& event);
    void cmdSubscribe(const dpp::slashcommand_t& event);
    void cmdUnsubscribe(const dpp::slashcommand_t& event);

    void buildHelpEmbeds();
    bool buildScrapeRankingsEmbeds();

    dpp::cluster m_bot;
    RankingsDatabaseManager& m_rankingsDbm;
    BotConfigDatabaseManager& m_botConfigDbm;
    EmbedGenerator m_embedGenerator;
    std::atomic<bool> m_bIsInitialized;

    std::unordered_map<dpp::snowflake, std::chrono::steady_clock::time_point> m_latestCommands;
    std::unordered_map<dpp::snowflake, std::chrono::steady_clock::time_point> m_latestInteractions;

    dpp::embed m_helpEmbed;

    dpp::embed m_scrapeRankingsFirstRangeEmbed;
    dpp::embed m_scrapeRankingsSecondRangeEmbed;
    dpp::embed m_scrapeRankingsThirdRangeEmbed;
    dpp::component m_scrapeRankingsActionRow;
    bool m_bScrapeRankingsEmbedsPopulated;
};

#endif /* __BOT_H__ */
