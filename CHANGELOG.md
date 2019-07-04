# Changelog

## [1.7.0] - Unreleased

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
- Added: [#5640](https://github.com/ethereum/aleth/pull/5640) Istanbul support: EIP-1702 Generalized Account Versioning Scheme.
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
- Removed: [#5631](https://github.com/ethereum/aleth/pull/5631) Removed PARANOID build option.
- Fixed: [#5562](https://github.com/ethereum/aleth/pull/5562) Don't send header request messages to peers that haven't sent us Status yet.
- Fixed: [#5581](https://github.com/ethereum/aleth/pull/5581) Fixed finding neighbour nodes in Discovery.
- Fixed: [#5599](https://github.com/ethereum/aleth/pull/5600) Prevent aleth from attempting concurrent connection to node which results in disconnect of original connection.
- Fixed: [#5609](https://github.com/ethereum/aleth/pull/5609) Log valid local enode-address when external IP is not known.
- Fixed: [#5627](https://github.com/ethereum/aleth/pull/5627) Correct testeth --help log output indentation.
- Fixed: [#5644](https://github.com/ethereum/aleth/pull/5644) Avoid attempting to sync with disconnected peers.
- Fixed: [#5647](https://github.com/ethereum/aleth/pull/5647) test_importRawBlock RPC method correctly fails in case of import failure.


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


[1.7.0]: https://github.com/ethereum/aleth/compare/v1.6.0-alpha.0...master
[1.6.0]: https://github.com/ethereum/aleth/releases/tag/v1.6.0
