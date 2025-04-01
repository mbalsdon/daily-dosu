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
#include <ctime>
#include <sstream>
#include <regex>
#include <iomanip>
#include <vector>

/* - - - - - - Constants - - - - - - - */

const int k_curlRetryWaitMs = 30000;

static constexpr std::size_t k_getRankingIDMaxNumIDs = 50;
static constexpr std::size_t k_getRankingIDMaxPage = 200;

static constexpr std::size_t k_numDisplayUsersTop = 15;
static constexpr std::size_t k_numDisplayUsersBottom = 5;
const std::size_t k_numDisplayUsers = std::max(k_numDisplayUsersTop, k_numDisplayUsersBottom);

static constexpr std::size_t k_numTopPlays = 100;
static constexpr std::size_t k_numDisplayTopPlays = 5;

const std::string k_defaultDate = "0000-00-00T00:00:00Z";

const std::chrono::hours k_minValidScrapeRankingsHour = std::chrono::hours(22);
const std::chrono::hours k_maxValidScrapeRankingsHour = std::chrono::hours(26);
const std::chrono::hours k_maxValidTopPlaysHour = std::chrono::hours(26);

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

const std::string k_scrapeRankingsFirstRangeButtonID = "sr_first_range_button";
const std::string k_scrapeRankingsSecondRangeButtonID = "sr_second_range_button";
const std::string k_scrapeRankingsThirdRangeButtonID = "sr_third_range_button";
const std::string k_scrapeRankingsFilterCountryButtonID = "sr_filter_country_button";
const std::string k_scrapeRankingsFilterCountryModalID = "sr_filter_country_modal";
const std::string k_scrapeRankingsFilterCountryTextInputID = "sr_filter_country_text_input";
const std::string k_scrapeRankingsSelectModeButtonID = "sr_select_mode_button";
const std::string k_scrapeRankingsSelectModeModalID = "sr_select_mode_modal";
const std::string k_scrapeRankingsSelectModeTextInputID = "sr_select_mode_text_input";
const std::string k_scrapeRankingsResetAllButtonID = "sr_reset_all_button";

const std::string k_topPlaysFilterCountryButtonID = "tp_filter_country_button";
const std::string k_topPlaysFilterCountryModalID = "tp_filter_country_modal";
const std::string k_topPlaysFilterCountryTextInputID = "tp_filter_country_text_input";
const std::string k_topPlaysSelectModeButtonID = "tp_select_mode_button";
const std::string k_topPlaysSelectModeModalID = "tp_select_mode_modal";
const std::string k_topPlaysSelectModeTextInputID = "tp_select_mode_text_input";
const std::string k_topPlaysResetAllButtonID = "tp_reset_all_button";

const std::pair<std::string, std::string> k_newsletterPageOptionScrapeRankings = std::make_pair("Rank Increases", "Rank Increases");
const std::pair<std::string, std::string> k_newsletterPageOptionTopPlays = std::make_pair("Top Plays", "Top Plays");

const std::string k_global = "GLOBAL";

/* - - - - - - - - Types - - - - - - - */

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

/**
 * ISO 8601 date and time, in UTC.
 *
 * YYYY-MM-DDThh:mm:ssZ
 * e.g. 2025-04-01T22:47:49Z
 */
class ISO8601DateTimeUTC
{
public:
    /**
     * Constructs using the current time.
     */
    ISO8601DateTimeUTC()
    {
        auto now = std::chrono::system_clock::now();
        auto nowTt = std::chrono::system_clock::to_time_t(now);
        std::tm* utcTm = std::gmtime(&nowTt);

        m_year = utcTm->tm_year + 1900;
        m_month = utcTm->tm_mon + 1;
        m_day = utcTm->tm_mday;
        m_hour = utcTm->tm_hour;
        m_minute = utcTm->tm_min;
        m_second = utcTm->tm_sec;
    }

    /**
     * Constructs from a string.
     *
     * Throws std::invalid_argument if improperly formatted.
     */
    ISO8601DateTimeUTC(std::string const& dateTimeString)
    {
        std::regex isoRegex(R"((\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2})(?:\.\d{3})?Z)");
        std::smatch matches;

        if (!std::regex_match(dateTimeString, matches, isoRegex))
        {
            throw std::invalid_argument("Invalid ISO 8601 UTC format! Expected YYYY-MM-DDThh:mm:ss[.xxx]Z but got " + dateTimeString);
        }

        // Store the matched components directly in member variables
        m_year = std::stoi(matches[1]);
        m_month = std::stoi(matches[2]);
        m_day = std::stoi(matches[3]);
        m_hour = std::stoi(matches[4]);
        m_minute = std::stoi(matches[5]);
        m_second = std::stoi(matches[6]);
    }

    std::string toString() const
    {
        std::ostringstream oss;
        oss << std::setfill('0')
            << std::setw(4) << m_year << '-'
            << std::setw(2) << m_month << '-'
            << std::setw(2) << m_day << 'T'
            << std::setw(2) << m_hour << ':'
            << std::setw(2) << m_minute << ':'
            << std::setw(2) << m_second << 'Z';
        return oss.str();
    }

    /**
     * YYYY-MM-DD
     */
    std::string toDateString() const
    {
        std::ostringstream oss;
        oss << std::setfill('0')
            << std::setw(4) << m_year << '-'
            << std::setw(2) << m_month << '-'
            << std::setw(2) << m_day;
        return oss.str();
    }

    uint64_t toEpochTime() const
    {
        std::tm timeinfo = {};
        timeinfo.tm_year = m_year - 1900;
        timeinfo.tm_mon = m_month - 1;
        timeinfo.tm_mday = m_day;
        timeinfo.tm_hour = m_hour;
        timeinfo.tm_min = m_minute;
        timeinfo.tm_sec = m_second;
        timeinfo.tm_isdst = 0; // No DST for UTC

        #ifndef _WIN32
            return static_cast<uint64_t>(timegm(&timeinfo));
        #else
            return static_cast<uint64_t>(_mkgmtime(&timeinfo));
        #endif
    }

    bool operator==(const ISO8601DateTimeUTC& other) const
    {
        return ((m_year == other.m_year) &&
                (m_month == other.m_month) &&
                (m_day == other.m_day) &&
                (m_hour == other.m_hour) &&
                (m_minute == other.m_minute) &&
                (m_second == other.m_second));
    }

    void addDays(int const& days)
    {
        if (days == 0) return;

        std::tm timeStruct = {};
        timeStruct.tm_year = m_year - 1900;
        timeStruct.tm_mon = m_month - 1;
        timeStruct.tm_mday = m_day + days;
        timeStruct.tm_hour = m_hour;
        timeStruct.tm_min = m_minute;
        timeStruct.tm_sec = m_second;

        std::time_t tt;
        #ifndef _WIN32
            tt = timegm(&timeStruct);
        #else
            tt = _mkgmtime(&timeStruct);
        #endif
        std::tm* newUtcTm = std::gmtime(&tt);

        m_year = newUtcTm->tm_year + 1900;
        m_month = newUtcTm->tm_mon + 1;
        m_day = newUtcTm->tm_mday;
        m_hour = newUtcTm->tm_hour;
        m_minute = newUtcTm->tm_min;
        m_second = newUtcTm->tm_sec;
    }

private:
    int m_year;
    int m_month;
    int m_day;
    int m_hour;
    int m_minute;
    int m_second;
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
    bool operator!=(const Gamemode& other) const { return m_mode != other.m_mode; }
    bool operator!=(Mode m) const { return m_mode != m; }

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

    int toOsutrackInt() const
    {
        switch (m_mode)
        {
        case Mode::Osu: return 0;
        case Mode::Taiko: return 1;
        case Mode::Catch: return 2;
        case Mode::Mania: return 3;
        default: return -1;
        }
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
} /* namespace std */

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

class OsuMods
{
public:
    OsuMods() = delete;
    ~OsuMods() = default;
    bool operator==(const OsuMods& other) const { return m_mods == other.m_mods; }

    OsuMods(std::vector<std::string> mods)
    {
        m_mods = mods;
    }

    OsuMods(std::string modsStr)
    {
        if (modsStr.length() % 2 != 0)
        {
            std::string reason = "Input string has an uneven number of characters! modsStr=" + modsStr;
            throw std::invalid_argument(reason);
        }

        if (!std::all_of(modsStr.begin(), modsStr.end(), [](char c) { return std::isalpha(c) && std::isupper(c); }))
        {
            std::string reason = "Input string must contain only uppercase alphabetical characters! modsStr=" + modsStr;
            throw std::invalid_argument(reason);
        }

        for (std::size_t i = 0; i < modsStr.length(); i += 2)
        {
            m_mods.push_back(modsStr.substr(i, 2));
        }
    }

    std::string toString() const
    {
        std::string ret = "";
        for (std::string const& modStr : m_mods)
        {
            ret += modStr;
        }
        return ret;
    }

    std::vector<std::string> get() const
    {
        return m_mods;
    }

    bool isNomod()
    {
        return m_mods.empty();
    }

private:
    std::vector<std::string> m_mods = {};
};

typedef std::size_t Page;

typedef int64_t UserID;
typedef std::string Username;
typedef std::string CountryCode;
typedef std::string ProfilePicture;
typedef double PerformancePoints;
typedef double Accuracy;
typedef int64_t HoursPlayed;
typedef int64_t Rank;
typedef int64_t BeatmapID;
typedef int64_t TotalScore;
typedef int64_t ScoreID;
typedef int64_t Combo;
typedef std::string LetterRank;
typedef int64_t HitCount;
typedef double StarRating;
typedef std::string DifficultyName;
typedef std::string BeatmapArtist;
typedef std::string BeatmapTitle;

struct RankingsUser
{
    UserID userID = -1;
    Username username = "";
    CountryCode countryCode = "";
    ProfilePicture pfpLink = "";
    PerformancePoints performancePoints = -1.;
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
            (performancePoints >= 0.) &&
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

struct Beatmap
{
    BeatmapID beatmapID = -1;
    StarRating starRating = -1.;
    DifficultyName difficultyName = "";
    BeatmapArtist artist = "";
    BeatmapTitle title = "";
    Username mapsetCreator = "";
    Combo maxCombo = -1;
};

/**
 * A play set by a user (NOT TotalScore).
 */
struct Score
{
    ScoreID scoreID = -1;
    OsuMods mods {""};
    PerformancePoints performancePoints = -1.;
    Accuracy accuracy = -1.;
    TotalScore totalScore = -1;
    ISO8601DateTimeUTC createdAt {k_defaultDate};
    Combo combo = -1;
    LetterRank letterRank = "";
    HitCount count300 = -1;
    HitCount count100 = -1;
    HitCount count50 = -1;
    HitCount countMiss = -1;
    Beatmap beatmap {};
    RankingsUser user {};
};

struct TopPlay
{
    int64_t rank = -1;
    Score score = {};

    bool isValid() const
    {
        return (
            (rank > 0) &&
            (rank <= static_cast<int64_t>(k_numTopPlays)) &&
            (score.performancePoints >= 0.) &&
            (score.totalScore > 0) &&
            (score.createdAt.toString() != k_defaultDate) &&
            (!score.letterRank.empty()) &&
            (score.beatmap.beatmapID > 0) &&
            (score.user.userID > 0)
        );
    }
};

#endif /* __UTIL_H__ */
