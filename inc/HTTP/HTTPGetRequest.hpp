#pragma once

#include "IHTTPRequest.hpp"

class HTTPGetRequest : public IHTTPRequest {
private:
	RequestLine _requestLine;
	std::map<std::string, std::string> _headers;

public:
		HTTPGetRequest();
		~HTTPGetRequest() override;

		bool parseRequest(const std::string &data) override;
		Method getMethod() const override;
		std::string getUri() const override;
		std::string getVersion() const override;
		std::string getBody() const override;
		std::string getHeader(const std::string &key) const override;
		std::map<std::string, std::string> getHeaders() const override;
};
