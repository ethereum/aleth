#include <boost/filesystem.hpp>
#include "BlockChainListener.h"
#include "BlockChain.h"


using namespace eth;
using namespace std;


void BlockChainListener::onBlockImport(State& _state, BlockInfo _block)
{

    BlockInfo bi = _block;
    map<Address, u256> fullAddressMap = _state.addresses();
    std::ostringstream os;

    string blockNumber = "";
    stringstream ss;
    ss <<  setfill('0') << setw(7) << boost::lexical_cast<string>(bi.number);
    blockNumber = ss.str();

    os << "{" << endl;
    os << "  \"coinbase\": \""       << bi.coinbaseAddress  << "\"," << endl;
    os << "  \"difficulty\": \""     << bi.difficulty       << "\"," << endl;
    os << "  \"extra_data\": \""     << "0x"        << "\"," << endl;
    os << "  \"gas_limit\": \""      << bi.gasLimit         << "\"," << endl;
    os << "  \"gas_used\": \""       << bi.gasUsed          << "\"," << endl;
    os << "  \"min_gas_price\": \""  << bi.minGasPrice      << "\"," << endl;
    os << "  \"nonce\": \"0x"        << bi.nonce            << "\"," << endl;
    os << "  \"number\": \""         << bi.number           << "\"," << endl;
    os << "  \"prevhash\": \"0x"     << bi.parentHash       << "\"," << endl;

    os << "  \"state\": {" << endl;

    string out= "";

    for( map<Address, u256>::const_iterator it = fullAddressMap.begin();
         it != fullAddressMap.end();
         ++it )
    {
        Address key     = it->first;
        string  key_s   = toString(key);
        string  balance = escaped(toBigEndianString(_state.balance(key)));
        string  nonce   = escaped( toString( _state.transactionsFrom(key) ) );

        map<u256, u256> storage = _state.storage(key);
        storage.empty();

        string storage_key;
        std::ostringstream os_storage;
        if (!storage.empty())
        {
            storage_key = toHex( toBigEndianString( _state.storageRoot(key) ) );
            os_storage << "      \"storage\": {" << endl;

            /// traverse the storage
            for( map<u256, u256>::const_iterator it = storage.begin();
                 it != storage.end();
                 ++it )
            {

                u256 key   = it -> first;
                u256 value = it -> second;

                stringstream ss_key;
                ss_key <<  "0x" << setfill('0') << setw(64) << toHex( toBigEndianString( key ) );
                os_storage << "        \"" << ss_key.str() << "\": \"0x" << toHex( toCompactBigEndian( value ) ) << "\"";

                it++;
                if (it  != storage.end()){
                    os_storage << ",";
                }
                it--;

                os_storage << endl;

            }
            os_storage << "      }," << endl;

        } else
        {
            os_storage << "      \"storage\": { }," << endl;
            storage_key = "";
        }

        string code;
        if (!_state.code(key).empty())
        {
            code = "0x" + toHex( _state.code(key) );
        } else
        {
            code = "0x";
        }

        _state.storage(key);
        os << "    \"" << key_s << "\": {\n";
        os << "      \"balance\": \""      << _state.balance(key)          << "\",\n";
        os << "      \"code\": \""         << code    << "\",\n";
        os << "      \"nonce\": \""        << _state.transactionsFrom(key) << "\",\n";
        os << os_storage.str();
        os << "      \"storage_root\": \"" << storage_key << "\"";
        os << "\n    }";

        it++;
        if (it  != fullAddressMap.end())
        {
            os << ",";
        }
        it--;

        os << endl;
    }

    string transactionRootStr = "";
    if (bi.transactionsRoot != 0)
    {
        transactionRootStr = toHex( toBigEndianString(  bi.transactionsRoot ));
    }

    os << "  }," << endl;
    os << "  \"state_root\": \""     <<    toHex( toBigEndianString( bi.stateRoot ) )    << "\"," << endl;
    os << "  \"timestamp\": \""      <<    bi.timestamp       << "\"," << endl;
    os << "  \"transactions\": "     <<    "[ ]"              << "," << endl;
    os << "  \"tx_list_root\": \""   <<    transactionRootStr <<   "\","            << endl;
    os << "  \"uncles_hash\": \"0x"  <<    toHex( toBigEndianString(  bi.sha3Uncles ))    << "\"" << endl;
    os << "}";

    out = out + os.str();

    /// actuall data dump
    boost::filesystem::create_directories("dmp");
    std::vector<unsigned char> source(out.begin(), out.end());

    writeFile("./dmp/" +   blockNumber  + "_c.dmp", source);
};



