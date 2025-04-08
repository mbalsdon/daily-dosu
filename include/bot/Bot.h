#ifndef __BOT_H__
#define __BOT_H__

#include "RankingsDatabase.h"
#include "TopPlaysDatabase.h"
#include "BotConfigDatabase.h"
#include "EmbedGenerator.h"
#include "Util.h"

#include <dpp/dpp.h>

#include <string>
#include <cstdint>
#include <unordered_map>
#include <chrono>
#include <atomic>
#include <mutex>
#include <memory>

constexpr std::chrono::seconds k_cmdRateLimitPeriod = std::chrono::seconds(2);
constexpr std::chrono::seconds k_interactionRateLimitPeriod = std::chrono::seconds(1);

const std::string k_cmdNewsletterPageOptionDesc = "Which newsletter to retrieve";
const std::string k_cmdSubscribePageOptionDesc = "Which newsletter to subscribe to";
const std::string k_cmdUnsubscribePageOptionDesc = "Which newsletter to unsubscribe from";

const std::string k_newsletterPageOptionName = "page";

/**
 * DPP wrapper for the Discord bot. All event handling logic (and nothing else) should go here.
 * Where possible, the strategy should be to build embeds/components as little as possible, opting
 * instead to save them to memory such that we save compute / disk accesses.
 */
class Bot
{
public:
    Bot(std::string const& botToken, std::shared_ptr<RankingsDatabase> pRankingsDb, std::shared_ptr<TopPlaysDatabase> pTopPlaysDb, std::shared_ptr<BotConfigDatabase> pBotConfigDb);
    ~Bot();

    void start();
    void stop();
    void scrapeRankingsCallback();
    void topPlaysCallback();

private:
    void onLog_(dpp::log_t const& event) const noexcept;
    void onReady_(dpp::ready_t const& event);
    void onSlashCommand_(dpp::slashcommand_t const& event);
    void onButtonClick_(dpp::button_click_t const& event);
    void onFormSubmit_(dpp::form_submit_t const& event);

    void onCompletion_(dpp::confirmation_callback_t const& callback, std::string const& customID) const;
    void onCompletionReply_(dpp::confirmation_callback_t const& callback, std::string const& customID, dpp::interaction_create_t const& event) const;

    void deleteGlobalCommand_(std::string const& cmdName);

    void cmdHelp_(dpp::slashcommand_t const& event) const;
    void cmdPing_(dpp::slashcommand_t const& event) const;
    void cmdNewsletter_(dpp::slashcommand_t const& event);
    void cmdSubscribe_(dpp::slashcommand_t const& event);
    void cmdUnsubscribe_(dpp::slashcommand_t const& event);

    void buildStaticComponents_();
    [[nodiscard]] bool buildScrapeRankingsNewsletter_(std::string const& countryCode, RankRange const& rankRange, Gamemode const& mode, dpp::message& message /* out */);
    [[nodiscard]] bool buildTopPlaysNewsletter_(std::string const& countryCode, Gamemode const& mode, dpp::message& message /* out */);

    dpp::cluster m_bot;
    std::unique_ptr<EmbedGenerator> m_pEmbedGenerator = std::make_unique<EmbedGenerator>();
    std::shared_ptr<RankingsDatabase> m_pRankingsDb;
    std::shared_ptr<TopPlaysDatabase> m_pTopPlaysDb;
    std::shared_ptr<BotConfigDatabase> m_pBotConfigDb;
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
    dpp::component m_topPlaysActionRow1;
};

#endif /* __BOT_H__ */
