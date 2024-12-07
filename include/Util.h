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
#include <unordered_map>

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
const std::string k_filterCountryModalID = "filter_country_modal";
const std::string k_filterCountryTextInputID = "filter_country_text_input";
const std::string k_selectModeButtonID = "select_mode_button";
const std::string k_selectModeModalID = "select_mode_modal";
const std::string k_selectModeTextInputID = "select_mode_text_input";
const std::string k_resetAllButtonID = "reset_all_button";

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

class ISO3166Alpha2Converter
{
public:
    /**
     * Assumes input is all uppercase.
     */
    [[nodiscard]] static std::string_view toAlpha2(std::string_view const input) noexcept
    {
        if (input.length() == 2)
        {
            auto it = m_A2ToA2.find(input);
            return it != m_A2ToA2.end() ? it->second : "";
        }
        else if (input.length() == 3)
        {
            auto it = m_A3ToA2.find(input);
            return it != m_A3ToA2.end() ? it->second : "";
        }
        else if (input == k_global) // Special case
        {
            return k_global;
        }
        else
        {
            auto it = m_nameToA2.find(input);
            return it != m_nameToA2.end() ? it->second : "";
        }
    }

private: // weeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee
    static inline const std::unordered_map<std::string_view, std::string_view> m_A2ToA2 = {
        { "AF", "AF" }, { "AX", "AX" }, { "AL", "AL" }, { "DZ", "DZ" }, { "AS", "AS" }, { "AD", "AD" }, { "AO", "AO" }, { "AI", "AI" }, { "AQ", "AQ" }, { "AG", "AG" }, { "AR", "AR" }, { "AM", "AM" }, { "AW", "AW" }, { "AU", "AU" }, { "AT", "AT" }, { "AZ", "AZ" }, { "BS", "BS" }, { "BH", "BH" }, { "BD", "BD" }, { "BB", "BB" }, { "BY", "BY" }, { "BE", "BE" }, { "BZ", "BZ" }, { "BJ", "BJ" }, { "BM", "BM" }, { "BT", "BT" }, { "BO", "BO" }, { "BQ", "BQ" }, { "BA", "BA" }, { "BW", "BW" }, { "BV", "BV" }, { "BR", "BR" }, { "IO", "IO" }, { "BN", "BN" }, { "BG", "BG" }, { "BF", "BF" }, { "BI", "BI" }, { "CV", "CV" }, { "KH", "KH" }, { "CM", "CM" }, { "CA", "CA" }, { "KY", "KY" }, { "CF", "CF" }, { "TD", "TD" }, { "CL", "CL" }, { "CN", "CN" }, { "CX", "CX" }, { "CC", "CC" }, { "CO", "CO" }, { "KM", "KM" }, { "CG", "CG" }, { "CD", "CD" }, { "CK", "CK" }, { "CR", "CR" }, { "CI", "CI" }, { "HR", "HR" }, { "CU", "CU" }, { "CW", "CW" }, { "CY", "CY" }, { "CZ", "CZ" }, { "DK", "DK" }, { "DJ", "DJ" }, { "DM", "DM" }, { "DO", "DO" }, { "EC", "EC" }, { "EG", "EG" }, { "SV", "SV" }, { "GQ", "GQ" }, { "ER", "ER" }, { "EE", "EE" }, { "SZ", "SZ" }, { "ET", "ET" }, { "FK", "FK" }, { "FO", "FO" }, { "FJ", "FJ" }, { "FI", "FI" }, { "FR", "FR" }, { "GF", "GF" }, { "PF", "PF" }, { "TF", "TF" }, { "GA", "GA" }, { "GM", "GM" }, { "GE", "GE" }, { "DE", "DE" }, { "GH", "GH" }, { "GI", "GI" }, { "GR", "GR" }, { "GL", "GL" }, { "GD", "GD" }, { "GP", "GP" }, { "GU", "GU" }, { "GT", "GT" }, { "GG", "GG" }, { "GN", "GN" }, { "GW", "GW" }, { "GY", "GY" }, { "HT", "HT" }, { "HM", "HM" }, { "VA", "VA" }, { "HN", "HN" }, { "HK", "HK" }, { "HU", "HU" }, { "IS", "IS" }, { "IN", "IN" }, { "ID", "ID" }, { "IR", "IR" }, { "IQ", "IQ" }, { "IE", "IE" }, { "IM", "IM" }, { "IL", "IL" }, { "IT", "IT" }, { "JM", "JM" }, { "JP", "JP" }, { "JE", "JE" }, { "JO", "JO" }, { "KZ", "KZ" }, { "KE", "KE" }, { "KI", "KI" }, { "KP", "KP" }, { "KR", "KR" }, { "KW", "KW" }, { "KG", "KG" }, { "LA", "LA" }, { "LV", "LV" }, { "LB", "LB" }, { "LS", "LS" }, { "LR", "LR" }, { "LY", "LY" }, { "LI", "LI" }, { "LT", "LT" }, { "LU", "LU" }, { "MO", "MO" }, { "MG", "MG" }, { "MW", "MW" }, { "MY", "MY" }, { "MV", "MV" }, { "ML", "ML" }, { "MT", "MT" }, { "MH", "MH" }, { "MQ", "MQ" }, { "MR", "MR" }, { "MU", "MU" }, { "YT", "YT" }, { "MX", "MX" }, { "FM", "FM" }, { "MD", "MD" }, { "MC", "MC" }, { "MN", "MN" }, { "ME", "ME" }, { "MS", "MS" }, { "MA", "MA" }, { "MZ", "MZ" }, { "MM", "MM" }, { "NA", "NA" }, { "NR", "NR" }, { "NP", "NP" }, { "NL", "NL" }, { "NC", "NC" }, { "NZ", "NZ" }, { "NI", "NI" }, { "NE", "NE" }, { "NG", "NG" }, { "NU", "NU" }, { "NF", "NF" }, { "MK", "MK" }, { "MP", "MP" }, { "NO", "NO" }, { "OM", "OM" }, { "PK", "PK" }, { "PW", "PW" }, { "PS", "PS" }, { "PA", "PA" }, { "PG", "PG" }, { "PY", "PY" }, { "PE", "PE" }, { "PH", "PH" }, { "PN", "PN" }, { "PL", "PL" }, { "PT", "PT" }, { "PR", "PR" }, { "QA", "QA" }, { "RE", "RE" }, { "RO", "RO" }, { "RU", "RU" }, { "RW", "RW" }, { "BL", "BL" }, { "SH", "SH" }, { "KN", "KN" }, { "LC", "LC" }, { "MF", "MF" }, { "PM", "PM" }, { "VC", "VC" }, { "WS", "WS" }, { "SM", "SM" }, { "ST", "ST" }, { "SA", "SA" }, { "SN", "SN" }, { "RS", "RS" }, { "SC", "SC" }, { "SL", "SL" }, { "SG", "SG" }, { "SX", "SX" }, { "SK", "SK" }, { "SI", "SI" }, { "SB", "SB" }, { "SO", "SO" }, { "ZA", "ZA" }, { "GS", "GS" }, { "SS", "SS" }, { "ES", "ES" }, { "LK", "LK" }, { "SD", "SD" }, { "SR", "SR" }, { "SJ", "SJ" }, { "SE", "SE" }, { "CH", "CH" }, { "SY", "SY" }, { "TW", "TW" }, { "TJ", "TJ" }, { "TZ", "TZ" }, { "TH", "TH" }, { "TL", "TL" }, { "TG", "TG" }, { "TK", "TK" }, { "TO", "TO" }, { "TT", "TT" }, { "TN", "TN" }, { "TR", "TR" }, { "TM", "TM" }, { "TC", "TC" }, { "TV", "TV" }, { "UG", "UG" }, { "UA", "UA" }, { "AE", "AE" }, { "GB", "GB" }, { "US", "US" }, { "UM", "UM" }, { "UY", "UY" }, { "UZ", "UZ" }, { "VU", "VU" }, { "VE", "VE" }, { "VN", "VN" }, { "VG", "VG" }, { "VI", "VI" }, { "WF", "WF" }, { "EH", "EH" }, { "YE", "YE" }, { "ZM", "ZM" }, { "ZW", "ZW" }
    };

    static inline const std::unordered_map<std::string_view, std::string_view> m_A3ToA2 = {
        { "AFG", "AF" }, { "ALA", "AX" }, { "ALB", "AL" }, { "DZA", "DZ" }, { "ASM", "AS" }, { "AND", "AD" }, { "AGO", "AO" }, { "AIA", "AI" }, { "ATA", "AQ" }, { "ATG", "AG" }, { "ARG", "AR" }, { "ARM", "AM" }, { "ABW", "AW" }, { "AUS", "AU" }, { "AUT", "AT" }, { "AZE", "AZ" }, { "BHS", "BS" }, { "BHR", "BH" }, { "BGD", "BD" }, { "BRB", "BB" }, { "BLR", "BY" }, { "BEL", "BE" }, { "BLZ", "BZ" }, { "BEN", "BJ" }, { "BMU", "BM" }, { "BTN", "BT" }, { "BOL", "BO" }, { "BES", "BQ" }, { "BIH", "BA" }, { "BWA", "BW" }, { "BVT", "BV" }, { "BRA", "BR" }, { "IOT", "IO" }, { "BRN", "BN" }, { "BGR", "BG" }, { "BFA", "BF" }, { "BDI", "BI" }, { "CPV", "CV" }, { "KHM", "KH" }, { "CMR", "CM" }, { "CAN", "CA" }, { "CYM", "KY" }, { "CAF", "CF" }, { "TCD", "TD" }, { "CHL", "CL" }, { "CHN", "CN" }, { "CXR", "CX" }, { "CCK", "CC" }, { "COL", "CO" }, { "COM", "KM" }, { "COG", "CG" }, { "COD", "CD" }, { "COK", "CK" }, { "CRI", "CR" }, { "CIV", "CI" }, { "HRV", "HR" }, { "CUB", "CU" }, { "CUW", "CW" }, { "CYP", "CY" }, { "CZE", "CZ" }, { "DNK", "DK" }, { "DJI", "DJ" }, { "DMA", "DM" }, { "DOM", "DO" }, { "ECU", "EC" }, { "EGY", "EG" }, { "SLV", "SV" }, { "GNQ", "GQ" }, { "ERI", "ER" }, { "EST", "EE" }, { "SWZ", "SZ" }, { "ETH", "ET" }, { "FLK", "FK" }, { "FRO", "FO" }, { "FJI", "FJ" }, { "FIN", "FI" }, { "FRA", "FR" }, { "GUF", "GF" }, { "PYF", "PF" }, { "ATF", "TF" }, { "GAB", "GA" }, { "GMB", "GM" }, { "GEO", "GE" }, { "DEU", "DE" }, { "GHA", "GH" }, { "GIB", "GI" }, { "GRC", "GR" }, { "GRL", "GL" }, { "GRD", "GD" }, { "GLP", "GP" }, { "GUM", "GU" }, { "GTM", "GT" }, { "GGY", "GG" }, { "GIN", "GN" }, { "GNB", "GW" }, { "GUY", "GY" }, { "HTI", "HT" }, { "HMD", "HM" }, { "VAT", "VA" }, { "HND", "HN" }, { "HKG", "HK" }, { "HUN", "HU" }, { "ISL", "IS" }, { "IND", "IN" }, { "IDN", "ID" }, { "IRN", "IR" }, { "IRQ", "IQ" }, { "IRL", "IE" }, { "IMN", "IM" }, { "ISR", "IL" }, { "ITA", "IT" }, { "JAM", "JM" }, { "JPN", "JP" }, { "JEY", "JE" }, { "JOR", "JO" }, { "KAZ", "KZ" }, { "KEN", "KE" }, { "KIR", "KI" }, { "PRK", "KP" }, { "KOR", "KR" }, { "KWT", "KW" }, { "KGZ", "KG" }, { "LAO", "LA" }, { "LVA", "LV" }, { "LBN", "LB" }, { "LSO", "LS" }, { "LBR", "LR" }, { "LBY", "LY" }, { "LIE", "LI" }, { "LTU", "LT" }, { "LUX", "LU" }, { "MAC", "MO" }, { "MDG", "MG" }, { "MWI", "MW" }, { "MYS", "MY" }, { "MDV", "MV" }, { "MLI", "ML" }, { "MLT", "MT" }, { "MHL", "MH" }, { "MTQ", "MQ" }, { "MRT", "MR" }, { "MUS", "MU" }, { "MYT", "YT" }, { "MEX", "MX" }, { "FSM", "FM" }, { "MDA", "MD" }, { "MCO", "MC" }, { "MNG", "MN" }, { "MNE", "ME" }, { "MSR", "MS" }, { "MAR", "MA" }, { "MOZ", "MZ" }, { "MMR", "MM" }, { "NAM", "NA" }, { "NRU", "NR" }, { "NPL", "NP" }, { "NLD", "NL" }, { "NCL", "NC" }, { "NZL", "NZ" }, { "NIC", "NI" }, { "NER", "NE" }, { "NGA", "NG" }, { "NIU", "NU" }, { "NFK", "NF" }, { "MKD", "MK" }, { "MNP", "MP" }, { "NOR", "NO" }, { "OMN", "OM" }, { "PAK", "PK" }, { "PLW", "PW" }, { "PSE", "PS" }, { "PAN", "PA" }, { "PNG", "PG" }, { "PRY", "PY" }, { "PER", "PE" }, { "PHL", "PH" }, { "PCN", "PN" }, { "POL", "PL" }, { "PRT", "PT" }, { "PRI", "PR" }, { "QAT", "QA" }, { "REU", "RE" }, { "ROU", "RO" }, { "RUS", "RU" }, { "RWA", "RW" }, { "BLM", "BL" }, { "SHN", "SH" }, { "KNA", "KN" }, { "LCA", "LC" }, { "MAF", "MF" }, { "SPM", "PM" }, { "VCT", "VC" }, { "WSM", "WS" }, { "SMR", "SM" }, { "STP", "ST" }, { "SAU", "SA" }, { "SEN", "SN" }, { "SRB", "RS" }, { "SYC", "SC" }, { "SLE", "SL" }, { "SGP", "SG" }, { "SXM", "SX" }, { "SVK", "SK" }, { "SVN", "SI" }, { "SLB", "SB" }, { "SOM", "SO" }, { "ZAF", "ZA" }, { "SGS", "GS" }, { "SSD", "SS" }, { "ESP", "ES" }, { "LKA", "LK" }, { "SDN", "SD" }, { "SUR", "SR" }, { "SJM", "SJ" }, { "SWE", "SE" }, { "CHE", "CH" }, { "SYR", "SY" }, { "TWN", "TW" }, { "TJK", "TJ" }, { "TZA", "TZ" }, { "THA", "TH" }, { "TLS", "TL" }, { "TGO", "TG" }, { "TKL", "TK" }, { "TON", "TO" }, { "TTO", "TT" }, { "TUN", "TN" }, { "TUR", "TR" }, { "TKM", "TM" }, { "TCA", "TC" }, { "TUV", "TV" }, { "UGA", "UG" }, { "UKR", "UA" }, { "ARE", "AE" }, { "GBR", "GB" }, { "USA", "US" }, { "UMI", "UM" }, { "URY", "UY" }, { "UZB", "UZ" }, { "VUT", "VU" }, { "VEN", "VE" }, { "VNM", "VN" }, { "VGB", "VG" }, { "VIR", "VI" }, { "WLF", "WF" }, { "ESH", "EH" }, { "YEM", "YE" }, { "ZMB", "ZM" }, { "ZWE", "ZW" }
    };

    static inline const std::unordered_map<std::string_view, std::string_view> m_nameToA2 = {
        { "AFGHANISTAN", "AF" }, { "ÅLAND ISLANDS", "AX" }, { "ALBANIA", "AL" }, { "ALGERIA", "DZ" }, { "AMERICAN SAMOA", "AS" }, { "ANDORRA", "AD" }, { "ANGOLA", "AO" }, { "ANGUILLA", "AI" }, { "ANTARCTICA", "AQ" }, { "ANTIGUA AND BARBUDA", "AG" }, { "ARGENTINA", "AR" }, { "ARMENIA", "AM" }, { "ARUBA", "AW" }, { "AUSTRALIA", "AU" }, { "AUSTRIA", "AT" }, { "AZERBAIJAN", "AZ" }, { "BAHAMAS", "BS" }, { "BAHRAIN", "BH" }, { "BANGLADESH", "BD" }, { "BARBADOS", "BB" }, { "BELARUS", "BY" }, { "BELGIUM", "BE" }, { "BELIZE", "BZ" }, { "BENIN", "BJ" }, { "BERMUDA", "BM" }, { "BHUTAN", "BT" }, { "BOLIVIA", "BO" }, { "BONAIRE", "BQ" }, { "BOSNIA AND HERZEGOVINA", "BA" }, { "BOTSWANA", "BW" }, { "BOUVET ISLAND", "BV" }, { "BRAZIL", "BR" }, { "BRITISH INDIAN OCEAN TERRITORY", "IO" }, { "BRUNEI DARUSSALAM", "BN" }, { "BULGARIA", "BG" }, { "BURKINA FASO", "BF" }, { "BURUNDI", "BI" }, { "CABO VERDE", "CV" }, { "CAMBODIA", "KH" }, { "CAMEROON", "CM" }, { "CANADA", "CA" }, { "CAYMAN ISLANDS", "KY" }, { "CENTRAL AFRICAN REPUBLIC", "CF" }, { "CHAD", "TD" }, { "CHILE", "CL" }, { "CHINA", "CN" }, { "CHRISTMAS ISLAND", "CX" }, { "COCOS (KEELING) ISLANDS", "CC" }, { "COLOMBIA", "CO" }, { "COMOROS", "KM" }, { "CONGO", "CG" }, { "CONGO", "CD" }, { "COOK ISLANDS", "CK" }, { "COSTA RICA", "CR" }, { "CÔTE D'IVOIRE", "CI" }, { "CROATIA", "HR" }, { "CUBA", "CU" }, { "CURAÇAO", "CW" }, { "CYPRUS", "CY" }, { "CZECHIA", "CZ" }, { "DENMARK", "DK" }, { "DJIBOUTI", "DJ" }, { "DOMINICA", "DM" }, { "DOMINICAN REPUBLIC", "DO" }, { "ECUADOR", "EC" }, { "EGYPT", "EG" }, { "EL SALVADOR", "SV" }, { "EQUATORIAL GUINEA", "GQ" }, { "ERITREA", "ER" }, { "ESTONIA", "EE" }, { "ESWATINI", "SZ" }, { "ETHIOPIA", "ET" }, { "FALKLAND ISLANDS (MALVINAS)", "FK" }, { "FAROE ISLANDS", "FO" }, { "FIJI", "FJ" }, { "FINLAND", "FI" }, { "FRANCE", "FR" }, { "FRENCH GUIANA", "GF" }, { "FRENCH POLYNESIA", "PF" }, { "FRENCH SOUTHERN TERRITORIES", "TF" }, { "GABON", "GA" }, { "GAMBIA", "GM" }, { "GEORGIA", "GE" }, { "GERMANY", "DE" }, { "GHANA", "GH" }, { "GIBRALTAR", "GI" }, { "GREECE", "GR" }, { "GREENLAND", "GL" }, { "GRENADA", "GD" }, { "GUADELOUPE", "GP" }, { "GUAM", "GU" }, { "GUATEMALA", "GT" }, { "GUERNSEY", "GG" }, { "GUINEA", "GN" }, { "GUINEA-BISSAU", "GW" }, { "GUYANA", "GY" }, { "HAITI", "HT" }, { "HEARD ISLAND AND MCDONALD ISLANDS", "HM" }, { "HOLY SEE", "VA" }, { "HONDURAS", "HN" }, { "HONG KONG", "HK" }, { "HUNGARY", "HU" }, { "ICELAND", "IS" }, { "INDIA", "IN" }, { "INDONESIA", "ID" }, { "IRAN", "IR" }, { "IRAQ", "IQ" }, { "IRELAND", "IE" }, { "ISLE OF MAN", "IM" }, { "ISRAEL", "IL" }, { "ITALY", "IT" }, { "JAMAICA", "JM" }, { "JAPAN", "JP" }, { "JERSEY", "JE" }, { "JORDAN", "JO" }, { "KAZAKHSTAN", "KZ" }, { "KENYA", "KE" }, { "KIRIBATI", "KI" }, { "NORTH KOREA", "KP" }, { "SOUTH KOREA", "KR" }, { "KUWAIT", "KW" }, { "KYRGYZSTAN", "KG" }, { "LAOS", "LA" }, { "LATVIA", "LV" }, { "LEBANON", "LB" }, { "LESOTHO", "LS" }, { "LIBERIA", "LR" }, { "LIBYA", "LY" }, { "LIECHTENSTEIN", "LI" }, { "LITHUANIA", "LT" }, { "LUXEMBOURG", "LU" }, { "MACAO", "MO" }, { "MADAGASCAR", "MG" }, { "MALAWI", "MW" }, { "MALAYSIA", "MY" }, { "MALDIVES", "MV" }, { "MALI", "ML" }, { "MALTA", "MT" }, { "MARSHALL ISLANDS", "MH" }, { "MARTINIQUE", "MQ" }, { "MAURITANIA", "MR" }, { "MAURITIUS", "MU" }, { "MAYOTTE", "YT" }, { "MEXICO", "MX" }, { "MICRONESIA", "FM" }, { "MOLDOVA", "MD" }, { "MONACO", "MC" }, { "MONGOLIA", "MN" }, { "MONTENEGRO", "ME" }, { "MONTSERRAT", "MS" }, { "MOROCCO", "MA" }, { "MOZAMBIQUE", "MZ" }, { "MYANMAR", "MM" }, { "NAMIBIA", "NA" }, { "NAURU", "NR" }, { "NEPAL", "NP" }, { "NETHERLANDS", "NL" }, { "NEW CALEDONIA", "NC" }, { "NEW ZEALAND", "NZ" }, { "NICARAGUA", "NI" }, { "NIGER", "NE" }, { "NIGERIA", "NG" }, { "NIUE", "NU" }, { "NORFOLK ISLAND", "NF" }, { "NORTH MACEDONIA", "MK" }, { "NORTHERN MARIANA ISLANDS", "MP" }, { "NORWAY", "NO" }, { "OMAN", "OM" }, { "PAKISTAN", "PK" }, { "PALAU", "PW" }, { "PALESTINE", "PS" }, { "PANAMA", "PA" }, { "PAPUA NEW GUINEA", "PG" }, { "PARAGUAY", "PY" }, { "PERU", "PE" }, { "PHILIPPINES", "PH" }, { "PITCAIRN", "PN" }, { "POLAND", "PL" }, { "PORTUGAL", "PT" }, { "PUERTO RICO", "PR" }, { "QATAR", "QA" }, { "RÉUNION", "RE" }, { "ROMANIA", "RO" }, { "RUSSIAN FEDERATION", "RU" }, { "RWANDA", "RW" }, { "SAINT BARTHÉLEMY", "BL" }, { "SAINT HELENA", "SH" }, { "SAINT KITTS AND NEVIS", "KN" }, { "SAINT LUCIA", "LC" }, { "SAINT MARTIN", "MF" }, { "SAINT PIERRE AND MIQUELON", "PM" }, { "SAINT VINCENT AND THE GRENADINES", "VC" }, { "SAMOA", "WS" }, { "SAN MARINO", "SM" }, { "SAO TOME AND PRINCIPE", "ST" }, { "SAUDI ARABIA", "SA" }, { "SENEGAL", "SN" }, { "SERBIA", "RS" }, { "SEYCHELLES", "SC" }, { "SIERRA LEONE", "SL" }, { "SINGAPORE", "SG" }, { "SINT MAARTEN", "SX" }, { "SLOVAKIA", "SK" }, { "SLOVENIA", "SI" }, { "SOLOMON ISLANDS", "SB" }, { "SOMALIA", "SO" }, { "SOUTH AFRICA", "ZA" }, { "SOUTH GEORGIA AND THE SOUTH SANDWICH ISLANDS", "GS" }, { "SOUTH SUDAN", "SS" }, { "SPAIN", "ES" }, { "SRI LANKA", "LK" }, { "SUDAN", "SD" }, { "SURINAME", "SR" }, { "SVALBARD AND JAN MAYEN", "SJ" }, { "SWEDEN", "SE" }, { "SWITZERLAND", "CH" }, { "SYRIAN ARAB REPUBLIC", "SY" }, { "TAIWAN", "TW" }, { "TAJIKISTAN", "TJ" }, { "TANZANIA", "TZ" }, { "THAILAND", "TH" }, { "TIMOR-LESTE", "TL" }, { "TOGO", "TG" }, { "TOKELAU", "TK" }, { "TONGA", "TO" }, { "TRINIDAD AND TOBAGO", "TT" }, { "TUNISIA", "TN" }, { "TÜRKIYE", "TR" }, { "TURKMENISTAN", "TM" }, { "TURKS AND CAICOS ISLANDS", "TC" }, { "TUVALU", "TV" }, { "UGANDA", "UG" }, { "UKRAINE", "UA" }, { "UNITED ARAB EMIRATES", "AE" }, { "UNITED KINGDOM OF GREAT BRITAIN AND NORTHERN IRELAND", "GB" }, { "UNITED STATES OF AMERICA", "US" }, { "UNITED STATES MINOR OUTLYING ISLANDS", "UM" }, { "URUGUAY", "UY" }, { "UZBEKISTAN", "UZ" }, { "VANUATU", "VU" }, { "VENEZUELA", "VE" }, { "VIETNAM", "VN" }, { "VIRGIN ISLANDS (BRITISH)", "VG" }, { "VIRGIN ISLANDS (U.S.)", "VI" }, { "WALLIS AND FUTUNA", "WF" }, { "WESTERN SAHARA", "EH" }, { "YEMEN", "YE" }, { "ZAMBIA", "ZM" }, { "ZIMBABWE", "ZW" }
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

    Value getValue() const { return m_value; }

private:
    Value m_value;
};

class Gamemode
{
public:
    enum Mode
    {
        Osu,
        Taiko,
        Mania,
        Catch
    };

    Gamemode() : m_mode(Mode::Osu) {}
    Gamemode(Mode m) : m_mode(m) {}
    Gamemode(int i) : m_mode(static_cast<Mode>(i)) {}

    bool operator==(const Gamemode& other) const { return m_mode == other.m_mode; }
    bool operator==(Mode m) const { return m_mode == m; }

    std::string toString() const
    {
        switch (m_mode)
        {
        case Mode::Osu: return "osu";
        case Mode::Taiko: return "taiko";
        case Mode::Mania: return "mania";
        case Mode::Catch: return "fruits";
        default: return "UNKNOWN";
        }
    }

    int toInt() const
    {
        return static_cast<int>(m_mode);
    }

    static bool fromString(const std::string s, Gamemode& mode)
    {
        std::string sl = s;
        std::transform(sl.begin(), sl.end(), sl.begin(), ::tolower);

        if ((sl == "osu") || (sl == "std") || (sl == "standard"))
        {
            mode = Mode::Osu;
            return true;
        }
        else if (sl == "taiko")
        {
            mode = Mode::Taiko;
            return true;
        }
        else if (sl == "mania")
        {
            mode = Mode::Mania;
            return true;
        }
        else if ((sl == "catch") || (sl == "fruits"))
        {
            mode = Mode::Catch;
            return true;
        }

        return false;
    }

    static Mode fromInt(const int i)
    {
        return static_cast<Mode>(i);
    }

    Mode getMode() const { return m_mode; }

private:
    Mode m_mode;
};

namespace std
{
template<>
struct hash<Gamemode>
{
    size_t operator()(const Gamemode& gm) const
    {
        return hash<int>()(gm.toInt());
    }
};
}

class EmbedMetadata
{
public:
    EmbedMetadata()
    : m_countryCode(k_global)
    , m_rankRange(RankRange())
    , m_gamemode(Gamemode())
    {}

    EmbedMetadata(std::string countryCode, RankRange rankRange, Gamemode gamemode)
    : m_countryCode(countryCode)
    , m_rankRange(rankRange)
    , m_gamemode(gamemode)
    {}

    std::string m_countryCode;
    RankRange m_rankRange;
    Gamemode m_gamemode;
};

#endif /* __UTIL_H__ */
