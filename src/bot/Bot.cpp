#include "Bot.h"
#include "Logger.h"

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
RankRange buttonIDToRankRange(std::string const& buttonID)
{
    if (buttonID == k_scrapeRankingsFirstRangeButtonID)
    {
        return RankRange(0);
    }
    else if (buttonID == k_scrapeRankingsSecondRangeButtonID)
    {
        return RankRange(1);
    }
    else
    {
        return RankRange(2);
    }
}

bool parseMetadata(dpp::message const& message, EmbedMetadata& metadata /* out */)
{
    if (message.embeds.size() < 1) return false;
    if (message.embeds[0].fields.size() < 1) return false;

    std::string metadataStr = message.embeds[0].fields[0].value;
    metadata = EmbedMetadata(metadataStr);
    return true;
}
} /* namespace */

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

/**
 * Bot constructor.
 */
Bot::Bot(std::string const& botToken, std::shared_ptr<RankingsDatabase> pRankingsDb, std::shared_ptr<TopPlaysDatabase> pTopPlaysDb, std::shared_ptr<BotConfigDatabase> pBotConfigDb)
    : m_bot(botToken)
    , m_pRankingsDb(pRankingsDb)
    , m_pTopPlaysDb(pTopPlaysDb)
    , m_pBotConfigDb(pBotConfigDb)
    , m_bIsInitialized(false)
{
    LOG_DEBUG("Constructing Bot");
    buildStaticComponents_();
}

/**
 * Bot destructor.
 */
Bot::~Bot()
{
    stop();
}

/**
 * Start the discord bot.
 */
void Bot::start()
{
    if (m_bIsInitialized)
    {
        return;
    }

    LOG_INFO("Starting Discord bot");

    m_onLogId = m_bot.on_log(std::bind(&Bot::onLog_, this, std::placeholders::_1));
    m_onReadyId = m_bot.on_ready(std::bind(&Bot::onReady_, this, std::placeholders::_1));
    m_onSlashCommandId = m_bot.on_slashcommand(std::bind(&Bot::onSlashCommand_, this, std::placeholders::_1));
    m_onButtonClickId = m_bot.on_button_click(std::bind(&Bot::onButtonClick_, this, std::placeholders::_1));
    m_onFormSubmitId = m_bot.on_form_submit(std::bind(&Bot::onFormSubmit_, this, std::placeholders::_1));

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
        return;
    }

    LOG_INFO("Stopping Discord bot");

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
    LOG_DEBUG("Executing callback for scrape job");

    // Build message
    dpp::message message;
    if (!buildScrapeRankingsNewsletter_(k_global, RankRange(0), Gamemode(0), message))
    {
        LOG_ERROR("Callback was triggered, but failed to build newsletter!");
        return;
    }

    // Send to channels
    for (auto const& channelID : m_pBotConfigDb->getSubscribedChannels(k_newsletterPageOptionScrapeRankings.second))
    {
        message.channel_id = channelID;
        std::string onCompletionID = std::string("scrape-rankings_send-to-channel_").append(std::to_string(channelID.operator uint64_t()));
        m_bot.message_create(message, std::bind(&Bot::onCompletion_, this, std::placeholders::_1, onCompletionID));
    }
}

/**
 * Build and send top plays newsletter to all subscribed channels.
 */
void Bot::topPlaysCallback()
{
    LOG_DEBUG("Executing callback for top plays job");

    // Build message
    dpp::message message;
    LOG_ERROR_THROW(
        buildTopPlaysNewsletter_(k_global, Gamemode(0), k_allMods, message),
        "Callback was triggered, but failed to build newsletter!"
    );

    // Send to channels
    for (auto const& channelID : m_pBotConfigDb->getSubscribedChannels(k_newsletterPageOptionTopPlays.second))
    {
        message.channel_id = channelID;
        std::string onCompletionID = std::string("top-plays_send-to-channel_").append(std::to_string(channelID.operator uint64_t()));
        m_bot.message_create(message, std::bind(&Bot::onCompletion_, this, std::placeholders::_1, onCompletionID));
    }
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

/**
 * Handle dpp log event.
 */
void Bot::onLog_(dpp::log_t const& event) const noexcept
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
void Bot::onReady_(dpp::ready_t const&)
{
    if (dpp::run_once<struct register_bot_commands>())
    {
        LOG_DEBUG("Registering slash commands");

        dpp::slashcommand newsletterSlashCommand(k_cmdNewsletter, k_cmdNewsletterDesc, m_bot.me.id);
        dpp::command_option newsletterOption(dpp::co_string, k_newsletterPageOptionName, k_cmdNewsletterPageOptionDesc, true);
        newsletterOption.add_choice(dpp::command_option_choice(k_newsletterPageOptionScrapeRankings.first, k_newsletterPageOptionScrapeRankings.second));
        newsletterOption.add_choice(dpp::command_option_choice(k_newsletterPageOptionTopPlays.first, k_newsletterPageOptionTopPlays.second));
        newsletterSlashCommand.add_option(newsletterOption);

        dpp::slashcommand subscribeSlashCommand(k_cmdSubscribe, k_cmdSubscribeDesc, m_bot.me.id);
        dpp::command_option subscribeOption(dpp::co_string, k_newsletterPageOptionName, k_cmdSubscribePageOptionDesc, true);
        subscribeOption.add_choice(dpp::command_option_choice(k_newsletterPageOptionScrapeRankings.first, k_newsletterPageOptionScrapeRankings.second));
        subscribeOption.add_choice(dpp::command_option_choice(k_newsletterPageOptionTopPlays.first, k_newsletterPageOptionTopPlays.second));
        subscribeSlashCommand.add_option(subscribeOption);

        dpp::slashcommand unsubscribeSlashCommand(k_cmdUnsubscribe, k_cmdUnsubscribeDesc, m_bot.me.id);
        dpp::command_option unsubscribeOption(dpp::co_string, k_newsletterPageOptionName, k_cmdUnsubscribePageOptionDesc, true);
        unsubscribeOption.add_choice(dpp::command_option_choice(k_newsletterPageOptionScrapeRankings.first, k_newsletterPageOptionScrapeRankings.second));
        unsubscribeOption.add_choice(dpp::command_option_choice(k_newsletterPageOptionTopPlays.first, k_newsletterPageOptionTopPlays.second));
        unsubscribeSlashCommand.add_option(unsubscribeOption);

        std::vector<dpp::slashcommand> commands = {
            dpp::slashcommand(k_cmdHelp, k_cmdHelpDesc, m_bot.me.id),
            dpp::slashcommand(k_cmdPing, k_cmdPingDesc, m_bot.me.id),
            newsletterSlashCommand,
            subscribeSlashCommand,
            unsubscribeSlashCommand
        };
        m_bot.global_bulk_command_create(commands);
    }
}

/**
 * Handle slash command event.
 */
void Bot::onSlashCommand_(dpp::slashcommand_t const& event)
{
    std::string cmdName = event.command.get_command_name();
    LOG_INFO("Received slash command /", cmdName, "; routing");

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
                event.reply(dpp::message("You are using commands too quickly! Please wait a few seconds").set_flags(dpp::m_ephemeral));
                return;
            }
        }
        m_latestCommands[userID] = now;
    }

    // Route command
    auto it = m_kCommandMap.find(cmdName);
    if (it != m_kCommandMap.end())
    {
        it->second(this, event);
    }
    else
    {
        LOG_WARN("Got unknown slash command /", cmdName, "; attempting to remove it");
        deleteGlobalCommand_(cmdName);
        event.reply(dpp::message("Command does not exist! If it is still listed in a few minutes, this may be a bug.").set_flags(dpp::m_ephemeral));
    }
}

/**
 * Handle button press event.
 */
void Bot::onButtonClick_(dpp::button_click_t const& event)
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
                event.reply(dpp::message("You are pressing buttons too quickly! Please wait a few seconds").set_flags(dpp::m_ephemeral));
                return;
            }
        }
        m_latestInteractions[userID] = now;
    }

    // Route buttonpress
    // scrapeRankings buttons
    if ((buttonID == k_scrapeRankingsFirstRangeButtonID) ||
        (buttonID == k_scrapeRankingsSecondRangeButtonID) ||
        (buttonID == k_scrapeRankingsThirdRangeButtonID))
    {
        dpp::message message = event.command.msg;
        EmbedMetadata embedMetadata;
        if (!parseMetadata(message, embedMetadata))
        {
            LOG_ERROR("Failed to parse metadata for current embed!");
            event.reply(dpp::message("Failed to parse metadata! This is a bug.").set_flags(dpp::m_ephemeral));
            return;
        }

        if (!buildScrapeRankingsNewsletter_(embedMetadata.getCountryCode(), buttonIDToRankRange(buttonID), embedMetadata.getGamemode(), message))
        {
            LOG_ERROR(buttonID, " was pressed, but failed to build newsletter!");
            event.reply(dpp::message("Failed to build new embed! This is a bug.").set_flags(dpp::m_ephemeral));
            return;
        }

        m_bot.message_edit(message, std::bind(&Bot::onCompletionReply_, this, std::placeholders::_1, buttonID, event));
    }
    else if (buttonID == k_scrapeRankingsFilterCountryButtonID)
    {
        dpp::interaction_modal_response modal = m_pEmbedGenerator->scrapeRankingsFilterCountryModal();
        event.dialog(modal, std::bind(&Bot::onCompletion_, this, std::placeholders::_1, buttonID));
    }
    else if (buttonID == k_scrapeRankingsSelectModeButtonID)
    {
        dpp::interaction_modal_response modal = m_pEmbedGenerator->scrapeRankingsFilterModeModal();
        event.dialog(modal, std::bind(&Bot::onCompletion_, this, std::placeholders::_1, buttonID));
    }
    else if (buttonID == k_scrapeRankingsResetAllButtonID)
    {
        dpp::message message = event.command.msg;

        if (!buildScrapeRankingsNewsletter_(k_global, RankRange(0), Gamemode(0), message))
        {
            LOG_ERROR(k_scrapeRankingsResetAllButtonID, " was pressed, but failed to build newsletter!");
            event.reply(dpp::message("Failed to build new embed! This is a bug.").set_flags(dpp::m_ephemeral));
            return;
        }

        m_bot.message_edit(message, std::bind(&Bot::onCompletionReply_, this, std::placeholders::_1, buttonID, event));
    }
    // getTopPlays buttons
    else if (buttonID == k_topPlaysFilterCountryButtonID)
    {
        dpp::interaction_modal_response modal = m_pEmbedGenerator->topPlaysFilterCountryModal();
        event.dialog(modal, std::bind(&Bot::onCompletion_, this, std::placeholders::_1, buttonID));
    }
    else if (buttonID == k_topPlaysFilterModsButtonID)
    {
        dpp::interaction_modal_response modal = m_pEmbedGenerator->topPlaysFilterModsModal();
        event.dialog(modal, std::bind(&Bot::onCompletion_, this, std::placeholders::_1, buttonID));
    }
    else if (buttonID == k_topPlaysSelectModeButtonID)
    {
        dpp::interaction_modal_response modal = m_pEmbedGenerator->topPlaysFilterModeModal();
        event.dialog(modal, std::bind(&Bot::onCompletion_, this, std::placeholders::_1, buttonID));
    }
    else if (buttonID == k_topPlaysResetAllButtonID)
    {
        dpp::message message = event.command.msg;

        if (!buildTopPlaysNewsletter_(k_global, Gamemode(0), k_allMods, message))
        {
            LOG_ERROR(k_topPlaysResetAllButtonID, " was pressed, but failed to build newsletter!");
            event.reply(dpp::message("Failed to build new embed! This is a bug.").set_flags(dpp::m_ephemeral));
            return;
        }

        m_bot.message_edit(message, std::bind(&Bot::onCompletionReply_, this, std::placeholders::_1, buttonID, event));
    }
}

/**
 * Handle form submission event.
 */
void Bot::onFormSubmit_(dpp::form_submit_t const& event)
{
    auto formID = event.custom_id;
    LOG_DEBUG("Processing form submission for modal ", formID);

    // Route form submission
    // scrapeRankings forms
    if (formID == k_scrapeRankingsFilterCountryModalID)
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
        if (!parseMetadata(message, embedMetadata))
        {
            LOG_ERROR("Failed to parse metadata for current embed!");
            event.reply(dpp::message("Failed to parse metadata! This is a bug.").set_flags(dpp::m_ephemeral));
            return;
        }

        if (!buildScrapeRankingsNewsletter_(countryCode, embedMetadata.getRankRange(), embedMetadata.getGamemode(), message))
        {
            LOG_ERROR(k_scrapeRankingsFilterCountryModalID, " was submitted with countryCode=", countryInput, ", but failed to build newsletter!");
            event.reply(dpp::message("Failed to build new embed! This is a bug.").set_flags(dpp::m_ephemeral));
            return;
        }

        m_bot.message_edit(message, std::bind(&Bot::onCompletionReply_, this, std::placeholders::_1, formID, event));
    }
    else if (formID == k_scrapeRankingsSelectModeModalID)
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
        if (!parseMetadata(message, embedMetadata))
        {
            LOG_ERROR("Failed to parse metadata for current embed!");
            event.reply(dpp::message("Failed to parse metadata! This is a bug.").set_flags(dpp::m_ephemeral));
            return;
        }

        if (!buildScrapeRankingsNewsletter_(embedMetadata.getCountryCode(), embedMetadata.getRankRange(), mode, message))
        {
            LOG_ERROR(k_scrapeRankingsSelectModeModalID, " was submitted with mode=", modeInput, ", but failed to build newsletter!");
            event.reply(dpp::message("Failed to build new embed! This is a bug.").set_flags(dpp::m_ephemeral));
            return;
        }

        m_bot.message_edit(message, std::bind(&Bot::onCompletionReply_, this, std::placeholders::_1, formID, event));
    }
    // getTopPlays forms
    else if (formID == k_topPlaysFilterCountryModalID)
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
        if (!parseMetadata(message, embedMetadata))
        {
            LOG_ERROR("Failed to parse metadata for current embed!");
            event.reply(dpp::message("Failed to parse metadata! This is a bug.").set_flags(dpp::m_ephemeral));
            return;
        }

        if (!buildTopPlaysNewsletter_(countryCode, embedMetadata.getGamemode(), embedMetadata.getMods().toString(), message))
        {
            LOG_ERROR(k_topPlaysFilterCountryModalID, " was submitted with countryCode=", countryInput, ", but failed to build newsletter!");
            event.reply(dpp::message("Failed to build new embed! This is a bug.").set_flags(dpp::m_ephemeral));
            return;
        }

        m_bot.message_edit(message, std::bind(&Bot::onCompletionReply_, this, std::placeholders::_1, formID, event));
    }
    else if (formID == k_topPlaysFilterModsModalID)
    {
        std::string modsInput = std::get<std::string>(event.components[0].components[0].value);
        LOG_DEBUG("Processing request to filter mods by ", modsInput);

        std::string modsInputNormalized = modsInput;
        std::transform(modsInputNormalized.begin(), modsInputNormalized.end(), modsInputNormalized.begin(), ::toupper);
        if (modsInputNormalized == "NM")
        {
            modsInputNormalized = "";
        }
        if (modsInputNormalized.length() % 2 != 0)
        {
            modsInputNormalized.pop_back();
        }

        OsuMods mods(modsInputNormalized);

        dpp::message message = event.command.msg;

        EmbedMetadata embedMetadata;
        if (!parseMetadata(message, embedMetadata))
        {
            LOG_ERROR("Failed to parse metadata for current embed!");
            event.reply(dpp::message("Failed to build new embed! This is a bug.").set_flags(dpp::m_ephemeral));
            return;
        }

        if(!buildTopPlaysNewsletter_(embedMetadata.getCountryCode(), embedMetadata.getGamemode(), mods.toString(), message))
        {
            LOG_ERROR(k_topPlaysFilterModsModalID, " was submitted with mods=", mods.toString(), ", but failed to build newsletter!");
            event.reply(dpp::message("Failed to build new embed! This is a bug.").set_flags(dpp::m_ephemeral));
            return;
        }

        m_bot.message_edit(message, std::bind(&Bot::onCompletionReply_, this, std::placeholders::_1, formID, event));
    }
    else if (formID == k_topPlaysSelectModeModalID)
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
        if (!parseMetadata(message, embedMetadata))
        {
            LOG_ERROR("Failed to parse metadata for current embed!");
            event.reply(dpp::message("Failed to parse metadata! This is a bug.").set_flags(dpp::m_ephemeral));
            return;
        }

        if (!buildTopPlaysNewsletter_(embedMetadata.getCountryCode(), mode, embedMetadata.getMods().toString(), message))
        {
            LOG_ERROR(k_topPlaysSelectModeModalID, " was submitted with mode=", modeInput, ", but failed to build newsletter!");
            event.reply(dpp::message("Failed to build new embed! This is a bug.").set_flags(dpp::m_ephemeral));
            return;
        }

        m_bot.message_edit(message, std::bind(&Bot::onCompletionReply_, this, std::placeholders::_1, formID, event));
    }
}

/**
 * Generic completion callback handler.
 */
void Bot::onCompletion_(dpp::confirmation_callback_t const& callback, std::string const& customID) const
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
void Bot::onCompletionReply_(dpp::confirmation_callback_t const& callback, std::string const& customID, dpp::interaction_create_t const& event) const
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

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

/**
 * Run the help command.
 */
void Bot::cmdHelp_(dpp::slashcommand_t const& event) const
{
    dpp::message message;
    message.add_embed(m_helpEmbed);
    event.reply(message);
}

/**
 * Run the ping command.
 */
void Bot::cmdPing_(dpp::slashcommand_t const& event) const
{
    event.reply("Pong!");
}

/**
 * Run the newsletter command.
 */
void Bot::cmdNewsletter_(dpp::slashcommand_t const& event)
{
    dpp::message message;

    std::string newsletterPage = std::get<std::string>(event.get_parameter(k_newsletterPageOptionName));
    if (newsletterPage == k_newsletterPageOptionScrapeRankings.second)
    {
        if (!buildScrapeRankingsNewsletter_(k_global, RankRange(0), Gamemode(0), message))
        {
            LOG_WARN("Got command, but failed to build newsletter! newsletterPage=", newsletterPage);
            event.reply(dpp::message("Couldn't find newsletter! If you still see this message tomorrow, it might be a bug.").set_flags(dpp::m_ephemeral));
            return;
        }
    }
    else if (newsletterPage == k_newsletterPageOptionTopPlays.second)
    {
        if (!buildTopPlaysNewsletter_(k_global, Gamemode(0), k_allMods, message))
        {
            LOG_WARN("Got command, but failed to build newsletter! newsletterPage=", newsletterPage);
            event.reply(dpp::message("Couldn't find newsletter! If you still see this message tomorrow, it might be a bug.").set_flags(dpp::m_ephemeral));
            return;
        }
    }
    else
    {
        LOG_ERROR("Got command with unrecognized newsletterPage=", newsletterPage);
        event.reply(dpp::message("Unrecognized parameter! This is a bug."));
        return;
    }

    event.reply(message);
}

/**
 * Run the subscribe command.
 * Adds this channel to the list of channels to send daily messages to.
 */
void Bot::cmdSubscribe_(dpp::slashcommand_t const& event)
{
    dpp::permission permissions = event.command.app_permissions;
    if (!permissions.has(dpp::p_send_messages))
    {
        event.reply(dpp::message("Cannot subscribe! Bot lacks permissions to send messages in this channel").set_flags(dpp::m_ephemeral));
        return;
    }

    dpp::snowflake channelID = event.command.channel_id;
    std::string newsletterPage = std::get<std::string>(event.get_parameter(k_newsletterPageOptionName));

    if (!m_pBotConfigDb->isChannelSubscribed(channelID, newsletterPage))
    {
        m_pBotConfigDb->addSubscription(channelID, newsletterPage);
        event.reply("Daily messages have been enabled for page '" + newsletterPage + "' in this channel.");
    }
    else
    {
        event.reply("Daily messages for page '" + newsletterPage + "' are already enabled in this channel!");
    }
}

/**
 * Run the unsubscribe command.
 * Removes this channel to the list of channels to send daily messages to.
 */
void Bot::cmdUnsubscribe_(dpp::slashcommand_t const& event)
{
    dpp::snowflake channelID = event.command.channel_id;
    std::string newsletterPage = std::get<std::string>(event.get_parameter(k_newsletterPageOptionName));

    if (m_pBotConfigDb->isChannelSubscribed(channelID, newsletterPage))
    {
        m_pBotConfigDb->removeSubscription(channelID, newsletterPage);
        event.reply("Daily messages have been disabled for page '" + newsletterPage + "' in this channel.");
    }
    else
    {
        event.reply("Daily messages for page '" + newsletterPage + "' are already disabled in this channel!");
    }
}

//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\////\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

/**
 * Build and store unchanging components to memory.
 */
void Bot::buildStaticComponents_()
{
    LOG_DEBUG("Building static components");

    m_helpEmbed = m_pEmbedGenerator->helpEmbed();
    m_scrapeRankingsActionRow1 = m_pEmbedGenerator->scrapeRankingsActionRow1();
    m_scrapeRankingsActionRow2 = m_pEmbedGenerator->scrapeRankingsActionRow2();
    m_topPlaysActionRow1 = m_pEmbedGenerator->topPlaysActionRow1();
    /* FUTURE: The filter-by-country modal is static, but DPP doesn't seem to handle interaction_modal_response like other
    components/embeds/etc. If we store it here it gets double-freed when the program exits and I'm too lazy to figure out why.
    Check if this is stil the case when library is updated. */
}

/**
 * Delete global command by name.
 */
void Bot::deleteGlobalCommand_(std::string const& cmdName)
{
    LOG_DEBUG("Removing stale command ", cmdName);

    m_bot.global_commands_get(
        [this, cmdName](dpp::confirmation_callback_t const& getCallback)
        {
            if (getCallback.is_error())
            {
                auto error = getCallback.get_error();
                LOG_WARN("Failed to get global commands; ", error.message);
            }

            auto slashCommands = getCallback.get<dpp::slashcommand_map>();
            for (auto const& cmd : slashCommands)
            {
                if (cmd.second.name == cmdName)
                {
                    m_bot.global_command_delete(cmd.first,
                        [cmd](dpp::confirmation_callback_t const& deleteCallback)
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
 * Build scrapeRankings newsletter. Returns true if data was valid, false otherwise.
 */
[[nodiscard]] bool Bot::buildScrapeRankingsNewsletter_(std::string const& countryCode, RankRange const& rankRange, Gamemode const& mode, dpp::message& message /* out */)
{
    LOG_DEBUG("Building scrapeRankings newsletter for countryCode=", countryCode, ", rankRange=", rankRange.toString(), ", mode=", mode.toString());

    auto lastWriteTime = m_pRankingsDb->lastWriteTime();
    auto now = std::filesystem::file_time_type::clock::now();
    std::chrono::hours ageHours = std::chrono::duration_cast<std::chrono::hours>(now - lastWriteTime);
    if (ageHours > k_maxValidScrapeRankingsHour)
    {
        LOG_WARN("Data is invalid - ageHours=", ageHours.count(), " > ", k_maxValidScrapeRankingsHour.count());
        return false;
    }

    if (m_pRankingsDb->hasEmptyTable())
    {
        LOG_WARN("Data is invalid - rankings database has an empty table!");
        return false;
    }

    LOG_DEBUG("Data is valid - building rankings newsletter");

    auto range = rankRange.toRange();
    std::vector<RankImprovement> rangeTop = m_pRankingsDb->getTopRankImprovements(countryCode, range.first, range.second, k_numDisplayUsers, mode);
    std::vector<RankImprovement> rangeBottom = m_pRankingsDb->getBottomRankImprovements(countryCode, range.first, range.second, k_numDisplayUsers, mode);
    dpp::embed newsletterEmbed = m_pEmbedGenerator->scrapeRankingsEmbed(countryCode, rankRange, mode, rangeTop, rangeBottom);

    message.embeds.clear();
    message.components.clear();

    message.add_embed(newsletterEmbed);
    message.add_component(m_scrapeRankingsActionRow1);
    message.add_component(m_scrapeRankingsActionRow2);

    return true;
}

/**
 * Build scrapeRankings newsletter. Returns true if data was valid, false otherwise.
 */
[[nodiscard]] bool Bot::buildTopPlaysNewsletter_(std::string const& countryCode, Gamemode const& mode, std::string const& mods, dpp::message& message /* out */)
{
    LOG_DEBUG("Building getTopPlays newsletter for countryCode=", countryCode, ", mode=", mode.toString(), ", mods=", mods);

    auto lastWriteTime = m_pTopPlaysDb->lastWriteTime();
    auto now = std::filesystem::file_time_type::clock::now();
    std::chrono::hours ageHours = std::chrono::duration_cast<std::chrono::hours>(now - lastWriteTime);
    if (ageHours > k_maxValidTopPlaysHour)
    {
        LOG_WARN("Data is invalid - ageHours=", ageHours.count(), " > ", k_maxValidTopPlaysHour.count());
        return false;
    }

    if (m_pTopPlaysDb->hasEmptyTable())
    {
        LOG_WARN("Data is invalid - top plays database has an empty table!");
        return false;
    }

    LOG_DEBUG("Data is valid - building top plays newsletter");

    std::vector<TopPlay> topPlays = m_pTopPlaysDb->getTopPlays(countryCode, k_numDisplayTopPlays, mode, mods);

    dpp::embed newsletterEmbed = m_pEmbedGenerator->topPlaysEmbed(topPlays, mode, countryCode, mods);

    message.embeds.clear();
    message.components.clear();

    message.add_embed(newsletterEmbed);
    message.add_component(m_topPlaysActionRow1);

    return true;
}
