Creating a private network and deploying a contract with Remix
===============

???

Initializing aleth in the Command-Line Interface
------

**1.** Create a new aleth-account.
::
  aleth/aleth account new
  
You are then asked to enter and confirm a password for this account.

**2.** Create a JSON file (e.g. ``config.json``). Its content should look roughly like this: 
::
  {
    "sealEngine": "Ethash",
    "params": {
      "accountStartNonce": "0x00",
      "maximumExtraDataSize": "0x20",
      "homesteadForkBlock": "0x00",
      "daoHardforkBlock": "0x00",
      "EIP150ForkBlock": "0x00",
      "EIP158ForkBlock": "0x00",
      "byzantiumForkBlock": "0x00",
      "constantinopleForkBlock": "0x00",
      "minGasLimit": "0x5208",
      "maxGasLimit": "0x7fffffffffffffff",
      "tieBreakingGas": false,
      "gasLimitBoundDivisor": "0x0400",
      "minimumDifficulty": "0x100000",
      "difficultyBoundDivisor": "0x0800",
      "durationLimit": "0x0d",
      "blockReward": "0x4563918244F40000",
      "networkID" : "0x42",
      "chainID": "0x42",
      "allowFutureBlocks" : false
    },
    "genesis": {
      "nonce": "0x0000000000000042",
      "difficulty": "0x100000",
      "mixHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
      "author": "0x0000000000000000000000000000000000000000",
      "timestamp": "0x5A5B92D7",
      "parentHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
      "extraData": "0x655741534d2074657374206e6574776f726b2030",
      "gasLimit": "0x989680"
    },
    "accounts": {
      "0000000000000000000000000000000000000001": { "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 } }, "balance": "0x01" },
      "0000000000000000000000000000000000000002": { "precompiled": { "name": "sha256", "linear": { "base": 60, "word": 12 } }, "balance": "0x01" },
      "0000000000000000000000000000000000000003": { "precompiled": { "name": "ripemd160", "linear": { "base": 600, "word": 120 } }, "balance": "0x01" },
      "0000000000000000000000000000000000000004": { "precompiled": { "name": "identity", "linear": { "base": 15, "word": 3 } }, "balance": "0x01" },
      "0000000000000000000000000000000000000005": { "precompiled": { "name": "modexp" }, "balance": "0x01" },
      "0000000000000000000000000000000000000006": { "precompiled": { "name": "alt_bn128_G1_add", "linear": { "base": 500, "word": 0 } }, "balance": "0x01" },
      "0000000000000000000000000000000000000007": { "precompiled": { "name": "alt_bn128_G1_mul", "linear": { "base": 40000, "word": 0 } }, "balance": "0x01" },
      "0000000000000000000000000000000000000008": { "precompiled": { "name": "alt_bn128_pairing_product" }, "balance": "0x01" },
      "008a78302c6fe24cc74008c7bdae27b7243a7066" /* <= Enter your account code here */: {
          "balance" : "0x200000000000000000000000000000000000000000000000000000000000000"
      }
    }
  }
  
Save it preferably in your ``Documents`` folder.


**3.** Navigate to ``aleth/build`` and run ``config.json`` with aleth.
::
  aleth/aleth --config /Users/johndoe/Documents/config.json -m on -a 008a78302c6fe24cc74008c7bdae27b7243a7066 --no-discovery --pin --unsafe-transactions

**Note:** The account code used (``008a78302c6fe24cc74008c7bdae27b7243a7066``) has to be the same as in the ``config.json`` file!

**4.** Meanwhile, open a new window in your CLI, navigate into the ``aleth`` directory, and run
::
  scripts/jsonrpcproxy.py

Running it on Remix
------

???
