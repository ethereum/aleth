# Changelog

## [1.6.0] - Unreleased

- Added: [#5485](https://github.com/ethereum/aleth/pull/5485) aleth-bootnode now by default connects to official Ethereum bootnodes. This can be disabled with `--no-bootstrap` flag.
- Added: [#5505](https://github.com/ethereum/aleth/pull/5505) Allow building with libc++ on Linux.
- Added: [#5514](https://github.com/ethereum/aleth/pull/5514) Improved logging in case of RPC method failures.
- Added: [#5526](https://github.com/ethereum/aleth/pull/5526) Improved logging when loading chain config json containing syntax error.
- Changed: [#5464](https://github.com/ethereum/aleth/pull/5464) Upgrade OS and compilers in the docker image for tests.
- Removed: [#5538](https://github.com/ethereum/aleth/pull/5538) Removed --private flag from aleth command-line arguments.
- Fixed: [#5483](https://github.com/ethereum/aleth/pull/5483) Don't ping the same node more than once in a row; also fixes the assertion failure.
- Fixed: [#5512](https://github.com/ethereum/aleth/pull/5512) Calling `eth_call` without `from` argument. 
- Fixed: [#5502](https://github.com/ethereum/aleth/pull/5502) Fix Discovery terminating prematurely because of race condition.
- Fixed: [#5452](https://github.com/ethereum/aleth/pull/5452) Correctly handle Discovery messages when the peer changes public key.
- Fixed: [#5519](https://github.com/ethereum/aleth/pull/5519) Correctly handle Discovery messages with known public key but unknown endpoint.
- Fixed: [#5523](https://github.com/ethereum/aleth/pull/5523) [#5533](https://github.com/ethereum/aleth/pull/5533) Fix syncing terminating prematurely because of race condition.
- Fixed: [#5539](https://github.com/ethereum/aleth/pull/5539) Fix logic for determining if dao hard fork block header should be requested

[1.6.0]: https://github.com/ethereum/aleth/compare/v1.6.0-alpha.1...master