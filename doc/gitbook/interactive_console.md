# eth Interactive Console

In order to interact with the client you have two options. If you have no client running, you can start one and provide the `-i` argument, which will start a client with the interactive console. Alternatively, if you already have a client running, and you started it with the `-j` flag, you can just run `ethconsole`, which will give you exactly the same environment without running a second client.

The interactive console is a javascript console which contains a subset of the [JSON-RPC api](https://github.com/ethereum/wiki/wiki/JSON-RPC) plus some administration functions. To see all available functions type `web3` in the console prompt and press enter. For only the administrative functions type `web3.admin`.

- [Network connectivity](interactive_console.md#network-connectivity)
- [Mining](interactive_console.md#mining)
- [Miscellaneous administration](interactive_console.md#miscellaneous-administration)

##Network connectivity

### Querying network information

To figure out if you have any peers and if you are properly connected to the network you can type `web3.net`. This will conveniently provide something like:

```
> web3.net
{
  listening: true,
  getListening: [Function],
  peerCount: 2,
  getPeerCount: [Function]
}
```

To query any individual value you can just call it. For example:
```
> web3.net.peerCount
2
```

### Interacting with the network

If you would like to interact with the network you can use the network admin functions. You can query them with `web3.admin.net`.

```
> web3.admin.net
{
  start: [Function],
  stop: [Function],
  connect: [Function],
  peers: [Function],
  nodeInfo: [Function]
}
```

##### Starting the network

If the client is not connected to the network you can start listening with `web3.admin.net.start()`


```
> web3.net.listening
false
> web3.admin.net.start()
 ⚡   12:52:24|p2p  Worker stopping 17126 ms
 ⚡   12:52:24|ethsync  Worker stopping 17146 ms
  ℹ  12:52:24|p2p  UPnP device not found.
> true
> web3.net.listening
true
```

#### Stopping the network

In the same spirit you can stop listening with `web3.admin.net.stop()`


```
> web3.net.listening
true
> web3.admin.net.stop()
true
> web3.net.listening
false
> 
```

#### Getting a list of peers

If you would like to obtain a list with information on the peers you are connected to you can use `web3.admin.net.peers()`.

```
> web3.admin.net.peers()
[{
  caps: {
    eth: 61
  },
  clientVersion: 'Geth/v1.0.2-4591ae56/linux/go1.4.2',
  host: '52.16.188.185',
  id: 'a979fb575495b8d6db44f750317d0f4622bf4c2aa3365d6af7c284339968eef29b69ad0dce72a4d8db5ebb4968de0e3bec910127f134779fbcb0cb6d3331163c',
  lastPing: 41,
  notes: {
    ask: 'nothing',
    manners: 'nice',
    sync: 'ongoing'
  },
  port: 0
}, {
  caps: {
    eth: 61
  },
  clientVersion: '++eth-v0.9.40-a1e4483e/Gav's Node//RelWithDebInfo-Linux/g++/int',
  host: '92.51.165.126',
  id: '5374c1bff8df923d3706357eeb4983cd29a63be40a269aaa2296ee5f3b2119a8978c0ed68b8f6fc84aad0df18790417daadf91a4bfbb786a16c9b0a199fa254a',
  lastPing: 18,
  notes: {
    ask: 'nothing',
    manners: 'nice',
    sync: 'holding'
  },
  port: 30300
}]
> 
```

#### Getting your node information

To obtain your node ID along with other information acout your node's address in the network then use `web3.admin.net.nodeInfo()`


```
> web3.admin.net.nodeInfo()
{
  address: '209.131.41.48',
  enode: 'enode://5bf4613faca50a0ff181915b2d8e5f0a87c82ed5a57dabc9812937bdacb167cf1420652930143a743d6238e0279bd91862c72f9d1d0cbb73b86c4ca1cf966432@209.131.41.48:30303',
  id: '5bf4613faca50a0ff181915b2d8e5f0a87c82ed5a57dabc9812937bdacb167cf1420652930143a743d6238e0279bd91862c72f9d1d0cbb73b86c4ca1cf966432',
  listenAddr: '209.131.41.48:30303',
  name: '++eth-v0.9.41-cb61d09d/Lefteris'\ node//RelWithDebInfo-Linux/g++/int',
  port: 30303
}
```

#### Connecting to other nodes

Sometimes, peer discovery may not work properly, or you may want to connect to a particular node in the network. In those cases you can use the `web3.admin.net.connect()` function to manually connect to a peer.


```
> web3.admin.net.connect("5.1.83.226:30303")                                                                                                                                                      
true
```


If the above was succesfull we can see our new peer in the list:

```
> web3.admin.net.peers()
[{
  caps: {
    eth: 61
  },
  clientVersion: '++eth-v0.9.40-7faadaf4/EthDEV Frontier//RelWithDebInfo-Linux/g++/JIT',
  host: '5.1.83.226',
  id: '979b7fa28feeb35a4741660a16076f1943202cb72b6af70d327f053e248bab9ba81760f39d0701ef1d8f89cc1fbd2cacba0710a12cd5314d5e0c9021aa3637f9',
  lastPing: 31,
  notes: {
    ask: 'nothing',
    manners: 'nice',
    sync: 'holding'
  },
  port: 30303
}, {
  caps: {
    eth: 61
  },
  clientVersion: 'Geth/v1.0.2-4591ae56/linux/go1.4.2',
  host: '52.16.188.185',
  id: 'a979fb575495b8d6db44f750317d0f4622bf4c2aa3365d6af7c284339968eef29b69ad0dce72a4d8db5ebb4968de0e3bec910127f134779fbcb0cb6d3331163c',
  lastPing: 41,
  notes: {
    ask: 'nothing',
    manners: 'nice',
    sync: 'ongoing'
  },
  port: 0
}, {
  caps: {
    eth: 61
  },
  clientVersion: '++eth-v0.9.40-a1e4483e/Gav's Node//RelWithDebInfo-Linux/g++/int',
  host: '92.51.165.126',
  id: '5374c1bff8df923d3706357eeb4983cd29a63be40a269aaa2296ee5f3b2119a8978c0ed68b8f6fc84aad0df18790417daadf91a4bfbb786a16c9b0a199fa254a',
  lastPing: 17,
  notes: {
    ask: 'nothing',
    manners: 'nice',
    sync: 'ongoing'
  },
  port: 30300
}]
```

## Mining

You can also start and stop mining using the interactive console. To start mining use `web3.admin.eth.setMining(true)`

```
> web3.admin.eth.setMining(true)                                                                                                                                                                                                                                              
  ℹ  13:48:01|miner0  Loading full DAG of seedhash: #b903bd76…
> web3.admin.eth.setMining(ttrue
DAG  13:48:01|miner0  Generating DAG file. Progress: 0 %
DAG  13:48:04|miner0  Generating DAG file. Progress: 1 %
DAG  13:48:07|miner0  Generating DAG file. Progress: 2 %
 ⚡   13:48:08|eth  Stop worker 249 ms
 ⚡   13:48:09|eth  Stop worker 479 ms
 ⚡   13:48:09|eth  pause 480 ms
 ⚡   13:48:09|eth  Stop worker 480 ms
 ⚡   13:48:09|eth  pause 480 ms

```

Then again to stop mining simply invoke `web3.admin.eth.setMining(false)`

## Miscellaneous administration

#### Exiting the client

You can exit the client with a `Ctrl-C` signal but you can also use `web3.admin.exit()`

```
> web3.admin.exit()
true
 ⚡   13:36:50|eth  Stop worker 510 ms
  ℹ  13:36:50|eth  Closing blockchain DB
  ℹ  13:36:50|eth  Closing state DB
 ⚡   13:36:50|ethsync  Worker stopping 582 ms
 ⚡   13:36:50|p2p  Worker stopping 581 ms
lefteris@archlenovo ~/ew/cpp-ethereum$
```

#### Changing the log verbosity

If you would like to see more log messages you can change the log verbosity by `web3.admin.setVerbosity()`. This function takes a numeric argument from 0 to 99.

```
> web3.admin.setVerbosity(4)
true
⧎ ◌  13:40:21|p2p|17417d7b…|Geth/v1.0.1-99216f4a/linux/go1.4.2  GetBlockHashesByNumber ( 119085 - 119596 )
⧎ ◌  13:40:21|p2p|0c88df87…|Geth/Siberia19/v1.0.2-9fb7bc74/linux/go1.4.2  GetBlockHashesByNumber ( 116890 - 116890 )
⧎ ◌  13:40:21|p2p|17417d7b…|Geth/v1.0.1-99216f4a/linux/go1.4.2  GetBlocks ( 16 entries)
⧎ ◌  13:40:21|p2p|17417d7b…|Geth/v1.0.1-99216f4a/linux/go1.4.2  16 blocks known and returned; 0 blocks unknown; 0 blocks ignored
⧎ ◌  13:40:22|p2p|0c88df87…|Geth/Siberia19/v1.0.2-9fb7bc74/linux/go1.4.2  GetBlockHashesByNumber ( 116430 - 116430 )
⧎ ◌  13:40:22|p2p|0c88df87…|Geth/Siberia19/v1.0.2-9fb7bc74/linux/go1.4.2  GetBlockHashesByNumber ( 116660 - 116660 )
⧎ ◌  13:40:22|p2p|0c88df87…|Geth/Siberia19/v1.0.2-9fb7bc74/linux/go1.4.2  GetBlockHashesByNumber ( 116545 - 116545 )
⧎ ◌  13:40:22|p2p|0c88df87…|Geth/Siberia19/v1.0.2-9fb7bc74/linux/go1.4.2  GetBlockHashesByNumber ( 116487 - 116487 )
⧎ ◌  13:40:22|p2p|0c88df87…|Geth/Siberia19/v1.0.2-9fb7bc74/linux/go1.4.2  GetBlockHashesByNumber ( 116516 - 116516 )
⧎ ◌  13:40:22|p2p|0c88df87…|Geth/Siberia19/v1.0.2-9fb7bc74/linux/go1.4.2  GetBlockHashesByNumber ( 116530 - 116530 )
⧎ ◌  13:40:23|p2p|0c88df87…|Geth/Siberia19/v1.0.2-9fb7bc74/linux/go1.4.2  GetBlockHashesByNumber ( 116523 - 116523 )
⧎ ◌  13:40:23|p2p|0c88df87…|Geth/Siberia19/v1.0.2-9fb7bc74/linux/go1.4.2  GetBlockHashesByNumber ( 116526 - 116526 )
⧎ ◌  13:40:23|p2p|0c88df87…|Geth/Siberia19/v1.0.2-9fb7bc74/linux/go1.4.2  GetBlockHashesByNumber ( 116524 - 116524 )
⧎ ◌  13:40:23|p2p|0c88df87…|Geth/Siberia19/v1.0.2-9fb7bc74/linux/go1.4.2  GetBlockHashesByNumber ( 116525 - 116525 )
⧎ ◌  13:40:23|p2p|0c88df87…|Geth/Siberia19/v1.0.2-9fb7bc74/linux/go1.4.2  GetBlockHashesByNumber ( 116525 - 117036 )
⧎ ◌  13:40:23|p2p  Hello: Geth/v1.0.1/linux/go1.4.2 V[ 4 ] ##55581d43… (eth,61) 0
⧎ ◌  13:40:23|p2p|55581d43…|Geth/v1.0.1/linux/go1.4.2  Status: 61 / 1 / #95e83250… , TD: 582573549 = #a4f70ef1…
⧎ ◌  13:40:23|p2p|55581d43…|Geth/v1.0.1/linux/go1.4.2  Disconnect (reason: Unknown reason. )
⧎ ◌  13:40:23|p2p|55581d43…|Geth/v1.0.1/linux/go1.4.2  Closing peer session :-(
```

For a healthy logging level use the value of 1.
