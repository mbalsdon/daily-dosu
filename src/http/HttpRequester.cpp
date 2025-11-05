#include "HttpRequester.h"
#include "Logger.h"

#include <cstddef>
#include <string>

namespace
{
std::size_t curlWriteCallback(void* contents, std::size_t size, std::size_t nmemb, std::string* response)
{
    std::size_t totalSize = size * nmemb;
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}
} /* namespace */

/**
 * HTTPRequester constructor.
 */
HttpRequester::HttpRequester()
{
    m_curlHandle = curl_easy_init();
    LOG_ERROR_THROW(
        m_curlHandle,
        "Failed to initialize CURL handle!"
    );
}

/**
 * HTTPRequester destructor.
 */
HttpRequester::~HttpRequester() noexcept
{
    if (m_curlHandle)
    {
        curl_easy_cleanup(m_curlHandle);
    }
}

/**
 * Send HTTP request.
 */
[[nodiscard]] bool HttpRequester::makeRequest(
    std::string const& url,
    std::string const& method,
    std::vector<std::string> const& headers,
    std::string const& body,
    long& httpCode /* out */,
    std::string& responseData /* out */)
{
    curl_easy_reset(m_curlHandle);

    curl_easy_setopt(m_curlHandle, CURLOPT_URL, url.c_str());

    if (method == "GET")
    {
        curl_easy_setopt(m_curlHandle, CURLOPT_HTTPGET, 1L);
    }
    else if (method == "POST")
    {
        curl_easy_setopt(m_curlHandle, CURLOPT_POST, 1L);
    }
    else
    {
        curl_easy_setopt(m_curlHandle, CURLOPT_CUSTOMREQUEST, method);
    }

    if (!body.empty())
    {
        curl_easy_setopt(m_curlHandle, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(m_curlHandle, CURLOPT_POSTFIELDSIZE, body.length());
    }

    struct curl_slist* curlHeaders = nullptr;
    for (auto const& header : headers)
    {
        curlHeaders = curl_slist_append(curlHeaders, header.c_str());
    }
    if (curlHeaders)
    {
        curl_easy_setopt(m_curlHandle, CURLOPT_HTTPHEADER, curlHeaders);
    }

    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(m_curlHandle, CURLOPT_WRITEDATA, &responseData);

    curl_easy_setopt(m_curlHandle, CURLOPT_USERAGENT, "daily-dosu");
    curl_easy_setopt(m_curlHandle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(m_curlHandle, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(m_curlHandle, CURLOPT_TIMEOUT, 120L);
    curl_easy_setopt(m_curlHandle, CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(m_curlHandle, CURLOPT_NOSIGNAL, 1L);

    curl_easy_setopt(m_curlHandle, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(m_curlHandle, CURLOPT_TCP_KEEPIDLE, 120L);
    curl_easy_setopt(m_curlHandle, CURLOPT_TCP_KEEPINTVL, 60L);

    curl_easy_setopt(m_curlHandle, CURLOPT_LOW_SPEED_LIMIT, 100L);
    curl_easy_setopt(m_curlHandle, CURLOPT_LOW_SPEED_TIME, 60L);

    curl_easy_setopt(m_curlHandle, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(m_curlHandle, CURLOPT_SSL_VERIFYHOST, 2L);

    curl_easy_setopt(m_curlHandle, CURLOPT_DNS_SERVERS, "1.1.1.1,8.8.8.8");

    CURLcode curlResponse = curl_easy_perform(m_curlHandle);

    bool bSuccess = false;
    if (curlResponse == CURLE_OK)
    {
        curl_easy_getinfo(m_curlHandle, CURLINFO_RESPONSE_CODE, &httpCode);
        bSuccess = true;
    }
    else
    {
        LOG_ERROR("Failed to send HTTP request: ", curl_easy_strerror(curlResponse));
    }

    if (curlHeaders)
    {
        curl_slist_free_all(curlHeaders);
    }

    return bSuccess;
}
