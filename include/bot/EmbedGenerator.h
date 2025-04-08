#ifndef __EMBED_GENERATOR_H__
#define __EMBED_GENERATOR_H__

#include "Util.h"

#include <dpp/dpp.h>

#include <string>
#include <vector>
#include <sstream>
#include <string>

const std::string k_twemojiClockPrefix = "https://cdn.jsdelivr.net/gh/twitter/twemoji@14.0.2/assets/72x72/1f55";
const std::string k_twemojiClockSuffix = ".png";
const std::string k_iconImgUrl = "https://spreadnuts.s-ul.eu/HDlrjbqe.png";
const std::string k_defaultPfpUrl = "https://osu.ppy.sh/images/layout/avatar-guest.png";

constexpr uint32_t k_helpColor = dpp::colors::rust;
constexpr uint32_t k_firstRangeColor = dpp::colors::gold;
constexpr uint32_t k_secondRangeColor = dpp::colors::silver;
constexpr uint32_t k_thirdRangeColor = dpp::colors::bronze;
constexpr uint32_t k_topPlaysColor = dpp::colors::summer_sky;

/**
 * Helper class for building DPP components.
 */
class EmbedGenerator
{
public:
    EmbedGenerator() = default;
    ~EmbedGenerator() = default;

    // FUTURE: marking things const instead of static because we might some of these fns to be polymorphic later
    [[nodiscard]] dpp::embed helpEmbed() const;
    [[nodiscard]] dpp::embed scrapeRankingsEmbed(
        std::string const& countryCode,
        RankRange const& rankRange,
        Gamemode const& mode,
        std::vector<RankImprovement> const& top,
        std::vector<RankImprovement> const& bottom) const;
    [[nodiscard]] dpp::embed topPlaysEmbed(
        std::vector<TopPlay> const& topPlays,
        Gamemode const& mode,
        std::string const& countryCode) const;

    [[nodiscard]] dpp::component scrapeRankingsActionRow1() const;
    [[nodiscard]] dpp::component scrapeRankingsActionRow2() const;
    [[nodiscard]] dpp::component topPlaysActionRow1() const;
    [[nodiscard]] dpp::interaction_modal_response scrapeRankingsFilterCountryModal() const;
    [[nodiscard]] dpp::interaction_modal_response topPlaysFilterCountryModal() const;
    [[nodiscard]] dpp::interaction_modal_response scrapeRankingsFilterModeModal() const;
    [[nodiscard]] dpp::interaction_modal_response topPlaysFilterModeModal() const;


private:
    void addPlayersToScrapeRankingsDescription_(
        std::stringstream& description /* out */,
        std::vector<RankImprovement> const& players,
        bool const& bIsTop,
        Gamemode const& mode) const noexcept;
    void addPlayersToTopPlaysDescription_(
        std::stringstream& description /* out */,
        std::vector<TopPlay> const& topPlays,
        Gamemode const& mode) const noexcept;
};

#endif /* __EMBED_GENERATOR_H__ */
