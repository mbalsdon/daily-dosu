#ifndef __HTTP_REQUESTER_H__
#define __HTTP_REQUESTER_H__

#include <curl/curl.h>

#include <vector>
#include <string>

/**
 * Makes HTTP requests.
 */
class HttpRequester
{
public:
    HttpRequester();
    ~HttpRequester() noexcept;

    [[nodiscard]] bool makeRequest(
        std::string const& url,
        std::string const& method,
        std::vector<std::string> const& headers,
        std::string const& body,
        long& httpCode /* out */,
        std::string& responseData /* out */);

private:
    CURL* m_curlHandle;
};

#endif /* __HTTP_REQUESTER_H__ */