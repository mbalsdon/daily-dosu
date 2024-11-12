#ifndef __BOT_H__
#define __BOT_H__

#include "ServerConfig.h"
#include "Util.h"

#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>

#include <string>
#include <cstdint>
#include <unordered_map>
#include <chrono>

const std::string k_twemojiClockPrefix = "https://cdn.jsdelivr.net/gh/twitter/twemoji@14.0.2/assets/72x72/1f55";
const std::string k_twemojiClockSuffix = ".png";

const std::chrono::seconds k_cmdRateLimitPeriod = std::chrono::seconds(5);
const std::chrono::seconds k_buttonRateLimitPeriod = std::chrono::seconds(3);

enum class RankRange
{
    First,
    Second,
    Third
};

const uint32_t k_helpColor = dpp::colors::rust;
const uint32_t k_firstRangeColor = dpp::colors::gold;
const uint32_t k_secondRangeColor = dpp::colors::silver;
const uint32_t k_thirdRangeColor = dpp::colors::bronze;

const std::string k_firstRangeButtonID = "first_range_button";
const std::string k_secondRangeButtonID = "second_range_button";
const std::string k_thirdRangeButtonID = "third_range_button";

const std::string k_cmdHelp = "help";
const std::string k_cmdPing = "ping";
const std::string k_cmdNewsletter = "newsletter";
const std::string k_cmdSubscribe = "subscribe";
const std::string k_cmdUnsubscribe = "unsubscribe";

const std::string k_cmdHelpDesc = "List the bot's commands.";
const std::string k_cmdPingDesc = "Check if the bot is alive.";
const std::string k_cmdNewsletterDesc = "Display the daily newsletter.";
const std::string k_cmdSubscribeDesc = "Enable daily newsletters for this channel.";
const std::string k_cmdUnsubscribeDesc = "Disable daily newsletters for this channel.";

class Bot 
{
public:
    Bot(const std::string& botToken);

    void start();
    void scrapePlayersCallback();
    void backupServerConfig();

private:
    void cmdHelp(const dpp::slashcommand_t& event);
    void cmdPing(const dpp::slashcommand_t& event);
    void cmdNewsletter(const dpp::slashcommand_t& event);
    void cmdSubscribe(const dpp::slashcommand_t& event);
    void cmdUnsubscribe(const dpp::slashcommand_t& event);

    void loadEmbeds();
    dpp::embed createScrapePlayersEmbed(RankRange rankRange, nlohmann::json jsonUsersCompact);
    void addPlayersToDescription(std::stringstream& description, nlohmann::json jsonUsersCompact, RankRange rankRange, bool isTop);
    ProfilePicture getScrapePlayersEmbedThumbnail(nlohmann::json jsonUsersCompact, RankRange rankRange);
    dpp::component createScrapePlayersActionRow();

    dpp::cluster m_bot;
    ServerConfig m_serverConfig;
    std::unordered_map<dpp::snowflake, std::chrono::steady_clock::time_point> m_latestCommands;
    bool m_embedsPopulated;

    dpp::embed m_firstRangeEmbed;
    dpp::embed m_secondRangeEmbed;
    dpp::embed m_thirdRangeEmbed;
};

#endif /* __BOT_H__ */
