# PoA Private Chains

**NOTE: This chapter is work in progress.**

TurboEthereum supports Proof-of-Authority (PoA) private chains through the Fluidity core ethereum client `flu`. Proof-of-authority chains utilise a number of secret keys (authorities) to collaborate and create the longest chain instead of the public Ethereum network's proof-of-work scheme (Ethash).

`flu` is configured through a JSON-format file which specifies all of the various settings concerning the blockchain. Here is an example JSON file:

```
{
	"sealEngine": "BasicAuthority",
	"options": {
		"authorities": [
			"0xfa0c706a1410c8785baa7498325cf7461b325583",
			"0x7766d151b2c63cb096f624daa091ccb27a2c693f"
		]
	},
	"params": {
		"accountStartNonce": "0x",
		"maximumExtraDataSize": "0x1000000",
		"blockReward": "0x",
		"registrar": "",
		"networkID" : "0x45"
	},
	"genesis": {
		"author": "0x0000000000000000000000000000000000000000",
		"timestamp": "0x00",
		"parentHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
		"extraData": "0x",
		"gasLimit": "0x1000000000000"
	},
	"accounts": {
		"0000000000000000000000000000000000000001": { "wei": "1", "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 } } },
		"0000000000000000000000000000000000000002": { "wei": "1", "precompiled": { "name": "sha256", "linear": { "base": 60, "word": 12 } } },
		"0000000000000000000000000000000000000003": { "wei": "1", "precompiled": { "name": "ripemd160", "linear": { "base": 600, "word": 120 } } },
		"0000000000000000000000000000000000000004": { "wei": "1", "precompiled": { "name": "identity", "linear": { "base": 15, "word": 3 } } }
	},
	"network": {
		"nodes": [
			"enode://f12709ce6a10fdc76cea6129c6e85e44225d4539ac1e5b26d2cb73f436b9f34c2a1a623ea14a39893b10df2cdc4560e16f9db0aada9ac06b20bc4c5e5dd894c8@127.0.0.1:40401",
			"enode://9bd11ae2cdbdd670dcbd34fa04ff71d840a9fb658d166f4d58192ec8f3d23c07cda490e717d7707e37a4e193ee6cdc8d1ee45320badf3b5476d7a356e6ff9de5@127.0.0.1:40402"
		]
	}
}
```

Breaking it down:

```
"sealEngine": "BasicAuthority",
```

This states that the seal engine of the chain which we wish to use is the `BasicAuthority` seal engine; this creates a proof-of-authority chain (rather than the Ethash proof-of-work chain on the public network).

```
"options": {
	"authorities": [
		"0xfa0c706a1410c8785baa7498325cf7461b325583",
		"0x7766d151b2c63cb096f624daa091ccb27a2c693f"
	]
},
```

This provides a number of options for our `BasicAuthority` seal engine. In this case, we provide the Ethereum addresses of the two accounts which are authorised to sign a new block.

Make sure you own one of these or you'll find it very difficult to append new blocks.

```
"params": {
	"accountStartNonce": "0x",
	"maximumExtraDataSize": "0x1000000",
	"blockReward": "0x",
	"registrar": "",
	"networkID" : "0x45"
},
"genesis": {
	"author": "0x0000000000000000000000000000000000000000",
	"timestamp": "0x00",
	"parentHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
	"extraData": "0x",
	"gasLimit": "0x1000000000000"
},
```

This provides additional parameters for the chain's consensus logic. (The difference between parameters and options is too subtle for this guide.) In this case, we're setting allowing a generous gas limit and extra-data field size and otherwise making everything zero. This is fine for most private chains.

```
"accounts": {
		"0000000000000000000000000000000000000001": { "wei": "1", "precompiled": { "name": "ecrecover", "linear": { "base": 3000, "word": 0 } } },
		"0000000000000000000000000000000000000002": { "wei": "1", "precompiled": { "name": "sha256", "linear": { "base": 60, "word": 12 } } },
		"0000000000000000000000000000000000000003": { "wei": "1", "precompiled": { "name": "ripemd160", "linear": { "base": 600, "word": 120 } } },
		"0000000000000000000000000000000000000004": { "wei": "1", "precompiled": { "name": "identity", "linear": { "base": 15, "word": 3 } } }
},
```

This defines any accounts that are pre-specified in the genesis block. You can specify the balance, nonce, code and storage of each of the accounts. You can also use the powerful precompiled contract system to place precompiled contracts in each of the accounts together with gas-cost rules.

In this case we're placing a token value in each of the pre-compiled contracts accounts, standard practice to avoid the first transactor getting stung for using them. We're also setting them up similarly to the public network, with each of the four algorithms in the first four slots together with the gas costs from the public network.

Further precompiled contract algorithms can be added through creating and linking a library using the macro `ETH_REGISTER_PRECOMPILED`. For example, to create an algorithm which placed a simple byte-wise XOR checksum of the input into the output:

```
ETH_REGISTER_PRECOMPILED(xorchecksum)(bytesConstRef _in, bytesRef _out)
{
    // No point doing any computation if there's nowhere to place the result.
	if (_out.size() >= 1)
	{
        // XOR every byte in the input together into the accumulator acc.
        byte acc = 0;
        for (unsigned i = 0; i < _in.size(); ++i)
            acc ^= _in[i];
        // Place the result into the output.
        _out[0] = acc;
	}
}
```

With this defined, you would be at liberty to name a contract precompiled with:

```
"precompiled": {
    "name": "xorchecksum",
    "linear": { "base": 1, "word": 1 }
}
```

You can select the costs (1 gas plus 1 gas for each 32-byte word in the input) as you choose.


```
"network": {
	"nodes": [
		"enode://f12709ce6a10fdc76cea6129c6e85e44225d4539ac1e5b26d2cb73f436b9f34c2a1a623ea14a39893b10df2cdc4560e16f9db0aada9ac06b20bc4c5e5dd894c8@127.0.0.1:40401",
		"enode://9bd11ae2cdbdd670dcbd34fa04ff71d840a9fb658d166f4d58192ec8f3d23c07cda490e717d7707e37a4e193ee6cdc8d1ee45320badf3b5476d7a356e6ff9de5@0.0.0.0:0"
	]
}
```

Finally we set up the network. In this case we define which node IDs are allowed to connect (and be connected to) and give some IPs/ports for them. Note even if the IPs/ports are not known, it is important that all node IDs on the private network are listed. Listing IDs with an invalid IP/port, as in the second entry, is fine.

# Setting up a PoA Private Network

To set up your PoA private network, first determine the network ID of each node which will be sitting on it. Author a JSON file similar to that above but with the `network.nodes` array populated with those IDs (with, wherever possible, the right IPs/ports as this will make bootstrapping easier).

Finding the node ID is easy with `flu`: when it is run simply look out for a line beginning:

```
Node ID: enode://...
```

Determine, of those nodes, which will be able to sign new blocks. For each of them make sure you have at least one of the keys listed in your JSON file's `options.authorities` section.

Running the `flu` client on a fresh machine (or with a fresh `--path` directory) will result in a keystore with a single, freshly generated, account key.

You can find out what it is by looking for a line beginning:

```
Created key: 0x...
```

The client (perhaps on later invocations) can use this account to sign blocks if it is included in the `options.authorities` list.

You may author the rest of the JSON file as you choose, perhaps altering the `extraData` genesis field to customise the chain and avoid it being confused with other, older or newer chains.

When ready, you can start one or more nodes with this file (let's assume it is called `myconfig.json` and that you've copied it into each machine's `~` path:

```
$ flu console --config ~/myconfig.json
```

The `console` specifies that we want to have a Javascript console for interacting with our node (omit it for non-interactive mode) and the `--config` specifies our JSON configuration file.

To begin sealing, use `web3.admin.eth.setMining(true)` at the console. All of the rest of the JS API is available for you.

There are a number of options to help you configure things to your particular situation:

- `--path` Alter the path of the blockchain/state databases and the keys/secrets database. Defaults to `~/.fluidity`.
- `--master` Specify the password which is used to encrypt/decrypt the keys database and the default secret.
- `--client-name` Specify this particular node's name.
- `--public-ip` Specify the public IP of this node (i.e. the one that we advertise).
- `--listen-ip` Specify the IP that we are actually listening on (e.g. the local adaptor IP).
- `--listen-port` Specify the port which we should listen on.
- `--start-sealing` Start sealing blocks immediately.
- `--jsonrpc` Enable JSON-RPC server on port 8545.
- `--jsonrpc-port` Enable JSON-RPC server on the given port.
- `--jsonrpc-cors-domain` Configure the JSON-RPC server's CORS domain.

By enabling the JSON-RPC server you can use e.g. `ethconsole` to connect to and interact with the private chain.



