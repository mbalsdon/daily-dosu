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
#include <cstring>

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

/**
 * Convert rank range to dpp color.
 */
uint32_t rankRangeToColor(RankRange rankRange)
{
    switch (rankRange.m_value)
    {
    case RankRange::First: return k_firstRangeColor;
    case RankRange::Second: return k_secondRangeColor;
    case RankRange::Third: return k_thirdRangeColor;
    default: return k_firstRangeColor;
    }
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
dpp::embed EmbedGenerator::scrapeRankingsEmbed(const std::string countryCode, const RankRange rankRange, std::vector<RankImprovement> top, std::vector<RankImprovement> bottom)
{
    LOG_DEBUG("Building scrapeRankings embed for range ", rankRange.toString(), "...");

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
    embed.add_field("", metadataTag(countryCode, rankRange.toInt()));

    if (!top.empty())
    {
        embed.set_thumbnail(top.at(0).user.pfpLink);
    }
    else if (!bottom.empty())
    {
        embed.set_thumbnail(bottom.at(0).user.pfpLink);
    }
    else
    {
        embed.set_thumbnail(k_defaultPfpUrl);
    }

    std::stringstream description;
    description << std::fixed << std::setprecision(2);

    embed.set_color(Detail::rankRangeToColor(rankRange));
    description << "## :up_arrow: Largest rank increases (#" << rankRange.toRange().first << " - #" << rankRange.toRange().second << "):\n";
    addPlayersToScrapeRankingsDescription(description, std::move(top), true);
    description << "## :down_arrow: Largest rank decreases (#" << rankRange.toRange().first << " - #" << rankRange.toRange().second << "):\n";
    addPlayersToScrapeRankingsDescription(description, std::move(bottom), false);

    embed.set_description(description.str());

    return embed;
}

/**
 * Create first scrapeRankings action row.
 */
dpp::component EmbedGenerator::scrapeRankingsActionRow1()
{
    LOG_DEBUG("Building 1st scrapeRankings action row...");

    dpp::component firstRangeButton;
    firstRangeButton.set_type(dpp::cot_button);
    firstRangeButton.set_label("#" + std::to_string(RankRange(0).toRange().first) + " - #" + std::to_string(RankRange(0).toRange().second));
    firstRangeButton.set_style(dpp::cos_primary);
    firstRangeButton.set_id(k_firstRangeButtonID);

    dpp::component secondRangeButton;
    secondRangeButton.set_type(dpp::cot_button);
    secondRangeButton.set_label("#" + std::to_string(RankRange(1).toRange().first) + " - #" + std::to_string(RankRange(1).toRange().second));
    secondRangeButton.set_style(dpp::cos_primary);
    secondRangeButton.set_id(k_secondRangeButtonID);

    dpp::component thirdRangeButton;
    thirdRangeButton.set_type(dpp::cot_button);
    thirdRangeButton.set_label("#" + std::to_string(RankRange(2).toRange().first) + " - #" + std::to_string(RankRange(2).toRange().second));
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
 * Create second scrapeRankings action row.
 */
dpp::component EmbedGenerator::scrapeRankingsActionRow2()
{
    LOG_DEBUG("Building 2nd scrapeRankings action row...");

    dpp::component filterCountryButton;
    filterCountryButton.set_type(dpp::cot_button);
    filterCountryButton.set_label("Filter by country");
    filterCountryButton.set_style(dpp::cos_primary);
    filterCountryButton.set_id(k_filterCountryButtonID);

    dpp::component clearFiltersButton;
    clearFiltersButton.set_type(dpp::cot_button);
    clearFiltersButton.set_label("Clear filters");
    clearFiltersButton.set_style(dpp::cos_primary);
    clearFiltersButton.set_id(k_clearFiltersButtonID);

    dpp::component actionRow;
    actionRow.set_type(dpp::cot_action_row);
    actionRow.add_component(filterCountryButton);
    actionRow.add_component(clearFiltersButton);

    return actionRow;
}

/**
 * Create scrapeRankings filter-by-country modal.
 */
dpp::interaction_modal_response EmbedGenerator::scrapeRankingsFilterCountryModal()
{
    LOG_DEBUG("Building scrapeRankings filter-by-country modal...");

    dpp::component textInput;
    textInput.set_type(dpp::cot_text);
    textInput.set_label("Enter country to sort by:");
    textInput.set_id(k_filterCountryTextInputID);
    textInput.set_placeholder("e.g. KR / KOR / South Korea");
    textInput.set_min_length(2);
    textInput.set_text_style(dpp::text_short);

    dpp::interaction_modal_response modal;
    modal.set_custom_id(k_filterCountryModalID);
    modal.set_title("Filter by country");
    modal.add_component(textInput);

    return modal;
}

/**
 * WARNING: Implementation is very coupled with the format of embed metadata.
 * Also super unsafe for a variety of reasons 🤪
 *
 * Glean metadata from message embed.
 */
bool EmbedGenerator::parseMetadata(const dpp::message message, EmbedMetadata& metadata)
{
    LOG_DEBUG("Parsing metadata from message ", message.id.operator uint64_t());

    if (message.embeds.size() < 1)
    {
        return false;
    }

    if (message.embeds[0].fields.size() < 1)
    {
        return false;
    }

    std::string metadataStr = message.embeds[0].fields[0].value;

    std::string countrySearchStr = "`COUNTRY: ";
    std::string rangeSearchStr = "`\n`RANGE: ";

    std::size_t rangePos = metadataStr.find(rangeSearchStr);
    metadata.m_countryCode = metadataStr.substr(strlen(countrySearchStr.c_str()), rangePos - strlen(countrySearchStr.c_str()));

    std::string rangeStr = metadataStr.substr(rangePos + strlen(rangeSearchStr.c_str()), rangePos + strlen(rangeSearchStr.c_str()) - metadataStr.find_last_of('`'));
    metadata.m_rankRange = RankRange(rangeStr);

    return true;
}

/**
 * Create metadata tag.
 */
std::string EmbedGenerator::metadataTag(const std::string countryCode, const RankRange rankRange)
{
    return "`COUNTRY: " + countryCode + "`\n`RANGE: " + std::to_string(rankRange.toInt()) + "`";
}

/**
 * Helper function for scrapeRankingsEmbed.
 * Format and pipe to description based on if these are top/bottom players.
 */
void EmbedGenerator::addPlayersToScrapeRankingsDescription(std::stringstream& description, std::vector<RankImprovement> players, const bool bIsTop)
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
