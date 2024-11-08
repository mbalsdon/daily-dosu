#ifndef __BOT_H__
#define __BOT_H__

#include "OsuWrapper.h"

#include <dpp/dpp.h>

const std::string k_twemojiClockPrefix = "https://cdn.jsdelivr.net/gh/twitter/twemoji@14.0.2/assets/72x72/1f55";
const std::string k_twemojiClockSuffix = ".png";

enum class RankRange
{
    First,
    Second,
    Third
};

const uint32_t k_firstRangeColor = dpp::colors::gold;
const uint32_t k_secondRangeColor = dpp::colors::silver;
const uint32_t k_thirdRangeColor = dpp::colors::bronze;

const std::string k_firstRangeButtonID = "first_range_button";
const std::string k_secondRangeButtonID = "second_range_button";
const std::string k_thirdRangeButtonID = "third_range_button";

const std::string k_cmdPing = "ping";

class Bot 
{
public:
    Bot(const std::string& botToken);

    void start();
    void scrapePlayersCallback(int runHour);

private:
    void cmdPing(const dpp::slashcommand_t& event);

    dpp::embed createScrapePlayersEmbed(RankRange rankRange, int hour, nlohmann::json jsonUsersCompact);
    void addPlayersToDescription(std::stringstream& description, nlohmann::json jsonUsersCompact, RankRange rankRange, bool isTop);
    ProfilePicture getScrapePlayersEmbedThumbnail(nlohmann::json jsonUsersCompact, RankRange rankRange);

    dpp::cluster m_bot;
    dpp::embed m_firstRangeEmbed;
    dpp::embed m_secondRangeEmbed;
    dpp::embed m_thirdRangeEmbed;
};

#endif /* __BOT_H__ */
