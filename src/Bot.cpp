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
#include <algorithm>
#include <utility>

namespace
{
RankRange buttonIDToRankRange(std::string buttonID)
{
    if (buttonID == k_firstRangeButtonID)
    {
        return RankRange(0);
    }
    else if (buttonID == k_secondRangeButtonID)
    {
        return RankRange(1);
    }
    else
    {
        return RankRange(2);
    }
}
} /* namespace */

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
    buildStaticComponents();
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

    m_onLogId = m_bot.on_log(std::bind(&Bot::onLog, this, std::placeholders::_1));
    m_onReadyId = m_bot.on_ready(std::bind(&Bot::onReady, this, std::placeholders::_1));
    m_onSlashCommandId = m_bot.on_slashcommand(std::bind(&Bot::onSlashCommand, this, std::placeholders::_1));
    m_onButtonClickId = m_bot.on_button_click(std::bind(&Bot::onButtonClick, this, std::placeholders::_1));
    m_onFormSubmitId = m_bot.on_form_submit(std::bind(&Bot::onFormSubmit, this, std::placeholders::_1));

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

    m_bot.on_ready.detach(m_onLogId);
    m_bot.on_slashcommand.detach(m_onReadyId);
    m_bot.on_button_click.detach(m_onSlashCommandId);
    m_bot.on_log.detach(m_onButtonClickId);
    m_bot.on_form_submit.detach(m_onFormSubmitId);

    m_bot.shutdown();

    m_bIsInitialized = false;
}

/**
 * Build and send newsletter to all subscribed channels.
 */
void Bot::scrapeRankingsCallback()
{
    LOG_DEBUG("Executing callback for scrape job...");

    // Build message
    dpp::message message;
    if (!buildNewsletter(k_global, RankRange(0), Gamemode(0), message))
    {
        LOG_ERROR("Callback was triggered, but failed to build newsletter!");
        return;
    }

    // Send to channels
    for (const auto& channelID : m_botConfigDbm.getChannelIDs())
    {
        message.channel_id = channelID;
        std::string onCompletionID = std::string("send-to-channel_").append(std::to_string(channelID.operator uint64_t()));
        m_bot.message_create(message, std::bind(&Bot::onCompletion, this, std::placeholders::_1, onCompletionID));
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
    {
        std::lock_guard<std::mutex> lock(m_latestCommandsMtx);
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
    }

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
        deleteGlobalCommand(cmdName);
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
    {
        std::lock_guard<std::mutex> lock(m_latestInteractionsMtx);
        if (m_latestInteractions.count(userID))
        {
            auto lastUsed = m_latestInteractions[userID];
            if (now - lastUsed < k_interactionRateLimitPeriod)
            {
                LOG_DEBUG("Buttonpress from user ", userID.operator uint64_t(), " was blocked due to ratelimit");
                event.reply(dpp::message("You are pressing buttons too quickly! Please wait a few seconds...").set_flags(dpp::m_ephemeral));
                return;
            }
        }
        m_latestInteractions[userID] = now;
    }

    // Route buttonpress
    if ((buttonID == k_firstRangeButtonID) ||
        (buttonID == k_secondRangeButtonID) ||
        (buttonID == k_thirdRangeButtonID))
    {
        dpp::message message = event.command.msg;
        EmbedMetadata embedMetadata;
        if (!m_embedGenerator.parseMetadata(message, embedMetadata))
        {
            LOG_ERROR("Failed to parse metadata for current embed!");
            event.reply(dpp::message("Failed to parse metadata! This is a bug...").set_flags(dpp::m_ephemeral));
            return;
        }

        if (!buildNewsletter(embedMetadata.m_countryCode, buttonIDToRankRange(buttonID), embedMetadata.m_gamemode, message))
        {
            LOG_ERROR(buttonID, " was pressed, but failed to build newsletter!");
            event.reply(dpp::message("Failed to build new embed! This is a bug...").set_flags(dpp::m_ephemeral));
            return;
        }

        m_bot.message_edit(message, std::bind(&Bot::onCompletionReply, this, std::placeholders::_1, buttonID, event));
    }
    else if (buttonID == k_filterCountryButtonID)
    {
        dpp::interaction_modal_response modal = m_embedGenerator.scrapeRankingsFilterCountryModal();
        event.dialog(modal, std::bind(&Bot::onCompletion, this, std::placeholders::_1, buttonID));
    }
    else if (buttonID == k_selectModeButtonID)
    {
        dpp::interaction_modal_response modal = m_embedGenerator.scrapeRankingsFilterModeModal();
        event.dialog(modal, std::bind(&Bot::onCompletion, this, std::placeholders::_1, buttonID));
    }
    else if (buttonID == k_resetAllButtonID)
    {
        dpp::message message = event.command.msg;

        if (!buildNewsletter(k_global, RankRange(0), Gamemode(0), message))
        {
            LOG_ERROR(k_resetAllButtonID, " was pressed, but failed to build newsletter!");
            event.reply(dpp::message("Failed to build new embed! This is a bug...").set_flags(dpp::m_ephemeral));
            return;
        }

        m_bot.message_edit(message, std::bind(&Bot::onCompletionReply, this, std::placeholders::_1, buttonID, event));
    }
}

/**
 * Handle form submission event.
 */
void Bot::onFormSubmit(const dpp::form_submit_t& event)
{
    auto formID = event.custom_id;
    LOG_DEBUG("Processing form submission for modal ", formID);

    // Route form submission
    if (formID == k_filterCountryModalID)
    {
        std::string countryInput = std::get<std::string>(event.components[0].components[0].value);
        LOG_DEBUG("Processing request to filter country by ", countryInput);

        std::string countryInputUpper = countryInput;
        std::transform(countryInputUpper.begin(), countryInputUpper.end(), countryInputUpper.begin(), ::toupper);

        std::string countryCode = std::string(ISO3166Alpha2Converter::toAlpha2(countryInputUpper));
        if (countryCode.empty())
        {
            event.reply(dpp::message(countryInput + " is not a valid country!").set_flags(dpp::m_ephemeral));
            return;
        }

        dpp::message message = event.command.msg;

        EmbedMetadata embedMetadata;
        if (!m_embedGenerator.parseMetadata(message, embedMetadata))
        {
            LOG_ERROR("Failed to parse metadata for current embed!");
            event.reply(dpp::message("Failed to parse metadata! This is a bug...").set_flags(dpp::m_ephemeral));
            return;
        }

        if (!buildNewsletter(countryCode, embedMetadata.m_rankRange, embedMetadata.m_gamemode, message))
        {
            LOG_ERROR(k_filterCountryModalID, " was submitted with countryCode=", countryInput, ", but failed to build newsletter!");
            event.reply(dpp::message("Failed to build new embed! This is a bug...").set_flags(dpp::m_ephemeral));
            return;
        }

        m_bot.message_edit(message, std::bind(&Bot::onCompletionReply, this, std::placeholders::_1, formID, event));
    }
    else if (formID == k_selectModeModalID)
    {
        std::string modeInput = std::get<std::string>(event.components[0].components[0].value);
        LOG_DEBUG("Processing request to select mode ", modeInput);

        std::string modeInputUpper = modeInput;

        Gamemode mode;
        if (!Gamemode::fromString(modeInput, mode))
        {
            event.reply(dpp::message(modeInput + " is not a valid gamemode!").set_flags(dpp::m_ephemeral));
            return;
        }

        dpp::message message = event.command.msg;

        EmbedMetadata embedMetadata;
        if (!m_embedGenerator.parseMetadata(message, embedMetadata))
        {
            LOG_ERROR("Failed to parse metadata for current embed!");
            event.reply(dpp::message("Failed to parse metadata! This is a bug...").set_flags(dpp::m_ephemeral));
            return;
        }

        if (!buildNewsletter(embedMetadata.m_countryCode, embedMetadata.m_rankRange, mode, message))
        {
            LOG_ERROR(k_selectModeModalID, " was submitted with mode=", modeInput, ", but failed to build newsletter!");
            event.reply(dpp::message("Failed to build new embed! This is a bug...").set_flags(dpp::m_ephemeral));
            return;
        }

        m_bot.message_edit(message, std::bind(&Bot::onCompletionReply, this, std::placeholders::_1, formID, event));
    }
}

/**
 * Generic completion callback handler.
 */
void Bot::onCompletion(const dpp::confirmation_callback_t& callback, const std::string customID)
{
    if (callback.is_error())
    {
        auto error = callback.get_error();
        LOG_WARN("Failed to handle interaction for ", customID, "; ", error.message);
    }
}

/**
 * Generic completion callback handler. Replies to event.
 */
void Bot::onCompletionReply(const dpp::confirmation_callback_t& callback, const std::string customID, const dpp::interaction_create_t event)
{
    if (callback.is_error())
    {
        auto error = callback.get_error();
        LOG_WARN("Failed to handle interaction for ", customID, "; ", error.message);

        switch (error.code)
        {
        case 50001:
            event.reply(dpp::message("Bot is missing channel access!").set_flags(dpp::m_ephemeral));
            return;
        case 50013:
            event.reply(dpp::message("Bot is missing permissions!").set_flags(dpp::m_ephemeral));
            return;
        default:
            event.reply(dpp::message("Button failed! Got error '" + error.message + "'... This is possibly a bug.").set_flags(dpp::m_ephemeral));
            return;
        }
    }
    else // not error
    {
        event.reply();
        return;
    }
}

/**
 * Delete global command by name.
 */
void Bot::deleteGlobalCommand(std::string cmdName)
{
    LOG_DEBUG("Removing stale command ", cmdName, "...");

    m_bot.global_commands_get(
        [this, cmdName](const dpp::confirmation_callback_t& getCallback)
        {
            if (getCallback.is_error())
            {
                auto error = getCallback.get_error();
                LOG_WARN("Failed to get global commands; ", error.message);
            }

            auto slashCommands = getCallback.get<dpp::slashcommand_map>();
            for (const auto& cmd : slashCommands)
            {
                if (cmd.second.name == cmdName)
                {
                    m_bot.global_command_delete(cmd.first,
                        [cmd](const dpp::confirmation_callback_t& deleteCallback)
                        {
                            if (deleteCallback.is_error())
                            {
                                auto error = deleteCallback.get_error();
                                LOG_WARN("Failed to delete global command ", cmd.first, "; ", error.message);
                            }
                        });
                }
            }
        });
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
    dpp::message message;
    if (!buildNewsletter(k_global, RankRange(0), Gamemode(0), message))
    {
        LOG_WARN("Got command, but failed to build newsletter!");
        event.reply(dpp::message("Couldn't find newsletter! If you still see this message tomorrow, it might be a bug.").set_flags(dpp::m_ephemeral));
        return;
    }

    event.reply(message);
}

/**
 * Run the subscribe command.
 * Adds this channel to the list of channels to send daily messages to.
 */
void Bot::cmdSubscribe(const dpp::slashcommand_t& event)
{
    dpp::permission permissions = event.command.app_permissions;
    if (!permissions.has(dpp::p_send_messages))
    {
        event.reply(dpp::message("Cannot subscribe! Bot lacks permissions to send messages in this channel...").set_flags(dpp::m_ephemeral));
        return;
    }

    dpp::snowflake channelID = event.command.channel_id;
    if (!m_botConfigDbm.channelExists(channelID))
    {
        m_botConfigDbm.addChannel(channelID);
        event.reply("Daily messages have been enabled for this channel.");
    }
    else
    {
        event.reply("Daily messages are already enabled for this channel!");
    }
}

/**
 * Run the unsubscribe command.
 * Removes this channel to the list of channels to send daily messages to.
 */
void Bot::cmdUnsubscribe(const dpp::slashcommand_t& event)
{
    dpp::snowflake channelID = event.command.channel_id;
    if (m_botConfigDbm.channelExists(channelID))
    {
        event.reply("Daily messages have been disabled for this channel.");
        m_botConfigDbm.removeChannel(channelID);
    }
    else
    {
        event.reply("Daily messages are already disabled for this channel!");
    }
}

/**
 * Build and store unchanging components to memory.
 */
void Bot::buildStaticComponents()
{
    LOG_DEBUG("Building static components...");

    m_helpEmbed = m_embedGenerator.helpEmbed();
    m_scrapeRankingsActionRow1 = m_embedGenerator.scrapeRankingsActionRow1();
    m_scrapeRankingsActionRow2 = m_embedGenerator.scrapeRankingsActionRow2();
    /* FUTURE: The filter-by-country modal is static, but DPP doesn't seem to handle interaction_modal_response like other
    components/embeds/etc. If we store it here it gets double-freed when the program exits and I'm too lazy to figure out why.
    Check if this is stil the case when library is updated. */
}

/**
 * Build newsletter. Returns true if data was valid, false otherwise.
 */
bool Bot::buildNewsletter(const std::string countryCode, const RankRange rankRange, const Gamemode mode, dpp::message& message)
{
    LOG_DEBUG("Building newsletter for countryCode=", countryCode, ", rankRange=", rankRange.toString(), " mode=", mode.toString(), "...");

    auto lastWriteTime = m_rankingsDbm.lastWriteTime();
    auto now = std::filesystem::file_time_type::clock::now();
    std::chrono::hours ageHours = std::chrono::duration_cast<std::chrono::hours>(now - lastWriteTime);
    if (ageHours > k_maxValidScrapeRankingsHour)
    {
        LOG_WARN("Data is invalid - ageHours=", ageHours.count(), " > ", k_maxValidScrapeRankingsHour.count());
        return false;
    }

    if (m_rankingsDbm.hasEmptyRankingsTable())
    {
        LOG_WARN("Data is invalid - rankings database is empty!");
        return false;
    }

    LOG_DEBUG("Data is valid - building newsletter...");

    auto range = rankRange.toRange();
    std::vector<RankImprovement> rangeTop = m_rankingsDbm.getTopRankImprovements(countryCode, range.first, range.second, k_numDisplayUsers, mode);
    std::vector<RankImprovement> rangeBottom = m_rankingsDbm.getBottomRankImprovements(countryCode, range.first, range.second, k_numDisplayUsers, mode);
    dpp::embed newsletterEmbed = m_embedGenerator.scrapeRankingsEmbed(std::move(countryCode), std::move(rankRange), std::move(mode), std::move(rangeTop), std::move(rangeBottom));

    message.embeds.clear();
    message.components.clear();

    message.add_embed(newsletterEmbed);
    message.add_component(m_scrapeRankingsActionRow1);
    message.add_component(m_scrapeRankingsActionRow2);

    return true;
}
