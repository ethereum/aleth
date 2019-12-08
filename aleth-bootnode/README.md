# Aleth-bootnode, a C++ Ethereum discovery bootnode implementation
> A small, low memory usage C++ application which runs the Ethereum devp2p discovery protocol.

Aleth-bootnode is an application which runs the Ethereum devp2p discovery protocol (v4), which is the protocol that all Ethereum nodes use to find each other so they can communicate and do things like exchange blocks and transactions. You can read more about the discovery protocol here: https://github.com/ethereum/devp2p/wiki/Discovery-Overview

Aleth-bootnode doesn't support syncing (it doesn't understand the devp2p/RLPx TCP-based transport protocols such as the [Ethereum wire protocol](https://github.com/ethereum/wiki/wiki/Ethereum-Wire-Protocol)) and doesn't interact with a blockchain so it's small  (the binary size is < 2 MB) and has very low resource usage (typically < 5 MB of private working set memory).

By default, Aleth-bootnode will connect to the official Ethereum bootnodes and participate in the discovery process. You can also use Aleth-bootnode to run your own private network discovery server (so your own Ethereum nodes can find each other without being visible to Ethereum nodes outside of your network) via the `--no-bootstrap` option and manually adding the Aleth-bootnode enode URL to your Ethereum nodes.

## Install
Aleth-bootnode is a part of the [Aleth project](https://github.com/ethereum/aleth) and binaries are included in Aleth releases, so please see the [Aleth README](https://github.com/ethereum/aleth/blob/master/README.md) for instructions on building from source, downloading release binaries, and more.

## Usage
The following is the output of `aleth-bootnode.exe -h` on Windows:
```
NAME:
    aleth-bootnode
USAGE:
    aleth-bootnode [options]

GENERAL OPTIONS:
    -h [ --help ]         Show this help message and exit

NETWORKING:
    --public-ip <ip>          Force advertised public IP to the given IP (default: auto)
    --listen-ip <ip>(:<port>) Listen on the given IP for incoming connections (default: 0.0.0.0)
    --listen <port>           Listen on the given port for incoming connections (default: 30303)
    --allow-local-discovery   Include local addresses in the discovery process. Used for testing purposes.
    --no-bootstrap            Do not connect to the default Ethereum bootnode servers
LOGGING OPTIONS:
    -v [ --log-verbosity ] <0 - 4>        Set the log verbosity from 0 to 4 (default: 2).
    --log-channels <channel_list>         Space-separated list of the log channels to show (default: show all channels).
    --log-exclude-channels <channel_list> Space-separated list of the log channels to hide.
  ```

## Contributing
See the [Contribute section of the Aleth README for details](https://github.com/ethereum/aleth/blob/master/README.md#Contribute).

## License
Aleth-bootnode is a part of the Aleth project and covered by it's license. Please see the [License section of the Aleth README for details](https://github.com/ethereum/aleth/blob/master/README.md#License).