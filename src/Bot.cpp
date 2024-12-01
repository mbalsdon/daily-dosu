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
#include <cstddef>
#include <functional>
#include <thread>

/**
 * Bot constructor.
 */
Bot::Bot(const std::string& botToken, RankingsDatabaseManager& rankingsDbm, BotConfigDatabaseManager& botConfigDbm)
    : m_bot(botToken)
    , m_rankingsDbm(rankingsDbm)
    , m_botConfigDbm(botConfigDbm)
    , m_embedGenerator()
    , m_bIsInitialized(false)
{
    LOG_DEBUG("Constructing Bot...");
    buildHelpEmbeds();
    buildScrapeRankingsEmbeds();
}

/**
 * Bot destructor.
 */
Bot::~Bot()
{
    LOG_DEBUG("Destructing Bot...");
    stop();
}

/**
 * Start the discord bot.
 */
void Bot::start()
{
    if (m_bIsInitialized)
    {
        LOG_WARN("Bot already started!");
        return;
    }

    LOG_INFO("Starting Discord bot...");

    m_bot.on_log(std::bind(&Bot::onLog, this, std::placeholders::_1));
    m_bot.on_ready(std::bind(&Bot::onReady, this, std::placeholders::_1));
    m_bot.on_slashcommand(std::bind(&Bot::onSlashCommand, this, std::placeholders::_1));
    m_bot.on_button_click(std::bind(&Bot::onButtonClick, this, std::placeholders::_1));

    m_bot.start(dpp::st_return);

    m_bIsInitialized = true;
}

/**
 * Stop the discord bot.
 */
void Bot::stop()
{
    if (!m_bIsInitialized)
    {
        LOG_WARN("Bot already stopped!");
        return;
    }

    LOG_INFO("Stopping Discord bot...");

    m_bot.shutdown();

    m_bot.on_ready({});
    m_bot.on_slashcommand({});
    m_bot.on_button_click({});
    m_bot.on_log({});

    m_bIsInitialized = false;
}

/**
 * Reads user data from disk, building and sending associated embeds to all subscribed channels.
 */
void Bot::scrapeRankingsCallback()
{
    LOG_DEBUG("Executing callback for scrape job...");

    if (!buildScrapeRankingsEmbeds())
    {
        LOG_ERROR("Callback was triggered, but failed to build embeds!");
        return;
    }

    dpp::message message;
    message.add_embed(m_scrapeRankingsFirstRangeEmbed);
    message.add_component(m_scrapeRankingsActionRow);

    // Send message to subscriber list
    for (const auto& channelID : m_botConfigDbm.getChannelIDs())
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
 * Handle dpp log event.
 */
void Bot::onLog(const dpp::log_t& event)
{
    switch (event.severity)
    {
    case dpp::ll_trace:
        LOG_DEBUG("[TRACE] ", event.message);
        break;
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
}

/**
 * Handle ready signal event.
 */
void Bot::onReady(const dpp::ready_t&)
{
    if (dpp::run_once<struct register_bot_commands>())
    {
        LOG_DEBUG("Registering slash commands...");
        m_bot.global_command_create(dpp::slashcommand(k_cmdHelp, k_cmdHelpDesc, m_bot.me.id));
        m_bot.global_command_create(dpp::slashcommand(k_cmdPing, k_cmdPingDesc, m_bot.me.id));
        m_bot.global_command_create(dpp::slashcommand(k_cmdNewsletter, k_cmdNewsletterDesc, m_bot.me.id));
        m_bot.global_command_create(dpp::slashcommand(k_cmdSubscribe, k_cmdSubscribeDesc, m_bot.me.id));
        m_bot.global_command_create(dpp::slashcommand(k_cmdUnsubscribe, k_cmdUnsubscribeDesc, m_bot.me.id));
    }
}

/**
 * Handle slash command event.
 */
void Bot::onSlashCommand(const dpp::slashcommand_t& event)
{
    std::string cmdName = event.command.get_command_name();
    LOG_INFO("Received slash command /", cmdName, "; routing...");

    // Verify user is under ratelimit
    auto now = std::chrono::steady_clock::now();
    auto userID = event.command.usr.id;
    if (m_latestCommands.count(userID))
    {
        auto lastUsed = m_latestCommands[userID];
        if (now - lastUsed < k_cmdRateLimitPeriod)
        {
            LOG_DEBUG("Command from user ", userID.operator uint64_t(), " was blocked due to ratelimit!");
            event.reply(dpp::message("You are using commands too quickly! Please wait a few seconds...").set_flags(dpp::m_ephemeral));
            return;
        }
    }
    m_latestCommands[userID] = now;

    // Route command
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
    else
    {
        LOG_WARN("Got unknown slash command /", cmdName, "; attempting to remove it...");
        std::thread(&Bot::deleteGlobalCommand, this, cmdName).detach();
        event.reply(dpp::message("Command does not exist! If it is still listed in a few minutes, this may be a bug.").set_flags(dpp::m_ephemeral));
    }
}

/**
 * Handle button press event.
 */
void Bot::onButtonClick(const dpp::button_click_t& event)
{
    auto buttonID = event.custom_id;
    LOG_DEBUG("Processing button click for button ", buttonID);

    // Verify user is under ratelimit
    auto now = std::chrono::steady_clock::now();
    auto userID = event.command.usr.id;
    if (m_latestCommands.count(userID))
    {
        auto lastUsed = m_latestCommands[userID];
        if (now - lastUsed < k_buttonRateLimitPeriod)
        {
            LOG_DEBUG("Buttonpress from user ", userID.operator uint64_t(), " was blocked due to ratelimit");
            event.reply(dpp::message("You are pressing buttons too quickly! Please wait a few seconds...").set_flags(dpp::m_ephemeral));
            return;
        }
    }
    m_latestCommands[userID] = now;

    // Route buttonpress
    if ((buttonID == k_firstRangeButtonID) ||
        (buttonID == k_secondRangeButtonID) ||
        (buttonID == k_thirdRangeButtonID))
    {
        dpp::message message = event.command.msg;
        message.embeds.clear();
        if (buttonID == k_firstRangeButtonID)
        {
            message.embeds.push_back(m_scrapeRankingsFirstRangeEmbed);
        }
        else if (buttonID == k_secondRangeButtonID)
        {
            message.embeds.push_back(m_scrapeRankingsSecondRangeEmbed);
        }
        else
        {
            message.embeds.push_back(m_scrapeRankingsThirdRangeEmbed);
        }
        m_bot.message_edit(message, [buttonID](const dpp::confirmation_callback_t& callback)
        {
            if (callback.is_error())
            {
                LOG_WARN("Failed to edit message after button ", buttonID, " was pressed; ", callback.get_error().message);
            }
        });
        event.reply();
    }
}

/**
 * WARNING: This function is blocking and should be called in a seperate thread rather than in event handlers.
 *
 * Delete global command by name.
 */
void Bot::deleteGlobalCommand(std::string cmdName)
{
    LOG_DEBUG("Removing stale command ", cmdName, "...");
    try
    {
        auto slashCommands = m_bot.global_commands_get_sync();
        for (const auto& cmd : slashCommands)
        {
            if (cmd.second.name == cmdName)
            {
                m_bot.global_command_delete_sync(cmd.first);
            }
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR("Caught error while trying to delete command ", cmdName, "; ", e.what());
    }
}

/**
 * Run the help command.
 */
void Bot::cmdHelp(const dpp::slashcommand_t& event)
{
    dpp::message message;
    message.add_embed(m_helpEmbed);
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
    if (!m_bScrapeRankingsEmbedsPopulated)
    {
        event.reply(dpp::message("Couldn't find newsletter! If you still see this message tomorrow, it might be a bug.").set_flags(dpp::m_ephemeral));
        return;
    }

    dpp::message message;
    message.add_embed(m_scrapeRankingsFirstRangeEmbed);
    message.add_component(m_scrapeRankingsActionRow);

    event.reply(message);
}

/**
 * Run the subscribe command.
 * Adds this channel to the list of channels to send daily messages to.
 */
void Bot::cmdSubscribe(const dpp::slashcommand_t& event)
{
    dpp::snowflake channelID = event.command.channel_id;
    m_botConfigDbm.addChannel(channelID);
    event.reply("Daily messages have been enabled for this channel.");
}

/**
 * Run the unsubscribe command.
 * Removes this channel to the list of channels to send daily messages to.
 */
void Bot::cmdUnsubscribe(const dpp::slashcommand_t& event)
{
    dpp::snowflake channelID = event.command.channel_id;
    m_botConfigDbm.removeChannel(channelID);
    event.reply("Daily messages have been disabled for this channel.");
}

/**
 * Construct help embeds and store in memory.
 */
void Bot::buildHelpEmbeds()
{
    LOG_DEBUG("Populating help embeds...");
    m_helpEmbed = m_embedGenerator.helpEmbed();
}

/**
 * Construct scrapeRankings embeds and store in memory if associated data is valid.
 * Return true if data is valid, false otherwise.
 */
bool Bot::buildScrapeRankingsEmbeds()
{
    LOG_DEBUG("Populating scrapeRankings embeds...");
    auto lastWriteTime = m_rankingsDbm.lastWriteTime();
    auto now = std::filesystem::file_time_type::clock::now();
    std::chrono::hours ageHours = std::chrono::duration_cast<std::chrono::hours>(now - lastWriteTime);
    if ((ageHours > std::chrono::hours(25)) || m_rankingsDbm.isRankingsEmpty())
    {
        LOG_DEBUG("Data is invalid, leaving scrapeRankings embeds empty...");
        m_bScrapeRankingsEmbedsPopulated = false;
    }
    else
    {
        LOG_DEBUG("Data is valid, populating scrapeRankings embeds..");
        std::size_t numDisplayUsers = k_numDisplayUsersTop + k_numDisplayUsersBottom;

        std::vector<RankImprovement> firstRangeTop = m_rankingsDbm.getTopRankImprovements(1, k_firstRangeMax, numDisplayUsers);
        std::vector<RankImprovement> firstRangeBottom = m_rankingsDbm.getBottomRankImprovements(1, k_firstRangeMax, numDisplayUsers);
        m_scrapeRankingsFirstRangeEmbed = m_embedGenerator.scrapeRankingsEmbed(RankRange::First, firstRangeTop, firstRangeBottom);

        std::vector<RankImprovement> secondRangeTop = m_rankingsDbm.getTopRankImprovements(k_firstRangeMax + 1, k_secondRangeMax, numDisplayUsers);
        std::vector<RankImprovement> secondRangeBottom = m_rankingsDbm.getBottomRankImprovements(k_firstRangeMax + 1, k_secondRangeMax, numDisplayUsers);
        m_scrapeRankingsSecondRangeEmbed = m_embedGenerator.scrapeRankingsEmbed(RankRange::Second, secondRangeTop, secondRangeBottom);

        std::vector<RankImprovement> thirdRangeTop = m_rankingsDbm.getTopRankImprovements(k_secondRangeMax + 1, k_thirdRangeMax, numDisplayUsers);
        std::vector<RankImprovement> thirdRangeBottom = m_rankingsDbm.getBottomRankImprovements(k_secondRangeMax + 1, k_thirdRangeMax, numDisplayUsers);
        m_scrapeRankingsThirdRangeEmbed = m_embedGenerator.scrapeRankingsEmbed(RankRange::Third, thirdRangeTop, thirdRangeBottom);

        m_scrapeRankingsActionRow = m_embedGenerator.scrapeRankingsActionRow();

        m_bScrapeRankingsEmbedsPopulated = true;
    }

    return m_bScrapeRankingsEmbedsPopulated;
}
