# Changelog

## [1.7.0] - Unreleased

- Added: [#5537](https://github.com/ethereum/aleth/pull/5537) Creating Ethereum Node Record (ENR) at program start.
- Added: [#5557](https://github.com/ethereum/aleth/pull/5557) Improved debug logging of full sync.
- Added: [#5564](https://github.com/ethereum/aleth/pull/5564) Improved help output of Aleth by adding list of channels.
- Added: [#5575](https://github.com/ethereum/aleth/pull/5575) Log active peer count and peer list every 30 seconds.
- Added: [#5580](https://github.com/ethereum/aleth/pull/5580) Enable syncing from ETC nodes for blocks < dao hard fork block.
- Changed: [#5559](https://github.com/ethereum/aleth/pull/5559) Update peer validation error messages.
- Changed: [#5568](https://github.com/ethereum/aleth/pull/5568) Improve rlpx handshake log messages and create new rlpx log channel.
- Changed: [#5576](https://github.com/ethereum/aleth/pull/5576) Moved sstore_combinations and static_Call50000_sha256 tests to stTimeConsuming test suite. (testeth runs them only with `--all` flag)
- Fixed: [#5562](https://github.com/ethereum/aleth/pull/5562) Don't send header request messages to peers that haven't sent us Status yet.
- Fixed: [#5581](https://github.com/ethereum/aleth/pull/5581) Fixed finding neighbour nodes in Discovery.

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
