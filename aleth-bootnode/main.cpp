/*
    This file is part of aleth.

    aleth is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    aleth is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with aleth.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <libdevcore/FileSystem.h>
#include <libdevcore/LoggingProgramOptions.h>
#include <libethcore/Common.h>
#include <libp2p/Host.h>
#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>
#include <iostream>
#include <thread>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

using namespace dev;
using namespace dev::p2p;
using namespace dev::eth;
using namespace std;

namespace
{
string const c_programName = "aleth-bootnode";
string const c_networkConfigFileName = c_programName + "-network.rlp";
}  // namespace

int main(int argc, char** argv)
{
    setDefaultOrCLocale();

    bool allowLocalDiscovery = false;

    po::options_description generalOptions("GENERAL OPTIONS", c_lineWidth);
    auto addGeneralOption = generalOptions.add_options();
    addGeneralOption("help,h", "Show this help message and exit\n");

    LoggingOptions loggingOptions;
    po::options_description loggingProgramOptions(
        createLoggingProgramOptions(c_lineWidth, loggingOptions));

    po::options_description clientNetworking("NETWORKING", c_lineWidth);
    auto addNetworkingOption = clientNetworking.add_options();
#if ETH_MINIUPNPC
    addNetworkingOption(
        "upnp", po::value<string>()->value_name("<on/off>"), "Use UPnP for NAT (default: on)");
#endif
    addNetworkingOption("public-ip", po::value<string>()->value_name("<ip>"),
        "Force advertised public IP to the given IP (default: auto)");
    addNetworkingOption("listen-ip", po::value<string>()->value_name("<ip>(:<port>)"),
        "Listen on the given IP for incoming connections (default: 0.0.0.0)");
    addNetworkingOption("listen", po::value<unsigned short>()->value_name("<port>"),
        "Listen on the given port for incoming connections (default: 30303)");
    addNetworkingOption("allow-local-discovery", po::bool_switch(&allowLocalDiscovery),
        "Include local addresses in the discovery process. Used for testing purposes.");
    po::options_description allowedOptions("Allowed options");
    allowedOptions.add(generalOptions).add(loggingProgramOptions).add(clientNetworking);

    po::variables_map vm;
    try
    {
        po::parsed_options parsed = po::parse_command_line(argc, argv, allowedOptions);
        po::store(parsed, vm);
        po::notify(vm);
    }
    catch (po::error const& e)
    {
        cout << e.what() << "\n";
        return AlethErrors::ArgumentProcessingFailure;
    }

    if (vm.count("help"))
    {
        cout << "NAME:\n"
             << "   " << c_programName << "\n"
             << "USAGE:\n"
             << "   " << c_programName << " [options]\n\n";
        cout << generalOptions << clientNetworking << loggingProgramOptions;
        return AlethErrors::Success;
    }

    /// Networking params.
    string listenIP;
    unsigned short listenPort = c_defaultListenPort;
    string publicIP;
    bool upnp = true;

#if ETH_MINIUPNPC
    if (vm.count("upnp"))
    {
        string m = vm["upnp"].as<string>();
        if (isTrue(m))
            upnp = true;
        else if (isFalse(m))
            upnp = false;
        else
        {
            cerr << "Bad "
                 << "--upnp"
                 << " option: " << m << "\n";
            return -1;
        }
    }
#endif

    if (vm.count("public-ip"))
        publicIP = vm["public-ip"].as<string>();
    if (vm.count("listen-ip"))
        listenIP = vm["listen-ip"].as<string>();
    if (vm.count("listen"))
        listenPort = vm["listen"].as<unsigned short>();
    setupLogging(loggingOptions);
    if (loggingOptions.verbosity > 0)
        cout << EthGrayBold << c_programName << ", a C++ Ethereum bootnode implementation" EthReset
             << "\n";

    auto netPrefs = publicIP.empty() ? NetworkConfig(listenIP, listenPort, upnp) :
                                       NetworkConfig(publicIP, listenIP, listenPort, upnp);
    netPrefs.allowLocalDiscovery = allowLocalDiscovery;
    auto netData = contents(getDataDir() / fs::path(c_networkConfigFileName));

    Host h(c_programName, netPrefs, &netData);
    h.start();

    cout << "Node ID: " << h.enode() << endl;

    ExitHandler exitHandler;
    signal(SIGTERM, &ExitHandler::exitHandler);
    signal(SIGABRT, &ExitHandler::exitHandler);
    signal(SIGINT, &ExitHandler::exitHandler);

    while (!exitHandler.shouldExit())
        this_thread::sleep_for(chrono::milliseconds(1000));

    h.stop();

    netData = h.saveNetwork();
    if (!netData.empty())
        writeFile(getDataDir() / fs::path(c_networkConfigFileName), &netData);

    return AlethErrors::Success;
}
