#pragma once
#include <string>
#include <jsonrpccpp/server/connectors/httpserver.h>

namespace dev
{

class SafeHttpServer: public jsonrpc::HttpServer
{
public:
	/// "using HttpServer" won't work with msvc 2013, so we need to copy'n'paste constructor
	SafeHttpServer(int port, const std::string& sslcert = "", const std::string& sslkey = "", int threads = 50):
		HttpServer(port, sslcert, sslkey, threads) {};

	/// override HttpServer implementation
	bool virtual SendResponse(std::string const& _response, void* _addInfo = NULL) override;
	bool virtual SendOptionsResponse(void* _addInfo) override;

	void setAllowedOrigin(std::string const& _origin) { m_allowedOrigin = _origin; }
	std::string const& allowedOrigin() const { return m_allowedOrigin; }

private:
	std::string m_allowedOrigin = "";
};

}
