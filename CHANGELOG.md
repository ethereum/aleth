# Changelog

## [1.9.0] - Unreleased

- Added: [#5868](https://github.com/ethereum/aleth/pull/5868) `test_importRawBlock` RPC method reports the detailed reason when import fails.
- Changed: [#5893](https://github.com/ethereum/aleth/pull/5893) testeth: RlpTests treat empty `out` field as invalid.
- Removed: [#5885](https://github.com/ethereum/aleth/pull/5885) Discontinue `testeth --filltests` support for BlockchainTests, TransitionTests, BCGeneralStateTests.

## [1.8.0] - 2019-12-16

- Added: [#5699](https://github.com/ethereum/aleth/pull/5699) EIP 2046: Reduced gas cost for static calls made to precompiles.
- Added: [#5741](https://github.com/ethereum/aleth/pull/5741) Support for individual EIP activation to facilitate EIP-centric network upgrade process.
- Added: [#5752](https://github.com/ethereum/aleth/pull/5752) [#5753](https://github.com/ethereum/aleth/pull/5753) [#5809](https://github.com/ethereum/aleth/pull/5809) Implement EIP1380 (reduced gas costs for call-to-self).
- Added: [#5859](https://github.com/ethereum/aleth/pull/5859) EIP-2384 Istanbul/Berlin Difficulty Bomb Delay and **Muir Glacier** fork support.
- Changed: [#5750](https://github.com/ethereum/aleth/pull/5750) Use `testeth -t <SUITE_NAME> -- --testfile <PATH>` to run the tests from file at any path. Use `testeth -t <SUITE_NAME> -- --testfile <PATH> --singletest <TEST_NAME>` to run only single test from any file.
- Changed: [#5801](https://github.com/ethereum/aleth/pull/5801) `testeth -t BlockchainTests` command now doesn't run the tests for the forks before Istanbul. To run those tests use a separate LegacyTests suite with command `testeth -t LegacyTests/Constantinople/BlockchainTests`.
- Changed: [#5807](https://github.com/ethereum/aleth/pull/5807) Optimize selfdestruct opcode in LegacyVM by reducing state accesses in certain out-of-gas scenarios.
- Changed: [#5806](https://github.com/ethereum/aleth/pull/5806) Optimize selfdestruct opcode in aleth-interpreter by reducing state accesses in certain out-of-gas scenarios.
- Changed: [#5837](https://github.com/ethereum/aleth/pull/5837) [#5839](https://github.com/ethereum/aleth/pull/5839) [#5845](https://github.com/ethereum/aleth/pull/5845) [#5846](https://github.com/ethereum/aleth/pull/5846) Output format of `testeth --jsontrace` command changed to better match output of geth's evm tool and to integrate with evmlab project.
- Changed: [#5848](https://github.com/ethereum/aleth/pull/5848) `aleth-vm --codefile <PATH>` now reads bytecode file from path and `aleth-vm --codefile - <bytecode>` now reads bytecode from standard input.
- Changed: [#5864](https://github.com/ethereum/aleth/pull/5864) Allow a user to send multiple transactions before a new block is mined.
- Removed: [#5760](https://github.com/ethereum/aleth/pull/5760) Official support for Visual Studio 2015 has been dropped. Compilation with this compiler is expected to stop working after migration to C++14.
- Removed: [#5840](https://github.com/ethereum/aleth/pull/5840) The list of precompiled contracts is not required in config files anymore.
- Removed: [#5850](https://github.com/ethereum/aleth/pull/5850) accounts section is now optional in config files.
- Fixed: [#5792](https://github.com/ethereum/aleth/pull/5792) Faster and cheaper execution of RPC functions which query blockchain state (e.g. getBalance).
- Fixed: [#5811](https://github.com/ethereum/aleth/pull/5811) RPC methods querying transactions (`eth_getTransactionByHash`, `eth_getBlockByNumber`) return correct `v` value.
- Fixed: [#5821](https://github.com/ethereum/aleth/pull/5821) `test_setChainParams` correctly initializes custom configuration of precompiled contracts.
- Fixed: [#5826](https://github.com/ethereum/aleth/pull/5826) Fix blocking bug in database rebuild functionality - users can now rebuild their databases via Aleth's '-R' switch.
- Fixed: [#5827](https://github.com/ethereum/aleth/pull/5827) Detect database upgrades and automatically rebuild the database when they occur.
- Fixed: [#5852](https://github.com/ethereum/aleth/pull/5852) Output correct original opcodes instead of synthetic `PUSHC`/`JUMPC`/`JUMPCI` in VM trace.
- Fixed: [#5829](https://github.com/ethereum/aleth/pull/5829) web3.eth.getBlock now returns block size in bytes. This requires a (automatic) database rebuild which can take a while depending on how many blocks are in the local chain.
- Fixed: [#5866](https://github.com/ethereum/aleth/pull/5866) Update output of `debug_accountRangeAt` and `eth_getTransactionCount` RPC functions to conform to Geth's output.
- Fixed: [#5865](https://github.com/ethereum/aleth/pull/5865) Fix bug which causes syncing to become permanently stuck.

## [1.7.2] - 2019-11-22

- Fixed: [#5834](https://github.com/ethereum/aleth/pull/5834) Fix segmentation fault during sync.
- Fixed: [#5841](https://github.com/ethereum/aleth/pull/5841) Revert the change introduced in 1.7.0 to wait 2 secods after sending `Disconnect` to peer before closing the socket, as it caused instabilty during sync.

## [1.7.1] - 2019-11-18

- Fixed: [#5830](https://github.com/ethereum/aleth/pull/5830) Fix cost of ECADD and ECMULL for Istanbul in Mainnet and Ropsten chain configs.

## [1.7.0] - 2019-11-14

- Added: [#5537](https://github.com/ethereum/aleth/pull/5537) Creating Ethereum Node Record (ENR) at program start.
- Added: [#5571](https://github.com/ethereum/aleth/pull/5571) Support Discovery v4 ENR Extension messages.
- Added: [#5557](https://github.com/ethereum/aleth/pull/5557) Improved debug logging of full sync.
- Added: [#5564](https://github.com/ethereum/aleth/pull/5564) Improved help output of Aleth by adding list of channels.
- Added: [#5575](https://github.com/ethereum/aleth/pull/5575) Log active peer count and peer list every 30 seconds.
- Added: [#5580](https://github.com/ethereum/aleth/pull/5580) Enable syncing from ETC nodes for blocks < dao hard fork block.
- Added: [#5591](https://github.com/ethereum/aleth/pull/5591) Network logging bugfixes and improvements and add p2pcap log channel.
- Added: [#5588](https://github.com/ethereum/aleth/pull/5588) Testeth prints similar test suite name suggestions, when the name passed in `-t` argument is not found.
- Added: [#5593](https://github.com/ethereum/aleth/pull/5593) Dynamically updating host ENR.
- Added: [#5624](https://github.com/ethereum/aleth/pull/5624) Remove useless peers from peer list.
- Added: [#5634](https://github.com/ethereum/aleth/pull/5634) Bootnodes for Rinkeby and Goerli.
- Added: [#5640](https://github.com/ethereum/aleth/pull/5640) Support EIP-1702 Generalized Account Versioning Scheme (active only in Experimental fork.)
- Added: [#5691](https://github.com/ethereum/aleth/pull/5691) Istanbul support: EIP-2028 Transaction data gas cost reduction.
- Added: [#5696](https://github.com/ethereum/aleth/pull/5696) [#5708](https://github.com/ethereum/aleth/pull/5708) Istanbul support: EIP-1344 ChainID opcode.
- Added: [#5700](https://github.com/ethereum/aleth/pull/5700) [#5725](https://github.com/ethereum/aleth/pull/5725) Istanbul support: EIP 1884 Repricing for trie-size-dependent opcodes.
- Added: [#5701](https://github.com/ethereum/aleth/issues/5701) Outputs ENR text representation in admin.nodeInfo RPC.
- Added: [#5705](https://github.com/ethereum/aleth/pull/5705) Istanbul support: EIP 1108 Reduce alt_bn128 precompile gas costs.
- Added: [#5707](https://github.com/ethereum/aleth/pull/5707) Aleth waits for 2 seconds after sending disconnect to peer before closing socket.
- Added: [#5709](https://github.com/ethereum/aleth/pull/5709) [#5728](https://github.com/ethereum/aleth/pull/5728) Istanbul support: EIP-2200 Structured Definitions for Net Gas Metering.
- Added: [#5751](https://github.com/ethereum/aleth/pull/5751) Istanbul support: EIP-152 Add BLAKE2 compression function `F` precompile.
- Added: [#5755](https://github.com/ethereum/aleth/pull/5755) testeth now runs `stChainId`, `stSLoadTest`, `stSelfBalance` tests for Istanbul.
- Added: [#5758](https://github.com/ethereum/aleth/pull/5758) Istanbul support: activation in Ropsten config.
- Added: [#5762](https://github.com/ethereum/aleth/pull/5762) aleth-vm supports `--network Istanbul` option.
- Changed: [#5532](https://github.com/ethereum/aleth/pull/5532) The leveldb is upgraded to 1.22. This is breaking change on Windows and the old databases are not compatible.
- Changed: [#5559](https://github.com/ethereum/aleth/pull/5559) Update peer validation error messages.
- Changed: [#5568](https://github.com/ethereum/aleth/pull/5568) Improve rlpx handshake log messages and create new rlpx log channel.
- Changed: [#5570](https://github.com/ethereum/aleth/pull/5570) Now it's not necessary to recompile with VMTRACE flag to get VM trace log. Just use `testeth -- --vmtrace` or `aleth -v 4 --log-vmtrace`.
- Changed: [#5576](https://github.com/ethereum/aleth/pull/5576) Moved sstore_combinations and static_Call50000_sha256 tests to stTimeConsuming test suite. (testeth runs them only with `--all` flag)
- Changed: [#5589](https://github.com/ethereum/aleth/pull/5589) Make aleth output always line-buffered even when redirected to file or pipe.
- Changed: [#5602](https://github.com/ethereum/aleth/pull/5602) Better predicting external IP address and UDP port.
- Changed: [#5605](https://github.com/ethereum/aleth/pull/5605) Network logging bugfixes and improvements and add warpcap log channel.
- Changed: [#5628](https://github.com/ethereum/aleth/pull/5628) Don't try to endlessly reconnect to official Ethereum bootnodes.
- Changed: [#5632](https://github.com/ethereum/aleth/pull/5632) RocksDB support is disabled by default. Enable with `-DROCKSB=ON` CMake option.
- Changed: [#5648](https://github.com/ethereum/aleth/pull/5648) `BlockChainTests` suite is split into `BlockChainTests/ValidBlocks` and `BlockChainTests/InvalidBlocks`.
- Changed: [#5678](https://github.com/ethereum/aleth/pull/5678) Enable optimizer in aleth-interpreter by default.
- Changed: [#5675](https://github.com/ethereum/aleth/pull/5675) Disconnect from peer when syncing is disabled for peer.
- Changed: [#5676](https://github.com/ethereum/aleth/pull/5676) When receiving large batches of new block hashes, process up to 1024 hashes instead of disabling the peer.
- Changed: [#5719](https://github.com/ethereum/aleth/pull/5719) Enable support for Visual Studio 2017 on Windows.
- Changed: [#5713](https://github.com/ethereum/aleth/pull/5713) Propagate new blocks after PoW check rather than after import into the blockchain.
- Changed: [#5734](https://github.com/ethereum/aleth/pull/5734) debug_accountRangeAt RPC method is renamed to debug_accountRange to conform with geth and retesteth requirements.
- Changed: [#5735](https://github.com/ethereum/aleth/pull/5735) `testeth -t GeneralStateTests` and `testeth -t BCGeneralStateTests` commands now don't run the tests for the forks before Istanbul. To run those tests use a separate `LegacyTests` suite with commands `testeth -t LegacyTests/Constantinople/GeneralStateTests` and `testeth -t LegacyTests/Constantinople/BCGeneralStateTests -- --all`
- Changed: [#5810](https://github.com/ethereum/aleth/pull/5810) [EVMC] has been upgraded to version 7.0.0.
- Removed: [#5631](https://github.com/ethereum/aleth/pull/5631) Removed PARANOID build option.
- Fixed: [#5562](https://github.com/ethereum/aleth/pull/5562) Don't send header request messages to peers that haven't sent us Status yet.
- Fixed: [#5581](https://github.com/ethereum/aleth/pull/5581) Fixed finding neighbour nodes in Discovery.
- Fixed: [#5599](https://github.com/ethereum/aleth/pull/5600) Prevent aleth from attempting concurrent connection to node which results in disconnect of original connection.
- Fixed: [#5609](https://github.com/ethereum/aleth/pull/5609) Log valid local enode-address when external IP is not known.
- Fixed: [#5627](https://github.com/ethereum/aleth/pull/5627) Correct testeth --help log output indentation.
- Fixed: [#5644](https://github.com/ethereum/aleth/pull/5644) Avoid attempting to sync with disconnected peers.
- Fixed: [#5647](https://github.com/ethereum/aleth/pull/5647) test_importRawBlock RPC method correctly fails in case of import failure.
- Fixed: [#5664](https://github.com/ethereum/aleth/pull/5664) Behavior in corner case tests about touching empty Precompiles now agrees with geth's results.
- Fixed: [#5662](https://github.com/ethereum/aleth/pull/5662) Correct depth value when aleth-interpreter invokes `evmc_host_interface::call` callback.
- Fixed: [#5666](https://github.com/ethereum/aleth/pull/5666) aleth-interpreter returns `EVMC_INVALID_INSTRUCTION` when `INVALID` opcode is encountered and `EVMC_UNKNOWN_INSTRUCTION` for undefined opcodes.
- Fixed: [#5706](https://github.com/ethereum/aleth/pull/5706) Stop tracking sent transactions after they've been imported into the blockchain.
- Fixed: [#5687](https://github.com/ethereum/aleth/pull/5687) Limit transaction queue's dropped transaction history to 1024 transactions.
- Fixed: [#5718](https://github.com/ethereum/aleth/pull/5718) Avoid checking contract balance or destination account existence when executing self-destruct operations on Frontier and Homestead.
- Fixed: [#5803](https://github.com/ethereum/aleth/pull/5803) Client version string reported by RPC and devp2p now better matches the format used by other clients. This will allow aleth to be correctly listed on ethernodes.org.

## [1.6.0] - 2019-04-16

- Added: [#5485](https://github.com/ethereum/aleth/pull/5485) aleth-bootnode now by default connects to official Ethereum bootnodes. This can be disabled with `--no-bootstrap` flag.
- Added: [#5505](https://github.com/ethereum/aleth/pull/5505) Allow building with libc++ on Linux.
- Added: [#5514](https://github.com/ethereum/aleth/pull/5514) Improved logging in case of RPC method failures.
- Added: [#5526](https://github.com/ethereum/aleth/pull/5526) Improved logging when loading chain config json containing syntax error.
- Changed: [#5464](https://github.com/ethereum/aleth/pull/5464) Upgrade OS and compilers in the docker image for tests.
- Changed: [#5560](https://github.com/ethereum/aleth/pull/5560) Upgrade [ethash](https://github.com/chfast/ethash) library to version 0.4.4.
- Removed: [#5538](https://github.com/ethereum/aleth/pull/5538) Removed --private flag from aleth command-line arguments.
- Fixed: [#5483](https://github.com/ethereum/aleth/pull/5483) Don't ping the same node more than once in a row; also fixes the assertion failure.
- Fixed: [#5512](https://github.com/ethereum/aleth/pull/5512) Calling `eth_call` without `from` argument.
- Fixed: [#5502](https://github.com/ethereum/aleth/pull/5502) Fix Discovery terminating prematurely because of race condition.
- Fixed: [#5452](https://github.com/ethereum/aleth/pull/5452) Correctly handle Discovery messages when the peer changes public key.
- Fixed: [#5519](https://github.com/ethereum/aleth/pull/5519) Correctly handle Discovery messages with known public key but unknown endpoint.
- Fixed: [#5523](https://github.com/ethereum/aleth/pull/5523) [#5533](https://github.com/ethereum/aleth/pull/5533) Fix syncing terminating prematurely because of race condition.
- Fixed: [#5539](https://github.com/ethereum/aleth/pull/5539) Fix logic for determining if dao hard fork block header should be requested.
- Fixed: [#5547](https://github.com/ethereum/aleth/pull/5547) Fix unnecessary slow-down of eth_flush RPC method.
- Fixed: [#5548](https://github.com/ethereum/aleth/pull/5548) Fix rlp tool for long hexadecimal string inputs.
- Fixed: [#5181](https://github.com/ethereum/aleth/issues/5181) Fix building on PowerPC architecture where -mtune=generic is not available.


[1.9.0]: https://github.com/ethereum/aleth/compare/v1.8.0...master
[1.8.0]: https://github.com/ethereum/aleth/releases/tag/v1.8.0
[1.7.2]: https://github.com/ethereum/aleth/releases/tag/v1.7.2
[1.7.1]: https://github.com/ethereum/aleth/releases/tag/v1.7.1
[1.7.0]: https://github.com/ethereum/aleth/releases/tag/v1.7.0
[1.6.0]: https://github.com/ethereum/aleth/releases/tag/v1.6.0

[EVMC]: https://github.com/ethereum/evmc
