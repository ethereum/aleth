#include <microhttpd.h>
#include <sstream>
#include "SafeHttpServer.h"

using namespace std;
using namespace dev;

/// structure copied from libjson-rpc-cpp httpserver version 0.6.0
struct mhd_coninfo
{
	struct MHD_PostProcessor *postprocessor;
	MHD_Connection* connection;
	stringstream request;
	jsonrpc::HttpServer* server;
	int code;
};

bool SafeHttpServer::SendResponse(string const& _response, void* _addInfo)
{
	struct mhd_coninfo* client_connection = static_cast<struct mhd_coninfo*>(_addInfo);
	struct MHD_Response *result = MHD_create_response_from_data(_response.size(),(void *) _response.c_str(), 0, 1);

	MHD_add_response_header(result, "Content-Type", "application/json");
	MHD_add_response_header(result, "Access-Control-Allow-Origin", m_allowedOrigin.c_str());

	int ret = MHD_queue_response(client_connection->connection, client_connection->code, result);
	MHD_destroy_response(result);
	return ret == MHD_YES;
}

bool SafeHttpServer::SendOptionsResponse(void* _addInfo)
{
	struct mhd_coninfo* client_connection = static_cast<struct mhd_coninfo*>(_addInfo);
	struct MHD_Response *result = MHD_create_response_from_data(0, NULL, 0, 1);

	MHD_add_response_header(result, "Allow", "POST, OPTIONS");
	MHD_add_response_header(result, "Access-Control-Allow-Origin", m_allowedOrigin.c_str());
	MHD_add_response_header(result, "Access-Control-Allow-Headers", "origin, content-type, accept");
	MHD_add_response_header(result, "DAV", "1");

	int ret = MHD_queue_response(client_connection->connection, client_connection->code, result);
	MHD_destroy_response(result);
	return ret == MHD_YES;
}
