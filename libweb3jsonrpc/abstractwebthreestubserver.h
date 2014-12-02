/**
 * This file is generated by jsonrpcstub, DO NOT CHANGE IT MANUALLY!
 */

#ifndef JSONRPC_CPP_STUB_ABSTRACTWEBTHREESTUBSERVER_H_
#define JSONRPC_CPP_STUB_ABSTRACTWEBTHREESTUBSERVER_H_

#include <jsonrpccpp/server.h>

class AbstractWebThreeStubServer : public jsonrpc::AbstractServer<AbstractWebThreeStubServer>
{
    public:
        AbstractWebThreeStubServer(jsonrpc::AbstractServerConnector &conn) : jsonrpc::AbstractServer<AbstractWebThreeStubServer>(conn)
        {
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_coinbase", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING,  NULL), &AbstractWebThreeStubServer::eth_coinbaseI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_setCoinbase", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_STRING, NULL), &AbstractWebThreeStubServer::eth_setCoinbaseI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_listening", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN,  NULL), &AbstractWebThreeStubServer::eth_listeningI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_setListening", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_BOOLEAN, NULL), &AbstractWebThreeStubServer::eth_setListeningI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_mining", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN,  NULL), &AbstractWebThreeStubServer::eth_miningI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_setMining", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_BOOLEAN, NULL), &AbstractWebThreeStubServer::eth_setMiningI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_gasPrice", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING,  NULL), &AbstractWebThreeStubServer::eth_gasPriceI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_accounts", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_ARRAY,  NULL), &AbstractWebThreeStubServer::eth_accountsI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_peerCount", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_INTEGER,  NULL), &AbstractWebThreeStubServer::eth_peerCountI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_defaultBlock", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_INTEGER,  NULL), &AbstractWebThreeStubServer::eth_defaultBlockI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_setDefaultBlock", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_INTEGER, NULL), &AbstractWebThreeStubServer::eth_setDefaultBlockI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_number", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_INTEGER,  NULL), &AbstractWebThreeStubServer::eth_numberI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_balanceAt", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, "param1",jsonrpc::JSON_STRING, NULL), &AbstractWebThreeStubServer::eth_balanceAtI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_stateAt", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, "param1",jsonrpc::JSON_STRING,"param2",jsonrpc::JSON_STRING, NULL), &AbstractWebThreeStubServer::eth_stateAtI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_storageAt", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",jsonrpc::JSON_STRING, NULL), &AbstractWebThreeStubServer::eth_storageAtI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_countAt", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_REAL, "param1",jsonrpc::JSON_STRING, NULL), &AbstractWebThreeStubServer::eth_countAtI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_codeAt", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, "param1",jsonrpc::JSON_STRING, NULL), &AbstractWebThreeStubServer::eth_codeAtI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_transact", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, "param1",jsonrpc::JSON_OBJECT, NULL), &AbstractWebThreeStubServer::eth_transactI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_call", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, "param1",jsonrpc::JSON_OBJECT, NULL), &AbstractWebThreeStubServer::eth_callI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_blockByHash", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",jsonrpc::JSON_STRING, NULL), &AbstractWebThreeStubServer::eth_blockByHashI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_blockByNumber", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",jsonrpc::JSON_INTEGER, NULL), &AbstractWebThreeStubServer::eth_blockByNumberI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_transactionByHash", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",jsonrpc::JSON_STRING,"param2",jsonrpc::JSON_INTEGER, NULL), &AbstractWebThreeStubServer::eth_transactionByHashI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_transactionByNumber", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",jsonrpc::JSON_INTEGER,"param2",jsonrpc::JSON_INTEGER, NULL), &AbstractWebThreeStubServer::eth_transactionByNumberI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_uncleByHash", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",jsonrpc::JSON_STRING,"param2",jsonrpc::JSON_INTEGER, NULL), &AbstractWebThreeStubServer::eth_uncleByHashI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_uncleByNumber", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_OBJECT, "param1",jsonrpc::JSON_INTEGER,"param2",jsonrpc::JSON_INTEGER, NULL), &AbstractWebThreeStubServer::eth_uncleByNumberI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_compilers", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_ARRAY,  NULL), &AbstractWebThreeStubServer::eth_compilersI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_lll", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, "param1",jsonrpc::JSON_STRING, NULL), &AbstractWebThreeStubServer::eth_lllI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_solidity", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, "param1",jsonrpc::JSON_STRING, NULL), &AbstractWebThreeStubServer::eth_solidityI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_serpent", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, "param1",jsonrpc::JSON_STRING, NULL), &AbstractWebThreeStubServer::eth_serpentI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_newFilter", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_INTEGER, "param1",jsonrpc::JSON_OBJECT, NULL), &AbstractWebThreeStubServer::eth_newFilterI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_newFilterString", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_INTEGER, "param1",jsonrpc::JSON_STRING, NULL), &AbstractWebThreeStubServer::eth_newFilterStringI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_uninstallFilter", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_INTEGER, NULL), &AbstractWebThreeStubServer::eth_uninstallFilterI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_changed", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_INTEGER, NULL), &AbstractWebThreeStubServer::eth_changedI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_filterLogs", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_ARRAY, "param1",jsonrpc::JSON_INTEGER, NULL), &AbstractWebThreeStubServer::eth_filterLogsI);
            this->bindAndAddMethod(new jsonrpc::Procedure("eth_logs", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_ARRAY, "param1",jsonrpc::JSON_OBJECT, NULL), &AbstractWebThreeStubServer::eth_logsI);
            this->bindAndAddMethod(new jsonrpc::Procedure("db_put", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_STRING,"param2",jsonrpc::JSON_STRING,"param3",jsonrpc::JSON_STRING, NULL), &AbstractWebThreeStubServer::db_putI);
            this->bindAndAddMethod(new jsonrpc::Procedure("db_get", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, "param1",jsonrpc::JSON_STRING,"param2",jsonrpc::JSON_STRING, NULL), &AbstractWebThreeStubServer::db_getI);
            this->bindAndAddMethod(new jsonrpc::Procedure("db_putString", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_STRING,"param2",jsonrpc::JSON_STRING,"param3",jsonrpc::JSON_STRING, NULL), &AbstractWebThreeStubServer::db_putStringI);
            this->bindAndAddMethod(new jsonrpc::Procedure("db_getString", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, "param1",jsonrpc::JSON_STRING,"param2",jsonrpc::JSON_STRING, NULL), &AbstractWebThreeStubServer::db_getStringI);
            this->bindAndAddMethod(new jsonrpc::Procedure("shh_post", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_OBJECT, NULL), &AbstractWebThreeStubServer::shh_postI);
            this->bindAndAddMethod(new jsonrpc::Procedure("shh_newIdentity", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING,  NULL), &AbstractWebThreeStubServer::shh_newIdentityI);
            this->bindAndAddMethod(new jsonrpc::Procedure("shh_haveIdentity", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_STRING, NULL), &AbstractWebThreeStubServer::shh_haveIdentityI);
            this->bindAndAddMethod(new jsonrpc::Procedure("shh_newGroup", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, "param1",jsonrpc::JSON_STRING,"param2",jsonrpc::JSON_STRING, NULL), &AbstractWebThreeStubServer::shh_newGroupI);
            this->bindAndAddMethod(new jsonrpc::Procedure("shh_addToGroup", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_STRING, "param1",jsonrpc::JSON_STRING,"param2",jsonrpc::JSON_STRING, NULL), &AbstractWebThreeStubServer::shh_addToGroupI);
            this->bindAndAddMethod(new jsonrpc::Procedure("shh_newFilter", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_INTEGER, "param1",jsonrpc::JSON_OBJECT, NULL), &AbstractWebThreeStubServer::shh_newFilterI);
            this->bindAndAddMethod(new jsonrpc::Procedure("shh_uninstallFilter", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_BOOLEAN, "param1",jsonrpc::JSON_INTEGER, NULL), &AbstractWebThreeStubServer::shh_uninstallFilterI);
            this->bindAndAddMethod(new jsonrpc::Procedure("shh_changed", jsonrpc::PARAMS_BY_POSITION, jsonrpc::JSON_ARRAY, "param1",jsonrpc::JSON_INTEGER, NULL), &AbstractWebThreeStubServer::shh_changedI);
        }

        inline virtual void eth_coinbaseI(const Json::Value &request, Json::Value &response)
        {
            (void)request;
            response = this->eth_coinbase();
        }
        inline virtual void eth_setCoinbaseI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_setCoinbase(request[0u].asString());
        }
        inline virtual void eth_listeningI(const Json::Value &request, Json::Value &response)
        {
            (void)request;
            response = this->eth_listening();
        }
        inline virtual void eth_setListeningI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_setListening(request[0u].asBool());
        }
        inline virtual void eth_miningI(const Json::Value &request, Json::Value &response)
        {
            (void)request;
            response = this->eth_mining();
        }
        inline virtual void eth_setMiningI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_setMining(request[0u].asBool());
        }
        inline virtual void eth_gasPriceI(const Json::Value &request, Json::Value &response)
        {
            (void)request;
            response = this->eth_gasPrice();
        }
        inline virtual void eth_accountsI(const Json::Value &request, Json::Value &response)
        {
            (void)request;
            response = this->eth_accounts();
        }
        inline virtual void eth_peerCountI(const Json::Value &request, Json::Value &response)
        {
            (void)request;
            response = this->eth_peerCount();
        }
        inline virtual void eth_defaultBlockI(const Json::Value &request, Json::Value &response)
        {
            (void)request;
            response = this->eth_defaultBlock();
        }
        inline virtual void eth_setDefaultBlockI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_setDefaultBlock(request[0u].asInt());
        }
        inline virtual void eth_numberI(const Json::Value &request, Json::Value &response)
        {
            (void)request;
            response = this->eth_number();
        }
        inline virtual void eth_balanceAtI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_balanceAt(request[0u].asString());
        }
        inline virtual void eth_stateAtI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_stateAt(request[0u].asString(), request[1u].asString());
        }
        inline virtual void eth_storageAtI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_storageAt(request[0u].asString());
        }
        inline virtual void eth_countAtI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_countAt(request[0u].asString());
        }
        inline virtual void eth_codeAtI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_codeAt(request[0u].asString());
        }
        inline virtual void eth_transactI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_transact(request[0u]);
        }
        inline virtual void eth_callI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_call(request[0u]);
        }
        inline virtual void eth_blockByHashI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_blockByHash(request[0u].asString());
        }
        inline virtual void eth_blockByNumberI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_blockByNumber(request[0u].asInt());
        }
        inline virtual void eth_transactionByHashI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_transactionByHash(request[0u].asString(), request[1u].asInt());
        }
        inline virtual void eth_transactionByNumberI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_transactionByNumber(request[0u].asInt(), request[1u].asInt());
        }
        inline virtual void eth_uncleByHashI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_uncleByHash(request[0u].asString(), request[1u].asInt());
        }
        inline virtual void eth_uncleByNumberI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_uncleByNumber(request[0u].asInt(), request[1u].asInt());
        }
        inline virtual void eth_compilersI(const Json::Value &request, Json::Value &response)
        {
            (void)request;
            response = this->eth_compilers();
        }
        inline virtual void eth_lllI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_lll(request[0u].asString());
        }
        inline virtual void eth_solidityI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_solidity(request[0u].asString());
        }
        inline virtual void eth_serpentI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_serpent(request[0u].asString());
        }
        inline virtual void eth_newFilterI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_newFilter(request[0u]);
        }
        inline virtual void eth_newFilterStringI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_newFilterString(request[0u].asString());
        }
        inline virtual void eth_uninstallFilterI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_uninstallFilter(request[0u].asInt());
        }
        inline virtual void eth_changedI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_changed(request[0u].asInt());
        }
        inline virtual void eth_filterLogsI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_filterLogs(request[0u].asInt());
        }
        inline virtual void eth_logsI(const Json::Value &request, Json::Value &response)
        {
            response = this->eth_logs(request[0u]);
        }
        inline virtual void db_putI(const Json::Value &request, Json::Value &response)
        {
            response = this->db_put(request[0u].asString(), request[1u].asString(), request[2u].asString());
        }
        inline virtual void db_getI(const Json::Value &request, Json::Value &response)
        {
            response = this->db_get(request[0u].asString(), request[1u].asString());
        }
        inline virtual void db_putStringI(const Json::Value &request, Json::Value &response)
        {
            response = this->db_putString(request[0u].asString(), request[1u].asString(), request[2u].asString());
        }
        inline virtual void db_getStringI(const Json::Value &request, Json::Value &response)
        {
            response = this->db_getString(request[0u].asString(), request[1u].asString());
        }
        inline virtual void shh_postI(const Json::Value &request, Json::Value &response)
        {
            response = this->shh_post(request[0u]);
        }
        inline virtual void shh_newIdentityI(const Json::Value &request, Json::Value &response)
        {
            (void)request;
            response = this->shh_newIdentity();
        }
        inline virtual void shh_haveIdentityI(const Json::Value &request, Json::Value &response)
        {
            response = this->shh_haveIdentity(request[0u].asString());
        }
        inline virtual void shh_newGroupI(const Json::Value &request, Json::Value &response)
        {
            response = this->shh_newGroup(request[0u].asString(), request[1u].asString());
        }
        inline virtual void shh_addToGroupI(const Json::Value &request, Json::Value &response)
        {
            response = this->shh_addToGroup(request[0u].asString(), request[1u].asString());
        }
        inline virtual void shh_newFilterI(const Json::Value &request, Json::Value &response)
        {
            response = this->shh_newFilter(request[0u]);
        }
        inline virtual void shh_uninstallFilterI(const Json::Value &request, Json::Value &response)
        {
            response = this->shh_uninstallFilter(request[0u].asInt());
        }
        inline virtual void shh_changedI(const Json::Value &request, Json::Value &response)
        {
            response = this->shh_changed(request[0u].asInt());
        }
        virtual std::string eth_coinbase() = 0;
        virtual bool eth_setCoinbase(const std::string& param1) = 0;
        virtual bool eth_listening() = 0;
        virtual bool eth_setListening(const bool& param1) = 0;
        virtual bool eth_mining() = 0;
        virtual bool eth_setMining(const bool& param1) = 0;
        virtual std::string eth_gasPrice() = 0;
        virtual Json::Value eth_accounts() = 0;
        virtual int eth_peerCount() = 0;
        virtual int eth_defaultBlock() = 0;
        virtual bool eth_setDefaultBlock(const int& param1) = 0;
        virtual int eth_number() = 0;
        virtual std::string eth_balanceAt(const std::string& param1) = 0;
        virtual std::string eth_stateAt(const std::string& param1, const std::string& param2) = 0;
        virtual Json::Value eth_storageAt(const std::string& param1) = 0;
        virtual double eth_countAt(const std::string& param1) = 0;
        virtual std::string eth_codeAt(const std::string& param1) = 0;
        virtual std::string eth_transact(const Json::Value& param1) = 0;
        virtual std::string eth_call(const Json::Value& param1) = 0;
        virtual Json::Value eth_blockByHash(const std::string& param1) = 0;
        virtual Json::Value eth_blockByNumber(const int& param1) = 0;
        virtual Json::Value eth_transactionByHash(const std::string& param1, const int& param2) = 0;
        virtual Json::Value eth_transactionByNumber(const int& param1, const int& param2) = 0;
        virtual Json::Value eth_uncleByHash(const std::string& param1, const int& param2) = 0;
        virtual Json::Value eth_uncleByNumber(const int& param1, const int& param2) = 0;
        virtual Json::Value eth_compilers() = 0;
        virtual std::string eth_lll(const std::string& param1) = 0;
        virtual std::string eth_solidity(const std::string& param1) = 0;
        virtual std::string eth_serpent(const std::string& param1) = 0;
        virtual int eth_newFilter(const Json::Value& param1) = 0;
        virtual int eth_newFilterString(const std::string& param1) = 0;
        virtual bool eth_uninstallFilter(const int& param1) = 0;
        virtual bool eth_changed(const int& param1) = 0;
        virtual Json::Value eth_filterLogs(const int& param1) = 0;
        virtual Json::Value eth_logs(const Json::Value& param1) = 0;
        virtual bool db_put(const std::string& param1, const std::string& param2, const std::string& param3) = 0;
        virtual std::string db_get(const std::string& param1, const std::string& param2) = 0;
        virtual bool db_putString(const std::string& param1, const std::string& param2, const std::string& param3) = 0;
        virtual std::string db_getString(const std::string& param1, const std::string& param2) = 0;
        virtual bool shh_post(const Json::Value& param1) = 0;
        virtual std::string shh_newIdentity() = 0;
        virtual bool shh_haveIdentity(const std::string& param1) = 0;
        virtual std::string shh_newGroup(const std::string& param1, const std::string& param2) = 0;
        virtual std::string shh_addToGroup(const std::string& param1, const std::string& param2) = 0;
        virtual int shh_newFilter(const Json::Value& param1) = 0;
        virtual bool shh_uninstallFilter(const int& param1) = 0;
        virtual Json::Value shh_changed(const int& param1) = 0;
};

#endif //JSONRPC_CPP_ABSTRACTWEBTHREESTUBSERVER_H_
