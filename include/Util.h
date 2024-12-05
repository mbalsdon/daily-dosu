#ifndef __UTIL_H__
#define __UTIL_H__

#include <string>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <chrono>
#include <array>
#include <string_view>
#include <utility>
#include <algorithm>

/* - - - - - - Constants - - - - - - - */

static constexpr std::size_t k_getRankingIDMaxNumIDs = 50;
static constexpr std::size_t k_getRankingIDMaxPage = 200;

static constexpr std::size_t k_numDisplayUsersTop = 15;
static constexpr std::size_t k_numDisplayUsersBottom = 5;
const std::size_t k_numDisplayUsers = std::max(k_numDisplayUsersTop, k_numDisplayUsersBottom);

const std::chrono::hours k_minValidScrapeRankingsHour = std::chrono::hours(22);
const std::chrono::hours k_maxValidScrapeRankingsHour = std::chrono::hours(26);

const std::filesystem::path k_rootDir = std::filesystem::path(__FILE__).parent_path().parent_path();
const std::filesystem::path k_dosuConfigFilePath = k_rootDir / "dosu_config.json";
const std::filesystem::path k_dataDir = k_rootDir / "data";

const std::string k_cmdHelp = "help";
const std::string k_cmdPing = "ping";
const std::string k_cmdNewsletter = "newsletter";
const std::string k_cmdSubscribe = "subscribe";
const std::string k_cmdUnsubscribe = "unsubscribe";

const std::string k_cmdHelpDesc = "List the bot's commands.";
const std::string k_cmdPingDesc = "Check if the bot is alive.";
const std::string k_cmdNewsletterDesc = "Display the daily newsletter.";
const std::string k_cmdSubscribeDesc = "Enable daily newsletters for this channel.";
const std::string k_cmdUnsubscribeDesc = "Disable daily newsletters for this channel.";

const std::string k_firstRangeButtonID = "first_range_button";
const std::string k_secondRangeButtonID = "second_range_button";
const std::string k_thirdRangeButtonID = "third_range_button";
const std::string k_filterCountryButtonID = "filter_country_button";
const std::string k_clearFiltersButtonID = "clear_filters_button";
const std::string k_filterCountryModalID = "filter_country_modal";
const std::string k_filterCountryTextInputID = "filter_country_text_input";

const std::string k_global = "GLOBAL";

/* - - - - - - - - Types - - - - - - - */

typedef std::string OAuthToken;
typedef std::size_t Page;

typedef int64_t UserID;
typedef std::string Username;
typedef std::string CountryCode;
typedef std::string ProfilePicture;
typedef int64_t PerformancePoints;
typedef double Accuracy;
typedef int64_t HoursPlayed;
typedef int64_t Rank;

struct RankingsUser
{
    UserID userID = -1;
    Username username = "";
    CountryCode countryCode = "";
    ProfilePicture pfpLink = "";
    PerformancePoints performancePoints = -1;
    Accuracy accuracy = -1.;
    HoursPlayed hoursPlayed = -1;
    Rank yesterdayRank = -1;
    Rank currentRank = -1;

    bool isValid() const
    {
        return (
            (userID > 0) &&
            !username.empty() &&
            !countryCode.empty() &&
            !pfpLink.empty() &&
            (performancePoints >= 0) &&
            (accuracy >= 0.) &&
            (hoursPlayed >= 0) &&
            (currentRank >= 1)
        );
    }
};

struct RankImprovement
{
    RankingsUser user;
    double relativeImprovement;
};

struct ISO3166_Alpha2
{
    static constexpr std::array<std::string_view, 249> codes = { // weeeeee
        "AF", "AX", "AL", "DZ", "AS", "AD", "AO", "AI", "AQ", "AG", "AR", "AM", "AW", "AU", "AT", "AZ", "BS", "BH", "BD", "BB", "BY", "BE", "BZ", "BJ", "BM", "BT", "BO", "BQ", "BA", "BW", "BV", "BR", "IO", "BN", "BG", "BF", "BI", "KH", "CM", "CA", "CV", "KY", "CF", "TD", "CL", "CN", "CX", "CC", "CO", "KM", "CG", "CD", "CK", "CR", "CI", "HR", "CU", "CW", "CY", "CZ", "DK", "DJ", "DM", "DO", "EC", "EG", "SV", "GQ", "ER", "EE", "ET", "FK", "FO", "FJ", "FI", "FR", "GF", "PF", "TF", "GA", "GM", "GE", "DE", "GH", "GI", "GR", "GL", "GD", "GP", "GU", "GT", "GG", "GN", "GW", "GY", "HT", "HM", "VA", "HN", "HK", "HU", "IS", "IN", "ID", "IR", "IQ", "IE", "IM", "IL", "IT", "JM", "JP", "JE", "JO", "KZ", "KE", "KI", "KP", "KR", "KW", "KG", "LA", "LV", "LB", "LS", "LR", "LY", "LI", "LT", "LU", "MO", "MK", "MG", "MW", "MY", "MV", "ML", "MT", "MH", "MQ", "MR", "MU", "YT", "MX", "FM", "MD", "MC", "MN", "ME", "MS", "MA", "MZ", "MM", "NA", "NR", "NP", "NL", "NC", "NZ", "NI", "NE", "NG", "NU", "NF", "MP", "NO", "OM", "PK", "PW", "PS", "PA", "PG", "PY", "PE", "PH", "PN", "PL", "PT", "PR", "QA", "RE", "RO", "RU", "RW", "BL", "SH", "KN", "LC", "MF", "PM", "VC", "WS", "SM", "ST", "SA", "SN", "RS", "SC", "SL", "SG", "SX", "SK", "SI", "SB", "SO", "ZA", "GS", "SS", "ES", "LK", "SD", "SR", "SJ", "SZ", "SE", "CH", "SY", "TW", "TJ", "TZ", "TH", "TL", "TG", "TK", "TO", "TT", "TN", "TR", "TM", "TC", "TV", "UG", "UA", "AE", "GB", "US", "UM", "UY", "UZ", "VU", "VE", "VN", "VG", "VI", "WF", "EH", "YE", "ZM", "ZW"
    };
};



class RankRange
{
public:
    enum Value
    {
        First,
        Second,
        Third
    };

    RankRange() : m_value(Value::First) {}
    RankRange(Value v) : m_value(v) {}
    RankRange(int i) : m_value(static_cast<Value>(i)) {}
    RankRange(std::string s) : m_value(static_cast<Value>(std::stoi(s))) {}

    bool operator==(const RankRange& other) const { return m_value == other.m_value; }
    bool operator==(Value v) const { return m_value == v; }

    std::string toString() const
    {
        switch (m_value)
        {
        case Value::First: return "First";
        case Value::Second: return "Second";
        case Value::Third: return "Third";
        default: return "Unknown";
        }
    }

    int toInt() const
    {
        return static_cast<int>(m_value);
    }

    std::pair<int64_t, int64_t> toRange() const
    {
        switch (m_value)
        {
        case Value::First: return std::make_pair(1, 100);
        case Value::Second: return std::make_pair(101, 1000);
        case Value::Third: return std::make_pair(1001, 10000);
        default: return std::make_pair(0, 10000);
        }
    }

    Value m_value;
};

class EmbedMetadata
{
public:
    EmbedMetadata()
    : m_countryCode(k_global)
    , m_rankRange(RankRange())
    {}

    EmbedMetadata(std::string countryCode, RankRange rankRange)
    : m_countryCode(countryCode)
    , m_rankRange(rankRange)
    {}

    std::string m_countryCode;
    RankRange m_rankRange;
};

#endif /* __UTIL_H__ */
