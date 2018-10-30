Database Layout
===============

Aleth uses three databases, all of them are essentially just
key-value storages (MemoryDB, LevelDB, or RocksDB is used depending on
the runtime settings). Their physical disk locations are as follows (note
that this doesn't apply to MemoryDB):

-  Blocks - ``{ETH_DATABASE_DIR}/{GENESIS_HASH}/blocks``
-  Extras -
   ``{ETH_DATABASE_DIR}/{GENESIS_HASH}/{DATABASE_VERSION}/extras``
-  State -
   ``{ETH_DATABASE_DIR}/{GENESIS_HASH}/{DATABASE_VERSION}/state``

where

``{ETH_DATABASE_DIR}`` - base Aleth database directory
``{GENESIS_HASH}`` - hex representation of first 4 bytes of genesis
block hash (``d4e56740`` for main net, ``41941023`` for Ropsten)
``{DATABASE_VERSION}`` - encoded current version of the database layout
(``12041`` as of the time of this writing)

Blocks
------

The blockchain storage, the only thing stored here is binary
(RLP-encoded) data of the blocks.

Every record is:

::

    blockHash => blockData

Low-level access to both Blocks and Extras databases is encapsulated in
`BlockChain
class <https://github.com/ethereum/aleth/blob/master/libethereum/BlockChain.h>`__.

Extras
------

Additional data to support efficient queries to the blockchain data.

To distinguish between the types of records, a byte-long constant is
concatenated to the keys. ``+`` in the following description means this
concatenation.

-  For each block stored in Blocks DB, Extras has the following records:

::

    blockHash + ExtraDetails => rlp(number, totalDiffiulty, parentHash, rlp(childrenHashes)) // ExtraDetails = 0
    blockHash + ExtraLogBlooms => rlp(blockLogBlooms) // ExtraLogBlooms = 3
    blockHash + ExtraReceipts => rlp(receipts) // ExtraReceipts = 4
    blockNumber + ExtraBlockHash => blockHash // ExtraBlockHash = 1

-  For each transaction in the blockchain Extras has the following
   records:

   ::

       transactionHash + ExtraTransactionAddress => rlp(blockHash, transactionIndex) // ExtraTransactionAddress = 2

-  Records storing log blooms for a number of blocks at once have the
   form:

   ::

       chunkId + ExtraBlocksBlooms => blooms // ExtraBlocksBlooms = 5

   where ``chunkId = index * 255 + level``. See comment to
   `BlockChain::blocksBlooms()
   method <https://github.com/ethereum/cpp-ethereum/blob/db7278413edf701901d2a054b32a31c2722708d5/libethereum/BlockChain.h#L193-L206>`__
   for details.

-  Additional records, one instance of each:

   ::

       "best" => lastBlockHash // best block of the canonical chain
       "chainStart" => firstBlockHash // used when we don't have the full chain, for example after snapshot import

State
-----

The data representing the full Ethereum state (i.e. all the accounts).
The State data forms a `Merkle Patricia
Trie <https://github.com/ethereum/wiki/wiki/Patricia-Tree>`__ and the
database stores the nodes of this trie.

-  Nodes of the trie for the mapping ``sha3(address) => accountData``,
   where according to Yellow Paper
   ``accountData = rlp(nonce, balance, storageRoot, codeHash)``.
-  For each account with non-empty storage there is a storage trie with
   nodes for the mapping ``sha3(key) => value``.
-  For each account with non-empty code, it is stored separately out of
   the tries: ``sha3(code) => code``.
-  For each key of all the tries above the mapping of sha3 hash to its
   preimage (address or storage key) is stored:
   ``hash + 255 => preimage`` (``+`` is concatenation).

For the code managing the state see `State
class <https://github.com/ethereum/aleth/blob/master/libethereum/State.h>`__
(also note free function ``commit`` there). Merkle Patricia Trie
implemenation is in
`TrieDB.h <https://github.com/ethereum/aleth/blob/master/libdevcore/TrieDB.h>`__.
For lower-level code accessing the database itself see
`OverlayDB <https://github.com/ethereum/aleth/blob/master/libdevcore/OverlayDB.h>`__
and
`StateCacheDB` <https://github.com/ethereum/aleth/blob/master/libdevcore/StateCacheB.h>`__.
