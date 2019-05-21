Creating A Private Network With Two Syncing Nodes
==================================================
These instructions cover creating an Ethereum private network consisting of two nodes - one node will mine blocks and the other node will connect to the first node and sync the mined blocks to its own block database. Both nodes will run a private chain configuration.


What is a Private Network?
--------------------------
For the purposes of this documentation, a **private network** can be thought of as a network of Ethereum nodes only accessible on your physical machine.

What is a Private Chain?
-------------------------
An Ethereum chain is some state (e.g. accounts and balances and/or contract code) and a set of rules for interacting with that state.  **Private chain** is used in this documentation to refer to an Ethereum chain whose configuration is only available on your physical machine.

Mining
------
- Mining will be done using the Aleth CPU miner (Aleth doesn't include a GPU miner because of the high maintenance and support costs. Please see `EthMiner <https://github.com/ethereum-mining/ethminer>`_ for GPU mining support).
- We will only start mining for one node to keep your machine responsive.
- If mining slows down your system too much or you'd like to have both nodes mine, you can tune the number of mining threads via ``-t <thread count>``.

Chain Configuration
-------------------
- You typically initialize a private chain using a chain configuration json file (this isn't strictly required, but it makes testing easier since you can do things like lower the difficulty rate and pre-fund addresses).
- Here's an example file:

::

    {
        "sealEngine": "Ethash",
        "params": {
            "accountStartNonce": "0x00",
            "maximumExtraDataSize": "0x20",
            "homesteadForkBlock": "0x00",
            "EIP150ForkBlock": "0x00",
            "EIP158ForkBlock": "0x00",
            "byzantiumForkBlock": "0x00",
            "constantinopleForkBlock": "0x00",
            "constantinopleFixForkBlock": "0x00",
            "minGasLimit": "0x5208",
            "maxGasLimit": "0x7fffffffffffffff",
            "tieBreakingGas": false,
            "gasLimitBoundDivisor": "0x0400",
            "minimumDifficulty": "0x100000",
            "difficultyBoundDivisor": "0x0800",
            "durationLimit": "0x0d",
            "blockReward": "0x4563918244F40000",
            "networkID" : "0x42",
            "chainID": "0x42",
            "allowFutureBlocks" : false
        },
        "genesis": {
            "nonce": "0x0000000000000042",
            "difficulty": "0x100000",
            "mixHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
            "author": "0x0000000000000000000000000000000000000000",
            "timestamp": "0x5A5B92D7",
            "parentHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
            "extraData": "0x655741534d2074657374206e6574776f726b2030",
            "gasLimit": "0x989680"
        },
        "accounts": {
            "0000000000000000000000000000000000000001": { "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 } }, "balance": "0x01" },
            "0000000000000000000000000000000000000002": { "precompiled": { "name": "sha256", "linear": { "base": 60, "word": 12 } }, "balance": "0x01" },
            "0000000000000000000000000000000000000003": { "precompiled": { "name": "ripemd160", "linear": { "base": 600, "word": 120 } }, "balance": "0x01" },
            "0000000000000000000000000000000000000004": { "precompiled": { "name": "identity", "linear": { "base": 15, "word": 3 } }, "balance": "0x01" },
            "0000000000000000000000000000000000000005": { "precompiled": { "name": "modexp" }, "balance": "0x01" },
            "0000000000000000000000000000000000000006": { "precompiled": { "name": "alt_bn128_G1_add", "linear": { "base": 500, "word": 0 } }, "balance": "0x01" },
            "0000000000000000000000000000000000000007": { "precompiled": { "name": "alt_bn128_G1_mul", "linear": { "base": 40000, "word": 0 } }, "balance": "0x01" },
            "0000000000000000000000000000000000000008": { "precompiled": { "name": "alt_bn128_pairing_product" }, "balance": "0x01" },
            "00fd4aaf9713f5bb664c20a462acc4ebc363d1a6": { "balance" : "0x200000000000000000000000000000000000000000000000000000000000000" },
            "004d5baf9ab6a9c29c73725ec35023e330af2bc8": { "balance" : "0x200000000000000000000000000000000000000000000000000000000000000" }
        }
    }

- **Both nodes must use the same chain configuration file:** The chain configuration is used to create the chain's genesis state, so using a different configuration for each node means that they won't be able to peer with each other. An example of this is shown in the `Common Problems`_ section.

Instructions
============
*The output in this section was generated on Windows 10 running aleth 1.6.0-alpha.1-31+commit.ad7204c9*

1. Create a key pair for each node:

::

    aleth.exe account new


Enter the desired password when prompted

**Example:**
::

    aleth account new
    [2019-04-02 19:59:42.515684] [0x000041f0] [info]    ReadingC:\Users\nilse\AppData\Roaming\Web3\keys\4f04a5ed-87e4-1e4d-4367-604db42bdcff.json
    [2019-04-02 19:59:42.520265] [0x000041f0] [info]    ReadingC:\Users\nilse\AppData\Roaming\Web3\keys\84258fde-b0d9-747e-b70f-f55e14831192.json
    [2019-04-02 19:59:42.520265] [0x000041f0] [info]    ReadingC:\Users\nilse\AppData\Roaming\Web3\keys\9067d973-1c8d-fa86-a312-14c90188f610.json
    Enter a passphrase with which to secure this account:Please confirm the passphrase by entering it again: Created key 623b80dd-d008-4cd4-dd06-c36f0f64296c
    Address: 007e13502a8b226b0f19e7412db75352dc1d0760

    aleth account new
    [2019-04-02 19:59:53.847216] [0x000011b4] [info]    ReadingC:\Users\nilse\AppData\Roaming\Web3\keys\4f04a5ed-87e4-1e4d-4367-604db42bdcff.json
    [2019-04-02 19:59:53.849343] [0x000011b4] [info]    ReadingC:\Users\nilse\AppData\Roaming\Web3\keys\623b80dd-d008-4cd4-dd06-c36f0f64296c.json
    [2019-04-02 19:59:53.850400] [0x000011b4] [info]    ReadingC:\Users\nilse\AppData\Roaming\Web3\keys\84258fde-b0d9-747e-b70f-f55e14831192.json
    [2019-04-02 19:59:53.850400] [0x000011b4] [info]    ReadingC:\Users\nilse\AppData\Roaming\Web3\keys\9067d973-1c8d-fa86-a312-14c90188f610.json
    Enter a passphrase with which to secure this account:Please confirm the passphrase by entering it again: Created key ab921356-8c9e-54ff-e3e7-da5c2f7aa685
    Address: 002c73acd4bc217998966964d27f0ee79a47befb


2. Add each address generated in the previous step to the ``accounts`` section of your chain configuration file (we'll refer to this as ``config.json``) along with the desired balance in wei. For example, the following initializes each account with 2 ether (2000000000000000000 wei):

::

    "accounts": {
        "0000000000000000000000000000000000000001": { "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 } }, "balance": "0x01" },
        "0000000000000000000000000000000000000002": { "precompiled": { "name": "sha256", "linear": { "base": 60, "word": 12 } }, "balance": "0x01" },
        "0000000000000000000000000000000000000003": { "precompiled": { "name": "ripemd160", "linear": { "base": 600, "word": 120 } }, "balance": "0x01" },
        "0000000000000000000000000000000000000004": { "precompiled": { "name": "identity", "linear": { "base": 15, "word": 3 } }, "balance": "0x01" },
        "0000000000000000000000000000000000000005": { "precompiled": { "name": "modexp" }, "balance": "0x01" },
        "0000000000000000000000000000000000000006": { "precompiled": { "name": "alt_bn128_G1_add", "linear": { "base": 500, "word": 0 } }, "balance": "0x01" },
        "0000000000000000000000000000000000000007": { "precompiled": { "name": "alt_bn128_G1_mul", "linear": { "base": 40000, "word": 0 } }, "balance": "0x01" },
        "0000000000000000000000000000000000000008": { "precompiled": { "name": "alt_bn128_pairing_product" }, "balance": "0x01" },
        "007e13502a8b226b0f19e7412db75352dc1d0760": {
            "balance" : "2000000000000000000"
        },
        "002c73acd4bc217998966964d27f0ee79a47befb": {
            "balance" : "2000000000000000000"
        }
    }


3. Start the first node:

::

    aleth -m on --config <file> -a <addr> --no-discovery --unsafe-transactions --listen <port>

-m on                       Enable CPU mining
--config                    Set chain configuration
-a                          Set recipient of mining block reward
--no-discovery              Disable devp2p discovery protocol (it's unnecessary in a two-node network, we'll be adding a peer manually)
--unsafe-transactions       Don't require approval before sending each transaction (unnecessary for testing purposes)
--listen                    Set TCP port to listen on for incoming peer connections


**Example:**

::

    aleth -m on --config config.json -a 00fd4aaf9713f5bb664c20a462acc4ebc363d1a6 --no-discovery --unsafe-transactions --listen 30303


Make note of the node's URL (which starts with ``enode://``) since you'll need to supply it when starting the second node. The URL should be logged within the first few lines of console output.

**Example:**

::

    aleth, a C++ Ethereum client
    INFO  04-01 20:34:38 main net    Id: ##fb867844…
    aleth 1.6.0-alpha.1-28+commit.32bb833e.dirty
    Mining Beneficiary: 84258fde-b0d9-747e-b70f-f55e14831192 - 00fd4aaf9713f5bb664c20a462acc4ebc363d1a6
    INFO  04-01 20:34:40 p2p  info   UPnP device not found.
    WARN  04-01 20:34:40 p2p  warn   "_enabled" parameter is false, discovery is disabled
    Node ID: enode://fb867844056920bbf0dd0945faff8a7a249d33726786ec367461a6c023cae62d7b2bb725a07e2f9832eb05be89e71cf81acf22022215b51a561929c37419531a@127.0.0.1:30303


The node should start mining blocks after a minute or two:

::

    INFO  04-01 20:38:59 main rpc    JSON-RPC socket path: \\.\pipe\\geth.ipc
    JSONRPC Admin Session Key: 2C/gbvE/pxQ=
    INFO  04-01 20:38:59 main client Mining Beneficiary: @00fd4aaf…
    INFO  04-01 20:40:36 miner2 client Block sealed #1
    INFO  04-01 20:40:36 eth  client Tried to seal sealed block...
    INFO  04-01 20:40:36 eth  client 1 blocks imported in 1 ms (515.198 blocks/s) in #1
    INFO  04-01 20:40:37 miner0 client Block sealed #2
    INFO  04-01 20:40:37 eth  client 1 blocks imported in 3 ms (316.056 blocks/s) in #2
    INFO  04-01 20:40:39 miner1 client Block sealed #3
    INFO  04-01 20:40:39 eth  client 1 blocks imported in 3 ms (300.842 blocks/s) in #3


4. Start the second node:

::

    aleth --config <file> --no-discovery --unsafe-transactions --listen <port> --peerset required:<url> --db-path <path>

--config        Specify the same chain config file
--listen        Specify a different port
--peerset       Specify URL of the first node
--db-path       Path to save sync'd blocks. Aleth saves blocks by default to ``%APPDATA%\Ethereum`` on Windows and ``$HOME/.ethereum`` on Linux. You need to specify a different path for your second node otherwise you'll run into database access issues. See the `Common Problems`_ section for an example of this error.


**Example:**

::

    aleth --config config.json --no-discovery --unsafe-transactions --listen 30305 --db-path %APPDATA%\EthereumPrivate_01 --peerset required:enode://fb867844056920bbf0dd0945faff8a7a249d33726786ec367461a6c023cae62d7b2bb725a07e2f9832eb05be89e71cf81acf22022215b51a561929c37419531a@127.0.0.1:30303


5. The node should connect to the first node and start syncing blocks:

::

    aleth, a C++ Ethereum client
    INFO  04-01 20:47:55 main net    Id: ##d4a0335d…
    aleth 1.6.0-alpha.1-28+commit.32bb833e.dirty
    Mining Beneficiary: 84258fde-b0d9-747e-b70f-f55e14831192 - 00fd4aaf9713f5bb664c20a462acc4ebc363d1a6
    INFO  04-01 20:47:59 p2p  info   UPnP device not found.
    WARN  04-01 20:47:59 p2p  warn   "_enabled" parameter is false, discovery is disabled
    Node ID: enode://d4a0335d481fe816a7d580a298870066c3c24af60cd1c2875bd2598befedfbd5a43942f41e04f6e92d1081de72843f15ff5fb9c8f65cb31bdce1357514f02491@127.0.0.1:30305
    INFO  04-01 20:47:59 main rpc    JSON-RPC socket path: \\.\pipe\\geth.ipc
    JSONRPC Admin Session Key: rtsy5ehS1JA=
    INFO  04-01 20:47:59 p2p  sync   5def5843…|aleth/1.6.0-alpha.1-28+commit.32bb833e.dirty/windows/msvc19.0.24215.1/debug Starting full sync
    INFO  04-01 20:48:01 eth  client 26 blocks imported in 177 ms (146.424 blocks/s) in #26
    INFO  04-01 20:48:02 eth  client 50 blocks imported in 262 ms (190.531 blocks/s) in #76
    INFO  04-01 20:48:02 eth  client 56 blocks imported in 300 ms (186.602 blocks/s) in #132
    INFO  04-01 20:48:02 eth  client 59 blocks imported in 265 ms (222.067 blocks/s) in #191


Common Problems
===============
"Unrecognized peerset" error
---------------------------------
**Example:**

::

    Unrecognized peerset: required:enode://5def584369536c059df3cd86280200beb51829319e4bd1a8bb19df885babe215db30eafa548861b558ae4ac65d546a2d96a5664fade83ba3605c45b6bd88cc51@0.0.0.0:0


Your ``peerset`` string is formatted incorrectly. You probably need to update the IP address in the node URL to ``127.0.0.1:<port>``, where ``<port>`` is the port number you supplied to node 1 via ``--listen``.

"Database already open" error
-------------------------------
**Example:**

::

    aleth, a C++ Ethereum client
    INFO  04-01 20:50:31 main net    Id: ##a7dbe409…
    WARN  04-01 20:50:31 main warn   Database "C:\Users\nilse\AppData\Roaming\EthereumPrivate_00\ddce0f53\blocks"or "C:\Users\nilse\AppData\Roaming\EthereumPrivate_00\ddce0f53\12041\extras"already open. You appear to have another instance of ethereum running. Bailing.


Both of your Aleth nodes are trying to use the same location for their block databases. Set one of your nodes' database paths to a different location (``--db-path``).


Node 2 doesn't sync with node 1
-------------------------------
This means that node 2 couldn't successfully peer with node 1. A possible reason for this when running a private network is using a different chain config for each node. To see if this is the issue, enable verbose logging on node 1 (``-v4 --log-channels net sync``) to get helpful logs for debugging.

**Example**: Here are the node 1 logs when node 1 and node 2 use different chain configuration files (note the ``Peer not suitable for sync: Invalid genesis hash.`` error):

::

    TRACE 04-01 20:57:53 p2p  net    p2p.connect.ingress receiving auth from 127.0.0.1:61309
    TRACE 04-01 20:57:53 p2p  net    Listening on local port 30303
    TRACE 04-01 20:57:53 p2p  net    p2p.connect.ingress sending ack to 127.0.0.1:61309
    TRACE 04-01 20:57:53 p2p  net    p2p.connect.ingress sending capabilities handshake
    TRACE 04-01 20:57:53 p2p  net    p2p.connect.ingress recvd hello header
    TRACE 04-01 20:57:53 p2p  net    p2p.connect.ingress hello frame: success. starting session.
    DEBUG 04-01 20:57:53 p2p  net    Hello: aleth/1.6.0-alpha.1-28+commit.32bb833e.dirty/windows/msvc19.0.24215.1/debug V[4] ##8b7b78e1… (eth,63) 30305
    DEBUG 04-01 20:57:53 p2p  net    New session for capability eth; idOffset: 16
    TRACE 04-01 20:57:53 p2p  net    <- [ 0x3F, 0x42, 0x179D6F06, 0x9A610A1C26FFF584E79421406D77ABF46E9FDE72E11D2F6E8B880D3F5E84EDE8, 0xDDCE0F53ABB8348FDF758C4DABBD9C0A7BBD359CBE6E74AC60A2F12F6B9BAA74 ]
    TRACE 04-01 20:57:53 p2p  net    <- [ ]
    DEBUG 04-01 20:57:53 p2p  net    p2p.host.peer.register ##8b7b78e1…
    TRACE 04-01 20:57:53 p2p  net    8b7b78e1…|aleth/1.6.0-alpha.1-28+commit.32bb833e.dirty/windows/msvc19.0.24215.1/debug Error reading: An established connection was aborted by the software in your host machine
    TRACE 04-01 20:57:53 p2p  net    8b7b78e1…|aleth/1.6.0-alpha.1-28+commit.32bb833e.dirty/windows/msvc19.0.24215.1/debug Closing 127.0.0.1:61309 (Low-level TCP communication error.)
    DEBUG 04-01 20:57:53 p2p  net    8b7b78e1…|aleth/1.6.0-alpha.1-28+commit.32bb833e.dirty/windows/msvc19.0.24215.1/debug Closing peer session :-(
    TRACE 04-01 20:57:58 p2p  net    p2p.connect.ingress receiving auth from 127.0.0.1:61323
    TRACE 04-01 20:57:58 p2p  net    Listening on local port 30303
    TRACE 04-01 20:57:58 p2p  net    p2p.connect.ingress sending ack to 127.0.0.1:61323
    TRACE 04-01 20:57:58 p2p  net    p2p.connect.ingress sending capabilities handshake
    TRACE 04-01 20:57:58 p2p  net    p2p.connect.ingress recvd hello header
    TRACE 04-01 20:57:58 p2p  net    p2p.connect.ingress hello frame: success. starting session.
    DEBUG 04-01 20:57:58 p2p  net    Hello: aleth/1.6.0-alpha.1-28+commit.32bb833e.dirty/windows/msvc19.0.24215.1/debug V[4] ##8b7b78e1… (eth,63) 30305
    DEBUG 04-01 20:57:58 p2p  net    New session for capability eth; idOffset: 16
    TRACE 04-01 20:57:58 p2p  net    <- [ 0x3F, 0x42, 0x179D6F06, 0x9A610A1C26FFF584E79421406D77ABF46E9FDE72E11D2F6E8B880D3F5E84EDE8, 0xDDCE0F53ABB8348FDF758C4DABBD9C0A7BBD359CBE6E74AC60A2F12F6B9BAA74 ]
    TRACE 04-01 20:57:58 p2p  net    <- [ ]
    DEBUG 04-01 20:57:58 p2p  net    p2p.host.peer.register ##8b7b78e1…
    TRACE 04-01 20:57:58 p2p  net    8b7b78e1…|aleth/1.6.0-alpha.1-28+commit.32bb833e.dirty/windows/msvc19.0.24215.1/debug -> 16 [ 0x3F, 0x42, 0x100000, 0xD8600904A41043A4E81D23863F178E7DC8B3C2CBAFA94EB4BBF5DC46BCCCE176, 0xD8600904A41043A4E81D23863F178E7DC8B3C2CBAFA94EB4BBF5DC46BCCCE176 ]
    DEBUG 04-01 20:57:58 p2p  sync   8b7b78e1…|aleth/1.6.0-alpha.1-28+commit.32bb833e.dirty/windows/msvc19.0.24215.1/debug Peer not suitable for sync: Invalid genesis hash.
    TRACE 04-01 20:57:58 p2p  net    8b7b78e1…|aleth/1.6.0-alpha.1-28+commit.32bb833e.dirty/windows/msvc19.0.24215.1/debug Disconnecting (our reason: Subprotocol reason.)
    TRACE 04-01 20:57:58 p2p  net    8b7b78e1…|aleth/1.6.0-alpha.1-28+commit.32bb833e.dirty/windows/msvc19.0.24215.1/debug <- [ 0x10 ]
    TRACE 04-01 20:57:58 p2p  net    8b7b78e1…|aleth/1.6.0-alpha.1-28+commit.32bb833e.dirty/windows/msvc19.0.24215.1/debug Closing 127.0.0.1:61323 (Subprotocol reason.)
    DEBUG 04-01 20:57:58 p2p  net    8b7b78e1…|aleth/1.6.0-alpha.1-28+commit.32bb833e.dirty/windows/msvc19.0.24215.1/debug Closing peer session :-(


"Couldn't start accepting connections on host. Failed to accept socket on <IP address>" error
--------------------------------------------------------------------------------------------------
**Example:**

::

    aleth, a C++ Ethereum client
    INFO  04-01 21:01:18 main net    Id: ##ac459be1…
    aleth 1.6.0-alpha.1-28+commit.32bb833e.dirty
    Mining Beneficiary: 84258fde-b0d9-747e-b70f-f55e14831192 - 00fd4aaf9713f5bb664c20a462acc4ebc363d1a6
    WARN  04-01 21:01:20 p2p  warn   Couldn't start accepting connections on host. Failed to accept socket on 0.0.0.0:30303.
    Throw location unknown (consider using BOOST_THROW_EXCEPTION)
    Dynamic exception type: class boost::exception_detail::clone_impl<struct boost::exception_detail::error_info_injector<class boost::system::system_error> >
    std::exception::what: bind: Only one usage of each socket address (protocol/network address/port) is normally permitted

You're running both nodes on the same listen port. Specify different ports when launching each node via ``--listen``.

