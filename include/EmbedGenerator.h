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

const uint32_t k_helpColor = dpp::colors::rust;
const uint32_t k_firstRangeColor = dpp::colors::gold;
const uint32_t k_secondRangeColor = dpp::colors::silver;
const uint32_t k_thirdRangeColor = dpp::colors::bronze;
const uint32_t k_topPlaysColor = dpp::colors::summer_sky;

/**
 * Helper class for building DPP components.
 */
class EmbedGenerator
{
public:
    EmbedGenerator() = default;
    ~EmbedGenerator() = default;

    dpp::embed helpEmbed();
    dpp::embed scrapeRankingsEmbed(const std::string countryCode, const RankRange rankRange, const Gamemode mode, std::vector<RankImprovement> top, std::vector<RankImprovement> bottom);

    dpp::component scrapeRankingsActionRow1();
    dpp::component scrapeRankingsActionRow2();
    dpp::component topPlaysActionRow1();
    dpp::interaction_modal_response scrapeRankingsFilterCountryModal();
    dpp::interaction_modal_response topPlaysFilterCountryModal();
    dpp::interaction_modal_response scrapeRankingsFilterModeModal();
    dpp::interaction_modal_response topPlaysFilterModeModal();
    bool parseScrapeRankingsMetadata(const dpp::message message, EmbedMetadata& metadata);
    bool parseTopPlaysMetadata(dpp::message const& message, EmbedMetadata& metadata /* out */);

    dpp::embed topPlaysEmbed(std::vector<TopPlay> const& topPlays, Gamemode const& mode, std::string const& countryCode);

private:
    std::string scrapeRankingsMetadataTag(const std::string countryCode, const RankRange rankRange, const Gamemode mode);
    std::string topPlaysMetadataTag(std::string const& countryCode, Gamemode const& mode);
    void addPlayersToScrapeRankingsDescription(std::stringstream& description, std::vector<RankImprovement> players, const bool bIsTop, const Gamemode mode);
    void addPlayersToTopPlaysDescription(std::stringstream& description, std::vector<TopPlay> const& topPlays, Gamemode const& mode);
};

#endif /* __EMBED_GENERATOR_H__ */
