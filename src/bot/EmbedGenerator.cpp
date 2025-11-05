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
#include <regex>
#include <optional>
#include <set>

namespace
{
/**
 * Convert given hour from system timezone to UTC.
 * Thanks to the dev who put this on SO 14 years ago and thanks to Claude for feeding it to me!
 */
[[nodiscard]] int localToUtc(int localHour) noexcept
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
[[nodiscard]] std::string toHourString(int const& hour)
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
[[nodiscard]] int convertTo12Hour(int const& hour) noexcept
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
[[nodiscard]] std::string toHexString(int const& i)
{
    std::stringstream ss;
    ss << std::hex << i;
    return ss.str();
}

/**
 * Convert rank range to dpp color.
 */
[[nodiscard]] uint32_t rankRangeToColor(RankRange const& rankRange) noexcept
{
    switch (rankRange.getValue())
    {
    case RankRange::First: return k_firstRangeColor;
    case RankRange::Second: return k_secondRangeColor;
    case RankRange::Third: return k_thirdRangeColor;
    default: return k_firstRangeColor;
    }
}

/**
 * Convert mods to discord emoji string.
 */
[[nodiscard]] std::string modsToEmojiString(OsuMods const& mods) noexcept
{
    std::string ret = "";

    std::set<std::string> modStrings = mods.get();
    std::size_t i = 0;
    for (auto modStr : modStrings)
    {
        if (i > 0)
        {
            ret += " ";
        }

        std::transform(modStr.begin(), modStr.end(), modStr.begin(), ::toupper);

        if (modStr == "MR") ret += DosuConfig::discordBotStrings[k_modMRKey];
        else if (modStr == "TP") ret += DosuConfig::discordBotStrings[k_modTPKey];
        else if (modStr == "SD") ret += DosuConfig::discordBotStrings[k_modSDKey];
        else if (modStr == "SO") ret += DosuConfig::discordBotStrings[k_modSOKey];
        else if (modStr == "AP") ret += DosuConfig::discordBotStrings[k_modAPKey];
        else if (modStr == "RX") ret += DosuConfig::discordBotStrings[k_modRXKey];
        else if (modStr == "RD") ret += DosuConfig::discordBotStrings[k_modRDKey];
        else if (modStr == "PF") ret += DosuConfig::discordBotStrings[k_modPFKey];
        else if (modStr == "NF") ret += DosuConfig::discordBotStrings[k_modNFKey];
        else if (modStr == "NC") ret += DosuConfig::discordBotStrings[k_modNCKey];
        else if (modStr == "CP") ret += DosuConfig::discordBotStrings[k_modCPKey];
        else if (modStr == "9K") ret += DosuConfig::discordBotStrings[k_mod9KKey];
        else if (modStr == "8K") ret += DosuConfig::discordBotStrings[k_mod8KKey];
        else if (modStr == "7K") ret += DosuConfig::discordBotStrings[k_mod7KKey];
        else if (modStr == "6K") ret += DosuConfig::discordBotStrings[k_mod6KKey];
        else if (modStr == "5K") ret += DosuConfig::discordBotStrings[k_mod5KKey];
        else if (modStr == "4K") ret += DosuConfig::discordBotStrings[k_mod4KKey];
        else if (modStr == "3K") ret += DosuConfig::discordBotStrings[k_mod3KKey];
        else if (modStr == "2K") ret += DosuConfig::discordBotStrings[k_mod2KKey];
        else if (modStr == "1K") ret += DosuConfig::discordBotStrings[k_mod1KKey];
        else if (modStr == "HD") ret += DosuConfig::discordBotStrings[k_modHDKey];
        else if (modStr == "HR") ret += DosuConfig::discordBotStrings[k_modHRKey];
        else if (modStr == "HT") ret += DosuConfig::discordBotStrings[k_modHTKey];
        else if (modStr == "FL") ret += DosuConfig::discordBotStrings[k_modFLKey];
        else if (modStr == "FI") ret += DosuConfig::discordBotStrings[k_modFIKey];
        else if (modStr == "EZ") ret += DosuConfig::discordBotStrings[k_modEZKey];
        else if (modStr == "DT") ret += DosuConfig::discordBotStrings[k_modDTKey];
        else if (modStr == "CM") ret += DosuConfig::discordBotStrings[k_modCMKey];
        else if (modStr == "AT") ret += DosuConfig::discordBotStrings[k_modATKey];
        else
        {
            LOG_ERROR("Unhandled mod string: ", modStr);
            ret += modStr;
        }

        ++i;
    }

    return ret;
}

/**
 * Convert letter rank to discord emoji string.
 */
[[nodiscard]] std::string letterRankToEmojiString(std::string const& letterRank) noexcept
{
    if (letterRank == "X")       return DosuConfig::discordBotStrings[k_letterRankXKey];
    else if (letterRank == "XH") return DosuConfig::discordBotStrings[k_letterRankXHKey];
    else if (letterRank == "S")  return DosuConfig::discordBotStrings[k_letterRankSKey];
    else if (letterRank == "SH") return DosuConfig::discordBotStrings[k_letterRankSHKey];
    else if (letterRank == "D")  return DosuConfig::discordBotStrings[k_letterRankDKey];
    else if (letterRank == "C")  return DosuConfig::discordBotStrings[k_letterRankCKey];
    else if (letterRank == "B")  return DosuConfig::discordBotStrings[k_letterRankBKey];
    else if (letterRank == "A")  return DosuConfig::discordBotStrings[k_letterRankAKey];
    else
    {
        LOG_ERROR("Unhandled letter rank: ", letterRank);
        return letterRank;
    }
}
} /* namespace */

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

/**
 * Create help embed.
 */
[[nodiscard]] dpp::embed EmbedGenerator::helpEmbed() const
{
    LOG_DEBUG("Building help embed");

    dpp::embed embed = dpp::embed()
        .set_timestamp(time(0))
        .set_color(k_helpColor)
        .set_footer(
            dpp::embed_footer()
            .set_text("https://ko-fi.com/spreadnuts")
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
[[nodiscard]] dpp::embed EmbedGenerator::scrapeRankingsEmbed(
    std::string const& countryCode,
    RankRange const& rankRange,
    Gamemode const& mode,
    std::vector<RankImprovement> const& top,
    std::vector<RankImprovement> const& bottom) const
{
    LOG_DEBUG("Building scrapeRankings embed for range=", rankRange.toString(), ", mode=", mode.toString(), "countryCode=", countryCode);

    // Add static embed fields
    int utcHour = localToUtc(DosuConfig::scrapeRankingsRunHour);
    std::string footerText = "Runs every day at " + toHourString(utcHour) + "UTC";
    std::string footerIcon = k_twemojiClockPrefix + toHexString(convertTo12Hour(utcHour) - 1) + k_twemojiClockSuffix;

    dpp::embed embed = dpp::embed()
        .set_author("Here's your daily dose of osu!", "https://ko-fi.com/spreadnuts", k_iconImgUrl) // zesty ahh bot
        .set_timestamp(time(0))
        .set_footer(
            dpp::embed_footer()
            .set_text(footerText)
            .set_icon(footerIcon)
        );

    // Add range-dependent embed fields
    embed.add_field("", EmbedMetadata(countryCode, rankRange, mode, std::nullopt).toEmbedString());

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

    embed.set_color(rankRangeToColor(rankRange));
    description << "## :up_arrow: Largest rank increases (#" << rankRange.toRange().first << " - #" << rankRange.toRange().second << "):\n";
    addPlayersToScrapeRankingsDescription_(description, top, true, mode);
    description << "## :down_arrow: Largest rank decreases (#" << rankRange.toRange().first << " - #" << rankRange.toRange().second << "):\n";
    addPlayersToScrapeRankingsDescription_(description, bottom, false, mode);

    embed.set_description(description.str());

    return embed;
}

/**
 * Create getTopPlays embed for given gamemode.
 */
[[nodiscard]] dpp::embed EmbedGenerator::topPlaysEmbed(
    std::vector<TopPlay> const& topPlays,
    Gamemode const& mode,
    std::string const& countryCode,
    std::string const& mods) const
{
    LOG_DEBUG("Building getTopPlays embed for mode=", mode.toString(), ", countryCode=", countryCode, ", mods=", mods);

    // Add static embed fields
    int utcHour = localToUtc(DosuConfig::topPlaysRunHour);
    std::string footerText = "Runs every day at " + toHourString(utcHour) + "UTC";
    std::string footerIcon = k_twemojiClockPrefix + toHexString(convertTo12Hour(utcHour) - 1) + k_twemojiClockSuffix;

    dpp::embed embed = dpp::embed()
        .set_author("Here's your daily dose of osu!", "https://ko-fi.com/spreadnuts", k_iconImgUrl)
        .set_timestamp(time(0))
        .set_color(k_topPlaysColor)
        .set_footer(
            dpp::embed_footer()
            .set_text(footerText)
            .set_icon(footerIcon)
        );

    // Add variable embed fields
    embed.add_field("", EmbedMetadata(countryCode, std::nullopt, mode, OsuMods(mods)).toEmbedString());
    if (!topPlays.empty())
    {
        embed.set_thumbnail(topPlays.at(0).score.user.pfpLink);
    }
    else
    {
        embed.set_thumbnail(k_defaultPfpUrl);
    }

    std::stringstream description;
    description << std::fixed << std::setprecision(2);

    description << "## :medal: Top plays:\n";
    addPlayersToTopPlaysDescription_(description, topPlays, mode);

    embed.set_description(description.str());

    return embed;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

/**
 * Create first scrapeRankings action row.
 */
[[nodiscard]] dpp::component EmbedGenerator::scrapeRankingsActionRow1() const
{
    LOG_DEBUG("Building 1st scrapeRankings action row");

    dpp::component firstRangeButton;
    firstRangeButton.set_type(dpp::cot_button);
    firstRangeButton.set_label("#" + std::to_string(RankRange(0).toRange().first) + " - #" + std::to_string(RankRange(0).toRange().second));
    firstRangeButton.set_style(dpp::cos_primary);
    firstRangeButton.set_id(k_scrapeRankingsFirstRangeButtonID);

    dpp::component secondRangeButton;
    secondRangeButton.set_type(dpp::cot_button);
    secondRangeButton.set_label("#" + std::to_string(RankRange(1).toRange().first) + " - #" + std::to_string(RankRange(1).toRange().second));
    secondRangeButton.set_style(dpp::cos_primary);
    secondRangeButton.set_id(k_scrapeRankingsSecondRangeButtonID);

    dpp::component thirdRangeButton;
    thirdRangeButton.set_type(dpp::cot_button);
    thirdRangeButton.set_label("#" + std::to_string(RankRange(2).toRange().first) + " - #" + std::to_string(RankRange(2).toRange().second));
    thirdRangeButton.set_style(dpp::cos_primary);
    thirdRangeButton.set_id(k_scrapeRankingsThirdRangeButtonID);

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
[[nodiscard]] dpp::component EmbedGenerator::scrapeRankingsActionRow2() const
{
    LOG_DEBUG("Building 2nd scrapeRankings action row");

    dpp::component filterCountryButton;
    filterCountryButton.set_type(dpp::cot_button);
    filterCountryButton.set_label("Filter by country");
    filterCountryButton.set_style(dpp::cos_primary);
    filterCountryButton.set_id(k_scrapeRankingsFilterCountryButtonID);

    dpp::component selectModeButton;
    selectModeButton.set_type(dpp::cot_button);
    selectModeButton.set_label("Select gamemode");
    selectModeButton.set_style(dpp::cos_primary);
    selectModeButton.set_id(k_scrapeRankingsSelectModeButtonID);

    dpp::component clearFiltersButton;
    clearFiltersButton.set_type(dpp::cot_button);
    clearFiltersButton.set_label("Reset all");
    clearFiltersButton.set_style(dpp::cos_primary);
    clearFiltersButton.set_id(k_scrapeRankingsResetAllButtonID);

    dpp::component actionRow;
    actionRow.set_type(dpp::cot_action_row);
    actionRow.add_component(filterCountryButton);
    actionRow.add_component(selectModeButton);
    actionRow.add_component(clearFiltersButton);

    return actionRow;
}

/**
 * Create first scrapeRankings action row.
 */
[[nodiscard]] dpp::component EmbedGenerator::topPlaysActionRow1() const
{
    LOG_DEBUG("Building 1st getTopPlays action row");

    dpp::component filterCountryButton;
    filterCountryButton.set_type(dpp::cot_button);
    filterCountryButton.set_label("Filter by country");
    filterCountryButton.set_style(dpp::cos_primary);
    filterCountryButton.set_id(k_topPlaysFilterCountryButtonID);

    dpp::component filterModsButton;
    filterModsButton.set_type(dpp::cot_button);
    filterModsButton.set_label("Filter by mods");
    filterModsButton.set_style(dpp::cos_primary);
    filterModsButton.set_id(k_topPlaysFilterModsButtonID);

    dpp::component selectModeButton;
    selectModeButton.set_type(dpp::cot_button);
    selectModeButton.set_label("Select gamemode");
    selectModeButton.set_style(dpp::cos_primary);
    selectModeButton.set_id(k_topPlaysSelectModeButtonID);

    dpp::component clearFiltersButton;
    clearFiltersButton.set_type(dpp::cot_button);
    clearFiltersButton.set_label("Reset all");
    clearFiltersButton.set_style(dpp::cos_primary);
    clearFiltersButton.set_id(k_topPlaysResetAllButtonID);

    dpp::component actionRow;
    actionRow.set_type(dpp::cot_action_row);
    actionRow.add_component(filterCountryButton);
    actionRow.add_component(filterModsButton);
    actionRow.add_component(selectModeButton);
    actionRow.add_component(clearFiltersButton);

    return actionRow;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

/**
 * Create scrapeRankings filter-by-country modal.
 */
[[nodiscard]] dpp::interaction_modal_response EmbedGenerator::scrapeRankingsFilterCountryModal() const
{
    LOG_DEBUG("Building scrapeRankings filter-by-country modal");

    dpp::component textInput;
    textInput.set_type(dpp::cot_text);
    textInput.set_label("Enter country to filter by:");
    textInput.set_id(k_scrapeRankingsFilterCountryTextInputID);
    textInput.set_placeholder("e.g. KR / KOR / South Korea");
    textInput.set_text_style(dpp::text_short);

    dpp::interaction_modal_response modal;
    modal.set_custom_id(k_scrapeRankingsFilterCountryModalID);
    modal.set_title("Filter by country");
    modal.add_component(textInput);

    return modal;
}

/**
 * Create scrapeRankings filter-by-mode modal.
 */
[[nodiscard]] dpp::interaction_modal_response EmbedGenerator::scrapeRankingsFilterModeModal() const
{
    LOG_DEBUG("Building scrapeRankings filter-by-mode modal");

    dpp::component textInput;
    textInput.set_type(dpp::cot_text);
    textInput.set_label("Select gamemode to view:");
    textInput.set_id(k_scrapeRankingsSelectModeTextInputID);
    textInput.set_placeholder("osu / taiko / catch / mania");
    textInput.set_text_style(dpp::text_short);

    dpp::interaction_modal_response modal;
    modal.set_custom_id(k_scrapeRankingsSelectModeModalID);
    modal.set_title("Select gamemode");
    modal.add_component(textInput);

    return modal;
}

/**
 * Create getTopPlays filter-by-country modal.
 */
[[nodiscard]] dpp::interaction_modal_response EmbedGenerator::topPlaysFilterCountryModal() const
{
    LOG_DEBUG("Building getTopPlays filter-by-country modal");

    dpp::component textInput;
    textInput.set_type(dpp::cot_text);
    textInput.set_label("Enter country to filter by:");
    textInput.set_id(k_topPlaysFilterCountryTextInputID);
    textInput.set_placeholder("e.g. KR / KOR / South Korea");
    textInput.set_text_style(dpp::text_short);

    dpp::interaction_modal_response modal;
    modal.set_custom_id(k_topPlaysFilterCountryModalID);
    modal.set_title("Filter by country");
    modal.add_component(textInput);

    return modal;
}

[[nodiscard]] dpp::interaction_modal_response EmbedGenerator::topPlaysFilterModsModal() const
{
    LOG_DEBUG("Building getTopPlays filter-by-mods modal");

    dpp::component textInput;
    textInput.set_type(dpp::cot_text);
    textInput.set_label("Enter mods to filter by:");
    textInput.set_id(k_topPlaysFilterModsTextInputID);
    textInput.set_placeholder("e.g. HD, HDDT, DTHRDTFL");
    textInput.set_text_style(dpp::text_short);

    dpp::interaction_modal_response modal;
    modal.set_custom_id(k_topPlaysFilterModsModalID);
    modal.set_title("Filter by mods");
    modal.add_component(textInput);

    return modal;
}

/**
 * Create getTopPlays filter-by-mode modal.
 */
[[nodiscard]] dpp::interaction_modal_response EmbedGenerator::topPlaysFilterModeModal() const
{
    LOG_DEBUG("Building getTopPlays filter-by-mode modal");

    dpp::component textInput;
    textInput.set_type(dpp::cot_text);
    textInput.set_label("Select gamemode to view:");
    textInput.set_id(k_topPlaysSelectModeTextInputID);
    textInput.set_placeholder("osu / taiko / catch / mania");
    textInput.set_text_style(dpp::text_short);

    dpp::interaction_modal_response modal;
    modal.set_custom_id(k_topPlaysSelectModeModalID);
    modal.set_title("Select gamemode");
    modal.add_component(textInput);

    return modal;
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

/**
 * Helper function for scrapeRankingsEmbed.
 * Format and pipe to description based on if these are top/bottom players.
 */
void EmbedGenerator::addPlayersToScrapeRankingsDescription_(
    std::stringstream& description /* out */,
    std::vector<RankImprovement> const& players,
    bool const& bIsTop,
    Gamemode const& mode) const noexcept
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
            user.userID << "/" << mode.toString() << ")** **(" << user.performancePoints << "pp | " << user.accuracy << "% | " << user.hoursPlayed << "hrs)**\n";

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
 * Helper function for topPlaysEmbed.
 * Format and pipe to description.
 */
void EmbedGenerator::addPlayersToTopPlaysDescription_(
    std::stringstream& description /* out */,
    std::vector<TopPlay> const& topPlays,
    Gamemode const& mode) const noexcept
{
    if (topPlays.size() < 1)
    {
        description << "Looks like nobody set top plays today... :shrug:\n";
        return;
    }

    for (std::size_t i = 0; i < topPlays.size(); ++i)
    {
        if (i > 0)
        {
            description << "\n";
        }

        TopPlay topPlay = topPlays.at(i);
        int64_t rank = topPlay.rank;
        Score score = topPlay.score;
        RankingsUser user = topPlay.score.user;
        Beatmap beatmap = topPlay.score.beatmap;

        // Fill in description
        // First line
        std::transform(user.countryCode.begin(), user.countryCode.end(), user.countryCode.begin(), ::tolower);
        description << "**" << rank << ".** :flag_" << user.countryCode << ": **[" << user.username << "](https://osu.ppy.sh/users/" << \
            user.userID << "/" << mode.toString() << ")** **(" << user.performancePoints << "pp | " << user.accuracy << "% | " << \
            user.hoursPlayed << "hrs)**\n";
        // Second line
        description << "▸ [" << beatmap.artist << " - " << beatmap.title << " [" << beatmap.difficultyName << \
            "]](https://osu.ppy.sh/beatmaps/" << beatmap.beatmapID << ") " << modsToEmojiString(score.mods) << "\n";
        // Third line
        description << "▸ " << letterRankToEmojiString(score.letterRank) << " ― **" << score.performancePoints << "pp** ― " << score.accuracy * 100. << \
            "% ― " << score.combo << "x/" << beatmap.maxCombo << "x ― " << score.count300 << "/" << score.count100 << \
            (mode == Gamemode::Taiko ? "" : "/" + std::to_string(score.count50)) << "/" << score.countMiss << "\n";
        // Fourth line
        std::string mapsetCreatorUrlEncoded = std::regex_replace(beatmap.mapsetCreator, std::regex(" "), "%20");
        description << "▸ Set <t:" << score.createdAt.toEpochTime() << ":R> ― " << beatmap.starRating << "★ ― Mapset by [" << beatmap.mapsetCreator << \
            "](https://osu.ppy.sh/users/" << mapsetCreatorUrlEncoded << "/" << mode.toString() << ")\n";
    }
}
