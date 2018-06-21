cpp-ethereum guide
======= 

This book is intended as a practical user guide for the cpp-ethereum software distribution.

cpp-ethereum is a distribution of software including a number of diverse tools. This book begins with the installation instructions, before proceeding to introductions, walk-throughs and references for the various tools that make up cpp-ethereum.

The full software suite of cpp-ethereum includes:

- **aleth** (`aleth`) The mainline CLI Ethereum client. Run it in the background and it will connect to the Ethereum network; you can mine, make transactions and inspect the blockchain.
- `aleth-key` A key/wallet management tool for Ethereum keys. This lets you add, remove and change your keys as well as *cold wallet device*-friendly transaction inspection and signing.
- `ethminer` A standalone miner. This can be used to check how fast you can mine and will mine for you in concert with `eth`, `geth` and `pyethereum`.
- `rlp` An serialisation/deserialisation tool for the Recursive Length Prefix format.
