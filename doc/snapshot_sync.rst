Using snapshot sync
===============

Snapshot sync feature is still work in progress and currently is not very user-friendly yet. These are the instructions to make it work with the main net as of January 2018.

If you have Parity installed, alternative to the `Downloading snapshot` steps below is to get it from the directory like ``Parity/Ethereum/chains/ethereum/db/906a34e69aec8c0d/snapshot``

Downloading the snapshot
------

1. Start downloading providing the path to the directory to save the snapshot into
::
  aleth --download-snapshot SNAPSHOT_DIR -v 9

If it doesn't find the peer to download from, you can also provide the address of a running node with main net snapshot (can be a Parity node or an aleth node with imported snapshot)
::
  aleth --download-snapshot SNAPSHOT_DIR -v 9 --peerset required:493d52068ec72230688da539926f47a452b762bc348d2ab1491f399b532186d71d7c512e09ffb8e9c24d292d064c00f6234ef1221bc0d86093d2de32358d33da@52.169.85.130:30303 --pin --no-discovery

The one above is an Aleth node, if it doesn't work try one of the Parity boot nodes from https://github.com/paritytech/parity/blob/master/ethcore/res/ethereum/foundation.json#L176

2. Exit (Ctrl-C) when it says ``Snapshot download complete!``

Importing the snapshot
------

1. Start importing the snapshot providing the path to the snapshot directory
::
  aleth --import-snapshot SNAPSHOT_DIR -v 8

2. After import is complete, it will automatically switch to regular sync process to get the blocks between snapshot and the current chain head.
