#include "EmbedGenerator.h"
#include "DosuConfig.h"
#include "Logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <cstddef>
#include <algorithm>

namespace Detail
{
/**
 * Convert given hour from system timezone to UTC.
 * Thanks to the dev who put this on SO 14 years ago and thanks to Claude for feeding it to me!
 */
int localToUtc(int localHour)
{
    if (localHour < 0 || localHour > 23)
    {
        localHour = localHour % 24;
    }

    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm = *std::localtime(&time_t_now);
    std::tm utc_tm = *std::gmtime(&time_t_now);
    int offset = local_tm.tm_hour - utc_tm.tm_hour;
    if (local_tm.tm_mday > utc_tm.tm_mday || (local_tm.tm_mday < utc_tm.tm_mday && local_tm.tm_hour > utc_tm.tm_hour))
    {
        offset += 24;
    }
    else if (local_tm.tm_mday < utc_tm.tm_mday || (local_tm.tm_mday > utc_tm.tm_mday && local_tm.tm_hour < utc_tm.tm_hour))
    {
        offset -= 24;
    }

    int utcHour = localHour - offset;
    while (utcHour < 0)
    {
        utcHour += 24;
    }
    while (utcHour >= 24)
    {
        utcHour -= 24;
    }

    return utcHour;
}

/**
 * Return hour formatted as XX:XX.
 */
std::string toHourString(int hour)
{
    std::string ret = "";
    if (hour < 10)
    {
        ret += "0";
    }
    
    ret += std::to_string(hour) + ":00";
    return ret;
}

/**
 * Convert numbers in range 0-23 to 1-12.
 */
int convertTo12Hour(int hour)
{
    if (hour == 0)
    {
        return 12;
    }
    else if (hour > 0 && hour <= 12)
    {
        return hour;
    }
    else
    {
        return hour - 12;
    }
}

/**
 * Convert integer to hex string (e.g. 12 -> "c")
 */
std::string toHexString(int i)
{
    std::stringstream ss;
    ss << std::hex << i;
    return ss.str();
}
} /* namespace Detail*/

/**
 * Create help embed.
 */
dpp::embed EmbedGenerator::helpEmbed()
{
    LOG_DEBUG("Building help embed...");

    dpp::embed embed = dpp::embed()
        .set_timestamp(time(0))
        .set_color(k_helpColor)
        .set_footer(
            dpp::embed_footer()
            .set_text("https://github.com/mbalsdon/daily-dosu")
        );

    std::stringstream description;
    description << "`" << k_cmdHelp << "` - " << k_cmdHelpDesc << "\n";
    description << "`" << k_cmdPing << "` - " << k_cmdPingDesc << "\n";
    description << "`" << k_cmdNewsletter << "` - " << k_cmdNewsletterDesc << "\n";
    description << "`" << k_cmdSubscribe << "` - " << k_cmdSubscribeDesc << "\n";
    description << "`" << k_cmdUnsubscribe << "` - " << k_cmdUnsubscribeDesc << "\n";
    embed.set_description(description.str());

    return embed;
}

/**
 * Create scrapeRankings embed for given rank range.
 */
dpp::embed EmbedGenerator::scrapeRankingsEmbed(RankRange rankRange, std::vector<RankImprovement> top, std::vector<RankImprovement> bottom)
{
    LOG_DEBUG("Building scrapeRankings embed for range ", static_cast<int>(rankRange), "...");

    // Add static embed fields
    int utcHour = Detail::localToUtc(DosuConfig::scrapeRankingsRunHour);
    std::string footerText = "Runs every day at " + Detail::toHourString(utcHour) + "UTC";
    std::string footerIcon = k_twemojiClockPrefix + Detail::toHexString(Detail::convertTo12Hour(utcHour) - 1) + k_twemojiClockSuffix;
    
    dpp::embed embed = dpp::embed()
        .set_author("Here's your daily dose of osu!", "https://github.com/mbalsdon/daily-dosu", k_iconImgUrl) // zesty ahh bot
        .set_timestamp(time(0))
        .set_footer(
            dpp::embed_footer()
            .set_text(footerText)
            .set_icon(footerIcon)
        );
    
    // Add range-dependent embed fields
    std::stringstream description;
    description << std::fixed << std::setprecision(2);
    if (rankRange == RankRange::First)
    {
        embed.set_color(k_firstRangeColor);
        description << "## :up_arrow: Largest rank increases (#1 - #" << k_firstRangeMax << "):\n";
        addPlayersToScrapeRankingsDescription(description, top, true);
        description << "## :down_arrow: Largest rank decreases (#1 - #" << k_firstRangeMax << "):\n";
        addPlayersToScrapeRankingsDescription(description, bottom, false);
    }
    else if (rankRange == RankRange::Second)
    {
        embed.set_color(k_secondRangeColor);
        description << "## :up_arrow: Largest rank increases (#" << k_firstRangeMax + 1 << " - #" << k_secondRangeMax << "):\n";
        addPlayersToScrapeRankingsDescription(description, top, true);
        description << "## :down_arrow: Largest rank decreases (#" << k_firstRangeMax + 1 << " - #" << k_secondRangeMax << "):\n";
        addPlayersToScrapeRankingsDescription(description, bottom, false);
    }
    else
    {
        embed.set_color(k_thirdRangeColor);
        description << "## :up_arrow: Largest rank increases (#" << k_secondRangeMax + 1 << " - #" << k_thirdRangeMax << "):\n";
        addPlayersToScrapeRankingsDescription(description, top, true);
        description << "## :down_arrow: Largest rank decreases (#" << k_secondRangeMax + 1 << " - #" << k_thirdRangeMax << "):\n";
        addPlayersToScrapeRankingsDescription(description, bottom, false);
    }

    embed.set_description(description.str());
    embed.set_thumbnail(scrapeRankingsThumbnail(top, bottom));

    return embed;
}

/**
 * Create scrapeRankings action row.
 */
dpp::component EmbedGenerator::scrapeRankingsActionRow()
{
    LOG_DEBUG("Building scrapeRankings action row...");

    dpp::component firstRangeButton;
    firstRangeButton.set_type(dpp::cot_button);
    firstRangeButton.set_label("#1 - #" + std::to_string(k_firstRangeMax));
    firstRangeButton.set_style(dpp::cos_primary);
    firstRangeButton.set_id(k_firstRangeButtonID);

    dpp::component secondRangeButton;
    secondRangeButton.set_type(dpp::cot_button);
    secondRangeButton.set_label("#" + std::to_string(k_firstRangeMax + 1) + " - #" + std::to_string(k_secondRangeMax));
    secondRangeButton.set_style(dpp::cos_primary);
    secondRangeButton.set_id(k_secondRangeButtonID);

    dpp::component thirdRangeButton;
    thirdRangeButton.set_type(dpp::cot_button);
    thirdRangeButton.set_label("#" + std::to_string(k_secondRangeMax + 1) + " - #" + std::to_string(k_thirdRangeMax));
    thirdRangeButton.set_style(dpp::cos_primary);
    thirdRangeButton.set_id(k_thirdRangeButtonID);

    dpp::component actionRow;
    actionRow.set_type(dpp::cot_action_row);
    actionRow.add_component(firstRangeButton);
    actionRow.add_component(secondRangeButton);
    actionRow.add_component(thirdRangeButton);

    return actionRow;
}

/**
 * Format and pipe to description based on if these are top/bottom players.
 */
void EmbedGenerator::addPlayersToScrapeRankingsDescription(std::stringstream& description, std::vector<RankImprovement> players, bool bIsTop)
{
    std::size_t displayUsersMax = (bIsTop ? k_numDisplayUsersTop : k_numDisplayUsersBottom);

    std::size_t j = 1;
    for (std::size_t i = 0; i < displayUsersMax; ++i)
    {
        if (i >= players.size())
        {
            break;
        }

        if (j > 1)
        {
            description << "\n";
        }

        RankingsUser user = players.at(i).user;
        std::transform(user.countryCode.begin(), user.countryCode.end(), user.countryCode.begin(), ::tolower);
        double relativeImprovement = players.at(i).relativeImprovement * (bIsTop ? 100. : -100.);

        // First line
        description << "**" << j << ".** :flag_" << user.countryCode << ": **[" << user.username << "](https://osu.ppy.sh/users/" << \
            user.userID << "/osu)** **(" << user.performancePoints << "pp | " << user.accuracy << "% | " << user.hoursPlayed << "hrs)**\n";

        // Second line
        description << "▸ Rank " << (bIsTop ? "increased" : "dropped") << " from #" << user.yesterdayRank << " to #" << user.currentRank << \
            " (" << relativeImprovement << "%)\n";

        ++j;
    }

    if (j == 1)
    {
        description << "Looks like nobody " << (bIsTop ? "gained" : "lost") << " ranks today... :shrug:\n";
    }
}

/**
 * Return pfp link of the first player-to-be-displayed.
 */
ProfilePicture EmbedGenerator::scrapeRankingsThumbnail(std::vector<RankImprovement> top, std::vector<RankImprovement> bottom)
{
    LOG_DEBUG("Figuring out who to put as the scrapeRankings thumbnail...");

    if (!top.empty())
    {
        return top.at(0).user.pfpLink;
    }
    
    if (!bottom.empty())
    {
        return bottom.at(0).user.pfpLink;
    }

    return ProfilePicture(k_defaultPfpUrl);
}
