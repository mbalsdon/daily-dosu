#include "Bot.h"
#include "Util.h"
#include "Logger.h"
#include "DosuConfig.h"

#include <iostream>
#include <fstream>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <filesystem>

namespace Detail
{

/**
 * Convert RankRange to associated JSON key.
 */
std::string rankRangeToKey(RankRange rankRange)
{
    if (rankRange == RankRange::First)
    {
        return k_firstRangeKey;
    }
    else if (rankRange == RankRange::Second)
    {
        return k_secondRangeKey;
    }
    else if (rankRange == RankRange::Third)
    {
        return k_thirdRangeKey;
    }
    else
    {
        std::string reason = std::string("Bot::Detail::rankRangeToKey - invalid rank range: ").append(std::to_string(static_cast<int>(rankRange)));
        throw std::invalid_argument(reason);
    }
}
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
    , m_serverConfig()
{
    LOG_DEBUG("Constructing Bot instance...");

    // Read data from disk to memory, if it exists
    if (!std::filesystem::exists(k_usersCompactFilePath))
    {
        m_embedsPopulated = false;
        return;
    }

    loadEmbeds();
    m_embedsPopulated = true;
}

/**
 * Start the discord bot.
 */
void Bot::start()
{
    LOG_DEBUG("Starting Discord bot...");

    m_bot.on_log([this](const dpp::log_t& event)
    {
        switch (event.severity)
        {
        case dpp::ll_debug:
            LOG_DEBUG(event.message);
            break;
        case dpp::ll_info:
            LOG_INFO(event.message);
            break;
        case dpp::ll_warning:
            LOG_WARN(event.message);
            break;
        case dpp::ll_error:
            LOG_ERROR(event.message);
            break;
        case dpp::ll_critical:
            LOG_ERROR("[CRITICAL] ", event.message);
            break;
        }
    });

    m_bot.on_slashcommand([this](const dpp::slashcommand_t& event)
    {
        std::string cmdName = event.command.get_command_name();

        // Verify user is under ratelimit
        auto now = std::chrono::steady_clock::now();
        auto userID = event.command.usr.id;
        if (m_latestCommands.count(userID))
        {
            auto lastUsed = m_latestCommands[userID];
            if (now - lastUsed < k_cmdRateLimitPeriod)
            {
                LOG_DEBUG("Command from user ", userID, " was blocked due to ratelimit");
                event.reply("You are using commands too quickly! Please wait a few seconds...");
                return;
            }
        }
        m_latestCommands[userID] = now;

        // Route command
        LOG_INFO("Received slash command /", cmdName, "; routing...");
        if (cmdName == k_cmdHelp)
        {
            cmdHelp(event);
        }
        else if (cmdName == k_cmdPing)
        {
            cmdPing(event);
        }
        else if (cmdName == k_cmdNewsletter)
        {
            cmdNewsletter(event);
        }
        else if (cmdName == k_cmdSubscribe)
        {
            cmdSubscribe(event);
        }
        else if (cmdName == k_cmdUnsubscribe)
        {
            cmdUnsubscribe(event);
        }
    });

    m_bot.on_ready([this](const dpp::ready_t& event)
    {
        if (dpp::run_once<struct register_bot_commands>())
        {
            m_bot.global_command_create(dpp::slashcommand(k_cmdHelp, k_cmdHelpDesc, m_bot.me.id));
            m_bot.global_command_create(dpp::slashcommand(k_cmdPing, k_cmdPingDesc, m_bot.me.id));
            m_bot.global_command_create(dpp::slashcommand(k_cmdNewsletter, k_cmdNewsletterDesc, m_bot.me.id));
            m_bot.global_command_create(dpp::slashcommand(k_cmdSubscribe, k_cmdSubscribeDesc, m_bot.me.id));
            m_bot.global_command_create(dpp::slashcommand(k_cmdUnsubscribe, k_cmdUnsubscribeDesc, m_bot.me.id));
        }
    });

    m_bot.on_button_click([this](const dpp::button_click_t& event)
    {
        auto buttonID = event.custom_id;
        LOG_DEBUG("Processing button click for button ", buttonID);

        auto now = std::chrono::steady_clock::now();
        auto userID = event.command.usr.id;
        if (m_latestCommands.count(userID))
        {
            auto lastUsed = m_latestCommands[userID];
            if (now - lastUsed < k_buttonRateLimitPeriod)
            {
                LOG_DEBUG("Buttonpress from user ", userID, " was blocked due to ratelimit");
                event.reply();
                return;
            }
        }
        m_latestCommands[userID] = now;

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
 * WARNING: This function is not thread-safe!
 *
 * Reads user data from disk, building and sending associated embeds to all subscribed channels.
 */
void Bot::scrapePlayersCallback()
{
    LOG_DEBUG("Executing callback for scrape job...");

    loadEmbeds();

    dpp::component actionRow = createScrapePlayersActionRow();

    // Build message
    dpp::message message;
    message.add_embed(m_firstRangeEmbed);
    message.add_component(actionRow);

    // Send message to subscriber list
    for (const auto& channelID : m_serverConfig.getChannelList())
    {
        message.channel_id = channelID;
        m_bot.message_create(message, [channelID](const dpp::confirmation_callback_t& callback)
        {
            if (callback.is_error())
            {
                LOG_WARN("Failed to send message to channel ", channelID, "; ", callback.get_error().message);
            }
        });
    }
}

/**
 * Backup server config.
 */
void Bot::backupServerConfig()
{
    m_serverConfig.backup();
}

/**
 * Run the help command.
 */
void Bot::cmdHelp(const dpp::slashcommand_t& event)
{
    dpp::embed embed = dpp::embed()
        .set_timestamp(time(0))
        .set_color(k_helpColor)
        .set_footer(
            dpp::embed_footer()
            .set_text("https://github.com/mbalsdon/daily-dosu")
        );

    std::stringstream description;
    description << "`help` - " << k_cmdHelpDesc << "\n";
    description << "`ping` - " << k_cmdPingDesc << "\n";
    description << "`newsletter` - " << k_cmdNewsletterDesc << "\n";
    description << "`subscribe` - " << k_cmdSubscribeDesc << "\n";
    description << "`unsubscribe` - " << k_cmdUnsubscribeDesc << "\n";
    embed.set_description(description.str());

    dpp::message message;
    message.add_embed(embed);
    event.reply(message);
}

/**
 * Run the ping command.
 */
void Bot::cmdPing(const dpp::slashcommand_t& event)
{
    event.reply("Pong!");
}

/**
 * Run the newsletter command.
 */
void Bot::cmdNewsletter(const dpp::slashcommand_t& event)
{
    if (!m_embedsPopulated)
    {
        event.reply("Couldn't find newsletter! If you still see this message tomorrow, it might be a bug.");
        return;
    }

    dpp::message message;
    dpp::component actionRow = createScrapePlayersActionRow();
    message.add_embed(m_firstRangeEmbed);
    message.add_component(actionRow);
    event.reply(message);
}

/**
 * Run the subscribe command.
 * Adds this channel to the list of channels to send daily messages to.
 */
void Bot::cmdSubscribe(const dpp::slashcommand_t& event)
{
    dpp::snowflake channelID = event.command.channel_id;
    m_serverConfig.addChannel(channelID);
    event.reply("Daily messages have been enabled for this channel.");
}

/**
 * Run the unsubscribe command.
 * Removes this channel to the list of channels to send daily messages to.
 */
void Bot::cmdUnsubscribe(const dpp::slashcommand_t& event)
{
    dpp::snowflake channelID = event.command.channel_id;
    m_serverConfig.removeChannel(channelID);
    event.reply("Daily messages have been disabled for this channel.");
}

/**
 * Load user data from disk and pre-build embeds.
 * The embeds are the same for everybody so this avoids some redundant computation.
 */
void Bot::loadEmbeds()
{
    LOG_DEBUG("Loading embeds into memory...");

    nlohmann::json jsonUsersCompact;
    try
    {
        std::ifstream jsonFile(k_usersCompactFilePath);
        if (!jsonFile.is_open())
        {
            std::string reason = std::string("Bot::loadEmbeds - failed to open ").append(k_usersCompactFilePath.string());
            throw std::runtime_error(reason);
        }
        jsonUsersCompact = nlohmann::json::parse(jsonFile);
        jsonFile.close();
    }
    catch(const std::exception& e)
    {
        throw std::runtime_error(e.what());
    }

    m_firstRangeEmbed = createScrapePlayersEmbed(RankRange::First, jsonUsersCompact);
    m_secondRangeEmbed = createScrapePlayersEmbed(RankRange::Second, jsonUsersCompact);
    m_thirdRangeEmbed = createScrapePlayersEmbed(RankRange::Third, jsonUsersCompact);

    m_embedsPopulated = true;
}

/**
 * Create scrapePlayers discord embed.
 */
dpp::embed Bot::createScrapePlayersEmbed(RankRange rankRange, nlohmann::json jsonUsersCompact)
{
    if ((rankRange != RankRange::First) &&
        (rankRange != RankRange::Second) &&
        (rankRange != RankRange::Third))
    {
        std::string reason = std::string("Bot::createScrapePlayersEmbed - invalid rank range: ").append(std::to_string(static_cast<int>(rankRange)));
        throw std::invalid_argument(reason);
    }

    // Add static embed fields
    int hour = DosuConfig::scrapePlayersRunHour;
    std::string footerText = "Runs every day at " + Detail::toHourString(hour) + "PST";
    std::string footerIcon = k_twemojiClockPrefix + Detail::toHexString(Detail::convertTo12Hour(hour) - 1) + k_twemojiClockSuffix;
    dpp::embed embed = dpp::embed()
        .set_author("Here's your daily dose of osu!", "https://github.com/mbalsdon/daily-dosu", "https://spreadnuts.s-ul.eu/HDlrjbqe.png") // zesty ahh bot
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
    std::string usersKey = (isTop ? k_topUsersKey : k_bottomUsersKey);
    nlohmann::json jsonUsers = jsonUsersCompact.at(Detail::rankRangeToKey(rankRange)).at(usersKey);

    // Add valid players to description
    std::size_t displayUsersMax = k_numDisplayUsers / 2;
    std::size_t j = 1;
    for (std::size_t i = 0; i < displayUsersMax; ++i)
    {
        if (i >= jsonUsers.size())
        {
            break;
        }

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
            description << "\n";
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

        description << "**" << j << ".** :flag_" << countryCode << ": **[" << username << "](https://osu.ppy.sh/users/" << userID << "/osu)** **(" << performancePoints << "pp | " << accuracy << "% | " << hoursPlayed << "hrs)**\n";
        description << "▸ Rank " << (isTop ? "increased" : "dropped") << " from #" << yesterdayRank << " to #" << currentRank << " (" << rankChangePercent << "%)\n";

        ++j;
    }
}

/**
 * Return pfp link of first valid player in rank increase section.
 */
ProfilePicture Bot::getScrapePlayersEmbedThumbnail(nlohmann::json jsonUsersCompact, RankRange rankRange)
{
    nlohmann::json jsonUsers = jsonUsersCompact.at(Detail::rankRangeToKey(rankRange)).at(k_topUsersKey);
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

/**
 * Create scrapePlayers action row.
 */
dpp::component Bot::createScrapePlayersActionRow()
{
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
