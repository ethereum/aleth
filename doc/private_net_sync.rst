Creating a private network with 2 syncing nodes
===============================================
These instructions cover creating an Ethereum private network consisting of 2 nodes - 1 node will mine blocks and the other node will connect to the first node and sync the mined blocks to its own block database.

Before we start
===============

What is a private network?
=========================
TODO

What is a private chain?
========================
TODO:

Other notes
===========
* Mining will be done using a CPU miner (Aleth doesn't include a GPU miner for various reasons, the main one being the extremely high cost of end-user support - e.g. drivers, TODO: Look @ Pawel's notes in email)
* Only 1 node will be mining because both nodes are running on the same physical system, so having multiple CPU miners will significantly impact system responsiveness - TODO: will running 2 miners be worse than 1 miner since mining is configured by default to use as many threads as possible?
    * TODO: If you'd like to have both nodes mine (or system responsiveness is impacted by 1 miner), you can reduce the number of threads used by each CPU miner via the -t <thread count> flag. 
    * TODO: CPU miner is configured by default to use as many threads as possible - TODO: Does it just use 1 thread per CPU core? It presumably doesn't just create UINT_MAX threads? 
* You typically run a private network using a specialized chain configuration (though this isn't strictly required) in the form of a json file. TODO: more information? 
* Since the json file is used to create the private chain's genesis state, both clients must use the same chain configuration file otherwise they won't be able to connect with each other. TODO:?

Instructions
============
1. Create a key pair for each node
Execute the following command:

aleth.exe account new

Enter the desired password when prompted

2. Add each address (these are the public keys generated in the previous step) that you wish to send transactions with into the "accounts" section of your chain configuration json file (we'll just call this config.json) along with the desired balance (in TODO:?). For example:
TODO:

3. Start the first node by executing the following command (each of the options are explained below):

    Aleth -m on --config <file>  -a <addr> --no-discovery --unsafe-transactions --listen <port> --db-path <path>

○ -m on: Enables CPU mining
○ --config: Path to chain configuration file
○ -a: Sets recipient of mining block reward
○ --no-discovery: Disables the devp2p discovery protocol (it's unnecessary in a 2-node configuration, we'll be adding a peer manually)
○ --unsafe-transactions: Don't prompt before sending each transaction (unnecessary in a testing environment)
*  --listen: TCP port to listen for incoming peer connections. Required so that Aleth will start its devp2p protocol implementation - TODO?
* --db-path: Path to save blocks to. You don't technically need to supply this, but if you've run other syncing nodes on your machine you'll want to make sure you save the private network blocks to a different location (otherwise you can run into TODO)

Make note of the node's enode URL since you'll need to reference it when launching the second node. The enode URL should be logged within the first few lines. For example:

    aleth, a C++ Ethereum client
    INFO  04-01 20:34:38 main net    Id: ##fb867844…
    aleth 1.6.0-alpha.1-28+commit.32bb833e.dirty
    Mining Beneficiary: 84258fde-b0d9-747e-b70f-f55e14831192 - 00fd4aaf9713f5bb664c20a462acc4ebc363d1a6
    INFO  04-01 20:34:40 p2p  info   UPnP device not found.
    WARN  04-01 20:34:40 p2p  warn   "_enabled" parameter is false, discovery is disabled
    Node ID: enode://fb867844056920bbf0dd0945faff8a7a249d33726786ec367461a6c023cae62d7b2bb725a07e2f9832eb05be89e71cf81acf22022215b51a561929c37419531a@0.0.0.0:0
    INFO  04-01 20:34:40 main rpc    JSON-RPC socket path: \\.\pipe\\geth.ipc
    JSONRPC Admin Session Key: 7BPb1cysJuQ=
    INFO  04-01 20:34:40 main client Mining Beneficiary: @00fd4aaf…

If everything goes smoothly you should see the node start mining (empty) blocks after a minute or two:

		C:\Users\nilse\Documents\Code\aleth\build\aleth\Debug>aleth -m on --config %CODE%\config.json -a 00fd4aaf9713f5bb664c20a462acc4ebc363d1a6 --no-discovery --unsafe-transactions --listen 30303 --db-path %APPDATA%\EthereumPrivate_00
		aleth, a C++ Ethereum client
		INFO  04-01 20:38:58 main net    Id: ##5def5843…
		aleth 1.6.0-alpha.1-28+commit.32bb833e.dirty
		Mining Beneficiary: 84258fde-b0d9-747e-b70f-f55e14831192 - 00fd4aaf9713f5bb664c20a462acc4ebc363d1a6
		INFO  04-01 20:38:59 p2p  info   UPnP device not found.
		WARN  04-01 20:38:59 p2p  warn   "_enabled" parameter is false, discovery is disabled
		Node ID: enode://5def584369536c059df3cd86280200beb51829319e4bd1a8bb19df885babe215db30eafa548861b558ae4ac65d546a2d96a5664fade83ba3605c45b6bd88cc51@0.0.0.0:0
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

        Aleth --config <config.json path>  --no-discovery --unsafe-transactions --listen <port number e.g. 30307> --peerset required:<node 1's enode URL> --db-path <path to node 2's block database>

    ○ Notes: 
        § You need to use the same config.json file
        § --listen: Be sure to specify a different port
        § --peerset: Be sure to update the IP address in the enode URL to 127.0.0.1:<node 1 listen port>
        § We've omitted the mining options since this node will purely be a peer node and will not mine new blocks. Feel free to include the mining options if you would like this node to mine as well, though note that this will likely affect system responsiveness
        § You'll need to specify a different --db-path than node 1 since 2 nodes can't share the same block database. 

	So if we want to connect to node 1 we'd execute the following:
		
		C:\Users\nilse\Documents\Code\aleth\build\aleth\Debug>aleth --config %CODE%\config.json --no-discovery --unsafe-transactions --listen 30305 --db-path %APPDATA%\EthereumPrivate_01 --peerset required:enode://5def584369536c059df3cd86280200beb51829319e4bd1a8bb19df885babe215db30eafa548861b558ae4ac65d546a2d96a5664fade83ba3605c45b6bd88cc51@127.0.0.1:30303

    5. If all goes well the second node will connect to the first node and start syncing blocks:

    	5. The second node will connect to the first node and start syncing blocks:
	
		C:\Users\nilse\Documents\Code\aleth\build\aleth\Debug>aleth --config %CODE%\config.json --no-discovery --unsafe-transactions --listen 30305 --db-path %APPDATA%\EthereumPrivate_01 --peerset required:enode://5def584369536c059df3cd86280200beb51829319e4bd1a8bb19df885babe215db30eafa548861b558ae4ac65d546a2d96a5664fade83ba3605c45b6bd88cc51@127.0.0.1:30303
		aleth, a C++ Ethereum client
		INFO  04-01 20:47:55 main net    Id: ##d4a0335d…
		aleth 1.6.0-alpha.1-28+commit.32bb833e.dirty
		Mining Beneficiary: 84258fde-b0d9-747e-b70f-f55e14831192 - 00fd4aaf9713f5bb664c20a462acc4ebc363d1a6
		INFO  04-01 20:47:59 p2p  info   UPnP device not found.
		WARN  04-01 20:47:59 p2p  warn   "_enabled" parameter is false, discovery is disabled
		Node ID: enode://d4a0335d481fe816a7d580a298870066c3c24af60cd1c2875bd2598befedfbd5a43942f41e04f6e92d1081de72843f15ff5fb9c8f65cb31bdce1357514f02491@0.0.0.0:0
		INFO  04-01 20:47:59 main rpc    JSON-RPC socket path: \\.\pipe\\geth.ipc
		JSONRPC Admin Session Key: rtsy5ehS1JA=
		INFO  04-01 20:47:59 p2p  sync   5def5843…|aleth/1.6.0-alpha.1-28+commit.32bb833e.dirty/windows/msvc19.0.24215.1/debug Starting full sync
		INFO  04-01 20:48:01 eth  client 26 blocks imported in 177 ms (146.424 blocks/s) in #26
		INFO  04-01 20:48:02 eth  client 50 blocks imported in 262 ms (190.531 blocks/s) in #76
		INFO  04-01 20:48:02 eth  client 56 blocks imported in 300 ms (186.602 blocks/s) in #132
		INFO  04-01 20:48:02 eth  client 59 blocks imported in 265 ms (222.067 blocks/s) in #191


Common Problems
* Unrecognized peerset    
Unrecognized peerset: required:enode://5def584369536c059df3cd86280200beb51829319e4bd1a8bb19df885babe215db30eafa548861b558ae4ac65d546a2d96a5664fade83ba3605c45b6bd88cc51@0.0.0.0:0

You need to update the IP address in the enode URL to 127.0.0.1:<port> where <port> is the port number you supplied to node 1 via --listen

* Database already open
aleth, a C++ Ethereum client
INFO  04-01 20:50:31 main net    Id: ##a7dbe409…
WARN  04-01 20:50:31 main warn   Database "C:\Users\nilse\AppData\Roaming\EthereumPrivate_00\ddce0f53\blocks"or "C:\Users\nilse\AppData\Roaming\EthereumPrivate_00\ddce0f53\12041\extras"already open. You appear to have another instance of ethereum running. Bailing.

Both of your Aleth nodes are using the same database. You need to set one of your nodes' database path to a different location.

* Node 2 doesn't sync with node 1
This means that node 2 couldn't successfully peer with node 1, typically because you used a different config file for each node. You can get helpful debugging information by enabling verobse logging on node 1 (-v4) - be sure to also filter the log channels to net and sync (--log-channels net sync) otherwise it will be hard to pick out useful information in the logs.

For example, here's what node 1 logs when the nodes use different config files: TODO:

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

* Error: Couldn't start accepting connections on host. Failed to accept socket on <IP address>

This means that you're running both nodes on the same listen port. Be sure to specify different ports via --listen. 

Full error text:

    aleth, a C++ Ethereum client
    INFO  04-01 21:01:18 main net    Id: ##ac459be1…
    aleth 1.6.0-alpha.1-28+commit.32bb833e.dirty
    Mining Beneficiary: 84258fde-b0d9-747e-b70f-f55e14831192 - 00fd4aaf9713f5bb664c20a462acc4ebc363d1a6
    WARN  04-01 21:01:20 p2p  warn   Couldn't start accepting connections on host. Failed to accept socket on 0.0.0.0:30303.
    Throw location unknown (consider using BOOST_THROW_EXCEPTION)
    Dynamic exception type: class boost::exception_detail::clone_impl<struct boost::exception_detail::error_info_injector<class boost::system::system_error> >
    std::exception::what: bind: Only one usage of each socket address (protocol/network address/port) is normally permitted

    INFO  04-01 21:01:20 p2p  info   UPnP device not found.
    WARN  04-01 21:01:20 p2p  warn   "_enabled" parameter is false, discovery is disabled
