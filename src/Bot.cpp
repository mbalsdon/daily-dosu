#include "Bot.h"
#include "DosuConfig.h"
#include "ScrapePlayers.h"

#include <dpp/nlohmann/json.hpp>

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace Detail
{
/**
 * Return true if any json user fields are null.
 */
bool isJsonUserInvalid(nlohmann::json jsonUser)
{
    return (jsonUser.at(k_userIDKey).is_null() ||
            jsonUser.at(k_usernameKey).is_null() ||
            jsonUser.at(k_countryCodeKey).is_null() ||
            jsonUser.at(k_pfpLinkKey).is_null() ||
            jsonUser.at(k_hoursPlayedKey).is_null() ||
            jsonUser.at(k_currentRankKey).is_null() ||
            jsonUser.at(k_yesterdayRankKey).is_null() ||
            jsonUser.at(k_rankChangeRatioKey).is_null());
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
} /* namespace Detail */

/**
 * Bot constructor.
 */
Bot::Bot(const std::string& botToken) 
    : m_bot(botToken)
{}

/**
 * Start the discord bot.
 */
void Bot::start()
{
    std::cout << "Bot::start - starting Discord bot..." << std::endl;

    m_bot.on_log(dpp::utility::cout_logger());

    m_bot.on_slashcommand([this](const dpp::slashcommand_t& event)
    {
        std::string cmdName = event.command.get_command_name();
        std::cout << "Bot - received slash command /" << cmdName << "; routing..." << std::endl;

        if (cmdName == k_cmdPing)
        {
            cmdPing(event);
        }
    });

    m_bot.on_ready([this](const dpp::ready_t& event)
    {
        if (dpp::run_once<struct register_bot_commands>())
        {
            m_bot.global_command_create(dpp::slashcommand(k_cmdPing, "Ping pong!", m_bot.me.id));
        }
    });

    m_bot.on_button_click([this](const dpp::button_click_t& event)
    {
        auto buttonID = event.custom_id;
        if ((buttonID == k_firstRangeButtonID) ||
            (buttonID == k_secondRangeButtonID) ||
            (buttonID == k_thirdRangeButtonID))
        {
            dpp::message message = event.command.msg;
            message.embeds.clear();
            if (buttonID == k_firstRangeButtonID)
            {
                message.embeds.push_back(m_firstRangeEmbed);
            }
            else if (buttonID == k_secondRangeButtonID)
            {
                message.embeds.push_back(m_secondRangeEmbed);
            }
            else
            {
                message.embeds.push_back(m_thirdRangeEmbed);
            }
            m_bot.message_edit(message);
            event.reply();
        }
    });

    m_bot.start(dpp::st_wait);
}

/**
 * Run the ping command.
 */
void Bot::cmdPing(const dpp::slashcommand_t& event)
{
    event.reply("Pong!");
}

/**
 * Read user data and display it thru a discord embed.
 */
void Bot::scrapePlayersCallback(int hour)
{
    if ((hour < 0) || (hour > 23))
    {
        hour = hour % 24;
        std::cout << "Bot::scrapePlayersCallback - hour is out of bounds; normalizing to " << hour << std::endl;
    }

    // Read data into memory
    nlohmann::json jsonUsersCompact;
    try
    {
        std::filesystem::path rootDir = std::filesystem::path(__FILE__).parent_path().parent_path();
        std::filesystem::path dataFilePath = rootDir / k_dataDirName / k_usersCompactFileName;
        std::ifstream jsonFile(dataFilePath);
        if (!jsonFile.is_open())
        {
            std::string reason = std::string("Bot::scrapePlayersCallback - failed to open ").append(dataFilePath.string());
            throw std::runtime_error(reason);
        }
        jsonUsersCompact = nlohmann::json::parse(jsonFile);
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(e.what());
    }

    // Build embeds
    m_firstRangeEmbed = createScrapePlayersEmbed(RankRange::First, hour, jsonUsersCompact);
    m_secondRangeEmbed = createScrapePlayersEmbed(RankRange::Second, hour, jsonUsersCompact);
    m_thirdRangeEmbed = createScrapePlayersEmbed(RankRange::Third, hour, jsonUsersCompact);

    // Build buttons
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

    // Build message
    dpp::message message;
    message.add_embed(m_firstRangeEmbed);
    message.add_component(actionRow);

    message.channel_id = 1001155170777960618;
    m_bot.message_create(message);
}

/**
 * Create scrapePlayers discord embed.
 */
dpp::embed Bot::createScrapePlayersEmbed(RankRange rankRange, int hour, nlohmann::json jsonUsersCompact)
{
    if ((hour < 0) || (hour > 23))
    {
        hour = hour % 24;
        std::cout << "Bot::scrapePlayersCallback - hour is out of bounds; normalizing to " << hour << std::endl;
    }

    if ((rankRange != RankRange::First) &&
        (rankRange != RankRange::Second) &&
        (rankRange != RankRange::Third))
    {
        std::string reason = std::string("Bot::createScrapePlayersEmbed - invalid rank range: ").append(std::to_string(static_cast<int>(rankRange)));
        throw std::runtime_error(reason);
    }

    // Add static embed fields
    std::string footerText = "Runs every day at " + Detail::toHourString(hour) + "PST"; // FUTURE: clean (use chrono, convert to UTC without offset)
    std::string footerIcon = k_twemojiClockPrefix + Detail::toHexString(Detail::convertTo12Hour(hour) - 1) + k_twemojiClockSuffix;
    dpp::embed embed = dpp::embed()
        .set_author("Here's your daily dose of osu!", "https://github.com/mbalsdon/daily-dosu", "") // zesty ahh bot
        .set_timestamp(time(0))
        .set_footer(
            dpp::embed_footer()
            .set_text(footerText)
            .set_icon(footerIcon)
        );

    // Add range dependent embed fields
    std::stringstream description;
    description << std::fixed << std::setprecision(2);
    if (rankRange == RankRange::First)
    {
        embed.set_color(k_firstRangeColor);
        description << "## :up_arrow: Largest rank increases (#1 - #" << k_firstRangeMax << "):\n";
        addPlayersToDescription(description, jsonUsersCompact, rankRange, true);
        description << "## :down_arrow: Largest rank decreases (#1 - #" << k_firstRangeMax << "):\n";
        addPlayersToDescription(description, jsonUsersCompact, rankRange, false);
    }
    else if (rankRange == RankRange::Second)
    {
        embed.set_color(k_secondRangeColor);
        description << "## :up_arrow: Largest rank increases (#" << k_firstRangeMax + 1 << " - #" << k_secondRangeMax << "):\n";
        addPlayersToDescription(description, jsonUsersCompact, rankRange, true);
        description << "## :down_arrow: Largest rank decreases (#" << k_firstRangeMax + 1 << " - #" << k_secondRangeMax << "):\n";
        addPlayersToDescription(description, jsonUsersCompact, rankRange, false);
    }
    else
    {
        embed.set_color(k_thirdRangeColor);
        description << "## :up_arrow: Largest rank increases (#" << k_secondRangeMax + 1 << " - #" << k_thirdRangeMax << "):\n";
        addPlayersToDescription(description, jsonUsersCompact, rankRange, true);
        description << "## :down_arrow: Largest rank decreases (#" << k_secondRangeMax + 1 << " - #" << k_thirdRangeMax << "):\n";
        addPlayersToDescription(description, jsonUsersCompact, rankRange, false);
    }

    embed.set_description(description.str());
    embed.set_thumbnail(getScrapePlayersEmbedThumbnail(jsonUsersCompact, rankRange));

    return embed;
}

/**
 * Pipe to description based on given rank range and whether we want the top/bottom players.
 */
void Bot::addPlayersToDescription(std::stringstream& description, nlohmann::json jsonUsersCompact, RankRange rankRange, bool isTop)
{
    // Decide where to pull from
    nlohmann::json jsonUsers;
    if (rankRange == RankRange::First)
    {
        if (isTop)
        {
            jsonUsers = jsonUsersCompact.at(k_firstRangeKey).at(k_topUsersKey);
        }
        else
        {
            jsonUsers = jsonUsersCompact.at(k_firstRangeKey).at(k_bottomUsersKey);
        }
    }
    else if (rankRange == RankRange::Second)
    {
        if (isTop)
        {
            jsonUsers = jsonUsersCompact.at(k_secondRangeKey).at(k_topUsersKey);
        }
        else
        {
            jsonUsers = jsonUsersCompact.at(k_secondRangeKey).at(k_bottomUsersKey);
        }
    }
    else if (rankRange == RankRange::Third)
    {
        if (isTop)
        {
            jsonUsers = jsonUsersCompact.at(k_thirdRangeKey).at(k_topUsersKey);
        }
        else
        {
            jsonUsers = jsonUsersCompact.at(k_thirdRangeKey).at(k_bottomUsersKey);
        }
    }
    else
    {
        std::string reason = std::string("Bot::addPlayersToDescription - invalid rank range: ").append(std::to_string(static_cast<int>(rankRange)));
        throw std::runtime_error(reason);
    }

    // Add valid players to description
    size_t displayUsersMax = k_numDisplayUsers / 2;
    size_t j = 1;
    for (size_t i = 0; i < displayUsersMax; ++i)
    {
        nlohmann::json jsonUser = jsonUsers.at(i);
        if (Detail::isJsonUserInvalid(jsonUser))
        {
            ++displayUsersMax;
            continue;
        }

        RankChangeRatio rankChangeRatio = jsonUser.at(k_rankChangeRatioKey).get<RankChangeRatio>();
        if ((isTop && (rankChangeRatio <= 0.)) ||
            (!isTop && (rankChangeRatio >= 0.)))
        {
            break;
        }

        if (j > 1)
        {
            description << "▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫▫\n";
        }

        UserID userID = jsonUser.at(k_userIDKey).get<UserID>();
        Username username = jsonUser.at(k_usernameKey).get<Username>();
        CountryCode countryCode = jsonUser.at(k_countryCodeKey).get<CountryCode>();
        ProfilePicture pfpLink = jsonUser.at(k_pfpLinkKey).get<ProfilePicture>();
        HoursPlayed hoursPlayed = jsonUser.at(k_hoursPlayedKey).get<HoursPlayed>();
        Rank currentRank = jsonUser.at(k_currentRankKey).get<Rank>();
        Rank yesterdayRank = jsonUser.at(k_yesterdayRankKey).get<Rank>();
        PerformancePoints performancePoints = jsonUser.at(k_performancePointsKey).get<PerformancePoints>();
        Accuracy accuracy = jsonUser.at(k_accuracyKey).get<Accuracy>();

        std::transform(countryCode.begin(), countryCode.end(), countryCode.begin(), ::tolower);

        double rankChangePercent = rankChangeRatio * 100.;
        if (!isTop)
        {
            rankChangePercent = 0. - rankChangePercent;
        }

        description << "**" << j << ".** :flag_" << countryCode << ": **[" << username << "](https://osu.ppy.sh/users/" << userID << "/osu)** **(" << performancePoints << "pp | " << accuracy << "% | " << hoursPlayed << "hrs)**\n";
        description << "▸ Rank " << (isTop ? "increased" : "decreased") << " by " << rankChangePercent << "% (#" << yesterdayRank << " → #" << currentRank << ")\n";

        ++j;
    }
}

/**
 * Return pfp link of first valid player in rank increase section.
 */
ProfilePicture Bot::getScrapePlayersEmbedThumbnail(nlohmann::json jsonUsersCompact, RankRange rankRange)
{
    // Decide where to pull from
    nlohmann::json jsonUsers;
    if (rankRange == RankRange::First)
    {
        jsonUsers = jsonUsersCompact.at(k_firstRangeKey).at(k_topUsersKey);
    }
    else if (rankRange == RankRange::Second)
    {
        jsonUsers = jsonUsersCompact.at(k_secondRangeKey).at(k_topUsersKey);
    }
    else if (rankRange == RankRange::Third)
    {
        jsonUsers = jsonUsersCompact.at(k_thirdRangeKey).at(k_topUsersKey);
    }
    else
    {
        std::string reason = std::string("Bot::getScrapePlayersEmbedThumbnail - invalid rank range: ").append(std::to_string(static_cast<int>(rankRange)));
        throw std::runtime_error(reason);
    }

    // Return first valid player
    for (const auto& jsonUser : jsonUsers)
    {
        if (Detail::isJsonUserInvalid(jsonUser))
        {
            continue;
        }

        return jsonUser.at(k_pfpLinkKey).get<ProfilePicture>();
    }

    return ProfilePicture("https://a.ppy.sh/2?1657169614.png");
}
