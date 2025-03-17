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
#include <mutex>

const std::chrono::seconds k_cmdRateLimitPeriod = std::chrono::seconds(2);
const std::chrono::seconds k_interactionRateLimitPeriod = std::chrono::seconds(1);

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
    void onFormSubmit(const dpp::form_submit_t& event);

    void onCompletion(const dpp::confirmation_callback_t& callback, const std::string customID);
    void onCompletionReply(const dpp::confirmation_callback_t& callback, const std::string customID, const dpp::interaction_create_t event);

    void deleteGlobalCommand(std::string cmdName);

    void cmdHelp(const dpp::slashcommand_t& event);
    void cmdPing(const dpp::slashcommand_t& event);
    void cmdNewsletter(const dpp::slashcommand_t& event);
    void cmdSubscribe(const dpp::slashcommand_t& event);
    void cmdUnsubscribe(const dpp::slashcommand_t& event);

    void buildStaticComponents();
    bool buildNewsletter(const std::string countryCode, const RankRange rankRange, const Gamemode mode, dpp::message& message);

    dpp::cluster m_bot;
    RankingsDatabaseManager& m_rankingsDbm;
    BotConfigDatabaseManager& m_botConfigDbm;
    EmbedGenerator m_embedGenerator;
    std::atomic<bool> m_bIsInitialized;

    std::unordered_map<dpp::snowflake, std::chrono::steady_clock::time_point> m_latestCommands;
    std::unordered_map<dpp::snowflake, std::chrono::steady_clock::time_point> m_latestInteractions;
    std::mutex m_latestCommandsMtx;
    std::mutex m_latestInteractionsMtx;

    dpp::event_handle m_onLogId;
    dpp::event_handle m_onReadyId;
    dpp::event_handle m_onSlashCommandId;
    dpp::event_handle m_onButtonClickId;
    dpp::event_handle m_onFormSubmitId;

    dpp::embed m_helpEmbed;
    dpp::component m_scrapeRankingsActionRow1;
    dpp::component m_scrapeRankingsActionRow2;
};

#endif /* __BOT_H__ */
