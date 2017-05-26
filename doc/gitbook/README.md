TurboEthereum Guide
======= 

This book is intended as a practical user guide for the "Turbo" Ethereum software distribution, originally named after the language in which it is written, C++.

TurboEthereum is a large distribution of software including a number of diverse tools. This book begins with the installation instructions, before proceeding to introductions, walk-throughs and references for the various tools that make up TurboEthereum. 

The full software suite of TurboEthereum includes:

- **eth** (`eth`) The mainline CLI Ethereum client. Run it in the background and it will connect to the Ethereum network; you can mine, make transactions and inspect the blockchain.
- `ethkey` A key/wallet management tool for Ethereum keys. This lets you add, remove and change your keys as well as *cold wallet device*-friendly transaction inspection and signing.
- `ethminer` A standalone miner. This can be used to check how fast you can mine and will mine for you in concert with `eth`, `geth` and `pyethereum`.
- `ethvm` The Ethereum virtual machine emulator. You can use this to run EVM code.
- `rlp` An serialisation/deserialisation tool for the Recursive Length Prefix format.
