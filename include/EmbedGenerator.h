#ifndef __EMBED_GENERATOR_H__
#define __EMBED_GENERATOR_H__

#include "Util.h"

#include <dpp/dpp.h>

#include <string>
#include <vector>
#include <sstream>

const std::string k_twemojiClockPrefix = "https://cdn.jsdelivr.net/gh/twitter/twemoji@14.0.2/assets/72x72/1f55";
const std::string k_twemojiClockSuffix = ".png";
const std::string k_iconImgUrl = "https://spreadnuts.s-ul.eu/HDlrjbqe.png";
const std::string k_defaultPfpUrl = "https://osu.ppy.sh/images/layout/avatar-guest.png";

const uint32_t k_helpColor = dpp::colors::rust;
const uint32_t k_firstRangeColor = dpp::colors::gold;
const uint32_t k_secondRangeColor = dpp::colors::silver;
const uint32_t k_thirdRangeColor = dpp::colors::bronze;

/**
 * Helper class for building DPP components.
 */
class EmbedGenerator
{
public:
    EmbedGenerator() = default;
    ~EmbedGenerator() = default;

    dpp::embed helpEmbed();
    dpp::embed scrapeRankingsEmbed(RankRange rankRange, std::vector<RankImprovement> top, std::vector<RankImprovement> bottom);

    dpp::component scrapeRankingsActionRow();

private:
    void addPlayersToScrapeRankingsDescription(std::stringstream& description, std::vector<RankImprovement> players, bool bIsTop);
    ProfilePicture scrapeRankingsThumbnail(std::vector<RankImprovement> top, std::vector<RankImprovement> bottom);
};

#endif /* __EMBED_GENERATOR_H__ */
